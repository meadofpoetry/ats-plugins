#include <glib.h>
#include <gst/gl/gl.h>

struct Accumulator {
        GLfloat frozen;
        GLfloat black;
        GLfloat bright;
        GLfloat diff;
        GLint   visible;
};

static const gchar *shader_source =
        "#version 430\n"
        "#extension GL_ARB_compute_shader : enable\n"
        "#extension GL_ARB_shader_image_load_store : enable\n"
        "#extension GL_ARB_shader_storage_buffer_object : enable\n"
        "\n"
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "\n"
        "#define WHT_LVL 0.90196078\n"
        "// 210\n"
        "#define BLK_LVL 0.15625\n"
        "// 40\n"
        "#define WHT_DIFF 0.0234375\n"
        "// 6\n"
        "#define GRH_DIFF 0.0078125\n"
        "// 2\n"
        "#define KNORM 4.0\n"
        "#define L_DIFF 5\n"
        "\n"
        "#define BLOCK_SIZE 8\n"
        "\n"
        "#define BLOCKY 0\n"
        "#define LUMA 1\n"
        "#define BLACK 2\n"
        "#define DIFF 3\n"
        "#define FREEZE 4\n"
        "\n"
        "struct Accumulator {\n"
        "        float frozen;\n"
        "        float black;\n"
        "        float bright;\n"
        "        float diff;\n"
        "        int   visible;\n"
        "};\n"
        "\n"
        "layout (r8, binding = 0) uniform image2D tex;\n"
        "layout (r8, binding = 1) uniform image2D tex_prev;\n"
        "uniform int width;\n"
        "uniform int height;\n"
        "uniform int stride;\n"
        "uniform int black_bound;\n"
        "uniform int freez_bound;\n"
        "\n"
        "layout (std430, binding=10) buffer Interm {\n"
        "         Accumulator noize_data [];\n"
        "};\n"
        "\n"
        "//const uint wht_coef[20] = {8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 11, 11, 12, 14, 17, 27};\n"
        "//const uint ght_coef[20] = {4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 7, 7, 8, 10, 13, 23};\n"
        "const uint wht_coef[20] = {10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 13, 13, 14, 16, 19, 29};	       \n"
        "const uint ght_coef[20] = {6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 9, 9, 10, 12, 15, 25};\n"
        "//const uint wht_coef[20] = {6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 9, 9, 10, 12, 15, 25};\n"
        "//const uint ght_coef[20] = {2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 5, 5, 6, 8, 11, 21};\n"
        "\n"
        "layout (local_size_x = 1, local_size_y = 1) in;\n"
        "\n"
        "float get_coef(float noize, uint array[20]) {\n"
        "        uint ret_val;                                     \n"
        "        if((noize>100) || (noize<0))\n"
        "                ret_val = 0;                              \n"
        "        else                                              \n"
        "                ret_val = array[uint(noize/5)];                 \n"
        "        return float(ret_val/255.0);\n"
        "}\n"
        "       \n"
        "\n"
        "float compute_noize (ivec2 pos) {\n"
        "        float lvl;\n"
        "        float pix       = imageLoad(tex, pos).r;\n"
        "        float pix_right = imageLoad(tex, ivec2(pos.x + 1, pos.y)).r;\n"
        "        float pix_down  = imageLoad(tex, ivec2(pos.x, pos.y + 1)).r;\n"
        "        /* Noize */\n"
        "        float res = 0.0;\n"
        "        if ((pix < WHT_LVL) && (pix > BLK_LVL)) {\n"
        "                lvl = GRH_DIFF;\n"
        "        } else {\n"
        "                lvl = WHT_DIFF;\n"
        "        }\n"
        "        if (abs(pix - pix_right) >= lvl) {\n"
        "                res += 1.0/(8.0*8.0*2.0);\n"
        "        }\n"
        "        if (abs(pix - pix_down) >= lvl) {\n"
        "                res += 1.0/(8.0*8.0*2.0);\n"
        "        }\n"
        "        return res;\n"
        "}\n"
        "\n"
        "void main() {\n"
        "        uint block_pos = (gl_GlobalInvocationID.y * (width / BLOCK_SIZE)) + gl_GlobalInvocationID.x ;\n"
        "        /* Block init */\n"
        "        float noize = 0.0;\n"
        "        /* l r u d */\n"
        "        vec4 noize_v = vec4(0);\n"
        "        \n"
        "        noize_data[block_pos].black = 0.0;\n"
        "        noize_data[block_pos].frozen = 0.0;\n"
        "        noize_data[block_pos].bright = 0.0;\n"
        "        noize_data[block_pos].diff = 0.0;\n"
        "        noize_data[block_pos].visible = 0;\n"
        "        \n"
        "        for (int i = 0; i < BLOCK_SIZE; i++) {\n"
        "                for (int j = 0; j < BLOCK_SIZE; j++) {\n"
        "                        float diff_loc;\n"
        "                        ivec2 pix_pos = ivec2(gl_GlobalInvocationID.x * BLOCK_SIZE + i,\n"
        "                                              gl_GlobalInvocationID.y * BLOCK_SIZE + j);\n"
        "                        float pix = imageLoad(tex, pix_pos).r;\n"
        "                        /* Noize */\n"
        "                        noize += compute_noize(pix_pos);\n"
        "                        if (gl_GlobalInvocationID.x > 0)\n"
        "                                noize_v[0] += compute_noize(ivec2(pix_pos.x - BLOCK_SIZE, pix_pos.y));\n"
        "                        if (gl_GlobalInvocationID.y > 0)\n"
        "                                noize_v[3] += compute_noize(ivec2(pix_pos.x, pix_pos.y - BLOCK_SIZE));\n"
        "                        if (gl_GlobalInvocationID.x < (width / BLOCK_SIZE))\n"
        "                                noize_v[1] += compute_noize(ivec2(pix_pos.x + BLOCK_SIZE, pix_pos.y));\n"
        "                        if (gl_GlobalInvocationID.x < (height / BLOCK_SIZE))\n"
        "                                noize_v[2] += compute_noize(ivec2(pix_pos.x, pix_pos.y + BLOCK_SIZE));\n"
        "                        /* Brightness */\n"
        "                        noize_data[block_pos].bright += float(pix);\n"
        "                        /* Black */\n"
        "                        if (pix <= float(black_bound / 255.0)) {\n"
        "                                noize_data[block_pos].black += 1.0;\n"
        "                        }\n"
        "                        /* Diff */\n"
        "                        diff_loc = abs(pix - imageLoad(tex_prev, pix_pos).r);\n"
        "                        noize_data[block_pos].diff += diff_loc;\n"
        "                        /* Frozen */\n"
        "                        if (diff_loc <= float(freez_bound / 255.0)) {\n"
        "                                noize_data[block_pos].frozen += 1.0;\n"
        "                        }\n"
        "                }\n"
        "        }\n"
        "        /* Noize coeffs */\n"
        "        noize_v = 100.0 * vec4(max(noize_v[0], noize),\n"
        "                               max(noize_v[1], noize),\n"
        "                               max(noize_v[2], noize),\n"
        "                               max(noize_v[3], noize));\n"
        "        vec4 white_v = vec4(get_coef(noize_v[0], wht_coef),\n"
        "                            get_coef(noize_v[1], wht_coef),\n"
        "                            get_coef(noize_v[2], wht_coef),\n"
        "                            get_coef(noize_v[3], wht_coef));\n"
        "        vec4 grey_v = vec4(get_coef(noize_v[0], ght_coef),\n"
        "                           get_coef(noize_v[1], ght_coef),\n"
        "                           get_coef(noize_v[2], ght_coef),\n"
        "                           get_coef(noize_v[3], ght_coef));\n"
        "        /* compute borders */\n"
        "        if (gl_GlobalInvocationID.x == 0\n"
        "            || gl_GlobalInvocationID.x >= (width / BLOCK_SIZE)\n"
        "            || gl_GlobalInvocationID.y == 0\n"
        "            || gl_GlobalInvocationID.y >= (height / BLOCK_SIZE))\n"
        "                return;\n"
        "        ivec4 vis = {0,0,0,0};\n"
        "        for (int i = 0; i < BLOCK_SIZE; i++) {\n"
        "                /* l r u d */\n"
        "                ivec2 zero = ivec2(gl_GlobalInvocationID.x * BLOCK_SIZE,\n"
        "                                   gl_GlobalInvocationID.y * BLOCK_SIZE);\n"
        "                vec4 pixel = vec4(imageLoad(tex, ivec2(zero.x, zero.y + i)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x+BLOCK_SIZE-1, zero.y + i)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x + i, zero.y+BLOCK_SIZE-1)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x + i, zero.y)).r );\n"
        "                vec4 prev  = vec4(imageLoad(tex, ivec2(zero.x + 1, zero.y + i)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x+BLOCK_SIZE-2, zero.y + i)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x + i, zero.y+BLOCK_SIZE-2)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x + i, zero.y + 1)).r );\n"
        "                vec4 next  = vec4(imageLoad(tex, ivec2(zero.x - 1, zero.y + i)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x+BLOCK_SIZE, zero.y + i)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x + i, zero.y+BLOCK_SIZE)).r,\n"
        "                                  imageLoad(tex, ivec2(zero.x + i, zero.y - 1)).r );\n"
        "                vec4 next_next  = vec4(imageLoad(tex, ivec2(zero.x - 2, zero.y + i)).r,\n"
        "                                       imageLoad(tex, ivec2(zero.x+BLOCK_SIZE+1, zero.y + i)).r,\n"
        "                                       imageLoad(tex, ivec2(zero.x + i, zero.y+BLOCK_SIZE+1)).r,\n"
        "                                       imageLoad(tex, ivec2(zero.x + i, zero.y - 2)).r );\n"
        "                vec4 coef = vec4((pixel[0] < WHT_LVL) && (pixel[0] > BLK_LVL) ?\n"
        "                                 grey_v[0] : white_v[0],\n"
        "                                 (pixel[1] < WHT_LVL) && (pixel[1] > BLK_LVL) ?\n"
        "                                 grey_v[1] : white_v[1],\n"
        "                                 (pixel[2] < WHT_LVL) && (pixel[2] > BLK_LVL) ?\n"
        "                                 grey_v[2] : white_v[2],\n"
        "                                 (pixel[3] < WHT_LVL) && (pixel[3] > BLK_LVL) ?\n"
        "                                 grey_v[3] : white_v[3]);\n"
        "                vec4 denom = round( (abs(prev-pixel) + abs(next-next_next)) / KNORM);\n"
        "                denom = vec4(denom[0] == 0.0 ? 1.0 : denom[0],\n"
        "                             denom[1] == 0.0 ? 1.0 : denom[1],\n"
        "                             denom[2] == 0.0 ? 1.0 : denom[2],\n"
        "                             denom[3] == 0.0 ? 1.0 : denom[3]);\n"
        "                vec4 norm = abs(next-pixel) / denom;\n"
        "                vis += ivec4( norm[0] > coef[0] ? 1 : 0,\n"
        "                              norm[1] > coef[1] ? 1 : 0,\n"
        "                              norm[2] > coef[2] ? 1 : 0,\n"
        "                              norm[3] > coef[3] ? 1 : 0 );\n"
        "        }\n"
        "        /* counting visible blocks */\n"
        "        int loc_counter = 0;\n"
        "        for (int side = 0; side < 4; side++) {\n"
        "                if (vis[side] > L_DIFF)\n"
        "                        loc_counter += 1;\n"
        "        }\n"
        "        if (loc_counter >= 2)\n"
        "                noize_data[block_pos].visible = 1;\n"
        "}\n";


static const gchar *shader_aux_source =
        "#version 430\n"
        "#extension GL_ARB_compute_shader : enable\n"
        "#extension GL_ARB_shader_image_load_store : enable\n"
        "#extension GL_ARB_shader_storage_buffer_object : enable\n"
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "#define BLOCKY 0\n"
        "#define LUMA 1\n"
        "#define BLACK 2\n"
        "#define DIFF 3\n"
        "#define FREEZE 4\n"
        "struct Accumulator {\n"
        "        float frozen;\n"
        "        float black;\n"
        "        float bright;\n"
        "        float diff;\n"
        "        int   visible;\n"
        "};\n"
        "uniform int width;\n"
        "uniform int height;\n"
        "layout (std430, binding=10) buffer Interm {\n"
        "         Accumulator noize_data [];\n"
        "};\n"
        "layout (std430, binding=11) buffer Result {\n"
        "         float data [5];\n"
        "};\n"
        "layout (local_size_x = 1, local_size_y = 1) in;\n"
        "void main() {\n"
        "        if (gl_GlobalInvocationID.xy != vec2(0,0))\n"
        "                return;\n"
        "        \n"
        "        float bright = 0.0;\n"
        "        float diff = 0.0;\n"
        "        float frozen = 0.0;\n"
        "        float black  = 0.0;\n"
        "        float blocky = 0.0;  \n"
        "        for (int i = 0; i < (height * width / 64); i++) {\n"
        "                bright += float(noize_data[i].bright);\n"
        "                diff   += noize_data[i].diff;\n"
        "                frozen += noize_data[i].frozen;\n"
        "                black  += noize_data[i].black;\n"
        "                if (noize_data[i].visible == 1)\n"
        "                        blocky += 1.0;\n"
        "        }\n"
        "        data[LUMA] = 256.0 * bright / float(height * width);\n"
        "        data[DIFF] = 100.0 * diff / float(height * width);\n"
        "        data[BLACK] = 100.0 * black / float(height * width);\n"
        "        data[FREEZE] = 100.0 * frozen / float(height * width);\n"
        "        data[0] = 100.0 * blocky / float(height * width / 64);\n"
        "}";

// * *INDENT-ON* */
