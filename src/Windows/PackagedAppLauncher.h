#pragma once

#include <windows.h>
#include <string>

namespace dTranslate::Windows
{
    class PackagedAppLauncher
    {
    public:
        // Dynamically resolves the AUMID of the installed package from AppxManifest.xml or Package Family Name
        static std::wstring ResolveAumid();

        // Activates the application via IApplicationActivationManager::ActivateApplication
        static HRESULT ActivateApp(const wchar_t* arguments = nullptr, DWORD* outProcessId = nullptr);
    };
}
