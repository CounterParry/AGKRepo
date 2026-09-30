# Changelog

Notable changes to AGK are documented here.

## Unreleased

### Linux Tier 2

- Added the Linux `PlatformGetGraphicsConfig` hook and safe output initialization before delegating to the active renderer.
- Added Linux GLX/Xlib graphics binding values from the live GLFW context to the OpenGL2 renderer.
- Made Vulkan graphics-config queries initialize all outputs to null before reporting that export is unsupported.

### Android OpenXR

- Made `XR_FB_passthrough` optional so startup can continue when the runtime does not support it.
- Set the Android OpenXR template target SDK to API 23; compile SDK remains 31 and minimum SDK remains 16.

### Verification

- Linux engine and Tier 2 template builds completed on Ubuntu, and the template rendered its sample FPS counter in a Linux desktop session.
- Android native builds completed for `armeabi-v7a` and `arm64-v8a`, Gradle produced a debug APK, and OpenXR initialization was confirmed on a Quest 2.

### Follow-up

- Runtime validation of OpenGL2 `GetGraphicsConfig` values remains outstanding; Vulkan graphics-config export is unsupported.
- Full Quest rendering behavior and reported GLES warnings still need device validation.
- Windows OpenXR build and branch selection still need verification.
