
/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/Metal/Device.cpp
DirectX 12 implementation of the device interface.

******************************************************************************/

#include <Methane/Graphics/DirectX/Device.h>
#include <Methane/Graphics/DirectX/RenderContext.h>
#include <Methane/Graphics/DirectX/ComputeContext.h>
#include <Methane/Graphics/DirectX/ErrorHandling.h>

#include <Methane/Platform/Windows/Utils.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#ifdef _DEBUG
#include <dxgidebug.h>
#include <directx/d3d12sdklayers.h>
#include <sstream>

// Uncomment to enable debugger breakpoint on DirectX debug warning or error
// #define BREAK_ON_DIRECTX_DEBUG_LAYER_MESSAGE_ENABLED
#endif

#include <nowide/convert.hpp>
#include <array>
#include <algorithm>
#include <cassert>

namespace Methane::Graphics::DirectX
{

static std::string GetAdapterNameDxgi(IDXGIAdapter& adapter)
{
    META_FUNCTION_TASK();
    DXGI_ADAPTER_DESC desc{};
    adapter.GetDesc(&desc);
    return nowide::narrow(desc.Description);
}

bool IsSoftwareAdapterDxgi(IDXGIAdapter1& adapter)
{
    META_FUNCTION_TASK();
    DXGI_ADAPTER_DESC1 desc{};
    adapter.GetDesc1(&desc);
    return desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
}

#ifdef _DEBUG

static std::string_view GetMessageSeverityName(D3D12_MESSAGE_SEVERITY message_severity)
{
    META_FUNCTION_TASK();
    switch (message_severity) // NOSONAR - do not use magic_enum, because message severity names are formatted for logging
    {
    case D3D12_MESSAGE_SEVERITY_CORRUPTION: return "Corruption";
    case D3D12_MESSAGE_SEVERITY_ERROR:      return "Error";
    case D3D12_MESSAGE_SEVERITY_WARNING:    return "Warning";
    case D3D12_MESSAGE_SEVERITY_INFO:       return "Info";
    case D3D12_MESSAGE_SEVERITY_MESSAGE:    return "Message";
    default:                                return "Unknown";
    }
}

static std::string_view GetMessageCategoryName(D3D12_MESSAGE_CATEGORY message_category)
{
    META_FUNCTION_TASK();
    switch (message_category) // NOSONAR - do not use magic_enum, because message category names are formatted for logging
    {
    case D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED:   return "ApplicationDefined";
    case D3D12_MESSAGE_CATEGORY_MISCELLANEOUS:         return "Miscellaneous";
    case D3D12_MESSAGE_CATEGORY_INITIALIZATION:        return "Initialization";
    case D3D12_MESSAGE_CATEGORY_CLEANUP:               return "Cleanup";
    case D3D12_MESSAGE_CATEGORY_COMPILATION:           return "Compilation";
    case D3D12_MESSAGE_CATEGORY_STATE_CREATION:        return "StateCreation";
    case D3D12_MESSAGE_CATEGORY_STATE_SETTING:         return "StateSetting";
    case D3D12_MESSAGE_CATEGORY_STATE_GETTING:         return "StateGetting";
    case D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION: return "ResourceManipulation";
    case D3D12_MESSAGE_CATEGORY_EXECUTION:             return "Execution";
    case D3D12_MESSAGE_CATEGORY_SHADER:                return "Shader";
    default:                                           return "Unknown";
    }
}

// Debug layer message callback, which prints DirectX 12 validation messages to the platform debug output,
// similar to the Vulkan debug utils messenger callback in Methane::Graphics::Vulkan::System
static void __stdcall DebugMessageCallback(D3D12_MESSAGE_CATEGORY message_category,
                                           D3D12_MESSAGE_SEVERITY message_severity,
                                           D3D12_MESSAGE_ID message_id,
                                           LPCSTR message_description,
                                           void* /*context_ptr*/) // NOSONAR
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    ss << GetMessageSeverityName(message_severity) << " "
       << GetMessageCategoryName(message_category) << ":" << std::endl;
    ss << "\t- messageIdNumber: " << static_cast<int32_t>(message_id) << std::endl;
    ss << "\t- message:         " << (message_description ? message_description : "") << std::endl;

    Methane::Platform::PrintToDebugOutput(ss.str());
}

static DWORD ConfigureDeviceDebugFeature(const wrl::ComPtr<ID3D12Device>& device_cptr)
{
    META_FUNCTION_TASK();
    wrl::ComPtr<ID3D12InfoQueue> device_info_queue_cptr;
    if (!SUCCEEDED(device_cptr->QueryInterface(IID_PPV_ARGS(&device_info_queue_cptr))))
        return 0U;

#ifdef BREAK_ON_DIRECTX_DEBUG_LAYER_MESSAGE_ENABLED
    device_info_queue_cptr->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    device_info_queue_cptr->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
#endif

    std::array<D3D12_MESSAGE_ID, 0> skip_message_ids = {{ }};
    std::array<D3D12_MESSAGE_SEVERITY, 1> skip_message_severities = {{
        D3D12_MESSAGE_SEVERITY_INFO,
    }};

    D3D12_INFO_QUEUE_FILTER filter {};
    filter.DenyList.NumSeverities = static_cast<UINT>(skip_message_severities.size());
    filter.DenyList.pSeverityList = skip_message_severities.data();
    filter.DenyList.NumIDs  = static_cast<UINT>(skip_message_ids.size());
    filter.DenyList.pIDList = skip_message_ids.data();
    device_info_queue_cptr->AddStorageFilterEntries(&filter);

    // ID3D12InfoQueue1 with message callback support is available since Windows 10 version 2004 (build 19041)
    wrl::ComPtr<ID3D12InfoQueue1> device_info_queue_1_cptr;
    if (!SUCCEEDED(device_info_queue_cptr.As(&device_info_queue_1_cptr)))
    {
        META_LOG("WARNING: DirectX 12 debug messages can not be printed to debug output, " \
                 "because ID3D12InfoQueue1 interface is not supported by the system.");
        return 0U;
    }

    DWORD message_callback_cookie = 0U;
    if (!SUCCEEDED(device_info_queue_1_cptr->RegisterMessageCallback(&DebugMessageCallback,
                                                                    D3D12_MESSAGE_CALLBACK_FLAG_NONE,
                                                                    nullptr, &message_callback_cookie)))
    {
        META_LOG("WARNING: Failed to register DirectX 12 debug layer message callback.");
        return 0U;
    }

    return message_callback_cookie;
}

static void ReleaseDeviceDebugFeature(const wrl::ComPtr<ID3D12Device>& device_cptr, DWORD message_callback_cookie)
{
    META_FUNCTION_TASK();
    if (!device_cptr || !message_callback_cookie)
        return;

    wrl::ComPtr<ID3D12InfoQueue1> device_info_queue_1_cptr;
    if (SUCCEEDED(device_cptr->QueryInterface(IID_PPV_ARGS(&device_info_queue_1_cptr))))
    {
        device_info_queue_1_cptr->UnregisterMessageCallback(message_callback_cookie);
    }
}

#endif

Rhi::DeviceFeatureMask Device::GetSupportedFeatures(const wrl::ComPtr<IDXGIAdapter>& /*adapter_cptr*/, D3D_FEATURE_LEVEL /*feature_level*/)
{
    META_FUNCTION_TASK();
    Rhi::DeviceFeatureMask supported_features;
    // TODO: implement adapter features detection for DirectX
    supported_features.SetBitOn(Rhi::DeviceFeature::PresentToWindow);
    supported_features.SetBitOn(Rhi::DeviceFeature::AnisotropicFiltering);
    supported_features.SetBitOn(Rhi::DeviceFeature::ImageCubeArray);
    return supported_features;
}

Device::Device(const wrl::ComPtr<IDXGIAdapter>& adapter_cptr, D3D_FEATURE_LEVEL feature_level, const Capabilities& capabilities)
    : Base::Device(GetAdapterNameDxgi(*adapter_cptr.Get()),
                   IsSoftwareAdapterDxgi(static_cast<IDXGIAdapter1&>(*adapter_cptr.Get())),
                   capabilities)
    , m_adapter_cptr(adapter_cptr)
    , m_feature_level(feature_level)
{ }

Ptr<Rhi::IRenderContext> Device::CreateRenderContext(const Platform::AppEnvironment& env, tf::Executor& parallel_executor, const Rhi::RenderContextSettings& settings)
{
    META_FUNCTION_TASK();
    auto render_context_ptr = std::make_shared<RenderContext>(env, *this, parallel_executor, settings);
    render_context_ptr->Initialize(*this, true);
    return render_context_ptr;
}

Ptr<Rhi::IComputeContext> Device::CreateComputeContext(tf::Executor& parallel_executor, const Rhi::ComputeContextSettings& settings)
{
    META_FUNCTION_TASK();
    const auto compute_context_ptr = std::make_shared<ComputeContext>(*this, parallel_executor, settings);
    compute_context_ptr->Initialize(*this, true);
    return compute_context_ptr;
}

bool Device::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Base::Device::SetName(name))
        return false;

    if (m_device_cptr)
    {
        m_device_cptr->SetName(nowide::widen(name).c_str());
    }
    return true;
}

const wrl::ComPtr<ID3D12Device>& Device::GetNativeDevice() const
{
    META_FUNCTION_TASK();
    if (m_device_cptr)
        return m_device_cptr;

    ThrowIfFailed(D3D12CreateDevice(m_adapter_cptr.Get(), m_feature_level, IID_PPV_ARGS(&m_device_cptr)));
    if (!GetName().empty())
    {
        m_device_cptr->SetName(nowide::widen(GetName()).c_str());
    }

    if (D3D12_FEATURE_DATA_D3D12_OPTIONS5 feature_options_5{};
        m_device_cptr->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feature_options_5, sizeof(feature_options_5)) == S_OK)
    {
        m_feature_options_5 = feature_options_5;
    }

#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    if (Platform::Windows::IsDeveloperModeEnabled())
    {
        ThrowIfFailed(m_device_cptr->SetStablePowerState(TRUE), m_device_cptr.Get());
    }
    else
    {
        assert(false);
        META_LOG("WARNING: GPU instrumentation results may be unreliable because we failed to switch GPU to stable power state." \
                 "Enable Windows Developer Mode and try again.");
    }
#endif

#ifdef _DEBUG
    m_debug_message_callback_cookie = ConfigureDeviceDebugFeature(m_device_cptr);
#endif

    return m_device_cptr;
}

void Device::ReleaseNativeDevice()
{
    META_FUNCTION_TASK();
#ifdef _DEBUG
    ReleaseDeviceDebugFeature(m_device_cptr, m_debug_message_callback_cookie);
    m_debug_message_callback_cookie = 0U;
#endif
    m_device_cptr.Reset();
}

} // namespace Methane::Graphics::DirectX
