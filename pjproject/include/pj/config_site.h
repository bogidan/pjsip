// Default Location
//  pjproject-2.3/pjlib/include/pj/

#define PJMEDIA_HAS_VIDEO               0
#define PJMEDIA_HAS_OPENH264_CODEC      0
#define PJMEDIA_HAS_LIBYUV              0
#define PJMEDIA_VIDEO_DEV_HAS_SDL       0
#define PJMEDIA_VIDEO_DEV_HAS_DSHOW     0


#define PJMEDIA_AUDIO_DEV_HAS_WMME 0
//#define PJ_CONFIG_WIN32_WMME_DIRECT
#define PJ_CONFIG_MINIMAL_SIZE
#include <pj/config_site_sample.h>

#define PJ_LOG_MAX_LEVEL 0
#define PJMEDIA_HAS_ALAW_ULAW_TABLE 0
