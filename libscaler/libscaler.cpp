 /*
 * Copyright (C) 2013 The Android Open Source Project
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * \file      libscaler.cpp
 * \brief     source file for Scaler HAL
 * \author    Sunyoung Kang (sy0816.kang@samsung.com)
 * \date      2013/02/01
 *
 * <b>Revision History: </b>
 * - 2013.02.01 : Sunyoung Kang (sy0816.kang@samsung.com) \n
 *   Create
 * - 2013.04.10 : Cho KyongHo (pullip.cho@samsung.com) \n
 *   Refactoring
 *
 */


#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <system/graphics.h>
#define LOG_TAG "libexynosscaler"
#include <cutils/log.h>

#include "exynos_scaler.h"

//#define LOG_NDEBUG 0

#define SC_LOGERR(fmt, args...) ((void)ALOG(LOG_ERROR, LOG_TAG, "%s: " fmt " [%s]", __func__, ##args, strerror(errno)))
#define SC_LOGE(fmt, args...)     ((void)ALOG(LOG_ERROR, LOG_TAG, "%s: " fmt, __func__, ##args))
#define SC_LOGI(fmt, args...)     ((void)ALOG(LOG_INFO, LOG_TAG, "%s: " fmt, __func__, ##args))
#define SC_LOGI_IF(cond, fmt, args...)  do { \
    if (cond)                                \
        SC_LOGI(fmt, ##args);                \
    } while (0)
#define SC_LOGE_IF(cond, fmt, args...)  do { \
    if (cond)                                \
        SC_LOGE(fmt, ##args);                   \
    } while (0)
#define SC_LOG_ASSERT(cont, fmt, args...) ((void)ALOG_ASSERT(cond, "%s: " fmt, __func__, ##args))

#ifdef SC_DEBUG
#define SC_LOGD(args...) ((void)ALOG(LOG_INFO, LOG_TAG, ##args))
#define SC_LOGD_IF(cond, fmt, args...)  do { \
    if (cond)                                \
        SC_LOGD(fmt, ##args);                \
    } while (0)
#else
#define SC_LOGD(args...) do { } while (0)
#define SC_LOGD_IF(cond, fmt, args...)  do { } while (0)
#endif

#include "libscaler_obj_inc.cpp"

static CScaler *GetScaler(void *handle)
{
    if (handle == NULL) {
        SC_LOGE("NULL Scaler handle");
        return NULL;
    }

    CScaler *sc = reinterpret_cast<CScaler *>(handle);
    if (!sc->Valid()) {
        SC_LOGE("Invalid Scaler handle %p", handle);
        return NULL;
    }

    return sc;
}

void *exynos_sc_create(int dev_num)
{
    CScaler *sc;

    sc = new CScaler(dev_num);
    if (!sc) {
        SC_LOGE("Failed to allocate a Scaler handle for instance %d", dev_num);
        return NULL;
    }

    if (!sc->Valid()) {
        SC_LOGE("Failed to create a Scaler handle for instance %d", dev_num);
        delete sc;
        return NULL;
    }

    return reinterpret_cast<void *>(sc);
}

int exynos_sc_destroy(void *handle)
{
    CScaler *sc = GetScaler(handle);
    if (!sc)
        return -1;

    if (sc->Stop()) {
        ALOGE("Failed to stop Scaler (handle %p)", handle);
        return -1;
    }

    delete sc;

    return 0;
}

int exynos_sc_set_src_format(
        void        *handle,
        unsigned int width,
        unsigned int height,
        unsigned int crop_left,
        unsigned int crop_top,
        unsigned int crop_width,
        unsigned int crop_height,
        unsigned int v4l2_colorformat,
        unsigned int cacheable,
        unsigned int mode_drm,
        unsigned int premultiplied)
{
    CScaler *sc = GetScaler(handle);
    if (!sc)
        return -1;

    sc->SetImgFormat(CScaler::SC_SRC, width, height,
            crop_left, crop_top, crop_width, crop_height,
            v4l2_colorformat, cacheable, mode_drm, premultiplied);

    return 0;
}

int exynos_sc_set_dst_format(
        void        *handle,
        unsigned int width,
        unsigned int height,
        unsigned int crop_left,
        unsigned int crop_top,
        unsigned int crop_width,
        unsigned int crop_height,
        unsigned int v4l2_colorformat,
        unsigned int cacheable,
        unsigned int mode_drm,
        unsigned int premultiplied)
{
    CScaler *sc = GetScaler(handle);
    if (!sc)
        return -1;

    sc->SetImgFormat(CScaler::SC_DST, width, height,
                crop_left, crop_top, crop_width, crop_height,
                v4l2_colorformat, cacheable, mode_drm, premultiplied);

    return 0;
}

int exynos_sc_set_rotation(
        void *handle,
        int rot,
        int flip_h,
        int flip_v)
{
    CScaler *sc = GetScaler(handle);
    if (!sc)
        return -1;

    if (!sc->SetRotate(rot, flip_h, flip_v)) {
        SC_LOGE("Failed to set rotation degree %d, hflip %d, vflip %d",
                rot, flip_h, flip_v);
        return -1;
    }

    return 0;
}

int exynos_sc_set_src_addr(
        void *handle,
        void *addr[SC_NUM_OF_PLANES],
        int mem_type,
        int acquireFenceFd)
{
    CScaler *sc = GetScaler(handle);
    if (!sc)
        return -1;

    // acquireFenceFd is ignored by blocking mode
    sc->SetAddr(CScaler::SC_SRC, addr, mem_type);

    return 0;
}

int exynos_sc_set_dst_addr(
        void *handle,
        void *addr[SC_NUM_OF_PLANES],
        int mem_type,
        int acquireFenceFd)
{
    CScaler *sc = GetScaler(handle);
    if (!sc)
        return -1;

    // acquireFenceFd is ignored by blocking mode
    sc->SetAddr(CScaler::SC_DST, addr, mem_type);

    return 0;
}

int exynos_sc_convert(void *handle)
{
    CScaler *sc = GetScaler(handle);
    if (!sc)
        return -1;

    return sc->Start();
}

void *exynos_sc_create_exclusive(
        int dev_num,
        int allow_drm
        )
{
    CScaler *sc;

    sc = new CScaler(dev_num, allow_drm);
    if (!sc) {
        SC_LOGE("Failed to allocate a Scaler handle for instance %d", dev_num);
        return NULL;
    }

    if (!sc->Valid()) {
        SC_LOGE("Failed to create a Scaler handle for instance %d", dev_num);
        delete sc;
        return NULL;
    }

    SC_LOGD("Scaler %d is successfully created", dev_num);
    return reinterpret_cast<void *>(sc);
}

int exynos_sc_free_and_close(void *handle)
{
    return exynos_sc_destroy(handle);
}

int exynos_sc_stop_exclusive(void *handle)
{
    CScaler *sc = GetScaler(handle);
    if (!sc)
        return -1;

    if (sc->Stop()) {
        SC_LOGE("Failed to stop Scaler (handle %p)", handle);
        return -1;
    }

    return 0;
}

static void rotateValueHAL2SC(unsigned int transform,
    unsigned int *rotate,
    unsigned int *hflip,
    unsigned int *vflip)
{
    int rotate_flag = transform & 0x7;
    *rotate = 0;
    *hflip = 0;
    *vflip = 0;

    switch (rotate_flag) {
    case HAL_TRANSFORM_ROT_90:
        *rotate = 90;
        break;
    case HAL_TRANSFORM_ROT_180:
        *rotate = 180;
        break;
    case HAL_TRANSFORM_ROT_270:
        *rotate = 270;
        break;
    case HAL_TRANSFORM_FLIP_H | HAL_TRANSFORM_ROT_90:
        *rotate = 90;
        *vflip = 1; /* set vflip to compensate the rot & flip order. */
        break;
    case HAL_TRANSFORM_FLIP_V | HAL_TRANSFORM_ROT_90:
        *rotate = 90;
        *hflip = 1; /* set hflip to compensate the rot & flip order. */
        break;
    case HAL_TRANSFORM_FLIP_H:
        *hflip = 1;
         break;
    case HAL_TRANSFORM_FLIP_V:
        *vflip = 1;
         break;
    default:
        break;
    }
}

int exynos_sc_config_exclusive(
    void *handle,
    exynos_sc_img *src_img,
    exynos_sc_img *dst_img)
{
    CScaler *sc = GetScaler(handle);
    if (!sc)
        return -1;

    if ((src_img->drmMode && !sc->IsDRMAllowed()) ||
        (src_img->drmMode != dst_img->drmMode)) {
        SC_LOGE("Invalid DRM state request for Scaler%d (s=%d d=%d)",
                sc->GetScalerID(), src_img->drmMode, dst_img->drmMode);
        return -1;
    }

    unsigned int rot, flip_h, flip_v;
    rotateValueHAL2SC(dst_img->rot, &rot, &flip_h, &flip_v);
    if (!sc->SetRotate(rot, flip_h, flip_v)) {
        SC_LOGE("Failed to set rotation degree %d, hflip %d, vflip %d",
                rot, flip_h, flip_v);
        return -1;
    }

    int32_t src_color_space = HAL_PIXEL_FORMAT_2_V4L2_PIX(src_img->format);
    int32_t dst_color_space = HAL_PIXEL_FORMAT_2_V4L2_PIX(dst_img->format);

    sc->SetImgFormat(CScaler::SC_SRC, src_img->fw, src_img->fh,
            src_img->x, src_img->y, src_img->w, src_img->h,
            src_color_space, src_img->cacheable, src_img->drmMode);
    // narrow rgb ????
    sc->SetImgFormat(CScaler::SC_DST, dst_img->fw, dst_img->fh,
            dst_img->x, dst_img->y, dst_img->w, dst_img->h,
            dst_color_space, dst_img->cacheable, dst_img->drmMode);

    return 0;
}

int exynos_sc_run_exclusive(
    void *handle,
    exynos_sc_img *src_img,
    exynos_sc_img *dst_img)
{
    CScaler *sc = GetScaler(handle);
    if (!sc)
        return -1;

    void *addr[SC_NUM_OF_PLANES];

    addr[0] = (void *)src_img->yaddr;
    addr[1] = (void *)src_img->uaddr;
    addr[2] = (void *)src_img->vaddr;
    sc->SetAddr(CScaler::SC_SRC, addr, src_img->mem_type, src_img->acquireFenceFd);

    addr[0] = (void *)dst_img->yaddr;
    addr[1] = (void *)dst_img->uaddr;
    addr[2] = (void *)dst_img->vaddr;
    sc->SetAddr(CScaler::SC_DST, addr, dst_img->mem_type, dst_img->acquireFenceFd);

    if (sc->SetCtrl())
        return -1;

    if (sc->SetFormat())
        return -1;

    if (sc->ReqBufs())
        return -1;

    int fdReleaseFence[CScaler::SC_NUM_EDGES];

    if (sc->QBuf(fdReleaseFence))
        return -1;

    if (sc->StreamOn()) {
        close(fdReleaseFence[CScaler::SC_SRC]);
        close(fdReleaseFence[CScaler::SC_DST]);
        return -1;
    }

    src_img->releaseFenceFd = fdReleaseFence[CScaler::SC_SRC];
    dst_img->releaseFenceFd = fdReleaseFence[CScaler::SC_DST];

    return 0;
}

int exynos_sc_wait_frame_done_exclusive(
        void *handle)
{
    CScaler *sc = GetScaler(handle);
    if (!sc)
        return -1;

    return sc->DQBuf();
}
