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

struct buffer {
    void* start;
    size_t length;
};

static int fd = -1;
int* rgb = NULL;
int* ybuf = NULL;
struct buffer* buffers = NULL;
static unsigned int n_buffers = 0;

int y1192_tbl[256];
int v1634_tbl[256];
int v833_tbl[256];
int u400_tbl[256];
int u2066_tbl[256];

jint Java_com_ford_openxc_webcam_NativeWebcam_prepareCamera(JNIEnv* env,
        jobject thiz, jstring deviceName, jint width, jint height);
void Java_com_ford_openxc_webcam_NativeWebcam_loadNextFrame(JNIEnv* env,
        jobject thiz, jobject bitmap);
jboolean Java_com_ford_openxc_webcam_NativeWebcam_cameraAttached(JNIEnv* env,
        jobject thiz);
void Java_com_ford_openxc_webcam_NativeWebcam_stopCamera(JNIEnv* env,
        jobject thiz);
