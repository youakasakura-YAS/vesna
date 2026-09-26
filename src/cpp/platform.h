// platform.h — Vesna 跨平台抽象层
// Windows：UTF-16 API + 注册表；POSIX（Linux/macOS）：UTF-8 原生
#ifndef VESNA_PLATFORM_H
#define VESNA_PLATFORM_H

#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>
#include <direct.h>
#include <cstdlib>

namespace vesna {

// UUID v4 随机源（advapi32 SystemFunction036）
extern "C" BOOLEAN NTAPI SystemFunction036(PVOID, ULONG);
#define RtlGenRandom SystemFunction036

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

// ---- 网络（依赖系统 curl；TCP 探测用 Winsock） ----
inline std::string httpRequest(const std::string& url, const std::string& data, bool post) {
    std::string cmd;
    if (post) {
        cmd = "curl -s -m 30 -d \"" + data + "\" \"" + url + "\"";
    } else {
        cmd = "curl -s -m 30 \"" + url + "\"";
    }
    std::string out;
    FILE* p = _popen(cmd.c_str(), "r");
    if (!p) return "";
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), p)) > 0) out.append(buf, n);
    _pclose(p);
    return out;
}

// ---- FFI（加载系统库与符号） ----
inline void* ffiLoad(const std::string& dll) {
    HMODULE h = LoadLibraryW(utf8ToWide(dll).c_str());
    return h ? (void*)h : nullptr;
}
inline void* ffiSym(void* handle, const std::string& name) {
    if (!handle) return nullptr;
    FARPROC p = GetProcAddress((HMODULE)handle, name.c_str());
    return p ? (void*)p : nullptr;
}

inline int tcpPing(const std::string& host, int port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) { WSACleanup(); return -1; }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    addr.sin_addr.s_addr = inet_addr(host.c_str());
    int r = -1;
    if (addr.sin_addr.s_addr != INADDR_NONE) {
        r = connect(s, (struct sockaddr*)&addr, sizeof(addr));
    } else {
        struct hostent* he = gethostbyname(host.c_str());
        if (he && he->h_addr_list && he->h_addr_list[0]) {
            addr.sin_addr.s_addr = *(unsigned long*)he->h_addr_list[0];
            r = connect(s, (struct sockaddr*)&addr, sizeof(addr));
        }
    }
    closesocket(s);
    WSACleanup();
    return r == 0 ? 0 : 1;
}

}  // namespace vesna

#else  // POSIX

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dlfcn.h>

namespace vesna {

// UUID v4 随机源（advapi32 SystemFunction036）
extern "C" BOOLEAN NTAPI SystemFunction036(PVOID, ULONG);
#define RtlGenRandom SystemFunction036

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

// ---- 网络（依赖系统 curl；TCP 探测用 BSD socket） ----
inline std::string httpRequest(const std::string& url, const std::string& data, bool post) {
    std::string cmd;
    if (post) {
        cmd = "curl -s -m 30 -d \"" + data + "\" \"" + url + "\"";
    } else {
        cmd = "curl -s -m 30 \"" + url + "\"";
    }
    std::string out;
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return "";
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), p)) > 0) out.append(buf, n);
    pclose(p);
    return out;
}

// ---- FFI（加载系统库与符号） ----
inline void* ffiLoad(const std::string& dll) {
    void* h = dlopen(dll.c_str(), RTLD_NOW | RTLD_GLOBAL);
    return h;
}
inline void* ffiSym(void* handle, const std::string& name) {
    if (!handle) return nullptr;
    return dlsym(handle, name.c_str());
}

inline int tcpPing(const std::string& host, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = inet_addr(host.c_str());
    int r = -1;
    if (addr.sin_addr.s_addr != INADDR_NONE) {
        r = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    } else {
        struct hostent* he = gethostbyname(host.c_str());
        if (he && he->h_addr_list && he->h_addr_list[0]) {
            addr.sin_addr.s_addr = *(unsigned long*)he->h_addr_list[0];
            r = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
        }
    }
    close(fd);
    return r == 0 ? 0 : 1;
}

}  // namespace vesna

#endif  // _WIN32

#endif  // VESNA_PLATFORM_H
