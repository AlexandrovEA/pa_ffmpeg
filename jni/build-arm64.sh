#!/bin/bash

echo Using ANDROID_NDK=$ANDROID_NDK

$ANDROID_NDK/ndk-build -j12 APP_ABI=arm64-v8a GLOBAL_ARCH_MODE=arm64 $*

