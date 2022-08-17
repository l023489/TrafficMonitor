﻿#include "stdafx.h"
#include <algorithm>
#include <memory>
#include <tuple>
#include "D3D10Support1.h"
#include "Common.h"
#include "Nullable.hpp"

#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "D3D10_1.lib")

using Microsoft::WRL::ComPtr;

void CD3D10Device1::Data::CResourceTracker::AddResource(CDX10DeviceResource1* p_resource)
{
    auto existed_it = m_resource_pointer_set.find(p_resource);
    if (existed_it == m_resource_pointer_set.end())
    {
        m_resource_pointer_set.emplace(p_resource);
    }
}

void CD3D10Device1::Data::CResourceTracker::RemoveResource(CDX10DeviceResource1* p_resource)
{
    auto existed_it = m_resource_pointer_set.find(p_resource);
    if (existed_it != m_resource_pointer_set.end())
    {
        m_resource_pointer_set.erase(existed_it);
    }
}

auto CD3D10Device1::Data::CResourceTracker::GetTrackerSet() const noexcept
    -> const std::unordered_set<class CDX10DeviceResource1*>&
{
    return m_resource_pointer_set;
}

void CD3D10Device1::Recreate(Microsoft::WRL::ComPtr<IDXGIAdapter1> p_adapter1)
{
    m_sp_data->m_p_adapter1 = p_adapter1;
    ThrowIfFailed(
        D3D10CreateDevice1(
            m_sp_data->m_p_adapter1.Get(),
            m_sp_data->m_driver_type,
            m_sp_data->m_h_software,
            m_sp_data->m_flags,
            m_sp_data->m_feature_level,
            m_sp_data->m_sdk_version,
            &m_sp_data->m_p_device1),
        "Create D3D10Device1 failed.");

    auto ref_track_set = m_sp_data->m_resource_tracker.GetTrackerSet();
    for (auto&& it : ref_track_set)
    {
        it->OnDeviceRecreate(m_sp_data->m_p_device1);
    }
}

auto CD3D10Device1::Get() noexcept
    -> Microsoft::WRL::ComPtr<ID3D10Device1>
{
    return m_sp_data->m_p_device1;
}

auto CD3D10Device1::GetAdapter() noexcept
    -> Microsoft::WRL::ComPtr<IDXGIAdapter1>
{
    return m_sp_data->m_p_adapter1;
}

auto CD3D10Device1::GetDriverType() const noexcept
    -> D3D10_DRIVER_TYPE
{
    return m_sp_data->m_driver_type;
}

auto CD3D10Device1::SetDriverType(D3D10_DRIVER_TYPE driver_type) noexcept
    -> CD3D10Device1&
{
    m_sp_data->m_driver_type = driver_type;
    return *this;
}

auto CD3D10Device1::GetSoftwareHandle() const noexcept
    -> HMODULE
{
    return m_sp_data->m_h_software;
}

auto CD3D10Device1::SetSoftwareHandle(HMODULE h_software) noexcept
    -> CD3D10Device1&
{
    m_sp_data->m_h_software = h_software;
    return *this;
}

auto CD3D10Device1::GetFlags() const noexcept
    -> UINT
{
    return m_sp_data->m_flags;
}

auto CD3D10Device1::SetFlags(UINT flags) noexcept
    -> CD3D10Device1&
{
    m_sp_data->m_flags = flags;
    return *this;
}

auto CD3D10Device1::GetFeaturelLevel() const noexcept
    -> D3D10_FEATURE_LEVEL1
{
    return m_sp_data->m_feature_level;
}

auto CD3D10Device1::SetFeaturelLevel(D3D10_FEATURE_LEVEL1 featurel_level) noexcept
    -> CD3D10Device1&
{
    m_sp_data->m_feature_level = featurel_level;
    return *this;
}

auto CD3D10Device1::GetSdkVersion() const noexcept
    -> UINT
{
    return m_sp_data->m_sdk_version;
}

auto CD3D10Device1::SetSdkVersion(UINT sdk_version) noexcept
    -> CD3D10Device1&
{
    m_sp_data->m_sdk_version = sdk_version;
    return *this;
}

auto CD3D10Device1::GetData() noexcept
    -> std::shared_ptr<const Data>
{
    return std::const_pointer_cast<const Data>(m_sp_data);
}

bool CD3D10Support1::CheckSupport()
{
    static bool result = FunctionChecker::CheckFunctionExist(_T("D3D10_1.dll"), "D3D10CreateDevice1");
    return result;
}

auto CD3D10Support1::GetDeviceList(bool force_refresh)
    -> const std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>>&
{
    static CNullable<CStaticVariableWrapper<ComPtr<IDXGIFactory1>>> p_dxgi_factory1_wrapper{};
    if (!p_dxgi_factory1_wrapper || force_refresh)
    {
        Destroy(&p_dxgi_factory1_wrapper);
        EmplaceAt(&p_dxgi_factory1_wrapper);
        p_dxgi_factory1_wrapper.Construct([](ComPtr<IDXGIFactory1>* p_content)
                                          { CreateDXGIFactory1(IID_PPV_ARGS(&*p_content)); });
    }

    using DeviceVector = std::vector<ComPtr<IDXGIAdapter1>>;
    static CNullable<CStaticVariableWrapper<DeviceVector>> result{};
    if (!result || force_refresh)
    {
        Destroy(&result);
        EmplaceAt(&result);
        result.Construct([](DeviceVector* p_content)
                         {
            auto p_dxgi_factory1 = p_dxgi_factory1_wrapper.GetUnsafe().Get();
            UINT i = 0;
            do
            {
                ComPtr<IDXGIAdapter1> p_current_adapter1{};
                auto hr = p_dxgi_factory1->EnumAdapters1(i, &p_current_adapter1);
                if (hr == S_OK)
                {
                    ++i;
                    p_content->emplace_back(std::move(p_current_adapter1));
                    continue;
                }
                else if (hr == DXGI_ERROR_NOT_FOUND)
                {
                    break;
                }
                else if (FAILED(hr))
                {
                    ThrowIfFailed(hr, "EnumAdapters1 failed. Maybe ppAdapter parameter is NULL.");
                }
            } while (true); });
    }

    return result.GetUnsafe().Get();
}

CDXShaderException::CDXShaderException(HRESULT hr, const char* p_error, Microsoft::WRL::ComPtr<ID3DBlob> p_shader_error)
    : CHResultException{hr, p_error}, m_error{p_error}
{
    if (p_shader_error != nullptr)
    {
        m_error += static_cast<char*>(p_shader_error->GetBufferPointer());
    }
}

const char* CDXShaderException::what() const noexcept
{
    return m_error.c_str();
}

auto CShader::Compile() const
    -> Microsoft::WRL::ComPtr<ID3DBlob>
{
    if (m_is_macro_changed && !m_macros.empty())
    {
        m_shader_macros.resize(m_macros.size() + 1);
        std::transform(m_macros.begin(), m_macros.end(), m_shader_macros.begin(),
                       [](const ShaderMacro& current) -> D3D_SHADER_MACRO
                       {
                           return {current.m_name.c_str(), current.m_definition.c_str()};
                       });
        m_shader_macros.back() = {NULL, NULL};
    }
    m_is_macro_changed = false;

    if (m_is_config_changed)
    {
        ComPtr<ID3DBlob> p_error_message{};
        ThrowIfFailed<CDXShaderException>(
            D3DCompile(
                m_code.c_str(),
                m_code.size(),
                m_name.c_str(),
                m_macros.empty() ? NULL : m_shader_macros.data(),
                m_p_include == nullptr ? D3D_COMPILE_STANDARD_FILE_INCLUDE : m_p_include,
                m_entry_point.c_str(),
                m_target.c_str(),
                m_flags1,
                m_flags2,
                &m_p_cached_byte_code,
                &p_error_message),
            "Compile DX shader failed.",
            p_error_message);
        m_is_config_changed = false;
    }

    return m_p_cached_byte_code;
}

auto CShader::GetCode() const noexcept
    -> const std::string&
{
    return m_code;
}

auto CShader::SetCode(const std::string& code)
    -> CShader&
{
    m_is_config_changed = true;
    m_code = code;
    return *this;
}

auto CShader::GetEntryPoint() const noexcept
    -> const std::string&
{
    return m_entry_point;
}

auto CShader::SetEntryPoint(const std::string& entry_point)
    -> CShader&
{
    m_is_config_changed = true;
    m_entry_point = entry_point;
    return *this;
}

auto CShader::GetName() const noexcept
    -> const std::string&
{
    return m_name;
}

auto CShader::SetName(const std::string& name)
    -> CShader&
{
    m_is_config_changed = true;
    m_name = name;
    return *this;
}

auto CShader::GetTarget() const noexcept
    -> const std::string&
{
    return m_target;
}

auto CShader::SetTarget(const std::string& target)
    -> CShader&
{
    m_is_config_changed = true;
    m_target = target;
    return *this;
}

auto CShader::AddMacro(const ShaderMacro& macro)
    -> CShader&
{
    m_is_config_changed = true;
    m_macros.push_back(std::move(macro));
    return *this;
}

auto CShader::DeleteMacro(const std::string& name)
    -> CShader&
{
    m_is_config_changed = true;
    std::ignore = std::remove_if(m_macros.begin(), m_macros.end(), [&name](const ShaderMacro& x)
                                 { return x.m_name == name; });
    return *this;
}

auto CShader::GetMacros() const noexcept
    -> const std::vector<ShaderMacro>&
{
    return m_macros;
}

UINT CShader::GetFlags1() const noexcept
{
    return m_flags1;
}

auto CShader::SetFlags1(UINT flags1) noexcept
    -> CShader&
{
    m_is_config_changed = true;
    m_flags1 = flags1;
    return *this;
}

UINT CShader::GetFlags2() const noexcept
{
    return m_flags2;
}

auto CShader::SetFlags2(UINT flags2) noexcept
    -> CShader&
{
    m_is_config_changed = true;
    m_flags2 = flags2;
    return *this;
}

auto CShader::GetInclude() noexcept
    -> ID3DInclude*
{
    m_is_config_changed = true;
    return m_p_include;
}

auto CShader::SetInclude(ID3DInclude* p_indclude) noexcept
    -> CShader&
{
    m_is_config_changed = true;
    m_p_include = p_indclude;
    return *this;
}

CD3D10DrawCallWaiter::CD3D10DrawCallWaiter(Microsoft::WRL::ComPtr<ID3D10Device1> p_device)
{
    D3D10_QUERY_DESC query_desc{};
    query_desc.Query = D3D10_QUERY_EVENT;
    ThrowIfFailed<CD3D10Exception1>(
        p_device->CreateQuery(&query_desc, &m_p_query),
        "Create ID3D10Query failed.");
    m_p_query->End();
}

HRESULT CD3D10DrawCallWaiter::Wait() const noexcept
{
    HRESULT result;
    do
    {
        result = m_p_query->GetData(NULL, 0, 0);
    } while (result == S_FALSE);

    return result;
}

void CDX10DeviceResource1::StartTrack()
{
    auto sp_data = m_wp_data.lock();
    if (sp_data)
    {
        sp_data->m_resource_tracker.AddResource(this);
    }
}

void CDX10DeviceResource1::EndTrack()
{
    auto sp_data = m_wp_data.lock();
    if (sp_data)
    {
        sp_data->m_resource_tracker.RemoveResource(this);
    }
}

void CDX10DeviceResource1::OnSelfMove(
    CD3D10Device1::Data::CResourceTracker& ref_resource_tracker,
    CDX10DeviceResource1* from, CDX10DeviceResource1* to)
{
    if (from != to)
    {
        ref_resource_tracker.RemoveResource(from);
        ref_resource_tracker.AddResource(to);
    }
}

CDX10DeviceResource1::CDX10DeviceResource1(CD3D10Device1& ref_cd3d10_device1) noexcept
    : m_wp_data{ref_cd3d10_device1.GetData()}
{
    StartTrack();
}

CDX10DeviceResource1::~CDX10DeviceResource1() noexcept
{
    EndTrack();
}

CDX10DeviceResource1::CDX10DeviceResource1(const CDX10DeviceResource1& other) noexcept
    : m_wp_data{other.m_wp_data}
{
    StartTrack();
}

CDX10DeviceResource1::CDX10DeviceResource1(CDX10DeviceResource1&& other) noexcept
    : m_wp_data{other.m_wp_data}
{
    auto from = std::addressof(other);
    auto to = this;
    auto sp_data = m_wp_data.lock();
    if(sp_data)
    {
        OnSelfMove(sp_data->m_resource_tracker, from, to);
    }
}

bool CDX10DeviceResource1::CheckDeviceValid() const noexcept
{
    return m_wp_data.expired();
}
