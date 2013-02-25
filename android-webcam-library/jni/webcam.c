#include "webcam.h"

int errnoexit(const char *s) {
    LOGE("%s error %d, %s", s, errno, strerror(errno));
    return ERROR_LOCAL;
}

int xioctl(int fd, int request, void *arg) {
    int r;

    do {
        r = ioctl(fd, request, arg);
    } while(-1 == r && EINTR == errno);

    return r;
}

void cache_tbl() {
    // prepare tbl
    for(int i = 0; i < 256; i++) {
        y1192_tbl[i] = 1192 * (i - 16);
        if(y1192_tbl[i] < 0) {
            y1192_tbl[i] = 0;
        }

        v1634_tbl[i] = 1634 * (i - 128);
        v833_tbl[i] = 833 * (i - 128);
        u400_tbl[i] = 400 * (i - 128);
        u2066_tbl[i] = 2066 * (i - 128);
    }
}

int open_device(const char* dev_name) {
    struct stat st;

    if(-1 == stat(dev_name, &st)) {
        LOGE("Cannot identify '%s': %d, %s", dev_name, errno, strerror(errno));
        return ERROR_LOCAL;
    }

    if(!S_ISCHR(st.st_mode)) {
        LOGE("%s is not a valid device", dev_name);
        return ERROR_LOCAL;
    }

    fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);

    if(EACCES == errno) {
        LOGE("Insufficient permissions on '%s': %d, %s", dev_name, errno, strerror(errno));
    }

    if(-1 == fd) {
        LOGE("Cannot open '%s': %d, %s", dev_name, errno, strerror(errno));
        return ERROR_LOCAL;
    }

    return SUCCESS_LOCAL;
}

int init_mmap() {
    struct v4l2_requestbuffers req;
    CLEAR(req);
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if(-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if(EINVAL == errno) {
            LOGE("device does not support memory mapping");
            return ERROR_LOCAL;
        } else {
            return errnoexit("VIDIOC_REQBUFS");
        }
    }

    if(req.count < 2) {
        LOGE("Insufficient buffer memory");
        return ERROR_LOCAL;
    }

    buffers = calloc(req.count, sizeof(*buffers));

    if(!buffers) {
        LOGE("Out of memory");
        return ERROR_LOCAL;
    }

    for(n_buffers = 0; n_buffers < req.count; ++n_buffers) { struct v4l2_buffer buf;
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if(-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            return errnoexit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =
            mmap(NULL ,
                    buf.length,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    fd, buf.m.offset);

        if(MAP_FAILED == buffers[n_buffers].start)
            return errnoexit("mmap");
    }

    return SUCCESS_LOCAL;
}


int init_device(int width, int height) {
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if(-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if(EINVAL == errno) {
            LOGE("not a valid V4L2 device");
            return ERROR_LOCAL;
        } else {
            return errnoexit("VIDIOC_QUERYCAP");
        }
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGE("device is not a video capture device");
        return ERROR_LOCAL;
    }

    if(!(cap.capabilities & V4L2_CAP_STREAMING)) {
        LOGE("device does not support streaming i/o");
        return ERROR_LOCAL;
    }

    CLEAR(cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;

        if(-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
            switch(errno) {
                case EINVAL:
                    break;
                default:
                    break;
            }
        }
    }

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;

    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if(-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
        return errnoexit("VIDIOC_S_FMT");

    min = fmt.fmt.pix.width * 2;
    if(fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if(fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    return init_mmap();
}

int start_capture() {
    unsigned int i;
    enum v4l2_buf_type type;

    for(i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            return errnoexit("VIDIOC_QBUF");
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(-1 == xioctl(fd, VIDIOC_STREAMON, &type))
        return errnoexit("VIDIOC_STREAMON");

    return SUCCESS_LOCAL;
}

void yuyv422_to_abgry(unsigned char *src, int width, int height) {
    if((!rgb || !ybuf)) {
        return;
    }

    int frameSize = width * height * 2;
    int* lrgb = &rgb[0];
    int* lybuf = &ybuf[0];
    for(int i = 0; i < frameSize; i += 4) {
        unsigned char y1, y2, u, v;
        y1 = src[i];
        u = src[i + 1];
        y2 = src[i + 2];
        v = src[i + 3];

        int y1192_1 = y1192_tbl[y1];
        int r1 = (y1192_1 + v1634_tbl[v]) >> 10;
        int g1 = (y1192_1 - v833_tbl[v] - u400_tbl[u]) >> 10;
        int b1 = (y1192_1 + u2066_tbl[u]) >> 10;

        int y1192_2 = y1192_tbl[y2];
        int r2 = (y1192_2 + v1634_tbl[v]) >> 10;
        int g2 = (y1192_2 - v833_tbl[v] - u400_tbl[u]) >> 10;
        int b2 = (y1192_2 + u2066_tbl[u]) >> 10;

        r1 = r1 > 255 ? 255 : r1 < 0 ? 0 : r1;
        g1 = g1 > 255 ? 255 : g1 < 0 ? 0 : g1;
        b1 = b1 > 255 ? 255 : b1 < 0 ? 0 : b1;
        r2 = r2 > 255 ? 255 : r2 < 0 ? 0 : r2;
        g2 = g2 > 255 ? 255 : g2 < 0 ? 0 : g2;
        b2 = b2 > 255 ? 255 : b2 < 0 ? 0 : b2;

        *lrgb++ = 0xff000000 | b1 << 16 | g1 << 8 | r1;
        *lrgb++ = 0xff000000 | b2 << 16 | g2 << 8 | r2;

        if(lybuf != NULL) {
            *lybuf++ = y1;
            *lybuf++ = y2;
        }
    }
}

void process_image(const void *p, int width, int height) {
    yuyv422_to_abgry((unsigned char *)p, width, height);
}

int read_frame(int width, int height) {
    struct v4l2_buffer buf;
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        switch(errno) {
            case EAGAIN:
                return 0;
            case EIO:
            default:
                return errnoexit("VIDIOC_DQBUF");
        }
    }

    assert(buf.index < n_buffers);

    process_image(buffers[buf.index].start, width, height);

    if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        return errnoexit("VIDIOC_QBUF");

    return 1;
}

int stop_capturing() {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(-1 != fd && -1 == xioctl(fd, VIDIOC_STREAMOFF, &type)) {
        return errnoexit("VIDIOC_STREAMOFF");
    }

    return SUCCESS_LOCAL;
}

int uninit_device() {
    for(unsigned int i = 0; i < n_buffers; ++i) {
        if(-1 == munmap(buffers[i].start, buffers[i].length)) {
            return errnoexit("munmap");
        }
    }

    free(buffers);
    return SUCCESS_LOCAL;
}

int close_device() {
    int result = SUCCESS_LOCAL;
    if(-1 != fd && -1 == close(fd)) {
        result = errnoexit("close");
    }
    fd = -1;
    return result;
}

void shutdown_camera() {
    stop_capturing();
    uninit_device();
    close_device();

    if(rgb) {
        free(rgb);
    }

    if(ybuf) {
        free(ybuf);
    }
}

int prepare_camera(const char* dev_name, int width, int height) {
    int result = open_device(dev_name);
    if(result == ERROR_LOCAL) {
        return result;
    }

    result = init_device(width, height);

    if(result == ERROR_LOCAL) {
        return result;
    }

    result = start_capture();
    if(result != SUCCESS_LOCAL) {
        stop_capturing();
        uninit_device();
        close_device();
        LOGE("device reset");
    } else {
        int area = width * height;
        rgb = (int*)malloc(sizeof(int) * area);
        ybuf = (int*)malloc(sizeof(int) * area);
    }

    return result;
}

bool camera_detected() {
    return fd != -1;
}

void process_camera(int width, int height) {
    if(!camera_detected()) {
        LOGE("No camera device available");
        return;
    }

    for(;;) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        int result = select(fd + 1, &fds, NULL, NULL, &tv);
        if(-1 == result) {
            if(EINTR == errno) {
                continue;
            }
            errnoexit("select");
        } else if(0 == result) {
            LOGE("select timeout");
        }

        if(read_frame(width, height) == 1) {
            break;
        }
    }
}

void Java_com_ford_openxc_webcam_NativeWebcam_loadNextFrame(JNIEnv* env,
        jobject thiz, jobject bitmap) {
    AndroidBitmapInfo info;
    int result;
    if((result = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", result);
        return;
    }

    process_camera(info.width, info.height);

    if(!rgb || !ybuf) {
        return;
    }

    if(info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format is not RGBA_8888 !");
        return;
    }

    void* pixels;
    if((result = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", result);
    }

    int* colors = (int*) pixels;
    int *lrgb = &rgb[0];
    for(int i = 0; i < info.width * info.height; i++) {
        *colors++ = *lrgb++;
    }

    AndroidBitmap_unlockPixels(env, bitmap);
}

// Functions below this point are basically just JNI wrappers, no significant
// work done here.

jint Java_com_ford_openxc_webcam_NativeWebcam_prepareCamera(JNIEnv* env,
        jobject thiz, jstring deviceName, jint width, jint height) {
    const char* dev_name = (*env)->GetStringUTFChars(env, deviceName, 0);
    int result = prepare_camera(dev_name, width, height);
    (*env)->ReleaseStringUTFChars(env, deviceName, dev_name);
    return result;
}

jboolean Java_com_ford_openxc_webcam_NativeWebcam_cameraAttached(JNIEnv* env,
        jobject thiz) {
    return camera_detected();
}

void Java_com_ford_openxc_webcam_NativeWebcam_load(JNIEnv* env,
        jobject thiz, jint width, jint height) {
}

void Java_com_ford_openxc_webcam_NativeWebcam_stopCamera(JNIEnv* env,
        jobject thiz) {
    shutdown_camera();
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    cache_tbl();
    return JNI_VERSION_1_6;
}
