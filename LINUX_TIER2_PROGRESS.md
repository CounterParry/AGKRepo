# Linux Tier 2 Support: Implementation Log

## Scope and status

Work is limited to the Linux desktop Tier 2 engine and template. No Classic, Android, iOS, Pi, Minecraft, Pi-hole, router, DNS, or unrelated code was changed. The source changes are in `AGK/platform/linux/Source/LinuxCore.cpp`, `AGK/renderer/OpenGL2/OpenGL2.cpp`, and `AGK/renderer/Vulkan/AGKVulkan.cpp`.

## Graphics-config contract established from callers

`agk::GetGraphicsConfig` in `AGK/common/Source/Wrapper.cpp` forwards its six output slots to a platform hook. The only active callers found in source are the Windows and Android OpenXR templates:

- Windows consumes slots 1–3 as `HDC`, `HGLRC`, and `HWND`, which form `XrGraphicsBindingOpenGLWin32KHR`.
- Android consumes slots 1–4 as `EGLDisplay`, `EGLSurface`, `EGLContext`, and `EGLConfig`, which form `XrGraphicsBindingOpenGLESAndroidKHR`. Slots 5–6 are unused/null in the GLES renderer.
- Vulkan's renderer implementation says the API is unsupported. It previously left all six outputs untouched.
- No Linux or macOS `PlatformGetGraphicsConfig` implementation or Linux caller exists in this tree. Therefore the function has a renderer-specific native-handle contract, not one universal meaning for all six slots.

The repository's OpenXR platform declarations define `XrGraphicsBindingOpenGLXlibKHR` with these Linux GLX fields, in order: `Display*`, visual ID, `GLXFBConfig`, `GLXDrawable`, `GLXContext`. That provides the source-backed Linux mapping used here; slot 6 remains reserved and null. The FBConfig is recovered from the live GLFW GLX context's `GLX_FBCONFIG_ID`, then matched against the display's FBConfigs. The drawable comes from GLFW's `glfwGetGLXWindow`, not the Xlib `Window` handle. If the window, context, queried config, or visual cannot be obtained, outputs stay null.

Windows and Android platform hooks both delegate to `g_pRenderer->GetGraphicsConfig(...)` when a renderer exists. The new Linux hook follows that pattern, first setting all non-null output pointers to null. The macOS platform source has no matching hook. Its OpenGL2 renderer also used the Windows-only WGL handles in the shared method; this work keeps changes focused on Linux and does not attempt a macOS repair.

## Source changes

1. **`AGK/platform/linux/Source/LinuxCore.cpp`** — added `PlatformGetGraphicsConfig`, which initializes outputs defensively and delegates to the active renderer. This removes the Linux link-time undefined symbol without a shim.
2. **`AGK/renderer/OpenGL2/OpenGL2.cpp`** — kept WGL values inside the Windows branch and added the Linux GLX/Xlib tuple described above. Added GLFW native GLX declarations and safe null outputs. This removes Linux's compile failure on undefined WGL globals and returns handles belonging to the context AGK actually created.
3. **`AGK/renderer/Vulkan/AGKVulkan.cpp`** — initializes all six outputs to null before reporting that Vulkan graphics-config export is unsupported. Callers no longer observe uninitialized values.

No Makefile edits were needed to fix source compilation or symbol resolution. The engine compiles its vendored curl, PNG, JPEG, compression, physics, and other bundled code.

## Build and runtime results

- A **clean** full build of `libAGKLinux.a` completed with no WGL macro workaround. The source list includes the Linux core, OpenGL2, and Vulkan renderer implementations. Existing compiler warnings remain (including the legacy integer-to-pointer warning in OpenGL2 drawing code and `-std=c++11` passed to C compilation); they did not stop the build.
- A clean build of the Linux Tier 2 template completed and linked `AGK/apps/template_linux/build/LinuxApp64`. There were no remaining source undefined symbols or link errors. The missing `PlatformGetGraphicsConfig` symbol is defined in the archive; no temporary shim was linked.
- After the Ubuntu system dependencies were installed, a second **clean** engine build and clean template build both completed using system OpenAL, udev, GL, and X11 dependencies. There were no remaining source compile errors, undefined symbols, or link errors.
- Plain `ldd AGK/apps/template_linux/build/LinuxApp64` resolves all required shared libraries from system paths, including OpenAL, udev, X11/GLX, XRandR, Xinerama, Xcursor, and OpenAL's `libsndio.so.7` dependency. No libraries report `not found`.
- The template Makefile statically links GLFW. `libglfw3-dev` is not installed, so these clean builds used the pre-existing, locally built GLFW 3.2.1 prefix at `/tmp/agk-install` for GLFW headers and `libglfw3.a`. No `/tmp/agk-sysroot` libraries or `LD_LIBRARY_PATH` were used for the successful system-dependency build.
- An initial launch from the SSH session failed before GLFW could connect because that session had no usable display. This was an environment/access failure, not a renderer failure. A later real desktop test on node0001 succeeded; see “Successful Linux workstation graphical runtime test” below.

## Dependencies and installation status

The system now has `libopenal-dev`, `libopenal1`, `libudev-dev`, and `xorg-dev` installed, as well as `libgl-dev`. Ubuntu's `libglfw3-dev` package, which supplies GLFW headers and linker libraries expected by the app Makefile, is absent. Because this task prohibited changes outside the repository, that system package was not installed. The existing GLFW 3.2.1 build at `/tmp/agk-install` was used as the external GLFW dependency.

The previous system-install attempt before those packages were installed was:

```sh
sudo -n apt-get install -y libopenal-dev libudev-dev xorg-dev
```

It could not proceed because sudo required an interactive password. The earlier local sysroot was used only during the previous investigation/build; it was not used for the successful rebuild documented here.

## Reproduction commands used

Commands below assume the locally prepared GLFW 3.2.1 prefix remains at `/tmp/agk-install`; all other listed dependencies resolve from system installation. Run the engine build from `AGK/`:

```sh
make clean
CPLUS_INCLUDE_PATH=/tmp/agk-install/include make -j2 CFLAGS='-O2 -std=c++11'
```

Then clean and build the Tier 2 template from `AGK/apps/template_linux/`:

```sh
make clean
CPLUS_INCLUDE_PATH=/tmp/agk-install/include \
LIBRARY_PATH=/tmp/agk-install/lib make -j2
```

Inspect dependencies with the host's installed runtime libraries:

```sh
ldd build/LinuxApp64
```

Attempted non-graphical launch (before system OpenAL was installed):

```sh
LD_LIBRARY_PATH=/tmp/agk-sysroot/usr/lib/x86_64-linux-gnu timeout 10s ./build/LinuxApp64
```

## Remaining work and caveats

- The build used the existing GLFW 3.2.1 prefix because `libglfw3-dev` is absent. Ubuntu package `libglfw3-dev` supplies the headers and linker library required by the template's `-lglfw3` link. A full system-only rebuild would require that package, but installing it would modify `/usr` outside the requested repository boundary.
- The basic GLFW window and rendered FPS counter have since been verified in a real Linux desktop session on node0001. OpenGL2-specific initialization and live `GetGraphicsConfig` GLX values remain unverified; the template uses `PREFER_BEST` and does not call `GetGraphicsConfig`.
- Vulkan config export remains intentionally unsupported, but its six outputs are now deterministic null values.

The workspace has no Git metadata, so tracked/untracked status could not be checked using Git. Build products were generated under the existing Linux library and template build directories; all source edits are limited to the three files listed above, plus this log.

## Graphical runtime investigation (2026-09-27)

### Current session

The build artifacts were present at the start of this pass. The environment is still an SSH pseudo-terminal (`SSH_TTY=/dev/pts/0`, `XDG_SESSION_TYPE=tty`) with empty `DISPLAY` and `WAYLAND_DISPLAY`. There is an Xorg process on display `:0`, but it is serving the SDDM login greeter, not a logged-in `master` desktop session. Its authority file `/run/sddm/xauth_tqgLDB` is mode `0600` and owned by `sddm`.

### Source path inspected

- The Tier 2 template's `main` in `AGK/apps/template_linux/Core.cpp` calls `glfwInit`, gets the primary monitor/video mode, and then calls `agk::InitGraphics(..., AGK_RENDERER_MODE_PREFER_BEST, ...)`. After initialization it runs `App.Begin`, loops through `App.Loop` and `glfwPollEvents`, then cleans up and destroys the GLFW window.
- Linux `PlatformInitGraphics` first tries Vulkan for `PREFER_BEST` when Vulkan is enabled, and falls back to OpenGL2 if Vulkan initialization fails. Its OpenGL2 branch requests a GLFW OpenGL 2.0 context, creates the window, makes the context current, passes the X11 display/window to `OpenGL2Renderer::SetupWindow`, and calls renderer `Setup` before returning the window to the template.
- The template has no caller of `agk::GetGraphicsConfig`. Its public wrapper forwards to `PlatformGetGraphicsConfig`; the newly added platform and OpenGL2 functions are in the built executable/archive. Thus the normal template launch alone cannot exercise or validate the returned GLX values. A runtime diagnostic caller must execute after `InitGraphics` has completed, or a separate Tier 2 smoke-test app can request `AGK_RENDERER_MODE_ONLY_LOWEST` and call `GetGraphicsConfig` after initialization. This avoids changing the template's normal Vulkan-preferred default just to make the test deterministic.

### Commands and results in this pass

- `DISPLAY=:0 XAUTHORITY=/run/sddm/xauth_tqgLDB xrandr --current` — failed with X authorization denial; no display modes could be queried.
- `DISPLAY=:0 XAUTHORITY=/run/sddm/xauth_tqgLDB glxinfo -B` — failed with X authorization denial; OpenGL/GLX renderer information was not available.
- `DISPLAY=:0 XAUTHORITY=/run/sddm/xauth_tqgLDB timeout 10s AGK/apps/template_linux/build/LinuxApp64` — failed with X authorization denial followed by `Failed to initialize GLFW` and exit status 1. This did not reach window creation or renderer initialization and is not evidence of an AGK graphics failure.
- `ldd AGK/apps/template_linux/build/LinuxApp64` — all shared libraries resolved from system paths after the dependencies were installed, including `libopenal.so.1`, `libudev.so.1`, GLX/X11 libraries, and OpenAL's `libsndio.so.7` dependency.
- `nm -C AGK/apps/template_linux/build/LinuxApp64` — confirmed definitions for `AGK::agk::PlatformGetGraphicsConfig` and `AGK::OpenGL2Renderer::GetGraphicsConfig` in the built program.
- `gdb`, `Xvfb`, and `xvfb-run` are unavailable. More importantly, an Xvfb test would not satisfy the request for a normal actual desktop session, and the current SDDM-owned display cannot be used as the logged-in user's display.

No source changes or rebuilds were warranted in this pass: the only observed failure is session authentication before GLFW can connect, and source inspection does not show a Tier 2 fix for that environment condition. No temporary WGL define or graphics-config shim was used.

### To complete an actual desktop test

Log in to the Linux desktop on the machine that owns this checkout (or establish an SSH X11-forwarded session from that logged-in desktop). Open a terminal in `AGK/apps/template_linux`, verify `DISPLAY` is set and the session has X11/XWayland access, then launch `./build/LinuxApp64`. This GLFW 3.2.1 path uses X11/GLX native handles, so a Wayland desktop must provide its XWayland `DISPLAY` for this build. To force OpenGL2 deterministically, use a Tier 2 smoke-test target invoking `agk::InitGraphics` with `AGK_RENDERER_MODE_ONLY_LOWEST`; after it returns, call `agk::GetGraphicsConfig` and validate non-null display, visual ID, FBConfig, drawable, and context (and query the live context's FBConfig ID to confirm the tuple matches). The current normal template uses `PREFER_BEST`, so it may exercise Vulkan instead.

At the time of this investigation, actual desktop window creation and rendering were untested because the SSH session could not authenticate to the SDDM display. That gap was subsequently closed for the existing executable on node0001, as recorded below. OpenGL2-specific initialization, live GLX/Xlib config values, input handling, and orderly close/cleanup were still not established by that initial launch.

## Running the existing binary from the Linux workstation (2026-09-27)

The source stays on `node1101`; the simplest reliable first graphical test is to copy only the already built executable to the workstation and run it from a terminal in that workstation's logged-in desktop session. This keeps rendering on the workstation's local X11/XWayland + GLX stack and GPU. SSH X11 forwarding is a fallback, but GLX support over forwarding is commonly limited/indirect and may fail with GLFW 3.2.1; it is less suitable for checking an OpenGL renderer.

The executable is x86-64 and requires GLIBC up to `GLIBC_2.38`; use a workstation distribution with GLIBC 2.38 or newer (Ubuntu 24.04's GLIBC 2.39 qualifies). `readelf -d` shows direct runtime dependencies on GL, X11, XF86VidMode, XRandR, Xinerama, Xcursor, OpenAL, udev, libc, and libm. GLFW is statically linked, and this minimal template directory has no adjacent media/assets required to display its FPS counter. Check runtime availability on the workstation with `ldd`; also run `glxinfo -B` in the same desktop terminal to confirm its GLX display is available. On a Wayland session, the program needs XWayland (`DISPLAY`/GLX) because this build uses GLFW's X11/GLX native interface.

From a workstation desktop terminal (assuming the SSH host alias is `node1101` and account is `master`):

```sh
mkdir -p "$HOME/agk-tier2-test"
scp master@node1101:/home/master/projects/AGKRepo-main/AGK/apps/template_linux/build/LinuxApp64 "$HOME/agk-tier2-test/"
cd "$HOME/agk-tier2-test"
chmod u+x LinuxApp64
ldd ./LinuxApp64 | grep 'not found' || true
glxinfo -B
./LinuxApp64
```

The template window should display the sample's FPS counter; close it through the window manager to exercise normal cleanup. If `scp` needs the user's configured SSH alias, substitute that alias/address. If local `ldd` reports missing runtime libraries, install the corresponding runtime packages on the workstation before launch.

If running remotely is preferred, open a new workstation terminal and reconnect with `ssh -X master@node1101` (or the configured host alias), verify the new remote shell has a non-empty `DISPLAY`, and then run `glxinfo -B` followed by the remote executable. Do not use `-Y` by default because trusted X11 forwarding grants a remote application broader access to the workstation's X session. If forwarded `glxinfo` cannot initialize GLX, the remote binary cannot provide the requested GLX test over that connection; use the local-copy method.

This first run tests the basic GLFW/window/render path only. The existing template calls `InitGraphics` with `AGK_RENDERER_MODE_PREFER_BEST`, so Vulkan may be chosen instead of OpenGL2, and the template never calls `GetGraphicsConfig`. After the workstation display path is confirmed working, a separate Tier 2 diagnostic invocation will be needed to force `AGK_RENDERER_MODE_ONLY_LOWEST` and inspect the five GLX/Xlib values after initialization. No source changes have been made for this test plan.

## Successful Linux workstation graphical runtime test

The user reports that the existing Tier 2 executable built on node1101 was copied to the Linux workstation node0001 and successfully launched in its real graphical desktop session. The test confirmed:

- The Linux AGK Tier 2 executable runs on the Linux desktop.
- GLFW creates the game window successfully.
- The AMD RX 580 provides hardware-accelerated OpenGL/GLX on that workstation.
- The AGK window renders the sample FPS counter.
- The workstation initially lacked `libopenal.so.1`; installing Ubuntu runtime package `libopenal1` resolved the dependency and allowed the app to launch.
- This was a real desktop runtime test, not only a build or compile check.

This supersedes the earlier “untested” statements about basic window creation and graphical runtime. The earlier failures remain accurate only for the separate SSH session on node1101, which lacked authorized display access. The successful report establishes window creation and rendered output on node0001; unless a separate test forced the OpenGL2-only renderer, it does not by itself prove which renderer `PREFER_BEST` selected. It also does not exercise `GetGraphicsConfig` (the template has no caller), so the returned GLX/Xlib values remain untested. No AGK source changes were made for this runtime test.

## Android OpenXR build investigation

Inspection-only status: no Android OpenXR, Windows OpenXR, AGK engine, build-script, Gradle, or system configuration files were changed. No packages were installed and no build was attempted because the Android SDK/NDK are absent on node1101. The existing Linux backup at `~/projects/AGKRepo-linuxworking` was left untouched.

### A. Exact Android OpenXR template location

`/home/master/projects/AGKRepo-main/AGK/apps/template_android_openxr`

This is an Android application/native Tier 2 project that cross-compiles Android ABIs from a Linux host; it is not Linux desktop OpenXR support. The project contains a Gradle root (`settings.gradle`, root `build.gradle`, wrapper and `gradle/`), Android app module `AGK2Template/`, manifest/resources, Java activity sources, `src/main/jni/` C/C++ sources and NDK makefiles, vendored OpenXR headers, ABI-specific native libraries, and `src/main/libs/` notices/readmes. Shared Java support comes from `AGK/apps/android_common/` and those referenced directories are present. The module's native entry points are `main.c`, `Core.cpp`, `agkopenxr.cpp`, and `template.cpp`.

### B. Exact build process and commands

The checked-in Windows helper scripts show that the native NDK build is a separate first step. The project does not configure Gradle `externalNativeBuild`, CMake, or another Gradle task to invoke it. On Linux, the equivalent commands, after installing/configuring the toolchain and correcting the machine-local SDK path, are:

```sh
cd /home/master/projects/AGKRepo-main/AGK/apps/template_android_openxr/AGK2Template/src/main
"$ANDROID_SDK_ROOT/ndk/27.0.11718014/ndk-build" \
  NDK_OUT=../../build/jniObjs \
  NDK_LIBS_OUT=./jniLibs

cd /home/master/projects/AGKRepo-main/AGK/apps/template_android_openxr
bash ./gradlew :AGK2Template:assembleDebug
```

The wrapper requests Gradle 7.5. `bash ./gradlew` is used because the checked-in Unix wrapper currently lacks its executable bit. The batch scripts redirect NDK stderr to `src/main/log.txt`; those are Windows-only wrappers, not required build logic. Native output is directed to `AGK2Template/build/jniObjs` and `src/main/jniLibs`; Gradle then assembles/packages the app. The expected debug artifact is `AGK2Template/build/outputs/apk/debug/AGK2Template-debug.apk`. Existing APK/native outputs in the tree are prior artifacts and do not demonstrate a reproducible build on this host.

### C. Required tools and dependencies

- **Java/JDK:** AGP 7.2.1 requires a compatible JDK; JDK 11 is the minimum for AGP 7.x, and the installed JDK 17 is suitable for Gradle 7.5.
- **Gradle:** wrapper-pinned Gradle 7.5 (`gradle-7.5-bin.zip`). No system Gradle is needed.
- **Android Gradle Plugin:** 7.2.1 from Google Maven.
- **Android SDK platform:** API 31 (`compileSdkVersion 31`).
- **Android build tools:** 31.0.0 (`buildToolsVersion "31.0.0"`).
- **Android NDK:** 27.0.11718014, explicitly named in `jniCompile.bat` and `jniCompile_Desktop.bat` comments/configuration. `Application.mk` sets `APP_PLATFORM := android-24`, targets `armeabi-v7a` and `arm64-v8a`, release optimization, and `c++_static`.
- **Android command-line tools / sdkmanager:** needed to provision the pinned SDK and NDK packages. `adb`/platform-tools are useful for device deployment but not required to assemble the APK.
- **CMake:** not used by this project. CMake references in the vendored Meta README describe other Meta SDK samples/Quest Link flows and do not apply to this project's `Android.mk` build.
- **OpenXR headers:** bundled under `AGK2Template/src/main/jni/openxr/` (including `openxr.h`, `openxr_platform.h`, and reflection/platform headers).
- **OpenXR loader:** bundled for both ABIs; details in section G. No separate SDK-provided loader is required.
- **AGK Android archives:** bundled in `AGK/platform/android/jni/{arm64-v8a,armeabi-v7a}/`: `libAGKAndroid.a`, `libAGKBullet.a`, `libAGKAssimp.a`, and `libAGKCurl.a`.
- **Other native components:** Android NDK `android/native_app_glue`, Android system libraries `log`, `android`, EGL, GLESv2/GLESv3, zlib, OpenSLES, math, and the NDK C/C++ runtime. These are declared in `Android.mk`/`Application.mk` and supplied by the NDK/platform.
- **Java/app dependencies:** AppCompat 1.4.2 is fetched from Google/Maven repositories; `ouya-sdk.jar` is local. Java helper sources are included from `AGK/apps/android_common/`.
- **GLFW:** not used by the Android project.
- **Network:** first wrapper use and Gradle dependency resolution require access to Gradle distribution services and Google's/Maven artifact repositories. OpenXR and AGK libraries are local repository inputs.
- **Runtime-only requirement:** running on a headset requires a compatible Android OpenXR runtime/device. That is not needed to compile the APK and was not tested.

### D. Required tools/dependencies already installed on node1101

At inspection time, Ubuntu package metadata and executable/path checks showed:

- OpenJDK 17 JDK/JRE **17.0.20.1** and `javac` are installed. OpenJDK 8 and 11 runtime/JDK packages are also installed, but the selected `java`/`javac` are version 17.
- No `gradle` executable is installed. The project wrapper and wrapper JAR are present; no Gradle 7.5 wrapper distribution was found in the checked Gradle wrapper cache. The wrapper can fetch it when run, but no build was initiated.
- No Android SDK installation was found in the standard home, `/opt`, `/usr/lib`, or `/usr/share` locations checked; `ANDROID_HOME`, `ANDROID_SDK_ROOT`, `ANDROID_NDK_HOME`, and `JAVA_HOME` are unset.
- `sdkmanager`, `avdmanager`, `adb`, and `ndk-build` are not found on `PATH`; no Android platform `android.jar` or NDK `source.properties` was found in the checked locations.
- CMake and Ninja are not installed/found, but neither is required by this project.
- Repository-provided dependencies are present: OpenXR headers; both ABI OpenXR loader copies; the four AGK Android static archives for each ABI; OUYA JAR; Android common Java source trees.
- AndroidX AppCompat 1.4.2 and AGP 7.2.1 are Gradle-managed dependencies, not OS packages. Their availability was not validated by a build.

### E. Missing tools/dependencies

The Android command-line tools and SDK installation, including platform 31, build-tools 31.0.0, and NDK 27.0.11718014, are missing. Consequently `ndk-build` is unavailable. A cached Gradle 7.5 distribution/system Gradle was not found, although the wrapper can download Gradle. Android device deployment tooling (`adb`) is missing but is optional for compilation. CMake is also missing but is not a project requirement. JDK is not missing: JDK 17 is installed.

The project's `local.properties` exists but points at `C:\Users\danle\AppData\Local\Android\Sdk`; it is not a valid node1101 SDK location. It is explicitly marked machine-specific and not for version control. Gradle therefore needs a correct local SDK setting once the SDK is installed. No changes to this file were made.

### F. Windows-specific assumptions

- `local.properties` contains the developer's hard-coded Windows SDK path; this prevents Gradle from locating an SDK on Linux until the local path is corrected.
- `jniCompile.bat` and `jniCompile_Desktop.bat` are Windows batch scripts; the latter hard-codes `C:\Users\danle\AppData\Local\Android\Sdk\ndk\27.0.11718014`. They cannot execute on Linux, but their build action has a direct `ndk-build` equivalent shown in B. The general script also expects `NDK_PATH` to be set.
- `gradlew.bat` is Windows-only, but Unix `gradlew` is included. Its missing executable bit is easily handled by `bash ./gradlew` and is not a source incompatibility.
- `agkopenxr.cpp` defines `_ANDROID_`, enables `XR_USE_GRAPHICS_API_OPENGL_ES` and `XR_USE_PLATFORM_ANDROID`, and conditionally includes the Windows WGL code only under `_WINDOWS_`. The Windows graphics branch should therefore not be compiled for this Android target.
- No other intrinsic Windows host dependency was found in the Android project's source/build configuration. The host is only building Android ARM binaries.

### G. OpenXR loader origin and linkage

The project **contains its own OpenXR loader binaries** and does not fetch a loader during Gradle/NDK build or expect the Android SDK/NDK to provide one. The OpenXR header set is also vendored. The project README identifies the libraries as Meta OpenXR Libraries version 64.0 and says they may be replaced with Pico libraries for a Pico headset.

ABI-specific `libopenxr_loader.so` files exist in both `src/main/libs/{arm64-v8a,armeabi-v7a}/` and `src/main/jniLibs/{arm64-v8a,armeabi-v7a}/`. `readelf` identifies them as AArch64 and ARM ELF shared objects, respectively; each pair has matching Build IDs. `Android.mk` declares the `src/main/libs` copies as `PREBUILT_SHARED_LIBRARY` modules. However, the final `android_player` target appends those module names to `LOCAL_STATIC_LIBRARIES`. That conflicts with the NDK makefile distinction between prebuilt shared and static modules and is a likely genuine native build blocker/mislink; it has not been confirmed with `ndk-build` because the NDK is absent. The smallest likely source correction is to link the selected loader module through `LOCAL_SHARED_LIBRARIES` instead. Confirm against NDK 27.0.11718014 before changing it.

The existing packaged `jniLibs/.../libandroid_player.so` has `DT_NEEDED: libopenxr_loader.so`, as expected for dynamic loader linkage. Gradle's standard `jniLibs` packaging also includes the OpenXR loader and `android_player` shared libraries. The app's device runtime must provide a compatible OpenXR runtime; that is distinct from the bundled client loader.

### H. AGK Android engine linkage/integration

`Android.mk` declares the ABI-specific AGK archives as prebuilt static modules and links `AGKAndroid`, Bullet, Assimp, and Curl into the `android_player` shared library. The makefile also associates Bullet/Assimp with the AGKAndroid prebuilt module and lists the relevant archives on the final target. Include paths point to AGK `common/include`, renderer, Bullet, and local OpenXR headers. Native output module name `android_player` matches the Android manifest's `android.app.lib_name` metadata.

The Java `AGKActivity` comes from the shared `AGK/apps/android_common` source directories, and NDK `android_native_app_glue` supplies the Android native app lifecycle. `agkopenxr.cpp` initializes the Android OpenXR loader with the active `android_app`'s Java VM and Activity context via `xrInitializeLoaderKHR`. For graphics binding it calls `agk::GetGraphicsConfig` to obtain EGL display, surface, context, and config and fills `XrGraphicsBindingOpenGLESAndroidKHR`. This relies on the AGK Android renderer's live EGL state rather than creating a separate EGL context.

### I. Linux buildability assessment

The source/project is substantially Linux-host-buildable: Gradle/NDK are cross-platform, the Android branch is explicitly selected, native code and the required ARM archives/loader/header files are present, and Windows-only build scripts have standard Linux command equivalents. There is no inherent requirement to build on Windows.

It is **not currently buildable on node1101 as configured** because the Android SDK/NDK are absent and `local.properties` points to a Windows SDK directory. Those are environment/setup blockers. Independently, the shared OpenXR loader module being placed in `LOCAL_STATIC_LIBRARIES` is a likely Android.mk blocker that must be validated with the pinned NDK before claiming a clean source build. No build has been attempted, so there is no Linux build-success result yet.

### J. Specific blockers and smallest likely fixes

1. **Missing toolchain:** install/configure Android command-line tools, SDK platform 31, build-tools 31.0.0, and NDK 27.0.11718014; use existing JDK 17. This is environment provisioning, not an AGK source edit.
2. **Windows SDK path in local.properties:** point machine-local Gradle configuration at the installed Linux SDK (or use another supported local SDK configuration). Do not commit a host-specific path.
3. **OpenXR shared module classified as static:** build with pinned NDK to confirm. If rejected or mishandled, change only the target's loader linkage from `LOCAL_STATIC_LIBRARIES` to `LOCAL_SHARED_LIBRARIES`, retaining the matching ABI-specific `PREBUILT_SHARED_LIBRARY` module. Then rebuild and confirm APK packaging and `DT_NEEDED`/loader placement.
4. **Unix wrapper executable bit:** invoke using `bash ./gradlew`; changing the executable bit is optional and not necessary to compile.

### K. Proposed next step (not performed)

Provision the Android SDK/NDK in a user-owned Linux SDK directory, set machine-local SDK/JDK environment/configuration without touching the preserved Linux backup, then run the two commands in B. Treat the first native build output as authoritative for the `LOCAL_SHARED_LIBRARIES` issue. Only after recording the actual compiler/linker error should the minimum Android.mk correction be proposed for approval/implementation. No installation, source modification, build, or Android OpenXR runtime test has been performed in this investigation.

## Android SDK/NDK environment installed (2026-09-27)

Installed the minimum Android build environment requested for investigating the existing Android OpenXR template. All SDK tools/packages are under `/home/master/Android/Sdk`; nothing was installed under `/usr` and no system Java packages were installed or changed.

| Component | Installed version | Path |
|---|---:|---|
| Android Command-line Tools | 22.0 (`sdkmanager`) | `/home/master/Android/Sdk/cmdline-tools/latest` |
| Android SDK Platform | API 31 | `/home/master/Android/Sdk/platforms/android-31` |
| Android Build Tools | 31.0.0 | `/home/master/Android/Sdk/build-tools/31.0.0` |
| Android NDK | 27.0.11718014 (`27.0.11718014-beta1`, r27-beta1) | `/home/master/Android/Sdk/ndk/27.0.11718014` |
| Java | OpenJDK 17.0.20.1, pre-existing and unchanged | `/usr/lib/jvm/java-17-openjdk-amd64` |

Configured the existing project's machine-local SDK setting in `AGK/apps/template_android_openxr/local.properties` as:

```properties
sdk.dir=/home/master/Android/Sdk
```

That file is explicitly marked as machine-specific and not to be committed. The SDK command-line tools archive was downloaded from Google's official Android developer download page, and its published SHA-256 checksum was verified before extraction. `sdkmanager --version` reported 22.0; `sdkmanager --list_installed` confirmed Platform 31, Build Tools 31.0.0, and NDK 27.0.11718014. The platform `android.jar` is present, and the NDK `ndk-build --help` launcher executed successfully. `ndk-build --version` itself reports its underlying GNU Make version (4.3), so the NDK revision was verified from `source.properties` and sdkmanager package metadata instead.

No Android OpenXR project build was run. No AGK source, Android.mk, Gradle build file, Linux Tier 2 source, or backup file was modified. The next step remains to run the separately documented native `ndk-build` and Gradle commands only when building is requested; the Android.mk OpenXR shared-versus-static linkage concern remains unverified by an actual build.

### Android OpenXR first native build

**Result: failed during GNU make dependency-file parsing, before native compilation/linking.** As instructed, no fix was attempted, no source/build file was edited, and the Gradle APK build was not run.

Command, run from `/home/master/projects/AGKRepo-main/AGK/apps/template_android_openxr/AGK2Template/src/main`:

```sh
/home/master/Android/Sdk/ndk/27.0.11718014/ndk-build NDK_OUT=../../build/jniObjs NDK_LIBS_OUT=./jniLibs
```

Complete captured NDK output:

```text
Android NDK: WARNING: APP_PLATFORM android-24 is higher than android:minSdkVersion 1 in ./AndroidManifest.xml. NDK binaries will *not* be compatible with devices older than android-24. See https://android.googlesource.com/platform/ndk/+/master/docs/user/common_problems.md for more information.
../../build/jniObjs/local/arm64-v8a/objs/android_native_app_glue/android_native_app_glue.o.d:1: *** target pattern contains no '%'.  Stop.
```

The make error names the generated dependency file:

`AGK2Template/build/jniObjs/local/arm64-v8a/objs/android_native_app_glue/android_native_app_glue.o.d`

That pre-existing `.d` file contains Windows absolute paths. Its contents (all three rules) were:

```make
../../build/jniObjs/local/arm64-v8a/objs/android_native_app_glue/android_native_app_glue.o: \
  C:/Users/danle/AppData/Local/Android/Sdk/ndk/27.0.11718014/build/../sources/android/native_app_glue/android_native_app_glue.c \
  C:/Users/danle/AppData/Local/Android/Sdk/ndk/27.0.11718014/build/../sources/android/native_app_glue/android_native_app_glue.h
C:/Users/danle/AppData/Local/Android/Sdk/ndk/27.0.11718014/build/../sources/android/native_app_glue/android_native_app_glue.h:
```

GNU make reads the colon in the stale `C:/Users/...` prerequisites as make rule syntax and aborts with `target pattern contains no '%'`. The immediate failure is therefore caused by the old Windows-generated dependency file under `build/jniObjs`, not by a compile error in Android source. The NDK emitted a separate API-level warning because the source `AndroidManifest.xml` has no `uses-sdk` minimum; Gradle's module configuration separately declares minSdk 16 and `Application.mk` targets android-24. That warning was not the fatal error.

**OpenXR loader linkage question:** this run does not establish whether `libopenxr_loader.so`, declared as `PREBUILT_SHARED_LIBRARY` but listed in `LOCAL_STATIC_LIBRARIES`, is a build blocker. Make stopped while parsing the `android_native_app_glue` dependency file, before compiling/linking `android_player` or processing its loader libraries. The suspected mismatch remains unconfirmed.

Full raw output was captured during the run at `/tmp/agk_android_openxr_ndk_build.log`. No source fix, generated-file cleanup, or retry was performed. The next diagnostic action, if authorized, is to rebuild after removing or bypassing the stale Windows-generated NDK object/dependency output; then inspect the next authentic Linux NDK result. The Gradle APK build remains unattempted.

#### Retry after clearing stale generated NDK output

At the user's direction, cleared only the contents of `AGK2Template/build/jniObjs`, the allowed generated NDK object/output directory. Did not clear `src/main/jniLibs` because it also held bundled libraries; the successful NDK run overwrote/installed the generated ABI outputs there. No source, makefile, Gradle file, loader input, AGK input library, Linux Tier 2 file, or backup file was changed.

Re-ran the same command from `AGK2Template/src/main`:

```sh
/home/master/Android/Sdk/ndk/27.0.11718014/ndk-build NDK_OUT=../../build/jniObjs NDK_LIBS_OUT=./jniLibs
```

**Native build succeeded for both ABIs.** NDK compiled `main.c`, `Core.cpp`, `agkopenxr.cpp`, and `template.cpp`; processed the ABI-specific OpenXR loader as a prebuilt shared library; built `android_native_app_glue`; linked `libandroid_player.so`; and installed both shared objects to `src/main/jniLibs`.

Generated/installed ABI libraries:

- `AGK2Template/src/main/jniLibs/armeabi-v7a/libandroid_player.so`
- `AGK2Template/src/main/jniLibs/armeabi-v7a/libopenxr_loader.so`
- `AGK2Template/src/main/jniLibs/arm64-v8a/libandroid_player.so`
- `AGK2Template/src/main/jniLibs/arm64-v8a/libopenxr_loader.so`

The NDK intermediate outputs are under `AGK2Template/build/jniObjs/local/{armeabi-v7a,arm64-v8a}/`, including `libandroid_player.so`, `libopenxr_loader.so`, `libandroid_native_app_glue.a`, and object/dependency files. Both final `libandroid_player.so` files were verified as the expected 32-bit ARM and 64-bit AArch64 ELF files. `llvm-readelf -d` confirmed `DT_NEEDED: libopenxr_loader.so` in each, and the OpenXR loader was installed for each ABI. Thus, in this pinned NDK run, the loader was successfully handled and dynamically linked even though the makefile currently lists the prebuilt shared module under `LOCAL_STATIC_LIBRARIES`. The suspected mismatch did **not** produce a failure and no linkage change was made.

The successful native build emitted these warnings:

- `APP_PLATFORM android-24 is higher than android:minSdkVersion 1 in ./AndroidManifest.xml`; the manifest lacks `uses-sdk`, while the Gradle module separately declares minSdk 16. This was a warning, not a build failure.
- On arm64 `jni/Core.cpp` lines 566 and 571, integer `id` is cast to `void*` (`-Wint-to-void-pointer-cast`). Two warnings were reported.

Native command output is preserved at `/tmp/agk_android_openxr_ndk_build_after_clean.log`.

Then ran the requested Gradle command from the project root:

```sh
cd /home/master/projects/AGKRepo-main/AGK/apps/template_android_openxr
bash ./gradlew :AGK2Template:assembleDebug
```

**Gradle succeeded:** `BUILD SUCCESSFUL in 1m 1s`, 30 tasks executed. The APK exists at:

`/home/master/projects/AGKRepo-main/AGK/apps/template_android_openxr/AGK2Template/build/outputs/apk/debug/AGK2Template-debug.apk`

The APK was 12 MB and its ZIP listing contains `libandroid_player.so` and `libopenxr_loader.so` under both `lib/armeabi-v7a/` and `lib/arm64-v8a/`. This confirms packaging, not headset runtime behavior; the APK was not installed/launched on a device. Gradle output was captured at `/tmp/agk_android_openxr_gradle_assembleDebug.log`.

Additional non-fatal Gradle/tooling warnings: command-line tools 22.0 reported SDK XML v4 while AGP 7.2.1 understands through v3; the manifest has duplicate `LAUNCHER` categories; symbol stripping could not process `libandroid_player.so` and `libopenxr_loader.so`, so they were packaged as-is; shared Android Java sources report deprecated API/unchecked-operation notes; and Gradle 7.5 reports deprecated features that are incompatible with Gradle 8.0. During this build Gradle also installed Android SDK Platform-Tools revision 37.0.1 under `/home/master/Android/Sdk/platform-tools` because the build requested it.

No project source or build configuration was changed as part of the successful native/APK build. The initial Windows-dependency-file failure above is preserved as the first attempt's historical result; clearing only generated NDK intermediates allowed the authentic Linux NDK build and APK packaging to complete.

## Meta Quest 2 APK install investigation

Inspection only. No project file was modified and no build/rebuild or device install was run during this investigation.

### Reported install failure

The user reported that ADB was working and the Quest was authorized, but APK installation failed with this exact error:

```text
INSTALL_FAILED_DEPRECATED_SDK_VERSION: App package must target at least SDK version 23, but found 16
```

### Where target SDK 16 comes from

The installed APK was inspected with:

```sh
/home/master/Android/Sdk/build-tools/31.0.0/aapt dump badging AGK2Template/build/outputs/apk/debug/AGK2Template-debug.apk
```

It reports `compileSdkVersion='31'`, `sdkVersion='16'`, and `targetSdkVersion='16'`. The target value comes from the Android app module's `defaultConfig` in:

`AGK/apps/template_android_openxr/AGK2Template/build.gradle` — `targetSdkVersion 16` (line 11 at inspection time).

The source `AGK2Template/src/main/AndroidManifest.xml` does not declare a `<uses-sdk>` element or a target SDK. The Gradle manifest merger generates `uses-sdk` with min 16 and target 16 from the module's Gradle settings. The merged manifest and APK metadata independently confirm this.

### Current SDK configuration

- `compileSdkVersion`: **31**
- `targetSdkVersion`: **16**
- `minSdkVersion`: **16**
- Native `Application.mk` separately sets `APP_PLATFORM := android-24`; this is the NDK native API level and not the APK's target SDK. It was not changed during this inspection.

### SDK 16-specific dependencies found in the inspected project

Within `/AGK/apps/template_android_openxr` (including the project-owned Java, C/C++, OpenXR, manifest, and Gradle files), the only explicit target SDK 16 declaration is the module's Gradle setting. Searches found no `Build.VERSION.SDK_INT`, `VERSION_CODES`, runtime target-SDK checks, API-16 conditional branches, or code explicitly relying on target SDK 16. The manifest's main activity already declares `android:exported="true"`, as required for its intent filter when targeting Android 12/API 31; the merged AndroidX provider declares `android:exported="false"`.

This bounded inspection does not include shared Java implementation files referenced from outside the template directory (`AGK/apps/android_common`) or the linked prebuilt AGK Android engine archives. Therefore it cannot establish that every dependency has no target-sensitive behavior.

### Raising target SDK to API 31

The existing project already compiles against API 31, uses AGP 7.2.1, and has an explicit exported value on its intent-filter activity. Nothing in the inspected project-owned code directly requires target 16. To resolve the Quest's stated policy error, its immediate stated minimum is target API **23**; target 31 is within the already configured compile SDK level.

However, static inspection alone cannot certify target 31 as behaviorally safe. Targeting API 30+ enables scoped-storage enforcement on Android 11; the manifest currently requests `WRITE_EXTERNAL_STORAGE`, and Android documents that legacy storage opt-out is ignored once targeting API 30. Targeting API 31 also requires explicit mutability flags on created `PendingIntent` objects. No `PendingIntent` creation was found in project-owned source, but the shared Android Java sources and prebuilt engine were outside this inspection. Android's package-visibility restrictions may also affect installed-app queries in dependencies. These behaviors require auditing those dependencies and testing the app on the Quest before calling an API 31 target safe. [Android 11 storage behavior](https://developer.android.com/about/versions/11/privacy/storage), [Android 11 target behavior](https://developer.android.com/about/versions/11/behavior-changes-11), [Android 12 target behavior](https://developer.android.com/about/versions/12/behavior-changes-12).

Conclusion: changing target SDK to 31 is technically supported by the current compile SDK configuration and no direct API-16 source dependency was found in the inspected template. It is **not yet confirmed safe at runtime** because storage and target-31 compatibility of external shared/engine components have not been checked on device. Target API 23 is the minimum indicated by this install error; API 31 should be followed by compatibility testing.

### Files required for a target SDK change

For the APK's target value, the only source configuration that needs to change is:

`AGK/apps/template_android_openxr/AGK2Template/build.gradle`

Specifically, change `targetSdkVersion 16` in `defaultConfig`. `compileSdkVersion 31` and `minSdkVersion 16` do not need to change merely to set target 31. The adjacent `//noinspection ExpiredTargetSdkVersion` suppression is no longer needed for target 31 and could be removed as cleanup, but it does not set the APK target. No target SDK entry needs to be added to the source manifest, and no Android.mk, Application.mk, native, OpenXR, Java, or root Gradle change is indicated by this inspection. No such change was made.

### Target SDK 23 rebuild

### Target SDK 23 native NDK build output

Actual output read from `/tmp/agk_android_openxr_target23_ndk.log`:

```text
Android NDK: WARNING: APP_PLATFORM android-24 is higher than android:minSdkVersion 1 in ./AndroidManifest.xml. NDK binaries will *not* be compatible with devices older than android-24. See https://android.googlesource.com/platform/ndk/+/master/docs/user/common_problems.md for more information.
[armeabi-v7a] Install        : libandroid_player.so => jniLibs/armeabi-v7a/libandroid_player.so
[armeabi-v7a] Install        : libopenxr_loader.so => jniLibs/armeabi-v7a/libopenxr_loader.so
[arm64-v8a] Install        : libandroid_player.so => jniLibs/arm64-v8a/libandroid_player.so
[arm64-v8a] Install        : libopenxr_loader.so => jniLibs/arm64-v8a/libopenxr_loader.so
```

This saved target-23 NDK log contains only the warning and the four ABI install lines. It does not contain source compilation or `SharedLibrary` link lines; those are not present in this log and are not reproduced here.

Changed only `targetSdkVersion 16` to `targetSdkVersion 23` in `AGK2Template/build.gradle` to meet the Meta Quest 2 install error's stated minimum target API 23. Ran `bash ./gradlew :AGK2Template:assembleDebug` from the Android OpenXR project root; the rebuild succeeded (`BUILD SUCCESSFUL`). Verified with Build Tools 31.0.0 `aapt dump badging`: APK `targetSdkVersion='23'`, `sdkVersion='16'` (min SDK), and `compileSdkVersion='31'`.

APK: `/home/master/projects/AGKRepo-main/AGK/apps/template_android_openxr/AGK2Template/build/outputs/apk/debug/AGK2Template-debug.apk`.

Warnings only: the manifest contains duplicate `android.intent.category.LAUNCHER` entries (lines 52 and 54), and Gradle reports deprecated features that will be incompatible with Gradle 8.0. No errors occurred. The APK was not installed or launched. No other source/configuration changes were made.

#### Native build run before APK assembly

After clarifying the required build order, reran the Linux equivalent of `jniCompile.bat` from `AGK2Template/src/main` using the pinned NDK command:

```sh
/home/master/Android/Sdk/ndk/27.0.11718014/ndk-build NDK_OUT=../../build/jniObjs NDK_LIBS_OUT=./jniLibs
```

It succeeded and installed `libandroid_player.so` and `libopenxr_loader.so` for both `armeabi-v7a` and `arm64-v8a`. The only NDK warning was the existing `APP_PLATFORM android-24` versus manifest fallback `android:minSdkVersion 1` warning. Immediately afterward, `bash ./gradlew :AGK2Template:assembleDebug` succeeded (`BUILD SUCCESSFUL`; all 30 tasks were up-to-date). The APK at the path above was rechecked with `aapt` and still reports target SDK 23, min SDK 16, and compile SDK 31. It was not installed or launched. Raw outputs: `/tmp/agk_android_openxr_target23_ndk.log` and `/tmp/agk_android_openxr_target23_gradle_after_ndk.log`.
## Android OpenXR optional passthrough fix and Quest retest (2026-09-27)

### Root cause and source fix

The prior Quest 2 run showed that instance creation failed because Android requested `XR_FB_passthrough` as a required extension. The Quest runtime skipped exposing that extension to this package because its manifest does not declare `com.oculus.feature.PASSTHROUGH`. Source inspection confirmed the standard Android OpenXR template does not call AGK's passthrough enable/disable functionality. `XR_FB_passthrough` is therefore optional for this template; `XR_KHR_opengl_es_enable` remains required for its Android OpenGL ES graphics binding.

Changed only `AGK/apps/template_android_openxr/AGK2Template/src/main/jni/agkopenxr.cpp`:

- Keep `XR_KHR_opengl_es_enable` required and request `XR_FB_passthrough` only when the runtime advertises it.
- Check both OpenXR extension enumeration calls and report unavailable optional passthrough without aborting initialization.
- Mark initialization failed when a required extension is missing or `xrCreateInstance` fails/returns no valid handle.
- Guard `GetInstanceProperties()` against a null instance so it does not call an instance-dependent OpenXR function without a valid `XrInstance`.

No manifest feature was added because the standard template does not use passthrough. The Android manifest, Android.mk, Gradle files, SDK/NDK configuration, and Linux desktop Tier 2 source files were left unchanged. The fix follows the required OpenXR sequence: instance-dependent calls only proceed after successful creation of a valid instance, while optional passthrough does not prevent a GLES session from starting.

### node0001 build and APK verification

Machine: `node0001`. NDK `27.0.11718014-beta1`; Gradle 7.5; Android Gradle Plugin 7.2.1; compile SDK 31; target SDK 23; minimum SDK 16.

Exact native command, run from `AGK/apps/template_android_openxr/AGK2Template/src/main`:

```sh
/home/master/Android/Sdk/ndk/27.0.11718014/ndk-build NDK_OUT=../../build/jniObjs NDK_LIBS_OUT=./jniLibs
```

Native result: succeeded. Both `armeabi-v7a` and `arm64-v8a` compiled and linked `libandroid_player.so`; `libandroid_player.so` and `libopenxr_loader.so` were installed for both ABIs. The NDK emitted the existing warning that `APP_PLATFORM android-24` exceeds the manifest fallback `android:minSdkVersion 1`.

Exact Gradle command, run from `AGK/apps/template_android_openxr`:

```sh
bash ./gradlew :AGK2Template:assembleDebug
```

Gradle result: `BUILD SUCCESSFUL` (30 tasks, 4 executed and 26 up-to-date). Observed non-fatal messages included inability to strip `libandroid_player.so` (the libraries were packaged as-is) and Gradle deprecation notices for compatibility with Gradle 8.0.

APK: `/home/master/projects/AGKRepo-main/AGK/apps/template_android_openxr/AGK2Template/build/outputs/apk/debug/AGK2Template-debug.apk`, **12,294,133 bytes**. APK metadata reports compile SDK 31, target SDK 23, and minimum SDK 16. APK contents include `libandroid_player.so` and `libopenxr_loader.so` for both `armeabi-v7a` and `arm64-v8a`. The merged manifest has no passthrough feature declaration, as intended for this template.

### Quest 2 retest

The APK installed successfully on authorized Quest 2 device `1WMHH866EN1221`. Package: `com.mycompany.mytemplate`; resolved activity: `com.mycompany.mytemplate/com.thegamecreators.agk_player.AGKActivity`. `am start -W` returned `Status: ok`.

The retest confirms the previous no-instance failure is fixed: the log reports `XR_FB_passthrough is unavailable; continuing without optional passthrough support.`, then successful `xrCreateInstance`, `xrGetInstanceProperties` (runtime `Oculus - 207.299.0`), session creation, swapchain creation, and transition to VISIBLE/FOCUSED. This demonstrates OpenXR instance/session initialization. It does not conclusively demonstrate correct rendered application content.

The app was still foreground and had a process approximately 10 seconds after launch. Later, Android recorded process `30216` exiting itself at `2026-09-27 16:54:41.044`, status 0; the cause of this later clean exit is undetermined. The log also contains repeated `libEGL: called unimplemented OpenGL ES API` messages, plus `Failed to sync Actions` and `Failed to apply haptic feedback` messages. No app `AndroidRuntime` fatal exception, `FATAL EXCEPTION`, native fatal signal, or native crash was observed. The EGL messages mean renderer display should be checked in a further Quest run; no visual confirmation was captured here.

Full final Quest logcat capture: `/tmp/agk_quest2_optional_passthrough_fixed_final_logcat.txt` (11,729 lines; 1,496,684 bytes). Captured NDK and Gradle outputs: `/tmp/agk_android_openxr_optional_passthrough_node0001_ndk.log` and `/tmp/agk_android_openxr_optional_passthrough_node0001_gradle.log`.

### node1101 status and remaining verification

The node1101 host could not be resolved from this session (`ssh: Could not resolve hostname node1101: Temporary failure in name resolution`). Its project files were not inspected, compared, changed, or built; differences between node1101 and node0001 remain unknown. The fix and successful build above apply to node0001 only.

Physical Quest verification remains: confirm the intended AGK content renders normally and investigate the later clean self-exit and repeated EGL messages if reproducible. No speculative fix was made. No files beyond the Android OpenXR source change and this progress-log addition were changed for this work.

## node1101 synchronization from verified node0001 source (2026-09-27)

node0001 (`/home/master/projects/AGKRepo-main`) is the source of truth for the verified Linux Tier 2 and Android OpenXR changes. Before synchronization, checksum comparison of 5,000+ source/configuration candidates found one source content difference: Android `agkopenxr.cpp`. The three Linux Tier 2 files and Android `AGK2Template/build.gradle` were already identical. No node1101-only source changes were found in the compared candidate set. node0001 also had `README.md` and `LINUX_TIER2_INSPECTION.md` plus several `AGK_Build/**/readme.txt` files absent on node1101; those unrelated documentation/readme files were not copied. The progress log had newer history on node0001 and was preserved/extended here rather than replaced.

Before changing node1101, a full backup was created at `/home/master/projects/AGKRepo-linuxworking-before-node0001-sync` (7.0G). The only Android SDK path in node1101 `local.properties` was preserved as `/home/master/Android/Sdk`; that SDK and pinned NDK 27.0.11718014 are installed on node1101.

Files synchronized from node0001: `AGK/apps/template_android_openxr/AGK2Template/src/main/jni/agkopenxr.cpp`. The following requested files were checked and already matched node0001 byte-for-byte, so their contents needed no change: `AGK/platform/linux/Source/LinuxCore.cpp`, `AGK/renderer/OpenGL2/OpenGL2.cpp`, `AGK/renderer/Vulkan/AGKVulkan.cpp`, and `AGK/apps/template_android_openxr/AGK2Template/build.gradle`. This progress log was extended with the missing OpenXR fix/retest history and this synchronization record. node1101 source synchronization is complete; SHA-256 identity checks and Linux/Android build verification on node1101 remain to be performed. The successful Quest OpenXR initialization described above used the node0001 build; node1101 has not been runtime-tested on Quest.
## node1101 synchronized source and build verification (2026-09-27)

node0001 was reachable at `192.168.1.100` by SSH. The node1101 source backup was created before synchronization at `/home/master/projects/AGKRepo-linuxworking-before-node0001-sync` (7.0G). Checksum comparison of more than 5,000 source/configuration candidates found only one source-code content difference before synchronization: `AGK/apps/template_android_openxr/AGK2Template/src/main/jni/agkopenxr.cpp`. It was copied from node0001. The three Linux Tier 2 source files and Android `AGK2Template/build.gradle` already matched byte-for-byte. No node1101-only files in the compared source/configuration candidate set were found. Several node0001-only readme/documentation files were left untouched on node1101. Android build scripts, Android.mk, Application.mk, manifest, and Linux makefiles were also confirmed identical.

node1101's `local.properties` was not copied or changed; it already points to `/home/master/Android/Sdk`, which exists on node1101. The pinned NDK directory is installed there at `/home/master/Android/Sdk/ndk/27.0.11718014`, revision `27.0.11718014-beta1`.

### SHA-256 source identity

The following SHA-256 values matched on node0001 and node1101 after synchronization:

| File | SHA-256 |
|---|---|
| `AGK/platform/linux/Source/LinuxCore.cpp` | `85489e991ee0e31d4c58e691d85043af3a11612afff49e981d13e3c21b5e6277` |
| `AGK/renderer/OpenGL2/OpenGL2.cpp` | `963c68f29056530af5352d167a2ed17dc60e60a3a2675d92ccf2ae7f90ce3fe5` |
| `AGK/renderer/Vulkan/AGKVulkan.cpp` | `68db6c0e1050e04f2b3c28d2d106b81cf3ba9ca6460b5131691ef64a18ff21ed` |
| `AGK/apps/template_android_openxr/AGK2Template/src/main/jni/agkopenxr.cpp` | `937120bd219824d3480e889d672d20ae7924876ea218d0b06b1f93ad97a32444` |
| `AGK/apps/template_android_openxr/AGK2Template/build.gradle` | `56b0e3b1bdafb5ddf5e82edd812b2f7d1d89269de3fd8c368c436ca1474ff067` |

The progress-log SHA-256 values remain different because node1101's earlier historical entries were preserved instead of replacing the log. The missing OpenXR fix/retest history and synchronization/build records were appended on node1101.

### node1101 Linux Tier 2 build

No clean operation was run. From `AGK/`, ran `CPLUS_INCLUDE_PATH=/tmp/agk-install/include make -j2 CFLAGS='-O2 -std=c++11'`; from `AGK/apps/template_linux/`, ran `CPLUS_INCLUDE_PATH=/tmp/agk-install/include LIBRARY_PATH=/tmp/agk-install/lib make -j2`. Both commands succeeded. `AGK/platform/linux/Lib/Release64/libAGKLinux.a` and `AGK/apps/template_linux/build/LinuxApp64` were produced. No warning or error text was found in the captured build output. Full output captured on node0001: `/tmp/agk_sync_node1101_linux_build.log`.

### node1101 Android OpenXR build and APK

Environment: Gradle wrapper 7.5; Android Gradle Plugin 7.2.1; NDK `27.0.11718014-beta1`; compile SDK 31; target SDK 23; minimum SDK 16.

Native build, from `AGK/apps/template_android_openxr/AGK2Template/src/main`:

```sh
/home/master/Android/Sdk/ndk/27.0.11718014/ndk-build NDK_OUT=../../build/jniObjs NDK_LIBS_OUT=./jniLibs
```

Result: succeeded for `armeabi-v7a` and `arm64-v8a`; both `libandroid_player.so` and `libopenxr_loader.so` were installed for each ABI. The observed NDK warning says `APP_PLATFORM android-24` exceeds the manifest fallback `android:minSdkVersion 1`.

Gradle build, from `AGK/apps/template_android_openxr`:

```sh
bash ./gradlew :AGK2Template:assembleDebug
```

Result: `BUILD SUCCESSFUL in 13s` (30 tasks; 4 executed, 26 up-to-date). Non-fatal output: `libandroid_player.so` could not be stripped and was packaged as-is; Gradle deprecation notices say features are incompatible with Gradle 8.0; Android tooling warned it supports SDK XML through version 3 but encountered version 4.

APK: `/home/master/projects/AGKRepo-main/AGK/apps/template_android_openxr/AGK2Template/build/outputs/apk/debug/AGK2Template-debug.apk`, **12,295,209 bytes**. `aapt` verified compile SDK 31, target SDK 23, and minimum SDK 16. APK ZIP contents include `libandroid_player.so` and `libopenxr_loader.so` for both `armeabi-v7a` and `arm64-v8a`. Full captured output on node0001: `/tmp/agk_sync_node1101_ndk.log` and `/tmp/agk_sync_node1101_gradle.log`.

Node1101 now matches node0001 for the required Linux Tier 2 and Android OpenXR source/configuration files, and both Linux and Android builds passed there. No APK was installed or launched on a Quest from node1101. The Quest OpenXR initialization evidence recorded above is from the node0001 APK; node1101 has not had a separate Quest runtime test.

## Windows and Android OpenXR source comparison (2026-09-27)

Read-only source analysis. No source files were modified, and no build or runtime test was performed for this comparison.

The Android implementation is `AGK/apps/template_android_openxr/AGK2Template/src/main/jni/agkopenxr.cpp`. The corresponding Windows implementation is `AGK/apps/template_windows_openxr/agkopenxr.cpp`, included by `AGK/apps/template_windows_openxr/template.cpp`. The files share the same overall OpenXR lifecycle and most of the implementation. Their intentional platform branches use different OpenXR graphics extensions and bindings for desktop OpenGL/Win32 versus OpenGL ES/Android.

### Lifecycle comparison

| Area | Windows implementation | Android implementation | Assessment |
|---|---|---|---|
| Extension enumeration and selection | Requests `XR_KHR_opengl_enable` and `XR_EXT_debug_utils`. Both pass through the required-extension check. Its extension enumeration has unchecked calls and repeats the query. | Requires `XR_KHR_opengl_es_enable`. Checks both enumeration calls. `XR_FB_passthrough` is enabled only when advertised and otherwise startup continues without it. | The OpenGL/OpenGL ES extensions are platform/API requirements. The optional passthrough behavior is intentional and appropriate. Windows treats debug utils as required even though no debug-utils function use was found. |
| Instance creation | Creates an instance with Windows application metadata and marks failure if `xrCreateInstance` fails. | Uses Android application metadata, marks failure for a missing required extension, and checks both the result and returned instance handle. | Same lifecycle stage; Android has stronger failure handling. |
| Instance properties | Queries and logs runtime name/version; assumes the instance handle is valid. | Performs the same query and log, after checking for a null instance. | Logically equivalent in successful startup; Android is safer after earlier failure. |
| System selection | Requests `XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY` and queries system properties. | Same. | Equivalent and platform-independent. |
| Graphics binding and session | Gets AGK's `HDC`/`HGLRC`, queries `xrGetOpenGLGraphicsRequirementsKHR`, and creates a session with `XrGraphicsBindingOpenGLWin32KHR`. | Gets AGK's EGL display/surface/context/config, queries `xrGetOpenGLESGraphicsRequirementsKHR`, and uses `XrGraphicsBindingOpenGLESAndroidKHR`. The binding structure has no surface field, so only display/config/context are assigned. | These differences are required by Win32 desktop OpenGL versus Android OpenGL ES. Android flags invalid EGL values but still proceeds to attempt `xrCreateSession`. |
| Swapchains and reference spaces | Enumerates formats, creates color/depth swapchains per view, builds render targets, and creates world/view reference spaces. | Follows the same sequence and reference-space behavior with the OpenGL ES swapchain image type. | Structurally equivalent; graphics API image types differ as expected. Static inspection cannot guarantee all formats/operations work on every GLES runtime. |
| Frame loop and submission | Waits/begins frames, renders when session state and `shouldRender` allow, and ends frames with projection layers. | Uses the same OpenXR wait/begin/render/end sequence. Android's AGK pre-render calls differ from Windows: it does not make the same explicit update/shadow/2D/3D calls at that point. | OpenXR frame lifecycle is equivalent. The AGK rendering-path difference warrants device validation; source comparison alone does not prove it is harmless. |
| Image acquire/release | Acquires and waits for color/depth images, renders views, releases images, then submits projection views. | Same shared logic. | Equivalent in structure. Both paths log some acquire/wait errors and continue, potentially using invalid image state. |
| Events and session state | Handles OpenXR events, begins on `READY`, ends on `STOPPING`, and exits on loss/exit states. | Same OpenXR state handling. Android OS events are handled through native-app callbacks/main-loop processing; `PollSystemEvents` returns because that work happens elsewhere. | OpenXR event handling is equivalent. Android's separate OS event path is platform-specific. Both set running flags even if `xrBeginSession` or `xrEndSession` fails. |
| Shutdown | Normal shutdown destroys AGK resources, swapchains, spaces, session, and instance. | Same order, with passthrough objects handled when applicable. | Equivalent after successful initialization. Staged initialization can fail after creating resources, while normal `End` cleanup is gated on full initialization; partial-startup cleanup appears incomplete in both. |

### Windows branch selection concern

The inspected Windows `agkopenxr.cpp` has `//#define _WINDOWS_` commented out and `#define _ANDROID_` active. The Windows project file defines `_WINDOWS`, without the trailing underscore required by the source's `#ifdef _WINDOWS_` branches. Based on these checked-in files, the Android branches appear selected and the intended Windows branches appear unselected unless another build mechanism changes the macros. This is a significant source/build integration inconsistency; no build was run, so its concrete compile/runtime consequences were not established.

### Findings by requested category

- **OpenXR lifecycle consistency:** The intended platform implementations follow the same sequence: extensions, instance, instance properties, HMD system, graphics binding/session, swapchains, reference spaces, frame loop, events, and cleanup. Most code is shared.
- **Platform-specific differences:** Win32 desktop OpenGL uses `XR_KHR_opengl_enable` and `XrGraphicsBindingOpenGLWin32KHR`; Android GLES uses `XR_KHR_opengl_es_enable` and `XrGraphicsBindingOpenGLESAndroidKHR`. Android also initializes the loader with Android activity/VM context and integrates native-app lifecycle events. These are expected differences.
- **Potential inconsistencies:** Windows currently appears to select `_ANDROID_`, while the project defines `_WINDOWS` rather than `_WINDOWS_`. Windows also treats debug utils as required despite no apparent use. Android continues to attempt session creation after detecting invalid EGL handles.
- **Potential bugs:** Both implementations may continue frame operations after some OpenXR errors, and partial initialization failures may not clean up already-created resources. These are shared error-handling concerns, not Android-only lifecycle differences.
- **Things that require further testing:** Confirm Windows builds and selects its Win32/OpenGL branch. On Android, validate actual GLES rendering and submission on the headset, including the GLES runtime warnings recorded in the Quest retest section. Test session/frame error recovery and cleanup after partial initialization.

## Android phone access to node1101 Codex

Date: 2026-09-27. Initial inspection completed; setup is **partially complete**. No SSH, firewall, network, router, DNS, Codex installation, AGK source, or other service configuration was changed.

### Inspected state

- Host: `node1101`, Ubuntu 24.04.4 LTS.
- Current active LAN interface/address: `enp3s0f0`, `192.168.1.100/24`; the address is DHCP-assigned/dynamic. Default route is via `192.168.1.1`.
- OpenSSH Server package `openssh-server` version `1:9.6p1-3ubuntu13.19` is installed. `ssh.service` and `ssh.socket` are active; `ssh.socket` is enabled. The socket already listens on port 22 at `0.0.0.0:22` and `[::]:22`.
- SSH configuration is the Ubuntu default `/etc/ssh/sshd_config`, with no files in `/etc/ssh/sshd_config.d/`. It includes the usual commented defaults for password/public-key authentication and sets `UsePAM yes`, `KbdInteractiveAuthentication no`, and `X11Forwarding yes`. Systemd socket activation supplies the existing wildcard listeners. No SSH configuration change is needed for a LAN client to reach the current listener.
- `master` is UID/GID 1000 with home `/home/master` and shell `/bin/bash`.
- No `/home/master/.ssh` directory or user SSH key files exist. OpenSSH host keys are present under `/etc/ssh`. Authentication has not been tested from a phone; with no authorized user key, the expected initial method is the account password if password authentication is enabled by the installed OpenSSH defaults.
- `codex` resolves to `/usr/local/bin/codex`, pointing to `/usr/local/lib/node_modules/@openai/codex/bin/codex.js`. `codex --version` works from the AGK repository and reports `codex-cli 0.157.1`.
- `tmux` is not installed.
- `ufw status verbose` could not be run because sudo required a password. The readable `/etc/ufw/ufw.conf` says `ENABLED=no`; the `nftables` service is inactive/disabled. This does not prove the complete kernel firewall ruleset is empty. No firewall rules were changed. The router was not inspected or modified, so any pre-existing router port-forwarding state is unknown. No Internet exposure was configured by this work.
- `pihole-FTL.service` was observed running. No services or system configuration were changed.

### Changes and verification

No changes have yet been made. The SSH server is already running and listening on the expected LAN-reachable port, so it was intentionally left alone. `tmux` is the sole missing required package. Installing it is pending because this session has no passwordless sudo; no package was installed. The normal Codex command was verified, but tmux workflow verification cannot happen until tmux is installed.

No SSH login from the Android phone has been tested. The current server LAN address to use is `192.168.1.100`, port `22`, username `master`. Because the address is dynamic, it may change when the DHCP lease changes; no DHCP reservation or network configuration was made.

### Phone commands after tmux installation

Use Termux with OpenSSH (or an SSH client such as Termius) on the Android phone and connect with:

```sh
ssh master@192.168.1.100 -p 22
```

In the remote shell, attach to an existing session:

```sh
tmux attach -t codex
```

If none exists, create it:

```sh
tmux new -s codex
```

Inside that session, launch the existing Codex installation in the repository:

```sh
cd /home/master/projects/AGKRepo-main && codex
```

Codex is not configured to launch automatically at SSH login.

### Remaining step

Install only tmux using the normal Ubuntu package manager from an administrator-authorized shell:

```sh
sudo apt update && sudo apt install tmux
```

Then verify `tmux -V`, create/attach the `codex` session as above, and test an Android LAN SSH login. SSH currently binds wildcard IPv4/IPv6 addresses under the pre-existing systemd socket. No router forwarding was requested or configured; since firewall status and router forwarding were not fully verifiable without administrator/router access, those existing external-reachability conditions remain unknown.

### Android phone access continuation: tmux installed and session ready (2026-09-28)

The user installed tmux. Verified on node1101:

- `tmux` is `/usr/bin/tmux`, version **3.4**.
- Created detached tmux session `codex` for user `master`, with its shell working directory set to `/home/master/projects/AGKRepo-main`.
- `tmux has-session -t codex` succeeded, `tmux list-sessions` showed `codex: 1 windows`, and the pane path was `/home/master/projects/AGKRepo-main`.
- Codex still launches normally; current `/usr/local/bin/codex --version` reports **codex-cli 0.158.0**.
- `ssh.service` and `ssh.socket` remain active, with port 22 listening on `0.0.0.0` and `[::]`. Current server LAN address remains **192.168.1.100**. No SSH configuration, firewall, network interface, router, DHCP, DNS, Codex, AGK, or other service configuration was changed. Pi-hole FTL had been observed active in the initial inspection.
- The tmux session contains a shell only. Codex was deliberately **not** launched automatically; start it when desired from that session with `cd /home/master/projects/AGKRepo-main && codex`.

From Android, use **Termux with its OpenSSH client** (or a graphical SSH app such as Termius), then connect:

```sh
ssh master@192.168.1.100 -p 22
```

Accept the first-connection SSH host-key prompt only after confirming it is node1101. SSH uses the existing Ubuntu configuration/default password authentication; there are no user SSH keys under `/home/master/.ssh`. After login, attach to the ready session:

```sh
tmux attach -t codex
```

To detach later without stopping Codex, press `Ctrl-b`, then `d`. Reconnect over SSH and run `tmux attach -t codex` again. If the session is ever absent, create it with `tmux new -s codex`.

**Remaining verification:** The server-side tmux and Codex checks succeeded, but an SSH login from the Android phone has not yet been tested. The address is DHCP-assigned and can change. UFW was reported disabled and the nftables service inactive during initial inspection; complete firewall rules and router forwarding were not verified. No Internet exposure or router port forwarding was configured, and the pre-existing ssh.socket continues to bind all addresses, so any prior external reachability state remains unknown.
