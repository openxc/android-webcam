#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#define LOG_TAG "NativeWebcamJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define ERROR_LOCAL -1
#define SUCCESS_LOCAL 0

typedef struct {
    void* start;
    size_t length;
} buffer;

static int DEVICE_DESCRIPTOR = -1;
int* RGB_BUFFER = NULL;
int* Y_BUFFER = NULL;
buffer* FRAME_BUFFERS = NULL;
static unsigned int BUFFER_COUNT = 0;

int YUV_TABLE[5][256];

jint Java_com_ford_openxc_webcam_NativeWebcam_startCamera(JNIEnv* env,
        jobject thiz, jstring deviceName, jint width, jint height);
void Java_com_ford_openxc_webcam_NativeWebcam_loadNextFrame(JNIEnv* env,
        jobject thiz, jobject bitmap);
jboolean Java_com_ford_openxc_webcam_NativeWebcam_cameraAttached(JNIEnv* env,
        jobject thiz);
void Java_com_ford_openxc_webcam_NativeWebcam_stopCamera(JNIEnv* env,
        jobject thiz);
