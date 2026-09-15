#include <windows.h>
#include <fstream>
#include <iomanip>

static LONG WINAPI BoomifyCrashHandler(EXCEPTION_POINTERS* info) {
    DWORD code = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode : 0;
    void* address = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionAddress : nullptr;
    std::ofstream log("boomify_crash.log", std::ios::app);
    log << "Boomify fatal exception: 0x" << std::hex << std::uppercase << code
        << " at " << address << "\n";
    log.close();
    wchar_t msg[256];
    swprintf_s(msg, L"Boomify a rencontre une erreur fatale.\nCode : 0x%08X\n\nUn fichier boomify_crash.log a ete cree a cote de l'application.", (unsigned)code);
    MessageBoxW(nullptr, msg, L"Boomify - Crash", MB_OK | MB_ICONERROR | MB_TOPMOST);
    return EXCEPTION_EXECUTE_HANDLER;
}

struct BoomifyCrashGuard {
    BoomifyCrashGuard() { SetUnhandledExceptionFilter(BoomifyCrashHandler); }
};

static BoomifyCrashGuard g_boomifyCrashGuard;
