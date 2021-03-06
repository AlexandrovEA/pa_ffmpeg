# This doesn't properly work due to FFMPEG config variables conflicts. Use separate "ndk-build APP_ABI=armeabi" and "ndk-build APP_ABI=armeabi-v7a".
#APP_ABI := armeabi-v7a armeabi

APP_OPTIM := release

APP_DEBUGGABLE := false
APP_PLATFORM := android-21

APP_MODULES := libsoxr #libswresample # NOTE: this forces static lib build

# NOTE: gcc seems to be better for arm64 + disabled arm64 asm opts for resampler
# NOTE: comment after NDK_TOOLCHAIN_VERSION, causes space in var and stupid permission denied error
ifeq ($(APP_ABI),arm64-v8a)
	NDK_TOOLCHAIN_VERSION := clang
else
#	ifneq (,$(findstring -r20,$(ANDROID_NDK)))
#		# ndk-r20
#		NDK_TOOLCHAIN_VERSION := clang
#	else
#		NDK_TOOLCHAIN_VERSION := 4.9
#	endif
	NDK_TOOLCHAIN_VERSION := clang
endif

