LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifneq ($(TARGET_ARCH_ABI),x86)
	LOCAL_ARM_MODE := arm
endif

SOXR_SRC_DIR := soxr-0.1.3/src

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(FFMPEG_OVERRIDE_ROOT)/.. \
	$(FFMPEG_ROOT) \
		
	
LOCAL_MODULE    := libsoxr
LOCAL_SRC_FILES := \
	soxr.c \
	data-io.c \
	dbesi0.c \
	filter.c \
	fft4g64.c \
	cr.c \
	cr32s.c \
	avfft32s.c \
	util32s.c \
	

LOCAL_SRC_FILES := $(addprefix $(SOXR_SRC_DIR)/, $(LOCAL_SRC_FILES))
	
LOCAL_CFLAGS = $(GLOBAL_CFLAGS) -DSOXR_LIB -std=gnu99 -Ofast # -ftree-vectorize

include $(BUILD_STATIC_LIBRARY)

