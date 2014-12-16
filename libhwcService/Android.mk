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

ifeq ($(BOARD_USES_HWC_SERVICES),true)

LOCAL_PATH:= $(call my-dir)
# HAL module implemenation, not prelinked and stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.product.board>.so

include $(CLEAR_VARS)
LOCAL_SHARED_LIBRARIES := liblog libcutils libhardware_legacy libutils libbinder libexynosv4l2 libhdmi
LOCAL_CFLAGS += -DLOG_TAG=\"HWCService\"
LOCAL_CFLAGS += -DHWC_SERVICES

LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
	$(LOCAL_PATH)/../include \
	$(TOP)/hardware/samsung_slsi-cm/exynos/include \
	$(TOP)/hardware/samsung_slsi-cm/exynos/libhwcutils \
	$(TOP)/hardware/samsung_slsi-cm/exynos/libdisplay \
	$(TOP)/hardware/samsung_slsi-cm/exynos/libexynosutils \
	$(TOP)/hardware/samsung_slsi-cm/$(TARGET_SOC)/include \
	$(TOP)/hardware/samsung_slsi-cm/$(TARGET_SOC)/libhwcmodule \
	$(TOP)/system/core/libsync/include

LOCAL_ADDITIONAL_DEPENDENCIES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

ifeq ($(BOARD_HDMI_INCAPABLE), true)
LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi-cm/exynos/libhdmi_dummy
else
ifeq ($(BOARD_USES_NEW_HDMI), true)
LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi-cm/exynos/libhdmi
else
LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi-cm/exynos/libhdmi_legacy
endif
endif

ifeq ($(BOARD_TV_PRIMARY), true)
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../libhwc_tvprimary
else
LOCAL_C_INCLUDES += \
	$(TOP)/hardware/samsung_slsi-cm/exynos/libhwc
endif

ifeq ($(BOARD_USES_VIRTUAL_DISPLAY), true)
	LOCAL_CFLAGS += -DUSES_VIRTUAL_DISPLAY
	LOCAL_SHARED_LIBRARIES += libvirtualdisplay
	LOCAL_C_INCLUDES += $(TOP)/hardware/samsung_slsi-cm/exynos/libvirtualdisplay
endif

LOCAL_SRC_FILES := ExynosHWCService.cpp IExynosHWC.cpp

ifeq ($(BOARD_USES_WFD),true)
	LOCAL_CFLAGS += -DUSES_WFD
endif

ifeq ($(BOARD_USE_S3D_SUPPORT),true)
	LOCAL_CFLAGS += -DS3D_SUPPORT
endif

ifeq ($(BOARD_USES_CEC),true)
	LOCAL_CFLAGS += -DUSES_CEC
endif

ifeq ($(BOARD_TV_PRIMARY),true)
	LOCAL_CFLAGS += -DTV_PRIMARY
endif

LOCAL_MODULE := libExynosHWCService
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

endif
