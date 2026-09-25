// platform.h — Vesna 跨平台抽象层
// Windows：UTF-16 API + 注册表；POSIX（Linux/macOS）：UTF-8 原生
#ifndef VESNA_PLATFORM_H
#define VESNA_PLATFORM_H

#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include <cstdlib>

namespace vesna {

inline std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring out((size_t)n, L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], n);
    return out;
}

inline std::string wideToUtf8(const std::wstring& ws) {
    if (ws.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
    std::string out((size_t)n, '\0');
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &out[0], n, nullptr, nullptr);
    return out;
}

inline std::string exeDir() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0) return ".";
    std::wstring exe(buf, n);
    size_t pos = exe.find_last_of(L"/\\");
    return wideToUtf8(pos == std::wstring::npos ? L"." : exe.substr(0, pos));
}

inline void setConsoleUtf8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

inline int sysShell(const std::string& cmd) {
    return _wsystem(utf8ToWide(cmd).c_str());
}

inline int setEnvProc(const std::string& name, const std::string& value) {
    return _putenv_s(name.c_str(), value.c_str());
}

inline std::string getCwd() {
    wchar_t buf[MAX_PATH];
    if (_wgetcwd(buf, MAX_PATH) == nullptr) return ".";
    return wideToUtf8(buf);
}

inline int chDir(const std::string& path) {
    return _wchdir(utf8ToWide(path).c_str());
}

}  // namespace vesna

#else  // POSIX

#include <cstdlib>
#include <cstring>
#include <unistd.h>

namespace vesna {

inline std::wstring utf8ToWide(const std::string& s) {
    std::wstring out;
    out.reserve(s.size());
    for (unsigned char c : s) out.push_back((wchar_t)c);
    return out;
}

inline std::string wideToUtf8(const std::wstring& ws) {
    std::string out;
    out.reserve(ws.size());
    for (wchar_t c : ws) out.push_back((char)c);
    return out;
}

inline std::string exeDir() { return ""; }

inline void setConsoleUtf8() {}

inline int sysShell(const std::string& cmd) {
    return std::system(cmd.c_str());
}

inline int setEnvProc(const std::string& name, const std::string& value) {
    return setenv(name.c_str(), value.c_str(), 1);
}

inline std::string getCwd() {
    char buf[4096];
    if (getcwd(buf, sizeof(buf)) == nullptr) return ".";
    return std::string(buf);
}

inline int chDir(const std::string& path) {
    return chdir(path.c_str());
}

}  // namespace vesna

#endif  // _WIN32

#endif  // VESNA_PLATFORM_H
