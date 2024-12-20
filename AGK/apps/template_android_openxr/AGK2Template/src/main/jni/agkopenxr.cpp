// AGKOPENXR.CPP built from learning 
// materials from:  openxr-tutorial.com
//
// Original Work Copyright:
// Copyright 2023, The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0
// OpenXR Tutorial for Khronos Group

//#define _WINDOWS_
#define _ANDROID_

#ifndef _CPP_AGK_OPENXR_
#define _CPP_AGK_OPENXR_

// DEFINE /////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _WINDOWS_
#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_PLATFORM_WIN32
#endif

#ifdef _ANDROID_
#define XR_USE_GRAPHICS_API_OPENGL_ES
#define XR_USE_PLATFORM_ANDROID
#define OS_ANDROID
#define OPENGL_VERSION_MAJOR 3
#define OPENGL_VERSION_MINOR 2
#define GLSL_VERSION "320 es"
#define SPIRV_VERSION "99"
#define USE_SYNC_OBJECT 1  // 0 = GLsync, 1 = EGLSyncKHR, 2 = storage buffer
#endif

//#define _XR_DEBUGGING_

// INCLUDE /////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _WINDOWS_
#include <cmath>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <windows.h>
#include <gl/gl.h>
#include <gl/glext.h>
#include <gl/wglext.h>
#include "openxr/openxr.h"
#include "openxr/openxr_platform.h"
#define M_PI 3.14159265358979323846
#include "agkopenxr.h"
#endif

#ifdef _ANDROID_
#include <string>
#include <sstream>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <iomanip>
#include "agk.h"
#include "agkopenxr.h"
#include <time.h>
#include <unistd.h>
#include <dirent.h>  // for opendir/closedir
#include <pthread.h>
#include <malloc.h>                     // for memalign
#include <dlfcn.h>                      // for dlopen
#include <sys/prctl.h>                  // for prctl( PR_SET_NAME )
#include <sys/stat.h>                   // for gettid
#include <sys/syscall.h>                // for syscall
#include <android/log.h>                // for __android_log_print
#include <android/input.h>              // for AKEYCODE_ etc.
#include <android/window.h>             // for AWINDOW_FLAG_KEEP_SCREEN_ON
#include <android/native_window_jni.h>  // for native window JNI
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES3/gl3platform.h>
#if OPENGL_VERSION_MAJOR == 3
#if OPENGL_VERSION_MINOR == 1
#include <GLES3/gl31.h>
#elif OPENGL_VERSION_MINOR == 2
#include <GLES3/gl32.h>
#endif
#endif
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#endif

// GL FUNCTION POINTERS /////////////////////////////////////////////////////////////////////////////////
#ifdef _WINDOWS_
//PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebuffer = nullptr;
//PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffers = nullptr;
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR = nullptr;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
PFNGLSCISSORINDEXEDPROC glScissorIndexed = nullptr;
PFNGLVIEWPORTINDEXEDFPROC glViewportIndexedf = nullptr;
PFNGLDEPTHRANGEINDEXEDPROC glDepthRangeIndexed = nullptr;
//PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatus = nullptr;
//PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2D = nullptr;
// Extern Function pointers
extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebuffer;
extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffers;
extern PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatus;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2D;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffers;
#endif

#ifdef _ANDROID_
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC   glFramebufferTextureMultiviewOVR = nullptr;
PFNGLSCISSORINDEXEDNVPROC                 glScissorIndexed = nullptr;
PFNGLVIEWPORTINDEXEDFNVPROC               glViewportIndexedf = nullptr;
#endif

// AGK ACCESS //////////////////////////////////////////////////////////////////////////////////////////
#ifdef _WINDOWS_
#endif

#ifdef _ANDROID_
extern "C"
{
    void keyboardmode( int mode );
    void pauseapp();
    void resumeapp(); 
    void onstart(void* ptr);
}
#endif

// XR MESSAGE ///////////////////////////////////////////////////////////////////////////////////////////
#ifdef _WINDOWS_
void XR_MESSAGE(std::string Message)
{    
    Message = Message + "\n";
    std::cout << Message;  // Print the message to the console
    OutputDebugStringA(Message.c_str());
}
#endif

#ifdef _ANDROID_
#define XR_MESSAGE(...) {                                                            \
        std::ostringstream ostr;                                                     \
        ostr<<__VA_ARGS__;                                                           \
        __android_log_write(ANDROID_LOG_DEBUG, "agkopenxr.cpp", ostr.str().c_str()); \
    }
#endif

// APP STATUS ///////////////////////////////////////////////////////////////////////////////////////////
enum eAppStatus eStatus = Stage_1_Not_Init_Status;
void setopenxrstatus(enum eAppStatus Status)
{
    eStatus = Status;
}
eAppStatus getopenxrstatus()        
{
    return eStatus;
}

// APP MODE
enum eAppMode eMode = Not_Active_Mode;
void setopenxrmode(enum eAppMode Mode)
{
    eMode = Mode;
}
eAppMode getopenxrmode()
{
    return eMode;
}

// INITIALISED
enum eInitialised eInit = Not_Initialised;
void setwindowinitialised(enum eInitialised Initialised)
{
    eInit = Initialised;
}
enum eInitialised getwindowinitialised()
{
    return eInit;
}

// MATHS
double DegreesToRadians(double degrees)
{
    return degrees * (double(M_PI) / 180.0);
}
double RadiansToDegrees(double radians)
{
    return radians * (180.0 / double(M_PI));
}
float DegreesToRadians(float degrees)
{
    return degrees * (float(M_PI) / 180.0f);
}
float RadiansToDegrees(float radians)
{
    return radians * (180.0f / float(M_PI));
}
void NormalizeQuaternion(float& qw, float& qx, float& qy, float& qz)
{
    float norm = sqrt(qw * qw + qx * qx + qy * qy + qz * qz);
    if (norm == 0)
    {
        qw = 1; qx = 0; qy = 0; qz = 0;  // Default to identity quaternion to avoid division by zero
    }
    else
    {
        qw /= norm;
        qx /= norm;
        qy /= norm;
        qz /= norm;
    }
}
void QuaternionMultiply(float q1w, float q1x, float q1y, float q1z,	float q2w, float q2x, float q2y, float q2z,	bool NormalOrder, float& rw, float& rx, float& ry, float& rz)
{
	if (NormalOrder)
	{
		// q1 * q2
		rw = q1w * q2w - q1x * q2x - q1y * q2y - q1z * q2z;
		rx = q1w * q2x + q1x * q2w + q1y * q2z - q1z * q2y;
		ry = q1w * q2y - q1x * q2z + q1y * q2w + q1z * q2x;
		rz = q1w * q2z + q1x * q2y - q1y * q2x + q1z * q2w;
	}
	else
	{
		// q2 * q1
		rw = q2w * q1w - q2x * q1x - q2y * q1y - q2z * q1z;
		rx = q2w * q1x + q2x * q1w + q2y * q1z - q2z * q1y;
		ry = q2w * q1y - q2x * q1z + q2y * q1w + q2z * q1x;
		rz = q2w * q1z + q2x * q1y - q2y * q1x + q2z * q1w;
	}

	// Normalize the result to prevent floating-point drift over multiple operations
	NormalizeQuaternion(rw, rx, ry, rz);
}
void RotateVectorByQuaternion(float qw, float qx, float qy, float qz, 
                              float vx, float vy, float vz,
                              float &outX, float &outY, float &outZ)
{
    // Normalize the quaternion to ensure it is a unit quaternion
    NormalizeQuaternion(qw, qx, qy, qz);

    // Quaternion-vector multiplication
    float uvx = qy*vz - qz*vy;
    float uvy = qz*vx - qx*vz;
    float uvz = qx*vy - qy*vx;

    float uuvx = qy*uvz - qz*uvy;
    float uuvy = qz*uvx - qx*uvz;
    float uuvz = qx*uvy - qy*uvx;

    float w2v = qw * 2.0f * (vx*qx + vy*qy + vz*qz);

    outX = vx + uvx * 2.0f * qw + uuvx * 2.0f;
    outY = vy + uvy * 2.0f * qw + uuvy * 2.0f;
    outZ = vz + uvz * 2.0f * qw + uuvz * 2.0f;
}
void RightToLeftCoordinateSystem(float x, float y, float z, float qw, float qx, float qy, float qz,
                                 float &rx, float &ry, float &rz, float &rqw, float &rqx, float &rqy, float &rqz)
{
    // Convert position - Flip the Z-axis
    rx = x;
    ry = y;
    rz = -z;  // Inverting Z for left-handed coordinate system

    // Convert quaternion - Flip W and Z components
    rqw = -qw;
    rqx = qx;
    rqy = qy;
    rqz = -qz;  // Inverting Z for left-handed coordinate system
}
void RightToLeftCoordinateSystem(float x, float y, float z,
                                 float &rx, float &ry, float &rz)
{
    // Convert position - Flip the Z-axis
    rx = x;
    ry = y;
    rz = -z;  // Inverting Z for left-handed coordinate system
}
void RightToLeftCoordinateSystem(float qw, float qx, float qy, float qz,
                                 float &rqw, float &rqx, float &rqy, float &rqz)
{
    // Convert quaternion - Flip W and Z components
    rqw = -qw;
    rqx = qx;
    rqy = qy;
    rqz = -qz;  // Inverting Z for left-handed coordinate system
}
void RotatePointXYZ(double x, double y, double z, double angleDegreesX, double angleDegreesY, double angleDegreesZ,
                    double& xPrime, double& yPrime, double& zPrime)
{
    double radX = DegreesToRadians(angleDegreesX);
    double radY = DegreesToRadians(angleDegreesY);
    double radZ = DegreesToRadians(angleDegreesZ);

    // Rotation around X-axis
    double y1 = y * cos(radX) - z * sin(radX);
    double z1 = y * sin(radX) + z * cos(radX);
    double x1 = x;

    // Rotation around Y-axis
    double z2 = z1 * cos(radY) - x1 * sin(radY);
    double x2 = z1 * sin(radY) + x1 * cos(radY);
    double y2 = y1;

    // Rotation around Z-axis
    xPrime = x2 * cos(radZ) - y2 * sin(radZ);
    yPrime = x2 * sin(radZ) + y2 * cos(radZ);
    zPrime = z2;
}

// AGK and OpenXR
namespace agkopenxr
{
    // Pick Your World Type
    //XrReferenceSpaceType WorldType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    XrReferenceSpaceType WorldType = XR_REFERENCE_SPACE_TYPE_STAGE;
    #ifdef _WINDOWS_
    const int FrameRate           = 90;
    const float Default_Near      = 0.1f;
    const float Default_Far       = 50.0f;
    #endif
    #ifdef _ANDROID_
    const int FrameRate           = 72;
	const float Default_Near      = 0.1f;
    const float Default_Far       = 50.0f;
    #endif
    const float ScreenImageWidth  = 1920;
    const float ScreenImageHeight = 1080;

    #ifdef _ANDROID_
    struct android_app *AndroidApp;
    struct engine *Engine;
    #endif

    class AGKOpenXR
    {
    public:
        AGKOpenXR()
        {
        }
        ~AGKOpenXR() = default;

        const XrPosef m_IdentityPose = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};

        bool         m_ViewUpToDate        = false;
        XrPosef      m_ViewRH              = m_IdentityPose;           // Right-Handed
        XrPosef      m_ViewBuild           = m_IdentityPose;           // Right-Handed // Orientation only changes here...
        XrVector3f   m_ViewBuildDegrees    = m_IdentityPose.position;  // Right-Handed

        bool         m_WorldUpToDate       = false;
        XrPosef      m_World               = m_IdentityPose;           // Left-Handed
        XrVector3f   m_WorldDegrees        = m_IdentityPose.position;  // Left-Handed        
        XrPosef      m_WorldRH             = m_IdentityPose;           // Right-Handed
        XrPosef      m_WorldBuild          = m_IdentityPose;           // Right-Handed // Position only changes here...
        XrVector3f   m_WorldBuildDegrees   = m_IdentityPose.position;  // Right-Handed

        XrVector3f   m_Offset              = m_IdentityPose.position;  // BUG FIX: corrects the position drift caused by rotatex/rotatey/rotatez.

        XrVector3f   m_Move           = m_IdentityPose.position;
        XrVector3f   m_Rotate         = m_IdentityPose.position;

        bool       m_LeftHand                    = false;
        bool       m_LeftResponding              = false;
        XrPosef    m_Left;                                                         // Left-Handed
        XrVector3f m_LeftDegrees                 = m_IdentityPose.position;        // Left-Handed
        bool       m_LeftHand_X_Button           = false;
        bool       m_LeftHand_Y_Button           = false;
        float      m_LeftHand_Grip_Button        = 0.0f;
        bool       m_LeftHand_Thumbstick_Click   = false;
        bool       m_LeftHand_Menu_Button        = false;
        float      m_LeftHand_Trigger            = 0.0f;
        float      m_LeftHand_Thumbstick_X       = 0.0f;
        float      m_LeftHand_Thumbstick_Y       = 0.0f;

        bool       m_RightHand                   = false; 
        bool       m_RightResponding             = false;
        XrPosef    m_Right;                                                       // Left-Handed
        XrVector3f m_RightDegrees                = m_IdentityPose.position;       // Left-Handed
        bool       m_RightHand_A_Button          = false;
        bool       m_RightHand_B_Button          = false;
        float      m_RightHand_Grip_Button       = 0.0f;
        bool       m_RightHand_Thumbstick_Click  = false;
        bool       m_RightHand_Home_Button       = false;
        float      m_RightHand_Trigger           = 0.0f;
        float      m_RightHand_Thumbstick_X      = 0.0f;
        float      m_RightHand_Thumbstick_Y      = 0.0f;

        bool    m_ReferenceSpaceChanged          = false;

        float   m_Haptic[2]                      = {0, 0};

        bool  m_FollowHMDLook       = true;
        bool  m_FollowHMDY          = false;
        float m_Near                = Default_Near;
        float m_Far                 = Default_Far;
        float m_WorldScale          = 1.00f;

        std::string m_SystemName;
        std::string m_SystemTracking;
        std::string m_SystemID;
        std::string m_VendorID;

        #ifdef _WINDOWS_
        void InitOpenXR()
        #endif
        #ifdef _ANDROID_
        void InitOpenXR(struct engine *engine)
        #endif
        {
            XR_MESSAGE("-- Init OpenXR: Start ----------------------------------------");

            if (getopenxrstatus() == Stage_1_Not_Init_Status)
            {
                #ifdef _WINDOWS_
                //glBindFramebuffer                = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
                //glGenFramebffers                 = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
                glFramebufferTextureMultiviewOVR   = (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)wglGetProcAddress("glFramebufferTextureMultiviewOVR");
                //glCheckFramebufferStatus         = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
                //glFramebufferTexture2D           = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
                glGenVertexArrays                  = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
                glBindVertexArray                  = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
                glDeleteVertexArrays               = (PFNGLDELETEVERTEXARRAYSPROC)wglGetProcAddress("glDeleteVertexArrays");
                glViewportIndexedf                 = (PFNGLVIEWPORTINDEXEDFPROC)wglGetProcAddress("glViewportIndexedf");      // 4.1+
                glDepthRangeIndexed                = (PFNGLDEPTHRANGEINDEXEDPROC)wglGetProcAddress("glDepthRangeIndexed");  // 4.1+
                glScissorIndexed                   = (PFNGLSCISSORINDEXEDPROC)wglGetProcAddress("glScissorIndexed");  // 4.1+
                
                setopenxrstatus(Stage_2_OpenXR_Init_Status);
                setwindowinitialised(Window_Initialised);
                setopenxrstatus(Stage_3_AGK_Active_Status);
                #endif

                #ifdef _ANDROID_
                glFramebufferTextureMultiviewOVR   = (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)eglGetProcAddress("glFramebufferTextureMultiviewOVR");
                glViewportIndexedf                 = (PFNGLVIEWPORTINDEXEDFNVPROC)eglGetProcAddress("glViewportIndexedfNV");
                glScissorIndexed                   = (PFNGLSCISSORINDEXEDNVPROC)eglGetProcAddress("glScissorIndexedNV");

                AndroidApp = engine->app;
                Engine = engine;

                // Allow interaction with JNI and the JVM on this thread.
                // https://developer.android.com/training/articles/perf-jni#threads
                JNIEnv *env;
                AndroidApp->activity->vm->AttachCurrentThread(&env, nullptr);

                // https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_loader_init
                // Load xrInitializeLoaderKHR() function pointer. On Android, the loader must be initialized with variables from android_app *.
                // Without this, there's is no loader and thus our function calls to OpenXR would fail.
                XrInstance m_instance = XR_NULL_HANDLE;  // Dummy XrInstance variable for OPENXR_CHECK macro.
                PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR = nullptr;
                XrResult result = xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction *)&xrInitializeLoaderKHR);
                
                if (result != XR_SUCCESS)
                {
                    setopenxrstatus(Failed_Status);
                    XR_MESSAGE("Failed to get InstanceProcAddr for xrInitializeLoaderKHR.");
                    return;
                }

                if (!xrInitializeLoaderKHR)
                {
                    setopenxrstatus(Failed_Status);
                    return;
                }

                // Fill out an XrLoaderInitInfoAndroidKHR structure and initialize the loader for Android.
                XrLoaderInitInfoAndroidKHR loaderInitializeInfoAndroid{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
                loaderInitializeInfoAndroid.applicationVM = AndroidApp->activity->vm;
                loaderInitializeInfoAndroid.applicationContext = AndroidApp->activity->clazz;
                result = xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR *)&loaderInitializeInfoAndroid);
                
                if (result != XR_SUCCESS)
                {
                    setopenxrstatus(Failed_Status);
                    XR_MESSAGE("Failed to initialize Loader for Android.");
                    return;
                }

                // Set userData and Callback for PollSystemEvents().
                AndroidApp->onAppCmd = AGKOpenXR::AndroidAppHandleCmd;
                
                setopenxrstatus(Stage_2_OpenXR_Init_Status);
                #endif
            }
            XR_MESSAGE("-- Init OpenXR: End ----------------------------------------");
        }
        
        int  Begin(int ObjectID, int ScreenIMG)
        { 
            XR_MESSAGE("openxr begin");

            m_MathObject  = ObjectID;
            m_ScreenImage = ScreenIMG;

            if (getwindowinitialised() == Window_Initialised &&
                getopenxrstatus()      == Stage_3_AGK_Active_Status  )
            {
                for (int a = 0; a < 13; a++)
                {
                    if (a ==  0) CreateInstance();
                    if (a ==  1) GetInstanceProperties();
                    if (a ==  2) GetSystemID();
                    if (a ==  3) CreateActionSet();
                    if (a ==  4) SuggestBindings();
                    if (a ==  5) GetViewConfigurationViews();
                    if (a ==  6) GetEnvironmentBlendModes();
                    if (a ==  7) CreateSession();
                    if (a ==  8) CreateActionPoses();
                    if (a ==  9) AttachActionSet();
                    if (a == 10) CreateReferenceSpace();
                    if (a == 11) CreateSwapchains();
                    if (a == 12) CreateAGKEnviroment();

                    if (getopenxrstatus() == Failed_Status) return 0;
                }

                setopenxrstatus(Stage_4_OpenXR_Active_Status);
                return 1;
            }
          
            return 0;
        }

        void Update()
        {
            #ifdef _XR_DEBUGGING_
            XR_MESSAGE("openxr update");
            #endif

            if (getwindowinitialised() == Window_Initialised           &&
                getopenxrstatus()      == Stage_4_OpenXR_Active_Status )
            {
                if (m_applicationRunning)
                {
                    m_Updated = true;

                    PollSystemEvents();
                    PollEvents();

                    if (m_sessionRunning)
                    {   
                        PreRenderFrame();
                    }
                    else
                    {
                        #ifdef _WINDOWS_
                        agk::Sleep(20000);
                        #endif

                        #ifdef _ANDROID_
                        usleep( 20000 );
                        #endif
                    }
                }
            }
        }

        void Sync()
        {
            #ifdef _XR_DEBUGGING_
            XR_MESSAGE("openxr loop");
            #endif

            if (getwindowinitialised() == Window_Initialised           &&
                getopenxrstatus()      == Stage_4_OpenXR_Active_Status )
            {
                if (m_applicationRunning)
                {
                    if (m_Updated == false)
                    {
                        PollSystemEvents();
                        PollEvents();
                    }

                    if (m_sessionRunning)
                    {
                        RenderFrame();
                    }
                    else
                    {
                        #ifdef _WINDOWS_
                        agk::Sleep(20000);
                        #endif
                        
                        #ifdef _ANDROID_
                        usleep( 20000 );
                        #endif
                    }
                }
            }
        }

        void End()
        {
            XR_MESSAGE("openxr end");

            if (getopenxrstatus()      == Stage_4_OpenXR_Active_Status &&
                getwindowinitialised() == Window_Initialised           )
            {
                DestroyAGKEnviroment();
                DestroySwapchains();
                DestroyReferenceSpace();
                DestroySession();
                DestroyInstance();
         
                setopenxrstatus(Shutdown_Status);
            }
        }

        void SetPosition(float X, float Y, float Z)
        { 
            m_Offset.x = 0.0f;
            m_Offset.y = 0.0f;
            m_Offset.z = 0.0f;

            X = 0 - X;
            Y = 0 - Y;
            Z = 0 - Z;

            float newPlayerX, newPlayerY, newPlayerZ;
            RightToLeftCoordinateSystem(X, Y, Z, newPlayerX, newPlayerY, newPlayerZ);

            float rotateX, rotateY, rotateZ;
            QuaternionToEulerDegrees(m_WorldBuild.orientation.w,
                m_WorldBuild.orientation.x,
                m_WorldBuild.orientation.y,
                m_WorldBuild.orientation.z,
                rotateX, rotateY, rotateZ);

            m_Rotate.x = -rotateX;
            m_Rotate.y = -rotateY;
            m_Rotate.z = -rotateZ;

            rotatex(1, m_frameState.predictedDisplayTime);
            rotatey(1, m_frameState.predictedDisplayTime);
            rotatez(1, m_frameState.predictedDisplayTime);
                
            m_WorldUpToDate = false;
            RebuildReferenceSpace();

            float currentWorldX = m_WorldBuild.position.x;
            float currentWorldY = m_WorldBuild.position.y;
            float currentWorldZ = m_WorldBuild.position.z;

            float currentPlayerX = 0 - m_WorldRH.position.x;
            float currentPlayerY = 0 - m_WorldBuild.position.y;
            float currentPlayerZ = 0 - m_WorldRH.position.z;

            float difX = currentWorldX - currentPlayerX;
            float difY = currentWorldY - currentPlayerY;
            float difZ = currentWorldZ - currentPlayerZ;

            float newWorldX = newPlayerX + difX;
            float newWorldY = newPlayerY + difY;
            float newWorldZ = newPlayerZ + difZ;

            m_WorldBuild.position.x = newWorldX;
            m_WorldBuild.position.y = newWorldY;
            m_WorldBuild.position.z = newWorldZ;

            m_WorldUpToDate = false;
            RebuildReferenceSpace();

            m_Rotate.x = rotateX;
            m_Rotate.y = rotateY;
            m_Rotate.z = rotateZ;

            rotatex(1, m_frameState.predictedDisplayTime);
            rotatey(1, m_frameState.predictedDisplayTime);
            rotatez(1, m_frameState.predictedDisplayTime);
        }

        void SetRotation(float X, float Y, float Z)
        {
            // Not Finished Yet, just recentering for now, X Y Z are ignored
            m_WorldBuild.position    = m_IdentityPose.position;
            m_WorldBuild.orientation = m_IdentityPose.orientation;
            m_WorldUpToDate = false;
            m_Offset.x = 0.0f;
            m_Offset.y = 0.0f;
            m_Offset.z = 0.0f;
            RebuildReferenceSpace();
        }

    private:

        void CreateInstance()
        {
            XR_MESSAGE("-- Create Instance: Start ---------------------------------------------");

            // Fill out an XrApplicationInfo structure detailing the names and OpenXR version.
            // The application/engine name and version are user-definied. These may help IHVs or runtimes.
            XrApplicationInfo AI;
            #ifdef _WINDOWS_
            strncpy(AI.applicationName, "Template_Windows_OpenXR", XR_MAX_APPLICATION_NAME_SIZE);
            #endif
            #ifdef _ANDROID_
            strncpy(AI.applicationName, "Template_Android_OpenXR", XR_MAX_APPLICATION_NAME_SIZE);
            #endif

            AI.applicationVersion = 1;
            strncpy(AI.engineName, "AppGameKit", XR_MAX_ENGINE_NAME_SIZE);
            AI.engineVersion = 1;
            AI.apiVersion = m_version;

            // Add additional instance layers/extensions that the application wants.
            // Add both required and requested instance extensions.
            {
                m_instanceExtensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
                #ifdef _WINDOWS_
                m_instanceExtensions.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
                #endif      
                #ifdef _ANDROID_
                m_instanceExtensions.push_back(XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME);
                #endif      
            }

            // Get all the API Layers from the OpenXR runtime.
            uint32_t apiLayerCount = 0;
            std::vector<XrApiLayerProperties> apiLayerProperties;
            XrResult result = xrEnumerateApiLayerProperties(0, &apiLayerCount, nullptr);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Create Instance: Failed to enumerate ApiLayerProperties.");
                return;
            }

            apiLayerProperties.resize(apiLayerCount, {XR_TYPE_API_LAYER_PROPERTIES});
            result = xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount, apiLayerProperties.data());
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE( "Create Instance: Failed to enumerate ApiLayerProperties.");
                return;
            }

            // Check the requested API layers against the ones from the OpenXR. If found add it to the Active API Layers.
            for (auto &requestLayer : m_apiLayers)
            {
                for (auto &layerProperty : apiLayerProperties)
                {
                    // strcmp returns 0 if the strings match.
                    if (strcmp(requestLayer.c_str(), layerProperty.layerName) != 0)
                    {
                        continue;
                    }
                    else
                    {
                        m_activeAPILayers.push_back(requestLayer.c_str());
                        break;
                    }
                }
            }

            // Get all the Instance Extensions from the OpenXR instance.
            uint32_t extensionCount = 0;
            std::vector<XrExtensionProperties> extensionProperties;
            result = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Create Instance: Failed to enumerate InstanceExtensionProperties.");
                return;
            }

            extensionProperties.resize(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
            
            result = xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensionProperties.data());
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Create Instance: Failed to enumerate InstanceExtensionProperties.");
                return;
            }

            // Check the requested Instance Extensions against the ones from the OpenXR runtime.
            // If an extension is found add it to Active Instance Extensions.
            // Log error if the Instance Extension is not found.
            for (auto &requestedInstanceExtension : m_instanceExtensions)
            {
                bool found = false;
                for (auto &extensionProperty : extensionProperties)
                {
                    // strcmp returns 0 if the strings match.
                    if (strcmp(requestedInstanceExtension.c_str(), extensionProperty.extensionName) != 0)
                    {
                        continue;
                    }
                    else
                    {
                        m_activeInstanceExtensions.push_back(requestedInstanceExtension.c_str());
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    std::stringstream sStr;
                    sStr << "Create Instance: Failed to find OpenXR instance extension: " << requestedInstanceExtension;
                    std::string message = sStr.str();
                    XR_MESSAGE(message);

                    return;
                }
            }

            // Fill out an XrInstanceCreateInfo structure and create an XrInstance.
            XrInstanceCreateInfo instanceCI{XR_TYPE_INSTANCE_CREATE_INFO};
            instanceCI.createFlags = 0;
            instanceCI.applicationInfo = AI;
            instanceCI.enabledApiLayerCount = static_cast<uint32_t>(m_activeAPILayers.size());
            instanceCI.enabledApiLayerNames = m_activeAPILayers.data();
            instanceCI.enabledExtensionCount = static_cast<uint32_t>(m_activeInstanceExtensions.size());
            instanceCI.enabledExtensionNames = m_activeInstanceExtensions.data();
            result = xrCreateInstance(&instanceCI, &m_instance);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Create Instance: Failed to create Instance.");
                return;
            }

            XR_MESSAGE("-- Create Instance: End ---------------------------");
        }

        void GetInstanceProperties()
        {
            XR_MESSAGE("-- Get Instance Properties: Start -----------------------------------");

            // Get the instance's properties and log the runtime name and version.
            XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
            XrResult result = xrGetInstanceProperties(m_instance, &instanceProperties);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Get Instance Properties: Failed to get InstanceProperties.");
                return;
            }

            std::stringstream sStr;
            sStr << "OpenXR Runtime: " << instanceProperties.runtimeName << " - "
                << XR_VERSION_MAJOR(instanceProperties.runtimeVersion) << "."
                << XR_VERSION_MINOR(instanceProperties.runtimeVersion) << "."
                << XR_VERSION_PATCH(instanceProperties.runtimeVersion);
            std::string message = sStr.str();
            XR_MESSAGE(message);
        
            XR_MESSAGE("-- Get Instance Properties: End -----------------------------------");
        }

        void GetSystemID()
        {
            XR_MESSAGE("-- Get System ID: Start --------------------------------------------");

            // Get the XrSystemId from the instance and the supplied XrFormFactor.
            XrSystemGetInfo systemGI{XR_TYPE_SYSTEM_GET_INFO};
            systemGI.formFactor = m_formFactor;
            XrResult result = xrGetSystem(m_instance, &systemGI, &m_systemID);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Get System ID: Failed to get SystemID.");
                return;
            }

            // Get the System's properties for some general information about the hardware and the vendor.
            result = xrGetSystemProperties(m_instance, m_systemID, &m_systemProperties);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Get System ID: Failed to get SystemProperties.");
                return;
            }

            // Save Useful Information
            m_SystemName = m_systemProperties.systemName;
            std::stringstream sStr;
            sStr << "Orientation Tracking:" << (m_systemProperties.trackingProperties.orientationTracking ? "Yes" : "No") << " Position Tracking:" << (m_systemProperties.trackingProperties.positionTracking ? "Yes" : "No");
            m_SystemTracking = sStr.str();
            sStr.str("");
            sStr << "System ID:" << m_systemProperties.systemId;
            m_SystemID = sStr.str();
            sStr.str("");
            sStr << "Vendor ID:" << m_systemProperties.vendorId;
            m_VendorID = sStr.str();
            
            XR_MESSAGE("-- Get System ID: End --------------------------------------------");
        }

        void CreateActionSet()
        {
            XR_MESSAGE("-- Create Action Set: Start -------------------------------------------------");

            XrActionSetCreateInfo actionSetCI{XR_TYPE_ACTION_SET_CREATE_INFO};
            strncpy(actionSetCI.actionSetName, "agkopenxractionset", XR_MAX_ACTION_SET_NAME_SIZE);
            strncpy(actionSetCI.localizedActionSetName, "AGK OpenXR Actionset", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);
            actionSetCI.priority = 0;
            XrResult result = xrCreateActionSet(m_instance, &actionSetCI, &m_actionSet);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Create Action Set: Failed to create ActionSet.");
                return;
            }

            auto CreateAction = [this](XrAction &xrAction, const char *name, XrActionType xrActionType, std::vector<const char *> subaction_paths = {}) -> void
            {
                XrActionCreateInfo actionCI{XR_TYPE_ACTION_CREATE_INFO};
                // The type of action: float input, pose, haptic output etc.
                actionCI.actionType = xrActionType;
                // Subaction paths, e.g. left and right hand. To distinguish the same action performed on different devices.
                std::vector<XrPath> subaction_xrpaths;
                for (auto p : subaction_paths)
                {
                    subaction_xrpaths.push_back(CreateXrPath(p));
                }
                actionCI.countSubactionPaths = (uint32_t)subaction_xrpaths.size();
                actionCI.subactionPaths = subaction_xrpaths.data();
                // The internal name the runtime uses for this Action.
                strncpy(actionCI.actionName, name, XR_MAX_ACTION_NAME_SIZE);
                // Localized names are required so there is a human-readable action name to show the user if they are rebinding the Action in an options screen.
                strncpy(actionCI.localizedActionName, name, XR_MAX_LOCALIZED_ACTION_NAME_SIZE);
                XrResult result = xrCreateAction(m_actionSet, &actionCI, &xrAction);
                
                if (result != XR_SUCCESS)
                {
                    setopenxrstatus(Failed_Status);
                    XR_MESSAGE("Create Action Set: Failed to create Action.");
                    return;
                }
            };

            // Creating button actions for left and right hand controllers
            CreateAction(m_palmPoseAction, "palm-pose", XR_ACTION_TYPE_POSE_INPUT, {"/user/hand/left", "/user/hand/right"});
            
            CreateAction(m_LeftHand_X_Button_Action, "left_x_button", XR_ACTION_TYPE_BOOLEAN_INPUT, {"/user/hand/left"});
            CreateAction(m_LeftHand_Y_Button_Action, "left_y_button", XR_ACTION_TYPE_BOOLEAN_INPUT, {"/user/hand/left"});
            CreateAction(m_LeftHand_Trigger_Button_Action, "left_trigger", XR_ACTION_TYPE_FLOAT_INPUT, {"/user/hand/left"});
            CreateAction(m_LeftHand_Grip_Button_Action, "left_grip", XR_ACTION_TYPE_FLOAT_INPUT, {"/user/hand/left"});
            CreateAction(m_LeftHand_Thumbstick_Action, "left_thumbstick", XR_ACTION_TYPE_VECTOR2F_INPUT, {"/user/hand/left"});
            CreateAction(m_LeftHand_Thumbstick_Click_Action, "left_thumbstick_click", XR_ACTION_TYPE_BOOLEAN_INPUT, {"/user/hand/left"});
            CreateAction(m_LeftHand_Buzz_Action, "left_buzz", XR_ACTION_TYPE_VIBRATION_OUTPUT, {"/user/hand/left"});

            CreateAction(m_RightHand_A_Button_Action, "right_a_button", XR_ACTION_TYPE_BOOLEAN_INPUT, {"/user/hand/right"});
            CreateAction(m_RightHand_B_Button_Action, "right_b_button", XR_ACTION_TYPE_BOOLEAN_INPUT, {"/user/hand/right"});
            CreateAction(m_RightHand_Trigger_Button_Action, "right_trigger", XR_ACTION_TYPE_FLOAT_INPUT, {"/user/hand/right"});
            CreateAction(m_RightHand_Grip_Button_Action, "right_grip", XR_ACTION_TYPE_FLOAT_INPUT, {"/user/hand/right"});
            CreateAction(m_RightHand_Thumbstick_Action, "right_thumbstick", XR_ACTION_TYPE_VECTOR2F_INPUT, {"/user/hand/right"});
            CreateAction(m_RightHand_Thumbstick_Click_Action, "right_thumbstick_click", XR_ACTION_TYPE_BOOLEAN_INPUT, {"/user/hand/right"});
            CreateAction(m_RightHand_Buzz_Action, "right_buzz", XR_ACTION_TYPE_VIBRATION_OUTPUT, {"/user/hand/right"});

            // For later convenience we create the XrPaths for the subaction path names.
            m_handPaths[0] = CreateXrPath("/user/hand/left");
            m_handPaths[1] = CreateXrPath("/user/hand/right");

            XR_MESSAGE("-- Create Action Set: End -------------------------------------------------");
        }

        void SuggestBindings()
        {
            XR_MESSAGE("-- Suggest Bindings: Start ------------------------------------------------------");

            auto SuggestBindings = [this](const char *profile_path, std::vector<XrActionSuggestedBinding> bindings) -> bool
            {
                // The application can call xrSuggestInteractionProfileBindings once per interaction profile that it supports.
                XrInteractionProfileSuggestedBinding interactionProfileSuggestedBinding{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
                interactionProfileSuggestedBinding.interactionProfile = CreateXrPath(profile_path);
                interactionProfileSuggestedBinding.suggestedBindings = bindings.data();
                interactionProfileSuggestedBinding.countSuggestedBindings = (uint32_t)bindings.size();
                if (xrSuggestInteractionProfileBindings(m_instance, &interactionProfileSuggestedBinding) == XrResult::XR_SUCCESS)
                {
                    std::stringstream sStr;
                    sStr << "Success to suggest bindings with " << profile_path;
                    std::string sString = sStr.str();
                    XR_MESSAGE(sString);
                    return true;
                }
                
                std::stringstream sStr;
                sStr << "Failed to suggest bindings with " << profile_path;
                std::string sString = sStr.str();
                XR_MESSAGE(sString);
                return false;
            };

            bool any_ok = false;

            any_ok |= SuggestBindings("/interaction_profiles/khr/simple_controller",
            {
                {m_LeftHand_X_Button_Action, CreateXrPath("/user/hand/left/input/select/click")},
                {m_RightHand_A_Button_Action, CreateXrPath("/user/hand/right/input/select/click")},
                {m_palmPoseAction, CreateXrPath("/user/hand/left/input/grip/pose")},
                {m_palmPoseAction, CreateXrPath("/user/hand/right/input/grip/pose")},
                {m_LeftHand_Buzz_Action, CreateXrPath("/user/hand/left/output/haptic")},
                {m_RightHand_Buzz_Action, CreateXrPath("/user/hand/right/output/haptic")}
            });

            any_ok |= SuggestBindings("/interaction_profiles/oculus/touch_controller",
            {
              /*{m_RightHand_Home_Button_Action,      CreateXrPath("/user/hand/right/input/system/click")},
                {m_LeftHand_Menu_Button_Action,       CreateXrPath("/user/hand/left/input/menu/click")},*/
                {m_palmPoseAction,                    CreateXrPath("/user/hand/left/input/grip/pose")},
                {m_palmPoseAction,                    CreateXrPath("/user/hand/right/input/grip/pose")},
                {m_LeftHand_X_Button_Action,          CreateXrPath("/user/hand/left/input/x/click")},
                {m_LeftHand_Y_Button_Action,          CreateXrPath("/user/hand/left/input/y/click")},
                {m_LeftHand_Trigger_Button_Action,    CreateXrPath("/user/hand/left/input/trigger/value")},
                {m_LeftHand_Grip_Button_Action,       CreateXrPath("/user/hand/left/input/squeeze/value")},
                {m_LeftHand_Thumbstick_Click_Action,  CreateXrPath("/user/hand/left/input/thumbstick/click")},
                {m_LeftHand_Thumbstick_Action,        CreateXrPath("/user/hand/left/input/thumbstick")},
                {m_LeftHand_Buzz_Action,              CreateXrPath("/user/hand/left/output/haptic")},
                {m_RightHand_A_Button_Action,         CreateXrPath("/user/hand/right/input/a/click")},
                {m_RightHand_B_Button_Action,         CreateXrPath("/user/hand/right/input/b/click")},
                {m_RightHand_Trigger_Button_Action,   CreateXrPath("/user/hand/right/input/trigger/value")},
                {m_RightHand_Grip_Button_Action,      CreateXrPath("/user/hand/right/input/squeeze/value")},
                {m_RightHand_Thumbstick_Click_Action, CreateXrPath("/user/hand/right/input/thumbstick/click")},
                {m_RightHand_Thumbstick_Action,       CreateXrPath("/user/hand/right/input/thumbstick")},
                {m_RightHand_Buzz_Action,             CreateXrPath("/user/hand/right/output/haptic")}
            });

            if (!any_ok)
            {
                XR_MESSAGE("ERROR: Find Bindings Failed...");
                setopenxrstatus(Failed_Status);
                return;
            }

            XR_MESSAGE("-- Suggest Bindings: End ------------------------------------------------------");
        }
        
        void GetViewConfigurationViews()
        {
            XR_MESSAGE("-- Get View Config: Start ---------------------------------------------------");

            // Gets the View Configuration Types. The first call gets the count of the array that will be returned. The next call fills out the array.
            uint32_t viewConfigurationCount = 0;
            XrResult result = xrEnumerateViewConfigurations(m_instance, m_systemID, 0, &viewConfigurationCount, nullptr);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to enumerate View Configurations.");
                return;
            }

            m_viewConfigurations.resize(viewConfigurationCount);
            result = xrEnumerateViewConfigurations(m_instance, m_systemID, viewConfigurationCount, &viewConfigurationCount, m_viewConfigurations.data());
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to enumerate View Configurations.");
                return;
            }

            // Pick the first application supported View Configuration Type con supported by the hardware.
            for (const XrViewConfigurationType &viewConfiguration : m_applicationViewConfigurations)
            {
                if (std::find(m_viewConfigurations.begin(), m_viewConfigurations.end(), viewConfiguration) != m_viewConfigurations.end())
                {
                    m_viewConfiguration = viewConfiguration;
                    if (viewConfiguration == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO)   XR_MESSAGE("Mono view mode.");
                    if (viewConfiguration == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) XR_MESSAGE("Stereo view mode.");
                    break;
                }
            }

            if (m_viewConfiguration == XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM)
            {
                XR_MESSAGE("Failed to find a view configuration type. Defaulting to XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO.");
                m_viewConfiguration = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            }

            // Gets the View Configuration Views. The first call gets the count of the array that will be returned. The next call fills out the array.
            uint32_t viewConfigurationViewCount = 0;
            result = xrEnumerateViewConfigurationViews(m_instance, m_systemID, m_viewConfiguration, 0, &viewConfigurationViewCount, nullptr);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to enumerate ViewConfiguration Views.");
                return;
            }

            m_viewConfigurationViews.resize(viewConfigurationViewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
            result = xrEnumerateViewConfigurationViews(m_instance, m_systemID, m_viewConfiguration, viewConfigurationViewCount, &viewConfigurationViewCount, m_viewConfigurationViews.data());
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to enumerate ViewConfiguration Views.");
                return;
            }

            XR_MESSAGE("-- Get View Config: End ---------------------------------------------------");
        }

        void GetEnvironmentBlendModes()
        {
            XR_MESSAGE("-- Get Enviro Blend Modes: Start -----------------------------------------");

            // Retrieves the available blend modes. The first call gets the count of the array that will be returned. The next call fills out the array.
            uint32_t environmentBlendModeCount = 0;
            XrResult result = xrEnumerateEnvironmentBlendModes(m_instance, m_systemID, m_viewConfiguration, 0, &environmentBlendModeCount, nullptr);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to enumerate EnvironmentBlend Modes.");
                return;
            }

            m_environmentBlendModes.resize(environmentBlendModeCount);
            result = xrEnumerateEnvironmentBlendModes(m_instance, m_systemID, m_viewConfiguration, environmentBlendModeCount, &environmentBlendModeCount, m_environmentBlendModes.data());
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to enumerate EnvironmentBlend Modes.");
                return;
            }

            // Pick the first application supported blend mode supported by the hardware.
            for (const XrEnvironmentBlendMode &environmentBlendMode : m_applicationEnvironmentBlendModes)
            {
                if (std::find(m_environmentBlendModes.begin(), m_environmentBlendModes.end(), environmentBlendMode) != m_environmentBlendModes.end())
                {
                    m_environmentBlendMode = environmentBlendMode;
                    break;
                }
            }
            if (m_environmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM)
            {
                XR_MESSAGE("Failed to find a compatible blend mode. Defaulting to XR_ENVIRONMENT_BLEND_MODE_OPAQUE.");
                m_environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
            }

            XR_MESSAGE("-- Get Enviro Blend Modes: End -----------------------------------------");
        }
        
        #ifdef _WINDOWS_
        XrGraphicsBindingOpenGLWin32KHR graphicsBinding{};
        HDC   m_hDC   = 0; // config1
        HGLRC m_hGLRC = 0; // config2  
        HWND  m_hWnd  = 0; // config3
        #endif

        #ifdef _ANDROID_
        PFN_xrGetOpenGLESGraphicsRequirementsKHR xrGetOpenGLESGraphicsRequirementsKHR = nullptr;
        XrGraphicsBindingOpenGLESAndroidKHR graphicsBinding{};
        #endif

        void *GetGraphicsBinding()
        {
            XR_MESSAGE("Get Graphic Binding: Start");

            #ifdef _WINDOWS_
            void** notused = nullptr;

            agk::GetGraphicsConfig(reinterpret_cast<void**>(&m_hDC),
                                   reinterpret_cast<void**>(&m_hGLRC),
                                   reinterpret_cast<void**>(&m_hWnd),
                                   notused, notused, notused);

            // https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/f122f9f1fc729e2dc82e12c3ce73efa875182854/src/tests/hello_xr/graphicsplugin_opengl.cpp#L123-L144
            graphicsBinding       = { XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
            graphicsBinding.hDC   = m_hDC;
            graphicsBinding.hGLRC = m_hGLRC;
            return &graphicsBinding;
            #endif

            #ifdef _ANDROID_
            EGLDisplay display  = EGL_NO_DISPLAY;
            EGLSurface surface  = EGL_NO_SURFACE;
            EGLContext context  = EGL_NO_CONTEXT;
            EGLConfig eglconfig = 0;

            // Initialize void* variables to point to the EGL variables
            void* displayPtr   = &display;
            void* surfacePtr   = &surface;
            void* contextPtr   = &context;
            void* eglconfigPtr = &eglconfig;
            void* notUsed1     = nullptr; 
            void* notUsed2     = nullptr;

            // Call the configuration function
            agk::GetGraphicsConfig(&displayPtr, &surfacePtr, &contextPtr, &eglconfigPtr, &notUsed1, &notUsed2);

            // Cast back to the original types and assign the values
            display   = *static_cast<EGLDisplay*>(displayPtr);
            surface   = *static_cast<EGLSurface*>(surfacePtr);
            context   = *static_cast<EGLContext*>(contextPtr);
            eglconfig = *static_cast<EGLConfig*>(eglconfigPtr);

            if (display == EGL_NO_DISPLAY)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Get Graphic Binding: EGL_NO_DISPLAY");
            }
            if (surface == EGL_NO_SURFACE)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Get Graphic Binding: EGL_NO_SURFACE");
            }
            if (context == EGL_NO_CONTEXT)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Get Graphic Binding: EGL_NO_CONTEXT");
            }
            if (eglconfig == 0)   
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Get Graphic Binding: eglconfig is zero.");
            }

            graphicsBinding = {XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};
            graphicsBinding.display = display;
            graphicsBinding.config  = eglconfig;
            graphicsBinding.context = context;
            #endif
        
            XR_MESSAGE("Get Graphic Binding: End");
            return &graphicsBinding;
        }

        void CreateSession()
        {
            XR_MESSAGE("-- Create Session: Start ----------------------------------------------------");

            #ifdef _WINDOWS_
            XrGraphicsRequirementsOpenGLKHR graphicsRequirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
            PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetGraphicsRequirements = nullptr;
            XrResult result = xrGetInstanceProcAddr(m_instance, "xrGetOpenGLGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetGraphicsRequirements));

            if (result != XR_SUCCESS || !pfnGetGraphicsRequirements)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Create Session: Failed to get xrGetOpenGLGraphicsRequirementsKHR function pointer.");
                return;
            }

            result = pfnGetGraphicsRequirements(m_instance, m_systemID, &graphicsRequirements);

            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Create Session: Failed to get graphics requirements for OpenGL.");
                return;
            }
            #endif

            #ifdef _ANDROID_
            XrResult result = xrGetInstanceProcAddr(m_instance, "xrGetOpenGLESGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&xrGetOpenGLESGraphicsRequirementsKHR);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Create Session: Failed to get InstanceProcAddr for xrGetOpenGLESGraphicsRequirementsKHR.");
                return;
            }
            
            XrGraphicsRequirementsOpenGLESKHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
            
            result = xrGetOpenGLESGraphicsRequirementsKHR(m_instance, m_systemID, &graphicsRequirements);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE( "Create Session: Failed to get Graphics Requirements for OpenGLES.");
                return;
            }
            #endif

            // Create an XrSessionCreateInfo structure.
            XrSessionCreateInfo sessionCI{XR_TYPE_SESSION_CREATE_INFO};

            // Fill out the XrSessionCreateInfo structure and create an XrSession.
            sessionCI.next        = GetGraphicsBinding();
            sessionCI.createFlags = 0;
            sessionCI.systemId    = m_systemID;

            result = xrCreateSession(m_instance, &sessionCI, &m_session);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Create Session: Failed to create Session.");
                return;
            }

            XR_MESSAGE("-- Create Session: End -----------------------------------------------------------");
        }

        void CreateActionPoses()
        {
            XR_MESSAGE("-- Create Action Poses: Start ------------------------------------");

            // Create an xrSpace for a pose action.
            auto CreateActionPoseSpace = [this](XrSession session, XrAction xrAction, const char *subaction_path = nullptr) -> XrSpace
            {
                XrSpace xrSpace;
                const XrPosef xrPoseIdentity = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};
                // Create frame of reference for a pose action
                XrActionSpaceCreateInfo actionSpaceCI{XR_TYPE_ACTION_SPACE_CREATE_INFO};
                actionSpaceCI.action = xrAction;
                actionSpaceCI.poseInActionSpace = xrPoseIdentity;
                if (subaction_path) actionSpaceCI.subactionPath = CreateXrPath(subaction_path);

                XrResult result = xrCreateActionSpace(session, &actionSpaceCI, &xrSpace);

                if (result != XR_SUCCESS)
                {
                    XR_MESSAGE("Failed to create action space!");
                    setopenxrstatus(Failed_Status);
                }

                return xrSpace;
            };
            m_handPoseSpace[0] = CreateActionPoseSpace(m_session, m_palmPoseAction, "/user/hand/left");
            m_handPoseSpace[1] = CreateActionPoseSpace(m_session, m_palmPoseAction, "/user/hand/right");

            if (m_handPoseSpace[0] == XR_NULL_HANDLE ||
                m_handPoseSpace[1] == XR_NULL_HANDLE )
            {
                XR_MESSAGE("Pose space handle is null.");
                setopenxrstatus(Failed_Status);
                return;
            }
            
            XR_MESSAGE("-- Create Action Poses: End ------------------------------------");
        }
        
        void AttachActionSet()
        {
            XR_MESSAGE("-- Attach Action Set: Start ---------------------------------------------------");

            XrSessionActionSetsAttachInfo actionSetAttachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
            actionSetAttachInfo.countActionSets = 1;
            actionSetAttachInfo.actionSets = &m_actionSet;
            XrResult result = xrAttachSessionActionSets(m_session, &actionSetAttachInfo);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to attach ActionSet to Session.");
                return;
            }

            XR_MESSAGE("-- Attach Action Set: End-----------------------------------------------------");
        }

        void CreateReferenceSpace()
        {
            XR_MESSAGE("-- Create Reference Space: Start ----------------------------");

            // Create World Reference Space
            m_WorldBuild = m_IdentityPose;
            XrReferenceSpaceCreateInfo worldSpaceCreateInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
            worldSpaceCreateInfo.referenceSpaceType = WorldType;
            worldSpaceCreateInfo.poseInReferenceSpace = m_WorldBuild;
            XrResult result = xrCreateReferenceSpace(m_session, &worldSpaceCreateInfo, &m_WorldSpace);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to create World Reference Space.");
                return;
            }
            m_WorldUpToDate = true;

            // Create VIEW Reference Space
            m_ViewBuild = m_IdentityPose;
            XrReferenceSpaceCreateInfo viewSpaceCreateInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
            viewSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
            viewSpaceCreateInfo.poseInReferenceSpace = m_ViewBuild;
            result = xrCreateReferenceSpace(m_session, &viewSpaceCreateInfo, &m_ViewSpace);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to create View Reference Space.");
                return;
            }
            m_ViewUpToDate = true;

            XR_MESSAGE("-- Create Ref Space: End ----------------------------");
        }

        int   m_SwapChainWidth   =    0;
        int   m_SwapChainHeight  =    0;
        float m_FieldOfViewHor   = -1.0f;
        float m_FieldOfViewVer   = -1.0f;
       
        int64_t SelectColorSwapchainFormat(const std::vector<int64_t> &formats)
        {
            const std::vector<int64_t> &supportSwapchainFormats = GetSupportedColorSwapchainFormats();

            const std::vector<int64_t>::const_iterator &swapchainFormatIt = std::find_first_of(formats.begin(), formats.end(), std::begin(supportSwapchainFormats), std::end(supportSwapchainFormats));

            if (swapchainFormatIt == formats.end())
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("ERROR: Unable to find supported Color Swapchain Format");
                return 0;
            }

            return *swapchainFormatIt;
        }
        int64_t SelectDepthSwapchainFormat(const std::vector<int64_t> &formats)
        {
            const std::vector<int64_t> &supportSwapchainFormats = GetSupportedDepthSwapchainFormats();

            const std::vector<int64_t>::const_iterator &swapchainFormatIt = std::find_first_of(formats.begin(), formats.end(), std::begin(supportSwapchainFormats), std::end(supportSwapchainFormats));

            if (swapchainFormatIt == formats.end())
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("ERROR: Unable to find supported Depth Swapchain Format");
                return 0;
            }

            return *swapchainFormatIt;
        }
        const std::vector<int64_t> GetSupportedColorSwapchainFormats()
        {
            // https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/f122f9f1fc729e2dc82e12c3ce73efa875182854/src/tests/hello_xr/graphicsplugin_opengles.cpp#L208-L216
            GLint glMajorVersion = 0;
            glGetIntegerv(GL_MAJOR_VERSION, &glMajorVersion);
            if (glMajorVersion >= 3)
            {
                return {GL_RGBA8, GL_RGBA8_SNORM, GL_SRGB8_ALPHA8};
            }
            else
            {
                return {GL_RGBA8, GL_RGBA8_SNORM};
            }
        }
        const std::vector<int64_t> GetSupportedDepthSwapchainFormats()
        {
            return
            {
                GL_DEPTH_COMPONENT32F,
                GL_DEPTH_COMPONENT24,
                GL_DEPTH_COMPONENT16
            };
        }
        enum class SwapchainType : uint8_t
        {
            COLOR,
            DEPTH
        };

        #ifdef _WINDOWS_
        std::unordered_map < XrSwapchain, std::pair<SwapchainType, std::vector<XrSwapchainImageOpenGLKHR>>> swapchainImagesMap{};
        #endif

        #ifdef _ANDROID_
        std::unordered_map < XrSwapchain, std::pair<SwapchainType, std::vector<XrSwapchainImageOpenGLESKHR>>> swapchainImagesMap{};
        #endif

        void FreeSwapchainImageData(XrSwapchain swapchain)
        {
            swapchainImagesMap[swapchain].second.clear();
            swapchainImagesMap.erase(swapchain);
        }
        XrSwapchainImageBaseHeader* GetSwapchainImageData(XrSwapchain swapchain, uint32_t index)
        {
            return (XrSwapchainImageBaseHeader*)&swapchainImagesMap[swapchain].second[index];
        }
        void* GetSwapchainImage(XrSwapchain swapchain, uint32_t index)
        {
            return (void*)(uint64_t)swapchainImagesMap[swapchain].second[index].image;
        }
        XrSwapchainImageBaseHeader* AllocateSwapchainImageData(XrSwapchain swapchain, SwapchainType type, uint32_t count)
        {
            swapchainImagesMap[swapchain].first = type;
            swapchainImagesMap[swapchain].second.resize(count, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});
            return reinterpret_cast<XrSwapchainImageBaseHeader *>(swapchainImagesMap[swapchain].second.data());
        }
        struct ImageViewCreateInfo
        {
            void* image;
            enum class Type : uint8_t {
                RTV,
                DSV,
                SRV,
                UAV
            } type;
            enum class View : uint8_t {
                TYPE_1D,
                TYPE_2D,
                TYPE_3D,
                TYPE_CUBE,
                TYPE_1D_ARRAY,
                TYPE_2D_ARRAY,
                TYPE_CUBE_ARRAY,
            } view;
            int64_t format;
            enum class Aspect : uint8_t {
                COLOR_BIT = 0x01,
                DEPTH_BIT = 0x02,
                STENCIL_BIT = 0x04
            } aspect;
            uint32_t baseMipLevel;
            uint32_t levelCount;
            uint32_t baseArrayLayer;
            uint32_t layerCount;
        };
        std::unordered_map<GLuint, ImageViewCreateInfo> imageViews{};
        void *CreateImageView(const ImageViewCreateInfo &imageViewCI)
        {
            GLuint framebuffer = 0;
            glGenFramebuffers(1, &framebuffer);

            GLenum attachment = imageViewCI.aspect == ImageViewCreateInfo::Aspect::COLOR_BIT ? GL_COLOR_ATTACHMENT0 : GL_DEPTH_ATTACHMENT;

            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            if (imageViewCI.view == ImageViewCreateInfo::View::TYPE_2D_ARRAY)
            {
                glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, attachment, (GLuint)(uint64_t)imageViewCI.image, imageViewCI.baseMipLevel, imageViewCI.baseArrayLayer, imageViewCI.layerCount);
            }
            else if (imageViewCI.view == ImageViewCreateInfo::View::TYPE_2D)
            {
                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, (GLuint)(uint64_t)imageViewCI.image, imageViewCI.baseMipLevel);
            }
            else
            {
                XR_MESSAGE("ERROR: OPENGL: Unknown ImageView View type.");
            }

            GLenum result = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
            if (result != GL_FRAMEBUFFER_COMPLETE)
            {
                XR_MESSAGE("ERROR: OPENGL: Framebuffer is not complete");
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            imageViews[framebuffer] = imageViewCI;
            return (void *)(uint64_t)framebuffer;
        }
        void CreateSwapchains()
        {
            XR_MESSAGE("-- Create Swapchains: Start --------------------------------------");

            // Get the supported swapchain formats as an array of int64_t and ordered by runtime preference.
            uint32_t formatCount = 0;
            XrResult result = xrEnumerateSwapchainFormats(m_session, 0, &formatCount, nullptr);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to enumerate Swapchain Formats");
                return;
            }

            std::vector<int64_t> formats(formatCount);
            
            result = xrEnumerateSwapchainFormats(m_session, formatCount, &formatCount, formats.data());
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to enumerate Swapchain Formats");
                return;
            }

            if (SelectDepthSwapchainFormat(formats) == 0)
            { 
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to find depth format for Swapchain.");
                return;
            }

            //Resize the SwapchainInfo to match the number of view in the View Configuration.
            m_colorSwapchainInfos.resize(m_viewConfigurationViews.size());
            m_depthSwapchainInfos.resize(m_viewConfigurationViews.size());

            // Per view, create a color and depth swapchain, and their associated image views.
            for (size_t i = 0; i < m_viewConfigurationViews.size(); i++)
            {
                SwapchainInfo &colorSwapchainInfo = m_colorSwapchainInfos[i];
                SwapchainInfo &depthSwapchainInfo = m_depthSwapchainInfos[i];

                // Fill out an XrSwapchainCreateInfo structure and create an XrSwapchain.
                // Color.
                XrSwapchainCreateInfo swapchainCI{XR_TYPE_SWAPCHAIN_CREATE_INFO};
                swapchainCI.createFlags = 0;
                swapchainCI.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
                swapchainCI.format = SelectColorSwapchainFormat(formats);                // Use GraphicsAPI to select the first compatible format.
                swapchainCI.sampleCount = m_viewConfigurationViews[i].recommendedSwapchainSampleCount;  // Use the recommended values from the XrViewConfigurationView.
                swapchainCI.width = m_viewConfigurationViews[i].recommendedImageRectWidth;
                swapchainCI.height = m_viewConfigurationViews[i].recommendedImageRectHeight;
                m_SwapChainWidth = swapchainCI.width;
                m_SwapChainHeight =  swapchainCI.height;
                swapchainCI.faceCount = 1;
                swapchainCI.arraySize = 1;
                swapchainCI.mipCount = 1; 
                XrResult result = xrCreateSwapchain(m_session, &swapchainCI, &colorSwapchainInfo.swapchain);
                
                if (result != XR_SUCCESS)
                {
                    setopenxrstatus(Failed_Status);
                    XR_MESSAGE("Failed to create Color Swapchain");
                    return;
                }

                colorSwapchainInfo.swapchainFormat = swapchainCI.format;  // Save the swapchain format for later use.

                // Depth.
                swapchainCI.createFlags = 0;
                swapchainCI.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                swapchainCI.format = SelectDepthSwapchainFormat(formats);                // Use GraphicsAPI to select the first compatible format.
                swapchainCI.sampleCount = m_viewConfigurationViews[i].recommendedSwapchainSampleCount;  // Use the recommended values from the XrViewConfigurationView.
                swapchainCI.width = m_viewConfigurationViews[i].recommendedImageRectWidth;
                swapchainCI.height = m_viewConfigurationViews[i].recommendedImageRectHeight;
                swapchainCI.faceCount = 1;
                swapchainCI.arraySize = 1;
                swapchainCI.mipCount = 1;
                result = xrCreateSwapchain(m_session, &swapchainCI, &depthSwapchainInfo.swapchain);
                
                if (result != XR_SUCCESS)
                {
                    setopenxrstatus(Failed_Status);
                    XR_MESSAGE("Failed to create Depth Swapchain");
                    return;
                }

                depthSwapchainInfo.swapchainFormat = swapchainCI.format;  // Save the swapchain format for later use.

                // Get the number of images in the color/depth swapchain and allocate Swapchain image data via GraphicsAPI to store the returned array.
                uint32_t colorSwapchainImageCount = 0;
                result = xrEnumerateSwapchainImages(colorSwapchainInfo.swapchain, 0, &colorSwapchainImageCount, nullptr);
                
                if (result != XR_SUCCESS)
                {
                    setopenxrstatus(Failed_Status);
                    XR_MESSAGE("Failed to enumerate Color Swapchain Images.");
                    return;
                }

                XrSwapchainImageBaseHeader *colorSwapchainImages = AllocateSwapchainImageData(colorSwapchainInfo.swapchain, SwapchainType::COLOR, colorSwapchainImageCount);
                result = xrEnumerateSwapchainImages(colorSwapchainInfo.swapchain, colorSwapchainImageCount, &colorSwapchainImageCount, colorSwapchainImages);
                
                if (result != XR_SUCCESS)
                {
                    setopenxrstatus(Failed_Status);
                    XR_MESSAGE("Failed to enumerate Color Swapchain Images.");
                    return;
                }

                uint32_t depthSwapchainImageCount = 0;
                result = xrEnumerateSwapchainImages(depthSwapchainInfo.swapchain, 0, &depthSwapchainImageCount, nullptr);
                
                if (result != XR_SUCCESS)
                {
                    setopenxrstatus(Failed_Status);
                    XR_MESSAGE("Failed to enumerate Depth Swapchain Images.");
                    return;
                }

                XrSwapchainImageBaseHeader *depthSwapchainImages = AllocateSwapchainImageData(depthSwapchainInfo.swapchain, SwapchainType::DEPTH, depthSwapchainImageCount);
                result = xrEnumerateSwapchainImages(depthSwapchainInfo.swapchain, depthSwapchainImageCount, &depthSwapchainImageCount, depthSwapchainImages);
                
                if (result != XR_SUCCESS)
                {
                    setopenxrstatus(Failed_Status);
                    XR_MESSAGE("Failed to enumerate Depth Swapchain Images.");
                    return;
                }

                // Per image in the swapchains, fill out a GraphicsAPI::ImageViewCreateInfo structure and create a color/depth image view.
                for (uint32_t j = 0; j < colorSwapchainImageCount; j++)
                {
                    ImageViewCreateInfo imageViewCI;
                    imageViewCI.image = GetSwapchainImage(colorSwapchainInfo.swapchain, j);
                    imageViewCI.type = ImageViewCreateInfo::Type::RTV;
                    imageViewCI.view = ImageViewCreateInfo::View::TYPE_2D;
                    imageViewCI.format = colorSwapchainInfo.swapchainFormat;
                    imageViewCI.aspect = ImageViewCreateInfo::Aspect::COLOR_BIT;
                    imageViewCI.baseMipLevel = 0;
                    imageViewCI.levelCount = 1;
                    imageViewCI.baseArrayLayer = 0;
                    imageViewCI.layerCount = 1;
                    colorSwapchainInfo.imageViews.push_back(CreateImageView(imageViewCI));
                }
                for (uint32_t j = 0; j < depthSwapchainImageCount; j++)
                {
                    ImageViewCreateInfo imageViewCI;
                    imageViewCI.image = GetSwapchainImage(depthSwapchainInfo.swapchain, j);
                    imageViewCI.type = ImageViewCreateInfo::Type::DSV;
                    imageViewCI.view = ImageViewCreateInfo::View::TYPE_2D;
                    imageViewCI.format = depthSwapchainInfo.swapchainFormat;
                    imageViewCI.aspect = ImageViewCreateInfo::Aspect::DEPTH_BIT;
                    imageViewCI.baseMipLevel = 0;
                    imageViewCI.levelCount = 1;
                    imageViewCI.baseArrayLayer = 0;
                    imageViewCI.layerCount = 1;
                    depthSwapchainInfo.imageViews.push_back(CreateImageView(imageViewCI));
                }

                // Display Swapchain Size
                std::stringstream sStr;
                sStr << "Swapchain Size Width:" << m_SwapChainWidth << " Height:" << m_SwapChainHeight;
                std::string sString;
                sString = sStr.str();
                XR_MESSAGE(sString.c_str());
            }
            XR_MESSAGE("-- Create Swapchains: End --------------------------------------");
        }

        void CreateAGKEnviroment()
        {  
            XR_MESSAGE("-- Create AGK Enviroment: Start -----------------------------------------------------------");

            if (m_SwapChainWidth > 0 && m_SwapChainHeight > 0)
            {    
                agk::SetOrientationAllowed( 0, 0, 1, 0 );
                agk::UpdateDeviceSize(m_SwapChainWidth, m_SwapChainHeight);
                #ifdef _WINDOWS_
                agk::SetWindowSize(m_SwapChainWidth, m_SwapChainHeight, 0);
                #endif
                #ifdef _ANDROID_
                agk::SetWindowSize(m_SwapChainWidth, m_SwapChainHeight, 1);
                #endif
                agk::SetScreenResolution(m_SwapChainWidth, m_SwapChainHeight);
                agk::SetVirtualResolution(m_SwapChainWidth, m_SwapChainHeight);
                agk::SetCameraAspect(1, float(m_SwapChainWidth/m_SwapChainHeight));                
                agk::SetSyncRate(FrameRate, 0);
                agk::SetCameraOffCenter(1, 1);

                if (m_MathObject == -1) m_MathObject = 9999;
                agk::CreateObjectBox(m_MathObject, 1, 1, 1);
                agk::SetObjectVisible(m_MathObject, 0);

                if (m_ScreenImage == -1) m_ScreenImage = 9999;
                agk::CreateRenderImage(m_ScreenImage, int(ScreenImageWidth), int(ScreenImageHeight), 0, 0);
            }

            XR_MESSAGE("-- Create AGK Enviroment: End -----------------------------------------------------");
        }

        void DestroyAGKEnviroment()
        {
            XR_MESSAGE("-- Destroy Resources: Start -----------------------------------");
            agk::SetCameraOffCenter(1, 0);
            if (m_MathObject != -1)
            {
                agk::DeleteObject(m_MathObject);
                m_MathObject = -1;
            }
            if (m_ScreenImage != -1)
            {
                agk::DeleteImage(m_ScreenImage);
                m_ScreenImage = -1;
            }
            XR_MESSAGE("-- Destroy Resources: End -----------------------------------");
        }

        void DestroySwapchains()
        {
            XR_MESSAGE("-- Destroy Swapchains: Start --------------------------------------");
            // Per view in the view configuration:
            for (size_t i = 0; i < m_viewConfigurationViews.size(); i++)
            {
                SwapchainInfo &colorSwapchainInfo = m_colorSwapchainInfos[i];
                SwapchainInfo &depthSwapchainInfo = m_depthSwapchainInfos[i];

                for (void *&imageView : colorSwapchainInfo.imageViews)
                {
                }
                for (void *&imageView : depthSwapchainInfo.imageViews)
                {
                }

                // Destroy the swapchains.
                XrResult result = xrDestroySwapchain(colorSwapchainInfo.swapchain);
                
                if (result != XR_SUCCESS)
                {
                    setopenxrstatus(Failed_Status);
                    XR_MESSAGE("Failed to destroy Color Swapchain");
                }

                result = xrDestroySwapchain(depthSwapchainInfo.swapchain);
                
                if (result != XR_SUCCESS)
                {
                    setopenxrstatus(Failed_Status);
                    XR_MESSAGE("Failed to destroy Depth Swapchain");
                }

            }

            XR_MESSAGE("-- Destroy Swapchains: End --------------------------------------");
        }

        void DestroyReferenceSpace()
        {
            XR_MESSAGE("-- Destroy Ref Space: Start -----------------------------------------");
            XrResult result = xrDestroySpace(m_WorldSpace);
            m_WorldUpToDate = false;

            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to destroy world space.");
            }

            result = xrDestroySpace(m_ViewSpace);
            m_ViewUpToDate = false;
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to destroy view space.");
            }

            XR_MESSAGE("-- Destroy Ref Space: End -----------------------------------------");
        }

        void DestroySession()
        {
            XR_MESSAGE("-- Destroy Session: Start --------------------------------------------------");
            XrResult result = xrDestroySession(m_session);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE("Failed to destroy Session.");
            }

            XR_MESSAGE("-- Destroy Session: End --------------------------------------------------");
        }

        void DestroyInstance()
        {
            XR_MESSAGE("-- Destroy Instance: Start ------------------------------------------");
            XrResult result = xrDestroyInstance(m_instance);
            
            if (result != XR_SUCCESS)
            {
                setopenxrstatus(Failed_Status);
                XR_MESSAGE( "Failed to destroy Instance.");
            }

            XR_MESSAGE("-- Destroy Instance: End ------------------------------------------");
        }
            
        void PreRenderFrame()
        {
            #ifdef _XR_DEBUGGING_
            XR_MESSAGE("-- Prerender Frame: Start ------------------------------------");
            #endif

            // Get the XrFrameState for timing and rendering info.
            XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
            XrResult result = xrWaitFrame(m_session, &frameWaitInfo, &m_frameState);
            
            if (result != XR_SUCCESS)
            {
                XR_MESSAGE("Failed to wait for XR Frame.");
            }

            PollActions(m_frameState.predictedDisplayTime);
            m_Updated = true;

            #ifdef _XR_DEBUGGING_
            XR_MESSAGE("-- Prerender Frame: End ----------------------------------------------");
            #endif
        }

        void RenderFrame()
        {
            #ifdef _XR_DEBUGGING_
            XR_MESSAGE("-- Render Frame: Start ------------------------------------");
            #endif

            // Render To Image
            #ifdef _XR_DEBUGGING_
            int iSize = int(sPrintLines.size());
            for (int a = 0; a < iSize; a++)
            {
                agk::Print(sPrintLines[a].c_str());
            }
            sPrintLines.resize(0);
            iSize = int(sPermPrintLines.size());
            for (int a = 0; a < iSize; a++)
            {
                agk::Print(sPermPrintLines[a].c_str());
            }
            #endif

            #ifdef _WINDOWS_
            agk::SetRenderToImage(m_ScreenImage, -1);
            agk::Update();
            agk::ClearScreen();
            agk::RenderShadowMap();
            agk::Render2DBack();
            agk::Render3D();
            agk::Render2DFront();
            agk::SetRenderToScreen();
            agk::SetCameraOffCenter(1, 0);
            agk::Sync(); // Also Updates AGK's Frame Per Second
            agk::SetCameraOffCenter(1, 1);
            #endif      

            #ifdef _ANDROID_
            agk::SetRenderToImage(m_ScreenImage, -1);
            agk::ClearScreen();
            agk::Sync(); // Also Updates AGK's Frame Per Second
            agk::SetRenderToScreen();
            #endif      

            XrResult result;

            if (m_Updated == false)
            {
                // Get the XrFrameState for timing and rendering info.
                XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
               result = xrWaitFrame(m_session, &frameWaitInfo, &m_frameState);
                
                if (result != XR_SUCCESS)
                {
                    XR_MESSAGE("Failed to wait for XR Frame.");
                }
            }

            // Tell the OpenXR compositor that the application is beginning the frame.
            XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
            result = xrBeginFrame(m_session, &frameBeginInfo);
            
            if (result != XR_SUCCESS)
            {
                XR_MESSAGE("Failed to begin the XR Frame.");
            }

            // Variables for rendering and layer composition.
            bool rendered = false;
            RenderLayerInfo renderLayerInfo;
            renderLayerInfo.predictedDisplayTime = m_frameState.predictedDisplayTime;

            // Check that the session is active and that we should render.
            bool sessionActive = (m_sessionState == XR_SESSION_STATE_SYNCHRONIZED || m_sessionState == XR_SESSION_STATE_VISIBLE || m_sessionState == XR_SESSION_STATE_FOCUSED);
            if (sessionActive && m_frameState.shouldRender)
            {
                // poll actions here because they require a predicted display time, which we've only just obtained.
                if (m_Updated == false) PollActions(m_frameState.predictedDisplayTime);

                // Render the stereo image and associate one of swapchain images with the XrCompositionLayerProjection structure.
                rendered = RenderLayer(renderLayerInfo);
                if (rendered)
                {
                    renderLayerInfo.layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader *>(&renderLayerInfo.layerProjection));
                }
            }

            // Tell OpenXR that we are finished with this frame; specifying its display time, environment blending and layers.
            XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
            frameEndInfo.displayTime = m_frameState.predictedDisplayTime;
            frameEndInfo.environmentBlendMode = m_environmentBlendMode;
            frameEndInfo.layerCount = static_cast<uint32_t>(renderLayerInfo.layers.size());
            frameEndInfo.layers = renderLayerInfo.layers.data();
            result = xrEndFrame(m_session, &frameEndInfo);
            
             m_Updated = false;

            if (result != XR_SUCCESS)
            {
                XR_MESSAGE("Failed to end the XR Frame.");
            }
            
            #ifdef _XR_DEBUGGING_
            XR_MESSAGE("-- Render Frame: End ----------------------------------------------");
            #endif
        }

        GLuint m_setFramebuffer = 0;
        GLuint m_vertexArray = 0;
        struct RenderLayerInfo;
        struct Viewport
        {
            float x;
            float y;
            float width;
            float height;
            float minDepth;
            float maxDepth;
        };
        struct Offset2D
        {
            int32_t x;
            int32_t y;
        };
        struct Extent2D
        {
            uint32_t width;
            uint32_t height;
        };
        struct Rect2D
        {
            Offset2D offset;
            Extent2D extent;
        };

        void SetRenderAttachments(void **colorViews, size_t colorViewCount, void *depthStencilView, uint32_t width, uint32_t height)
        {
            // Reset Framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &m_setFramebuffer);
            m_setFramebuffer = 0;
            glGenFramebuffers(1, &m_setFramebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, m_setFramebuffer);

            // Color
            for (size_t i = 0; i < colorViewCount; i++)
            {
                GLenum attachment = GL_COLOR_ATTACHMENT0;

                GLuint glColorView = (GLuint)(uint64_t)colorViews[i];
                const ImageViewCreateInfo &imageViewCI = imageViews[glColorView];

                if (imageViewCI.view == ImageViewCreateInfo::View::TYPE_2D_ARRAY)
                {
                    glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, (GLuint)(uint64_t)imageViewCI.image, imageViewCI.baseMipLevel, imageViewCI.baseArrayLayer, imageViewCI.layerCount);
                }
                else if (imageViewCI.view == ImageViewCreateInfo::View::TYPE_2D)
                {
                    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, (GLuint)(uint64_t)imageViewCI.image, imageViewCI.baseMipLevel);
                }
                else
                {
                    XR_MESSAGE("ERROR: OPENGL: Unknown ImageView View type.");
                }
            }

            // DepthStencil
            if (depthStencilView)
            {
                GLuint glDepthView = (GLuint)(uint64_t)depthStencilView;
                const ImageViewCreateInfo &imageViewCI = imageViews[glDepthView];

                if (imageViewCI.view == ImageViewCreateInfo::View::TYPE_2D_ARRAY)
                {
                    glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLuint)(uint64_t)imageViewCI.image, imageViewCI.baseMipLevel, imageViewCI.baseArrayLayer, imageViewCI.layerCount);
                }
                else if (imageViewCI.view == ImageViewCreateInfo::View::TYPE_2D)
                {
                    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, (GLuint)(uint64_t)imageViewCI.image, imageViewCI.baseMipLevel);
                }
                else
                {
                    XR_MESSAGE("ERROR: OPENGL: Unknown ImageView View type.");
                }
            }

            GLenum result = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
            if (result != GL_FRAMEBUFFER_COMPLETE)
            {
                XR_MESSAGE( "ERROR: OPENGL: Framebuffer is not complete.");
            }
        }
        bool RenderLayer(RenderLayerInfo &renderLayerInfo)
        {
            #ifdef _XR_DEBUGGING_
            XR_MESSAGE("-- Render Layer: Start -------------------------------------------");
            #endif

            std::vector<XrView> views(m_viewConfigurationViews.size(), {XR_TYPE_VIEW});
            XrViewState viewState{XR_TYPE_VIEW_STATE};  // Will contain information on whether the position and/or orientation is valid and/or tracked.
            XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
            viewLocateInfo.viewConfigurationType = m_viewConfiguration;
            viewLocateInfo.displayTime = renderLayerInfo.predictedDisplayTime;
            viewLocateInfo.space = m_WorldSpace;
            uint32_t viewCount = 0;
            XrResult result = xrLocateViews(m_session, &viewLocateInfo, &viewState, static_cast<uint32_t>(views.size()), &viewCount, views.data());
            if (result != XR_SUCCESS)
            {
                XR_MESSAGE("Failed to locate Views.");
                return false;
            }

            // Resize the layer projection views to match the view count. The layer projection views are used in the layer projection.
            renderLayerInfo.layerProjectionViews.resize(viewCount, {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW});

            // Per view in the view configuration:
            for (uint32_t i = 0; i < viewCount; i++)
            {
                SwapchainInfo &colorSwapchainInfo = m_colorSwapchainInfos[i];
                SwapchainInfo &depthSwapchainInfo = m_depthSwapchainInfos[i];
                uint32_t colorImageIndex = 0;
                uint32_t depthImageIndex = 0;
                XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
                result = xrAcquireSwapchainImage(colorSwapchainInfo.swapchain, &acquireInfo, &colorImageIndex);
                
                if (result != XR_SUCCESS)
                {
                    XR_MESSAGE("Failed to acquire Image from the Color Swapchain");
                }

                result = xrAcquireSwapchainImage(depthSwapchainInfo.swapchain, &acquireInfo, &depthImageIndex);
                
                if (result != XR_SUCCESS)
                {
                    XR_MESSAGE("Failed to acquire Image from the Depth Swapchain");
                }

                XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
                waitInfo.timeout = XR_INFINITE_DURATION;
                result = xrWaitSwapchainImage(colorSwapchainInfo.swapchain, &waitInfo);
                
                if (result != XR_SUCCESS)
                {
                    XR_MESSAGE("Failed to wait for Image from the Color Swapchain");
                }

                result = xrWaitSwapchainImage(depthSwapchainInfo.swapchain, &waitInfo);
                
                if (result != XR_SUCCESS)
                {
                    XR_MESSAGE("Failed to wait for Image from the Depth Swapchain");
                }

                const uint32_t &width = m_viewConfigurationViews[i].recommendedImageRectWidth;
                const uint32_t &height = m_viewConfigurationViews[i].recommendedImageRectHeight;

                Viewport viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
                Rect2D scissor    = {{(int32_t)0, (int32_t)0}, {width, height}};
                float nearZ       = m_Near;
                float farZ        = m_Far;

                renderLayerInfo.layerProjectionViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
                renderLayerInfo.layerProjectionViews[i].pose = views[i].pose;
                renderLayerInfo.layerProjectionViews[i].fov  = views[i].fov;
                renderLayerInfo.layerProjectionViews[i].subImage.swapchain = colorSwapchainInfo.swapchain;
                renderLayerInfo.layerProjectionViews[i].subImage.imageRect.offset.x = 0;
                renderLayerInfo.layerProjectionViews[i].subImage.imageRect.offset.y = 0;
                renderLayerInfo.layerProjectionViews[i].subImage.imageRect.extent.width = static_cast<int32_t>(width);
                renderLayerInfo.layerProjectionViews[i].subImage.imageRect.extent.height = static_cast<int32_t>(height);
                renderLayerInfo.layerProjectionViews[i].subImage.imageArrayIndex = 0; 

                // Begin Rendering
                {
                    glGenVertexArrays(1, &m_vertexArray);
                    glBindVertexArray(m_vertexArray);

                    glGenFramebuffers(1, &m_setFramebuffer);
                    glBindFramebuffer(GL_FRAMEBUFFER, m_setFramebuffer);
                }

                SetRenderAttachments(&colorSwapchainInfo.imageViews[colorImageIndex], 1, depthSwapchainInfo.imageViews[depthImageIndex], width, height);

                glViewportIndexedf((GLuint)i, viewport.x, viewport.y, viewport.width, viewport.height);
                #ifdef _WINDOWS_
                glDepthRangeIndexed((GLuint)i, (GLdouble)viewport.minDepth, (GLdouble)viewport.maxDepth);
                #endif
                glScissorIndexed((GLuint)i, (GLint)scissor.offset.x, (GLint)scissor.offset.y, (GLsizei)scissor.extent.width, (GLsizei)scissor.extent.height);

                float x, y, z, qw, qx, qy, qz;
                RightToLeftCoordinateSystem(views[i].pose.position.x, views[i].pose.position.y, views[i].pose.position.z,
                                            views[i].pose.orientation.w, views[i].pose.orientation.x, views[i].pose.orientation.y, views[i].pose.orientation.z,
                                            x, y, z, qw, qx, qy, qz);

                // AGK CAMERA POSITION      
                agk::SetCameraPosition(1, (x + m_Offset.x) * m_WorldScale, (y + m_Offset.y) * m_WorldScale, (z + m_Offset.z) * m_WorldScale); // z flipped for left-handed Y-up system, locals are already flipped.

                // Setting the camera rotation quaternion
                agk::SetCameraRotationQuat(1, qw, qx, qy, qz);

                // FOV
                float angleLeft  = views[i].fov.angleLeft;
                float angleRight = views[i].fov.angleRight;
                float angleUp    = views[i].fov.angleUp;
                float angleDown  = views[i].fov.angleDown;

                m_FieldOfViewVer = (atan(-angleDown) + atan(angleUp))    * (180.0f / float(M_PI));
                m_FieldOfViewHor = (atan(-angleLeft) + atan(angleRight)) * (180.0f / float(M_PI));

                agk::SetCameraRange(1, nearZ, farZ);
                //agk::SetScissor((float)scissor.offset.x, (float)scissor.offset.y, (float)scissor.extent.width, (float)scissor.extent.height);
                agk::SetScissor(0, 0, 0, 0);

                float tanLeft  = tanf(angleLeft);
                float tanRight = tanf(angleRight);
                float tanUp    = tanf(angleUp);
                float tanDown  = tanf(angleDown);

                // Calculate the near plane extents
                float left   = tanLeft  * nearZ;
                float right  = tanRight * nearZ;
                float bottom = tanDown  * nearZ;
                float top    = tanUp    * nearZ;

                agk::SetCameraBounds(1,
                    left,
                    right,
                    top,
                    bottom);

                if (i == 0)
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("openxr sync left eye");
                    #endif
                    //agk::Sync();

                    //agk::Update();
                    //agk::Render();
                    //agk::Swap();

                    agk::Update();
                    agk::ClearScreen();
                    agk::RenderShadowMap();
	                agk::Render2DBack();
	                agk::Render3D();
	                agk::Render2DFront();

                    //agk::Swap();
                }
                else /*if (i == 1)*/
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("openxr sync right eye");
                    #endif
                    //agk::Sync();

                    //agk::Update();
                    //agk::Render();
                    //agk::Swap();

                    agk::Update();
                    agk::ClearScreen();
                    agk::RenderShadowMap();
	                agk::Render2DBack();
	                agk::Render3D();
	                agk::Render2DFront();

                    //agk::Swap();
                }

                // End Rendering
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    glDeleteFramebuffers(1, &m_setFramebuffer);
                    m_setFramebuffer = 0;

                    glBindVertexArray(0);
                    glDeleteVertexArrays(1, &m_vertexArray);
                    m_vertexArray = 0;
                }

                // Give the swapchain image back to OpenXR, allowing the compositor to use the image.
                XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                result = xrReleaseSwapchainImage(colorSwapchainInfo.swapchain, &releaseInfo);
                
                if (result != XR_SUCCESS)
                {
                    XR_MESSAGE("Failed to release Image back to the Color Swapchain");
                }

                result = xrReleaseSwapchainImage(depthSwapchainInfo.swapchain, &releaseInfo);
                
                if (result != XR_SUCCESS)
                {
                    XR_MESSAGE("Failed to release Image back to the Depth Swapchain");  
                }

            }

            // Fill out the XrCompositionLayerProjection structure for usage with xrEndFrame().
            renderLayerInfo.layerProjection.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
            renderLayerInfo.layerProjection.space = m_WorldSpace; 
            renderLayerInfo.layerProjection.viewCount = static_cast<uint32_t>(renderLayerInfo.layerProjectionViews.size());
            renderLayerInfo.layerProjection.views = renderLayerInfo.layerProjectionViews.data();

            #ifdef _XR_DEBUGGING_
            XR_MESSAGE("-- Render Layer: End --------------------------------------");
            #endif
            return true;
        }

    public:
        #ifdef _ANDROID_
        // Processes the next command from the Android OS. It updates AndroidAppState.
        static void AndroidAppHandleCmd(struct android_app *app, int32_t cmd)
        {
            #ifdef _XR_DEBUGGING_
            XR_MESSAGE("-- AndroidAppHandleCmd: Start -------------------------------------------");
            #endif

            engine *appState = (engine *)app->userData;

            switch (cmd)
            {
            // There is no APP_CMD_CREATE. The ANativeActivity creates the application thread from onCreate().
            // The application thread then calls android_main().
                case APP_CMD_START:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_START");
                    #endif
                    break;
                }
                case APP_CMD_RESUME:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_RESUME");
                    #endif
                    appState->resumed = true;
                    if ( /*app_mode == 0 || app_mode == -2*/
                        getopenxrmode() == Paused_Mode     ||
                        getopenxrmode() == Not_Active_Mode )
                    {
                        onstart(Engine->app->activity);
                        resumeapp();
                        Engine->animating = 1;
                        if (Engine->accelerometerSensor != NULL) {
                            ASensorEventQueue_enableSensor(Engine->sensorEventQueue, Engine->accelerometerSensor);
                            int minRate = ASensor_getMinDelay(Engine->accelerometerSensor);
                            //if ( minRate < 16667 ) minRate = 16667;
                            ASensorEventQueue_setEventRate(Engine->sensorEventQueue, Engine->accelerometerSensor, minRate);
                        }
                        if (Engine->gyroSensor != NULL) {
                            ASensorEventQueue_enableSensor(Engine->sensorEventQueue, Engine->gyroSensor);
                            int minRate = ASensor_getMinDelay(Engine->gyroSensor);
                            //if ( minRate < 16667 ) minRate = 16667;
                            ASensorEventQueue_setEventRate(Engine->sensorEventQueue, Engine->gyroSensor, minRate);
                        }
                        if (Engine->proximitySensor != NULL) {
                            ASensorEventQueue_enableSensor(Engine->sensorEventQueue, Engine->proximitySensor);
                            int minRate = ASensor_getMinDelay(Engine->proximitySensor);
                            //if ( minRate < 16667 ) minRate = 16667;
                            ASensorEventQueue_setEventRate(Engine->sensorEventQueue, Engine->proximitySensor, minRate);
                        }
                        if (Engine->lightSensor != NULL) {
                            ASensorEventQueue_enableSensor(Engine->sensorEventQueue, Engine->lightSensor);
                            int minRate = ASensor_getMinDelay(Engine->lightSensor);
                            //if ( minRate < 16667 ) minRate = 16667;
                            ASensorEventQueue_setEventRate(Engine->sensorEventQueue, Engine->lightSensor, minRate);
                        }
                        if (Engine->magneticSensor != NULL) {
                            ASensorEventQueue_enableSensor(Engine->sensorEventQueue, Engine->magneticSensor);
                            int minRate = ASensor_getMinDelay(Engine->magneticSensor);
                            //if ( minRate < 16667 ) minRate = 16667;
                            ASensorEventQueue_setEventRate(Engine->sensorEventQueue, Engine->magneticSensor, minRate);
                        }
                        if (Engine->rotVectorSensor != NULL) {
                            ASensorEventQueue_enableSensor(Engine->sensorEventQueue, Engine->rotVectorSensor);
                            int minRate = ASensor_getMinDelay(Engine->rotVectorSensor);
                            //if ( minRate < 16667 ) minRate = 16667;
                            ASensorEventQueue_setEventRate(Engine->sensorEventQueue, Engine->rotVectorSensor, minRate);
                        }
                        //app_mode = -1;
                        setopenxrmode(Go_Mode);
                    }
                    break;
                }
                case APP_CMD_WINDOW_RESIZED:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_WINDOW_RESIZED");
                    #endif

                    break;
                }
                case APP_CMD_WINDOW_REDRAW_NEEDED:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_WINDOW_REDRAW_NEEDED");
                    #endif

                    break;
                }
                case APP_CMD_CONTENT_RECT_CHANGED:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_CONTENT_RECT_CHANGED");
                    #endif

                    break;
                }
                case APP_CMD_PAUSE:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_PAUSE");
                    #endif

                    appState->resumed = false;
                    if ( /*app_mode < 0*/ getopenxrmode() < Paused_Mode )
                    {
                        // app_mode = 0;
                        setopenxrmode(Paused_Mode);

                        if (Engine->accelerometerSensor != NULL)
                        {
                            ASensorEventQueue_disableSensor(Engine->sensorEventQueue, Engine->accelerometerSensor);
                        }
                        if (Engine->gyroSensor != NULL)
                        {
                            ASensorEventQueue_disableSensor(Engine->sensorEventQueue, Engine->gyroSensor);
                        }
                        if (Engine->proximitySensor != NULL)
                        {
                            ASensorEventQueue_disableSensor(Engine->sensorEventQueue, Engine->proximitySensor);
                        }
                        if (Engine->lightSensor != NULL)
                        {
                            ASensorEventQueue_disableSensor(Engine->sensorEventQueue, Engine->lightSensor);
                        }
                        if (Engine->magneticSensor != NULL)
                        {
                            ASensorEventQueue_disableSensor(Engine->sensorEventQueue, Engine->magneticSensor);
                        }
                        if (Engine->rotVectorSensor != NULL)
                        {
                            ASensorEventQueue_disableSensor(Engine->sensorEventQueue, Engine->rotVectorSensor);
                        }
                        Engine->animating = 0;
                        if ( /* initialised > 0 */ getwindowinitialised() == Window_Initialised ) pauseapp();
                    }
                    break;
                }
                case APP_CMD_STOP:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_STOP");
                    #endif

                    break;
                }
                case APP_CMD_DESTROY:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_DESTROY");
                    #endif

                    appState->nativeWindow = nullptr;
                    break;
                }
                case APP_CMD_INIT_WINDOW:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_INIT_WINDOW");
                    #endif

                    {
                        appState->nativeWindow = app->window;
        				//initialised = 1;
				        setwindowinitialised(Window_Initialised);
				        appState->animating = 1;
                    }
                    break;
                }
                case APP_CMD_SAVE_STATE:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_SAVE_STATE");
                    #endif 

                    break;
                }
                case APP_CMD_GAINED_FOCUS:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_GAINED_FOCUS");
                    #endif 

                    if ( /*app_mode == 1*/ getopenxrmode() == Lost_Focus_Mode)
                    {
                        onstart(Engine->app->activity);
                        resumeapp();
                        Engine->animating = 1;
                        if (Engine->accelerometerSensor != NULL)
                        {
                            ASensorEventQueue_enableSensor(Engine->sensorEventQueue, Engine->accelerometerSensor);
                            int minRate = ASensor_getMinDelay(Engine->accelerometerSensor);
                            //if ( minRate < 16667 ) minRate = 16667;
                            ASensorEventQueue_setEventRate(Engine->sensorEventQueue, Engine->accelerometerSensor, minRate);
                        }
                        if (Engine->gyroSensor != NULL)
                        {
                            ASensorEventQueue_enableSensor(Engine->sensorEventQueue, Engine->gyroSensor);
                            int minRate = ASensor_getMinDelay(Engine->gyroSensor);
                            //if ( minRate < 16667 ) minRate = 16667;
                            ASensorEventQueue_setEventRate(Engine->sensorEventQueue, Engine->gyroSensor, minRate);
                        }
                        if (Engine->proximitySensor != NULL)
                        {
                            ASensorEventQueue_enableSensor(Engine->sensorEventQueue, Engine->proximitySensor);
                            int minRate = ASensor_getMinDelay(Engine->proximitySensor);
                            //if ( minRate < 16667 ) minRate = 16667;
                            ASensorEventQueue_setEventRate(Engine->sensorEventQueue, Engine->proximitySensor, minRate);
                        }
                        if (Engine->lightSensor != NULL)
                        {
                            ASensorEventQueue_enableSensor(Engine->sensorEventQueue, Engine->lightSensor);
                            int minRate = ASensor_getMinDelay(Engine->lightSensor);
                            //if ( minRate < 16667 ) minRate = 16667;
                            ASensorEventQueue_setEventRate(Engine->sensorEventQueue, Engine->lightSensor, minRate);
                        }
                        if (Engine->magneticSensor != NULL)
                        {
                            ASensorEventQueue_enableSensor(Engine->sensorEventQueue, Engine->magneticSensor);
                            int minRate = ASensor_getMinDelay(Engine->magneticSensor);
                            //if ( minRate < 16667 ) minRate = 16667;
                            ASensorEventQueue_setEventRate(Engine->sensorEventQueue, Engine->magneticSensor, minRate);
                        }
                        if (Engine->rotVectorSensor != NULL)
                        {
                            ASensorEventQueue_enableSensor(Engine->sensorEventQueue, Engine->rotVectorSensor);
                            int minRate = ASensor_getMinDelay(Engine->rotVectorSensor);
                            //if ( minRate < 16667 ) minRate = 16667;
                            ASensorEventQueue_setEventRate(Engine->sensorEventQueue, Engine->rotVectorSensor, minRate);
                        }
                        //app_mode = -1;
                        setopenxrmode(Go_Mode);
                    }
                    break;
                }
                case APP_CMD_LOST_FOCUS:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_LOST_FOCUS");
                    #endif

                    if ( /*app_mode < 0*/ getopenxrmode() < Paused_Mode )
                    {
                        //app_mode = 1;
                        setopenxrmode(Lost_Focus_Mode);
                        if (Engine->accelerometerSensor != NULL)
                        {
                            ASensorEventQueue_disableSensor(Engine->sensorEventQueue, Engine->accelerometerSensor);
                        }
                        if (Engine->gyroSensor != NULL)
                        {
                            ASensorEventQueue_disableSensor(Engine->sensorEventQueue, Engine->gyroSensor);
                        }
                        if (Engine->proximitySensor != NULL)
                        {
                            ASensorEventQueue_disableSensor(Engine->sensorEventQueue, Engine->proximitySensor);
                        }
                        if (Engine->lightSensor != NULL)
                        {
                            ASensorEventQueue_disableSensor(Engine->sensorEventQueue, Engine->lightSensor);
                        }
                        if (Engine->magneticSensor != NULL)
                        {
                            ASensorEventQueue_disableSensor(Engine->sensorEventQueue, Engine->magneticSensor);
                        }
                        if (Engine->rotVectorSensor != NULL)
                        {
                            ASensorEventQueue_disableSensor(Engine->sensorEventQueue, Engine->rotVectorSensor);
                        }
                        Engine->animating = 0;
                        if ( /* initialised > 0 */ getwindowinitialised() == Window_Initialised ) pauseapp();
                    }
                    break;
                }
                case APP_CMD_CONFIG_CHANGED:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_CONFIG_CHANGED");
                    #endif 

                    {
                        AConfiguration *config2 = AConfiguration_new();
                        AConfiguration_fromAssetManager( config2, Engine->app->activity->assetManager);
                        int orien = AConfiguration_getOrientation( config2 );
                        int keyboard = AConfiguration_getKeyboard( config2 );
                        int keyshidden = AConfiguration_getKeysHidden( config2 );
                        LOGI("Config change, Orientation: %d, Keyboard: %d, KeysHidden: %d", orien, keyboard, keyshidden);
                        AConfiguration_delete( config2 );

                        if ( keyboard == ACONFIGURATION_KEYBOARD_QWERTY ) keyboardmode( 1 ); // physical
                        else keyboardmode( 2 ); // virtual
                    }
                    break;
                }
                case APP_CMD_LOW_MEMORY:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_LOW_MEMORY");
                    #endif

                    break;
                }
                case APP_CMD_TERM_WINDOW:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_TERM_WINDOW");
                    #endif

                    appState->nativeWindow = nullptr;
                    setopenxrstatus(Shutdown_Status);
                    break;
                }
                case APP_CMD_INPUT_CHANGED:
                {
                    #ifdef _XR_DEBUGGING_
                    XR_MESSAGE("APP_CMD_INPUT_CHANGED");
                    #endif 

                    break;
                }
                default:
                {
                    #ifdef _XR_DEBUGGING_
                    std::stringstream sStr;
                    sStr << "Default:" << cmd;
                    std::string sString = sStr.str();
                    XR_MESSAGE(sString.c_str());
                    #endif
                    return;
                }

            }
            
            #ifdef _XR_DEBUGGING_
            XR_MESSAGE("-- AndroidAppHandleCmd: End -------------------------------------------");
            #endif
        }
        #endif

    private:
        void PollEvents()
        {
            #ifdef _XR_DEBUGGING_
            XR_MESSAGE("-- Poll Events: Start ----------------------------------------");
            #endif
            // Poll OpenXR for a new event.
            XrEventDataBuffer eventData{XR_TYPE_EVENT_DATA_BUFFER};
            auto XrPollEvents = [&]() -> bool
            {
                eventData = {XR_TYPE_EVENT_DATA_BUFFER};
                return xrPollEvent(m_instance, &eventData) == XR_SUCCESS;
            };

            while (XrPollEvents())
            {
                switch (eventData.type)
                {
                    // Log the number of lost events from the runtime.
                    case XR_TYPE_EVENT_DATA_EVENTS_LOST:
                    {
                        XrEventDataEventsLost *eventsLost = reinterpret_cast<XrEventDataEventsLost *>(&eventData);

                        std::stringstream sStr;
                        sStr << "OPENXR: Events Lost: " << eventsLost->lostEventCount;
                        std::string sString = sStr.str();
                        XR_MESSAGE(sString);

                        break;
                    }
                    // Log that an instance loss is pending and shutdown the application.
                    case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                    {
                        XrEventDataInstanceLossPending *instanceLossPending = reinterpret_cast<XrEventDataInstanceLossPending *>(&eventData);
                        
                        std::stringstream sStr;
                        sStr << "OPENXR: Instance Loss Pending at: " << instanceLossPending->lossTime;
                        std::string sString = sStr.str();
                        XR_MESSAGE(sString);
                        
                        m_sessionRunning = false;
                        m_applicationRunning = false;
                        break;
                    }
                    // Log that the interaction profile has changed.
                    case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                    {
                        XrEventDataInteractionProfileChanged *interactionProfileChanged = reinterpret_cast<XrEventDataInteractionProfileChanged *>(&eventData);

                        std::stringstream sStr;
                        sStr << "OPENXR: Interaction Profile changed for Session: " << interactionProfileChanged->session;
                        std::string sString = sStr.str();
                        XR_MESSAGE(sString);

                        if (interactionProfileChanged->session != m_session)
                        {
                            XR_MESSAGE("XrEventDataInteractionProfileChanged for unknown Session");
                            break;
                        }
                        RecordCurrentBindings();
                        break;
                    }
                    // Log that there's a reference space change pending.
                    case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
                    {
                        XrEventDataReferenceSpaceChangePending *referenceSpaceChangePending = reinterpret_cast<XrEventDataReferenceSpaceChangePending *>(&eventData);

                        std::stringstream sStr;
                        sStr << "OPENXR: Reference Space Change pending for Session: " << referenceSpaceChangePending->session;
                        std::string sString = sStr.str();
                        XR_MESSAGE(sString);

                        m_ReferenceSpaceChanged = true;

                        if (referenceSpaceChangePending->session != m_session)
                        {
                            XR_MESSAGE("XrEventDataReferenceSpaceChangePending for unknown Session");
                            break;
                        }
                        break;
                    }
                    // Session State changes:
                    case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
                    {
                        XR_MESSAGE("OPENXR: XrEventDataSessionStateChanged...");

                        XrEventDataSessionStateChanged *sessionStateChanged = reinterpret_cast<XrEventDataSessionStateChanged *>(&eventData);
                        if (sessionStateChanged->session != m_session)
                        {
                            XR_MESSAGE("XrEventDataSessionStateChanged for unknown Session");
                            break;
                        }

                        if (sessionStateChanged->state == XR_SESSION_STATE_READY)
                        {
                            // SessionState is ready. Begin the XrSession using the XrViewConfigurationType.
                            XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                            sessionBeginInfo.primaryViewConfigurationType = m_viewConfiguration;
                            XrResult result = xrBeginSession(m_session, &sessionBeginInfo);
                            if (result != XR_SUCCESS) XR_MESSAGE("Failed to begin Session.");

                            m_sessionRunning = true;
                        }
                        if (sessionStateChanged->state == XR_SESSION_STATE_STOPPING)
                        {
                            // SessionState is stopping. End the XrSession.
                            XrResult result = xrEndSession(m_session);
                            if (result != XR_SUCCESS) XR_MESSAGE("Failed to end Session.");

                            m_sessionRunning = false;
                        }
                        if (sessionStateChanged->state == XR_SESSION_STATE_EXITING)
                        {
                            // SessionState is exiting. Exit the application.
                            m_sessionRunning = false;
                            m_applicationRunning = false;
                        }
                        if (sessionStateChanged->state == XR_SESSION_STATE_LOSS_PENDING)
                        {
                            // SessionState is loss pending. Exit the application.
                            // It's possible to try a reestablish an XrInstance and XrSession, but we will simply exit here.
                            m_sessionRunning = false;
                            m_applicationRunning = false;
                        }
                        // Store state for reference across the application.
                        m_sessionState = sessionStateChanged->state;
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            #ifdef _XR_DEBUGGING_
            XR_MESSAGE("-- Poll Events: End ----------------------------------------");
            #endif
        }
        void PollActions(XrTime predictedTime)
        {
            #ifdef _XR_DEBUGGING_
            XR_MESSAGE("-- Poll Actions: Start --------------------------------------");
            #endif 

            // Update our action set with up-to-date input data.
            // First, we specify the actionSet we are polling.
            XrActiveActionSet activeActionSet{};
            activeActionSet.actionSet = m_actionSet;
            activeActionSet.subactionPath = XR_NULL_PATH; // Global path if not using subactions

            // Now we sync the Actions to make sure they have current data.
            XrActionsSyncInfo actionsSyncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
            actionsSyncInfo.countActiveActionSets = 1;
            actionsSyncInfo.activeActionSets = &activeActionSet;
            XrResult result = xrSyncActions(m_session, &actionsSyncInfo);
            if (result != XR_SUCCESS) XR_MESSAGE("Failed to sync Actions.");
            XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
            actionStateGetInfo.action = m_palmPoseAction;

            // Update View & Player Input
            int count = 1;
            while (count > 0)
            {
                bool  rotated = false;
                float currentWorldX = m_World.position.x + m_Offset.x;
                float currentWorldY = m_World.position.y + m_Offset.y;
                float currentWorldZ = m_World.position.z + m_Offset.z;

                count = 0;
                
                XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
                result = xrLocateSpace(m_ViewSpace, m_WorldSpace, predictedTime, &spaceLocation);
                if (result != XR_SUCCESS)
                {
                    XR_MESSAGE("Failed to locate Space.");
                    return;
                }
                if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_TRACKED_BIT)    != 0 &&
                    (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT) != 0 )
                {
                    m_WorldRH = spaceLocation.pose;
            
                    RightToLeftCoordinateSystem(
                        spaceLocation.pose.position.x,
                        spaceLocation.pose.position.y,
                        spaceLocation.pose.position.z,
                        spaceLocation.pose.orientation.w,
                        spaceLocation.pose.orientation.x,
                        spaceLocation.pose.orientation.y,
                        spaceLocation.pose.orientation.z,
                        m_World.position.x,
                        m_World.position.y,
                        m_World.position.z,
                        m_World.orientation.w,
                        m_World.orientation.x,
                        m_World.orientation.y,
                        m_World.orientation.z);

                    QuaternionToEulerDegrees(
                        m_World.orientation.w,
                        m_World.orientation.x,
                        m_World.orientation.y,
                        m_World.orientation.z,
                        m_WorldDegrees.x,
                        m_WorldDegrees.y,
                        m_WorldDegrees.z);
                }

                // Move
                if (rotatex(1, predictedTime))
                {
                    rotated = true;
                    count++;
                }
                else if (rotatey(1, predictedTime))
                {
                    rotated = true;
                    count++;
                }
                else if (rotatez(1, predictedTime))
                {
                    rotated = true;
                    count++;
                }
                else
                {
                    if (movex(1, predictedTime))   count++;
                    if (movey(1, predictedTime))   count++;
                    if (movez(1, predictedTime))   count++;
                }

                if (rotated)
                {
                    XrSpaceLocation spaceLocation{ XR_TYPE_SPACE_LOCATION };
                    result = xrLocateSpace(m_ViewSpace, m_WorldSpace, predictedTime, &spaceLocation);
                    if (result != XR_SUCCESS)
                    {
                        XR_MESSAGE("Failed to locate Space.");
                        return;
                    }
                    if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_TRACKED_BIT) != 0 &&
                        (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT) != 0)
                    {
                        m_WorldRH = spaceLocation.pose;

                        RightToLeftCoordinateSystem(
                            spaceLocation.pose.position.x,
                            spaceLocation.pose.position.y,
                            spaceLocation.pose.position.z,
                            spaceLocation.pose.orientation.w,
                            spaceLocation.pose.orientation.x,
                            spaceLocation.pose.orientation.y,
                            spaceLocation.pose.orientation.z,
                            m_World.position.x,
                            m_World.position.y,
                            m_World.position.z,
                            m_World.orientation.w,
                            m_World.orientation.x,
                            m_World.orientation.y,
                            m_World.orientation.z);

                        QuaternionToEulerDegrees(
                            m_World.orientation.w,
                            m_World.orientation.x,
                            m_World.orientation.y,
                            m_World.orientation.z,
                            m_WorldDegrees.x,
                            m_WorldDegrees.y,
                            m_WorldDegrees.z);
                    }

                    float updatedWorldX = m_World.position.x;
                    float updatedWorldY = m_World.position.y;
                    float updatedWorldZ = m_World.position.z;

                    // Update Offset so current world position does not change
                    m_Offset.x = currentWorldX - updatedWorldX;
                    m_Offset.y = currentWorldY - updatedWorldY;
                    m_Offset.z = currentWorldZ - updatedWorldZ;
                }
            }

            // For each hand, get the pose state if possible.
            for (int i = 0; i < 2; i++)
            {
                // Specify the subAction Path.
                actionStateGetInfo.subactionPath = m_handPaths[i];
                XrResult result = xrGetActionStatePose(m_session, &actionStateGetInfo, &m_handPoseState[i]);
                if (result != XR_SUCCESS) XR_MESSAGE("Failed to get Pose State.");

                if (m_handPoseState[i].isActive)
                {
                    XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
                    XrResult result = xrLocateSpace(m_handPoseSpace[i], m_WorldSpace, predictedTime, &spaceLocation);
                    if (XR_UNQUALIFIED_SUCCESS(result))
                    {
                        bool bContinue = false;

                        if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_TRACKED_BIT) != 0 &&
                            (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT) != 0)
                        {
                            m_handPose[i] = spaceLocation.pose;

                            if (i == 0)
                            {  
                                m_LeftHand = true;
                                m_LeftResponding = true;

                                RightToLeftCoordinateSystem(
                                    m_handPose[i].position.x,
                                    m_handPose[i].position.y,
                                    m_handPose[i].position.z,
                                    m_handPose[i].orientation.w,
                                    m_handPose[i].orientation.x,
                                    m_handPose[i].orientation.y,
                                    m_handPose[i].orientation.z, 
                                    m_Left.position.x,
                                    m_Left.position.y,
                                    m_Left.position.z,
                                    m_Left.orientation.w,
                                    m_Left.orientation.x,
                                    m_Left.orientation.y,
                                    m_Left.orientation.z);

                                QuaternionToEulerDegrees(
                                    m_Left.orientation.w,
                                    m_Left.orientation.x,
                                    m_Left.orientation.y,
                                    m_Left.orientation.z,
                                    m_LeftDegrees.x,
                                    m_LeftDegrees.y,
                                    m_LeftDegrees.z);                                    

                                m_Left_Last = m_Left;

                                bContinue = true;
                            }
                            else if (i == 1)
                            { 
                                m_RightHand = true; 
                                m_RightResponding = true;

                                RightToLeftCoordinateSystem(
                                    m_handPose[i].position.x,
                                    m_handPose[i].position.y,
                                    m_handPose[i].position.z,
                                    m_handPose[i].orientation.w,
                                    m_handPose[i].orientation.x,
                                    m_handPose[i].orientation.y,
                                    m_handPose[i].orientation.z, 
                                    m_Right.position.x,
                                    m_Right.position.y,
                                    m_Right.position.z,
                                    m_Right.orientation.w,
                                    m_Right.orientation.x,
                                    m_Right.orientation.y,
                                    m_Right.orientation.z);

                                QuaternionToEulerDegrees(
                                    m_Right.orientation.w,
                                    m_Right.orientation.x,
                                    m_Right.orientation.y,
                                    m_Right.orientation.z,
                                    m_RightDegrees.x,
                                    m_RightDegrees.y,
                                    m_RightDegrees.z);   

                                m_Right_Last = m_Right;

                                bContinue = true;
                            }
                        }
                        else if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 ||
                                 (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0)
                        {
                            m_handPose[i] = spaceLocation.pose;

                            if (i == 0)
                            {
                                m_LeftHand = true;
                                m_LeftResponding = false;
                                m_Left = m_Left_Last;

                                bContinue = true;                             
                            }
                            else if (i == 1)
                            {
                                m_RightHand = true;
                                m_RightResponding = false;
                                m_Right = m_Right_Last;

                                bContinue = true;                             
                            }
                        }
                        else
                        {
                            if (i == 0)
                            {
                                m_LeftHand = false;
                                m_LeftResponding = false;
                                m_Left.position.x  = -1000;
                                m_Left.position.y  = -1000;
                                m_Left.position.z  = -1000;
                                m_Left.orientation.w = 1;
                                m_Left.orientation.x = 0;
                                m_Left.orientation.y = 0;
                                m_Left.orientation.z = 0;
                                m_LeftDegrees = {0.0f, 0.0f, 0.0f};

                                m_Left_Last = m_Left;

                                m_LeftHand_X_Button           = false;
                                m_LeftHand_Y_Button           = false;
                                m_LeftHand_Grip_Button        = 0.0f;
                                m_LeftHand_Thumbstick_Click   = false;
                                m_LeftHand_Menu_Button        = false;
                                m_LeftHand_Trigger            = 0.0f;
                                m_LeftHand_Thumbstick_X       = 0.0f;
                                m_LeftHand_Thumbstick_Y       = 0.0f;
                            }
                            else if (i == 1)
                            {
                                m_RightHand = false;
                                m_RightResponding = false;
                                m_Right.position.x  = -1000;
                                m_Right.position.y  = -1000;
                                m_Right.position.z  = -1000;
                                m_Right.orientation.w = 1;
                                m_Right.orientation.x = 0;
                                m_Right.orientation.y = 0;
                                m_Right.orientation.z = 0;
                                m_RightDegrees = {0.0f, 0.0f, 0.0f};

                                m_Right_Last = m_Right;

                                m_RightHand_A_Button          = false;
                                m_RightHand_B_Button          = false;
                                m_RightHand_Grip_Button       = 0.0f;
                                m_RightHand_Thumbstick_Click  = false;
                                m_RightHand_Home_Button       = false;
                                m_RightHand_Trigger           = 0.0f;
                                m_RightHand_Thumbstick_X      = 0.0f;
                                m_RightHand_Thumbstick_Y      = 0.0f;
                            }
                        }

                        if (bContinue)
                        {
                            if (i == 0)
                            {  
                                // Left Hand X Button
                                {
                                    XrActionStateBoolean leftHandXButtonState{XR_TYPE_ACTION_STATE_BOOLEAN};
                                    XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                                    actionStateGetInfo.action = m_LeftHand_X_Button_Action;
                                    m_LeftHand_X_Button = false;

                                    result = xrGetActionStateBoolean(m_session, &actionStateGetInfo, &leftHandXButtonState);
                                    if (result == XR_SUCCESS)
                                    {
                                        if (leftHandXButtonState.isActive == XR_TRUE)
                                        {
                                            m_LeftHand_X_Button = bool(leftHandXButtonState.currentState);
                                        }
                                    }
                                    else
                                    {
                                        XR_MESSAGE("Failed to get state for Left Hand X Button.");
                                    }        
                                }    

                                // Left Hand Y Button
                                {
                                    XrActionStateBoolean leftHandYButtonState{XR_TYPE_ACTION_STATE_BOOLEAN};
                                    XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                                    actionStateGetInfo.action = m_LeftHand_Y_Button_Action;
                                    m_LeftHand_Y_Button = false;

                                    result = xrGetActionStateBoolean(m_session, &actionStateGetInfo, &leftHandYButtonState);
                                    if (result == XR_SUCCESS)
                                    {
                                        if (leftHandYButtonState.isActive == XR_TRUE)
                                        {
                                            m_LeftHand_Y_Button = bool(leftHandYButtonState.currentState);
                                        }
                                    }
                                    else
                                    {
                                        XR_MESSAGE("Failed to get state for Left Hand B Button.");
                                    }
                                }

                                // Left Hand Trigger Button
                                {
                                    XrActionStateFloat leftHandTriggerButtonState{XR_TYPE_ACTION_STATE_FLOAT};
                                    XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                                    actionStateGetInfo.action = m_LeftHand_Trigger_Button_Action;
                                    m_LeftHand_Trigger = 0.0f;

                                    result = xrGetActionStateFloat(m_session, &actionStateGetInfo, &leftHandTriggerButtonState);
                                    if (result == XR_SUCCESS)
                                    {
                                        if (leftHandTriggerButtonState.isActive == XR_TRUE)
                                        {
                                            m_LeftHand_Trigger = float(leftHandTriggerButtonState.currentState);
                                        }
                                    }
                                    else
                                    {
                                        XR_MESSAGE("Failed to get state for Left Hand Trigger Button.");
                                    }
                                }

                                // Left Hand Grip Button
                                {
                                    XrActionStateFloat leftHandGripButtonState{XR_TYPE_ACTION_STATE_FLOAT};
                                    XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                                    actionStateGetInfo.action = m_LeftHand_Grip_Button_Action;
                                    m_LeftHand_Grip_Button = 0.0f;

                                    result = xrGetActionStateFloat(m_session, &actionStateGetInfo, &leftHandGripButtonState);
                                    if (result == XR_SUCCESS)
                                    {
                                        if (leftHandGripButtonState.isActive == XR_TRUE)
                                        {
                                            m_LeftHand_Grip_Button = float(leftHandGripButtonState.currentState);
                                        }
                                    }
                                    else
                                    {
                                        XR_MESSAGE("Failed to get state for Left Hand Grip Button.");
                                    }
                                }

                                // Left Hand Thumbstick Position
                                {
                                    XrActionStateVector2f leftHandThumbstickState{XR_TYPE_ACTION_STATE_VECTOR2F};
                                    XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                                    actionStateGetInfo.action = m_LeftHand_Thumbstick_Action;
                                    actionStateGetInfo.subactionPath = CreateXrPath("/user/hand/left");
                                    m_LeftHand_Thumbstick_X = 0.0f;
                                    m_LeftHand_Thumbstick_Y = 0.0f;

                                    result = xrGetActionStateVector2f(m_session, &actionStateGetInfo, &leftHandThumbstickState);
                                    if (result == XR_SUCCESS)
                                    {
                                        if (leftHandThumbstickState.isActive == XR_TRUE)
                                        {
                                            m_LeftHand_Thumbstick_X = float(leftHandThumbstickState.currentState.x);
                                            m_LeftHand_Thumbstick_Y = float(leftHandThumbstickState.currentState.y);
                                        }
                                    }
                                    else
                                    {
                                        XR_MESSAGE("Failed to get state for Left Hand Thumbstick.");
                                    }
                                }

                                // Left Hand Thumbstick Click
                                {
                                    XrActionStateBoolean leftHandThumbstickClickState{XR_TYPE_ACTION_STATE_BOOLEAN};  // Structure to hold the button state
                                    XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                                    actionStateGetInfo.action = m_LeftHand_Thumbstick_Click_Action;  // Reference to the thumbstick click action
                                    m_LeftHand_Thumbstick_Click = false;  // Initialize state to false

                                    result = xrGetActionStateBoolean(m_session, &actionStateGetInfo, &leftHandThumbstickClickState);
                                    if (result == XR_SUCCESS)
                                    {
                                        if (leftHandThumbstickClickState.isActive == XR_TRUE)
                                        {
                                            m_LeftHand_Thumbstick_Click = bool(leftHandThumbstickClickState.currentState);  // Set to current state (true/false)
                                        }
                                    }
                                    else
                                    {
                                        XR_MESSAGE("Failed to get state for Left Hand Thumbstick Click.");  // Proper error handling
                                    }
                                }

                                /*// Left Hand Menu Click
                                {
                                    XrActionStateBoolean leftHandMenuButtonState{XR_TYPE_ACTION_STATE_BOOLEAN};
                                    XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                                    actionStateGetInfo.action = m_LeftHand_Menu_Button_Action;
                                    m_LeftHand_Menu_Button = false;

                                    result = xrGetActionStateBoolean(m_session, &actionStateGetInfo, &leftHandMenuButtonState);
                                    if (result == XR_SUCCESS)
                                    {
                                        if (leftHandMenuButtonState.isActive == XR_TRUE)
                                        {
                                            m_LeftHand_Menu_Button = bool(leftHandMenuButtonState.currentState);
                                        }
                                    }
                                    else
                                    {
                                        XR_MESSAGE("Failed to get state for Left Hand Menu Button.");
                                    }        
                                }
                                */
                            } // Left hand
                            else if (i == 1)
                            { 
                                // Right Hand A Button
                                {
                                    XrActionStateBoolean rightHandAButtonState{XR_TYPE_ACTION_STATE_BOOLEAN};
                                    XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                                    actionStateGetInfo.action = m_RightHand_A_Button_Action;
                                    m_RightHand_A_Button = false;

                                    result = xrGetActionStateBoolean(m_session, &actionStateGetInfo, &rightHandAButtonState);
                                    if (result == XR_SUCCESS)
                                    {
                                        if (rightHandAButtonState.isActive == XR_TRUE)
                                        {
                                            m_RightHand_A_Button = bool(rightHandAButtonState.currentState);
                                        }
                                    }
                                    else
                                    {
                                        XR_MESSAGE("Failed to get state for Right Hand A Button.");
                                    }        
                                }    

                                // Right Hand B Button
                                {
                                    XrActionStateBoolean rightHandBButtonState{XR_TYPE_ACTION_STATE_BOOLEAN};
                                    XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                                    actionStateGetInfo.action = m_RightHand_B_Button_Action;
                                    m_RightHand_B_Button = false;

                                    result = xrGetActionStateBoolean(m_session, &actionStateGetInfo, &rightHandBButtonState);
                                    if (result == XR_SUCCESS)
                                    {
                                        if (rightHandBButtonState.isActive == XR_TRUE)
                                        {
                                            m_RightHand_B_Button = bool(rightHandBButtonState.currentState);
                                        }
                                    }
                                    else
                                    {
                                        XR_MESSAGE("Failed to get state for Right Hand B Button.");
                                    }
                                }

                                // Right Hand Trigger Button
                                {
                                    XrActionStateFloat rightHandTriggerButtonState{XR_TYPE_ACTION_STATE_FLOAT};
                                    XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                                    actionStateGetInfo.action = m_RightHand_Trigger_Button_Action;
                                    m_RightHand_Trigger = 0.0f;

                                    result = xrGetActionStateFloat(m_session, &actionStateGetInfo, &rightHandTriggerButtonState);
                                    if (result == XR_SUCCESS)
                                    {
                                        if (rightHandTriggerButtonState.isActive == XR_TRUE)
                                        {
                                            m_RightHand_Trigger = float(rightHandTriggerButtonState.currentState);
                                        }
                                    }
                                    else
                                    {
                                        XR_MESSAGE("Failed to get state for Right Hand Trigger Button.");
                                    }
                                }

                                // Right Hand Grip Button
                                {
                                    XrActionStateFloat rightHandGripButtonState{XR_TYPE_ACTION_STATE_FLOAT};
                                    XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                                    actionStateGetInfo.action = m_RightHand_Grip_Button_Action;
                                    m_RightHand_Grip_Button = 0.0f;

                                    result = xrGetActionStateFloat(m_session, &actionStateGetInfo, &rightHandGripButtonState);
                                    if (result == XR_SUCCESS)
                                    {
                                        if (rightHandGripButtonState.isActive == XR_TRUE)
                                        {
                                            m_RightHand_Grip_Button = float(rightHandGripButtonState.currentState);
                                        }
                                    }
                                    else
                                    {
                                        XR_MESSAGE("Failed to get state for Right Hand Grip Button.");
                                    }
                                }

                                // Right Hand Thumbstick Position
                                {
                                    XrActionStateVector2f rightHandThumbstickState{XR_TYPE_ACTION_STATE_VECTOR2F};
                                    XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                                    actionStateGetInfo.action = m_RightHand_Thumbstick_Action;
                                    actionStateGetInfo.subactionPath = CreateXrPath("/user/hand/right");
                                    m_RightHand_Thumbstick_X = 0.0f;
                                    m_RightHand_Thumbstick_Y = 0.0f;

                                    result = xrGetActionStateVector2f(m_session, &actionStateGetInfo, &rightHandThumbstickState);
                                    if (result == XR_SUCCESS)
                                    {
                                        if (rightHandThumbstickState.isActive == XR_TRUE)
                                        {
                                            m_RightHand_Thumbstick_X = float(rightHandThumbstickState.currentState.x);
                                            m_RightHand_Thumbstick_Y = float(rightHandThumbstickState.currentState.y);
                                        }
                                    }
                                    else
                                    {
                                        XR_MESSAGE("Failed to get state for Right Hand Thumbstick.");
                                    }
                                }

                                // Right Hand Thumbstick Click
                                {
                                    XrActionStateBoolean rightHandThumbstickClickState{XR_TYPE_ACTION_STATE_BOOLEAN};  // Structure to hold the button state
                                    XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                                    actionStateGetInfo.action = m_RightHand_Thumbstick_Click_Action;  // Reference to the thumbstick click action
                                    m_RightHand_Thumbstick_Click = false;  // Initialize state to false

                                    result = xrGetActionStateBoolean(m_session, &actionStateGetInfo, &rightHandThumbstickClickState);
                                    if (result == XR_SUCCESS)
                                    {
                                        if (rightHandThumbstickClickState.isActive == XR_TRUE)
                                        {
                                            m_RightHand_Thumbstick_Click = bool(rightHandThumbstickClickState.currentState);  // Set to current state (true/false)
                                        }
                                    }
                                    else
                                    {
                                        XR_MESSAGE("Failed to get state for Right Hand Thumbstick Click.");  // Proper error handling
                                    }
                                }
                            
                                /*// Right Hand Home Click
                                {
                                    XrActionStateBoolean rightHandHomeButtonState{XR_TYPE_ACTION_STATE_BOOLEAN};
                                    XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                                    actionStateGetInfo.action = m_RightHand_Home_Button_Action;
                                    m_RightHand_Home_Button = false;

                                    result = xrGetActionStateBoolean(m_session, &actionStateGetInfo, &rightHandHomeButtonState);
                                    if (result == XR_SUCCESS)
                                    {
                                        if (rightHandHomeButtonState.isActive == XR_TRUE)
                                        {
                                            m_RightHand_Home_Button = bool(rightHandHomeButtonState.currentState);
                                        }
                                    }
                                    else
                                    {
                                        XR_MESSAGE("Failed to get state for Right Hand Home Button.");
                                    }        
                                }
                                */
                            } // Right hand
                        }
                    }
                    else
                    {
                        m_handPoseState[i].isActive = false;
                        if (i == 0)
                        {   
                            m_LeftHand = false;
                            m_Left.position.x  = -1000;
                            m_Left.position.y  = -1000;
                            m_Left.position.z  = -1000;
                            m_Left.orientation.w = 1;
                            m_Left.orientation.x = 0;
                            m_Left.orientation.y = 0;
                            m_Left.orientation.z = 0;

                            m_Left_Last = m_Left; 

                            m_LeftHand_X_Button           = false;
                            m_LeftHand_Y_Button           = false;
                            m_LeftHand_Grip_Button        = 0.0f;
                            m_LeftHand_Thumbstick_Click   = false;
                            m_LeftHand_Menu_Button        = false;
                            m_LeftHand_Trigger            = 0.0f;
                            m_LeftHand_Thumbstick_X       = 0.0f;
                            m_LeftHand_Thumbstick_Y       = 0.0f;
                        } // Left hand
                        else
                        { 
                            m_Right.position.x  = -1000;
                            m_Right.position.y  = -1000;
                            m_Right.position.z  = -1000;
                            m_Right.orientation.w = 1;
                            m_Right.orientation.x = 0;
                            m_Right.orientation.y = 0;
                            m_Right.orientation.z = 0;

                            m_Right_Last = m_Right;

                            m_RightHand_A_Button          = false;
                            m_RightHand_B_Button          = false;
                            m_RightHand_Grip_Button       = 0.0f;
                            m_RightHand_Thumbstick_Click  = false;
                            m_RightHand_Home_Button       = false;
                            m_RightHand_Trigger           = 0.0f;
                            m_RightHand_Thumbstick_X      = 0.0f;
                            m_RightHand_Thumbstick_Y      = 0.0f;
                        } // Right hand
                    }
                }
            }

            // Haptic Feedback
            for (int i = 0; i < 2; i++)
            {
                m_Haptic[i] *= 0.5f;
                if (m_Haptic[i] < 0.01f) m_Haptic[i] = 0.0f;

                XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
                vibration.amplitude = m_Haptic[i];
                vibration.duration  = XR_MIN_HAPTIC_DURATION;
                vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

                XrHapticActionInfo hapticActionInfo{XR_TYPE_HAPTIC_ACTION_INFO};
                if (i == 0) hapticActionInfo.action = m_LeftHand_Buzz_Action;
                if (i == 1) hapticActionInfo.action = m_RightHand_Buzz_Action;
                hapticActionInfo.subactionPath = m_handPaths[i];
                XrResult result = xrApplyHapticFeedback(m_session, &hapticActionInfo, (XrHapticBaseHeader *)&vibration);
                
                if (result != XR_SUCCESS) XR_MESSAGE("Failed to apply haptic feedback.");
            }

            #ifdef _XR_DEBUGGING_
            XR_MESSAGE("-- Poll Actions: End --------------------------------------");
            #endif
        }
        void PollSystemEvents()
        {
            #ifdef _ANDROID_
            // This is done in the main loop...
            return; 
            XR_MESSAGE("-- Poll System Events: Start -----------------------------------");
            // Checks whether Android has requested that application should by destroyed.
            if (AndroidApp->destroyRequested != 0)
            {
                m_applicationRunning = false;
                return;
            }

            while (true)
            {
                // Poll and process the Android OS system events.
                struct android_poll_source *source = nullptr;
                int events = 0;
                // The timeout depends on whether the application is active.
                const int timeoutMilliseconds = (!Engine->resumed && !m_sessionRunning && AndroidApp->destroyRequested == 0) ? -1 : 0;
                if (ALooper_pollOnce(timeoutMilliseconds, nullptr, &events, (void **)&source) >= 0)
                {
                    if (source != nullptr)
                    {
                        source->process(AndroidApp, source);
                    }
                }
                else
                {
                    break;
                }
            }
            #endif
            XR_MESSAGE("-- Poll System Events: End -----------------------------------");
        }
    
    private:
        void QuaternionToEulerDegrees(float qw, float qx, float qy, float qz, float& x, float& y, float& z)
        {
            agk::SetObjectPosition(m_MathObject, 0, 0, 0);
            agk::SetObjectRotationQuat(m_MathObject, qw, qx, qy, qz);
            x = agk::GetObjectAngleX(m_MathObject);
            y = agk::GetObjectAngleY(m_MathObject);
            z = agk::GetObjectAngleZ(m_MathObject);
        }
        void EulerDegreesToQuaternions(float x, float y, float z, float& qw, float& qx, float& qy, float& qz)
        {
            agk::SetObjectPosition(m_MathObject, 0, 0, 0);
            agk::SetObjectRotation(m_MathObject, x, y, z);
            qw = agk::GetObjectQuatW(m_MathObject);
            qx = agk::GetObjectQuatX(m_MathObject);
            qy = agk::GetObjectQuatY(m_MathObject);
            qz = agk::GetObjectQuatZ(m_MathObject);
        }
        void RebuildReferenceSpace()
        {
            if (m_ViewUpToDate == false)
            {
                QuaternionToEulerDegrees(
                    m_ViewBuild.orientation.w,
                    m_ViewBuild.orientation.x,
                    m_ViewBuild.orientation.y,
                    m_ViewBuild.orientation.z,
                    m_ViewBuildDegrees.x,
                    m_ViewBuildDegrees.y,
                    m_ViewBuildDegrees.z);                    

                // Destroy the old reference space
                xrDestroySpace(m_ViewSpace);
                
                // Create Choosen View Reference Space
                XrReferenceSpaceCreateInfo ViewSpaceCreateInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
                ViewSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
                ViewSpaceCreateInfo.poseInReferenceSpace = m_ViewBuild;
                XrResult result = xrCreateReferenceSpace(m_session, &ViewSpaceCreateInfo, &m_ViewSpace);
                
                if (result != XR_SUCCESS)
                {
                    setopenxrstatus(Failed_Status);
                    XR_MESSAGE("Failed to create View reference space.");
                    return;
                }

                m_ViewUpToDate = true;
            }

            if (m_WorldUpToDate == false)
            {
                QuaternionToEulerDegrees(
                    m_WorldBuild.orientation.w,
                    m_WorldBuild.orientation.x,
                    m_WorldBuild.orientation.y,
                    m_WorldBuild.orientation.z,
                    m_WorldBuildDegrees.x,
                    m_WorldBuildDegrees.y,
                    m_WorldBuildDegrees.z);                    

                // Destroy the old reference space
                xrDestroySpace(m_WorldSpace);
                
                // Create Choosen World Reference Space
                XrReferenceSpaceCreateInfo worldSpaceCreateInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
                worldSpaceCreateInfo.referenceSpaceType = WorldType;
                worldSpaceCreateInfo.poseInReferenceSpace = m_WorldBuild;
                XrResult result = xrCreateReferenceSpace(m_session, &worldSpaceCreateInfo, &m_WorldSpace);
                
                if (result != XR_SUCCESS)
                {
                    setopenxrstatus(Failed_Status);
                    XR_MESSAGE("Failed to create world reference space.");
                    return;
                }
                
                m_WorldUpToDate = true;
            }
        }

        bool movex(float Time, XrTime predictedDisplayTime)
        {
            if (m_Move.x != 0.0f)
            {
                float Distance = m_Move.x;
                m_Move.x = 0.0f;

                // Locate the views from the view configuration within the (reference) space at the display time.
                std::vector<XrView> views(m_viewConfigurationViews.size(), {XR_TYPE_VIEW});
                XrViewState viewState{XR_TYPE_VIEW_STATE};  // Will contain information on whether the position and/or orientation is valid and/or tracked.
                XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
                viewLocateInfo.viewConfigurationType = m_viewConfiguration;
                viewLocateInfo.displayTime = predictedDisplayTime;
                viewLocateInfo.space = m_WorldSpace;
                uint32_t viewCount = 0;
                XrResult result = xrLocateViews(m_session, &viewLocateInfo, &viewState, static_cast<uint32_t>(views.size()), &viewCount, views.data());

                if (result == XR_SUCCESS)
                {
                    XrQuaternionf viewOrientation   = views[0].pose.orientation;
                    XrQuaternionf localOrientation  = m_WorldBuild.orientation;
                    XrQuaternionf finishOrientation;

                    QuaternionMultiply(viewOrientation.w, viewOrientation.x, viewOrientation.y, viewOrientation.z,
                                    localOrientation.w, localOrientation.x, localOrientation.y, localOrientation.z, true,
                                    finishOrientation.w,  finishOrientation.x,  finishOrientation.y,  finishOrientation.z);

                    float moveX = 1.0f, moveY = 0.0f, moveZ = 0.0f;
                    XrVector3f movement;

                    RotateVectorByQuaternion(finishOrientation.w, finishOrientation.x, finishOrientation.y, finishOrientation.z, 
                                            moveX, moveY, moveZ,
                                            movement.x, movement.y, movement.z);

                    float speed = Distance / Time;

                    m_WorldBuild.position.x += movement.x * speed;
                    if (m_FollowHMDY) m_WorldBuild.position.y += movement.y * speed;
                    m_WorldBuild.position.z += movement.z * speed;

                    m_WorldUpToDate = false;
                    RebuildReferenceSpace();
                    return true;
                }
            }
            return false;
        }
        bool movey(float Time, XrTime predictedDisplayTime)
        {
            if (m_Move.y != 0.0f)
            {
                float Distance = m_Move.y;
                m_Move.y = 0.0f;

                // Locate the views from the view configuration within the (reference) space at the display time.
                std::vector<XrView> views(m_viewConfigurationViews.size(), {XR_TYPE_VIEW});
                XrViewState viewState{XR_TYPE_VIEW_STATE};  // Will contain information on whether the position and/or orientation is valid and/or tracked.
                XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
                viewLocateInfo.viewConfigurationType = m_viewConfiguration;
                viewLocateInfo.displayTime = predictedDisplayTime;
                viewLocateInfo.space = m_WorldSpace;
                uint32_t viewCount = 0;
                XrResult result = xrLocateViews(m_session, &viewLocateInfo, &viewState, static_cast<uint32_t>(views.size()), &viewCount, views.data());

                if (result == XR_SUCCESS)
                {
                    XrQuaternionf viewOrientation   = views[0].pose.orientation;
                    XrQuaternionf localOrientation  = m_WorldBuild.orientation;
                    XrQuaternionf finishOrientation;

                    QuaternionMultiply(viewOrientation.w, viewOrientation.x, viewOrientation.y, viewOrientation.z,
                                    localOrientation.w, localOrientation.x, localOrientation.y, localOrientation.z, true,
                                    finishOrientation.w,  finishOrientation.x,  finishOrientation.y,  finishOrientation.z);

                    float moveX = 0.0f, moveY = 1.0f, moveZ = 0.0f;
                    XrVector3f movement;

                    RotateVectorByQuaternion(finishOrientation.w, finishOrientation.x, finishOrientation.y, finishOrientation.z, 
                                            moveX, moveY, moveZ,
                                            movement.x, movement.y, movement.z);

                    float speed = Distance / Time;

                    m_WorldBuild.position.x += movement.x * speed;
                    m_WorldBuild.position.y += movement.y * speed; // Primary movement along Y-axis
                    m_WorldBuild.position.z += movement.z * speed;

                    m_WorldUpToDate = false;
                    RebuildReferenceSpace();
                    return true;
                }
            }
            return false;
        }
        bool movez(float Time, XrTime predictedDisplayTime)
        {
            if (m_Move.z != 0.0f)
            {
                float Distance = m_Move.z;
                m_Move.z = 0.0f;

                // Locate the views from the view configuration within the (reference) space at the display time.
                std::vector<XrView> views(m_viewConfigurationViews.size(), {XR_TYPE_VIEW});
                XrViewState viewState{XR_TYPE_VIEW_STATE};  // Will contain information on whether the position and/or orientation is valid and/or tracked.
                XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
                viewLocateInfo.viewConfigurationType = m_viewConfiguration;
                viewLocateInfo.displayTime = predictedDisplayTime;
                viewLocateInfo.space = m_WorldSpace;
                uint32_t viewCount = 0;
                XrResult result = xrLocateViews(m_session, &viewLocateInfo, &viewState, static_cast<uint32_t>(views.size()), &viewCount, views.data());

                if (result == XR_SUCCESS)
                {
                    XrQuaternionf viewOrientation   = views[0].pose.orientation;
                    XrQuaternionf localOrientation  = m_WorldBuild.orientation;
                    XrQuaternionf finishOrientation;

                    QuaternionMultiply(viewOrientation.w, viewOrientation.x, viewOrientation.y, viewOrientation.z,
                                       localOrientation.w, localOrientation.x, localOrientation.y, localOrientation.z, true,
                                       finishOrientation.w,  finishOrientation.x,  finishOrientation.y,  finishOrientation.z);

                    float moveX = 0.0f, moveY = 0.0f, moveZ = 1.0f;
                    XrVector3f movement;

                    RotateVectorByQuaternion(finishOrientation.w, finishOrientation.x, finishOrientation.y, finishOrientation.z, 
                                            moveX, moveY, moveZ,
                                            movement.x, movement.y, movement.z);

                    float speed = Distance / Time;

                    m_WorldBuild.position.x += movement.x * speed;
                    if (m_FollowHMDY) m_WorldBuild.position.y += movement.y * speed;
                    m_WorldBuild.position.z += movement.z * speed;

                    m_WorldUpToDate = false;
                    RebuildReferenceSpace();
                    return true;
                }
            }
            return false;
        }        
        bool rotatex(float Time, XrTime predictedDisplayTime)
        {
            if (m_Rotate.x != 0.0f)
            {
                float amountX = m_Rotate.x;
                float amountY = 0;
                float amountZ = 0;

                m_Rotate.x = 0.0f;

                double newX, newY, newZ;

                RotatePointXYZ(m_WorldBuild.position.x, m_WorldBuild.position.y, m_WorldBuild.position.z,
                                amountX, amountY, amountZ,
                                newX, newY, newZ);

                m_WorldBuild.position.x = float(newX);
                m_WorldBuild.position.y = float(newY);
                m_WorldBuild.position.z = float(newZ);

                XrQuaternionf rotationQuaternion;
                EulerDegreesToQuaternions(amountX, amountY, amountZ, 
                    rotationQuaternion.w,
                    rotationQuaternion.x,
                    rotationQuaternion.y,
                    rotationQuaternion.z);

                XrQuaternionf finishOrientation;

                QuaternionMultiply(m_WorldBuild.orientation.w, m_WorldBuild.orientation.x, m_WorldBuild.orientation.y, m_WorldBuild.orientation.z,
                                    rotationQuaternion.w, rotationQuaternion.x, rotationQuaternion.y, rotationQuaternion.z, true,
                                    finishOrientation.w,  finishOrientation.x,  finishOrientation.y,  finishOrientation.z);
                            
                m_WorldBuild.orientation = finishOrientation;

                m_WorldUpToDate = false;
                RebuildReferenceSpace();

                return true;
            }

            return false;
        }
        bool rotatey(float Time, XrTime predictedDisplayTime)
        {
            if (m_Rotate.y != 0.0f)
            {
                float amountX = 0;
                float amountY = m_Rotate.y; 
                float amountZ = 0;

                m_Rotate.y = 0.0f;

                double newX, newY, newZ;

                RotatePointXYZ(m_WorldBuild.position.x, m_WorldBuild.position.y, m_WorldBuild.position.z,
                                amountX, amountY, amountZ,
                                newX, newY, newZ);

                m_WorldBuild.position.x = float(newX);
                m_WorldBuild.position.y = float(newY);
                m_WorldBuild.position.z = float(newZ);

                XrQuaternionf rotationQuaternion;
                EulerDegreesToQuaternions(amountX, amountY, amountZ, 
                    rotationQuaternion.w,
                    rotationQuaternion.x,
                    rotationQuaternion.y,
                    rotationQuaternion.z);

                XrQuaternionf finishOrientation;

                QuaternionMultiply(m_WorldBuild.orientation.w, m_WorldBuild.orientation.x, m_WorldBuild.orientation.y, m_WorldBuild.orientation.z,
                                    rotationQuaternion.w, rotationQuaternion.x, rotationQuaternion.y, rotationQuaternion.z, true,
                                    finishOrientation.w,  finishOrientation.x,  finishOrientation.y,  finishOrientation.z);
                        
                m_WorldBuild.orientation = finishOrientation;

                m_WorldUpToDate = false;
                RebuildReferenceSpace();

                return true;
            }

            return false;
        }        
        bool rotatez(float Time, XrTime predictedDisplayTime)
        {
            if (m_Rotate.z != 0.0f)
            {
                float amountX = 0;
                float amountY = 0;
                float amountZ = m_Rotate.z;

                m_Rotate.z = 0.0f;

                double newX, newY, newZ;

                RotatePointXYZ(m_WorldBuild.position.x, m_WorldBuild.position.y, m_WorldBuild.position.z,
                                amountX, amountY, amountZ,
                                newX, newY, newZ);

                m_WorldBuild.position.x = float(newX);
                m_WorldBuild.position.y = float(newY);
                m_WorldBuild.position.z = float(newZ);

                XrQuaternionf rotationQuaternion;
                EulerDegreesToQuaternions(amountX, amountY, amountZ, 
                    rotationQuaternion.w,
                    rotationQuaternion.x,
                    rotationQuaternion.y,
                    rotationQuaternion.z);

                XrQuaternionf finishOrientation;

                QuaternionMultiply(m_WorldBuild.orientation.w, m_WorldBuild.orientation.x, m_WorldBuild.orientation.y, m_WorldBuild.orientation.z,
                                    rotationQuaternion.w, rotationQuaternion.x, rotationQuaternion.y, rotationQuaternion.z, true,
                                    finishOrientation.w,  finishOrientation.x,  finishOrientation.y,  finishOrientation.z);
                            
                m_WorldBuild.orientation = finishOrientation;

                m_WorldUpToDate = false;
                RebuildReferenceSpace();

                return true;
            }

            return false;
        }

        void RecordCurrentBindings()
        {
            XR_MESSAGE("-- Record Current Bindings: Start -----------------------------------------------");
            if (m_session)
            {
                // now we are ready to:
                XrInteractionProfileState interactionProfile = {XR_TYPE_INTERACTION_PROFILE_STATE, 0, 0};
                // for each action, what is the binding?
                XrResult result = xrGetCurrentInteractionProfile(m_session, m_handPaths[0], &interactionProfile);
                if (result != XR_SUCCESS) XR_MESSAGE("Failed to get profile.");

                if (interactionProfile.interactionProfile)
                {
                    std::stringstream sStr;
                    sStr << "user/hand/left ActiveProfile " << FromXrPath(interactionProfile.interactionProfile).c_str();
                    std::string sString = sStr.str();
                    XR_MESSAGE(sString);
                }
                result = xrGetCurrentInteractionProfile(m_session, m_handPaths[1], &interactionProfile);
                if (result != XR_SUCCESS) XR_MESSAGE("Failed to get profile.");
                if (interactionProfile.interactionProfile)
                {
                    std::stringstream sStr;
                    sStr << "user/hand/right ActiveProfile " << FromXrPath(interactionProfile.interactionProfile).c_str();
                    std::string sString = sStr.str();
                    XR_MESSAGE(sString);
                }
            }

            XR_MESSAGE("-- Record Current Bindings: End -----------------------------------------------");
        }
        XrPath CreateXrPath(const char *path_string)
        {
            XrPath xrPath = XR_NULL_PATH;
            XrResult result = xrStringToPath(m_instance, path_string, &xrPath);
            if (result != XR_SUCCESS) XR_MESSAGE("Failed to create XrPath from string.");
            return xrPath;
        }
        std::string FromXrPath(XrPath path)
        {
            uint32_t strl;
            char text[XR_MAX_PATH_LENGTH];
            XrResult res;
            res = xrPathToString(m_instance, path, XR_MAX_PATH_LENGTH, &strl, text);
            std::string str;
            if (res == XR_SUCCESS)
            {
                str = text;
            }
            else
            {
                XR_MESSAGE("Failed to retrieve path.");
            }
            return str;
        }

        #ifdef _WINDOWS_
        XrVersion                 m_version  = XR_MAKE_VERSION(1, 0, 0);
        #endif
        #ifdef _ANDROID_
        XrVersion                 m_version  = XR_CURRENT_API_VERSION;
        #endif
        XrInstance                m_instance = XR_NULL_HANDLE;
        std::vector<const char *> m_activeAPILayers = {};
        std::vector<const char *> m_activeInstanceExtensions = {};
        std::vector<std::string>  m_apiLayers = {};
        std::vector<std::string>  m_instanceExtensions = {};

        XrFrameState              m_frameState{XR_TYPE_FRAME_STATE};

        XrFormFactor       m_formFactor       = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        XrSystemId         m_systemID         = {};
        XrSystemProperties m_systemProperties = {XR_TYPE_SYSTEM_PROPERTIES};
    
        XrSession      m_session            = {};
        XrSessionState m_sessionState       = XR_SESSION_STATE_UNKNOWN;
        bool           m_applicationRunning = true;
        bool           m_sessionRunning     = false;

        std::vector<XrViewConfigurationType> m_applicationViewConfigurations = {XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO};
        std::vector<XrViewConfigurationType> m_viewConfigurations;
        XrViewConfigurationType              m_viewConfiguration = XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;
        std::vector<XrViewConfigurationView> m_viewConfigurationViews;

        struct SwapchainInfo
        {
            XrSwapchain         swapchain = XR_NULL_HANDLE;
            int64_t             swapchainFormat = 0;
            std::vector<void *> imageViews;
        };
        std::vector<SwapchainInfo> m_colorSwapchainInfos = {};
        std::vector<SwapchainInfo> m_depthSwapchainInfos = {};

        std::vector<XrEnvironmentBlendMode> m_applicationEnvironmentBlendModes = {XR_ENVIRONMENT_BLEND_MODE_OPAQUE, XR_ENVIRONMENT_BLEND_MODE_ADDITIVE};
        std::vector<XrEnvironmentBlendMode> m_environmentBlendModes = {};
        XrEnvironmentBlendMode              m_environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM;

        XrSpace m_ViewSpace         = XR_NULL_HANDLE;
        XrSpace m_WorldSpace        = XR_NULL_HANDLE;
        struct RenderLayerInfo
        {
            XrTime predictedDisplayTime = 0;
            std::vector<XrCompositionLayerBaseHeader *> layers;
            XrCompositionLayerProjection layerProjection = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
            std::vector<XrCompositionLayerProjectionView> layerProjectionViews;
        };

        XrActionSet m_actionSet;

        XrAction    m_LeftHand_X_Button_Action;
        XrAction    m_LeftHand_Y_Button_Action;
        XrAction    m_LeftHand_Trigger_Button_Action;
        XrAction    m_LeftHand_Grip_Button_Action;
        XrAction    m_LeftHand_Thumbstick_Action;
        XrAction    m_LeftHand_Thumbstick_Click_Action;
        XrAction    m_LeftHand_Buzz_Action;
        //XrAction    m_LeftHand_Menu_Button_Action;

        XrAction    m_RightHand_A_Button_Action;
        XrAction    m_RightHand_B_Button_Action;
        XrAction    m_RightHand_Trigger_Button_Action;
        XrAction    m_RightHand_Grip_Button_Action;
        XrAction    m_RightHand_Thumbstick_Action;
        XrAction    m_RightHand_Thumbstick_Click_Action;
        XrAction    m_RightHand_Buzz_Action;
        //XrAction    m_RightHand_Home_Button_Action;

        float             m_viewHeightM = 0; // In STAGE space, viewHeightM should be 0. In LOCAL space, it should be offset downwards, below the viewer's initial position.
        XrAction          m_palmPoseAction;
        XrPath            m_handPaths[2] = {0, 0};
        XrSpace           m_handPoseSpace[2];
        XrActionStatePose m_handPoseState[2] = {{XR_TYPE_ACTION_STATE_POSE}, {XR_TYPE_ACTION_STATE_POSE}};
        XrPosef           m_handPose[2] =
        {
            {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, m_viewHeightM}},
            {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, m_viewHeightM}}
        };
        XrPosef           m_Left_Last  = m_IdentityPose;  // Remember Last Known Position 
        XrPosef           m_Right_Last = m_IdentityPose; 

        bool  m_Updated = false; // Update The Hand Position Early If Needed
        int   m_MathObject  = -1;
        int   m_ScreenImage = -1;

        #ifdef _XR_DEBUGGING_
        std::vector <std::string> sPrintLines;
         std::vector <std::string> sPermPrintLines;
        void print(std::string Text)
        {
            sPrintLines.push_back(Text);
        }
        void printperm(std::string Text, bool Clear)
        {
            if (Clear)      sPermPrintLines.resize(0);
            if (Text != "") sPermPrintLines.push_back(Text);
        }
        #endif
    };

    AGKOpenXR openxrapp;

    void  SetOpenXRScale(float Scale)
    {
        openxrapp.m_WorldScale = Scale;
    }
	float GetOpenXRScale()
    {
        return openxrapp.m_WorldScale;
    }
    void  SetCameraRange(float Near, float Far)
    {
        openxrapp.m_Near = Near;
        openxrapp.m_Far = Far;
    }
    void  SetFollowHMDLook(int Look)
    {
        if (Look) openxrapp.m_FollowHMDLook = true;
        else      openxrapp.m_FollowHMDLook = false;
    }
	void  SetFollowHMDY(int Setting)
    {
        if (Setting) openxrapp.m_FollowHMDY = true;
        else         openxrapp.m_FollowHMDY = false;
    }
     
    void  SetPosition(float X, float Y, float Z)
    {
        openxrapp.SetPosition(X / openxrapp.m_WorldScale, Y / openxrapp.m_WorldScale, Z / openxrapp.m_WorldScale);
    }
    void  SetRotation(float X, float Y, float Z)
    {
         openxrapp.SetRotation(X, Y, Z);
    }
    void  MoveX(float Distance)
    {
        openxrapp.m_Move.x = Distance / openxrapp.m_WorldScale;
    }
    void  MoveY(float Distance)
    {
        openxrapp.m_Move.y = Distance / openxrapp.m_WorldScale;
    }
    void  MoveZ(float Distance)
    {
        openxrapp.m_Move.z = Distance / openxrapp.m_WorldScale;
    }
    void  RotateX(float Amount)
    {
        openxrapp.m_Rotate.x = Amount;
    }
    void  RotateY(float Amount)
    {
        openxrapp.m_Rotate.y = Amount;
    }
    void  RotateZ(float Amount)
    {
        openxrapp.m_Rotate.z = Amount;
    }
    
    float GetX()
    {
        return (openxrapp.m_World.position.x + openxrapp.m_Offset.x) * openxrapp.m_WorldScale;
    }
	float GetY_Head()
    {
        return (openxrapp.m_World.position.y + openxrapp.m_Offset.y) * openxrapp.m_WorldScale;
    }
	float GetY_Feet()
    {
        return (openxrapp.m_WorldBuild.position.y + openxrapp.m_Offset.y) * openxrapp.m_WorldScale;
    }
    float GetZ()
    {
        return (openxrapp.m_World.position.z + openxrapp.m_Offset.z) * openxrapp.m_WorldScale;
    }
    float GetAngleX()
    {
        return openxrapp.m_WorldDegrees.x;
    }
    float GetAngleY()
    {
        return openxrapp.m_WorldDegrees.y;
    }
    float GetAngleZ()
    {
        return openxrapp.m_WorldDegrees.z;
    }
    float GetQuatW()
    {
        return openxrapp.m_World.orientation.w;
    }
    float GetQuatX()
    {
        return openxrapp.m_World.orientation.x;
    }
    float GetQuatY()
    {
         return openxrapp.m_World.orientation.y;
    }
    float GetQuatZ()
    {
         return openxrapp.m_World.orientation.z;
    }    
    
    int   LeftExists()
    {
         if (openxrapp.m_LeftHand) return 1;
        return 0;
    }  
    int   LeftResponding()
    {
        if (openxrapp.m_LeftResponding) return 1;
        return 0;
    }
    void  GetLeft(float *X, float *Y, float *Z, float *QuatW, float *QuatX, float *QuatY, float *QuatZ)
    {
        *X = (openxrapp.m_Left.position.x + openxrapp.m_Offset.x) * openxrapp.m_WorldScale;
        *Y = (openxrapp.m_Left.position.y + openxrapp.m_Offset.y) * openxrapp.m_WorldScale;
        *Z = (openxrapp.m_Left.position.z + openxrapp.m_Offset.z) * openxrapp.m_WorldScale;
        *QuatW = openxrapp.m_Left.orientation.w;
        *QuatX = openxrapp.m_Left.orientation.x;
        *QuatY = openxrapp.m_Left.orientation.y;
        *QuatZ = openxrapp.m_Left.orientation.z;
    }
    float GetLeftX()
    {
        return (openxrapp.m_Left.position.x + openxrapp.m_Offset.x) * openxrapp.m_WorldScale;
    }
	float GetLeftY()
    {
        return (openxrapp.m_Left.position.y + openxrapp.m_Offset.y) * openxrapp.m_WorldScale;
    }
	float GetLeftZ()
    {
        return (openxrapp.m_Left.position.z + openxrapp.m_Offset.z) * openxrapp.m_WorldScale;
    }
    float GetLeftQuatW()
    {
        return openxrapp.m_Left.orientation.w;
    }
    float GetLeftQuatX()
    {
         return openxrapp.m_Left.orientation.x;
    }
    float GetLeftQuatY()
    {
         return openxrapp.m_Left.orientation.y;
    }	
    float GetLeftQuatZ()
    {
         return openxrapp.m_Left.orientation.z;
    }
    float GetLeftAngleX()
    {
        return openxrapp.m_LeftDegrees.x;
    }
	float GetLeftAngleY()
    {
        return openxrapp.m_LeftDegrees.y;
    }
	float GetLeftAngleZ()
    {
        return openxrapp.m_LeftDegrees.z;
    }
    bool  GetLeftButtonXPressed()
    {
        return openxrapp.m_LeftHand_X_Button;
    }
    bool  GetLeftButtonYPressed()
    {
        return openxrapp.m_LeftHand_Y_Button;
    }
    float GetLeftButtonGripPressed()
    {
        return openxrapp.m_LeftHand_Grip_Button;
    }
    bool  GetLeftButtonThumbstickClickPressed()
    {
        return openxrapp.m_LeftHand_Thumbstick_Click;
    }
    bool  GetLeftButtonMenuPressed()
    {
        return openxrapp.m_LeftHand_Menu_Button;
    }
    float GetLeftTrigger()
    {
        return openxrapp.m_LeftHand_Trigger;
    }
    void  GetLeftThumbstick(float *X, float *Y)
    {
        *X  = openxrapp.m_LeftHand_Thumbstick_X;
        *Y  = openxrapp.m_LeftHand_Thumbstick_Y;
    }
    void  SetLeftHaptic(float Amount)
    {
        openxrapp.m_Haptic[0] = Amount;
    }
 
    int   RightExists()
    {
        if (openxrapp.m_RightHand) return 1;
        return 0;
    }
    int   RightResponding()
    {
        if (openxrapp.m_RightResponding) return 1;
        return 0;
    }
    void  GetRight(float *X, float *Y, float *Z, float *QuatW, float *QuatX, float *QuatY, float *QuatZ)
    {
        *X = (openxrapp.m_Right.position.x + openxrapp.m_Offset.x) * openxrapp.m_WorldScale;
        *Y = (openxrapp.m_Right.position.y + openxrapp.m_Offset.y) * openxrapp.m_WorldScale;
        *Z = (openxrapp.m_Right.position.z + openxrapp.m_Offset.z) * openxrapp.m_WorldScale;
        *QuatW = openxrapp.m_Right.orientation.w;
        *QuatX = openxrapp.m_Right.orientation.x;
        *QuatY = openxrapp.m_Right.orientation.y;
        *QuatZ = openxrapp.m_Right.orientation.z;
    }
    float GetRightX()
    {
        return (openxrapp.m_Right.position.x + openxrapp.m_Offset.x) * openxrapp.m_WorldScale;
    }
	float GetRightY()
    {
        return (openxrapp.m_Right.position.y + openxrapp.m_Offset.y) * openxrapp.m_WorldScale;
    }
	float GetRightZ()
    {
        return (openxrapp.m_Right.position.z + openxrapp.m_Offset.z) * openxrapp.m_WorldScale;
    }
	float GetRightAngleX()
    {
        return openxrapp.m_RightDegrees.x;
    }
	float GetRightAngleY()
    {
        return openxrapp.m_RightDegrees.y;
    }
	float GetRightAngleZ()
    {
        return openxrapp.m_RightDegrees.z;
    }
    float GetRightQuatW()
    {
        return openxrapp.m_Right.orientation.w;
    }
    float GetRightQuatX()
    {
        return openxrapp.m_Right.orientation.x;
    }
    float GetRightQuatY()
    {
        return openxrapp.m_Right.orientation.y;
    }	
    float GetRightQuatZ()
    {
        return openxrapp.m_Right.orientation.z;
    }
    bool  GetRightButtonAPressed()
    {
        return openxrapp.m_RightHand_A_Button;
    }
    bool  GetRightButtonBPressed()
    {
        return openxrapp.m_RightHand_B_Button;
    }
    float GetRightButtonGripPressed()
    {
        return openxrapp.m_RightHand_Grip_Button;
    }
    bool  GetRightButtonThumbstickClickPressed()
    {
        return openxrapp.m_RightHand_Thumbstick_Click;
    }
    bool  GetRightButtonHomePressed()
    {
        return openxrapp.m_RightHand_Home_Button;
    }
    float GetRightTrigger()
    {
        return openxrapp.m_RightHand_Trigger;
    }
    void  GetRightThumbstick(float *X, float *Y)
    {
        *X  = openxrapp.m_RightHand_Thumbstick_X;
        *Y  = openxrapp.m_RightHand_Thumbstick_Y;
    }
    void  SetRightHaptic(float Amount)
    {
        openxrapp.m_Haptic[1] = Amount;   
    }

    std::string GetSystemName()
    {
        return openxrapp.m_SystemName;
    }
    std::string GetSystemTracking()
    {
        return openxrapp.m_SystemTracking;
    }
	std::string GetSystemID()
    {
        return openxrapp.m_SystemID;
    }
    std::string GetVendorID()
    {
        return openxrapp.m_VendorID;
    }

    int   Begin(int ObjectID, int ScreenIMG)
    {
        return openxrapp.Begin(ObjectID, ScreenIMG);
    }
    void  UpdateOpenXR()
    {
        openxrapp.Update();
    }    
    void  Sync()
    {
        openxrapp.Sync();
    }
    void  End()
    {
        openxrapp.End();
    }
    bool  GetSpaceUpdateNeeded()
    {
        bool result = openxrapp.m_ReferenceSpaceChanged;
        openxrapp.m_ReferenceSpaceChanged = false;
        return result;
    }
}

#ifdef _WINDOWS_
void initopenxr(/*struct engine* engine*/)
{
    agkopenxr::openxrapp.InitOpenXR(/*engine*/);
}
void endopenxr()
{
   agkopenxr::openxrapp.End();
}
#endif

#ifdef _ANDROID_
void initopenxr(struct engine *engine)
{
    agkopenxr::openxrapp.InitOpenXR(engine);
}
void endopenxr()
{
    agkopenxr::openxrapp.End();
}
extern "C"
{
    void initopenxr_c(struct engine *engine)
    {
        initopenxr(engine);
    }
    void endopenxr_c()
    {
        endopenxr();
    }
}
#endif

#endif
