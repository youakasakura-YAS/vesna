# -*- coding: utf-8 -*-
"""1.9.0 收尾：builtins + tmLanguage + CHANGELOG + RELEASE-NOTES + README"""
import io, json

# ---- 1) builtins 双语 ----
en_sec = '''
## 1.9 Networking / File encryption

### TCP sockets (native)

| Function | Description |
|---|---|
| `#tcp_connect(host; port)` | connect to a TCP server, return the socket handle (int) |
| `#tcp_listen(port)` | bind + listen on a port, return the listening socket handle (int) |
| `#tcp_accept(srv)` | blocking accept on a listening socket, return the client socket handle (int) |
| `#tcp_send(sock; data)` | send the whole string over the socket, return bytes sent (int) |
| `#tcp_recv(sock; maxlen)` | receive up to `maxlen` bytes, return them as a string; empty string when the peer closed |
| `#tcp_close(sock)` | close a socket |

Example (echo server):

```
srv = #tcp_listen('9000'),
cli = #tcp_accept(srv),
msg = #tcp_recv(cli; '4096'),
#tcp_send(cli; "echo: " + msg),
#tcp_close(cli),
#tcp_close(srv),
```

Sockets are plain ints, so you can store them in lists/dicts and pass them between `#thread` workers.

### File / folder encryption (AES-256-CBC, in place)

| Function | Description |
|---|---|
| `#encrypt_file(path; key)` | encrypt a file in place; content is replaced by base64 ciphertext; returns `true` |
| `#decrypt_file(path; key)` | restore a file encrypted by `#encrypt_file`; wrong key or non-Vesna data raises an error; returns `true` |
| `#encrypt_dir(dir; key)` | recursively encrypt every file under a directory in place, return the number of files processed |
| `#decrypt_dir(dir; key)` | recursively restore files encrypted by `#encrypt_dir`, return the number of files restored |

Notes: key is derived via SHA-256 (same as `#aes_encrypt`); encrypted files carry a `VSENC1` magic prefix so wrong-key decrypts are detected. Operation is in place — back up before batch runs if needed.
'''
zh_sec = '''
## 1.9 网络 / 文件加解密

### TCP 套接字（原生）

| 函数 | 说明 |
|---|---|
| `#tcp_connect(host; port)` | 连接 TCP 服务器，返回套接字句柄（整数） |
| `#tcp_listen(port)` | 在端口上 bind + listen，返回监听句柄（整数） |
| `#tcp_accept(srv)` | 在监听句柄上阻塞接受连接，返回客户端句柄（整数） |
| `#tcp_send(sock; data)` | 把字符串完整发送到套接字，返回已发送字节数（整数） |
| `#tcp_recv(sock; maxlen)` | 接收至多 `maxlen` 字节并以字符串返回；对端关闭时返回空串 |
| `#tcp_close(sock)` | 关闭套接字 |

回显服务示例：

```
srv = #tcp_listen('9000'),
cli = #tcp_accept(srv),
msg = #tcp_recv(cli; '4096'),
#tcp_send(cli; "echo: " + msg),
#tcp_close(cli),
#tcp_close(srv),
```

句柄就是普通整数，可存进列表/字典，也可跨 `#thread` 线程传递。

### 文件 / 文件夹加解密（AES-256-CBC，原地操作）

| 函数 | 说明 |
|---|---|
| `#encrypt_file(path; key)` | 原地加密文件；内容替换为 base64 密文；返回 `true` |
| `#decrypt_file(path; key)` | 还原 `#encrypt_file` 加密的文件；密钥错误或非 Vesna 密文会报错；返回 `true` |
| `#encrypt_dir(dir; key)` | 递归原地加密目录下所有文件，返回处理文件数 |
| `#decrypt_dir(dir; key)` | 递归还原 `#encrypt_dir` 加密的文件，返回还原文件数 |

说明：密钥经 SHA-256 派生（与 `#aes_encrypt` 一致）；密文带 `VSENC1` 魔数前缀，错误密钥解密会被识别。操作为原地覆盖——批量执行前如需可先备份。
'''

def insert(path, sec, anchor):
    c = io.open(path, encoding='utf-8').read()
    assert anchor in c, path
    c = c.replace(anchor, sec + '\n' + anchor, 1)
    io.open(path, 'w', encoding='utf-8', newline='\n').write(c)
    print('patched', path)

insert(r'F:\Vesna\docs\builtins.md', en_sec, '## Full example')
insert(r'F:\Vesna\docs\builtins.zh-CN.md', zh_sec, '## 完整例子')

# ---- 2) tmLanguage：追加 10 个内置 ----
p = r'F:\Vesna\vesna-vscode\syntaxes\vesna.tmLanguage.json'
d = json.load(io.open(p, encoding='utf-8'))
s = json.dumps(d, ensure_ascii=False)
old = '|http_server|file_time|truncate|arch)\\b'
new = '|http_server|file_time|truncate|arch|tcp_connect|tcp_listen|tcp_accept|tcp_send|tcp_recv|tcp_close|encrypt_file|decrypt_file|encrypt_dir|decrypt_dir)\\b'
assert old in s, 'tmLanguage anchor'
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8', newline='\n').write(s)
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
