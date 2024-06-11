@echo off

rem define the %NDK_PATH% environment variable on your system.
rem !!!! Must use ndk version 27.0.11718014 for openxr !!!!

set NDKBUILDCMD="%NDK_PATH%\ndk-build"

rem Set the paths to the OpenXR loader libraries for different architectures
cd
set OPENXR_LOADER_ARM64_PATH=libs/arm64-v8a/libopenxr_loader.so
set OPENXR_LOADER_ARM7_PATH=libs/armeabi-v7a/libopenxr_loader.so

rem Call NDK build with additional library paths for both architectures
call %NDKBUILDCMD% NDK_OUT=../../build/jniObjs NDK_LIBS_OUT=./jniLibs NDK_LIBS="%OPENXR_LOADER_ARM64_PATH%;%OPENXR_LOADER_ARM7_PATH%" 2> log.txt

rem pause