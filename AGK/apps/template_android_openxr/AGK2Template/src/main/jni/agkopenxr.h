#include <unistd.h>
#include <jni.h>
#include <errno.h>
#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/configuration.h>
#include <android/window.h>

#include <stdlib.h>
#include <string.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN,  "native-activity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "native-activity", __VA_ARGS__))

#ifdef __cplusplus
extern "C"
{
#endif

    struct engine
    {
        struct android_app* app;

        ASensorManager* sensorManager;
        const ASensor* accelerometerSensor;
        const ASensor* gyroSensor;
        const ASensor* proximitySensor;
        const ASensor* lightSensor;
        const ASensor* magneticSensor;
        const ASensor* rotVectorSensor;
        ASensorEventQueue* sensorEventQueue;

        int animating;
        int32_t width;
        int32_t height;

        // From OpenXR
        ANativeWindow *nativeWindow;
        bool resumed;
    };

#ifdef __cplusplus
}
#endif

struct initdata
{
    struct ANativeActivity* activity;
    ANativeWindow* window;
};

enum eAppStatus
{
    Stage_1_Not_Init_Status      = 0, // Must be run in order for OpenXR and AGK to work together...
    Stage_2_OpenXR_Init_Status   = 1,
    Stage_3_AGK_Active_Status    = 2,
    Stage_4_OpenXR_Active_Status = 3,
    Shutdown_Status              = 100,
    Failed_Status                = 101,
};

enum eAppMode
{
    Not_Active_Mode      = -2,
    Go_Mode              = -1,
    Paused_Mode          =  0,
    Lost_Focus_Mode      =  1,
};

enum eInitialised
{
    Not_Initialised            = 0,
    Window_Initialised         = 1,
};

#ifdef __cplusplus
extern "C"
{
#endif

void setopenxrstatus(enum eAppStatus Status);
enum eAppStatus getopenxrstatus();
void setopenxrmode(enum eAppMode Mode);
enum eAppMode getopenxrmode();
void setwindowinitialised(enum eInitialised Initialised);
enum eInitialised getwindowinitialised();
void initopenxr_c(struct engine *engine);
void loopopenxr_c();
void endopenxr_c();
void engine_draw_frame(struct engine* engine);

#ifdef __cplusplus
}
#endif



