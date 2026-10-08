#include "pch.h"
#include "PackagedAppLauncher.h"
#include <shobjidl.h>
#include <appmodel.h>
#include <vector>
#include <filesystem>
#include <fstream>
#include <regex>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")

namespace dTranslate::Windows
{
    std::wstring PackagedAppLauncher::ResolveAumid()
    {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::filesystem::path dir = std::filesystem::path(exePath).parent_path();
        std::filesystem::path manifestPath = dir / L"AppxManifest.xml";

        std::wstring pkgName = L"dTranslate";
        std::wstring pkgPublisher = L"CN=dTranslate Dev";
        std::wstring appId = L"App";

        if (std::filesystem::exists(manifestPath))
        {
            try
            {
                std::ifstream f(manifestPath, std::ios::binary);
                if (f.is_open())
                {
                    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                    std::regex idRegex("<Identity\\s+Name=\"([^\"]+)\"\\s+Publisher=\"([^\"]+)\"");
                    std::smatch idMatch;
                    if (std::regex_search(content, idMatch, idRegex) && idMatch.size() >= 3)
                    {
                        std::string n = idMatch[1].str();
                        std::string p = idMatch[2].str();
                        pkgName.assign(n.begin(), n.end());
                        pkgPublisher.assign(p.begin(), p.end());
                    }

                    std::regex appRegex("<Application\\s+Id=\"([^\"]+)\"");
                    std::smatch appMatch;
                    if (std::regex_search(content, appMatch, appRegex) && appMatch.size() >= 2)
                    {
                        std::string a = appMatch[1].str();
                        appId.assign(a.begin(), a.end());
                    }
                }
            }
            catch (...) {}
        }

        PACKAGE_ID pkgId = {};
        pkgId.name = const_cast<PWSTR>(pkgName.c_str());
        pkgId.publisher = const_cast<PWSTR>(pkgPublisher.c_str());

        UINT32 count = 0;
        PackageFamilyNameFromId(&pkgId, &count, nullptr);
        if (count > 0)
        {
            std::vector<wchar_t> familyBuffer(count);
            if (PackageFamilyNameFromId(&pkgId, &count, familyBuffer.data()) == ERROR_SUCCESS)
            {
                std::wstring familyName(familyBuffer.data());
                return familyName + L"!" + appId;
            }
        }

        return L"dTranslate_4evqteexctg80!" + appId;
    }

    HRESULT PackagedAppLauncher::ActivateApp(const wchar_t* arguments, DWORD* outProcessId)
    {
        HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        IApplicationActivationManager* actMgr = nullptr;
        HRESULT hr = CoCreateInstance(
            CLSID_ApplicationActivationManager,
            nullptr,
            CLSCTX_LOCAL_SERVER,
            IID_PPV_ARGS(&actMgr));

        if (SUCCEEDED(hr) && actMgr)
        {
            std::wstring aumid = ResolveAumid();
            DWORD pid = 0;
            hr = actMgr->ActivateApplication(aumid.c_str(), arguments ? arguments : L"", AO_NONE, &pid);
            if (SUCCEEDED(hr) && outProcessId)
            {
                *outProcessId = pid;
            }
            actMgr->Release();
        }

        if (SUCCEEDED(hrCo))
        {
            CoUninitialize();
        }

        return hr;
    }
}
