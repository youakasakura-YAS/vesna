# -*- coding: utf-8 -*-
"""1.9.0 收尾 v2：tmLanguage + CHANGELOG + RELEASE-NOTES + README（builtins 已 patch 过）"""
import io, json

# ---- 2) tmLanguage：直接改原文件文本 ----
p = r'F:\Vesna\vesna-vscode\syntaxes\vesna.tmLanguage.json'
c = io.open(p, encoding='utf-8').read()
old = '|http_server|file_time|truncate|arch)\\\\b'
new = '|http_server|file_time|truncate|arch|tcp_connect|tcp_listen|tcp_accept|tcp_send|tcp_recv|tcp_close|encrypt_file|decrypt_file|encrypt_dir|decrypt_dir)\\\\b'
assert old in c, 'tmLanguage anchor'
c = c.replace(old, new)
io.open(p, 'w', encoding='utf-8', newline='\n').write(c)
print('tmLanguage updated')

# ---- 3) CHANGELOG 双语 ----
en = '''## 1.9.0

### Added
- **TCP sockets (native, no third-party deps)**: `#tcp_connect(host; port)`, `#tcp_listen(port)`, `#tcp_accept(srv)`, `#tcp_send(sock; data)`, `#tcp_recv(sock; maxlen)`, `#tcp_close(sock)` — listen / accept / send / receive for building custom servers and clients; handles are plain ints, work across `#thread` workers (Windows winsock + POSIX sockets)
- **File / folder encryption**: `#encrypt_file(path; key)` / `#decrypt_file(path; key)` encrypt/restore a single file in place (AES-256-CBC, base64 ciphertext); `#encrypt_dir(dir; key)` / `#decrypt_dir(dir; key)` recurse a whole folder; `VSENC1` magic prefix makes wrong-key decryption fail loudly
- VSCode extension 0.9.0 (syntax highlighting for the 10 new builtins)

## 1.8.0
'''
zh = '''## 1.9.0

### 新增
- **TCP 套接字（原生，无第三方依赖）**：`#tcp_connect(host; port)`、`#tcp_listen(port)`、`#tcp_accept(srv)`、`#tcp_send(sock; data)`、`#tcp_recv(sock; maxlen)`、`#tcp_close(sock)` —— 监听 / 接受 / 收发，可自建服务端与客户端；句柄是普通整数，可跨 `#thread` 线程使用（Windows winsock + POSIX socket）
- **文件 / 文件夹加解密**：`#encrypt_file(path; key)` / `#decrypt_file(path; key)` 原地加密/还原单个文件（AES-256-CBC，base64 密文）；`#encrypt_dir(dir; key)` / `#decrypt_dir(dir; key)` 递归处理整个目录；`VSENC1` 魔数前缀让错误密钥解密直接报错
- VSCode 插件 0.9.0（10 个新内置的高亮）

## 1.8.0
'''
for p, block in [(r'F:\Vesna\CHANGELOG.md', en), (r'F:\Vesna\CHANGELOG.zh-CN.md', zh)]:
    c = io.open(p, encoding='utf-8').read()
    assert '## 1.8.0' in c
    c = c.replace('## 1.8.0', block, 1)
    io.open(p, 'w', encoding='utf-8', newline='\n').write(c)
    print('CHANGELOG updated', p)

# ---- 4) RELEASE-NOTES-1.9.0.md ----
notes = '''# Vesna 1.9.0 Release Notes / 发布说明

> 中文见下半部分。English first.

## English

Vesna 1.9.0 adds native TCP sockets and whole-file / whole-folder AES encryption — no third-party dependencies.

### TCP sockets (native)
- `#tcp_connect(host; port)` — connect to a server, returns the socket handle
- `#tcp_listen(port)` — bind + listen, returns the listening handle
- `#tcp_accept(srv)` — blocking accept, returns the client handle
- `#tcp_send(sock; data)` — send a full string, returns bytes sent
- `#tcp_recv(sock; maxlen)` — receive up to maxlen bytes (empty string on peer close)
- `#tcp_close(sock)` — close

Handles are plain ints: store them in lists/dicts, share them across `#thread` workers. Windows uses winsock, POSIX uses BSD sockets (compile-time branch).

### File / folder encryption (AES-256-CBC, in place)
- `#encrypt_file(path; key)` / `#decrypt_file(path; key)` — single file, in place; content becomes base64 ciphertext / is restored
- `#encrypt_dir(dir; key)` / `#decrypt_dir(dir; key)` — recursive over a whole folder, returns the file count
- Key is derived via SHA-256 (same as `#aes_encrypt`); ciphertext carries a `VSENC1` magic prefix so a wrong key fails loudly instead of silently producing garbage

### Verify
- TCP echo round-trip (listener + accept + recv + send) verified end to end
- File and folder encrypt -> decrypt round-trips byte-identical; wrong-key decrypt raises
- Full golden regression byte-identical; LSP smoke PASS

---

## 中文

Vesna 1.9.0 增加原生 TCP 套接字与整文件/整文件夹 AES 加密——无第三方依赖。

### TCP 套接字（原生）
- `#tcp_connect(host; port)` —— 连接服务器，返回套接字句柄
- `#tcp_listen(port)` —— bind + listen，返回监听句柄
- `#tcp_accept(srv)` —— 阻塞接受连接，返回客户端句柄
- `#tcp_send(sock; data)` —— 完整发送字符串，返回已发送字节数
- `#tcp_recv(sock; maxlen)` —— 接收至多 maxlen 字节（对端关闭返回空串）
- `#tcp_close(sock)` —— 关闭

句柄是普通整数：可存进列表/字典，可跨 `#thread` 线程共享。Windows 用 winsock，POSIX 用 BSD socket（编译期分支）。

### 文件 / 文件夹加解密（AES-256-CBC，原地操作）
- `#encrypt_file(path; key)` / `#decrypt_file(path; key)` —— 单文件原地加解密；内容变为 base64 密文 / 还原
- `#encrypt_dir(dir; key)` / `#decrypt_dir(dir; key)` —— 递归处理整个目录，返回文件数
- 密钥经 SHA-256 派生（与 `#aes_encrypt` 一致）；密文带 `VSENC1` 魔数前缀——错误密钥会直接报错，而不是静默产生乱码

### 验证
- TCP 回显往返（监听 + 接受 + 收 + 发）端到端实测
- 文件与文件夹加密 → 解密往返字节一致；错误密钥解密报错
- 全量 golden 回归字节级一致；LSP 冒烟通过
'''
io.open(r'F:\Vesna\RELEASE-NOTES-1.9.0.md', 'w', encoding='utf-8', newline='\n').write(notes)
print('RELEASE-NOTES-1.9.0.md created')

# ---- 5) README 双语 ----
def patch(path, pairs):
    c = io.open(path, encoding='utf-8').read()
    for old, new in pairs:
        assert old in c, f'anchor {path}: {old[:40]}'
        c = c.replace(old, new)
    io.open(path, 'w', encoding='utf-8', newline='\n').write(c)
    print('patched', path)

patch(r'F:\Vesna\README.md', [
    ('![version](https://img.shields.io/badge/version-1.8.0-blue)',
     '![version](https://img.shields.io/badge/version-1.9.0-blue)'),
    ('Grab `vesna-1.8.0-windows-x64.zip` from [Releases](../../releases) and extract it anywhere.',
     'Grab `vesna-1.9.0-windows-x64.zip` from [Releases](../../releases) and extract it anywhere.'),
    ('hardened package manager + LSP go-to-definition/signature hints',
     'native TCP sockets (`#tcp_listen`/`#tcp_send`/...) and file/folder encryption (`#encrypt_file`/`#encrypt_dir`); hardened package manager + LSP go-to-definition/signature hints'),
])
patch(r'F:\Vesna\README.zh-CN.md', [
    ('![version](https://img.shields.io/badge/version-1.8.0-blue)',
     '![version](https://img.shields.io/badge/version-1.9.0-blue)'),
    ('从 [Releases](../../releases) 下载 `vesna-1.8.0-windows-x64.zip` 并解压到任意目录。',
     '从 [Releases](../../releases) 下载 `vesna-1.9.0-windows-x64.zip` 并解压到任意目录。'),
    ('加固的包管理器 + LSP 跳转定义/签名提示',
     '原生 TCP 套接字（`#tcp_listen`/`#tcp_send`/...）与文件/文件夹加解密（`#encrypt_file`/`#encrypt_dir`）；加固的包管理器 + LSP 跳转定义/签名提示'),
])

p = r'F:\Vesna\vesna-vscode\package.json'
d = json.load(io.open(p, encoding='utf-8'))
print('vsix version:', d['version'])
