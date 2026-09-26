# Vesna 1.9.0 Release Notes / 发布说明

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
