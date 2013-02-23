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

#define LOG_TAG "WebCam"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define IMG_WIDTH 640
#define IMG_HEIGHT 480

#define ERROR_LOCAL -1
#define SUCCESS_LOCAL 0

struct buffer {
    void* start;
    size_t length;
};

static int fd = -1;
struct buffer* buffers = NULL;
static unsigned int n_buffers = 0;

int* rgb = NULL;
int* ybuf = NULL;

int yuv_tbl_ready = 0;
int y1192_tbl[256];
int v1634_tbl[256];
int v833_tbl[256];
int u400_tbl[256];
int u2066_tbl[256];

int errnoexit(const char* s);

int xioctl(int fd, int request, void* arg);

void shutdown_camera();
int open_device(const char* videoid);
int init_device(void);
int init_mmap(void);
int start_capture(void);

int read_frame(void);
void process_image(const void* p);
void process_camera();

int stop_capturing(void);
int uninit_device(void);
int close_device(void);

bool camera_detected();

void yuyv422_to_abgry(unsigned char* src);

jint Java_com_ford_openxc_webcam_WebcamManager_prepareCamera(JNIEnv* env,
        jobject thiz, jstring deviceName);
void Java_com_ford_openxc_webcam_WebcamManager_processCamera(JNIEnv* env,
        jobject thiz);
jboolean Java_com_ford_openxc_webcam_WebcamManager_cameraAttached(JNIEnv* env,
        jobject thiz);
void Java_com_ford_openxc_webcam_WebcamManager_stopCamera(JNIEnv* env,
        jobject thiz);
void Java_com_ford_openxc_webcam_WebcamManager_pixeltobmp(JNIEnv* env,
        jobject thiz, jobject bitmap);
