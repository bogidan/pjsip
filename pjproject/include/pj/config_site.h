// Default Location
//  pjproject-2.3/pjlib/include/pj/


#define PJMEDIA_HAS_VIDEO               0
#define PJMEDIA_HAS_OPENH264_CODEC      0
#define PJMEDIA_HAS_LIBYUV              0
#define PJMEDIA_VIDEO_DEV_HAS_SDL       0
#define PJMEDIA_VIDEO_DEV_HAS_DSHOW     0


#define PJMEDIA_AUDIO_DEV_HAS_WMME 0
//#define PJ_CONFIG_WIN32_WMME_DIRECT
//#define PJ_CONFIG_MINIMAL_SIZE
//#include <pj/config_site_sample.h>

#ifdef DEBUG
#define PJ_LOG_MAX_LEVEL 4
#else
#define PJ_LOG_MAX_LEVEL 0
#endif
#define PJMEDIA_HAS_ALAW_ULAW_TABLE 0

// Disable Codecs
//  http://www.pjsip.org/pjmedia/docs/html/group__pjmedia__codec__config.htm
#define PJMEDIA_HAS_L16_CODEC		0
#define PJMEDIA_HAS_ILBC_CODEC		0
#define PJMEDIA_HAS_GSM_CODEC       0
#define PJMEDIA_HAS_SPEEX_CODEC     0
#define PJMEDIA_HAS_G722_CODEC      0
//#define PJSUA_DEFAULT_CODEC_QUALITY 4

// Increase Buffer Sizes
//#define PJMEDIA_SOUND_BUFFER_COUNT 16

// Resample Settings
//#define PJMEDIA_RESAMPLE_IMP PJMEDIA_RESAMPLE_NONE
//#define PJMEDIA_RESAMPLE_IMP PJMEDIA_RESAMPLE_LIBRESAMPLE

//#define PJSUA_DEFAULT_CLOCK_RATE        16000
#define PJSUA_DEFAULT_EC_TAIL_LEN           0
//#define PJSUA_DEFAULT_AUDIO_FRAME_PTIME    20

//#define PJMEDIA_CONF_USE_SWITCH_BOARD       1