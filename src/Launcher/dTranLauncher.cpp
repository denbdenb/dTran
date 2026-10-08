#include <windows.h>
#include <shobjidl.h>
#include <appmodel.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <regex>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")

static std::wstring ResolveAumid()
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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    IApplicationActivationManager* actMgr = nullptr;
    hr = CoCreateInstance(
        CLSID_ApplicationActivationManager,
        nullptr,
        CLSCTX_LOCAL_SERVER,
        IID_PPV_ARGS(&actMgr));

    if (FAILED(hr) || !actMgr)
    {
        wchar_t msg[256];
        swprintf_s(msg, L"Failed to initialize Windows ApplicationActivationManager (0x%08X).", hr);
        MessageBoxW(nullptr, msg, L"dTran Launcher Error", MB_ICONERROR | MB_OK);
        if (SUCCEEDED(hr)) CoUninitialize();
        return 1;
    }

    std::wstring aumid = ResolveAumid();
    DWORD pid = 0;
    hr = actMgr->ActivateApplication(aumid.c_str(), pCmdLine ? pCmdLine : L"", AO_NONE, &pid);
    actMgr->Release();
    CoUninitialize();

    if (FAILED(hr))
    {
        wchar_t msg[512];
        swprintf_s(msg,
            L"Failed to launch dTran (AUMID: %s)\nError: 0x%08X\n\n"
            L"The application package may not be properly registered. Please reinstall dTran using Setup.",
            aumid.c_str(), hr);
        MessageBoxW(nullptr, msg, L"dTran Launcher Error", MB_ICONERROR | MB_OK);
        return 2;
    }

    return 0;
}
