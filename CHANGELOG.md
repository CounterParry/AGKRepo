# CounterParry Fork Changelog

This log covers additions and fixes in the CounterParry fork after the GitHub `main` baseline at commit `e1f3d93` (2026-09-30). Linux Tier 2 graphics support and the initial Android passthrough support are already documented in that baseline and are not repeated here.

## Android OpenXR and Meta Quest

- Added `XR_EXT_hand_tracking` support with left and right hand trackers, joint pose polling, and hand gesture input mapping. Controller input remains the default; hand tracking can be enabled or disabled through the template API.
- Exposed `EnableHandTracking()`, `DisableHandTracking()`, `IsHandTrackingSupported()`, `GetHandTrackingActive()`, and `GetPassthroughActive()` in the template API.
- Declared Meta and Horizon OS hand tracking permissions and features in the Android manifest, including the high-frequency tracking hint. Declared the Meta passthrough feature for Quest.
- Hardened OpenXR lifecycle and error handling: partial initialization is cleaned up, session transitions are checked, frame and swapchain failures reach teardown safely, and acquired images are released on failure paths.
- Fixed the stereo eye-buffer aspect ratio calculation and changed grip getter declarations to match their floating-point implementations.
- Updated the Android OpenXR headers and loader to Khronos OpenXR 1.1.53 / Meta OpenXR SDK v85, and configured native builds for NDK 27 and both Android ABIs.

## Windows OpenXR

- Added `_WINDOWS_` to the standard Windows OpenXR project's Debug and Release preprocessor definitions and guarded the source-level definition in `agkopenxr.cpp`.

## README

- Added a fork introduction describing Tier 2 C++ and Meta Quest development, stating the AppGameKit Studio ownership requirement, and summarizing Linux, Android VR, and Windows VR support.

## Verification

- Android JNI compilation completed for `armeabi-v7a` and `arm64-v8a`; the debug APK was built, installed, and launched on Quest 2. Startup logs showed hand tracking availability and passthrough startup. The headset check did not fully validate hand gestures or all query commands.
- The Windows OpenXR template changes were inspected but not built.
