# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:= $(call my-dir)
# HAL module implemenation, not prelinked and stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := liblog libcutils libEGL libGLESv1_CM libhardware \
	libhardware_legacy libion_exynos libutils libsync libexynosgscaler \
	libexynosv4l2 libMcClient libexynosutils
ifeq ($(BOARD_USES_HWC_SERVICES),true)
	LOCAL_SHARED_LIBRARIES += libExynosHWCService
	LOCAL_CFLAGS += -DHWC_SERVICES

ifeq ($(BOARD_USES_WFD),true)
	LOCAL_CFLAGS += -DUSES_WFD
	LOCAL_SHARED_LIBRARIES += libfimg
	LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi-cm/exynos/libfimg4x
endif

ifeq ($(BOARD_USE_S3D_SUPPORT),true)
	LOCAL_CFLAGS += -DS3D_SUPPORT
endif
endif

ifeq ($(BOARD_USES_CEC),true)
	LOCAL_SHARED_LIBRARIES += libcec
	LOCAL_CFLAGS += -DUSES_CEC
endif

ifeq ($(BOARD_USES_GSC_VIDEO),true)
	LOCAL_CFLAGS += -DGSC_VIDEO
endif

ifeq ($(BOARD_USES_FB_PHY_LINEAR),true)
	LOCAL_CFLAGS += -DUSE_FB_PHY_LINEAR
endif

ifeq ($(BOARD_USES_U4A),true)
	LOCAL_CFLAGS += -DUSES_U4A
endif

ifeq ($(BOARD_HDMI_INCAPABLE), true)
	LOCAL_CFLAGS += -DHDMI_INCAPABLE
endif

LOCAL_CFLAGS += -DLOG_TAG=\"hwcomposer\"

LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
	$(LOCAL_PATH)/../include \
	$(TOP)/hardware/samsung_slsi-cm/exynos/include \
	$(TOP)/hardware/samsung_slsi-cm/exynos/libexynosutils \
	$(TOP)/hardware/samsung_slsi-cm/exynos/libcec \
	$(TOP)/hardware/samsung_slsi-cm/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi-cm/$(TARGET_SOC)/libhwcmodule

LOCAL_ADDITIONAL_DEPENDENCIES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_SRC_FILES := ExynosHWC.cpp

LOCAL_MODULE := hwcomposer.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
