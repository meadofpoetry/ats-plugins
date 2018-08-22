/*
 * TODO copyright
 */

#ifndef _GSTVIDEOANALYSIS_
#define _GSTVIDEOANALYSIS_

#include <gst/gl/gstglfilter.h>

G_BEGIN_DECLS

#define GST_TYPE_VIDEOANALYSIS                  \
        (gst_videoanalysis_get_type())
#define GST_VIDEOANALYSIS(obj)                                          \
        (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VIDEOANALYSIS,GstVideoAnalysis))
#define GST_VIDEOANALYSIS_CLASS(klass)                                  \
        (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VIDEOANALYSIS,GstVideoAnalysisClass))
#define GST_IS_VIDEOANALYSIS(obj)                                       \
        (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VIDEOANALYSIS))
#define GST_IS_VIDEOANALYSIS_CLASS(obj)                                 \
        (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VIDEOANALYSIS))

typedef struct _GstVideoAnalysis GstVideoAnalysis;
typedef struct _GstVideoAnalysisClass GstVideoAnalysisClass;

struct _GstVideoAnalysis
{
        GstGLFilter filter;

        GstGLFilterRenderFunc     analysis_func;
        GstBuffer   *             prev_buffer;
        GstGLMemory *             prev_tex;
        
        /* public */
        /* guint       period;
        gfloat      loss;
        guint       black_pixel_lb;
        guint       pixel_diff_lb;
        BOUNDARY    params_boundary [PARAM_NUMBER]; */
        /* private */
        /* guint       frame;
        guint       frame_limit;
        float       fps_period;
        guint       frames_in_sec;
        gfloat      cont_err_duration [PARAM_NUMBER];
        VideoParams params;
        Error       errors [PARAM_NUMBER];
        guint8      *past_buffer;
        BLOCK       *blocks; */
};

struct _GstVideoAnalysisClass
{
        GstGLFilterClass filter_class;

        /* void (*data_signal) (GstVideoFilter *filter, GstBuffer* d); */
};

GType gst_videoanalysis_get_type (void);


G_END_DECLS

#endif /* _GSTVIDEOANALYSIS_ */
