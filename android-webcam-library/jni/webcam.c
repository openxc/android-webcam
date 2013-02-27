#include "webcam.h"

int errnoexit(const char *s) {
    LOGE("%s error %d, %s", s, errno, strerror(errno));
    return ERROR_LOCAL;
}

/* Private: Repeat an ioctl call until it completes and is not interrupted by a
 * a signal.
 *
 * The ioctl may still succeed or fail, so do check the return status.
 *
 * fd - the file descriptor for the ioctl.
 * request - the type of IOCTL to request.
 * arg - the target argument for the ioctl.
 *
 * Returns the status of the ioctl when it completes.
 */
int xioctl(int fd, int request, void *arg) {
    int r;

    do {
        r = ioctl(fd, request, arg);
    } while(-1 == r && EINTR == errno);

    return r;
}

/* Private: Generate and cache the lookup table necessary to convert from YUV to
 * ARGB.
 */
void cache_yuv_lookup_table(int table[5][256]) {
    for(int i = 0; i < 256; i++) {
        table[0][i] = 1192 * (i - 16);
        if(table[0][i] < 0) {
            table[0][i] = 0;
        }

        table[1][i] = 1634 * (i - 128);
        table[2][i] = 833 * (i - 128);
        table[3][i] = 400 * (i - 128);
        table[4][i] = 2066 * (i - 128);
    }
}

/* Private: Open the video device at the named device node.
 *
 * dev_name - the path to a device, e.g. /dev/video0
 * fd - an output parameter to store the file descriptor once opened.
 *
 * Returns SUCCESS_LOCAL if the device was found and opened and ERROR_LOCAL if
 * an error occurred.
 */
int open_device(const char* dev_name, int* fd) {
    struct stat st;
    if(-1 == stat(dev_name, &st)) {
        LOGE("Cannot identify '%s': %d, %s", dev_name, errno, strerror(errno));
        return ERROR_LOCAL;
    }

    if(!S_ISCHR(st.st_mode)) {
        LOGE("%s is not a valid device", dev_name);
        return ERROR_LOCAL;
    }

    *fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if(-1 == *fd) {
        LOGE("Cannot open '%s': %d, %s", dev_name, errno, strerror(errno));
        if(EACCES == errno) {
            LOGE("Insufficient permissions on '%s': %d, %s", dev_name, errno,
                    strerror(errno));
        }
        return ERROR_LOCAL;
    }

    return SUCCESS_LOCAL;
}

/* Private: Initialize memory mapped buffers for video frames.
 */
int init_mmap(int fd) {
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

    FRAME_BUFFERS = calloc(req.count, sizeof(*FRAME_BUFFERS));
    if(!FRAME_BUFFERS) {
        LOGE("Out of memory");
        return ERROR_LOCAL;
    }

    for(BUFFER_COUNT = 0; BUFFER_COUNT < req.count; ++BUFFER_COUNT) {
        struct v4l2_buffer buf;
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = BUFFER_COUNT;

        if(-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
            return errnoexit("VIDIOC_QUERYBUF");
        }

        FRAME_BUFFERS[BUFFER_COUNT].length = buf.length;
        FRAME_BUFFERS[BUFFER_COUNT].start = mmap(NULL, buf.length,
                PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        if(MAP_FAILED == FRAME_BUFFERS[BUFFER_COUNT].start) {
            return errnoexit("mmap");
        }
    }

    return SUCCESS_LOCAL;
}


int init_device(int fd, int width, int height) {
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

    return init_mmap(fd);
}

int start_capture(int fd) {
    unsigned int i;
    enum v4l2_buf_type type;

    for(i = 0; i < BUFFER_COUNT; ++i) {
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

void yuyv422_to_argb(unsigned char *src, int width, int height, int* rgb_buffer,
        int* y_buffer) {
    if((!rgb_buffer || !y_buffer)) {
        return;
    }

    int frameSize = width * height * 2;
    int* lrgb = &rgb_buffer[0];
    int* lybuf = &y_buffer[0];
    for(int i = 0; i < frameSize; i += 4) {
        unsigned char y1, y2, u, v;
        y1 = src[i];
        u = src[i + 1];
        y2 = src[i + 2];
        v = src[i + 3];

        int y1192_1 = YUV_TABLE[0][y1];
        int r1 = (y1192_1 + YUV_TABLE[1][v]) >> 10;
        int g1 = (y1192_1 - YUV_TABLE[2][v] - YUV_TABLE[3][u]) >> 10;
        int b1 = (y1192_1 + YUV_TABLE[4][u]) >> 10;

        int y1192_2 = YUV_TABLE[0][y2];
        int r2 = (y1192_2 + YUV_TABLE[1][v]) >> 10;
        int g2 = (y1192_2 - YUV_TABLE[2][v] - YUV_TABLE[3][u]) >> 10;
        int b2 = (y1192_2 + YUV_TABLE[4][u]) >> 10;

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

int read_frame(int fd, buffer* frame_buffers, int width, int height, int* rgb_buffer,
        int* y_buffer) {
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

    assert(buf.index < BUFFER_COUNT);

    yuyv422_to_argb(frame_buffers[buf.index].start, width, height, rgb_buffer,
            y_buffer);

    if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        return errnoexit("VIDIOC_QBUF");

    return 1;
}

int stop_capturing(int fd) {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(-1 != fd && -1 == xioctl(fd, VIDIOC_STREAMOFF, &type)) {
        return errnoexit("VIDIOC_STREAMOFF");
    }

    return SUCCESS_LOCAL;
}

int uninit_device() {
    for(unsigned int i = 0; i < BUFFER_COUNT; ++i) {
        if(-1 == munmap(FRAME_BUFFERS[i].start, FRAME_BUFFERS[i].length)) {
            return errnoexit("munmap");
        }
    }

    free(FRAME_BUFFERS);
    return SUCCESS_LOCAL;
}

int close_device(int* fd) {
    int result = SUCCESS_LOCAL;
    if(-1 != *fd && -1 == close(*fd)) {
        result = errnoexit("close");
    }
    *fd = -1;
    return result;
}

void process_camera(int fd, buffer* frame_buffers, int width,
        int height, int* rgb_buffer, int* ybuf) {
    if(fd == -1) {
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

        if(read_frame(fd, frame_buffers, width, height, rgb_buffer, ybuf) == 1) {
            break;
        }
    }
}

void stop_camera() {
    stop_capturing(DEVICE_DESCRIPTOR);
    uninit_device();
    close_device(&DEVICE_DESCRIPTOR);

    if(RGB_BUFFER) {
        free(RGB_BUFFER);
    }

    if(Y_BUFFER) {
        free(Y_BUFFER);
    }
}

void Java_com_ford_openxc_webcam_NativeWebcam_loadNextFrame(JNIEnv* env,
        jobject thiz, jobject bitmap) {
    AndroidBitmapInfo info;
    int result;
    if((result = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed, error=%d", result);
        return;
    }

    if(info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format is not RGBA_8888 !");
        return;
    }

    int* colors;
    if((result = AndroidBitmap_lockPixels(env, bitmap, (void*)&colors)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed, error=%d", result);
    }

    if(!RGB_BUFFER || !Y_BUFFER) {
        LOGE("Unable to load frame, buffers not initialized");
        return;
    }

    process_camera(DEVICE_DESCRIPTOR, FRAME_BUFFERS, info.width, info.height,
            RGB_BUFFER, Y_BUFFER);

    int *lrgb = &RGB_BUFFER[0];
    for(int i = 0; i < info.width * info.height; i++) {
        *colors++ = *lrgb++;
    }

    AndroidBitmap_unlockPixels(env, bitmap);
}

jint Java_com_ford_openxc_webcam_NativeWebcam_startCamera(JNIEnv* env,
        jobject thiz, jstring deviceName, jint width, jint height) {
    const char* dev_name = (*env)->GetStringUTFChars(env, deviceName, 0);
    int result = open_device(dev_name, &DEVICE_DESCRIPTOR);
    (*env)->ReleaseStringUTFChars(env, deviceName, dev_name);
    if(result == ERROR_LOCAL) {
        return result;
    }

    result = init_device(DEVICE_DESCRIPTOR, width, height);
    if(result == ERROR_LOCAL) {
        return result;
    }

    result = start_capture(DEVICE_DESCRIPTOR);
    if(result != SUCCESS_LOCAL) {
        stop_camera();
        LOGE("Unable to start capture, resetting device");
    } else {
        int area = width * height;
        RGB_BUFFER = (int*)malloc(sizeof(int) * area);
        Y_BUFFER = (int*)malloc(sizeof(int) * area);
    }

    return result;
}

void Java_com_ford_openxc_webcam_NativeWebcam_stopCamera(JNIEnv* env,
        jobject thiz) {
    stop_camera();
}

jboolean Java_com_ford_openxc_webcam_NativeWebcam_cameraAttached(JNIEnv* env,
        jobject thiz) {
    return DEVICE_DESCRIPTOR != -1;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    cache_yuv_lookup_table(YUV_TABLE);
    return JNI_VERSION_1_6;
}
