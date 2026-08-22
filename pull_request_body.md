### Summary

In **Methane Kit v0.8.2**, the following improvements have been introduced:

- **Compiler Compatibility**: Build compatibility has been fixed for Visual Studio 2026 (MSVC v143+), latest Clang/AppleClang on macOS Sequoia/Xcode 26, and GCC. New CMake presets for VS2026 have been added alongside existing VS2022 presets. VS2026 requires CMake 4.2 or later; other presets require CMake 3.24+.
- **Vulkan RHI Stability and Validation Fixes**: Multiple Vulkan validation errors have been resolved, including semaphore lifecycle issues (`VUID-vkAcquireNextImageKHR-semaphore-01779`), descriptor set copy by-binding fix, empty pipeline stage masks (`VUID-vkCmdPipelineBarrier`), HLSL reflection extensions handling (`VUID-VkShaderModuleCreateInfo-pCode-08742`), and primitive restart behavior on MoltenVK. The per-device Vulkan dispatcher initialization bug on multi-GPU systems has been fixed. Image acquisition now retries on timeout and handles surface-lost conditions. (close #158)
- **Command Execution Stability**: A race condition in `CommandQueueTracking` that could cause command list resets to fail (`WaitUntilCompleted` / `CompleteExecution` FIFO ordering) has been fixed by refactoring `ProcessExecutingCommandListSet` into an atomic pop-and-wait operation. Frame-less command list sets no longer interfere with frame-zero completion state.
- **MacOS / Metal Build Fixes**: Metal Toolchain download is now automated in CI for Xcode 26+. Objective-C ARC handling in Vulkan RHI for Apple platforms has been fixed. MacOS app delegate now suppresses secure restorable state warnings and disables unsupported window state restoration.

### Graphics libraries

- Fixed `RenderCommandList::UpdateDrawingState` logic inversion that incorrectly skipped primitive type change tracking.
- `CommandListSet::GetCombinedName()` now returns `std::string` by value (member made `mutable`) to prevent data races when accessed concurrently with name-change callbacks. Added a mutable mutex to synchronize lazy semaphore creation and access.
- `CommandQueueTracking`: Replaced `GetNextExecutingCommandListSet()` with `PopNextExecutingCommandListSet()` that atomically pops and waits, preventing a race between `WaitUntilCompleted`, `CompleteExecution`, and the background execution-waiting thread; added `ProcessExecutingCommandListSet` template helper and `IsExecutingOnFrameIndex` for correct FIFO-ordered frame completion. Frame wait state is now only reset for frame-indexed command list sets, not frame-less ones.
- Added new `Methane::StbImage.h` wrapper header that correctly suppresses compiler warnings and conditionally disables SIMD for GCC debug builds, replacing ad-hoc `stb_image.h` inclusions. The STB dependency is now installed and exported for external consumers.

### Vulkan RHI

- **Semaphore lifecycle fix**: Removed the per-frame `vk::Fence` from `RenderContext::FrameSync`; image acquisition now waits on the render queue `WaitUntilCompleted` instead of a CPU-side fence, correctly satisfying `VUID-vkAcquireNextImageKHR-semaphore-01779`. Added `g_image_acquire_timeout_ns` (1 s, 64-bit `uint64_t`) with retry logic for timeout and surface-lost conditions.
- **Image acquisition robustness**: `AcquireNextImage()` now wraps `acquireNextImageKHR` in a retry loop, explicitly handling non-fatal results (`eTimeout`, `eNotReady`) by retrying, and `eErrorSurfaceLostKHR` by destroying the lost surface, recreating it, resetting the swapchain, and retrying.
- **Execution-completed semaphore**: Made `m_vk_unique_execution_completed_semaphore` lazily created and `mutable`; the semaphore is now only created and signalled for render command list sets that target a specific frame buffer, eliminating `VUID-vkQueueSubmit-pSignalSemaphores-00067` errors caused by permanently-signalled semaphores on non-frame command lists.
- **Device dispatcher**: `VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::Device)` is no longer called in `Device` constructor (fixes multi-GPU dispatch to the wrong ICD); it is now called once per `RenderContext` in `Initialize()`. Instance-level initialization is preserved per `Device.cpp` guidelines.
- **HLSL reflection extensions**: Added `VK_GOOGLE_HLSL_FUNCTIONALITY_1` and `VK_GOOGLE_USER_TYPE` device extensions; when not supported, SPIRV byte code is stripped of reflection instructions via `RemoveHlslReflectionFromSpirv()` before shader module creation, fixing `VUID-VkShaderModuleCreateInfo-pCode-08742`. Bounds validation is now always-on to prevent out-of-bounds access on malformed input.
- **Descriptor set copy**: Fixed `ProgramBindings` mutable descriptor copy to iterate per-binding rather than using a single flat count, preventing out-of-bounds descriptor access with non-contiguous bindings.
- **Pipeline barrier stage masks**: Empty `srcStageMask`/`dstStageMask` are now replaced with `eTopOfPipe`/`eBottomOfPipe` respectively, fixing `VUID-vkCmdPipelineBarrier-srcStageMask-03937` / `dstStageMask-03937`.
- **Primitive restart (MoltenVK)**: `RenderState` now detects `VK_KHR_portability_subset` and automatically enables primitive restart for strip/dynamic topologies on devices that cannot disable it, silencing MoltenVK "Metal does not support disabling primitive restart" warnings.
- **Surface lifetime management**: On `eErrorSurfaceLostKHR`, the old `vk::UniqueSurfaceKHR` is now moved to a local variable before creating the replacement, ensuring it is destroyed after swapchain cleanup rather than leaked via `release()`.
- **Removed MoltenVK workarounds** for `VkSubmitInfo` + `VkTimelineSemaphoreSubmitInfo` which are no longer necessary with the current Vulkan SDK.
- Removed stale `DebugUtilsMessengerCallback` message-ID suppression filters that are no longer needed; callback signature updated to use `vk::` types directly.
- Fixed `RenderPass::CreateNativeFrameBuffer` to validate attachment texture dimensions against frame size.
- Disabled Objective-C ARC for `RenderContext.mm` and `PlatformExt.mm` (workaround for `vulkan.hpp` constant-expression CAMetalLayer issue with ARC).
- Added explicit `[m_metal_view release]` in the Apple `RenderContext` destructor for non-ARC builds.

### Tests

- Added unit tests to cover `RenderCommandList` drawing with multiple primitive types in a single command list (`Draw` and `DrawIndexed`), covering the `PrimitiveType` change tracking bug.

### Build

- Added VS2026 (`Visual Studio 18 2026`) CMake configure and build presets for Win64/Win32 × DX/VK × Default/Profile/Scan variants. CMake 4.2+ is required for VS2026 presets.
- `Build.bat` default generator changed to VS2026; `--vs2022` flag added for backward compatibility.
- `Build/Unix/CI/InstallMacOsPrerequisites.sh` script added to download Metal Toolchain on Xcode 26+ using a BSD-compatible version comparison function.
- Fixed ARM64 architecture detection in `CMake/MethaneModules.cmake` to match all common variants (`ARM64`, `arm64`, `AARCH64`, `aarch64`) with a case-insensitive regex.
- Added `CLAUDE.md` with project guidance for Claude Code and updated CMake version requirements.

### Continuous Integration

- All CI workflows updated to use VS2026 presets on Windows.
- All GitHub Actions updated: `actions/checkout@v7`, `actions/cache@v6`, `actions/upload-artifact@v7`, `github/codeql-action/*@v4`.
- Sonar Scanner migrated from the legacy `RunSonarScanner.sh` shell script to `SonarSource/sonarqube-scan-action@v8.2.0` with separate steps for push and pull-request events.
- Added macOS prerequisites installation step in CI build, CodeQL, and Sonar scan workflows.
- Disabled CodeQL scan on macOS due to Metal Toolchain availability issues.
- Added security hardening: `persist-credentials: false` on checkout steps, `contents: read` permission grants, and proper GitHub expression syntax for Sonar project version.

### External libraries

- [DirectXShaderCompilerBinary](https://github.com/MethanePowered/DirectXShaderCompilerBinary) updated to `v1.9.2602` (pinned by immutable commit SHA)
- [VulkanHeaders](https://github.com/MethanePowered/VulkanHeaders) updated to `v1.4.357.0`
- [SPIRVCross](https://github.com/MethanePowered/SPIRVCrossBinary) updated to `v1.4.350.1`
- [Tracy](https://github.com/MethanePowered/Tracy) updated to `v0.13.1`
- [DirectXTex](https://github.com/MethanePowered/DirectXTex) updated to `v2.1.1` (2026-05)
- [DirectXHeaders](https://github.com/MethanePowered/DirectXHeaders) updated to `v1.619.5`
- [FMT](https://github.com/fmtlib/fmt) updated to `v12.2.0`
- [FTXUI](https://github.com/MethanePowered/FTXUI) updated to `v7.0.3`
- [HLSL++](https://github.com/MethanePowered/HLSLpp) updated to `v3.9`
- [TaskFlow](https://github.com/MethanePowered/TaskFlow) updated to `v4.1.0`
- [Catch2](https://github.com/MethanePowered/Catch2) updated to `v3.15.3`
- [CLI11](https://github.com/MethanePowered/CLI11) updated to `v2.7.2`
- [MagicEnum](https://github.com/MethanePowered/MagicEnum) updated to `v0.9.8`
- [IttApi](https://github.com/MethanePowered/IttApi) updated to `v3.28.2`
- [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) updated to `v0.43.1`
- [CMakeModules](https://github.com/MethanePowered/CMakeModules) updated (latest `methane` branch)
- [iOS-Toolchain.cmake](https://github.com/leetal/ios-cmake) updated to `v4.6.0` with correct standard configuration flag variable propagation for `try_compile`

<!-- This is an auto-generated comment: release notes by coderabbit.ai -->
## Summary by CodeRabbit

* **New Features**
  * Added Visual Studio 2026 build presets, while retaining Visual Studio 2022 support.
  * Expanded Apple platform toolchain support, including visionOS and combined simulator/device builds.
  * Added shared image-loading support, improved ARM64 detection, and a cross-platform application test runner.

* **Bug Fixes**
  * Improved Vulkan synchronization, shader compatibility, descriptor updates, and primitive restart handling.
  * Improved command-list processing and application shutdown reliability.
  * Fixed macOS window restoration compatibility.
  * Enhanced image acquisition robustness with retry logic for timeout and surface-lost conditions.
  * Fixed race conditions in semaphore access and frame completion tracking.
  * Corrected surface lifetime management to prevent leaks on device loss.

* **Documentation**
  * Updated build instructions and Metal toolchain setup guidance.
  * Documented CMake version requirements for VS2026 presets.
<!-- end of auto-generated comment: release notes by coderabbit.ai -->
