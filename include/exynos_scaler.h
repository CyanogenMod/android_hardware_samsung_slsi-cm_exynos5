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
 * \file      exynos_scaler.c
 * \brief     header file for Scaler HAL
 * \author    Sunyoung Kang (sy0816.kang@samsung.com)
 * \date      2013/02/01
 *
 * <b>Revision History: </b>
 * - 2013.02.01 : Sunyoung Kang (sy0816.kang@samsung.com) \n
 *   Create
 *
 * - 2013.04.26 : Cho KyongHo (pullip.cho@samsung.com \n
 *   Library rewrite
 *
 */

#ifndef _EXYNOS_SCALER_H_
#define _EXYNOS_SCALER_H_


#include <videodev2.h>
#include <stdbool.h>

#include "exynos_format.h"
#include "exynos_v4l2.h"

#define SC_DEV_NODE     "/dev/video"
#define SC_NODE(x)      (50 + x)

#define SC_NUM_OF_PLANES    (3)

// libgscaler's internal use only
typedef enum _HW_SCAL_ID {
    HW_SCAL0 = 4,
    HW_SCAL1,
    HW_SCAL2,
    HW_SCAL_MAX,
} HW_SCAL_ID;

// argument of non-blocking api
typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
    uint32_t fw;
    uint32_t fh;
    uint32_t format;
    uint32_t yaddr;
    uint32_t uaddr;
    uint32_t vaddr;
    uint32_t rot;
    uint32_t cacheable;
    uint32_t drmMode;
    uint32_t narrowRgb;
    int      acquireFenceFd;
    int      releaseFenceFd;
    int      mem_type;
} exynos_sc_img;

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Create libscaler handle
 *
 * \ingroup exynos_scaler
 *
 * \param dev_num
 *  scaler dev_num[in]
 *
 * \return
 * libscaler handle
 */
void *exynos_sc_create(int dev_num);

/*!
 * Destroy libscaler handle
 *
 * \ingroup exynos_scaler
 *
 * \param handle
 *   libscaler handle[in]
 */
int exynos_sc_destroy(void *handle);

/*!
 * Convert color space with presetup color format
 *
 * \ingroup exynos_scaler
 *
 * \param handle
 *   libscaler handle[in]
 *
 * \return
 *   error code
 */
int exynos_sc_convert(void *handle);

/*!
 * Set source format.
 *
 * \ingroup exynos_scaler
 *
 * \param handle
 *   libscaler handle[in]
 *
 * \param width
 *   image width[in]
 *
 * \param height
 *   image height[in]
 *
 * \param crop_left
 *   image left crop size[in]
 *
 * \param crop_top
 *   image top crop size[in]
 *
 * \param crop_width
 *   cropped image width[in]
 *
 * \param crop_height
 *   cropped image height[in]
 *
 * \param v4l2_colorformat
 *   color format[in]
 *
 * \param cacheable
 *   ccacheable[in]
 *
 * \param mode_drm
 *   mode_drm[in]
 *
 * \param premultiplied
 *   pre-multiplied format[in]
 *
 * \return
 *   error code
 */
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
    unsigned int premultiplied);

/*!
 * Set destination format.
 *
 * \ingroup exynos_scaler
 *
 * \param handle
 *   libscaler handle[in]
 *
 * \param width
 *   image width[in]
 *
 * \param height
 *   image height[in]
 *
 * \param crop_left
 *   image left crop size[in]
 *
 * \param crop_top
 *   image top crop size[in]
 *
 * \param crop_width
 *   cropped image width[in]
 *
 * \param crop_height
 *   cropped image height[in]
 *
 * \param v4l2_colorformat
 *   color format[in]
 *
 * \param cacheable
 *   ccacheable[in]
 *
 * \param mode_drm
 *   mode_drm[in]
 *
 * \param premultiplied
 *   pre-multiplied format[in]
 *
 * \return
 *   error code
 */
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
    unsigned int premultiplied);

/*!
 * Set source buffer
 *
 * \ingroup exynos_scaler
 *
 * \param handle
 *   libscaler handle[in]
 *
 * \param addr
 *   buffer pointer array[in]
 *
 * \param mem_type
 *   memory type[in]
 *
 * \param acquireFenceFd
 *   acquire fence fd for the buffer or -1[in]
 *
 * \return
 *   error code
 */

int exynos_sc_set_src_addr(
    void *handle,
    void *addr[SC_NUM_OF_PLANES],
    int mem_type,
    int acquireFenceFd);

/*!
 * Set destination buffer
 *
 * \param handle
 *   libscaler handle[in]
 *
 * \param addr
 *   buffer pointer array[in]
 *
 * \param mem_type
 *   memory type[in]
 *
 * \param acquireFenceFd
 *   acquire fence fd for the buffer or -1[in]
 *
 * \return
 *   error code
 */
int exynos_sc_set_dst_addr(
    void *handle,
    void *addr[SC_NUM_OF_PLANES],
    int mem_type,
    int acquireFenceFd);

/*!
 * Set rotation.
 *
 * \ingroup exynos_scaler
 *
 * \param handle
 *   libscaler handle[in]
 *
 * \param rot
 *   image rotation. It should be multiple of 90[in]
 *
 * \param flip_h
 *   image flip_horizontal[in]
 *
 * \param flip_v
 *   image flip_vertical[in]
 *
 * \return
 *   error code
 */
int exynos_sc_set_rotation(
    void *handle,
    int rot,
    int flip_h,
    int flip_v);

////// non-blocking /////

void *exynos_sc_create_exclusive(
    int dev_num,
    int allow_drm);

int exynos_sc_config_exclusive(
    void *handle,
    exynos_sc_img *src_img,
    exynos_sc_img *dst_img);

int exynos_sc_run_exclusive(
    void *handle,
    exynos_sc_img *src_img,
    exynos_sc_img *dst_img);

int exynos_sc_wait_frame_done_exclusive
(void *handle);

int exynos_sc_stop_exclusive
(void *handle);

int exynos_sc_free_and_close
(void *handle);

#ifdef __cplusplus
}
#endif

#endif /* _EXYNOS_SCALER_H_ */
