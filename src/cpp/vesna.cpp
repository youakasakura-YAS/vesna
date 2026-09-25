// vesna.cpp — Vesna 1.2.0 C++ 实现：词法 / 预处理 / 解析
// 从 src/vesna.py 移植，保持语言语义一致
#include "vesna.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <chrono>
#include <cctype>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <direct.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#ifdef _WIN32
#include <conio.h>
#endif
#include <iterator>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <thread>
#include <mutex>
#include "crypto.h"

#include "platform.h"

namespace vesna {

static std::string parentDir(const std::string& path);
static std::string strFloat(double f);
const std::string VERSION = "1.6.0";


// ============================================================
// 标识符 intern
// ============================================================
static std::unordered_map<std::string, int64_t> g_intern_map;
static std::vector<std::string> g_intern_names;

int64_t internId(const std::string& s) {
    auto it = g_intern_map.find(s);
    if (it != g_intern_map.end()) return it->second;
    int64_t id = (int64_t)g_intern_names.size();
    g_intern_map.emplace(s, id);
    g_intern_names.push_back(s);
    return id;
}

const std::string& internName(int64_t id) {
    if (id >= 0 && id < (int64_t)g_intern_names.size()) return g_intern_names[(size_t)id];
    static const std::string empty;
    return empty;
}

// ============================================================
// 值
// ============================================================
Value mkNone() { return Value(); }
Value mkBool(bool b) { Value v; v.v = b; return v; }
Value mkInt(int64_t i) { Value v; v.v = i; return v; }
Value mkFloat(double f) { Value v; v.v = f; return v; }
Value mkStr(std::string s) { Value v; v.v = std::move(s); return v; }
Value mkList() { Value v; v.v = std::make_shared<ListVal>(); return v; }
Value mkGroup() { Value v; v.v = std::make_shared<GroupVal>(); return v; }
Value mkDict() { Value v; v.v = std::make_shared<DictVal>(); return v; }

bool isNum(const Value& v) {
    return v.t() == Value::T::INT || v.t() == Value::T::FLOAT;
}

bool vesnaEq(const Value& a, const Value& b) {
    if (a.t() != b.t()) {
        if (isNum(a) && isNum(b)) {
            if (a.t() == Value::T::INT && b.t() == Value::T::INT) return a.i() == b.i();
            double x = (a.t() == Value::T::INT) ? (double)a.i() : a.f();
            double y = (b.t() == Value::T::INT) ? (double)b.i() : b.f();
            return x == y;
        }
        return false;
    }
    switch (a.t()) {
        case Value::T::NONE: return true;
        case Value::T::BOOL: return a.b() == b.b();
        case Value::T::INT: return a.i() == b.i();
        case Value::T::FLOAT: return a.f() == b.f();
        case Value::T::STR: return a.s() == b.s();
        case Value::T::LIST:
        case Value::T::GROUP: {
            const auto& x = (a.t() == Value::T::LIST) ? a.list()->items : a.group()->items;
            const auto& y = (b.t() == Value::T::LIST) ? b.list()->items : b.group()->items;
            if (x.size() != y.size()) return false;
            for (size_t i = 0; i < x.size(); ++i)
                if (!vesnaEq(x[i], y[i])) return false;
            return true;
        }
        case Value::T::DICT: {
            const auto& x = a.dict()->pairs;
            const auto& y = b.dict()->pairs;
            if (x.size() != y.size()) return false;
            for (const auto& p : x) {
                bool found = false;
                for (const auto& q : y)
                    if (vesnaEq(p.first, q.first) && vesnaEq(p.second, q.second)) { found = true; break; }
                if (!found) return false;
            }
            return true;
        }
    }
    return false;
}

bool truthy(const Value& v) {
    switch (v.t()) {
        case Value::T::NONE: return false;
        case Value::T::BOOL: return v.b();
        case Value::T::INT: return v.i() != 0;
        case Value::T::FLOAT: return v.f() != 0.0;
        case Value::T::STR: return !v.s().empty();
        case Value::T::LIST: return !v.list()->items.empty();
        case Value::T::GROUP: return !v.group()->items.empty();
        case Value::T::DICT: return !v.dict()->pairs.empty();
    }
    return true;
}

// float 的字符串表示：整数化输出整数；否则最短表示（同 Python str()）
static std::string floatToStr(double f) {
    if (f == (double)(int64_t)f && std::abs(f) < 1e15) {
        return std::to_string((int64_t)f);
    }
    char buf[64];
    auto res = std::to_chars(buf, buf + sizeof(buf), f);
    if (res.ec == std::errc()) return std::string(buf, res.ptr);
    return std::to_string(f);
}

std::string fmt(const Value& v) {
    switch (v.t()) {
        case Value::T::NONE: return "none";
        case Value::T::BOOL: return v.b() ? "true" : "false";
        case Value::T::INT: return "'" + std::to_string(v.i()) + "'";
        case Value::T::FLOAT: return "'" + floatToStr(v.f()) + "'";
        case Value::T::STR: return v.s();
        case Value::T::LIST: {
            std::string out = "[";
            for (size_t i = 0; i < v.list()->items.size(); ++i) {
                if (i) out += ";";
                out += fmt(v.list()->items[i]);
            }
            return out + "]";
        }
        case Value::T::GROUP: {
            std::string out = "(";
            for (size_t i = 0; i < v.group()->items.size(); ++i) {
                if (i) out += ";";
                out += fmt(v.group()->items[i]);
            }
            return out + ")";
        }
        case Value::T::DICT: {
            std::string out = "{";
            for (size_t i = 0; i < v.dict()->pairs.size(); ++i) {
                if (i) out += ",";
                out += fmt(v.dict()->pairs[i].first) + ":" + fmt(v.dict()->pairs[i].second);
            }
            return out + "}";
        }
    }
    return "unknown";
}

std::string typeName(const Value& v) {
    switch (v.t()) {
        case Value::T::NONE: return "none";
        case Value::T::BOOL: return "bool";
        case Value::T::INT: return "int";
        case Value::T::FLOAT: return "float";
        case Value::T::STR: return "str";
        case Value::T::LIST: return "list";
        case Value::T::GROUP: return "group";
        case Value::T::DICT: return "dict";
    }
    return "unknown";
}

// Dict 操作
static std::pair<Value, Value>* dictFind(DictVal& d, const Value& k) {
    for (auto& p : d.pairs)
        if (vesnaEq(p.first, k)) return &p;
    return nullptr;
}
static void dictSet(DictVal& d, const Value& k, const Value& v) {
    for (auto& p : d.pairs) {
        if (vesnaEq(p.first, k)) { p.second = v; return; }
    }
    d.pairs.emplace_back(k, v);
}

// ============================================================
// 字符串工具
// ============================================================
static std::string toUpperAscii(const std::string& s) {
    std::string out = s;
    for (auto& c : out) if (c >= 'a' && c <= 'z') c -= 32;
    return out;
}
static std::string toLowerAscii(const std::string& s) {
    std::string out = s;
    for (auto& c : out) if (c >= 'A' && c <= 'Z') c += 32;
    return out;
}

static bool isNumberStr(const std::string& s) {
    static const std::regex re(R"(-?\d+(\.\d+)?|-?\.\d+)");
    return std::regex_match(s, re);
}

static std::string unescape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char c = s[i + 1];
            switch (c) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case '\\': out += '\\'; break;
                case '"': out += '"'; break;
                case '\'': out += '\''; break;
                case '0': out += '\0'; break;
                default: out += '\\'; out += c; break;
            }
            i += 1;
        } else {
            out += s[i];
        }
    }
    return out;
}

// ============================================================
// 文件系统（宽字符路径，兼容中文）
// ============================================================
// universal newlines：与 Python 文本模式一致（\\r\\n、\\r 均转 \\n）
static std::string univNewlines(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        char c = in[i];
        if (c == '\r') {
            out += '\n';
            if (i + 1 < in.size() && in[i + 1] == '\n') ++i;
        } else {
            out += c;
        }
    }
    return out;
}

// Windows 文本模式写入：\\n -> \\r\\n
static std::string toWindowsNewlines(const std::string& in) {
    if (in.find('\n') == std::string::npos) return in;
    std::string out;
    out.reserve(in.size() + 16);
    for (char c : in) {
        if (c == '\n') out += '\r';
        out += c;
    }
    return out;
}

std::string readFileUtf8(const std::string& path) {
    std::wstring wp = utf8ToWide(path);
    FILE* f = _wfopen(wp.c_str(), L"rb");
    if (!f) throw VesnaError("-fread 失败: 无法打开 " + path);
    std::string content;
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) content.append(buf, n);
    fclose(f);
    return univNewlines(content);
}

static void writeFileUtf8(const std::string& path, const std::string& content, bool append) {
    std::wstring wp = utf8ToWide(path);
    FILE* f = _wfopen(wp.c_str(), append ? L"ab" : L"wb");
    if (!f) throw VesnaError("-fwrite 失败: 无法写入 " + path);
    std::string out = toWindowsNewlines(content);
    fwrite(out.data(), 1, out.size(), f);
    fclose(f);
}

bool fileExists(const std::string& path) {
    std::wstring wp = utf8ToWide(path);
    DWORD attr = GetFileAttributesW(wp.c_str());
    return attr != INVALID_FILE_ATTRIBUTES;
}

static bool isDirectory(const std::string& path) {
    std::wstring wp = utf8ToWide(path);
    DWORD attr = GetFileAttributesW(wp.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

static void makeDirs(const std::string& path) {
    std::wstring wp = utf8ToWide(path);
    std::wstring cur;
    size_t i = 0;
    if (wp.size() >= 2 && wp[1] == L':') { cur = wp.substr(0, 3); i = 3; }  // 盘符
    else if (wp.size() >= 1 && (wp[0] == L'/' || wp[0] == L'\\')) { cur = wp.substr(0, 1); i = 1; }
    while (i < wp.size()) {
        size_t j = wp.find_first_of(L"/\\", i);
        std::wstring part = wp.substr(i, j == std::wstring::npos ? std::wstring::npos : j - i);
        if (!part.empty()) {
            if (!cur.empty() && cur.back() != L'\\' && cur.back() != L'/') cur += L'\\';
            cur += part;
            CreateDirectoryW(cur.c_str(), nullptr);
        }
        if (j == std::wstring::npos) break;
        i = j + 1;
    }
}

static std::vector<std::string> listDir(const std::string& path) {
    std::vector<std::string> out;
    std::wstring pattern = utf8ToWide(path);
    if (!pattern.empty() && pattern.back() != L'\\' && pattern.back() != L'/') pattern += L'\\';
    pattern += L'*';
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) throw VesnaError("-ls 失败: 无法打开目录 " + path);
    do {
        std::wstring name = fd.cFileName;
        if (name != L"." && name != L"..") out.push_back(wideToUtf8(name));
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    std::sort(out.begin(), out.end());
    return out;
}

static void removeTree(const std::string& path) {
    std::wstring wp = utf8ToWide(path);
    DWORD attr = GetFileAttributesW(wp.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) return;
    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        std::wstring pattern = wp + L"\\*";
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                std::wstring name = fd.cFileName;
                if (name != L"." && name != L"..") {
                    std::wstring child = wp + L"\\" + name;
                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) removeTree(wideToUtf8(child));
                    else DeleteFileW(child.c_str());
                }
            } while (FindNextFileW(h, &fd));
            FindClose(h);
        }
        RemoveDirectoryW(wp.c_str());
    } else {
        DeleteFileW(wp.c_str());
    }
}

static void copyPath(const std::string& src, const std::string& dst) {
    std::wstring ws = utf8ToWide(src), wd = utf8ToWide(dst);
    DWORD attr = GetFileAttributesW(ws.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) throw VesnaError("-copy 源不存在: " + src);
    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        std::wstring pattern = ws + L"\\*";
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE) return;
        do {
            std::wstring name = fd.cFileName;
            if (name != L"." && name != L"..") {
                std::wstring childSrc = ws + L"\\" + name;
                std::wstring childDst = wd + L"\\" + name;
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    CreateDirectoryW(childDst.c_str(), nullptr);
                    copyPath(wideToUtf8(childSrc), wideToUtf8(childDst));
                } else {
                    CopyFileW(childSrc.c_str(), childDst.c_str(), FALSE);
                }
            }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    } else {
        std::wstring parent = wd.substr(0, wd.find_last_of(L"/\\"));
        if (!parent.empty()) CreateDirectoryW(parent.c_str(), nullptr);
        if (!CopyFileW(ws.c_str(), wd.c_str(), FALSE))
            throw VesnaError("-copy 失败: " + src);
    }
}

// glob：目录 + 通配符匹配
static bool fnmatchSimple(const std::string& pat, const std::string& name) {
    // 支持 * 和 ?
    std::regex re;
    std::string reStr = "^";
    for (char c : pat) {
        if (c == '*') reStr += ".*";
        else if (c == '?') reStr += ".";
        else {
            if (std::string(".+^$()[]{}|\\").find(c) != std::string::npos) reStr += '\\';
            reStr += c;
        }
    }
    reStr += "$";
    return std::regex_match(name, std::regex(reStr));
}

static std::vector<std::string> globPaths(const std::string& pat) {
    std::vector<std::string> out;
    std::string dir, fname;
    size_t slash = pat.find_last_of("/\\");
    if (slash == std::string::npos) { dir = "."; fname = pat; }
    else { dir = pat.substr(0, slash); fname = pat.substr(slash + 1); }
    if (dir.empty()) dir = ".";
    std::vector<std::string> entries;
    try { entries = listDir(dir); } catch (...) { return out; }
    for (const auto& e : entries) {
        if (fnmatchSimple(fname, e)) {
            std::string full = dir;
            if (full.back() != '\\' && full.back() != '/') full += '\\';
            full += e;
            out.push_back(full);
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

// ============================================================
// 词法
// ============================================================
static const std::set<std::string> KEYWORDS = {
    "if", "elif", "else", "while", "for", "in",
    "def", "back", "break", "continue",
    "try", "catch", "import", "and", "or", "not",
};

// 惰性构建（g_builtinNames 定义在 BUILTINS 之后，避免静态初始化顺序问题）
static const std::set<std::string>& builtinSet() {
    static const std::set<std::string> m = []{
        std::set<std::string> s;
        for (const auto& p : g_builtinNames) s.insert(p.first);
        s.insert("into");  // 类型转换关键字（未在注册表）
        return s;
    }();
    return m;
}

static const std::set<std::string> TYPE_KEYWORDS = {"int", "str", "float", "list", "dict", "bool"};

std::vector<Token> lexExpr(const std::string& s, int ln) {
    std::vector<Token> toks;
    size_t i = 0;
    size_t n = s.size();
    while (i < n) {
        char c = s[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { ++i; continue; }

        // #f"..." 插值字符串
        if (c == '#' && i + 2 < n && s[i + 1] == 'f' && s[i + 2] == '"') {
            size_t j = i + 3;
            std::string buf;
            while (j < n && s[j] != '"') {
                if (s[j] == '\\' && j + 1 < n) { buf += s[j]; buf += s[j + 1]; j += 2; }
                else { buf += s[j]; ++j; }
            }
            if (j >= n) throw VesnaError("插值字符串未闭合", ln);
            Token t; t.kind = TK_INTERP; t.value = unescape(buf); t.line = ln;
            toks.push_back(std::move(t));
            i = j + 1;
            continue;
        }

        // #xxx 内置函数
        if (c == '#' && i + 1 < n && (std::isalpha((unsigned char)s[i + 1]) || s[i + 1] == '_')) {
            size_t j = i + 1;
            while (j < n && (std::isalnum((unsigned char)s[j]) || s[j] == '_')) ++j;
            std::string w = s.substr(i + 1, j - i - 1);
            if (builtinSet().count(w)) {
                Token t; t.kind = TK_BUILTIN; t.value = w; t.line = ln;
                toks.push_back(std::move(t));
                i = j;
                continue;
            }
        }

        // 字符串
        if (c == '"') {
            size_t j = i + 1;
            std::string buf;
            while (j < n && s[j] != '"') {
                if (s[j] == '\\' && j + 1 < n) { buf += s[j]; buf += s[j + 1]; j += 2; }
                else { buf += s[j]; ++j; }
            }
            if (j >= n) throw VesnaError("字符串未闭合", ln);
            Token t; t.kind = TK_STRING; t.value = unescape(buf); t.line = ln;
            toks.push_back(std::move(t));
            i = j + 1;
            continue;
        }

        // 数字 '...'
        if (c == '\'') {
            size_t j = s.find('\'', i + 1);
            if (j == std::string::npos) throw VesnaError("数字未闭合", ln);
            std::string body = s.substr(i + 1, j - i - 1);
            if (!isNumberStr(body)) throw VesnaError("无效数字: '" + body + "'", ln);
            Token t; t.kind = TK_NUMBER; t.value = body; t.line = ln;
            toks.push_back(std::move(t));
            i = j + 1;
            continue;
        }

        // 标识符 / 关键字
        if (std::isalpha((unsigned char)c) || c == '_') {
            size_t j = i;
            while (j < n && (std::isalnum((unsigned char)s[j]) || s[j] == '_')) ++j;
            std::string w = s.substr(i, j - i);
            Token t; t.line = ln;
            if (w == "true") { t.kind = TK_BOOL; t.bval = true; }
            else if (w == "false") { t.kind = TK_BOOL; t.bval = false; }
            else if (w == "none") { t.kind = TK_NONE; }
            else if (w == "or") { t.kind = TK_OR; t.value = w; }
            else if (w == "and") { t.kind = TK_AND; t.value = w; }
            else if (w == "not") { t.kind = TK_NOT; t.value = w; }
            else if (KEYWORDS.count(w)) { t.kind = TK_KW; t.value = w; }
            else if (TYPE_KEYWORDS.count(w)) { t.kind = TK_TYPE; t.value = w; }
            else { t.kind = TK_IDENT; t.value = w; }
            toks.push_back(std::move(t));
            i = j;
            continue;
        }

        // 双字符
        if (i + 1 < n) {
            std::string two = s.substr(i, 2);
            Token t; t.line = ln;
            bool hit = true;
            if (two == "+=") { t.kind = TK_PLUSEQ; t.value = "+"; }
            else if (two == "-=") { t.kind = TK_MINUSEQ; t.value = "-"; }
            else if (two == "*=") { t.kind = TK_STAREQ; t.value = "*"; }
            else if (two == "/=") { t.kind = TK_SLASHEQ; t.value = "/"; }
            else if (two == "./") { t.kind = TK_IDIV; t.value = "./"; }
            else if (two == "/.") { t.kind = TK_FDIV; t.value = "/."; }
            else if (two == "/-") { t.kind = TK_MOD; t.value = "/-"; }
            else if (two == "==") { t.kind = TK_EQ; t.value = "=="; }
            else if (two == "!=") { t.kind = TK_NE; t.value = "!="; }
            else if (two == ">=") { t.kind = TK_GE; t.value = ">="; }
            else if (two == "<=") { t.kind = TK_LE; t.value = "<="; }
            else hit = false;
            if (hit) { toks.push_back(std::move(t)); i += 2; continue; }
        }

        // 单字符
        Token t; t.line = ln;
        switch (c) {
            case '+': t.kind = TK_PLUS; t.value = "+"; break;
            case '-': t.kind = TK_MINUS; t.value = "-"; break;
            case '*': t.kind = TK_STAR; t.value = "*"; break;
            case '/': t.kind = TK_SLASH; t.value = "/"; break;
            case '=': t.kind = TK_ASSIGN; t.value = "="; break;
            case '<': t.kind = TK_LT; t.value = "<"; break;
            case '>': t.kind = TK_GT; t.value = ">"; break;
            case '(': t.kind = TK_LPAREN; t.value = "("; break;
            case ')': t.kind = TK_RPAREN; t.value = ")"; break;
            case '[': t.kind = TK_LBRACK; t.value = "["; break;
            case ']': t.kind = TK_RBRACK; t.value = "]"; break;
            case '{': t.kind = TK_LBRACE; t.value = "{"; break;
            case '}': t.kind = TK_RBRACE; t.value = "}"; break;
            case ',': t.kind = TK_COMMA; t.value = ","; break;
            case ';': t.kind = TK_SEMI; t.value = ";"; break;
            case ':': t.kind = TK_COLON; t.value = ":"; break;
            default:
                throw VesnaError(std::string("未知字符: ") + c, ln);
        }
        toks.push_back(std::move(t));
        ++i;
    }
    Token eof; eof.kind = TK_EOF; eof.line = ln;
    toks.push_back(std::move(eof));
    return toks;
}

// ============================================================
// 预处理
// ============================================================
static std::string extractVesna(const std::string& src) {
    size_t i = src.find("<vesna>");
    if (i == std::string::npos) return src;
    size_t j = src.find("</vesna>", i);
    if (j == std::string::npos) return src.substr(i + 7);
    return src.substr(i + 7, j - (i + 7));
}

static std::string removeComments(const std::string& src) {
    std::string out;
    out.reserve(src.size());
    size_t i = 0;
    size_t n = src.size();
    bool in_dq = false, escape = false;
    while (i < n) {
        char c = src[i];
        if (in_dq) {
            if (escape) escape = false;
            else if (c == '\\') escape = true;
            else if (c == '"') in_dq = false;
            out += c;
            ++i;
            continue;
        }
        if (c == '"') { in_dq = true; out += c; ++i; continue; }
        if (c == '/' && i + 1 < n && src[i + 1] == '*') {
            size_t j = src.find("*/", i + 2);
            if (j == std::string::npos) {
                out.append(std::count(src.begin() + (std::ptrdiff_t)i, src.end(), '\n'), '\n');
                break;
            }
            out.append(std::count(src.begin() + (std::ptrdiff_t)i,
                                  src.begin() + (std::ptrdiff_t)(j + 2), '\n'), '\n');
            i = j + 2;
        } else {
            out += c;
            ++i;
        }
    }
    return out;
}

static std::vector<std::pair<std::string, int>> splitLogicalLines(const std::string& src) {
    std::vector<std::pair<std::string, int>> result;
    std::string buf;
    int buf_start = 1;
    int depth_paren = 0, depth_brack = 0, depth_brace = 0;
    bool in_dq = false, in_sq = false, escape = false;
    int phys_no = 1;
    for (char c : src) {
        if (buf.empty()) buf_start = phys_no;
        if (c == '\n') {
            // 行结束：判断是否闭合
            if (depth_paren > 0 || depth_brack > 0 || depth_brace > 0 || in_dq || in_sq)
                buf += ' ';
            else {
                result.emplace_back(buf, buf_start);
                buf.clear();
            }
            ++phys_no;
            continue;
        }
        if (escape) { escape = false; buf += c; continue; }
        if (in_dq) {
            if (c == '\\') escape = true;
            else if (c == '"') in_dq = false;
            buf += c;
            continue;
        }
        if (in_sq) {
            if (c == '\'') in_sq = false;
            buf += c;
            continue;
        }
        if (c == '"') { in_dq = true; buf += c; }
        else if (c == '\'') { in_sq = true; buf += c; }
        else if (c == '(') { ++depth_paren; buf += c; }
        else if (c == ')') { --depth_paren; buf += c; }
        else if (c == '[') { ++depth_brack; buf += c; }
        else if (c == ']') { --depth_brack; buf += c; }
        else if (c == '{') { ++depth_brace; buf += c; }
        else if (c == '}') { --depth_brace; buf += c; }
        else buf += c;
    }
    if (!buf.empty()) result.emplace_back(buf, buf_start);
    return result;
}

static std::vector<std::string> splitStatements(const std::string& text) {
    std::vector<std::string> result;
    std::string buf;
    bool in_dq = false, in_sq = false, escape = false;
    int depth = 0;
    for (char c : text) {
        if (escape) { escape = false; buf += c; continue; }
        if (in_dq) {
            if (c == '\\') escape = true;
            else if (c == '"') in_dq = false;
            buf += c;
            continue;
        }
        if (in_sq) {
            if (c == '\'') in_sq = false;
            buf += c;
            continue;
        }
        if (c == '"') { in_dq = true; buf += c; continue; }
        if (c == '\'') { in_sq = true; buf += c; continue; }
        if (c == '(' || c == '[' || c == '{') { ++depth; buf += c; continue; }
        if (c == ')' || c == ']' || c == '}') { --depth; buf += c; continue; }
        if (c == ',' && depth == 0) { result.push_back(buf); buf.clear(); continue; }
        buf += c;
    }
    if (!buf.empty()) result.push_back(buf);
    return result;
}

std::vector<Line> preprocess(const std::string& src) {
    std::string s = removeComments(extractVesna(src));
    std::vector<Line> lines;
    for (const auto& [text, phys_no] : splitLogicalLines(s)) {
        for (std::string& stmtRaw : splitStatements(text)) {
            std::string st = stmtRaw;
            // 去首尾空白
            size_t b = st.find_first_not_of(" \t\r\n");
            if (b == std::string::npos) continue;
            size_t e = st.find_last_not_of(" \t\r\n");
            st = st.substr(b, e - b + 1);

            int indent = 0;
            while (!st.empty() && st[0] == '-') { ++indent; st = st.substr(1); }
            b = st.find_first_not_of(" \t\r\n");
            if (b != std::string::npos) st = st.substr(b);
            else st.clear();

            bool block_start = false;
            if (!st.empty() && st.back() == '-') {
                block_start = true;
                st.pop_back();
                size_t e2 = st.find_last_not_of(" \t\r\n");
                if (e2 == std::string::npos) st.clear();
                else st = st.substr(0, e2 + 1);
            }
            if (!st.empty()) {
                Line ln;
                ln.indent = indent;
                ln.content = st;
                ln.block_start = block_start;
                ln.line_no = phys_no;
                lines.push_back(std::move(ln));
            }
        }
    }
    return lines;
}

// ============================================================
// 表达式解析
// ============================================================
static std::string binopName(const std::string& sym) {
    if (sym == "+") return "PLUS";
    if (sym == "-") return "MINUS";
    if (sym == "*") return "STAR";
    if (sym == "/") return "SLASH";
    if (sym == "==") return "==";
    if (sym == "!=") return "!=";
    if (sym == "<") return "LT";
    if (sym == ">") return "GT";
    if (sym == "<=") return "<=";
    if (sym == ">=") return ">=";
    return sym;
}

class ExprParser {
    const std::vector<Token>& toks;
    size_t pos = 0;

    const Token& peek() { return toks[pos]; }
    const Token& advance() { return toks[pos++]; }
    const Token& expect(TokKind k) {
        const Token& t = peek();
        if (t.kind != k) throw VesnaError("期望 " + tokKindName(k) + "，实际 " + tokKindName(t.kind), t.line);
        return advance();
    }
    static std::string tokKindName(TokKind k) {
        switch (k) {
            case TK_EOF: return "EOF";
            case TK_RPAREN: return "RPAREN";
            case TK_SEMI: return "SEMI";
            case TK_COLON: return "COLON";
            case TK_TYPE: return "TYPE";
            default: return "TOKEN";
        }
    }

public:
    explicit ExprParser(const std::vector<Token>& t) : toks(t) {}

    // 左值解析（赋值目标：变量名 [下标]）
    std::shared_ptr<Expr> parseLValue() {
        const Token& t = peek();
        if (t.kind != TK_IDENT) throw VesnaError("赋值左侧必须是变量", t.line);
        std::string name = t.value;
        advance();
        if (peek().kind == TK_LBRACK) {
            advance();
            auto idx = parse();
            expect(TK_RBRACK);
            auto e = std::make_shared<Expr>(); e->k = Expr::K::INDEX; e->str = name; e->nid = internId(name); e->b = idx;
            e->a = nullptr;
            return e;
        }
        auto e = std::make_shared<Expr>(); e->k = Expr::K::VAR; e->str = name; e->nid = internId(name);
        return e;
    }

    std::shared_ptr<Expr> parseStatement() {
        auto e = parse();
        if (peek().kind != TK_EOF)
            throw VesnaError("表达式后有多余内容: " + (peek().value.empty() ? tokKindName(peek().kind) : peek().value), peek().line);
        return e;
    }

    std::shared_ptr<Expr> parse() { return orExpr(); }

private:
    std::shared_ptr<Expr> orExpr() {
        auto l = andExpr();
        while (peek().kind == TK_OR) {
            advance();
            auto r = andExpr();
            auto e = std::make_shared<Expr>(); e->k = Expr::K::OR; e->a = l; e->b = r;
            l = e;
        }
        return l;
    }

    std::shared_ptr<Expr> andExpr() {
        auto l = notExpr();
        while (peek().kind == TK_AND) {
            advance();
            auto r = notExpr();
            auto e = std::make_shared<Expr>(); e->k = Expr::K::AND; e->a = l; e->b = r;
            l = e;
        }
        return l;
    }

    std::shared_ptr<Expr> notExpr() {
        if (peek().kind == TK_NOT) {
            advance();
            auto e = std::make_shared<Expr>(); e->k = Expr::K::NOT; e->a = notExpr();
            return e;
        }
        return comparison();
    }

    std::shared_ptr<Expr> comparison() {
        auto l = additive();
        while (peek().kind == TK_EQ || peek().kind == TK_NE || peek().kind == TK_LT ||
               peek().kind == TK_GT || peek().kind == TK_LE || peek().kind == TK_GE) {
            std::string op = binopName(advance().value);
            auto r = additive();
            auto e = std::make_shared<Expr>(); e->k = Expr::K::BIN; e->op = op; e->a = l; e->b = r;
            l = e;
        }
        return l;
    }

    std::shared_ptr<Expr> additive() {
        auto l = mul();
        while (peek().kind == TK_PLUS || peek().kind == TK_MINUS) {
            std::string op = binopName(advance().value);
            auto r = mul();
            auto e = std::make_shared<Expr>(); e->k = Expr::K::BIN; e->op = op; e->a = l; e->b = r;
            l = e;
        }
        return l;
    }

    std::shared_ptr<Expr> mul() {
        auto l = unary();
        while (peek().kind == TK_STAR || peek().kind == TK_SLASH || peek().kind == TK_IDIV ||
               peek().kind == TK_FDIV || peek().kind == TK_MOD) {
            std::string op = binopName(advance().value);
            auto r = unary();
            auto e = std::make_shared<Expr>(); e->k = Expr::K::BIN; e->op = op; e->a = l; e->b = r;
            l = e;
        }
        return l;
    }

    std::shared_ptr<Expr> unary() {
        if (peek().kind == TK_MINUS) {
            advance();
            auto e = std::make_shared<Expr>(); e->k = Expr::K::NEG; e->a = unary();
            return e;
        }
        return postfix();
    }

    std::shared_ptr<Expr> postfix() {
        auto e = primary();
        while (peek().kind == TK_LBRACK) {
            advance();
            auto idx = parse();
            expect(TK_RBRACK);
            auto n = std::make_shared<Expr>(); n->k = Expr::K::INDEX; n->a = e; n->b = idx;
            e = n;
        }
        return e;
    }

    std::shared_ptr<Expr> primary() {
        const Token& t = peek();

        if (t.kind == TK_NUMBER) {
            advance();
            auto e = std::make_shared<Expr>();
            e->k = Expr::K::NUM;
            if (t.value.find('.') != std::string::npos) { e->is_float = true; e->fnum = std::stod(t.value); }
            else { e->is_float = false; e->inum = std::stoll(t.value); }
            return e;
        }
        if (t.kind == TK_STRING) {
            advance();
            auto e = std::make_shared<Expr>(); e->k = Expr::K::STR; e->str = t.value;
            return e;
        }
        if (t.kind == TK_BOOL) {
            advance();
            auto e = std::make_shared<Expr>(); e->k = Expr::K::BOOL; e->bval = t.bval;
            return e;
        }
        if (t.kind == TK_NONE) {
            advance();
            auto e = std::make_shared<Expr>(); e->k = Expr::K::NONE; return e;
        }
        if (t.kind == TK_INTERP) {
            advance();
            auto e = std::make_shared<Expr>(); e->k = Expr::K::INTERP; e->str = t.value;
            return e;
        }

        if (t.kind == TK_BUILTIN) {
            std::string name = t.value;
            advance();
            expect(TK_LPAREN);
            auto e = std::make_shared<Expr>();
            if (name == "into") {
                const Token& tt = peek();
                if (tt.kind != TK_TYPE) throw VesnaError("-into 第一个参数必须是类型关键字", tt.line);
                advance();
                expect(TK_SEMI);
                e->k = Expr::K::INTO; e->str = tt.value; e->a = parse();
                expect(TK_RPAREN);
                return e;
            }
            if (name == "args") {
                expect(TK_RPAREN);
                e->k = Expr::K::BUILTIN; e->str = name;
                return e;
            }
            e->k = Expr::K::BUILTIN;
            e->str = name;
            if (peek().kind != TK_RPAREN) {
                e->args.push_back(parse());
                while (peek().kind == TK_SEMI) {
                    advance();
                    e->args.push_back(parse());
                }
            }
            expect(TK_RPAREN);
            return e;
        }

        if (t.kind == TK_IDENT) {
            std::string name = t.value;
            advance();
            if (peek().kind == TK_LPAREN) {
                advance();
                auto e = std::make_shared<Expr>(); e->k = Expr::K::CALL; e->str = name; e->nid = internId(name);
                if (peek().kind != TK_RPAREN) {
                    e->args.push_back(parse());
                    while (peek().kind == TK_SEMI) {
                        advance();
                        e->args.push_back(parse());
                    }
                }
                expect(TK_RPAREN);
                return e;
            }
            auto e = std::make_shared<Expr>(); e->k = Expr::K::VAR; e->str = name; e->nid = internId(name);
            return e;
        }

        if (t.kind == TK_LPAREN) {
            advance();
            auto first = parse();
            if (peek().kind == TK_SEMI) {
                auto e = std::make_shared<Expr>(); e->k = Expr::K::GROUP;
                e->args.push_back(first);
                while (peek().kind == TK_SEMI) {
                    advance();
                    e->args.push_back(parse());
                }
                expect(TK_RPAREN);
                return e;
            }
            expect(TK_RPAREN);
            return first;
        }

        if (t.kind == TK_LBRACK) {
            advance();
            auto e = std::make_shared<Expr>(); e->k = Expr::K::LIST;
            if (peek().kind != TK_RBRACK) {
                e->args.push_back(parse());
                while (peek().kind == TK_SEMI) {
                    advance();
                    e->args.push_back(parse());
                }
            }
            expect(TK_RBRACK);
            return e;
        }

        if (t.kind == TK_LBRACE) {
            advance();
            auto e = std::make_shared<Expr>(); e->k = Expr::K::DICT;
            if (peek().kind != TK_RBRACE) {
                auto k = parse();
                expect(TK_COLON);
                auto v = parse();
                e->pairs.emplace_back(k, v);
                while (peek().kind == TK_SEMI) {
                    advance();
                    auto k2 = parse();
                    expect(TK_COLON);
                    auto v2 = parse();
                    e->pairs.emplace_back(k2, v2);
                }
            }
            expect(TK_RBRACE);
            return e;
        }

        throw VesnaError("表达式不完整（可能是括号未闭合），实际 " + tokKindName(t.kind), t.line);
    }
};

std::shared_ptr<Expr> parseExpr(const std::string& s, int ln) {
    return ExprParser(lexExpr(s, ln)).parseStatement();
}

// ============================================================
// 语句解析
// ============================================================
// 将语句行解析为 AST（Parser::parse 内部使用）
// Parser::parse —— 顶层语句列表
std::vector<std::shared_ptr<Stmt>> Parser::parse() {
    std::vector<std::shared_ptr<Stmt>> out;
    while (pos < lines.size()) {
        auto s = stmt(0);
        if (!s) break;
        out.push_back(s);
    }
    return out;
}

std::shared_ptr<Stmt> Parser::stmt(int min_indent) {
    if (pos >= lines.size()) return nullptr;
    const Line& ln = lines[pos];
    if (ln.indent < min_indent) return nullptr;
    size_t start = pos;
    try {
        auto s = stmtInner(min_indent);
        if (s) s->line = ln.line_no;
        return s;
    } catch (VesnaError& e) {
        if (pos <= start) pos = start + 1;
        int line = e.line >= 0 ? e.line : ln.line_no;
        errors.emplace_back(line, e.msg);
        auto st = std::make_shared<Stmt>();
        st->k = Stmt::K::ERR;
        st->err_msg = e.msg;
        st->err_line = line;
        return st;
    }
}

static std::string trimStr(std::string s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

std::shared_ptr<Stmt> Parser::stmtInner(int min_indent) {
    (void)min_indent;
    Line& ln = lines[pos];
    const std::string& c = ln.content;

    if (c.rfind("if ", 0) == 0 && ln.block_start) return parseIf(ln);
    if (c.rfind("while ", 0) == 0 && ln.block_start) return parseWhile(ln);
    if (c.rfind("for ", 0) == 0 && ln.block_start) return parseFor(ln);
    if (c.rfind("def ", 0) == 0 && ln.block_start) return parseDef(ln);
    if (c == "try" && ln.block_start) return parseTry(ln);
    if (c.rfind("import ", 0) == 0) {
        ++pos;
        auto st = std::make_shared<Stmt>();
        st->k = Stmt::K::IMPORT;
        st->import_name = trimStr(c.substr(7));
        return st;
    }

    ++pos;

    if (c == "break") {
        auto st = std::make_shared<Stmt>(); st->k = Stmt::K::BREAK; return st;
    }
    if (c == "continue") {
        auto st = std::make_shared<Stmt>(); st->k = Stmt::K::CONTINUE; return st;
    }
    if (c.rfind("back(", 0) == 0 && !c.empty() && c.back() == ')') {
        auto st = std::make_shared<Stmt>();
        st->k = Stmt::K::BACK;
        st->expr = parseExpr(c.substr(5, c.size() - 6), ln.line_no);
        return st;
    }

    return parseSimple(c, ln.line_no);
}

std::shared_ptr<Stmt> Parser::parseIf(const Line& ln) {
    auto cond = parseExpr(ln.content.substr(3), ln.line_no);
    ++pos;
    std::vector<std::pair<std::shared_ptr<Expr>, std::vector<std::shared_ptr<Stmt>>>> branches;
    auto cur_cond = cond;
    std::vector<std::shared_ptr<Stmt>> cur_body;
    while (true) {
        while (pos < lines.size()) {
            const Line& nxt = lines[pos];
            if (nxt.indent != ln.indent + 1) break;
            auto s = stmt(ln.indent + 1);
            if (s) cur_body.push_back(s);
            else break;
        }
        if (pos >= lines.size()) break;
        const Line& nxt = lines[pos];
        if (nxt.indent != ln.indent) break;
        const std::string& c = nxt.content;
        if (c.rfind("elif ", 0) == 0) {
            branches.emplace_back(cur_cond, cur_body);
            cur_cond = parseExpr(c.substr(5), nxt.line_no);
            cur_body.clear();
            ++pos;
        } else if (c == "else") {
            branches.emplace_back(cur_cond, cur_body);
            cur_cond = nullptr;
            cur_body.clear();
            ++pos;
        } else {
            break;
        }
    }
    branches.emplace_back(cur_cond, cur_body);
    auto st = std::make_shared<Stmt>();
    st->k = Stmt::K::IF;
    st->branches = std::move(branches);
    return st;
}

std::shared_ptr<Stmt> Parser::parseWhile(const Line& ln) {
    auto cond = parseExpr(ln.content.substr(6), ln.line_no);
    ++pos;
    auto st = std::make_shared<Stmt>();
    st->k = Stmt::K::WHILE;
    st->cond = cond;
    while (pos < lines.size()) {
        const Line& nxt = lines[pos];
        if (nxt.indent != ln.indent + 1) break;
        auto s = stmt(ln.indent + 1);
        if (s) st->body.push_back(s);
    }
    return st;
}

std::shared_ptr<Stmt> Parser::parseFor(const Line& ln) {
    static const std::regex re(R"(^for\s+(\w+)\s+in\s+(.+)$)");
    std::smatch m;
    if (!std::regex_match(ln.content, m, re))
        throw VesnaError("for 语法错误，应为: for 变量 in 表达式-", ln.line_no);
    auto st = std::make_shared<Stmt>();
    st->k = Stmt::K::FOR;
    st->var = m[1].str();
    st->var_nid = internId(st->var);
    st->cond = parseExpr(m[2].str(), ln.line_no);
    ++pos;
    while (pos < lines.size()) {
        const Line& nxt = lines[pos];
        if (nxt.indent != ln.indent + 1) break;
        auto s = stmt(ln.indent + 1);
        if (s) st->body.push_back(s);
    }
    return st;
}

static std::vector<std::pair<int64_t, std::shared_ptr<Expr>>> splitParams(const std::string& s, int ln) {
    std::vector<std::pair<int64_t, std::shared_ptr<Expr>>> out;
    std::string t = trimStr(s);
    if (t.empty()) return out;
    std::vector<std::string> parts;
    int depth = 0;
    std::string cur;
    for (char ch : t) {
        if (ch == '(' || ch == '[' || ch == '{') { ++depth; cur += ch; }
        else if (ch == ')' || ch == ']' || ch == '}') { --depth; cur += ch; }
        else if (ch == ';' && depth == 0) { parts.push_back(trimStr(cur)); cur.clear(); }
        else cur += ch;
    }
    if (!cur.empty()) parts.push_back(trimStr(cur));
    for (const auto& p : parts) {
        size_t eq = p.find('=');
        if (eq != std::string::npos) {
            std::string name = trimStr(p.substr(0, eq));
            std::string def = trimStr(p.substr(eq + 1));
            out.emplace_back(internId(name), parseExpr(def, ln));
        } else {
            out.emplace_back(internId(p), nullptr);
        }
    }
    return out;
}

std::shared_ptr<Stmt> Parser::parseDef(const Line& ln) {
    static const std::regex re(R"(^def\s+(\w+)\s*\((.*)\)\s*$)");
    std::smatch m;
    if (!std::regex_match(ln.content, m, re))
        throw VesnaError("def 语法错误", ln.line_no);
    auto st = std::make_shared<Stmt>();
    st->k = Stmt::K::DEF;
    st->fname = m[1].str();
    st->params = splitParams(m[2].str(), ln.line_no);
    ++pos;
    while (pos < lines.size()) {
        const Line& nxt = lines[pos];
        if (nxt.indent != ln.indent + 1) break;
        auto s = stmt(ln.indent + 1);
        if (s) st->body.push_back(s);
    }
    return st;
}

std::shared_ptr<Stmt> Parser::parseTry(const Line& ln) {
    ++pos;
    auto st = std::make_shared<Stmt>();
    st->k = Stmt::K::TRY;
    while (pos < lines.size()) {
        const Line& nxt = lines[pos];
        if (nxt.indent != ln.indent + 1) break;
        if (nxt.content.rfind("catch ", 0) == 0) break;
        auto s = stmt(ln.indent + 1);
        if (s) st->try_body.push_back(s);
    }
    if (pos >= lines.size() || lines[pos].content.rfind("catch ", 0) != 0)
        throw VesnaError("try 需要 catch", ln.line_no);
    const Line& catch_line = lines[pos];
    static const std::regex re(R"(^catch\s+(\w+)\s*$)");
    std::smatch m;
    if (!std::regex_match(catch_line.content, m, re))
        throw VesnaError("catch 语法错误，应为: catch 变量", catch_line.line_no);
    st->catch_nid = internId(m[1].str());
    ++pos;
    while (pos < lines.size()) {
        const Line& nxt = lines[pos];
        if (nxt.indent != ln.indent + 1) break;
        auto s = stmt(ln.indent + 1);
        if (s) st->catch_body.push_back(s);
    }
    return st;
}

std::shared_ptr<Stmt> Parser::parseSimple(const std::string& content, int ln_no) {
    auto toks = lexExpr(content, ln_no);
    int depth = 0;
    int op_pos = -1;
    std::string op_kind;
    for (size_t i = 0; i < toks.size(); ++i) {
        TokKind k = toks[i].kind;
        if (k == TK_LPAREN || k == TK_LBRACK || k == TK_LBRACE) ++depth;
        else if (k == TK_RPAREN || k == TK_RBRACK || k == TK_RBRACE) --depth;
        else if (depth == 0 && (k == TK_ASSIGN || k == TK_PLUSEQ || k == TK_MINUSEQ ||
                                k == TK_STAREQ || k == TK_SLASHEQ)) {
            op_pos = (int)i;
            op_kind = (k == TK_ASSIGN) ? "ASSIGN" : toks[i].value;
            break;
        }
    }
    auto st = std::make_shared<Stmt>();
    if (op_pos >= 0) {
        std::vector<Token> left(toks.begin(), toks.begin() + op_pos);
        Token eof; eof.kind = TK_EOF; eof.line = ln_no;
        left.push_back(eof);
        std::vector<Token> right(toks.begin() + op_pos + 1, toks.end());
        auto lv = ExprParser(left).parseLValue();
        st->k = Stmt::K::ASSIGN;
        st->op_kind = op_kind;
        st->val = ExprParser(right).parseStatement();
        st->lv_name = lv->str;
        st->lv_nid = lv->nid;
        st->lv_idx = (lv->k == Expr::K::INDEX) ? lv->b : nullptr;
        return st;
    }
    st->k = Stmt::K::EXPR;
    st->expr = ExprParser(toks).parseStatement();
    return st;
}

// ============================================================
// 环境
// ============================================================
const Value* Env::getRef(int64_t nid) const {
    auto it = vars.find(nid);
    if (it != vars.end()) return &it->second;
    if (parent) return parent->getRef(nid);
    return nullptr;
}

void Env::set(int64_t nid, const Value& v) {
    vars[nid] = v;
}

std::shared_ptr<Function> Env::getFunc(int64_t nid) const {
    auto it = funcs.find(nid);
    if (it != funcs.end()) return it->second;
    if (parent) return parent->getFunc(nid);
    throw VesnaError("未定义函数: " + internName(nid));
}

void Env::setFunc(int64_t nid, const std::shared_ptr<Function>& fn) {
    funcs[nid] = fn;
}

// ============================================================
// 运行时：数值运算辅助
// ============================================================
static const std::unordered_map<std::string, std::string> COMPOUND_OP = {
    {"+", "PLUS"}, {"-", "MINUS"}, {"*", "STAR"}, {"/", "SLASH"},
};

static Value numArith(const Value& l, const Value& r, char op) {
    bool fl = l.t() == Value::T::FLOAT, fr = r.t() == Value::T::FLOAT;
    if (!fl && !fr) {
        int64_t a = l.i(), b = r.i();
        switch (op) {
            case '+': return mkInt(a + b);
            case '-': return mkInt(a - b);
            case '*': return mkInt(a * b);
        }
    }
    double a = fl ? l.f() : (double)l.i();
    double b = fr ? r.f() : (double)r.i();
    switch (op) {
        case '+': return mkFloat(a + b);
        case '-': return mkFloat(a - b);
        case '*': return mkFloat(a * b);
    }
    return mkNone();
}

static void numOnly(const Value& a, const Value& b) {
    if (a.t() == Value::T::BOOL || b.t() == Value::T::BOOL) throw VesnaError("该运算符需要数字");
    if (!isNum(a) || !isNum(b)) throw VesnaError("该运算符需要数字");
}

// 比较运算符要求可比较（数字或同类型），否则报错
static void numOnlyCmp(const Value& a, const Value& b) {
    if (isNum(a) && isNum(b)) return;
    if (a.t() != b.t()) throw VesnaError("无法比较: " + fmt(a) + " 和 " + fmt(b));
    if (a.t() != Value::T::STR && a.t() != Value::T::BOOL)
        throw VesnaError("无法比较: " + fmt(a) + " 和 " + fmt(b));
}

static double numVal(const Value& v) {
    return v.t() == Value::T::INT ? (double)v.i() : v.f();
}

static int64_t pyModInt(int64_t a, int64_t b) {
    int64_t r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) r += b;
    return r;
}

static double pyModFloat(double a, double b) {
    double r = std::fmod(a, b);
    if (r != 0.0 && ((r < 0) != (b < 0))) r += b;
    return r;
}

// ============================================================
// 运行时：Interp
// ============================================================
void Interp::run(const std::vector<std::shared_ptr<Stmt>>& program) {
    if (dbg) frames.emplace_back("<main>", 0);
    for (auto& s : program) exec(s, g);
    if (dbg) frames.pop_back();
}

void Interp::exec(const std::shared_ptr<Stmt>& stmt, const std::shared_ptr<Env>& env) {
    if (dbg && stmt->line > 0) dbgCheck(stmt, env);
    switch (stmt->k) {
        case Stmt::K::ASSIGN:
            assign(stmt->lv_nid, stmt->lv_idx, stmt->op_kind, eval(stmt->val, env), env);
            break;

        case Stmt::K::EXPR:
            eval(stmt->expr, env);
            break;

        case Stmt::K::BACK:
            throw ReturnSignal(eval(stmt->expr, env));

        case Stmt::K::BREAK:
            throw BreakSignal();

        case Stmt::K::CONTINUE:
            throw ContinueSignal();

        case Stmt::K::IF: {
            for (auto& [cond, body] : stmt->branches) {
                if (!cond || truthy(eval(cond, env))) {
                    for (auto& s : body) exec(s, env);
                    break;
                }
            }
            break;
        }

        case Stmt::K::WHILE: {
            while (truthy(eval(stmt->cond, env))) {
                try {
                    for (auto& s : stmt->body) exec(s, env);
                } catch (BreakSignal&) {
                    break;
                } catch (ContinueSignal&) {
                    continue;
                }
            }
            break;
        }

        case Stmt::K::FOR: {
            Value iterable = eval(stmt->cond, env);
            std::vector<Value> items;
            if (iterable.t() == Value::T::DICT) {
                for (auto& p : iterable.dict()->pairs) items.push_back(p.first);
            } else if (iterable.t() == Value::T::LIST) {
                items = iterable.list()->items;
            } else if (iterable.t() == Value::T::GROUP) {
                items = iterable.group()->items;
            } else if (iterable.t() == Value::T::STR) {
                for (char c : iterable.s()) items.push_back(mkStr(std::string(1, c)));
            } else {
                throw VesnaError("无法迭代: " + fmt(iterable));
            }
            for (auto& item : items) {
                env->set(stmt->var_nid, item);
                try {
                    for (auto& s : stmt->body) exec(s, env);
                } catch (BreakSignal&) {
                    break;
                } catch (ContinueSignal&) {
                    continue;
                }
            }
            break;
        }

        case Stmt::K::DEF: {
            auto fn = std::make_shared<Function>();
            fn->name = stmt->fname;
            fn->params = stmt->params;
            fn->body = stmt->body;
            fn->closure = env;
            env->setFunc(internId(stmt->fname), fn);
            kept_.push_back(env);  // 保活被闭包引用的环境
            break;
        }

        case Stmt::K::TRY: {
            try {
                for (auto& s : stmt->try_body) exec(s, env);
            } catch (VesnaError& e) {
                env->set(stmt->catch_nid, mkStr(e.str()));
                for (auto& s : stmt->catch_body) exec(s, env);
            }
            break;
        }

        case Stmt::K::IMPORT:
            doImport(stmt->import_name, env);
            break;

        case Stmt::K::ERR:
            throw VesnaError(stmt->err_msg, stmt->err_line);

        default:
            throw VesnaError("未知语句");
    }
}

// ---- 调试器 ----
void Interp::dbgCheck(const std::shared_ptr<Stmt>& stmt, const std::shared_ptr<Env>& env) {
    dbg_line = stmt->line;
    dbg_env = env;
    dbg_fname = frames.empty() ? "<main>" : frames.back().first;
    if (!frames.empty()) frames.back().second = dbg_line;
    bool hit = dbg_breaks.count(stmt->line) > 0;
    bool stepping = false;
    if (dbg_mode == 1) stepping = true;
    else if (dbg_mode == 2) stepping = (int)frames.size() <= dbg_next_depth;
    if (hit || stepping) dbgLoop();
}

void Interp::dbgLoop() {
    std::cout << "停在 " << dbg_file << ":" << dbg_line << "（" << dbg_fname << "）\n";
    while (true) {
        std::cout << "vesna-dbg> ";
        std::cout.flush();
        std::string line;
        if (!std::getline(std::cin, line)) { dbg_mode = 0; return; }
        std::string cmd = trimStr(line);
        if (cmd.empty()) continue;
        if (cmd == "c" || cmd == "continue") { dbg_mode = 0; return; }
        if (cmd == "n" || cmd == "next") { dbg_mode = 2; dbg_next_depth = (int)frames.size(); return; }
        if (cmd == "s" || cmd == "step") { dbg_mode = 1; return; }
        if (cmd == "q" || cmd == "quit") { std::cout << "已退出调试器\n"; std::exit(0); }
        if (cmd == "bt" || cmd == "backtrace") { dbgPrintFrames(); continue; }
        if (cmd == "list" || cmd == "l") { dbgPrintList(); continue; }
        if (cmd.rfind("break ", 0) == 0 || cmd.rfind("b ", 0) == 0) {
            int pos = (cmd[1] == ' ') ? 2 : 6;
            int ln = atoi(trimStr(cmd.substr(pos)).c_str());
            if (ln <= 0) { std::cout << "用法: break <行号>\n"; continue; }
            dbg_breaks.insert(ln);
            std::cout << "断点 @ " << ln << "\n";
            continue;
        }
        if (cmd.rfind("del ", 0) == 0) {
            int ln = atoi(trimStr(cmd.substr(4)).c_str());
            dbg_breaks.erase(ln);
            std::cout << "已删除断点 " << ln << "\n";
            continue;
        }
        if (cmd.rfind("print ", 0) == 0 || cmd.rfind("p ", 0) == 0) {
            std::string expr = trimStr(cmd.substr(cmd[1] == ' ' ? 2 : 6));
            try {
                auto e = parseExpr(expr, dbg_line);
                Value v = eval(e, dbg_env);
                std::cout << fmt(v) << "\n";
            } catch (VesnaError& err) {
                std::cout << "错误: " << err.msg << "\n";
            }
            continue;
        }
        if (cmd == "vars" || cmd == "v") {
            for (auto& [id, val] : dbg_env->vars)
                std::cout << "  " << internName(id) << " = " << fmt(val) << "\n";
            continue;
        }
        if (cmd == "help" || cmd == "h") { dbgHelp(); continue; }
        std::cout << "未知命令，输入 help 查看\n";
    }
}

void Interp::dbgPrintFrames() {
    for (size_t i = frames.size(); i-- > 0;)
        std::cout << "  #" << i << " " << frames[i].first << ":" << frames[i].second << "\n";
}

void Interp::dbgPrintList() {
    std::string src;
    try { src = readFileUtf8(dbg_file); } catch (...) { std::cout << "无法读取源码\n"; return; }
    std::vector<std::string> lines;
    std::string cur;
    for (char ch : src) {
        if (ch == '\n') { lines.push_back(cur); cur.clear(); }
        else cur += ch;
    }
    if (!cur.empty() || src.empty()) lines.push_back(cur);
    int lo = std::max(1, dbg_line - 3), hi = std::min((int)lines.size(), dbg_line + 3);
    for (int i = lo; i <= hi; ++i) {
        std::string mark = (i == dbg_line) ? "=>" : "  ";
        std::cout << mark << " " << i << " | " << (i - 1 < (int)lines.size() ? lines[i - 1] : "") << "\n";
    }
}

void Interp::dbgHelp() {
    std::cout << "命令:\n"
              << "  c / continue   继续运行\n"
              << "  n / next       执行下一语句（不进入函数）\n"
              << "  s / step       单步（进入函数）\n"
              << "  b <行号>       设置断点\n"
              << "  del <行号>     删除断点\n"
              << "  p <表达式>     求值表达式\n"
              << "  vars           列出当前变量\n"
              << "  bt             查看调用栈\n"
              << "  list           查看附近源码\n"
              << "  q / quit       退出\n";
}

void Interp::assign(int64_t nid, const std::shared_ptr<Expr>& idx,
                    const std::string& op_kind, const Value& val, const std::shared_ptr<Env>& env) {
    if (!idx) {
        Value v = val;
        if (op_kind != "ASSIGN") {
            auto it = COMPOUND_OP.find(op_kind);
            if (it == COMPOUND_OP.end()) throw VesnaError("未知复合运算符 " + op_kind);
            const Value* oldv = env->getRef(nid);
            v = binop(it->second, oldv ? *oldv : mkStr(internName(nid)), val);
        }
        env->set(nid, v);
        return;
    }
    Value idxv = eval(idx, env);
    const Value* target_p = env->getRef(nid);
    Value target = target_p ? *target_p : mkStr(internName(nid));
    if (target.t() == Value::T::LIST) {
        if (idxv.t() == Value::T::FLOAT && idxv.f() == (double)(int64_t)idxv.f()) idxv = mkInt((int64_t)idxv.f());
        if (idxv.t() != Value::T::INT) throw VesnaError("下标必须是整数");
        int64_t py = (idxv.i() > 0) ? idxv.i() - 1 : idxv.i();
        if (py < 0) py += (int64_t)target.list()->items.size();
        if (py < 0 || py >= (int64_t)target.list()->items.size())
            throw VesnaError("下标越界: " + fmt(idxv));
        Value v = val;
        if (op_kind != "ASSIGN") {
            auto it = COMPOUND_OP.find(op_kind);
            if (it == COMPOUND_OP.end()) throw VesnaError("未知复合运算符 " + op_kind);
            v = binop(it->second, target.list()->items[(size_t)py], val);
        }
        target.list()->items[(size_t)py] = v;
    } else if (target.t() == Value::T::DICT) {
        Value v = val;
        if (op_kind != "ASSIGN") {
            auto it = COMPOUND_OP.find(op_kind);
            if (it == COMPOUND_OP.end()) throw VesnaError("未知复合运算符 " + op_kind);
            auto* old = dictFind(*target.dict(), idxv);
            Value oldv = old ? old->second : mkNone();
            v = binop(it->second, oldv, val);
        }
        dictSet(*target.dict(), idxv, v);
    } else {
        throw VesnaError("只能对列表或字典下标赋值");
    }
}

Value Interp::eval(const std::shared_ptr<Expr>& e, const std::shared_ptr<Env>& env) {
    switch (e->k) {
        case Expr::K::NUM:
            return e->is_float ? mkFloat(e->fnum) : mkInt(e->inum);
        case Expr::K::STR:
            return mkStr(e->str);
        case Expr::K::BOOL:
            return mkBool(e->bval);
        case Expr::K::NONE:
            return mkNone();
        case Expr::K::VAL:
            return e->val;
        case Expr::K::VAR: {
            const Value* v = env->getRef(e->nid);
            return v ? *v : mkStr(e->str);
        }
        case Expr::K::INTERP:
            return mkStr(interpStr(e->str, env));
        case Expr::K::NEG: {
            Value v = eval(e->a, env);
            if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("一元负号只能用于数字");
            return v.t() == Value::T::INT ? mkInt(-v.i()) : mkFloat(-v.f());
        }
        case Expr::K::NOT:
            return mkBool(!truthy(eval(e->a, env)));
        case Expr::K::AND: {
            if (!truthy(eval(e->a, env))) return mkBool(false);
            return mkBool(truthy(eval(e->b, env)));
        }
        case Expr::K::OR: {
            if (truthy(eval(e->a, env))) return mkBool(true);
            return mkBool(truthy(eval(e->b, env)));
        }
        case Expr::K::BIN:
            return binop(e->op, eval(e->a, env), eval(e->b, env));
        case Expr::K::GROUP: {
            Value v = mkGroup();
            for (auto& x : e->args) v.group()->items.push_back(eval(x, env));
            return v;
        }
        case Expr::K::LIST: {
            Value v = mkList();
            for (auto& x : e->args) v.list()->items.push_back(eval(x, env));
            return v;
        }
        case Expr::K::DICT: {
            Value v = mkDict();
            for (auto& [k, val] : e->pairs) dictSet(*v.dict(), eval(k, env), eval(val, env));
            return v;
        }
        case Expr::K::INDEX: {
            Value t = eval(e->a, env);
            Value i = eval(e->b, env);
            if (i.t() == Value::T::STR) {
                if (t.t() == Value::T::DICT) {
                    auto* p = dictFind(*t.dict(), i);
                    if (!p) throw VesnaError("键不存在: " + i.s());
                    return p->second;
                }
                throw VesnaError("键不存在: " + i.s());
            }
            if (i.t() == Value::T::FLOAT && i.f() == (double)(int64_t)i.f()) i = mkInt((int64_t)i.f());
            if (t.t() == Value::T::DICT) {
                auto* p = dictFind(*t.dict(), i);
                if (!p) throw VesnaError("键不存在: " + fmt(i));
                return p->second;
            }
            if (i.t() != Value::T::INT) throw VesnaError("下标必须是整数");
            int64_t idx = i.i();
            size_t size = 0;
            if (t.t() == Value::T::LIST) size = t.list()->items.size();
            else if (t.t() == Value::T::GROUP) size = t.group()->items.size();
            else if (t.t() == Value::T::STR) size = t.s().size();
            else throw VesnaError("无法索引: " + typeName(t));
            int64_t py = (idx > 0) ? idx - 1 : idx;
            if (py < 0) py += (int64_t)size;
            if (py < 0 || py >= (int64_t)size) throw VesnaError("下标越界: " + fmt(i));
            if (t.t() == Value::T::LIST) return t.list()->items[(size_t)py];
            if (t.t() == Value::T::GROUP) return t.group()->items[(size_t)py];
            return mkStr(std::string(1, t.s()[(size_t)py]));
        }
        case Expr::K::BUILTIN:
            return builtin(e->str, e->args, env);
        case Expr::K::INTO:
            return into(e->str, eval(e->a, env));
        case Expr::K::CALL:
            return call(e->str, e->nid, e->args, env);
    }
    throw VesnaError("未知表达式");
}

static bool valueLess(const Value& a, const Value& b) {
    if (isNum(a) && isNum(b)) {
        if (a.t() == Value::T::INT && b.t() == Value::T::INT) return a.i() < b.i();
        return numVal(a) < numVal(b);
    }
    if (a.t() != b.t()) throw VesnaError("无法比较: " + fmt(a) + " 和 " + fmt(b));
    switch (a.t()) {
        case Value::T::STR: return a.s() < b.s();
        case Value::T::BOOL: return a.b() < b.b();
        default: throw VesnaError("无法比较: " + fmt(a) + " 和 " + fmt(b));
    }
}

Value Interp::binop(const std::string& op, const Value& l, const Value& r) {
    if (op == "PLUS") {
        if (l.t() == Value::T::STR && r.t() == Value::T::STR) return mkStr(l.s() + r.s());
        if (l.t() == Value::T::BOOL || r.t() == Value::T::BOOL) throw VesnaError("布尔值不能相加");
        if (isNum(l) && isNum(r)) return numArith(l, r, '+');
        throw VesnaError("无法相加: " + fmt(l) + " + " + fmt(r));
    }
    if (op == "MINUS") { numOnly(l, r); return numArith(l, r, '-'); }
    if (op == "STAR") { numOnly(l, r); return numArith(l, r, '*'); }
    if (op == "SLASH") {
        numOnly(l, r);
        double b = numVal(r);
        if (b == 0) throw VesnaError("除数不能为 0");
        return mkFloat(numVal(l) / b);
    }
    if (op == "./") {  // 整除，截断向零
        numOnly(l, r);
        double b = numVal(r);
        if (b == 0) throw VesnaError("除数不能为 0");
        return mkInt((int64_t)(numVal(l) / b));
    }
    if (op == "/.") {  // 小数部分
        numOnly(l, r);
        double b = numVal(r);
        if (b == 0) throw VesnaError("除数不能为 0");
        double q = numVal(l) / b;
        return mkFloat(q - (double)(int64_t)q);
    }
    if (op == "/-") {  // 取模（Python 语义）
        numOnly(l, r);
        double b = numVal(r);
        if (b == 0) throw VesnaError("除数不能为 0");
        if (l.t() == Value::T::INT && r.t() == Value::T::INT) return mkInt(pyModInt(l.i(), r.i()));
        return mkFloat(pyModFloat(numVal(l), b));
    }
    if (op == "==") return mkBool(vesnaEq(l, r));
    if (op == "!=") return mkBool(!vesnaEq(l, r));
    if (op == "LT") { numOnlyCmp(l, r); return mkBool(valueLess(l, r)); }
    if (op == "GT") { numOnlyCmp(l, r); return mkBool(valueLess(r, l)); }
    if (op == "<=") { numOnlyCmp(l, r); return mkBool(!valueLess(r, l)); }
    if (op == ">=") { numOnlyCmp(l, r); return mkBool(!valueLess(l, r)); }
    throw VesnaError("未知运算符 " + op);
}

// ============================================================
// 线程表（并发内置）：结果/错误按 id 存取，析构时 join 全部线程
// ============================================================
struct ThreadTable {
    std::mutex m;
    std::unordered_map<int64_t, std::thread> threads;
    std::unordered_map<int64_t, Value> results;
    std::unordered_map<int64_t, std::string> errors;
    int64_t next_id = 1;
    ~ThreadTable() {
        for (auto& kv : threads)
            if (kv.second.joinable()) kv.second.join();
    }
};
static ThreadTable g_threads;
static std::mutex g_out_mutex;                 // print 输出锁
static std::unordered_map<std::string, std::mutex*> g_locks;   // 命名互斥锁
static std::mutex g_locks_m;
Value Interp::call(const std::string& name, int64_t nid, const std::vector<std::shared_ptr<Expr>>& args,
                   const std::shared_ptr<Env>& env) {
    if (name == "print") {
        std::string line;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i) line += ' ';
            line += fmt(eval(args[i], env));
        }
        {
            std::lock_guard<std::mutex> lk(g_out_mutex);
            std::cout << line << '\n';
        }
        return mkNone();
    }
    if (name == "input") {
        std::string p;
        if (!args.empty()) {
            Value pv = eval(args[0], env);
            p = pv.t() == Value::T::STR ? pv.s() : fmt(pv);
        }
        std::cout << p;
        std::cout.flush();
        std::string line;
        if (!std::getline(std::cin, line)) return mkStr("");
        return mkStr(line);
    }
    auto fn = env->getFunc(nid);
    if (args.size() > fn->params.size())
        throw VesnaError(name + " 最多 " + std::to_string(fn->params.size()) + " 个参数");
    auto closure = fn->closure.lock();
    if (!closure) closure = env;
    auto local = std::make_shared<Env>(closure);
    for (size_t i = 0; i < fn->params.size(); ++i) {
        const auto& pname = fn->params[i].first;
        const auto& pdefault = fn->params[i].second;
        if (i < args.size()) local->set(pname, eval(args[i], env));
        else if (pdefault) local->set(pname, eval(pdefault, closure));
        else throw VesnaError(name + " 缺少参数 " + internName(pname));
    }
    if (dbg) frames.emplace_back(fn->name, 0);
    try {
        for (auto& s : fn->body) exec(s, local);
    } catch (ReturnSignal& rs) {
        if (dbg) frames.pop_back();
        return rs.value;
    }
    if (dbg) frames.pop_back();
    return mkNone();
}

// 线程内置：参数已求值为 Value，直接按名调用函数（不含 print/input 特判）
Value Interp::callFuncByValues(const std::string& name, int64_t nid,
                               const std::vector<Value>& argv,
                               const std::shared_ptr<Env>& env) {
    auto fn = env->getFunc(nid);
    if (!fn) throw VesnaError("线程中找不到函数: " + name);
    if (argv.size() > fn->params.size())
        throw VesnaError(name + " 最多 " + std::to_string(fn->params.size()) + " 个参数");
    auto closure = fn->closure.lock();
    if (!closure) closure = env;
    auto local = std::make_shared<Env>(closure);
    for (size_t i = 0; i < fn->params.size(); ++i) {
        const auto& pname = fn->params[i].first;
        const auto& pdefault = fn->params[i].second;
        if (i < argv.size()) local->set(pname, argv[i]);
        else if (pdefault) local->set(pname, eval(pdefault, closure));
        else throw VesnaError(name + " 缺少参数 " + internName(pname));
    }
    try {
        for (auto& s : fn->body) exec(s, local);
    } catch (ReturnSignal& rs) {
        return rs.value;
    }
    return mkNone();
}

Value Interp::into(const std::string& t, const Value& v) {
    if (t == "int") {
        if (v.t() == Value::T::BOOL) return mkInt(v.b() ? 1 : 0);
        if (v.t() == Value::T::INT) return v;
        if (v.t() == Value::T::FLOAT) return mkInt((int64_t)v.f());
        if (v.t() == Value::T::STR) {
            try {
                std::string s = trimStr(v.s());
                if (s.find('.') != std::string::npos || s.find('e') != std::string::npos ||
                    s.find('E') != std::string::npos)
                    return mkInt((int64_t)std::stod(s));
                return mkInt(std::stoll(s));
            } catch (...) {
                throw VesnaError("无法转成 int: " + v.s());
            }
        }
        throw VesnaError("无法转成 int: " + fmt(v));
    }
    if (t == "str") {
        if (v.t() == Value::T::STR) return v;
        if (v.t() == Value::T::BOOL) return mkStr(v.b() ? "true" : "false");
        if (v.t() == Value::T::INT) return mkStr(std::to_string(v.i()));
        if (v.t() == Value::T::FLOAT) return mkStr(strFloat(v.f()));
        return mkStr(fmt(v));
    }
    if (t == "float") {
        if (v.t() == Value::T::BOOL) return mkFloat(v.b() ? 1.0 : 0.0);
        if (v.t() == Value::T::INT) return mkFloat((double)v.i());
        if (v.t() == Value::T::FLOAT) return v;
        if (v.t() == Value::T::STR) {
            try { return mkFloat(std::stod(v.s())); }
            catch (...) { throw VesnaError("无法转成 float: " + v.s()); }
        }
        throw VesnaError("无法转成 float: " + fmt(v));
    }
    if (t == "bool") return mkBool(truthy(v));
    if (t == "list") {
        if (v.t() == Value::T::LIST) return v;
        if (v.t() == Value::T::GROUP) { Value out = mkList(); out.list()->items = v.group()->items; return out; }
        if (v.t() == Value::T::STR) {
            Value out = mkList();
            for (char c : v.s()) out.list()->items.push_back(mkStr(std::string(1, c)));
            return out;
        }
        throw VesnaError("无法转成 list: " + fmt(v));
    }
    if (t == "dict") {
        if (v.t() == Value::T::DICT) return v;
        throw VesnaError("无法转成 dict: " + fmt(v));
    }
    throw VesnaError("未知类型 " + t);
}

std::string Interp::interpStr(const std::string& tpl, const std::shared_ptr<Env>& env) {
    std::string out;
    size_t i = 0;
    while (i < tpl.size()) {
        if (tpl[i] == '(') {
            size_t j = tpl.find(')', i + 1);
            if (j == std::string::npos) { out += tpl.substr(i); break; }
            std::string name = tpl.substr(i + 1, j - i - 1);
            const Value* vp = env->getRef(internId(name));
            Value v = vp ? *vp : mkStr(name);
            if (v.t() == Value::T::STR) out += v.s();
            else if (v.t() == Value::T::BOOL) out += v.b() ? "true" : "false";
            else if (v.t() == Value::T::INT) out += std::to_string(v.i());
            else if (v.t() == Value::T::FLOAT) out += floatToStr(v.f());
            else out += fmt(v);
            i = j + 1;
        } else {
            out += tpl[i];
            ++i;
        }
    }
    return out;
}

void Interp::doImport(const std::string& name, const std::shared_ptr<Env>& env) {
    std::string n = trimStr(name);
    auto loadOne = [&](const std::string& p) -> bool {
        if (!fileExists(p)) return false;
        std::string src = readFileUtf8(p);
        Parser sub_parser(preprocess(src), p);
        auto program = sub_parser.parse();
        if (!sub_parser.errors.empty()) {
            auto [line, msg] = sub_parser.errors[0];
            throw VesnaError(n + ".ves: " + msg, line);
        }
        auto sub = std::make_shared<Interp>(argv, parentDir(p));
        sub->g->parent = env;  // 包内函数可沿 parent 链查找导入方脚本的函数/变量（#call 动态调用）
        sub->run(program);
        for (auto& [k, v] : sub->g->vars) env->set(k, v);
        for (auto& [k, fn] : sub->g->funcs) env->setFunc(k, fn);
        kept_.push_back(sub->g);  // 保活被导入函数闭包引用的环境
        return true;
    };
    std::vector<std::string> paths = {
        script_dir + "\\" + n + ".ves",
        script_dir + "\\lib\\" + n + ".ves",
        findVesnaHome() + "\\lib\\" + n + ".ves",
        findVesnaHome() + "\\packages\\" + n + "\\" + n + ".ves",
    };
    for (const auto& p : paths) if (loadOne(p)) return;
    // vpm 包：读取 vesna-pkg.json 的 entry 字段（entry 可与包名不同，如 hello_vesna -> hello.ves）
    std::string pkg_meta = findVesnaHome() + "\\packages\\" + n + "\\vesna-pkg.json";
    if (fileExists(pkg_meta)) {
        std::string msrc = readFileUtf8(pkg_meta);
        std::smatch m;
        static const std::regex entry_re("\"entry\"[ \t]*:[ \t]*\"([^\"]+)\"");
        if (std::regex_search(msrc, m, entry_re)) {
            std::string entry = m[1].str();
            if (entry.find("..") == std::string::npos && entry.find(':') == std::string::npos) {
                if (loadOne(findVesnaHome() + "\\packages\\" + n + "\\" + entry)) return;
            }
        }
    }
    throw VesnaError("找不到模块: " + n);
}

// ============================================================
// 内置函数辅助工具
// ============================================================
static std::string parentDir(const std::string& path) {
    std::wstring wp = utf8ToWide(path);
    size_t pos = wp.find_last_of(L"/\\");
    if (pos == std::wstring::npos) return ".";
    if (pos == 0) return wideToUtf8(wp.substr(0, 1));
    return wideToUtf8(wp.substr(0, pos));
}

static std::string pathAbs(const std::string& path) {
    std::error_code ec;
    auto abs = std::filesystem::absolute(std::filesystem::u8path(path), ec);
    return ec ? path : abs.u8string();
}

static std::vector<Value> splitStr(const std::string& s, const std::string& sep) {
    std::vector<Value> out;
    size_t start = 0;
    while (true) {
        size_t pos = s.find(sep, start);
        if (pos == std::string::npos) { out.push_back(mkStr(s.substr(start))); break; }
        out.push_back(mkStr(s.substr(start, pos - start)));
        start = pos + sep.size();
    }
    return out;
}

static int64_t utf8FirstCodepoint(const std::string& s) {
    if (s.empty()) return 0;
    unsigned char c = (unsigned char)s[0];
    if (c < 0x80) return c;
    if ((c & 0xE0) == 0xC0 && s.size() >= 2)
        return ((c & 0x1F) << 6) | ((unsigned char)s[1] & 0x3F);
    if ((c & 0xF0) == 0xE0 && s.size() >= 3)
        return ((c & 0x0F) << 12) | (((unsigned char)s[1] & 0x3F) << 6) | ((unsigned char)s[2] & 0x3F);
    if ((c & 0xF8) == 0xF0 && s.size() >= 4)
        return ((c & 0x07) << 18) | (((unsigned char)s[1] & 0x3F) << 12) |
               (((unsigned char)s[2] & 0x3F) << 6) | ((unsigned char)s[3] & 0x3F);
    return c;
}

static std::string utf8FromCodepoint(int64_t cp) {
    if (cp < 0x80) return std::string(1, (char)cp);
    if (cp < 0x800) {
        std::string s(2, '\0');
        s[0] = (char)(0xC0 | (cp >> 6));
        s[1] = (char)(0x80 | (cp & 0x3F));
        return s;
    }
    if (cp < 0x10000) {
        std::string s(3, '\0');
        s[0] = (char)(0xE0 | (cp >> 12));
        s[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        s[2] = (char)(0x80 | (cp & 0x3F));
        return s;
    }
    std::string s(4, '\0');
    s[0] = (char)(0xF0 | (cp >> 18));
    s[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    s[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    s[3] = (char)(0x80 | (cp & 0x3F));
    return s;
}

// Python str(float)：最短表示，整数补 .0
static std::string strFloat(double f) {
    char buf[64];
    auto res = std::to_chars(buf, buf + sizeof(buf), f);
    std::string s(buf, res.ptr);
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos &&
        s.find('E') == std::string::npos && s.find('n') == std::string::npos &&
        s.find("inf") == std::string::npos)
        s += ".0";
    return s;
}

// Python round：四舍六入五取偶
static int64_t pyRound(double x) {
    double f = std::floor(x);
    double d = x - f;
    if (d > 0.5) return (int64_t)f + 1;
    if (d < 0.5) return (int64_t)f;
    return ((int64_t)f % 2 == 0) ? (int64_t)f : (int64_t)f + 1;
}

static std::string titleAscii(const std::string& s) {
    std::string out = s;
    bool prevAlpha = false;
    for (auto& c : out) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            if (!prevAlpha) { if (c >= 'a' && c <= 'z') c -= 32; }
            else { if (c >= 'A' && c <= 'Z') c += 32; }
            prevAlpha = true;
        } else {
            prevAlpha = false;
        }
    }
    return out;
}

static std::string capitalizeAscii(const std::string& s) {
    std::string out = s;
    for (auto& c : out) {
        if (c >= 'A' && c <= 'Z') c += 32;
    }
    if (!out.empty() && out[0] >= 'a' && out[0] <= 'z') out[0] -= 32;
    return out;
}

// ============================================================
// 0.4 通用语言扩充：编码 / 哈希 / 随机 / 格式化
// ============================================================
static std::string base64EncodeStr(const std::string& in) {
    static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((in.size() + 2) / 3 * 4);
    size_t i = 0;
    while (i + 3 <= in.size()) {
        unsigned v = ((unsigned char)in[i] << 16) | ((unsigned char)in[i + 1] << 8) | (unsigned char)in[i + 2];
        out += tbl[(v >> 18) & 63]; out += tbl[(v >> 12) & 63];
        out += tbl[(v >> 6) & 63]; out += tbl[v & 63];
        i += 3;
    }
    if (i + 1 == in.size()) {
        unsigned v = (unsigned char)in[i] << 16;
        out += tbl[(v >> 18) & 63]; out += tbl[(v >> 12) & 63]; out += "==";
    } else if (i + 2 == in.size()) {
        unsigned v = ((unsigned char)in[i] << 16) | ((unsigned char)in[i + 1] << 8);
        out += tbl[(v >> 18) & 63]; out += tbl[(v >> 12) & 63]; out += tbl[(v >> 6) & 63]; out += '=';
    }
    return out;
}

static int b64ValOf(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static std::string base64DecodeStr(const std::string& in) {
    std::string out;
    int acc = 0, bits = 0;
    for (char c : in) {
        if (c == '=' || c == '\r' || c == '\n') continue;
        int v = b64ValOf(c);
        if (v < 0) throw VesnaError("-base64_decode 无效字符");
        acc = (acc << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out += (char)((acc >> bits) & 0xFF);
        }
    }
    return out;
}

static std::string urlEncodeStr(const std::string& s) {
    static const char* hexd = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : s) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            out += (char)c;
        } else if (c == ' ') {
            out += '+';
        } else {
            out += '%';
            out += hexd[c >> 4];
            out += hexd[c & 15];
        }
    }
    return out;
}

static std::string urlDecodeStr(const std::string& s) {
    std::string out;
    auto hv = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int hi = hv(s[i + 1]), lo = hv(s[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out += (char)((hi << 4) | lo);
                i += 2;
                continue;
            }
        }
        if (s[i] == '+') out += ' ';
        else out += s[i];
    }
    return out;
}

// FNV-1a 64（确定性哈希）
static std::string fnv1a64Str(const std::string& s) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (unsigned char c : s) {
        h ^= c;
        h *= 0x100000001b3ULL;
    }
    return std::to_string(h);
}


static std::mt19937_64& rngGen() {
    thread_local static std::mt19937_64 g(std::random_device{}());
    return g;
}

// %s %d %f %% 格式化（%s 输出 Vesna 裸值风格）

static std::string formatStr(const std::string& fstr, const std::vector<Value>& vals) {
    std::string out;
    size_t vi = 0;
    for (size_t i = 0; i < fstr.size(); ++i) {
        if (fstr[i] == '%' && i + 1 < fstr.size()) {
            size_t j = i + 1;
            int prec = -1;
            if (fstr[j] == '.') {
                size_t k = j + 1;
                int p = 0;
                while (k < fstr.size() && fstr[k] >= '0' && fstr[k] <= '9') { p = p * 10 + (fstr[k] - '0'); ++k; }
                prec = p;
                j = k;
            }
            if (j < fstr.size() && fstr[j] == '%') { out += '%'; i = j; continue; }
            if (j >= fstr.size()) { out += fstr.substr(i); break; }
            if (vi >= vals.size()) throw VesnaError("-format 参数不足");
            const Value& v = vals[vi++];
            if (fstr[j] == 's') {
                if (v.t() == Value::T::STR) out += v.s();
                else if (v.t() == Value::T::BOOL) out += v.b() ? "true" : "false";
                else if (v.t() == Value::T::INT) out += std::to_string(v.i());
                else if (v.t() == Value::T::FLOAT) out += floatToStr(v.f());
                else out += fmt(v);
            } else if (fstr[j] == 'd') {
                if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-format %d 需要数字");
                out += std::to_string((int64_t)numVal(v));
            } else if (fstr[j] == 'f') {
                if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-format %f 需要数字");
                char buf[64];
                if (prec >= 0) {
                    char fmtbuf[16];
                    snprintf(fmtbuf, sizeof(fmtbuf), "%%.%df", prec);
                    snprintf(buf, sizeof(buf), fmtbuf, numVal(v));
                } else {
                    snprintf(buf, sizeof(buf), "%.6f", numVal(v));
                }
                out += buf;
            } else {
                out += '%';
                if (prec >= 0) { out += '.'; out += std::to_string(prec); }
                out += fstr[j];
            }
            i = j;
        } else {
            out += fstr[i];
        }
    }
    return out;
}
// 十进制转 2/8/16 进制（无前缀、负数带 -、小写）
static std::string intToBaseStr(int64_t n, int base) {
    if (n == 0) return "0";
    bool neg = n < 0;
    uint64_t u = neg ? (uint64_t)(-(n + 1)) + 1 : (uint64_t)n;
    static const char* d = "0123456789abcdef";
    std::string s;
    while (u) { s += d[u % (uint64_t)base]; u /= (uint64_t)base; }
    if (neg) s += '-';
    std::reverse(s.begin(), s.end());
    return s;
}

// 判断模式是否为"纯字面量"（无正则元字符）——是则可用字符串查找代替 regex
static bool isRegexSafeLiteral(const std::string& p) {
    for (unsigned char c : p) {
        if (c >= 0x80) continue;  // 中文等多字节字符
        if (std::isalnum(c)) continue;
        if (c == '_' || c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        if (std::string(",-/:;@#%&=~!'\"<>").find((char)c) != std::string::npos) continue;
        return false;  // 正则元字符（.*+?()[]{}|^$\ 等）
    }
    return true;
}

// 正则缓存：避免同一模式反复编译（grep/findall 类任务的主要开销）
static std::regex& cachedRegex(const std::string& pat) {
    static std::unordered_map<std::string, std::regex> cache;
    auto it = cache.find(pat);
    if (it != cache.end()) return it->second;
    if (cache.size() > 256) cache.clear();
    auto res = cache.emplace(pat, std::regex(pat));  // 无效正则抛 regex_error
    return res.first->second;
}

// ============================================================
// 第三梯队辅助：JSON / 进程
// ============================================================
static void appendUtf8(std::string& out, unsigned cp) {
    if (cp < 0x80) out += (char)cp;
    else if (cp < 0x800) { out += (char)(0xC0 | (cp >> 6)); out += (char)(0x80 | (cp & 0x3F)); }
    else if (cp < 0x10000) { out += (char)(0xE0 | (cp >> 12)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); out += (char)(0x80 | (cp & 0x3F)); }
    else { out += (char)(0xF0 | (cp >> 18)); out += (char)(0x80 | ((cp >> 12) & 0x3F)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); out += (char)(0x80 | (cp & 0x3F)); }
}

static std::string jsonEscape(const std::string& s) {
    std::string out = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else out += (char)c;
        }
    }
    return out + "\"";
}

static std::string jsonWrite(const Value& v) {
    switch (v.t()) {
        case Value::T::NONE: return "null";
        case Value::T::BOOL: return v.b() ? "true" : "false";
        case Value::T::INT: return std::to_string(v.i());
        case Value::T::FLOAT: {
            std::string n = floatToStr(v.f());
            if (n.find('.') == std::string::npos && n.find('e') == std::string::npos &&
                n.find('E') == std::string::npos) n += ".0";
            return n;
        }
        case Value::T::STR: return jsonEscape(v.s());
        case Value::T::LIST: case Value::T::GROUP: {
            const auto& items = v.t() == Value::T::LIST ? v.list()->items : v.group()->items;
            std::string out = "[";
            for (size_t i = 0; i < items.size(); ++i) {
                if (i) out += ",";
                out += jsonWrite(items[i]);
            }
            return out + "]";
        }
        case Value::T::DICT: {
            std::string out = "{";
            for (size_t i = 0; i < v.dict()->pairs.size(); ++i) {
                if (i) out += ",";
                const auto& kv = v.dict()->pairs[i];
                std::string key = kv.first.t() == Value::T::STR ? kv.first.s() : fmt(kv.first);
                out += jsonEscape(key) + ":" + jsonWrite(kv.second);
            }
            return out + "}";
        }
    }
    return "null";
}

struct JsonParser {
    const std::string& s;
    size_t pos = 0;
    bool fail = false;
    explicit JsonParser(const std::string& str) : s(str) {}
    void ws() {
        while (pos < s.size() && (s[pos]==' '||s[pos]=='\t'||s[pos]=='\n'||s[pos]=='\r')) ++pos;
    }
    Value parseValue() {
        ws();
        if (pos >= s.size()) { fail = true; return mkNone(); }
        char c = s[pos];
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') return mkStr(parseString());
        if (c == 't') { if (s.compare(pos, 4, "true") == 0) { pos += 4; return mkBool(true); } fail = true; return mkNone(); }
        if (c == 'f') { if (s.compare(pos, 5, "false") == 0) { pos += 5; return mkBool(false); } fail = true; return mkNone(); }
        if (c == 'n') { if (s.compare(pos, 4, "null") == 0) { pos += 4; return mkNone(); } fail = true; return mkNone(); }
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();
        fail = true;
        return mkNone();
    }
    std::string parseString() {
        ++pos;
        std::string out;
        while (pos < s.size() && s[pos] != '"') {
            if (s[pos] == '\\' && pos + 1 < s.size()) {
                char e = s[pos + 1];
                pos += 2;
                switch (e) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case 'u': {
                        if (pos + 4 <= s.size()) {
                            unsigned cp = 0;
                            bool ok = true;
                            for (int k = 0; k < 4; ++k) {
                                char h = s[pos + k];
                                cp <<= 4;
                                if (h >= '0' && h <= '9') cp |= h - '0';
                                else if (h >= 'a' && h <= 'f') cp |= h - 'a' + 10;
                                else if (h >= 'A' && h <= 'F') cp |= h - 'A' + 10;
                                else { ok = false; break; }
                            }
                            pos += 4;
                            if (ok) {
                                if (cp >= 0xD800 && cp <= 0xDBFF && pos + 1 < s.size() &&
                                    s[pos] == '\\' && s[pos + 1] == 'u') {
                                    unsigned lo = 0;
                                    bool ok2 = true;
                                    for (int k = 0; k < 4; ++k) {
                                        char h = s[pos + 2 + k];
                                        lo <<= 4;
                                        if (h >= '0' && h <= '9') lo |= h - '0';
                                        else if (h >= 'a' && h <= 'f') lo |= h - 'a' + 10;
                                        else if (h >= 'A' && h <= 'F') lo |= h - 'A' + 10;
                                        else { ok2 = false; break; }
                                    }
                                    if (ok2 && lo >= 0xDC00 && lo <= 0xDFFF) {
                                        pos += 6;
                                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                                    }
                                }
                                appendUtf8(out, cp);
                            }
                        }
                        break;
                    }
                    default: out += e; break;
                }
            } else {
                out += s[pos];
                ++pos;
            }
        }
        if (pos < s.size()) ++pos;
        return out;
    }
    Value parseNumber() {
        size_t start = pos;
        if (pos < s.size() && s[pos] == '-') ++pos;
        while (pos < s.size() && (std::isdigit((unsigned char)s[pos]) || s[pos]=='.' ||
               s[pos]=='e' || s[pos]=='E' || s[pos]=='+' || s[pos]=='-')) ++pos;
        std::string num = s.substr(start, pos - start);
        if (num.find_first_of(".eE") != std::string::npos) {
            try { return mkFloat(std::stod(num)); } catch (...) { fail = true; return mkNone(); }
        }
        try { return mkInt(std::stoll(num)); }
        catch (...) {
            try { return mkFloat(std::stod(num)); } catch (...) { fail = true; return mkNone(); }
        }
    }
    Value parseArray() {
        ++pos;
        Value out = mkList();
        ws();
        if (pos < s.size() && s[pos] == ']') { ++pos; return out; }
        while (pos < s.size()) {
            Value v = parseValue();
            if (fail) return mkNone();
            out.list()->items.push_back(v);
            ws();
            if (pos < s.size() && s[pos] == ',') { ++pos; continue; }
            if (pos < s.size() && s[pos] == ']') { ++pos; break; }
            fail = true;
            return mkNone();
        }
        return out;
    }
    Value parseObject() {
        ++pos;
        Value out = mkDict();
        ws();
        if (pos < s.size() && s[pos] == '}') { ++pos; return out; }
        while (pos < s.size()) {
            ws();
            if (pos >= s.size() || s[pos] != '"') { fail = true; return mkNone(); }
            std::string key = parseString();
            ws();
            if (pos >= s.size() || s[pos] != ':') { fail = true; return mkNone(); }
            ++pos;
            Value v = parseValue();
            if (fail) return mkNone();
            out.dict()->pairs.emplace_back(mkStr(key), v);
            ws();
            if (pos < s.size() && s[pos] == ',') { ++pos; continue; }
            if (pos < s.size() && s[pos] == '}') { ++pos; break; }
            fail = true;
            return mkNone();
        }
        return out;
    }
};

static std::pair<int, std::string> procRun(const std::string& cmd) {
#ifdef _WIN32
    FILE* p = _popen(cmd.c_str(), "r");
#else
    FILE* p = popen(cmd.c_str(), "r");
#endif
    if (!p) return { -1, "" };
    std::string out;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), p)) > 0) out.append(buf, n);
#ifdef _WIN32
    int rc = _pclose(p);
#else
    int rc = pclose(p);
    if (rc != -1) rc = WEXITSTATUS(rc);
#endif
    return { rc, out };
}

// 内置函数注册表（名字 -> dispatch id）—— LSP 补全与 builtin() 共用
const std::vector<std::pair<std::string, int>> g_builtinNames = {
    {"up",1},{"down",2},{"len",3},{"sub",4},{"split",5},{"join",6},{"find",7},{"replace",8},{"append",9},{"pop",10},{"keys",11},{"values",12},{"type",13},{"args",14},{"fread",15},{"fwrite",16},{"fappend",17},{"fexists",18},{"exit",19},{"f",20},{"trim",21},{"startswith",22},{"endswith",23},{"lines",24},{"repeat",25},{"has_key",26},{"str",27},{"int",28},{"float",29},{"bool",30},{"char_at",31},{"sort",32},{"reverse",33},{"slice",34},{"map",35},{"filter",36},{"reduce",37},{"match",38},{"search",39},{"findall",40},{"gsub",41},{"ls",42},{"glob",43},{"stdin",44},{"ord",45},{"chr",46},{"is_digit",47},{"is_alpha",48},{"is_alnum",49},{"is_space",50},{"lstrip",51},{"rstrip",52},{"title",53},{"capitalize",54},{"count",55},{"rfind",56},{"min",57},{"max",58},{"sum",59},{"abs",60},{"round",61},{"pow",62},{"contains",63},{"mkdir",64},{"copy",65},{"rmdir",66},{"rename",67},{"getenv",68},{"setenv",69},{"cwd",70},{"chdir",71},{"regwrite",72},{"regdelete",73},{"shell",74},{"path_clean",75},{"regenv",146},{"cpdir",147},{"sqrt",76},{"floor",77},{"ceil",78},{"exp",79},{"log",80},{"log10",81},{"sin",82},{"cos",83},{"tan",84},{"sign",85},{"clamp",86},{"rand",87},{"randint",88},{"choice",89},{"shuffle",145},{"hex",90},{"bin",91},{"oct",92},{"pad",93},{"lpad",94},{"rpad",95},{"format",96},{"hash",97},{"range",98},{"first",99},{"last",100},{"take",101},{"drop",102},{"set",103},{"flatten",104},{"zip",105},{"insert",106},{"remove",107},{"index_of",108},{"enumerate",109},{"concat",110},{"get",111},{"items",112},{"pop_key",113},{"is_str",114},{"is_int",115},{"is_float",116},{"is_bool",117},{"is_list",118},{"is_dict",119},{"is_none",120},{"is_group",121},{"now",122},{"date",123},{"sleep",124},{"ticks",125},{"platform",126},{"temp_dir",127},{"fremove",128},{"fmove",129},{"fsize",130},{"is_dir",131},{"is_file",132},{"mkdirs",133},{"base64_encode",134},{"base64_decode",135},{"url_encode",136},{"url_decode",137},{"each",138},{"all",139},{"any",140},{"find_first",141},{"sort_by",142},{"throw",143},{"assert",144},{"thread",148},{"thread_join",149},{"thread_count",150},{"lock",151},{"unlock",152},{"http_get",153},{"http_post",154},{"tcp_ping",155},{"bin_read",156},{"bin_write",157},{"bin_hex",158},{"bin_unhex",159},{"bin_base64_encode",160},{"bin_base64_decode",161},{"json_encode",162},{"json_decode",163},{"re_groups",164},{"sha256",165},{"aes_encrypt",166},{"aes_decrypt",167},{"proc_run",168},{"ffi_call",169},
    {"csv_parse",170},{"csv_build",171},{"ini_read",172},{"ini_write",173},
    {"xml_parse",174},{"ffi_call_s",175},{"call",176},
    {"date_format",177},{"parse_time",178},{"uuid",179},{"http_server",180},{"file_time",181},{"truncate",182},{"arch",183}
};


// ============================================================
// 第四梯队 helper：CSV / INI / XML（纯 C++ 自研，无第三方依赖）
// ============================================================

static std::vector<std::vector<std::string>> csvParse(const std::string& s) {
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> row;
    std::string field;
    bool inQ = false;
    size_t i = 0, n = s.size();
    while (i < n) {
        char c = s[i];
        if (inQ) {
            if (c == '"') {
                if (i + 1 < n && s[i + 1] == '"') { field += '"'; i += 2; }
                else { inQ = false; ++i; }
            } else { field += c; ++i; }
        } else if (c == '"') { inQ = true; ++i; }
        else if (c == ',') { row.push_back(field); field.clear(); ++i; }
        else if (c == '\n' || c == '\r') {
            if (c == '\r' && i + 1 < n && s[i + 1] == '\n') ++i;
            row.push_back(field); field.clear();
            rows.push_back(row); row.clear();
            ++i;
        } else { field += c; ++i; }
    }
    if (!field.empty() || !row.empty()) { row.push_back(field); rows.push_back(row); }
    return rows;
}

static std::string csvBuild(const std::vector<std::vector<std::string>>& rows) {
    std::string out;
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i) out += ',';
            const std::string& f = row[i];
            if (f.find_first_of(",\"\"\n\r") != std::string::npos) {
                out += '"';
                for (char c : f) { if (c == '"') out += "\"\""; else out += c; }
                out += '"';
            } else out += f;
        }
        out += '\n';
    }
    return out;
}

static std::map<std::string, std::map<std::string, std::string>> iniParse(const std::string& s) {
    std::map<std::string, std::map<std::string, std::string>> out;
    std::string section;
    size_t i = 0, n = s.size();
    std::string line;
    auto flushLine = [&]() {
        size_t b = line.find_first_not_of(" \t\r");
        if (b == std::string::npos) return;
        size_t e = line.find_last_not_of(" \t\r");
        std::string l = line.substr(b, e - b + 1);
        if (l.empty() || l[0] == ';' || l[0] == '#') return;
        if (!l.empty() && l[0] == '[') {
            size_t rb = l.find(']');
            if (rb != std::string::npos) section = l.substr(1, rb - 1);
            return;
        }
        size_t eq = l.find('=');
        if (eq == std::string::npos) return;
        std::string k = l.substr(0, eq);
        std::string v = l.substr(eq + 1);
        size_t kb = k.find_first_not_of(" \t"); k = k.substr(kb == std::string::npos ? 0 : kb);
        size_t ke = k.find_last_not_of(" \t"); if (ke != std::string::npos) k = k.substr(0, ke + 1);
        size_t vb = v.find_first_not_of(" \t"); if (vb != std::string::npos) v = v.substr(vb);
        size_t ve = v.find_last_not_of(" \t"); if (ve != std::string::npos) v = v.substr(0, ve + 1);
        out[section][k] = v;
    };
    while (i < n) {
        char c = s[i];
        if (c == '\n' || c == '\r') {
            if (c == '\r' && i + 1 < n && s[i + 1] == '\n') ++i;
            flushLine();
            line.clear();
        } else line += c;
        ++i;
    }
    flushLine();
    return out;
}

static std::string iniBuild(const std::map<std::string, std::map<std::string, std::string>>& data) {
    std::string out;
    for (const auto& [sec, kv] : data) {
        out += "[" + sec + "]\n";
        for (const auto& [k, v] : kv) out += k + "=" + v + "\n";
        out += "\n";
    }
    return out;
}

struct XmlNode {
    std::string tag;
    std::map<std::string, std::string> attrs;
    std::vector<XmlNode> children;
    std::string text;
};

static void xmlUnescape(std::string& s) {
    std::string r;
    size_t i = 0, n = s.size();
    while (i < n) {
        if (s[i] == '&' && i + 3 < n && s[i + 3] == ';') {
            std::string ent = s.substr(i + 1, 2);
            if (ent == "lt") { r += '<'; i += 4; continue; }
            if (ent == "gt") { r += '>'; i += 4; continue; }
        }
        if (s[i] == '&' && i + 4 < n && s[i + 4] == ';') {
            std::string ent = s.substr(i + 1, 3);
            if (ent == "amp") { r += '&'; i += 5; continue; }
            if (ent == "quot") { r += '"'; i += 5; continue; }
        }
        if (s[i] == '&' && i + 5 < n && s[i + 5] == ';') {
            std::string ent = s.substr(i + 1, 4);
            if (ent == "apos") { r += '\''; i += 6; continue; }
        }
        r += s[i++];
    }
    s = r;
}

static bool xmlSkipWs(const std::string& s, size_t& i, size_t n) {
    while (i < n && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) ++i;
    return i < n;
}

// 递归解析一个元素（含子元素与文本）。返回 false 表示输入非法。
static bool xmlParseNode(const std::string& s, size_t& i, size_t n, XmlNode& node) {
    if (!xmlSkipWs(s, i, n) || s[i] != '<') return false;
    ++i;  // '<'
    if (i < n && s[i] == '/') return false;  // 意外的闭合标签
    size_t tagStart = i;
    while (i < n && s[i] != '>' && s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '/' && s[i] != '\r') ++i;
    if (i >= n || i == tagStart) return false;
    node.tag = s.substr(tagStart, i - tagStart);

    bool selfClose = false;
    // 属性
    while (true) {
        xmlSkipWs(s, i, n);
        if (i >= n) return false;
        if (s[i] == '>') { ++i; break; }
        if (s[i] == '/') {
            if (i + 1 < n && s[i + 1] == '>') { selfClose = true; i += 2; break; }
            return false;
        }
        size_t kb = i;
        while (i < n && s[i] != '=' && s[i] != ' ' && s[i] != '\t') ++i;
        if (i >= n) return false;
        std::string k = s.substr(kb, i - kb);
        xmlSkipWs(s, i, n);
        if (i >= n || s[i] != '=') return false;
        ++i;  // '='
        xmlSkipWs(s, i, n);
        if (i >= n || (s[i] != '"' && s[i] != '\'')) return false;
        char q = s[i];
        ++i;
        size_t vb = i;
        while (i < n && s[i] != q) ++i;
        if (i >= n) return false;
        std::string v = s.substr(vb, i - vb);
        ++i;  // 闭合引号
        xmlUnescape(v);
        node.attrs[k] = v;
    }
    if (selfClose) return true;

    // 内容：文本与子元素
    while (i < n) {
        size_t lt = s.find('<', i);
        if (lt == std::string::npos) { node.text += s.substr(i); i = n; break; }
        std::string t = s.substr(i, lt - i);
        xmlUnescape(t);
        node.text += t;
        i = lt;
        if (i + 1 < n && s[i + 1] == '/') {
            // 闭合标签
            ++i; ++i;
            size_t cStart = i;
            while (i < n && s[i] != '>') ++i;
            if (i >= n) return false;
            std::string closeTag = s.substr(cStart, i - cStart);
            ++i;
            return closeTag == node.tag;
        }
        XmlNode child;
        if (!xmlParseNode(s, i, n, child)) return false;
        node.children.push_back(std::move(child));
    }
    return true;
}

static bool xmlParse(const std::string& s, XmlNode& root) {
    size_t i = 0;
    size_t n = s.size();
    if (!xmlParseNode(s, i, n, root)) return false;
    return true;
}

// ============================================================
// 内置函数
// ============================================================
Value Interp::builtin(const std::string& name, const std::vector<std::shared_ptr<Expr>>& args,
                      const std::shared_ptr<Env>& env) {
    auto ev = [&](size_t i) -> Value { return eval(args[i], env); };
    auto argc = [&]() -> size_t { return args.size(); };
    static const std::unordered_map<std::string, int> g_bi = []{
        std::unordered_map<std::string, int> m;
        for (const auto& p : g_builtinNames) m[p.first] = p.second;
        return m;
    }();
    auto it = g_bi.find(name);
    if (it == g_bi.end()) throw VesnaError("未知内置 -" + name);
    switch (it->second) {

    case 1: {
        Value v = ev(0);
        if (v.t() != Value::T::STR) throw VesnaError("-up 需要字符串");
        return mkStr(toUpperAscii(v.s()));
    }
    case 2: {
        Value v = ev(0);
        if (v.t() != Value::T::STR) throw VesnaError("-down 需要字符串");
        return mkStr(toLowerAscii(v.s()));
    }
    case 3: {
        Value v = ev(0);
        switch (v.t()) {
            case Value::T::STR: return mkInt((int64_t)v.s().size());
            case Value::T::LIST: return mkInt((int64_t)v.list()->items.size());
            case Value::T::GROUP: return mkInt((int64_t)v.group()->items.size());
            case Value::T::DICT: return mkInt((int64_t)v.dict()->pairs.size());
            default: throw VesnaError("-len 需要字符串/列表/组/字典");
        }
    }
    case 4: {
        Value s = ev(0), st = ev(1), en = ev(2);
        if (s.t() != Value::T::STR) throw VesnaError("-sub 第一个参数需要字符串");
        if (st.t() != Value::T::INT || en.t() != Value::T::INT) throw VesnaError("-sub 下标需要整数");
        int64_t a = st.i() - 1, b = en.i();
        if (a < 0) a = 0;
        if (a > (int64_t)s.s().size()) a = (int64_t)s.s().size();
        if (b < a) b = a;
        if (b > (int64_t)s.s().size()) b = (int64_t)s.s().size();
        return mkStr(s.s().substr((size_t)a, (size_t)(b - a)));
    }
    case 5: {
        Value s = ev(0);
        std::string sep = " ";
        if (argc() > 1) {
            Value se = ev(1);
            if (se.t() != Value::T::STR) throw VesnaError("-split 第二个参数需要字符串");
            sep = se.s();
        }
        if (s.t() != Value::T::STR) throw VesnaError("-split 第一个参数需要字符串");
        if (sep.empty()) throw VesnaError("-split 分隔符不能为空");
        Value out = mkList();
        out.list()->items = splitStr(s.s(), sep);
        return out;
    }
    case 6: {
        Value lst = ev(0);
        std::string sep;
        if (argc() > 1) {
            Value se = ev(1);
            if (se.t() != Value::T::STR) throw VesnaError("-join 第二个参数需要字符串");
            sep = se.s();
        }
        if (lst.t() != Value::T::LIST && lst.t() != Value::T::GROUP)
            throw VesnaError("-join 第一个参数需要列表/组");
        const auto& items = lst.t() == Value::T::LIST ? lst.list()->items : lst.group()->items;
        std::string out;
        for (size_t i = 0; i < items.size(); ++i) {
            if (i) out += sep;
            out += items[i].t() == Value::T::STR ? items[i].s() : fmt(items[i]);
        }
        return mkStr(out);
    }
    case 7: {
        Value s = ev(0), sub = ev(1);
        if (s.t() != Value::T::STR) throw VesnaError("-find 需要字符串");
        size_t i = s.s().find(sub.s());
        return mkInt(i == std::string::npos ? 0 : (int64_t)i + 1);
    }
    case 8: {
        Value s = ev(0), oldv = ev(1), newv = ev(2);
        if (s.t() != Value::T::STR) throw VesnaError("-replace 第一个参数需要字符串");
        std::string out = s.s();
        size_t pos = 0;
        while ((pos = out.find(oldv.s(), pos)) != std::string::npos) {
            out.replace(pos, oldv.s().size(), newv.s());
            pos += newv.s().size();
        }
        return mkStr(out);
    }
    case 9: {
        Value lst = ev(0), v = ev(1);
        if (lst.t() != Value::T::LIST) throw VesnaError("-append 第一个参数需要列表");
        lst.list()->items.push_back(v);
        return lst;
    }
    case 10: {
        Value lst = ev(0);
        if (lst.t() != Value::T::LIST) throw VesnaError("-pop 需要列表");
        if (lst.list()->items.empty()) throw VesnaError("-pop 空列表");
        Value v = lst.list()->items.back();
        lst.list()->items.pop_back();
        return v;
    }
    case 11: {
        Value d = ev(0);
        if (d.t() != Value::T::DICT) throw VesnaError("-keys 需要字典");
        Value out = mkList();
        for (auto& p : d.dict()->pairs) out.list()->items.push_back(p.first);
        return out;
    }
    case 12: {
        Value d = ev(0);
        if (d.t() != Value::T::DICT) throw VesnaError("-values 需要字典");
        Value out = mkList();
        for (auto& p : d.dict()->pairs) out.list()->items.push_back(p.second);
        return out;
    }
    case 13: {
        return mkStr(typeName(ev(0)));
    }
    case 14: {
        Value out = mkList();
        for (auto& a : argv) out.list()->items.push_back(mkStr(a));
        return out;
    }
    case 15: {
        Value p = ev(0);
        if (p.t() != Value::T::STR) throw VesnaError("-fread 需要字符串");
        return mkStr(readFileUtf8(p.s()));
    }
    case 16: {
        Value p = ev(0), c = ev(1);
        if (p.t() != Value::T::STR) throw VesnaError("-fwrite 第一个参数需要字符串");
        writeFileUtf8(p.s(), c.t() == Value::T::STR ? c.s() : fmt(c), false);
        return mkNone();
    }
    case 17: {
        Value p = ev(0), c = ev(1);
        if (p.t() != Value::T::STR) throw VesnaError("-fappend 第一个参数需要字符串");
        writeFileUtf8(p.s(), c.t() == Value::T::STR ? c.s() : fmt(c), true);
        return mkNone();
    }
    case 18: {
        Value p = ev(0);
        return mkBool(fileExists(p.t() == Value::T::STR ? p.s() : fmt(p)));
    }
    case 19: {
        int code = 0;
        if (argc() > 0) {
            Value c = ev(0);
            code = (int)(c.t() == Value::T::INT ? c.i() : (int64_t)numVal(c));
        }
        throw ExitSignal(code);
    }
    case 20: {
        throw VesnaError("-f 必须紧接字符串: -f\"...\"");
    }

    // ---- 字符串 ----
    case 21: {
        Value v = ev(0);
        if (v.t() != Value::T::STR) throw VesnaError("-trim 需要字符串");
        return mkStr(trimStr(v.s()));
    }
    case 22: {
        Value s = ev(0), p = ev(1);
        if (s.t() != Value::T::STR) throw VesnaError("-startswith 第一个参数需要字符串");
        return mkBool(s.s().rfind(p.s(), 0) == 0);
    }
    case 23: {
        Value s = ev(0), p = ev(1);
        if (s.t() != Value::T::STR) throw VesnaError("-endswith 第一个参数需要字符串");
        return mkBool(s.s().size() >= p.s().size() &&
                      s.s().compare(s.s().size() - p.s().size(), p.s().size(), p.s()) == 0);
    }
    case 24: {
        Value v = ev(0);
        if (v.t() != Value::T::STR) throw VesnaError("-lines 需要字符串");
        Value out = mkList();
        std::string cur;
        for (size_t i = 0; i < v.s().size(); ++i) {
            char c = v.s()[i];
            if (c == '\n') { out.list()->items.push_back(mkStr(cur)); cur.clear(); }
            else if (c == '\r') {
                out.list()->items.push_back(mkStr(cur)); cur.clear();
                if (i + 1 < v.s().size() && v.s()[i + 1] == '\n') ++i;
            }
            else cur += c;
        }
        if (!cur.empty() || v.s().empty()) out.list()->items.push_back(mkStr(cur));
        return out;
    }
    case 25: {
        Value s = ev(0), n = ev(1);
        if (s.t() != Value::T::STR) throw VesnaError("-repeat 第一个参数需要字符串");
        int64_t cnt = n.t() == Value::T::INT ? n.i() : (int64_t)numVal(n);
        std::string out;
        out.reserve(s.s().size() * cnt);
        for (int64_t i = 0; i < cnt; ++i) out += s.s();
        return mkStr(out);
    }
    case 26: {
        Value d = ev(0), k = ev(1);
        if (d.t() != Value::T::DICT) throw VesnaError("-has_key 第一个参数需要字典");
        return mkBool(dictFind(*d.dict(), k) != nullptr);
    }
    case 27: return into("str", ev(0));
    case 28: return into("int", ev(0));
    case 29: return into("float", ev(0));
    case 30: return into("bool", ev(0));
    case 31: {
        Value s = ev(0), i = ev(1);
        if (s.t() != Value::T::STR) throw VesnaError("-char_at 第一个参数需要字符串");
        int64_t idx = i.t() == Value::T::INT ? i.i() : (int64_t)numVal(i);
        if (idx < 1 || idx > (int64_t)s.s().size())
            throw VesnaError("-char_at 下标越界: " + fmt(i));
        return mkStr(std::string(1, s.s()[(size_t)idx - 1]));
    }

    // ---- 列表 ----
    case 32: {
        Value v = ev(0);
        if (v.t() != Value::T::LIST && v.t() != Value::T::GROUP) throw VesnaError("-sort 需要列表/组");
        std::vector<Value> items = v.t() == Value::T::LIST ? v.list()->items : v.group()->items;
        std::stable_sort(items.begin(), items.end(), [](const Value& a, const Value& b) {
            return valueLess(a, b);
        });
        Value out = mkList();
        out.list()->items = std::move(items);
        return out;
    }
    case 33: {
        Value v = ev(0);
        if (v.t() != Value::T::LIST && v.t() != Value::T::GROUP) throw VesnaError("-reverse 需要列表/组");
        std::vector<Value> items = v.t() == Value::T::LIST ? v.list()->items : v.group()->items;
        std::reverse(items.begin(), items.end());
        Value out = mkList();
        out.list()->items = std::move(items);
        return out;
    }
    case 34: {
        Value lst = ev(0), st = ev(1), en = ev(2);
        if (lst.t() != Value::T::LIST && lst.t() != Value::T::GROUP && lst.t() != Value::T::STR)
            throw VesnaError("-slice 第一个参数需要列表/组/字符串");
        if (st.t() != Value::T::INT || en.t() != Value::T::INT) throw VesnaError("-slice 下标需要整数");
        size_t size = lst.t() == Value::T::STR ? lst.s().size()
                     : lst.t() == Value::T::LIST ? lst.list()->items.size()
                                               : lst.group()->items.size();
        int64_t a = st.i() - 1, b = en.i();
        if (a < 0) a = 0;
        if (a > (int64_t)size) a = (int64_t)size;
        if (b < a) b = a;
        if (b > (int64_t)size) b = (int64_t)size;
        if (lst.t() == Value::T::STR) return mkStr(lst.s().substr((size_t)a, (size_t)(b - a)));
        Value out = mkList();
        const auto& items = lst.t() == Value::T::LIST ? lst.list()->items : lst.group()->items;
        for (int64_t i = a; i < b; ++i) out.list()->items.push_back(items[(size_t)i]);
        return out;
    }

    case 35: case 36: {
        Value lst = ev(0);
        if (lst.t() != Value::T::LIST && lst.t() != Value::T::GROUP)
            throw VesnaError("-" + name + " 第一个参数需要列表/组");
        Value fnameV = ev(1);
        if (fnameV.t() != Value::T::STR)
            throw VesnaError("-" + name + " 第二个参数需要函数名字符串");
        auto fn = env->getFunc(internId(fnameV.s()));
        if (fn->params.empty()) throw VesnaError(fnameV.s() + " 需要 1 个参数");
        const auto& items = lst.t() == Value::T::LIST ? lst.list()->items : lst.group()->items;
        Value out = mkList();
        for (auto& item : items) {
            auto closure = fn->closure.lock();
            auto local = std::make_shared<Env>(closure);
            local->set(fn->params[0].first, item);
            Value result = mkNone();
            try {
                for (auto& s : fn->body) exec(s, local);
            } catch (ReturnSignal& rs) {
                result = rs.value;
            }
            if (name == "map") out.list()->items.push_back(result);
            else if (truthy(result)) out.list()->items.push_back(item);
        }
        return out;
    }
    case 37: {
        Value lst = ev(0);
        if (lst.t() != Value::T::LIST && lst.t() != Value::T::GROUP)
            throw VesnaError("-reduce 第一个参数需要列表/组");
        Value fnameV = ev(1);
        if (fnameV.t() != Value::T::STR) throw VesnaError("-reduce 第二个参数需要函数名字符串");
        Value init = argc() > 2 ? ev(2) : mkNone();
        auto fn = env->getFunc(internId(fnameV.s()));
        if (fn->params.size() < 2) throw VesnaError(fnameV.s() + " 需要 2 个参数");
        Value acc = init;
        const auto& items = lst.t() == Value::T::LIST ? lst.list()->items : lst.group()->items;
        for (auto& item : items) {
            auto closure = fn->closure.lock();
            auto local = std::make_shared<Env>(closure);
            local->set(fn->params[0].first, acc);
            local->set(fn->params[1].first, item);
            acc = mkNone();
            try {
                for (auto& s : fn->body) exec(s, local);
            } catch (ReturnSignal& rs) {
                acc = rs.value;
            }
        }
        return acc;
    }

    // ---- 正则 ----
    case 38: {
        Value s = ev(0), pat = ev(1);
        if (s.t() != Value::T::STR) throw VesnaError("-match 第一个参数需要字符串");
        if (isRegexSafeLiteral(pat.s())) return mkBool(s.s().find(pat.s()) != std::string::npos);
        try { return mkBool(std::regex_search(s.s(), cachedRegex(pat.s()))); }
        catch (std::regex_error&) { throw VesnaError("-match 无效正则: " + pat.s()); }
    }
    case 39: {
        Value s = ev(0), pat = ev(1);
        if (s.t() != Value::T::STR) throw VesnaError("-search 第一个参数需要字符串");
        if (isRegexSafeLiteral(pat.s())) {
            size_t pos = s.s().find(pat.s());
            Value out = mkList();
            if (pos != std::string::npos) out.list()->items.push_back(mkStr(pat.s()));
            return out;
        }
        try {
            std::smatch m;
            std::regex& re = cachedRegex(pat.s());
            if (!std::regex_search(s.s(), m, re)) return mkList();
            Value out = mkList();
            if (m.size() > 1) {
                for (size_t i = 1; i < m.size(); ++i) out.list()->items.push_back(mkStr(m[i].str()));
            } else {
                out.list()->items.push_back(mkStr(m[0].str()));
            }
            return out;
        } catch (std::regex_error&) { throw VesnaError("-search 无效正则: " + pat.s()); }
    }
    case 40: {
        Value s = ev(0), pat = ev(1);
        if (s.t() != Value::T::STR) throw VesnaError("-findall 第一个参数需要字符串");
        if (isRegexSafeLiteral(pat.s())) {
            Value out = mkList();
            size_t pos = 0;
            while ((pos = s.s().find(pat.s(), pos)) != std::string::npos) {
                out.list()->items.push_back(mkStr(pat.s()));
                pos += pat.s().size();
            }
            return out;
        }
        try {
            std::regex& re = cachedRegex(pat.s());
            Value out = mkList();
            auto begin = std::sregex_iterator(s.s().begin(), s.s().end(), re);
            auto end = std::sregex_iterator();
            for (auto it = begin; it != end; ++it) {
                const std::smatch& m = *it;
                if (m.size() > 1) {
                    Value g = mkGroup();
                    for (size_t i = 1; i < m.size(); ++i) g.group()->items.push_back(mkStr(m[i].str()));
                    out.list()->items.push_back(g);
                } else {
                    out.list()->items.push_back(mkStr(m[0].str()));
                }
            }
            return out;
        } catch (std::regex_error&) { throw VesnaError("-findall 无效正则: " + pat.s()); }
    }
    case 41: {
        Value s = ev(0), pat = ev(1), repl = ev(2);
        if (s.t() != Value::T::STR) throw VesnaError("-gsub 第一个参数需要字符串");
        if (isRegexSafeLiteral(pat.s()) && repl.s().find('\\') == std::string::npos &&
            repl.s().find('$') == std::string::npos) {
            std::string out = s.s();
            size_t pos = 0;
            while ((pos = out.find(pat.s(), pos)) != std::string::npos) {
                out.replace(pos, pat.s().size(), repl.s());
                pos += repl.s().size();
            }
            return mkStr(out);
        }
        try {
            std::string rp;
            for (size_t i = 0; i < repl.s().size(); ++i) {
                if (repl.s()[i] == '\\' && i + 1 < repl.s().size() &&
                    std::isdigit((unsigned char)repl.s()[i + 1])) {
                    rp += '$'; rp += repl.s()[i + 1]; ++i;
                } else if (repl.s()[i] == '\\' && i + 1 < repl.s().size() && repl.s()[i + 1] == '\\') {
                    rp += "\\\\"; ++i;
                } else {
                    rp += repl.s()[i];
                }
            }
            return mkStr(std::regex_replace(s.s(), cachedRegex(pat.s()), rp));
        } catch (std::regex_error&) { throw VesnaError("-gsub 无效正则: " + pat.s()); }
    }

    // ---- 文件 ----
    case 42: {
        std::string d = ".";
        if (argc() > 0) {
            Value v = ev(0);
            if (v.t() == Value::T::STR) d = v.s();
        }
        Value out = mkList();
        try {
            for (auto& e : listDir(d)) out.list()->items.push_back(mkStr(e));
        } catch (VesnaError&) { throw; }
        return out;
    }
    case 43: {
        Value p = ev(0);
        if (p.t() != Value::T::STR) throw VesnaError("-glob 需要字符串");
        Value out = mkList();
        for (auto& e : globPaths(p.s())) out.list()->items.push_back(mkStr(e));
        return out;
    }
    case 44: {
        std::string data((std::istreambuf_iterator<char>(std::cin)),
                         std::istreambuf_iterator<char>());
        return mkStr(univNewlines(data));
    }

    // ---- 字符 ----
    case 45: {
        Value c = ev(0);
        if (c.t() != Value::T::STR || c.s().empty()) throw VesnaError("-ord 需要非空字符串");
        return mkInt(utf8FirstCodepoint(c.s()));
    }
    case 46: {
        Value n = ev(0);
        int64_t cp = n.t() == Value::T::INT ? n.i() : (int64_t)numVal(n);
        if (cp < 0 || cp > 0x10FFFF) throw VesnaError("-chr 无效码点: " + std::to_string(cp));
        return mkStr(utf8FromCodepoint(cp));
    }
    case 47: {
        Value c = ev(0);
        return mkBool(c.t() == Value::T::STR && c.s().size() == 1 &&
                      c.s()[0] >= '0' && c.s()[0] <= '9');
    }
    case 48: {
        Value c = ev(0);
        return mkBool(c.t() == Value::T::STR && c.s().size() == 1 &&
                      std::isalpha((unsigned char)c.s()[0]));
    }
    case 49: {
        Value c = ev(0);
        return mkBool(c.t() == Value::T::STR && c.s().size() == 1 &&
                      std::isalnum((unsigned char)c.s()[0]));
    }
    case 50: {
        Value c = ev(0);
        return mkBool(c.t() == Value::T::STR && c.s().size() == 1 &&
                      std::isspace((unsigned char)c.s()[0]));
    }

    // ---- 字符串扩展 ----
    case 51: {
        Value v = ev(0);
        if (v.t() != Value::T::STR) throw VesnaError("-lstrip 需要字符串");
        size_t b = v.s().find_first_not_of(" \t\r\n");
        return mkStr(b == std::string::npos ? "" : v.s().substr(b));
    }
    case 52: {
        Value v = ev(0);
        if (v.t() != Value::T::STR) throw VesnaError("-rstrip 需要字符串");
        size_t e = v.s().find_last_not_of(" \t\r\n");
        return mkStr(e == std::string::npos ? "" : v.s().substr(0, e + 1));
    }
    case 53: {
        Value v = ev(0);
        if (v.t() != Value::T::STR) throw VesnaError("-title 需要字符串");
        return mkStr(titleAscii(v.s()));
    }
    case 54: {
        Value v = ev(0);
        if (v.t() != Value::T::STR) throw VesnaError("-capitalize 需要字符串");
        return mkStr(capitalizeAscii(v.s()));
    }
    case 55: {
        Value s = ev(0), sub = ev(1);
        if (s.t() != Value::T::STR) throw VesnaError("-count 第一个参数需要字符串");
        if (sub.s().empty()) return mkInt((int64_t)s.s().size() + 1);
        int64_t cnt = 0;
        size_t pos = 0;
        while ((pos = s.s().find(sub.s(), pos)) != std::string::npos) { ++cnt; pos += sub.s().size(); }
        return mkInt(cnt);
    }
    case 56: {
        Value s = ev(0), sub = ev(1);
        if (s.t() != Value::T::STR) throw VesnaError("-rfind 第一个参数需要字符串");
        size_t i = s.s().rfind(sub.s());
        return mkInt(i == std::string::npos ? 0 : (int64_t)i + 1);
    }

    // ---- 数学 ----
    case 57: case 58: {
        if (argc() == 0) throw VesnaError("-" + name + " 需要至少一个参数");
        Value v = ev(0);
        std::vector<Value> vals;
        if (v.t() == Value::T::LIST || v.t() == Value::T::GROUP) {
            const auto& items = v.t() == Value::T::LIST ? v.list()->items : v.group()->items;
            if (items.empty()) throw VesnaError("-" + name + " 不能是空列表");
            vals = items;
        } else {
            vals.push_back(v);
            for (size_t i = 1; i < argc(); ++i) vals.push_back(ev(i));
        }
        auto better = [&](const Value& a, const Value& b) {
            if (name == "min") return valueLess(a, b);
            return valueLess(b, a);
        };
        Value best = vals[0];
        for (size_t i = 1; i < vals.size(); ++i)
            if (better(vals[i], best)) best = vals[i];
        return best;
    }
    case 59: {
        Value v = ev(0);
        if (v.t() != Value::T::LIST && v.t() != Value::T::GROUP) throw VesnaError("-sum 需要列表/组");
        const auto& items = v.t() == Value::T::LIST ? v.list()->items : v.group()->items;
        bool anyFloat = false;
        for (auto& x : items) if (x.t() == Value::T::FLOAT) anyFloat = true;
        if (anyFloat) {
            double acc = 0;
            for (auto& x : items) {
                if (x.t() == Value::T::INT) acc += x.i();
                else if (x.t() == Value::T::FLOAT) acc += x.f();
                else if (x.t() == Value::T::BOOL) acc += x.b() ? 1 : 0;
                else throw VesnaError("-sum 只能对数字求和");
            }
            return mkFloat(acc);
        }
        int64_t acc = 0;
        for (auto& x : items) {
            if (x.t() == Value::T::INT) acc += x.i();
            else if (x.t() == Value::T::BOOL) acc += x.b() ? 1 : 0;
            else throw VesnaError("-sum 只能对数字求和");
        }
        return mkInt(acc);
    }
    case 60: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-abs 需要数字");
        return v.t() == Value::T::INT ? mkInt(v.i() < 0 ? -v.i() : v.i()) : mkFloat(std::abs(v.f()));
    }
    case 61: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-round 需要数字");
        double x = numVal(v);
        if (argc() > 1) {
            Value n = ev(1);
            int64_t nd = n.t() == Value::T::INT ? n.i() : (int64_t)numVal(n);
            double scale = std::pow(10.0, (double)nd);
            return mkFloat((double)pyRound(x * scale) / scale);
        }
        return mkInt(pyRound(x));
    }
    case 62: {
        Value a = ev(0), b = ev(1);
        if (!isNum(a) || !isNum(b)) throw VesnaError("-pow 需要数字");
        double x = numVal(a), y = numVal(b);
        if (x < 0 && std::floor(y) != y) throw VesnaError("-pow 结果为复数，不支持");
        double r = std::pow(x, y);
        if (std::isnan(r) || std::isinf(r)) throw VesnaError("-pow 无法计算");
        if (a.t() == Value::T::INT && b.t() == Value::T::INT && y >= 0 && r < 9.2e18)
            return mkInt((int64_t)r);
        return mkFloat(r);
    }

    // ---- 容器 ----
    case 63: {
        Value v = ev(0), x = ev(1);
        switch (v.t()) {
            case Value::T::LIST:
                for (auto& it : v.list()->items) if (vesnaEq(it, x)) return mkBool(true);
                return mkBool(false);
            case Value::T::GROUP:
                for (auto& it : v.group()->items) if (vesnaEq(it, x)) return mkBool(true);
                return mkBool(false);
            case Value::T::STR: {
                if (x.t() != Value::T::STR) throw VesnaError("-contains 字符串参数需要字符串");
                return mkBool(v.s().find(x.s()) != std::string::npos);
            }
            case Value::T::DICT:
                return mkBool(dictFind(*v.dict(), x) != nullptr);
            default:
                throw VesnaError("-contains 需要列表/组/字符串/字典");
        }
    }

    // ---- 文件系统 ----
    case 64: {
        Value p = ev(0);
        if (p.t() != Value::T::STR) throw VesnaError("-mkdir 需要字符串");
        makeDirs(p.s());
        return mkNone();
    }
    case 65: {
        Value s = ev(0), d = ev(1);
        if (s.t() != Value::T::STR || d.t() != Value::T::STR)
            throw VesnaError("-copy 参数需要字符串");
        if (!fileExists(s.s())) throw VesnaError("-copy 源不存在: " + s.s());
        if (isDirectory(s.s())) {
            makeDirs(d.s());
            copyPath(s.s(), d.s());
        } else {
            makeDirs(parentDir(d.s()));
            copyPath(s.s(), d.s());
        }
        return mkNone();
    }
    case 66: {
        Value p = ev(0);
        if (p.t() != Value::T::STR) throw VesnaError("-rmdir 需要字符串");
        if (fileExists(p.s())) removeTree(p.s());
        return mkNone();
    }
    case 67: {
        Value s = ev(0), d = ev(1);
        if (s.t() != Value::T::STR || d.t() != Value::T::STR)
            throw VesnaError("-rename 参数需要字符串");
        if (!MoveFileW(utf8ToWide(s.s()).c_str(), utf8ToWide(d.s()).c_str()))
            throw VesnaError("-rename 失败: " + s.s());
        return mkNone();
    }
    case 68: {
        Value k = ev(0);
        if (k.t() != Value::T::STR) throw VesnaError("-getenv 需要字符串");
        const char* v = std::getenv(k.s().c_str());
        return mkStr(v ? v : "");
    }
    case 69: {
        Value k = ev(0), v = ev(1);
        if (k.t() != Value::T::STR || v.t() != Value::T::STR)
            throw VesnaError("-setenv 参数需要字符串");
#ifdef _WIN32
        HKEY hk;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_SET_VALUE, &hk) != ERROR_SUCCESS)
            throw VesnaError("-setenv 失败: 无法打开 Environment 注册表键");
        std::wstring wk = utf8ToWide(k.s()), wv = utf8ToWide(v.s());
        LONG r = RegSetValueExW(hk, wk.c_str(), 0, REG_SZ,
                                (const BYTE*)wv.c_str(),
                                (DWORD)((wv.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hk);
        if (r != ERROR_SUCCESS) throw VesnaError("-setenv 失败: 写入注册表错误");
#endif
        if (setEnvProc(k.s(), v.s()) != 0)
            throw VesnaError("-setenv 失败: 无法设置进程环境变量");
        return mkNone();
    }
    case 70:
        return mkStr(getCwd());
    case 71: {
        Value p = ev(0);
        if (p.t() != Value::T::STR) throw VesnaError("-chdir 需要字符串");
        if (chDir(p.s()) != 0)
            throw VesnaError("-chdir 失败: " + p.s());
        return mkNone();
    }
    case 72: {
        Value rootV = ev(0), pathV = ev(1), keyV = ev(2), valueV = ev(3);
#ifdef _WIN32
        HKEY hroot = (rootV.s() == "HKLM") ? HKEY_LOCAL_MACHINE
                   : (rootV.s() == "HKCU") ? HKEY_CURRENT_USER : nullptr;
        if (!hroot) throw VesnaError("-regwrite 未知根: " + rootV.s());
        HKEY hk;
        if (RegCreateKeyExW(hroot, utf8ToWide(pathV.s()).c_str(), 0, nullptr, 0,
                            KEY_SET_VALUE, nullptr, &hk, nullptr) != ERROR_SUCCESS)
            throw VesnaError("-regwrite 失败");
        std::wstring wk = utf8ToWide(keyV.s()), wv = utf8ToWide(valueV.s());
        LONG r = RegSetValueExW(hk, wk.c_str(), 0, REG_SZ,
                                (const BYTE*)wv.c_str(),
                                (DWORD)((wv.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hk);
        if (r != ERROR_SUCCESS) throw VesnaError("-regwrite 失败");
        return mkNone();
#else
        (void)rootV; (void)pathV; (void)keyV; (void)valueV;
        throw VesnaError("-regwrite 当前平台不支持");
#endif
    }
    case 73: {
        Value rootV = ev(0), pathV = ev(1);
#ifdef _WIN32
        HKEY hroot = (rootV.s() == "HKLM") ? HKEY_LOCAL_MACHINE
                   : (rootV.s() == "HKCU") ? HKEY_CURRENT_USER : nullptr;
        if (!hroot) throw VesnaError("-regdelete 未知根: " + rootV.s());
        LONG r = RegDeleteTreeW(hroot, utf8ToWide(pathV.s()).c_str());
        if (r != ERROR_SUCCESS && r != ERROR_FILE_NOT_FOUND)
            throw VesnaError("-regdelete 失败");
        return mkNone();
#else
        (void)rootV; (void)pathV;
        throw VesnaError("-regdelete 当前平台不支持");
#endif
    }
    case 146: {  // -regenv(name): 读用户环境变量（Windows: HKCU\Environment），无则 ""
        Value k = ev(0);
        if (k.t() != Value::T::STR) throw VesnaError("-regenv 需要字符串");
#ifdef _WIN32
        HKEY hk = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0,
                          KEY_QUERY_VALUE, &hk) != ERROR_SUCCESS)
            return mkStr("");
        std::wstring wk = utf8ToWide(k.s());
        wchar_t buf[32768];
        DWORD size = sizeof(buf);
        LONG r = RegQueryValueExW(hk, wk.c_str(), nullptr, nullptr,
                                  (LPBYTE)buf, &size);
        RegCloseKey(hk);
        if (r != ERROR_SUCCESS) return mkStr("");
        size_t chars = size / sizeof(wchar_t);
        if (chars > 0 && buf[chars - 1] == L'\0') chars -= 1;
        return mkStr(wideToUtf8(std::wstring(buf, chars)));
#else
        return mkStr(std::getenv(k.s().c_str()) ? std::getenv(k.s().c_str()) : "");
#endif
    }
    case 74: {
        Value cmd = ev(0);
        if (cmd.t() != Value::T::STR) throw VesnaError("-shell 需要字符串");
        sysShell(cmd.s());
        return mkNone();
    }
    case 75: {
        Value p = ev(0);
        if (p.t() != Value::T::STR) throw VesnaError("-path_clean 需要字符串");
        return mkStr(pathAbs(p.s()));
    }

    // ---- 0.4 数学 ----
    case 76: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-sqrt 需要数字");
        double x = numVal(v);
        if (x < 0) throw VesnaError("-sqrt 负数无实根");
        return mkFloat(std::sqrt(x));
    }
    case 77: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-floor 需要数字");
        return mkInt((int64_t)std::floor(numVal(v)));
    }
    case 78: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-ceil 需要数字");
        return mkInt((int64_t)std::ceil(numVal(v)));
    }
    case 79: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-exp 需要数字");
        return mkFloat(std::exp(numVal(v)));
    }
    case 80: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-log 需要数字");
        double x = numVal(v);
        if (x <= 0) throw VesnaError("-log 参数必须大于 0");
        return mkFloat(std::log(x));
    }
    case 81: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-log10 需要数字");
        double x = numVal(v);
        if (x <= 0) throw VesnaError("-log10 参数必须大于 0");
        return mkFloat(std::log10(x));
    }
    case 82: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-sin 需要数字");
        return mkFloat(std::sin(numVal(v)));
    }
    case 83: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-cos 需要数字");
        return mkFloat(std::cos(numVal(v)));
    }
    case 84: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-tan 需要数字");
        return mkFloat(std::tan(numVal(v)));
    }
    case 85: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-sign 需要数字");
        double x = numVal(v);
        return mkInt(x > 0 ? 1 : (x < 0 ? -1 : 0));
    }
    case 86: {
        Value x = ev(0), lo = ev(1), hi = ev(2);
        if (x.t() == Value::T::BOOL || lo.t() == Value::T::BOOL || hi.t() == Value::T::BOOL ||
            !isNum(x) || !isNum(lo) || !isNum(hi)) throw VesnaError("-clamp 需要数字");
        double v = numVal(x);
        if (numVal(lo) > numVal(hi)) throw VesnaError("-clamp 下界不能大于上界");
        v = v < numVal(lo) ? numVal(lo) : (v > numVal(hi) ? numVal(hi) : v);
        if (x.t() == Value::T::INT && lo.t() == Value::T::INT && hi.t() == Value::T::INT)
            return mkInt((int64_t)v);
        return mkFloat(v);
    }
    case 87: {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return mkFloat(dist(rngGen()));
    }
    case 88: {
        Value a = ev(0), b = ev(1);
        if (a.t() == Value::T::BOOL || b.t() == Value::T::BOOL || !isNum(a) || !isNum(b))
            throw VesnaError("-randint 需要整数");
        int64_t lo = (int64_t)numVal(a), hi = (int64_t)numVal(b);
        if (hi < lo) { std::swap(lo, hi); }
        std::uniform_int_distribution<int64_t> dist(lo, hi);
        return mkInt(dist(rngGen()));
    }
    case 89: {
        Value lst = ev(0);
        if (lst.t() != Value::T::LIST && lst.t() != Value::T::GROUP) throw VesnaError("-choice 需要列表/组");
        const auto& items = lst.t() == Value::T::LIST ? lst.list()->items : lst.group()->items;
        if (items.empty()) throw VesnaError("-choice 空列表");
        return items[rngGen()() % items.size()];
    }
    case 90: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-hex 需要整数");
        return mkStr(intToBaseStr((int64_t)numVal(v), 16));
    }
    case 91: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-bin 需要整数");
        return mkStr(intToBaseStr((int64_t)numVal(v), 2));
    }
    case 92: {
        Value v = ev(0);
        if (v.t() == Value::T::BOOL || !isNum(v)) throw VesnaError("-oct 需要整数");
        return mkStr(intToBaseStr((int64_t)numVal(v), 8));
    }

    // ---- 0.4 字符串 ----
    case 93: case 94: case 95: {
        Value s = ev(0), w = ev(1);
        if (s.t() != Value::T::STR) throw VesnaError("-" + name + " 第一个参数需要字符串");
        if (w.t() != Value::T::INT) throw VesnaError("-" + name + " 宽度需要整数");
        int64_t width = w.i();
        std::string padc = " ";
        if (argc() > 2) {
            Value c = ev(2);
            if (c.t() != Value::T::STR || c.s().empty()) throw VesnaError("-" + name + " 填充字符需要非空字符串");
            padc = c.s();
        }
        int64_t len = (int64_t)s.s().size();
        if (width <= len) return s;
        int64_t total = width - len;
        if (name == "lpad") {
            std::string out;
            out.reserve((size_t)width);
            for (int64_t i = 0; i < total; ++i) out += padc;
            out += s.s();
            return mkStr(out);
        }
        if (name == "rpad") {
            std::string out = s.s();
            for (int64_t i = 0; i < total; ++i) out += padc;
            return mkStr(out);
        }
        int64_t left = total / 2, right = total - left;
        std::string out;
        out.reserve((size_t)width);
        for (int64_t i = 0; i < left; ++i) out += padc;
        out += s.s();
        for (int64_t i = 0; i < right; ++i) out += padc;
        return mkStr(out);
    }
    case 96: {
        std::vector<Value> vals;
        for (size_t i = 0; i < argc(); ++i) vals.push_back(ev(i));
        return mkStr(formatStr(vals.empty() ? "" : vals[0].t() == Value::T::STR ? vals[0].s() : fmt(vals[0]),
                              std::vector<Value>(vals.begin() + (vals.empty() ? 0 : 1), vals.end())));
    }
    case 97: {
        Value s = ev(0);
        if (s.t() != Value::T::STR) throw VesnaError("-hash 需要字符串");
        return mkStr(fnv1a64Str(s.s()));
    }

    // ---- 0.4 列表 ----
    case 98: {
        Value st = ev(0), en = ev(1);
        if (st.t() != Value::T::INT || en.t() != Value::T::INT) throw VesnaError("-range 参数需要整数");
        int64_t step = 1;
        if (argc() > 2) {
            Value sp = ev(2);
            if (sp.t() != Value::T::INT) throw VesnaError("-range 步长需要整数");
            step = sp.i();
        }
        if (step == 0) throw VesnaError("-range 步长不能为 0");
        Value out = mkList();
        int64_t i = st.i();
        if (step > 0) {
            while (i < en.i()) { out.list()->items.push_back(mkInt(i)); i += step; }
        } else {
            while (i > en.i()) { out.list()->items.push_back(mkInt(i)); i += step; }
        }
        return out;
    }
    case 99: case 100: {
        Value v = ev(0);
        if (v.t() != Value::T::LIST && v.t() != Value::T::GROUP) throw VesnaError("-" + name + " 需要列表/组");
        const auto& items = v.t() == Value::T::LIST ? v.list()->items : v.group()->items;
        if (items.empty()) return mkNone();
        return name == "first" ? items[0] : items.back();
    }
    case 101: case 102: {
        Value v = ev(0), n = ev(1);
        if (v.t() != Value::T::LIST && v.t() != Value::T::GROUP) throw VesnaError("-" + name + " 第一个参数需要列表/组");
        if (n.t() != Value::T::INT) throw VesnaError("-" + name + " 数量需要整数");
        const auto& items = v.t() == Value::T::LIST ? v.list()->items : v.group()->items;
        int64_t cnt = n.i();
        if (cnt < 0) cnt = 0;
        Value out = mkList();
        if (name == "take") {
            for (int64_t i = 0; i < cnt && i < (int64_t)items.size(); ++i) out.list()->items.push_back(items[(size_t)i]);
        } else {
            for (int64_t i = cnt; i < (int64_t)items.size(); ++i) out.list()->items.push_back(items[(size_t)i]);
        }
        return out;
    }
    case 103: {
        Value v = ev(0);
        if (v.t() != Value::T::LIST && v.t() != Value::T::GROUP) throw VesnaError("-set 需要列表/组");
        const auto& items = v.t() == Value::T::LIST ? v.list()->items : v.group()->items;
        Value out = mkList();
        for (auto& it : items) {
            bool dup = false;
            for (auto& o : out.list()->items) if (vesnaEq(o, it)) { dup = true; break; }
            if (!dup) out.list()->items.push_back(it);
        }
        return out;
    }
    case 104: {
        Value v = ev(0);
        if (v.t() != Value::T::LIST && v.t() != Value::T::GROUP) throw VesnaError("-flatten 需要列表/组");
        const auto& items = v.t() == Value::T::LIST ? v.list()->items : v.group()->items;
        Value out = mkList();
        for (auto& it : items) {
            if (it.t() == Value::T::LIST) {
                for (auto& x : it.list()->items) out.list()->items.push_back(x);
            } else if (it.t() == Value::T::GROUP) {
                for (auto& x : it.group()->items) out.list()->items.push_back(x);
            } else {
                out.list()->items.push_back(it);
            }
        }
        return out;
    }
    case 105: {
        Value a = ev(0), b = ev(1);
        if (a.t() != Value::T::LIST && a.t() != Value::T::GROUP) throw VesnaError("-zip 第一个参数需要列表/组");
        if (b.t() != Value::T::LIST && b.t() != Value::T::GROUP) throw VesnaError("-zip 第二个参数需要列表/组");
        const auto& x = a.t() == Value::T::LIST ? a.list()->items : a.group()->items;
        const auto& y = b.t() == Value::T::LIST ? b.list()->items : b.group()->items;
        Value out = mkList();
        size_t n = std::min(x.size(), y.size());
        for (size_t i = 0; i < n; ++i) {
            Value g = mkGroup();
            g.group()->items.push_back(x[i]);
            g.group()->items.push_back(y[i]);
            out.list()->items.push_back(g);
        }
        return out;
    }
    case 106: {
        Value a = ev(0), i = ev(1), x = ev(2);
        if (a.t() != Value::T::LIST && a.t() != Value::T::GROUP) throw VesnaError("-insert 第一个参数需要列表/组");
        if (i.t() != Value::T::INT) throw VesnaError("-insert 位置需要整数");
        const auto& items = a.t() == Value::T::LIST ? a.list()->items : a.group()->items;
        int64_t pos = i.i() - 1;
        if (pos < 0) pos = 0;
        if (pos > (int64_t)items.size()) pos = (int64_t)items.size();
        Value out = mkList();
        for (int64_t k = 0; k < pos; ++k) out.list()->items.push_back(items[(size_t)k]);
        out.list()->items.push_back(x);
        for (int64_t k = pos; k < (int64_t)items.size(); ++k) out.list()->items.push_back(items[(size_t)k]);
        return out;
    }
    case 107: {
        Value a = ev(0), i = ev(1);
        if (a.t() != Value::T::LIST && a.t() != Value::T::GROUP) throw VesnaError("-remove 第一个参数需要列表/组");
        if (i.t() != Value::T::INT) throw VesnaError("-remove 位置需要整数");
        const auto& items = a.t() == Value::T::LIST ? a.list()->items : a.group()->items;
        int64_t pos = i.i() - 1;
        if (pos < 0) pos += (int64_t)items.size();
        if (pos < 0 || pos >= (int64_t)items.size()) throw VesnaError("-remove 下标越界: " + fmt(i));
        Value out = mkList();
        for (int64_t k = 0; k < (int64_t)items.size(); ++k)
            if (k != pos) out.list()->items.push_back(items[(size_t)k]);
        return out;
    }
    case 108: {
        Value a = ev(0), x = ev(1);
        if (a.t() != Value::T::LIST && a.t() != Value::T::GROUP) throw VesnaError("-index_of 第一个参数需要列表/组");
        const auto& items = a.t() == Value::T::LIST ? a.list()->items : a.group()->items;
        for (size_t k = 0; k < items.size(); ++k)
            if (vesnaEq(items[k], x)) return mkInt((int64_t)k + 1);
        return mkInt(0);
    }
    case 109: {
        Value a = ev(0);
        if (a.t() != Value::T::LIST && a.t() != Value::T::GROUP) throw VesnaError("-enumerate 需要列表/组");
        const auto& items = a.t() == Value::T::LIST ? a.list()->items : a.group()->items;
        Value out = mkList();
        for (size_t k = 0; k < items.size(); ++k) {
            Value g = mkGroup();
            g.group()->items.push_back(mkInt((int64_t)k + 1));
            g.group()->items.push_back(items[k]);
            out.list()->items.push_back(g);
        }
        return out;
    }
    case 110: {
        Value a = ev(0), b = ev(1);
        if (a.t() != Value::T::LIST && a.t() != Value::T::GROUP) throw VesnaError("-concat 第一个参数需要列表/组");
        if (b.t() != Value::T::LIST && b.t() != Value::T::GROUP) throw VesnaError("-concat 第二个参数需要列表/组");
        const auto& x = a.t() == Value::T::LIST ? a.list()->items : a.group()->items;
        const auto& y = b.t() == Value::T::LIST ? b.list()->items : b.group()->items;
        Value out = mkList();
        out.list()->items = x;
        for (auto& it : y) out.list()->items.push_back(it);
        return out;
    }

    // ---- 0.4 字典 ----
    case 111: {
        Value d = ev(0), k = ev(1);
        if (d.t() != Value::T::DICT) throw VesnaError("-get 第一个参数需要字典");
        auto* p = dictFind(*d.dict(), k);
        if (p) return p->second;
        return argc() > 2 ? ev(2) : mkNone();
    }
    case 112: {
        Value d = ev(0);
        if (d.t() != Value::T::DICT) throw VesnaError("-items 需要字典");
        Value out = mkList();
        for (auto& p : d.dict()->pairs) {
            Value g = mkGroup();
            g.group()->items.push_back(p.first);
            g.group()->items.push_back(p.second);
            out.list()->items.push_back(g);
        }
        return out;
    }
    case 113: {
        Value d = ev(0), k = ev(1);
        if (d.t() != Value::T::DICT) throw VesnaError("-pop_key 第一个参数需要字典");
        for (size_t i = 0; i < d.dict()->pairs.size(); ++i) {
            if (vesnaEq(d.dict()->pairs[i].first, k)) {
                Value old = d.dict()->pairs[i].second;
                d.dict()->pairs.erase(d.dict()->pairs.begin() + (std::ptrdiff_t)i);
                return old;
            }
        }
        return mkNone();
    }

    // ---- 0.4 类型判断 ----
    case 114: return mkBool(ev(0).t() == Value::T::STR);
    case 115: return mkBool(ev(0).t() == Value::T::INT);
    case 116: return mkBool(ev(0).t() == Value::T::FLOAT);
    case 117: return mkBool(ev(0).t() == Value::T::BOOL);
    case 118: return mkBool(ev(0).t() == Value::T::LIST);
    case 119: return mkBool(ev(0).t() == Value::T::DICT);
    case 120: return mkBool(ev(0).t() == Value::T::NONE);
    case 121: return mkBool(ev(0).t() == Value::T::GROUP);

    // ---- 0.4 时间 / 系统 ----
    case 122: {
        return mkInt((int64_t)std::time(nullptr));
    }
    case 123: {
        std::string fmt = "%Y-%m-%d %H:%M:%S";
        if (argc() > 0) {
            Value f = ev(0);
            if (f.t() != Value::T::STR) throw VesnaError("-date 格式需要字符串");
            fmt = f.s();
        }
        std::time_t t = std::time(nullptr);
        std::tm tm = {};
        localtime_s(&tm, &t);
        char buf[256];
        std::strftime(buf, sizeof(buf), fmt.c_str(), &tm);
        return mkStr(buf);
    }
    case 124: {
        Value ms = ev(0);
        if (ms.t() != Value::T::INT) throw VesnaError("-sleep 需要整数毫秒");
        if (ms.i() < 0) return mkNone();
        Sleep((DWORD)ms.i());
        return mkNone();
    }
    case 125: {
        static const auto start = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        return mkInt((int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count());
    }
    case 126: {  // -platform
#ifdef _WIN32
        return mkStr("windows");
#elif defined(__APPLE__)
        return mkStr("mac");
#else
        return mkStr("linux");
#endif
    }
    case 127: {
#ifdef _WIN32
        wchar_t buf[MAX_PATH];
        DWORD n = GetTempPathW(MAX_PATH, buf);
        if (n == 0) return mkStr("");
        std::wstring w = buf;
        while (!w.empty() && (w.back() == L'\\' || w.back() == L'/')) w.pop_back();
        return mkStr(wideToUtf8(w));
#else
        const char* t = std::getenv("TMPDIR");
        return mkStr(t && *t ? t : "/tmp");
#endif
    }

    // ---- 0.4 文件 ----
    case 128: {
        Value p = ev(0);
        if (p.t() != Value::T::STR) throw VesnaError("-fremove 需要字符串");
        if (fileExists(p.s())) DeleteFileW(utf8ToWide(p.s()).c_str());
        return mkNone();
    }
    case 129: {
        Value s = ev(0), d = ev(1);
        if (s.t() != Value::T::STR || d.t() != Value::T::STR) throw VesnaError("-fmove 参数需要字符串");
        if (!fileExists(s.s())) throw VesnaError("-fmove 源不存在: " + s.s());
        if (!MoveFileW(utf8ToWide(s.s()).c_str(), utf8ToWide(d.s()).c_str()))
            throw VesnaError("-fmove 失败: " + s.s());
        return mkNone();
    }
    case 130: {
        Value p = ev(0);
        if (p.t() != Value::T::STR) throw VesnaError("-fsize 需要字符串");
        WIN32_FILE_ATTRIBUTE_DATA info;
        if (!GetFileAttributesExW(utf8ToWide(p.s()).c_str(), GetFileExInfoStandard, &info))
            throw VesnaError("-fsize 无法访问: " + p.s());
        ULARGE_INTEGER sz;
        sz.LowPart = info.nFileSizeLow;
        sz.HighPart = info.nFileSizeHigh;
        return mkInt((int64_t)sz.QuadPart);
    }
    case 131: {
        Value p = ev(0);
        if (p.t() != Value::T::STR) throw VesnaError("-is_dir 需要字符串");
        return mkBool(isDirectory(p.s()));
    }
    case 132: {
        Value p = ev(0);
        if (p.t() != Value::T::STR) throw VesnaError("-is_file 需要字符串");
        return mkBool(fileExists(p.s()) && !isDirectory(p.s()));
    }
    case 133: {
        Value p = ev(0);
        if (p.t() != Value::T::STR) throw VesnaError("-mkdirs 需要字符串");
        makeDirs(p.s());
        return mkNone();
    }

    // ---- 0.4 编码 ----
    case 134: {
        Value s = ev(0);
        if (s.t() != Value::T::STR) throw VesnaError("-base64_encode 需要字符串");
        return mkStr(base64EncodeStr(s.s()));
    }
    case 135: {
        Value s = ev(0);
        if (s.t() != Value::T::STR) throw VesnaError("-base64_decode 需要字符串");
        return mkStr(base64DecodeStr(s.s()));
    }
    case 136: {
        Value s = ev(0);
        if (s.t() != Value::T::STR) throw VesnaError("-url_encode 需要字符串");
        return mkStr(urlEncodeStr(s.s()));
    }
    case 137: {
        Value s = ev(0);
        if (s.t() != Value::T::STR) throw VesnaError("-url_decode 需要字符串");
        return mkStr(urlDecodeStr(s.s()));
    }

    // ---- 0.4 函数式 ----
    case 138: case 139: case 140: case 141: {
        Value lst = ev(0);
        if (lst.t() != Value::T::LIST && lst.t() != Value::T::GROUP)
            throw VesnaError("-" + name + " 第一个参数需要列表/组");
        Value fnameV = ev(1);
        if (fnameV.t() != Value::T::STR)
            throw VesnaError("-" + name + " 第二个参数需要函数名字符串");
        auto fn = env->getFunc(internId(fnameV.s()));
        if (fn->params.empty()) throw VesnaError(fnameV.s() + " 需要 1 个参数");
        const auto& items = lst.t() == Value::T::LIST ? lst.list()->items : lst.group()->items;
        if (name == "each") {
            for (auto& item : items) {
                auto closure = fn->closure.lock();
                auto local = std::make_shared<Env>(closure);
                local->set(fn->params[0].first, item);
                try {
                    for (auto& s : fn->body) exec(s, local);
                } catch (ReturnSignal&) {}
            }
            return lst;
        }
        bool all_true = true, any_true = false;
        Value found = mkNone();
        bool found_ok = false;
        for (auto& item : items) {
            auto closure = fn->closure.lock();
            auto local = std::make_shared<Env>(closure);
            local->set(fn->params[0].first, item);
            Value result = mkNone();
            try {
                for (auto& s : fn->body) exec(s, local);
            } catch (ReturnSignal& rs) {
                result = rs.value;
            }
            bool tr = truthy(result);
            if (name == "all") { if (!tr) { all_true = false; break; } }
            else if (name == "any") { if (tr) { any_true = true; break; } }
            else if (name == "find_first") { if (tr) { found = item; found_ok = true; break; } }
        }
        if (name == "all") return mkBool(all_true);
        if (name == "any") return mkBool(any_true);
        return found_ok ? found : mkNone();
    }
    case 142: {
        Value lst = ev(0);
        if (lst.t() != Value::T::LIST && lst.t() != Value::T::GROUP)
            throw VesnaError("-sort_by 第一个参数需要列表/组");
        Value fnameV = ev(1);
        if (fnameV.t() != Value::T::STR)
            throw VesnaError("-sort_by 第二个参数需要函数名字符串");
        auto fn = env->getFunc(internId(fnameV.s()));
        if (fn->params.empty()) throw VesnaError(fnameV.s() + " 需要 1 个参数");
        const auto& items = lst.t() == Value::T::LIST ? lst.list()->items : lst.group()->items;
        auto keyOf = [&](const Value& item) -> Value {
            auto closure = fn->closure.lock();
            auto local = std::make_shared<Env>(closure);
            local->set(fn->params[0].first, item);
            Value result = mkNone();
            try {
                for (auto& s : fn->body) exec(s, local);
            } catch (ReturnSignal& rs) {
                result = rs.value;
            }
            return result;
        };
        std::vector<Value> items_copy = items;
        std::stable_sort(items_copy.begin(), items_copy.end(), [&](const Value& a, const Value& b) {
            return valueLess(keyOf(a), keyOf(b));
        });
        Value out = mkList();
        out.list()->items = std::move(items_copy);
        return out;
    }

    // ---- 0.4 异常 ----
    case 143: {
        Value m = ev(0);
        throw VesnaError(m.t() == Value::T::STR ? m.s() : fmt(m));
    }
    case 144: {
        bool ok = truthy(ev(0));
        if (!ok) {
            std::string msg = "assert 失败";
            if (argc() > 1) {
                Value m = ev(1);
                msg = m.t() == Value::T::STR ? m.s() : fmt(m);
            }
            throw VesnaError(msg);
        }
        return mkNone();
    }
    case 145: {
        Value v = ev(0);
        if (v.t() != Value::T::LIST && v.t() != Value::T::GROUP) throw VesnaError("-shuffle 需要列表/组");
        std::vector<Value> items = v.t() == Value::T::LIST ? v.list()->items : v.group()->items;
        std::shuffle(items.begin(), items.end(), rngGen());
        Value out = mkList();
        out.list()->items = std::move(items);
        return out;
    }
    case 147: {  // -cpdir(src; dst): 递归复制目录（dst 不存在则创建）
        Value s = ev(0), d = ev(1);
        if (s.t() != Value::T::STR || d.t() != Value::T::STR)
            throw VesnaError("-cpdir 需要两个路径字符串");
        try {
            std::filesystem::path sp = std::filesystem::u8path(s.s());
            std::filesystem::path dp = std::filesystem::u8path(d.s());
            std::error_code ec;
            if (!std::filesystem::exists(sp, ec))
                throw VesnaError("-cpdir 源目录不存在: " + s.s());
            std::filesystem::create_directories(dp, ec);
            if (ec) throw VesnaError("-cpdir 创建目标目录失败: " + d.s());
            std::filesystem::copy(sp, dp,
                std::filesystem::copy_options::recursive |
                std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) throw VesnaError("-cpdir 复制失败: " + ec.message());
        } catch (VesnaError&) {
            throw;
        } catch (const std::exception& e) {
            throw VesnaError(std::string("-cpdir 异常: ") + e.what());
        }
        return mkNone();
    }

    // ---- 1.1 并发 ----
    case 148: {  // -thread(name; arg...)
        Value nameV = ev(0);
        if (nameV.t() != Value::T::STR) throw VesnaError("-thread 第一个参数需要函数名字符串");
        std::vector<Value> argv;
        for (size_t i = 1; i < argc(); ++i) argv.push_back(ev(i));
        int64_t nid = internId(nameV.s());
        if (!env->getFunc(nid)) throw VesnaError("-thread 未找到函数: " + nameV.s());
        auto gcopy = std::make_shared<Env>();
        for (auto& kv : g->vars) gcopy->vars.insert(kv);
        for (auto& kv : g->funcs) gcopy->funcs.insert(kv);
        gcopy->parent = g->parent;
        std::vector<std::string> av = this->argv;
        std::string sd = this->script_dir;
        int64_t id;
        {
            std::lock_guard<std::mutex> lk(g_threads.m);
            id = g_threads.next_id++;
            g_threads.errors.erase(id);
            g_threads.results.erase(id);
            g_threads.threads[id] = std::thread([id, nameV, nid, argv, gcopy, av, sd]() {
                Interp sub(av, sd);
                sub.g = gcopy;
                try {
                    Value r = sub.callFuncByValues(nameV.s(), nid, argv, gcopy);
                    std::lock_guard<std::mutex> lk2(g_threads.m);
                    g_threads.results[id] = r;
                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lk2(g_threads.m);
                    g_threads.errors[id] = std::string(e.what());
                }
            });
        }
        return mkInt(id);
    }
    case 149: {  // -thread_join(id)
        Value idV = ev(0);
        if (idV.t() != Value::T::INT) throw VesnaError("-thread_join 需要线程 id");
        int64_t id = idV.i();
        std::thread t;
        {
            std::lock_guard<std::mutex> lk(g_threads.m);
            auto it = g_threads.threads.find(id);
            if (it == g_threads.threads.end())
                throw VesnaError("-thread_join 线程不存在: " + std::to_string(id));
            t = std::move(it->second);
            g_threads.threads.erase(it);
        }
        if (t.joinable()) t.join();
        std::lock_guard<std::mutex> lk2(g_threads.m);
        auto eit = g_threads.errors.find(id);
        if (eit != g_threads.errors.end()) {
            std::string err = eit->second;
            g_threads.errors.erase(eit);
            throw VesnaError(err);
        }
        Value r = mkNone();
        auto rit = g_threads.results.find(id);
        if (rit != g_threads.results.end()) { r = rit->second; g_threads.results.erase(rit); }
        return r;
    }
    case 150: {  // -thread_count()
        std::lock_guard<std::mutex> lk(g_threads.m);
        return mkInt((int64_t)g_threads.threads.size());
    }
    case 151: {  // -lock(name)
        Value nV = ev(0);
        if (nV.t() != Value::T::STR) throw VesnaError("-lock 需要名称字符串");
        std::mutex* m;
        {
            std::lock_guard<std::mutex> lk(g_locks_m);
            auto it = g_locks.find(nV.s());
            if (it == g_locks.end()) {
                m = new std::mutex();
                g_locks[nV.s()] = m;
            } else m = it->second;
        }
        m->lock();
        return mkNone();
    }
    case 152: {  // -unlock(name)
        Value nV = ev(0);
        if (nV.t() != Value::T::STR) throw VesnaError("-unlock 需要名称字符串");
        std::mutex* m;
        {
            std::lock_guard<std::mutex> lk(g_locks_m);
            auto it = g_locks.find(nV.s());
            if (it == g_locks.end()) throw VesnaError("-unlock 未锁定: " + nV.s());
            m = it->second;
        }
        m->unlock();
        return mkNone();
    }

    // ---- 1.1 网络 ----
    case 153: {  // -http_get(url)
        Value u = ev(0);
        if (u.t() != Value::T::STR) throw VesnaError("-http_get 需要 URL 字符串");
        return mkStr(httpRequest(u.s(), "", false));
    }
    case 154: {  // -http_post(url; body)
        Value u = ev(0), b = ev(1);
        if (u.t() != Value::T::STR || b.t() != Value::T::STR)
            throw VesnaError("-http_post 需要 URL 与 body 字符串");
        return mkStr(httpRequest(u.s(), b.s(), true));
    }
    case 155: {  // -tcp_ping(host; port)
        Value h = ev(0), p = ev(1);
        if (h.t() != Value::T::STR || p.t() != Value::T::INT)
            throw VesnaError("-tcp_ping 需要主机字符串与端口整数");
        return mkInt(tcpPing(h.s(), (int)p.i()));
    }

    // ---- 1.1 二进制 ----
    case 156: {  // -bin_read(path) -> 字节列表
        Value p = ev(0);
        if (p.t() != Value::T::STR) throw VesnaError("-bin_read 需要路径字符串");
        std::ifstream f(std::filesystem::u8path(p.s()), std::ios::binary);
        if (!f) throw VesnaError("-bin_read 无法打开: " + p.s());
        std::vector<char> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        Value out = mkList();
        out.list()->items.reserve(buf.size());
        for (unsigned char ch : buf) out.list()->items.push_back(mkInt(ch));
        return out;
    }
    case 157: {  // -bin_write(path; bytes)
        Value p = ev(0), b = ev(1);
        if (p.t() != Value::T::STR) throw VesnaError("-bin_write 需要路径字符串");
        if (b.t() != Value::T::LIST && b.t() != Value::T::GROUP)
            throw VesnaError("-bin_write 需要字节列表");
        std::ofstream f(std::filesystem::u8path(p.s()), std::ios::binary);
        if (!f) throw VesnaError("-bin_write 无法写入: " + p.s());
        const auto& items = b.t() == Value::T::LIST ? b.list()->items : b.group()->items;
        for (auto& v : items) {
            if (v.t() != Value::T::INT || v.i() < 0 || v.i() > 255)
                throw VesnaError("-bin_write 字节须为 0-255 整数");
            f.put((char)v.i());
        }
        return mkNone();
    }
    case 158: {  // -bin_hex(bytes) -> 十六进制字符串
        Value b = ev(0);
        if (b.t() != Value::T::LIST && b.t() != Value::T::GROUP)
            throw VesnaError("-bin_hex 需要字节列表");
        const auto& items = b.t() == Value::T::LIST ? b.list()->items : b.group()->items;
        std::string out;
        static const char* hexd = "0123456789abcdef";
        for (auto& v : items) {
            int64_t byte = v.t() == Value::T::INT ? v.i() : 0;
            if (byte < 0 || byte > 255) throw VesnaError("-bin_hex 字节须为 0-255 整数");
            out += hexd[byte >> 4];
            out += hexd[byte & 15];
        }
        return mkStr(out);
    }
    case 159: {  // -bin_unhex(s) -> 字节列表
        Value s = ev(0);
        if (s.t() != Value::T::STR) throw VesnaError("-bin_unhex 需要十六进制字符串");
        const std::string& h = s.s();
        if (h.size() % 2 != 0) throw VesnaError("-bin_unhex 长度须为偶数");
        Value out = mkList();
        auto hexv = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        for (size_t i = 0; i < h.size(); i += 2) {
            int hi = hexv(h[i]), lo = hexv(h[i + 1]);
            if (hi < 0 || lo < 0) throw VesnaError("-bin_unhex 非法字符");
            out.list()->items.push_back(mkInt(hi * 16 + lo));
        }
        return out;
    }
    case 160: {  // -bin_base64_encode(bytes) -> 字符串
        Value b = ev(0);
        if (b.t() != Value::T::LIST && b.t() != Value::T::GROUP)
            throw VesnaError("-bin_base64_encode 需要字节列表");
        const auto& items = b.t() == Value::T::LIST ? b.list()->items : b.group()->items;
        std::string raw;
        for (auto& v : items) {
            if (v.t() != Value::T::INT || v.i() < 0 || v.i() > 255)
                throw VesnaError("-bin_base64_encode 字节须为 0-255 整数");
            raw.push_back((char)v.i());
        }
        return mkStr(base64EncodeStr(raw));
    }
    case 161: {  // -bin_base64_decode(s) -> 字节列表
        Value s = ev(0);
        if (s.t() != Value::T::STR) throw VesnaError("-bin_base64_decode 需要字符串");
        std::string raw = base64DecodeStr(s.s());
        Value out = mkList();
        for (unsigned char ch : raw) out.list()->items.push_back(mkInt(ch));
        return out;
    }

    // ---- 1.2 数据 / 加密 / 进程 / FFI ----
    case 162: {  // -json_encode(v)
        return mkStr(jsonWrite(ev(0)));
    }
    case 163: {  // -json_decode(s)
        Value s = ev(0);
        if (s.t() != Value::T::STR) throw VesnaError("-json_decode 需要字符串");
        JsonParser jp(s.s());
        Value out = jp.parseValue();
        if (jp.fail) throw VesnaError("-json_decode 解析失败");
        return out;
    }
    case 164: {  // -re_groups(s; pattern)
        Value s = ev(0), pat = ev(1);
        if (s.t() != Value::T::STR || pat.t() != Value::T::STR)
            throw VesnaError("-re_groups 需要字符串与正则");
        std::smatch m;
        Value out = mkList();
        if (std::regex_search(s.s(), m, cachedRegex(pat.s()))) {
            for (size_t i = 0; i < m.size(); ++i) {
                if (m[i].matched) out.list()->items.push_back(mkStr(m[i].str()));
                else out.list()->items.push_back(mkNone());
            }
        }
        return out;
    }
    case 165: {  // -sha256(s)
        Value s = ev(0);
        if (s.t() != Value::T::STR) throw VesnaError("-sha256 需要字符串");
        return mkStr(sha256Hex(s.s()));
    }
    case 166: {  // -aes_encrypt(data; key) -> base64
        Value d = ev(0), k = ev(1);
        if (d.t() != Value::T::STR || k.t() != Value::T::STR)
            throw VesnaError("-aes_encrypt 需要数据与密钥字符串");
        return mkStr(base64EncodeStr(aesEncryptCbc(d.s(), k.s())));
    }
    case 167: {  // -aes_decrypt(b64; key)
        Value d = ev(0), k = ev(1);
        if (d.t() != Value::T::STR || k.t() != Value::T::STR)
            throw VesnaError("-aes_decrypt 需要密文(base64)与密钥字符串");
        std::string raw = base64DecodeStr(d.s());
        if (raw.size() % 16 != 0) throw VesnaError("-aes_decrypt 密文长度非法");
        return mkStr(aesDecryptCbc(raw, k.s()));
    }
    case 168: {  // -proc_run(cmd) -> {exit; output}
        Value c = ev(0);
        if (c.t() != Value::T::STR) throw VesnaError("-proc_run 需要命令字符串");
        auto r = procRun(c.s());
        Value out = mkDict();
        out.dict()->pairs.emplace_back(mkStr("exit"), mkInt(r.first));
        out.dict()->pairs.emplace_back(mkStr("output"), mkStr(r.second));
        return out;
    }
    case 169: {  // -ffi_call(dll; func; args...)
        Value d = ev(0), f = ev(1);
        if (d.t() != Value::T::STR || f.t() != Value::T::STR)
            throw VesnaError("-ffi_call 需要 DLL 与函数名字符串");
        void* h = ffiLoad(d.s());
        if (!h) throw VesnaError("-ffi_call 无法加载库: " + d.s());
        void* fn = ffiSym(h, f.s());
        if (!fn) throw VesnaError("-ffi_call 未找到符号: " + f.s());
        int n = (int)argc() - 2;
        if (n > 6) throw VesnaError("-ffi_call 最多 6 个参数");
        std::vector<int64_t> p((size_t)n, 0);
        std::vector<std::string> bufs;
        for (int i = 0; i < n; ++i) {
            Value a = ev(i + 2);
            if (a.t() == Value::T::INT) p[(size_t)i] = a.i();
            else if (a.t() == Value::T::STR) {
                bufs.push_back(a.s());
                p[(size_t)i] = (int64_t)(intptr_t)bufs.back().c_str();
            } else throw VesnaError("-ffi_call 参数仅支持 int / 字符串");
        }
        int64_t r = 0;
        switch (n) {
            case 0: r = ((int64_t(*)())fn)(); break;
            case 1: r = ((int64_t(*)(int64_t))fn)(p[0]); break;
            case 2: r = ((int64_t(*)(int64_t,int64_t))fn)(p[0], p[1]); break;
            case 3: r = ((int64_t(*)(int64_t,int64_t,int64_t))fn)(p[0], p[1], p[2]); break;
            case 4: r = ((int64_t(*)(int64_t,int64_t,int64_t,int64_t))fn)(p[0], p[1], p[2], p[3]); break;
            case 5: r = ((int64_t(*)(int64_t,int64_t,int64_t,int64_t,int64_t))fn)(p[0], p[1], p[2], p[3], p[4]); break;
            case 6: r = ((int64_t(*)(int64_t,int64_t,int64_t,int64_t,int64_t,int64_t))fn)(p[0], p[1], p[2], p[3], p[4], p[5]); break;
        }
        return mkInt(r);
    }

    case 170: {  // -csv_parse(s) -> [[字段,...],...]
        Value v = ev(0);
        if (v.t() != Value::T::STR) throw VesnaError("-csv_parse 需要字符串");
        auto rows = csvParse(v.s());
        Value out = mkList();
        for (auto& row : rows) {
            Value rv = mkList();
            for (auto& f : row) rv.list()->items.push_back(mkStr(f));
            out.list()->items.push_back(rv);
        }
        return out;
    }
    case 171: {  // -csv_build(rows) -> 字符串
        Value v = ev(0);
        if (v.t() != Value::T::LIST) throw VesnaError("-csv_build 需要列表");
        std::vector<std::vector<std::string>> rows;
        for (auto& rv : v.list()->items) {
            if (rv.t() != Value::T::LIST) throw VesnaError("-csv_build 行必须是列表");
            std::vector<std::string> row;
            for (auto& f : rv.list()->items) {
                if (f.t() != Value::T::STR) throw VesnaError("-csv_build 字段必须是字符串");
                row.push_back(f.s());
            }
            rows.push_back(std::move(row));
        }
        return mkStr(csvBuild(rows));
    }
    case 172: {  // -ini_read(path) -> {section: {key: val}}
        Value v = ev(0);
        if (v.t() != Value::T::STR) throw VesnaError("-ini_read 需要路径");
        std::string txt = readFileUtf8(v.s());
        auto data = iniParse(txt);
        Value out = mkDict();
        for (auto& [sec, kv] : data) {
            Value secV = mkDict();
            for (auto& [k, val] : kv) dictSet(*secV.dict(), mkStr(k), mkStr(val));
            dictSet(*out.dict(), mkStr(sec), secV);
        }
        return out;
    }
    case 173: {  // -ini_write(path; data) -> none
        Value p = ev(0), d = ev(1);
        if (p.t() != Value::T::STR || d.t() != Value::T::DICT)
            throw VesnaError("-ini_write 需要路径与字典");
        std::map<std::string, std::map<std::string, std::string>> data;
        for (auto& [sk, sv] : d.dict()->pairs) {
            if (sk.t() != Value::T::STR || sv.t() != Value::T::DICT)
                throw VesnaError("-ini_write 字典结构必须是 {字符串: {字符串: 字符串}}");
            std::map<std::string, std::string> kv;
            for (auto& [kk, kvv] : sv.dict()->pairs) {
                if (kk.t() != Value::T::STR || kvv.t() != Value::T::STR)
                    throw VesnaError("-ini_write 键值必须是字符串");
                kv[kk.s()] = kvv.s();
            }
            data[sk.s()] = std::move(kv);
        }
        writeFileUtf8(p.s(), iniBuild(data), false);
        return mkNone();
    }
    case 174: {  // -xml_parse(s) -> {tag; attrs; children; text}（简易 DOM）
        Value v = ev(0);
        if (v.t() != Value::T::STR) throw VesnaError("-xml_parse 需要字符串");
        XmlNode root;
        if (!xmlParse(v.s(), root)) throw VesnaError("-xml_parse 无法解析 XML");
        std::function<Value(const XmlNode&)> toVal = [&](const XmlNode& node) -> Value {
            Value d = mkDict();
            dictSet(*d.dict(), mkStr("tag"), mkStr(node.tag));
            Value av = mkDict();
            for (auto& [k, val] : node.attrs) dictSet(*av.dict(), mkStr(k), mkStr(val));
            dictSet(*d.dict(), mkStr("attrs"), av);
            Value cv = mkList();
            for (auto& ch : node.children) cv.list()->items.push_back(toVal(ch));
            dictSet(*d.dict(), mkStr("children"), cv);
            dictSet(*d.dict(), mkStr("text"), mkStr(node.text));
            return d;
        };
        return toVal(root);
    }
    case 175: {  // -ffi_call_s(dll; func; args...) -> 返回 char* 的字符串
        Value d = ev(0), f = ev(1);
        if (d.t() != Value::T::STR || f.t() != Value::T::STR)
            throw VesnaError("-ffi_call_s 需要 DLL 与函数名字符串");
        void* h = ffiLoad(d.s());
        if (!h) throw VesnaError("-ffi_call_s 无法加载库: " + d.s());
        void* fn = ffiSym(h, f.s());
        if (!fn) throw VesnaError("-ffi_call_s 未找到符号: " + f.s());
        int n = (int)argc() - 2;
        if (n > 6) throw VesnaError("-ffi_call_s 最多 6 个参数");
        std::vector<int64_t> p((size_t)n, 0);
        std::vector<std::string> bufs;
        for (int i = 0; i < n; ++i) {
            Value a = ev(i + 2);
            if (a.t() == Value::T::INT) p[(size_t)i] = a.i();
            else if (a.t() == Value::T::STR) {
                bufs.push_back(a.s());
                p[(size_t)i] = (int64_t)(intptr_t)bufs.back().c_str();
            } else throw VesnaError("-ffi_call_s 参数仅支持 int / 字符串");
        }
        const char* r = nullptr;
        switch (n) {
            case 0: r = ((const char*(*)())fn)(); break;
            case 1: r = ((const char*(*)(int64_t))fn)(p[0]); break;
            case 2: r = ((const char*(*)(int64_t,int64_t))fn)(p[0], p[1]); break;
            case 3: r = ((const char*(*)(int64_t,int64_t,int64_t))fn)(p[0], p[1], p[2]); break;
            case 4: r = ((const char*(*)(int64_t,int64_t,int64_t,int64_t))fn)(p[0], p[1], p[2], p[3]); break;
            case 5: r = ((const char*(*)(int64_t,int64_t,int64_t,int64_t,int64_t))fn)(p[0], p[1], p[2], p[3], p[4]); break;
            case 6: r = ((const char*(*)(int64_t,int64_t,int64_t,int64_t,int64_t,int64_t))fn)(p[0], p[1], p[2], p[3], p[4], p[5]); break;
        }
        return mkStr(r ? std::string(r) : "");
    }
    case 176: {  // -call(fname; arg1; ...) 按名字调用函数（动态调用，fname 为函数名字符串）
        Value fnV = ev(0);
        if (fnV.t() != Value::T::STR)
            throw VesnaError("-call 第一个参数需要函数名字符串");
        std::vector<std::shared_ptr<Expr>> rest(args.begin() + 1, args.end());
        return call(fnV.s(), internId(fnV.s()), rest, env);
    }
    case 177: {  // -date_format(ts; fmt) 时间戳 -> 格式化字符串
        Value tV = ev(0);
        if (tV.t() != Value::T::INT) throw VesnaError("-date_format 需要整数时间戳");
        std::string fmt = "%Y-%m-%d %H:%M:%S";
        if (argc() > 1) {
            Value f = ev(1);
            if (f.t() != Value::T::STR) throw VesnaError("-date_format 格式需要字符串");
            fmt = f.s();
        }
        std::time_t t = (std::time_t)tV.i();
        std::tm tm = {};
        localtime_s(&tm, &t);
        char buf[256];
        std::strftime(buf, sizeof(buf), fmt.c_str(), &tm);
        return mkStr(buf);
    }
    case 178: {  // -parse_time(s; fmt) 格式化字符串 -> 时间戳（strptime 子集 %Y%m%d%H%M%S）
        Value sV = ev(0), fV = ev(1);
        if (sV.t() != Value::T::STR || fV.t() != Value::T::STR)
            throw VesnaError("-parse_time 需要字符串与格式");
        const std::string& s = sV.s();
        const std::string& fmt = fV.s();
        std::tm tm = {};
        tm.tm_isdst = -1;
        size_t si = 0, fi = 0;
        while (fi < fmt.size() && si < s.size()) {
            if (fmt[fi] == '%') {
                ++fi;
                if (fi >= fmt.size()) break;
                char c = fmt[fi];
                int val = 0;
                bool any = false;
                while (si < s.size() && !isdigit((unsigned char)s[si])) ++si;
                while (si < s.size() && isdigit((unsigned char)s[si])) {
                    val = val * 10 + (s[si] - '0');
                    ++si;
                    any = true;
                }
                if (!any) break;
                switch (c) {
                    case 'Y': tm.tm_year = val - 1900; break;
                    case 'm': tm.tm_mon = val - 1; break;
                    case 'd': tm.tm_mday = val; break;
                    case 'H': tm.tm_hour = val; break;
                    case 'M': tm.tm_min = val; break;
                    case 'S': tm.tm_sec = val; break;
                    default: break;
                }
            } else {
                ++fi;
            }
        }
        std::time_t t = std::mktime(&tm);
        if (t == (std::time_t)-1) throw VesnaError("-parse_time 解析失败");
        return mkInt((int64_t)t);
    }
    case 179: {  // -uuid() UUID v4
        unsigned char b[16] = {0};
#ifdef _WIN32
        RtlGenRandom(b, 16);
#else
        FILE* f = fopen("/dev/urandom", "rb");
        if (f) { (void)fread(b, 1, 16, f); fclose(f); }
        else { for (int i = 0; i < 16; ++i) b[i] = (unsigned char)(std::rand() & 255); }
#endif
        b[6] = (unsigned char)((b[6] & 0x0f) | 0x40);   // version 4
        b[8] = (unsigned char)((b[8] & 0x3f) | 0x80);   // variant 10xx
        static const char* hexd = "0123456789abcdef";
        std::string out;
        for (int i = 0; i < 16; ++i) {
            if (i == 4 || i == 6 || i == 8 || i == 10) out += '-';
            out += hexd[b[i] >> 4];
            out += hexd[b[i] & 15];
        }
        return mkStr(out);
    }
    case 180: {  // -http_server(port; handler) 阻塞式 HTTP 服务（单线程顺序处理）
        Value pV = ev(0), hV = ev(1);
        if (pV.t() != Value::T::INT || hV.t() != Value::T::STR)
            throw VesnaError("-http_server 需要端口整数与处理器函数名");
        int port = (int)pV.i();
        std::string handler = hV.s();
#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) throw VesnaError("-http_server WSAStartup 失败");
        SOCKET srv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (srv == INVALID_SOCKET) { WSACleanup(); throw VesnaError("-http_server 无法创建套接字"); }
        int opt = 1;
        setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons((u_short)port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            closesocket(srv); WSACleanup();
            throw VesnaError("-http_server 绑定失败: 端口 " + std::to_string(port) + " 被占用?");
        }
        if (listen(srv, 8) == SOCKET_ERROR) {
            closesocket(srv); WSACleanup(); throw VesnaError("-http_server listen 失败");
        }
        for (;;) {
            SOCKET cli = accept(srv, nullptr, nullptr);
            if (cli == INVALID_SOCKET) continue;
            std::string raw;
            char buf[4096];
            bool headerDone = false;
            while (!headerDone) {
                int n = recv(cli, buf, sizeof(buf), 0);
                if (n <= 0) break;
                raw.append(buf, (size_t)n);
                if (raw.find("\r\n\r\n") != std::string::npos) headerDone = true;
            }
            std::string method, path;
            size_t sp1 = raw.find(' ');
            size_t sp2 = (sp1 == std::string::npos) ? std::string::npos : raw.find(' ', sp1 + 1);
            if (sp1 != std::string::npos && sp2 != std::string::npos) {
                method = raw.substr(0, sp1);
                path = raw.substr(sp1 + 1, sp2 - sp1 - 1);
            }
            long clen = 0;
            {
                std::string lower = raw;
                for (auto& ch : lower) ch = (char)tolower((unsigned char)ch);
                size_t pos = lower.find("content-length:");
                if (pos != std::string::npos) {
                    pos += 15;
                    while (pos < lower.size() && (lower[pos] == ' ' || lower[pos] == '\t')) ++pos;
                    clen = atol(lower.c_str() + pos);
                }
            }
            std::string body;
            {
                size_t hb = raw.find("\r\n\r\n");
                if (hb != std::string::npos) {
                    size_t have = raw.size() - (hb + 4);
                    if (have < (size_t)clen) {
                        size_t need = (size_t)clen - have;
                        while (need > 0) {
                            int n = recv(cli, buf, (int)std::min<size_t>(need, sizeof(buf)), 0);
                            if (n <= 0) break;
                            raw.append(buf, (size_t)n);
                            need -= (size_t)n;
                        }
                    }
                    body = raw.substr(hb + 4, (size_t)clen);
                }
            }
            Value req = mkDict();
            dictSet(*req.dict(), mkStr("method"), mkStr(method));
            dictSet(*req.dict(), mkStr("path"), mkStr(path));
            size_t hb2 = raw.find("\r\n\r\n");
            dictSet(*req.dict(), mkStr("headers"),
                    mkStr(hb2 == std::string::npos ? raw : raw.substr(0, hb2)));
            dictSet(*req.dict(), mkStr("body"), mkStr(body));
            int code = 200;
            std::string respBody = "OK";
            std::string ctype = "text/plain; charset=utf-8";
            try {
                std::vector<std::shared_ptr<Expr>> callArgs;
                auto reqE = std::make_shared<Expr>();
                reqE->k = Expr::K::VAL;
                reqE->val = req;
                callArgs.push_back(reqE);
                Value r = call(handler, internId(handler), callArgs, env);
                if (r.t() == Value::T::STR) {
                    respBody = r.s();
                } else if (r.t() == Value::T::DICT) {
                    for (auto& [k, v] : r.dict()->pairs) {
                        if (k.t() == Value::T::STR && k.s() == "code" && v.t() == Value::T::INT) code = (int)v.i();
                        else if (k.t() == Value::T::STR && k.s() == "body" && v.t() == Value::T::STR) respBody = v.s();
                        else if (k.t() == Value::T::STR && k.s() == "type" && v.t() == Value::T::STR) ctype = v.s();
                    }
                } else throw VesnaError("-http_server 处理器必须返回字符串或字典");
            } catch (VesnaError& e) {
                code = 500;
                respBody = e.str();
                ctype = "text/plain; charset=utf-8";
            }
            std::string reason = code == 200 ? "OK" : (code == 404 ? "Not Found"
                                : (code == 500 ? "Internal Server Error" : "Error"));
            std::string resp = "HTTP/1.1 " + std::to_string(code) + " " + reason + "\r\n"
                + "Content-Type: " + ctype + "\r\n"
                + "Content-Length: " + std::to_string(respBody.size()) + "\r\n"
                + "Connection: close\r\n\r\n" + respBody;
            send(cli, resp.data(), (int)resp.size(), 0);
            closesocket(cli);
        }
#endif
        return mkNone();
    }
    case 181: {  // -file_time(path) 文件修改时间戳（秒）
        Value p = ev(0);
        if (p.t() != Value::T::STR) throw VesnaError("-file_time 需要路径字符串");
        std::error_code ec;
        auto ft = std::filesystem::last_write_time(std::filesystem::u8path(p.s()), ec);
        if (ec) throw VesnaError("-file_time 无法访问: " + p.s());
        auto sys = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ft - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        return mkInt((int64_t)std::chrono::duration_cast<std::chrono::seconds>(sys.time_since_epoch()).count());
    }
    case 182: {  // -truncate(path; size) 截断/扩展到指定字节数
        Value p = ev(0), sz = ev(1);
        if (p.t() != Value::T::STR || sz.t() != Value::T::INT)
            throw VesnaError("-truncate 需要路径字符串与整数大小");
        if (sz.i() < 0) throw VesnaError("-truncate 大小不能为负");
        if (sz.i() == 0) {
            std::ofstream f(std::filesystem::u8path(p.s()), std::ios::binary | std::ios::trunc);
            if (!f) throw VesnaError("-truncate 无法打开: " + p.s());
            return mkNone();
        }
        std::ofstream f(std::filesystem::u8path(p.s()), std::ios::binary | std::ios::trunc);
        if (!f) throw VesnaError("-truncate 无法打开: " + p.s());
        f.seekp((std::streamoff)(sz.i() - 1));
        f.put('\0');
        f.close();
        return mkNone();
    }
    case 183: {  // -arch() 架构
#if defined(_M_X64) || defined(__x86_64__)
        return mkStr("x64");
#elif defined(_M_ARM64) || defined(__aarch64__)
        return mkStr("arm64");
#elif defined(_M_IX86) || defined(__i386__)
        return mkStr("x86");
#else
        return mkStr("unknown");
#endif
    }


    }
    throw VesnaError("未知内置 -" + name);
}

// ============================================================
// 入口
// ============================================================
std::string findVesnaHome() {
    const char* env = getenv("VESNA_HOME");
    if (env && *env) return env;
    std::string exe_dir = exeDir();
    if (exe_dir.empty()) return ".";
    size_t pos2 = exe_dir.find_last_of("/\\");
    return (pos2 == std::string::npos) ? exe_dir : exe_dir.substr(0, pos2);
}

int runSource(const std::string& src, const std::vector<std::string>& argv,
              const std::string& script_dir, const std::string& filename, bool catch_exit,
              bool dbg, const std::string& dbg_file) {
    Parser parser(preprocess(src), filename);
    auto program = parser.parse();
    if (!parser.errors.empty()) {
        auto [line, msg] = parser.errors[0];
        throw VesnaError(msg, line);
    }
    Interp interp(argv, script_dir);
    interp.dbg = dbg;
    interp.dbg_file = dbg_file.empty() ? filename : dbg_file;
    if (dbg) interp.dbg_mode = 1;   // --debug 启动即暂停在第一语句
    try {
        interp.run(program);
    } catch (ExitSignal& e) {
        if (catch_exit) return e.code;
        std::exit(e.code);
    } catch (BreakSignal&) {
        throw VesnaError("break/continue 只能用于循环内");
    } catch (ContinueSignal&) {
        throw VesnaError("break/continue 只能用于循环内");
    }
    return 0;
}

int runFile(const std::string& path, const std::vector<std::string>& argv) {
    std::string src = readFileUtf8(path);
    return runSource(src, argv, parentDir(path), path, true);
}

int runFileDbg(const std::string& path, const std::vector<std::string>& argv) {
    std::string src = readFileUtf8(path);
    return runSource(src, argv, parentDir(path), path, true, true, path);
}

// REPL 行读取（Windows 逐字符），Tab 补全内置名
static std::string replReadLine() {
    std::string line;
    while (true) {
        int ch = _getch();
        if (ch == '\r' || ch == '\n') {
            std::cout << "\n";
            return line;
        }
        if (ch == '\t') {
            // 找行内最后一个以 # 开头的 token
            size_t pos = line.find_last_of(" \t;(),[]{}");
            std::string tok = (pos == std::string::npos) ? line : line.substr(pos + 1);
            if (tok.size() >= 2 && tok[0] == '#') {
                std::string prefix = tok.substr(1);
                std::vector<std::string> hits;
                for (auto& nm : g_builtinNames) {
                    if (nm.first.compare(0, prefix.size(), prefix) == 0)
                        hits.push_back("#" + nm.first);
                }
                if (hits.size() == 1) {
                    std::string rest = hits[0].substr(tok.size());
                    std::cout << rest;
                    line += rest;
                } else if (hits.size() > 1) {
                    std::cout << "\n";
                    for (auto& h : hits) std::cout << h << "  ";
                    std::cout << "\n>>> " << line;
                } else {
                    // 无匹配：不打扰
                }
            }
            continue;
        }
        if (ch == '\b' || ch == 127) {
            if (!line.empty()) { line.pop_back(); std::cout << "\b \b"; }
            continue;
        }
        if (ch == 3) {  // Ctrl+C：清行
            std::cout << "^C\n>>> ";
            line.clear();
            continue;
        }
        line += (char)ch;
        std::cout << (char)ch;
    }
}

void repl() {
    std::cout << "Vesna " << VERSION << " — Scripts of spring\n输入空行退出\n\n";
    std::string buf;
    auto interp = std::make_shared<Interp>();
    while (true) {
        std::cout << (buf.empty() ? ">>> " : "... ");
        std::cout.flush();
        std::string line = replReadLine();
        if (std::cin.eof()) { std::cout << "\n"; break; }
        if (line.empty() && buf.empty()) break;
        if (!buf.empty()) buf += "\n";
        buf += line;
        try {
            Parser parser(preprocess(buf), "<repl>");
            auto program = parser.parse();
            for (auto& s : program) interp->exec(s, interp->g);
            buf.clear();
        } catch (VesnaError& e) {
            std::string m = e.str();
            if (m.find("未闭合") != std::string::npos) continue;
            std::cout << "错误: " << m << "\n";
            buf.clear();
        } catch (BreakSignal&) {
            std::cout << "错误: break/continue 只能用于循环内\n";
            buf.clear();
        } catch (ContinueSignal&) {
            std::cout << "错误: break/continue 只能用于循环内\n";
            buf.clear();
        } catch (ReturnSignal&) {
            std::cout << "错误: back 只能用于函数内\n";
            buf.clear();
        }
    }
}

}  // namespace vesna