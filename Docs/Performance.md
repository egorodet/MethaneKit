# Methane Kit Performance Notes

This document collects performance findings which are not obvious from the code and which have been
mistaken for engine defects in the past. Each section states what was measured, on what configuration,
and which explanations were tested and ruled out.

- [Test Environment](#test-environment)
- [DirectX 12 Frame Rate is Lower than Vulkan on Windows](#directx-12-frame-rate-is-lower-than-vulkan-on-windows)
  - [Measured Frame Rates](#measured-frame-rates)
  - [Per-Frame CPU Breakdown](#per-frame-cpu-breakdown)
  - [Ruled Out](#ruled-out)
  - [Conclusions](#conclusions)
- [Present Rate Drops After Switching Rendering Device](#present-rate-drops-after-switching-rendering-device)
- [How to Reproduce These Measurements](#how-to-reproduce-these-measurements)

## Test Environment

All numbers below were collected on a single machine. They are reproducible on that configuration, but the
absolute values and the ratios are driver- and hardware-dependent, so treat them as an illustration of the
mechanism rather than as universal constants.

| Component      | Value                                                                 |
|----------------|-----------------------------------------------------------------------|
| CPU            | AMD Ryzen 9 9950X3D (16 cores)                                        |
| Discrete GPU   | NVIDIA GeForce RTX 5090, driver 32.0.16.1088 (drives the display)     |
| Integrated GPU | AMD Radeon(TM) Graphics, driver 32.0.21036.18                         |
| Display        | 120 Hz current refresh rate (240 Hz maximum)                          |
| OS             | Windows 11 Pro, build 26200                                           |
| Build          | CMake presets `VS2026-Win64-DX-Release` and `VS2026-Win64-VK-Release` |
| Window         | 3072 x 1728, foreground, V-Sync off unless stated otherwise           |

Note that the Khronos validation layer was **not** installed on this machine, so the Vulkan builds ran without
any validation overhead. The D3D12 debug layer is compiled in under `_DEBUG` only, so the Release builds
compared here also ran without it. This makes the comparison fair, but it also means Vulkan Debug builds on a
machine with the Vulkan SDK installed will look considerably slower than the numbers below.

## DirectX 12 Frame Rate is Lower than Vulkan on Windows

With V-Sync disabled, the tutorials report roughly **twice the FPS on Vulkan compared to DirectX 12** when
running the same sample on the same GPU. This is not caused by the DirectX render command encoding being less
efficient: Methane's own per-frame overhead is the same in both backends, and the difference is concentrated in
two native API calls.

### Measured Frame Rates

Release builds, V-Sync off, discrete GPU:

| Tutorial               | Vulkan    | DirectX 12  | Ratio |
|------------------------|-----------|-------------|-------|
| `MethaneHelloTriangle` | 12330 FPS | 5366 FPS    | 2.30x |
| `MethaneTexturedCube`  | 11518 FPS | 4938 FPS    | 2.33x |
| `MethaneShadowCube`    | 8610 FPS  | 4628 FPS    | 1.86x |

Debug builds of the same tutorials, where per-frame CPU work in the engine itself dominates, show a much
smaller difference (1.20x - 1.38x), which is the first hint that the gap is not in the engine code.

With **V-Sync enabled both backends run at exactly 120 FPS**, i.e. at the display refresh rate. The gap only
appears when the application is allowed to submit and present far more frames than the display can show.

### Per-Frame CPU Breakdown

The per-frame CPU cost was measured with temporary `QueryPerformanceCounter` instrumentation placed in the
shared `Methane::Graphics::Base` code, so that both backends were measured through identical call paths, plus
timers around the native submit and present calls. `MethaneHelloTriangle`, Release, V-Sync off:

| Phase                                               | Vulkan                            | DirectX 12                                                                      |
|-----------------------------------------------------|-----------------------------------|---------------------------------------------------------------------------------|
| `WaitForGpu(FramePresented)`                        | 0.3 us                            | 0.2 us                                                                          |
| Command queue bookkeeping and executing-lists mutex | 1.7 us                            | 1.7 us                                                                          |
| **Backend submit**                                  | **10.5 us** (`vkQueueSubmit`)     | **61.4 us** (`ExecuteCommandLists` 59.0 + fence `Signal` 2.1 + bookkeeping 0.1) |
| **Present**                                         | **53.6 us** (`vkQueuePresentKHR`) | **67.2 us** (`IDXGISwapChain::Present`)                                         |
| Total frame time                                    | ~82 us (~12000 FPS)               | ~180 us (~5400 FPS)                                                             |

Two native calls account for about 64 us of the ~100 us per-frame difference:

- `ID3D12CommandQueue::ExecuteCommandLists` costs roughly **six times** more than `vkQueueSubmit`, even though
  exactly **one** command list is submitted per frame in both backends (verified by instrumenting the submitted
  command list count).
- `IDXGISwapChain::Present` costs about 14 us more than `vkQueuePresentKHR`.

The remainder is command list encoding and reset, which was not isolated further.

Everything Methane itself does per frame - queue bookkeeping, the executing command lists mutex, and the
frame-presented wait - measures around 2 us and is **identical between the two backends**.

### Ruled Out

Each of the following was tested with an A/B measurement and does not explain the difference:

| Hypothesis                                                                                                   | Result                                                                                                                                                |
|--------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------|
| DXGI frame latency waitable object paces the CPU (`WaitForSwapChainLatency`, which has no Vulkan equivalent) | No effect: 5093 -> 5243 FPS with the wait removed, within run-to-run noise                                                                            |
| Swap-chain tearing not enabled, so presentation is throttled                                                 | Already correct: `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING` and `DXGI_PRESENT_ALLOW_TEARING` are both set, and the frame rate is far above the refresh rate |
| Present queue is too shallow                                                                                 | Frame buffers count swept from 2 to 8: 5313 -> 5609 FPS, essentially flat                                                                             |
| D3D12 debug layer or GPU-based validation is active                                                          | Enabled under `_DEBUG` only, and no global override is configured in the registry                                                                     |
| Multiple command lists submitted per frame in DirectX                                                        | Measured: exactly one                                                                                                                                 |
| Hardware-accelerated GPU scheduling or power plan                                                            | Default scheduling, High performance power plan                                                                                                       |

One genuine structural asymmetry does exist: D3D12 requires a separate `ID3D12CommandQueue::Signal` after
`ExecuteCommandLists`, whereas `vkQueueSubmit` carries the fence itself. That extra queue operation measures
2.1 us, so removing it would recover roughly 1% - it is not the cause and no change was made for it.

### Conclusions

- The difference is a real and correctly measured difference in the cost of native D3D12 and Vulkan calls on
  this driver, not an inefficiency in the DirectX backend of Methane Kit.
- It only manifests in an uncapped micro-benchmark. At 5000+ FPS on a 120 Hz display both backends are
  submitting and presenting roughly 45 times more frames than can ever be displayed, so the per-call overhead
  becomes the entire workload. The ratio narrows as the scene gets heavier (2.30x on Hello Triangle, 1.86x on
  Shadow Cube) and disappears entirely when the frame rate is display-limited.
- If this needs to be pushed further, the next step is a driver-level capture (PIX for Windows or Nsight)
  around `ExecuteCommandLists`, to determine whether the ~60 us is a kernel transition, driver lock contention
  or submission back-pressure. That cannot be distinguished from inside the process. Note that trying to
  isolate presentation back-pressure by skipping the `Present` call does not work: the back buffer index never
  advances without a present and the application stops.

## Present Rate Drops After Switching Rendering Device

On hybrid-GPU systems, after the rendering device has been switched at runtime with `CTRL+X` and another
physical device has presented to the application window, the frame rate on the original device with V-Sync off
drops to exactly **twice the display refresh rate** (240 FPS on a 120 Hz display), down from ~3900 FPS before
the switch.

This is a driver-side demotion of the `VK_PRESENT_MODE_MAILBOX_KHR` presentation path for that window, and it
is permanent for the lifetime of the window: swap-chain re-creation, a full render context reset, surface
re-creation, window move and minimize/restore were all measured to have no effect. Selecting
`VK_PRESENT_MODE_IMMEDIATE_KHR` instead avoids the demotion (~2950 FPS), and the DirectX backend is not
affected at all (3144 -> 3120 FPS across the same device round-trip).

Methane Kit deliberately keeps preferring Mailbox over Immediate when V-Sync is disabled, because Mailbox
presents the most recent frame without tearing. The frame rate cap has no visible consequence: frames rendered
above the display refresh rate are discarded by Mailbox in any case, so the displayed image is identical. See
the note in `RenderContext::ChooseSwapPresentMode()` in
[Modules/Graphics/RHI/Vulkan/Sources/Methane/Graphics/Vulkan/RenderContext.cpp](../Modules/Graphics/RHI/Vulkan/Sources/Methane/Graphics/Vulkan/RenderContext.cpp).

A related correctness issue on the same code path - presentation silently stopping to update the window after
a device switch - is worked around by `RenderContext::PrimeSurfacePresentation()` in the same file.

## How to Reproduce These Measurements

Build the same tutorial with both backends in Release:

```console
cmake --build --preset VS2026-Win64-VK-Release --target MethaneHelloTriangle
cmake --build --preset VS2026-Win64-DX-Release --target MethaneHelloTriangle
```

The frame rate, frame time and CPU share are shown in the window title HUD. Useful command line options of the
tutorial applications:

| Option                              | Meaning                                                                                                           |
|-------------------------------------|-------------------------------------------------------------------------------------------------------------------|
| `-v,--vsync <0\|1>`                 | Vertical synchronization, off by default in tutorials                                                             |
| `-b,--frame-buffers <count>`        | Number of frame buffers in the swap-chain                                                                         |
| `-d,--device <index>`               | Render on GPU adapter by index, `-1` selects the software adapter                                                 |
| `-w,--wnd-size <W> <H>`             | Window size in pixels, or as a ratio of desktop size                                                              |
| `--print_debug_messages_to_console` | Print graphics API validation messages and other debug output to the console instead of the platform debug output |

Comparisons are only meaningful when both applications run in the foreground, on the same GPU adapter (check
the adapter name in the window title), with the same window size and the same V-Sync setting.

The per-frame breakdown in this document was produced with temporary instrumentation which is **not** part of
the code base. To reproduce it, wrap the calls of interest in a scoped timer based on
`QueryPerformanceCounter` which accumulates and reports an average every few thousand frames, and place it in
the shared `Base` implementations - `Base::CommandQueueTracking::Execute()` and
`Base::RenderContext::WaitForGpuFramePresented()` - so that both backends are measured identically, plus
around the native submit and present calls in the backend `CommandListSet` and `RenderContext`
implementations.
