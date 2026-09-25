// lsp.cpp — Vesna 原生 LSP 服务器（stdio JSON-RPC，0.5.0）
// 由 vesna.exe --lsp 启动；诊断复用 C++ Parser（错误收集），
// 补全 / 悬停 / 符号 / 折叠基于文本与内置注册表实现。
#include "vesna.hpp"

#include <cstdio>
#include <cstring>
#include <cctype>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <regex>
#include <algorithm>
#include <iostream>

namespace vesna {

namespace {

// ---------- JSON 输出辅助 ----------

std::string jsonEscape(const std::string& s) {
    std::string out = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
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

struct Doc {
    std::string text;
};

std::map<std::string, Doc> g_docs;  // uri -> text

// ---------- 诊断 ----------

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::string cur;
    for (char c : text) {
        if (c == '\n') { lines.push_back(cur); cur.clear(); }
        else if (c != '\r') cur += c;
    }
    lines.push_back(cur);
    return lines;
}

struct Diag {
    int line;              // 0-based
    std::string msg;
};

std::vector<Diag> diagnose(const std::string& text) {
    std::vector<Diag> diags;
    std::set<std::pair<int, std::string>> seen;

    auto add = [&](int line_no, const std::string& msg) {
        int line = std::max(0, (line_no > 0 ? line_no : 1) - 1);
        if (!seen.insert({line, msg}).second) return;
        diags.push_back({line, msg});
    };

    // 1. 预处理
    std::vector<Line> lines;
    try {
        lines = preprocess(text);
    } catch (VesnaError& e) {
        add(e.line, e.msg);
        return diags;
    } catch (std::exception& e) {
        add(1, std::string("内部错误: ") + e.what());
        return diags;
    }

    // 2. 解析（Parser 收集全部错误，含词法错误）
    try {
        Parser parser(lines);
        parser.parse();
        for (auto& pe : parser.errors) add(pe.first, pe.second);
    } catch (std::exception& e) {
        add(1, std::string("内部错误: ") + e.what());
    }
    return diags;
}

// ---------- 补全 ----------

const std::vector<std::pair<std::string, std::string>> LSP_KEYWORDS = {
    {"if", "条件判断"}, {"elif", "else if 分支"}, {"else", "否则分支"},
    {"while", "while 循环"}, {"for", "for 循环"}, {"in", "遍历"},
    {"break", "跳出循环"}, {"continue", "继续下一次循环"},
    {"def", "定义函数"}, {"back", "函数返回"},
    {"try", "异常捕获"}, {"catch", "捕获异常"}, {"import", "导入模块"},
    {"and", "逻辑与"}, {"or", "逻辑或"}, {"not", "逻辑非"},
    {"true", "真"}, {"false", "假"},
};

const std::vector<std::pair<std::string, std::string>> LSP_TYPES = {
    {"int", "整数类型"}, {"str", "字符串类型"}, {"float", "浮点类型"},
    {"list", "列表类型"}, {"dict", "字典类型"}, {"bool", "布尔类型"},
};

// 内置文档（悬停用；补全 detail 缺省时用此表）
const std::map<std::string, std::string> LSP_HOVER = {
    {"json_encode", "`#json_encode(v)` → JSON 字符串。dict 保持插入顺序。"},
    {"json_decode", "`#json_decode(s)` → dict/list/int/float/str/bool/none；非法输入报错。"},
    {"re_groups", "`#re_groups(s; pattern)` → 首个正则匹配的捕获组列表；组 0 为整段，未匹配组为 none。"},
    {"sha256", "`#sha256(s)` → 64 位十六进制 SHA-256 摘要（FIPS 180-4 验证）。"},
    {"aes_encrypt", "`#aes_encrypt(data; key)` → AES-256-CBC+PKCS7 加密的 base64（密钥经 SHA-256 派生）。"},
    {"aes_decrypt", "`#aes_decrypt(b64; key)` → 用相同密钥解密 base64 密文。"},
    {"proc_run", "`#proc_run(cmd)` → `{\"exit\": 码, \"output\": stdout}`。"},
    {"ffi_call", "`#ffi_call(\"dll\"; \"func\"; arg...)` → 调用共享库 C 函数，返回 64 位整数。"},
    {"thread", "`#thread(函数)` → 新线程执行函数，返回线程 id。"},
    {"thread_join", "`#thread_join(id)` → 等待线程结束并返回其返回值。"},
    {"thread_count", "`#thread_count()` → 存活线程数。"},
    {"lock", "`#lock(name)` → 获取命名互斥锁。"},
    {"unlock", "`#unlock(name)` → 释放命名互斥锁。"},
    {"http_get", "`#http_get(url)` → GET 请求响应文本。"},
    {"http_post", "`#http_post(url; body)` → POST 请求响应文本。"},
    {"tcp_ping", "`#tcp_ping(host; port)` → TCP 连通性测试。"},
    {"bin_read", "`#bin_read(path)` → 字节列表。"},
    {"bin_write", "`#bin_write(path; bytes)` → 写入字节列表。"},
    {"bin_hex", "`#bin_hex(bytes)` → 十六进制字符串。"},
    {"bin_unhex", "`#bin_unhex(s)` → 十六进制字符串还原为字节列表。"},
    {"bin_base64_encode", "`#bin_base64_encode(bytes)` → base64。"},
    {"bin_base64_decode", "`#bin_base64_decode(s)` → base64 还原为字节列表。"},
    {"fread", "`#fread(path)` → 读取整个文件文本。"},
    {"fwrite", "`#fwrite(path; text)` → 覆盖写入文件。"},
    {"fappend", "`#fappend(path; text)` → 追加写入文件。"},
    {"shell", "`#shell(cmd)` → 执行系统命令并返回输出。"},
    {"len", "`#len(v)` → 字符串/列表/dict 的长度。"},
    {"type", "`#type(v)` → 值的类型名。"},
    {"str", "`#str(v)` → 转字符串。"},
    {"int", "`#int(v)` → 转整数。"},
    {"float", "`#float(v)` → 转浮点。"},
    {"bool", "`#bool(v)` → 转布尔。"},
};

std::set<std::string> collectIdentifiers(const std::string& text) {
    std::set<std::string> ids;
    for (auto& line : splitLines(text)) {
        std::string l = line;
        size_t i = l.find("/*");
        if (i != std::string::npos) {
            size_t j = l.find("*/", i);
            l = l.substr(0, i) + (j != std::string::npos ? l.substr(j + 2) : "");
        }
        std::regex tokRe(R"([a-zA-Z_][a-zA-Z0-9_]*)");
        for (std::sregex_iterator it(l.begin(), l.end(), tokRe), end; it != end; ++it)
            ids.insert(it->str());
    }
    return ids;
}

std::string completionJson(const std::string& text) {
    std::string out = "[";
    bool first = true;
    auto addItem = [&](const std::string& label, const std::string& detail, int kind) {
        if (!first) out += ",";
        first = false;
        out += "{\"label\":" + jsonEscape(label) + ",\"detail\":" + jsonEscape(detail) +
               ",\"kind\":" + std::to_string(kind) + ",\"insertText\":" + jsonEscape(label) + "}";
    };
    for (auto& k : LSP_KEYWORDS) addItem(k.first, k.second, 14);
    for (auto& p : g_builtinNames) {
        std::string label = "#" + p.first;
        std::string detail = "内置";
        auto it = LSP_HOVER.find(p.first);
        if (it != LSP_HOVER.end()) detail = it->second;
        addItem(label, detail, 3);
    }
    for (auto& t : LSP_TYPES) addItem(t.first, t.second, 25);
    for (auto& name : collectIdentifiers(text)) {
        if (name == "true" || name == "false") continue;
        addItem(name, "变量", 6);
    }
    out += "]";
    return out;
}

// ---------- 悬停 ----------

std::string hoverJson(const std::string& text, int line, int character) {
    auto lines = splitLines(text);
    if (line < 0 || line >= (int)lines.size()) return "null";
    const std::string& lineText = lines[(size_t)line];
    if (character < 0) character = 0;
    std::string left = lineText.substr(0, (size_t)character);

    std::regex wordRe(R"([a-zA-Z_#][a-zA-Z0-9_#]*$)");
    std::smatch m;
    if (!std::regex_search(left, m, wordRe)) return "null";
    std::string word = m.str();

    std::string md;
    bool found = false;
    if (!word.empty() && word[0] == '#') {
        std::string key = word.substr(1);
        if (word == "#f") { md = "`#f\"...\"` → 插值字符串，`(变量)` 内插值。"; found = true; }
        else if (word == "#into") { md = "`#into(type; v)` → 显式类型转换。"; found = true; }
        else {
            auto it = LSP_HOVER.find(key);
            if (it != LSP_HOVER.end()) { md = it->second; found = true; }
            else {
                for (auto& k : LSP_KEYWORDS) (void)k;
                // 未收录文档的内置：给出一行通用说明
                bool known = false;
                for (auto& p : g_builtinNames) if (p.first == key) { known = true; break; }
                if (known) { md = "内置函数 `#" + key + "`。"; found = true; }
            }
        }
    } else {
        for (auto& k : LSP_KEYWORDS) {
            if (k.first == word) { md = k.second; found = true; break; }
        }
        if (!found) {
            std::regex defRe(R"(^\s*def\s+)" + word + R"(\b)");
            if (std::regex_search(lineText, defRe)) {
                md = "函数 `" + word + "`：" + lineText;
                found = true;
            }
        }
    }
    if (!found) return "null";

    std::string out = "{";
    out += "\"contents\":{\"kind\":\"markdown\",\"value\":" + jsonEscape("**`" + word + "`**\n\n" + md) + "},";
    out += "\"range\":{\"start\":{\"line\":" + std::to_string(line) +
           ",\"character\":" + std::to_string(std::max(0, character - (int)word.size())) +
           "},\"end\":{\"line\":" + std::to_string(line) +
           ",\"character\":" + std::to_string(character) + "}}}";
    return out;
}

// ---------- 符号 / 折叠 ----------

std::string symbolsJson(const std::string& text) {
    auto lines = splitLines(text);
    std::string out = "[";
    bool first = true;
    std::regex defRe(R"(^\s*def\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(?:\(|-))");
    for (size_t i = 0; i < lines.size(); ++i) {
        std::smatch m;
        if (std::regex_search(lines[i], m, defRe)) {
            if (!first) out += ",";
            first = false;
            int len = (int)lines[i].size();
            out += "{\"name\":" + jsonEscape(m.str(1)) + ",\"kind\":12,";
            out += "\"range\":{\"start\":{\"line\":" + std::to_string(i) + ",\"character\":0},";
            out += "\"end\":{\"line\":" + std::to_string(i) + ",\"character\":" + std::to_string(len) + "}},";
            out += "\"selectionRange\":{\"start\":{\"line\":" + std::to_string(i) + ",\"character\":0},";
            out += "\"end\":{\"line\":" + std::to_string(i) + ",\"character\":" + std::to_string(len) + "}}}";
        }
    }
    out += "]";
    return out;
}

std::string foldingJson(const std::string& text) {
    auto lines = splitLines(text);
    std::string out = "[";
    bool first = true;
    int depth = 0;
    int start = -1;
    auto emit = [&](int endLine) {
        if (start >= 0 && endLine - 1 > start) {
            if (!first) out += ",";
            first = false;
            out += "{\"startLine\":" + std::to_string(start) + ",\"startCharacter\":0,";
            out += "\"endLine\":" + std::to_string(endLine - 1) +
                   ",\"endCharacter\":" + std::to_string((int)lines[(size_t)(endLine - 1)].size()) + "}";
        }
    };
    for (size_t i = 0; i < lines.size(); ++i) {
        int d = 0;
        for (char ch : lines[i]) { if (ch == '-') ++d; else break; }
        if (d > depth && start < 0) { start = (int)i; depth = d; }
        else if (d < depth) {
            emit((int)i);
            depth = d;
            start = d > 0 ? (int)i : -1;
        }
    }
    if (start >= 0) emit((int)lines.size());
    out += "]";
    return out;
}

// ---------- JSON-RPC ----------

bool readMessage(std::string& out) {
    std::string headers;
    char line[4096];
    while (true) {
        if (!std::fgets(line, sizeof(line), stdin)) return false;
        std::string l = line;
        if (l == "\r\n" || l == "\n") break;
        headers += l;
    }
    size_t pos = headers.find("Content-Length:");
    if (pos == std::string::npos) return false;
    pos += 15;
    size_t end = headers.find("\n", pos);
    std::string lenStr = headers.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
    lenStr.erase(std::remove_if(lenStr.begin(), lenStr.end(), [](char c){ return c == ' ' || c == '\r' || c == '\t'; }), lenStr.end());
    int length = 0;
    try { length = std::stoi(lenStr); } catch (...) { return false; }
    if (length <= 0 || length > 64 * 1024 * 1024) return false;
    std::string body((size_t)length, '\0');
    size_t got = 0;
    while (got < (size_t)length) {
        size_t n = std::fread(&body[got], 1, (size_t)length - got, stdin);
        if (n == 0) return false;
        got += n;
    }
    out = body;
    return true;
}

void writeMessage(const std::string& body) {
        // CRT 文本模式会把 '\n' 自动转成 '\r\n'，因此这里只写 '\n'，
    // 否则会输出 '\r\r\n' 破坏 LSP framing
    std::string header = "Content-Length: " + std::to_string(body.size()) + "\n\n";
    std::fwrite(header.data(), 1, header.size(), stdout);
    std::fwrite(body.data(), 1, body.size(), stdout);
    std::fflush(stdout);
}

void sendResponse(int64_t id, const std::string& result) {
    writeMessage("{\"jsonrpc\":\"2.0\",\"id\":" + std::to_string(id) + ",\"result\":" + result + "}");
}

void sendNotification(const std::string& method, const std::string& params) {
    writeMessage("{\"jsonrpc\":\"2.0\",\"method\":" + jsonEscape(method) + ",\"params\":" + params + "}");
}

// 简易 JSON 取值（仅取字符串 / 整数 / 嵌套路径，LSP 请求所需）
std::string jsonFind(const std::string& json, const std::string& key, size_t from = 0) {
    size_t pos = json.find("\"" + key + "\"", from);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + key.size() + 2);
    if (pos == std::string::npos) return "";
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    if (pos < json.size() && json[pos] == '"') {
        std::string v;
        ++pos;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                char n = json[pos + 1];
                switch (n) {
                    case 'n': v += '\n'; pos += 2; break;
                    case 'r': v += '\r'; pos += 2; break;
                    case 't': v += '\t'; pos += 2; break;
                    case 'b': v += '\b'; pos += 2; break;
                    case 'f': v += '\f'; pos += 2; break;
                    case 'u': {
                        // \uXXXX -> UTF-8（仅基本多语言平面）
                        auto hex = [&](char h) -> int {
                            if (h >= '0' && h <= '9') return h - '0';
                            if (h >= 'a' && h <= 'f') return h - 'a' + 10;
                            if (h >= 'A' && h <= 'F') return h - 'A' + 10;
                            return -1;
                        };
                        int cp = 0;
                        for (int k = 1; k <= 4; ++k) {
                            int d = hex(json[pos + 1 + k]);
                            if (d < 0) { cp = -1; break; }
                            cp = cp * 16 + d;
                        }
                        pos += 6;
                        if (cp >= 0) {
                            if (cp < 0x80) v += (char)cp;
                            else if (cp < 0x800) {
                                v += (char)(0xC0 | (cp >> 6));
                                v += (char)(0x80 | (cp & 0x3F));
                            } else {
                                v += (char)(0xE0 | (cp >> 12));
                                v += (char)(0x80 | ((cp >> 6) & 0x3F));
                                v += (char)(0x80 | (cp & 0x3F));
                            }
                        }
                        break;
                    }
                    default: v += n; pos += 2; break;  // \\ \" \/ 等
                }
            }
            else v += json[pos++];
        }
        return v;
    }
    size_t start = pos;
    while (pos < json.size() && (std::isdigit((unsigned char)json[pos]) || json[pos] == '-')) ++pos;
    return json.substr(start, pos - start);
}

int64_t jsonFindInt(const std::string& json, const std::string& key, size_t from = 0) {
    std::string v = jsonFind(json, key, from);
    try { return std::stoll(v); } catch (...) { return 0; }
}

std::string diagnosticsJson(const std::vector<Diag>& diags) {
    std::string out = "[";
    for (size_t i = 0; i < diags.size(); ++i) {
        if (i) out += ",";
        out += "{\"range\":{\"start\":{\"line\":" + std::to_string(diags[i].line) +
               ",\"character\":0},\"end\":{\"line\":" + std::to_string(diags[i].line) +
               ",\"character\":9999}},\"severity\":1,\"source\":\"vesna\",\"message\":" +
               jsonEscape(diags[i].msg) + "}";
    }
    out += "]";
    return out;
}

}  // namespace

// ---------- 主循环 ----------

void runLsp() {
    while (true) {
        std::string body;
        if (!readMessage(body)) break;
        if (body.empty()) break;

        std::string method = jsonFind(body, "method");
        bool hasId = body.find("\"id\"") != std::string::npos;
        int64_t id = jsonFindInt(body, "id");

        if (method == "initialize") {
            std::string res =
                "{\"capabilities\":{"
                "\"textDocumentSync\":1,"
                "\"completionProvider\":{\"triggerCharacters\":[\"#\"]},"
                "\"hoverProvider\":true,\"codeActionProvider\":true,"
                "\"documentSymbolProvider\":true,\"foldingRangeProvider\":true},"
                "\"serverInfo\":{\"name\":\"vesna-lsp\",\"version\":\"1.4.0\"}}";
            sendResponse(id, res);
        }
        else if (method == "initialized") {
            // 无需处理
        }
        else if (method == "textDocument/didOpen") {
            std::string uri = jsonFind(body, "uri");
            std::string text = jsonFind(body, "text");
            g_docs[uri] = Doc{text};
            sendNotification("textDocument/publishDiagnostics",
                "{\"uri\":" + jsonEscape(uri) + ",\"diagnostics\":" + diagnosticsJson(diagnose(text)) + "}");
        }
        else if (method == "textDocument/didChange") {
            std::string uri = jsonFind(body, "uri");
            // contentChanges 最后一个元素的 text
            size_t pos = body.find("\"text\"", body.find("contentChanges"));
            std::string text = jsonFind(body, "text", body.find("contentChanges"));
            (void)pos;
            g_docs[uri] = Doc{text};
            sendNotification("textDocument/publishDiagnostics",
                "{\"uri\":" + jsonEscape(uri) + ",\"diagnostics\":" + diagnosticsJson(diagnose(text)) + "}");
        }
        else if (method == "textDocument/didClose") {
            std::string uri = jsonFind(body, "uri");
            g_docs.erase(uri);
            sendNotification("textDocument/publishDiagnostics",
                "{\"uri\":" + jsonEscape(uri) + ",\"diagnostics\":[]}");
        }
        else if (method == "textDocument/completion") {
            std::string uri = jsonFind(body, "uri");
            auto it = g_docs.find(uri);
            sendResponse(id, completionJson(it != g_docs.end() ? it->second.text : ""));
        }
        else if (method == "textDocument/hover") {
            std::string uri = jsonFind(body, "uri");
            size_t posLine = body.find("\"position\"");
            int64_t line = jsonFindInt(body, "line", posLine);
            int64_t character = jsonFindInt(body, "character", posLine);
            auto it = g_docs.find(uri);
            sendResponse(id, hoverJson(it != g_docs.end() ? it->second.text : "", (int)line, (int)character));
        }
        else if (method == "textDocument/documentSymbol") {
            std::string uri = jsonFind(body, "uri");
            auto it = g_docs.find(uri);
            sendResponse(id, symbolsJson(it != g_docs.end() ? it->second.text : ""));
        }
        else if (method == "textDocument/foldingRange") {
            std::string uri = jsonFind(body, "uri");
            auto it = g_docs.find(uri);
            sendResponse(id, foldingJson(it != g_docs.end() ? it->second.text : ""));
        }
        else if (method == "shutdown") {
            sendResponse(id, "null");
        }
        else if (method == "exit") {
            break;
        }
        else {
            if (hasId) sendResponse(id, "null");
        }
    }
}

}  // namespace vesna
