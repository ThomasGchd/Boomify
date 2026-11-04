// Boomify in-app portable updater (Windows, no installer).
// This file is included by main_v17_nav.cpp; it is not a separate entry point.
#include <urlmon.h>
#include <wincrypt.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

#ifndef BOOMIFY_BUILD_SHA
#define BOOMIFY_BUILD_SHA "local"
#endif

namespace {
constexpr const wchar_t* kUpdateBaseNav =
    L"https://github.com/ThomasGchd/Boomify/releases/download/alpha-rolling/";

std::wstring executablePathNav() {
    std::vector<wchar_t> buffer(32768, L'\0');
    DWORD n = GetModuleFileNameW(nullptr, buffer.data(), (DWORD)buffer.size());
    return (n && n < buffer.size()) ? std::wstring(buffer.data(), n) : L"";
}

std::wstring widenAsciiNav(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

bool isHexNav(const std::string& s, size_t length) {
    return s.size() == length &&
        std::all_of(s.begin(), s.end(), [](unsigned char ch) { return std::isxdigit(ch) != 0; });
}

bool downloadUpdateNav(const wchar_t* asset, const std::wstring& destination) {
    DeleteFileW(destination.c_str());
    std::wstring url = std::wstring(kUpdateBaseNav) + asset;
    HRESULT result = URLDownloadToFileW(nullptr, url.c_str(), destination.c_str(), 0, nullptr);
    if (FAILED(result)) {
        DeleteFileW(destination.c_str());
        return false;
    }
    return true;
}

bool fileSHA256Nav(const std::wstring& path, std::string& digest) {
    digest.clear();
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    bool ok = CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) != 0;
    if (ok) ok = CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash) != 0;
    std::array<BYTE, 32768> data{};
    while (ok) {
        DWORD count = 0;
        if (!ReadFile(file, data.data(), (DWORD)data.size(), &count, nullptr)) {
            ok = false;
            break;
        }
        if (count == 0) break;
        ok = CryptHashData(hash, data.data(), count, 0) != 0;
    }
    if (ok) {
        std::array<BYTE, 32> bytes{};
        DWORD size = (DWORD)bytes.size();
        ok = CryptGetHashParam(hash, HP_HASHVAL, bytes.data(), &size, 0) != 0 &&
             size == bytes.size();
        if (ok) {
            constexpr char hex[] = "0123456789abcdef";
            for (BYTE b : bytes) {
                digest.push_back(hex[b >> 4]);
                digest.push_back(hex[b & 15]);
            }
        }
    }
    if (hash) CryptDestroyHash(hash);
    if (provider) CryptReleaseContext(provider, 0);
    CloseHandle(file);
    return ok;
}

// The new executable can run as a short-lived helper after the old process exits.
// This avoids trying to overwrite a running Windows .exe.
int applyDownloadedUpdateNav(const std::wstring& target, DWORD parentPid) {
    HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, parentPid);
    if (parent) {
        WaitForSingleObject(parent, 20000);
        CloseHandle(parent);
    } else {
        Sleep(1500);
    }
    std::wstring source = executablePathNav();
    if (source.empty() || target.empty() || source == target) return 2;
    bool copied = false;
    for (int attempt = 0; attempt < 60; ++attempt) {
        if (CopyFileW(source.c_str(), target.c_str(), FALSE)) {
            copied = true;
            break;
        }
        Sleep(350);
    }
    if (!copied) {
        MessageBoxW(nullptr,
            L"Impossible de remplacer Boomify.exe. Verifiez les droits d'ecriture dans son dossier.\n"
            L"Le fichier telecharge reste disponible a cote de l'ancien.",
            L"Boomify - Mise a jour", MB_OK | MB_ICONERROR);
        return 3;
    }
    if ((INT_PTR)ShellExecuteW(nullptr, L"open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL) <= 32) {
        MessageBoxW(nullptr,
            L"Boomify a ete mis a jour, mais le redemarrage automatique a echoue.\n"
            L"Relancez simplement Boomify.exe.",
            L"Boomify - Mise a jour", MB_OK | MB_ICONWARNING);
        return 4;
    }
    return 0;
}

void cleanupDownloadedUpdateNav() {
    std::string sha = BOOMIFY_BUILD_SHA;
    if (sha.size() < 8 || !isHexNav(sha, 40)) return;
    std::wstring target = executablePathNav();
    if (target.empty()) return;
    std::wstring staged = target + L".update." + widenAsciiNav(sha.substr(0, 8)) + L".exe";
    if (GetFileAttributesW(staged.c_str()) == INVALID_FILE_ATTRIBUTES) return;
    std::thread([staged]() {
        for (int i = 0; i < 40; ++i) {
            Sleep(400);
            if (DeleteFileW(staged.c_str())) return;
            if (GetFileAttributesW(staged.c_str()) == INVALID_FILE_ATTRIBUTES) return;
        }
    }).detach();
}

void checkAndUpdateBoomifyNav(HWND owner) {
    std::wstring target = executablePathNav();
    if (target.empty()) {
        MessageBoxW(owner, L"Emplacement de Boomify.exe introuvable.",
                    L"Boomify - Mise a jour", MB_OK | MB_ICONERROR);
        return;
    }
    wchar_t tempPath[MAX_PATH + 1]{};
    DWORD n = GetTempPathW(MAX_PATH, tempPath);
    if (!n || n >= MAX_PATH) {
        MessageBoxW(owner, L"Dossier temporaire inaccessible.",
                    L"Boomify - Mise a jour", MB_OK | MB_ICONERROR);
        return;
    }
    std::wstring manifestPath = std::wstring(tempPath) + L"Boomify-version-check.txt";
    if (!downloadUpdateNav(L"version.txt", manifestPath)) {
        MessageBoxW(owner,
            L"Impossible de joindre le canal de mises a jour de Boomify.\n"
            L"Verifiez la connexion ou le pare-feu du PC.",
            L"Boomify - Mise a jour", MB_OK | MB_ICONWARNING);
        return;
    }
    std::string remoteSha, remoteHash, remoteHostHash;
    {
        std::ifstream in{std::filesystem::path(manifestPath)};
        in >> remoteSha >> remoteHash >> remoteHostHash;
    }
    DeleteFileW(manifestPath.c_str());
    if (!isHexNav(remoteSha, 40) || !isHexNav(remoteHash, 64)) {
        MessageBoxW(owner, L"Manifest de mise a jour invalide.",
                    L"Boomify - Mise a jour", MB_OK | MB_ICONERROR);
        return;
    }
    if (remoteSha == std::string(BOOMIFY_BUILD_SHA)) {
        // Older updaters only installed Boomify.exe. Recover the optional VST3
        // renderer on the next update check without replacing the main app.
        if(isHexNav(remoteHostHash,64)){
            std::filesystem::path helper=std::filesystem::path(target).parent_path()/L"BoomifyVST3Host.exe";
            std::string localHostHash;
            if(!fileSHA256Nav(helper.wstring(),localHostHash)||localHostHash!=remoteHostHash){
                std::wstring stage=target+L".vst3host.download";
                std::string got;
                if(downloadUpdateNav(L"BoomifyVST3Host.exe",stage)&&
                   fileSHA256Nav(stage,got)&&got==remoteHostHash&&
                   MoveFileExW(stage.c_str(),helper.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
                    MessageBoxW(owner,L"Moteur VST3 installe. Boomify est a jour.",
                                L"Boomify - Mise a jour",MB_OK|MB_ICONINFORMATION);
                    return;
                }
                DeleteFileW(stage.c_str());
                MessageBoxW(owner,L"Boomify est a jour, mais le moteur VST3 n'a pas pu etre installe.",
                             L"Boomify - Mise a jour",MB_OK|MB_ICONWARNING);
                return;
            }
        }
        MessageBoxW(owner, L"Boomify est deja a jour.",
                    L"Boomify - Mise a jour", MB_OK | MB_ICONINFORMATION);
        return;
    }
    std::wstring question =
        L"Nouvelle build disponible : " + widenAsciiNav(remoteSha.substr(0, 8)) +
        L".\n\nEnregistrez votre projet .boom avant de continuer.\n"
        L"Boomify va telecharger ses executables, puis se fermer et redemarrer.\n"
        L"Vos projets et samples ne seront pas effaces.\n\n"
        L"Installer la nouvelle version ?";
    if (MessageBoxW(owner, question.c_str(), L"Boomify - Mise a jour",
                    MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;

    std::wstring staged = target + L".update." + widenAsciiNav(remoteSha.substr(0, 8)) + L".exe";
    if (!downloadUpdateNav(L"Boomify.exe", staged)) {
        MessageBoxW(owner, L"Telechargement impossible. Controlez la connexion et les permissions du dossier.",
                    L"Boomify - Mise a jour", MB_OK | MB_ICONERROR);
        return;
    }
    std::string downloadedHash;
    if (!fileSHA256Nav(staged, downloadedHash) || downloadedHash != remoteHash) {
        DeleteFileW(staged.c_str());
        MessageBoxW(owner, L"Le fichier telecharge n'a pas la bonne empreinte SHA-256.\n"
                           L"Mise a jour annulee sans modifier Boomify.",
                    L"Boomify - Mise a jour", MB_OK | MB_ICONERROR);
        return;
    }
    // The third manifest hash authenticates the companion VST3 renderer.
    if (isHexNav(remoteHostHash, 64)) {
        std::filesystem::path hostPath=std::filesystem::path(target).parent_path()/L"BoomifyVST3Host.exe";
        std::wstring stagedHost=target+L".vst3host.download";
        if (!downloadUpdateNav(L"BoomifyVST3Host.exe", stagedHost)) {
            DeleteFileW(staged.c_str());
            MessageBoxW(owner,L"Impossible de telecharger le moteur VST3. Mise a jour annulee.",
                         L"Boomify - Mise a jour",MB_OK|MB_ICONERROR);
            return;
        }
        std::string actualHostHash;
        if (!fileSHA256Nav(stagedHost,actualHostHash)||actualHostHash!=remoteHostHash||
            !MoveFileExW(stagedHost.c_str(),hostPath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
            DeleteFileW(stagedHost.c_str());DeleteFileW(staged.c_str());
            MessageBoxW(owner,L"Le moteur VST3 n'a pas pu etre verifie ou installe. Mise a jour annulee.",
                         L"Boomify - Mise a jour",MB_OK|MB_ICONERROR);
            return;
        }
    }
    std::wstring command = L"\"" + staged + L"\" --boomify-apply \"" + target +
                           L"\" " + std::to_wstring(GetCurrentProcessId());
    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(L'\0');
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(staged.c_str(), mutableCommand.data(), nullptr, nullptr, FALSE,
                        0, nullptr, nullptr, &startup, &process)) {
        DeleteFileW(staged.c_str());
        MessageBoxW(owner, L"Le programme de remplacement n'a pas pu etre lance.",
                    L"Boomify - Mise a jour", MB_OK | MB_ICONERROR);
        return;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    stopAudio();
    PostMessageW(owner, WM_CLOSE, 0, 0);
}

bool isUpdateHelperNav() {
    int argc = 0;
    LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!args) return false;
    bool helper = argc == 4 && wcscmp(args[1], L"--boomify-apply") == 0;
    LocalFree(args);
    return helper;
}

int runUpdateHelperNav() {
    int argc = 0;
    LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!args || argc != 4) {
        if (args) LocalFree(args);
        return 2;
    }
    std::wstring target = args[2];
    DWORD pid = (DWORD)wcstoul(args[3], nullptr, 10);
    LocalFree(args);
    return applyDownloadedUpdateNav(target, pid);
}
} // namespace
