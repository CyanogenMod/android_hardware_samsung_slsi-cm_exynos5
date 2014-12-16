# Copyright (C) 2008 The Android Open Source Project
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

ifeq ($(filter-out exynos5,$(TARGET_BOARD_PLATFORM)),)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libexynosutils libexynosv4l2

# to talk to secure side
LOCAL_SHARED_LIBRARIES += libMcClient
LOCAL_STATIC_LIBRARIES := libsecurepath

ifeq ($(BOARD_USES_ONLY_GSC0_GSC1),true)
	LOCAL_CFLAGS += -DUSES_ONLY_GSC0_GSC1
endif

ifeq ($(BOARD_USES_SCALER), true)
LOCAL_CFLAGS += -DUSES_SCALER
LOCAL_SHARED_LIBRARIES += libexynosscaler
endif

ifeq ($(BOARD_USES_DT), true)
	LOCAL_CFLAGS += -DUSES_DT
endif

LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
	$(LOCAL_PATH)/../include \
	$(TOP)/hardware/samsung_slsi-cm/exynos/include \
	$(TOP)/hardware/samsung_slsi-cm/exynos/libexynosutils \
	$(TOP)/hardware/samsung_slsi-cm/exynos/libmpp

LOCAL_ADDITIONAL_DEPENDENCIES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_SRC_FILES := \
	libgscaler_obj.cpp \
	libgscaler.cpp

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := libexynosgscaler
include $(BUILD_SHARED_LIBRARY)

endif
