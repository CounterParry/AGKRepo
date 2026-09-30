# Changelog

Notable changes to AGK are documented here.

## Unreleased

### Linux Tier 2

- Added the Linux `PlatformGetGraphicsConfig` hook and made it initialize output handles safely before delegating to the active renderer.
- Added the Linux GLX/Xlib graphics binding values to the OpenGL2 renderer, using handles from the live GLFW context. Windows continues to return its WGL handles.
- Made Vulkan graphics-config queries initialize all six outputs to null before reporting that export is unsupported.

### Android OpenXR

- Made `XR_FB_passthrough` optional. Devices whose runtime does not expose the extension can continue without passthrough.
- Updated the Android OpenXR template target SDK to API 23 (compile SDK 31; minimum SDK remains 16).

### Verification

- Linux engine and Tier 2 template builds completed on Ubuntu; the template also launched and rendered its sample FPS counter in a Linux desktop session.
- Android native builds completed for `armeabi-v7a` and `arm64-v8a`, and Gradle produced a debug APK. OpenXR initialization was confirmed on a Quest 2.

### Follow-up

- The Linux desktop run did not specifically force OpenGL2 or call `GetGraphicsConfig`, so live GLX graphics-config values have not been runtime-verified. Vulkan graphics-config export remains unsupported.
- Quest testing confirmed initialization; full rendering behavior and reported GLES warnings still need device validation.
- Windows OpenXR branch selection and build behavior still need verification.
