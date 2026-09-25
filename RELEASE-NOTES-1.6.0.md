# Vesna 1.6.0 Release Notes / 发布说明

> 中文见下半部分。English first.

## English

Vesna 1.6.0 expands the language toward a general-purpose scripting language with web serving, time handling and file utilities — no syntax changes, all `#` builtins in the same lowercase-underscore style.

### What's new (7 builtins, dispatch 177-183)
- **`#http_server(port; "handler")`** — blocking HTTP server on native sockets. Per request it builds `req = {method; path; headers; body}` and invokes the named handler; the handler returns a string (HTTP 200) or `{code; body; type}` for status/type control. Perfect for tiny web services / mock servers.
- **`#date_format(ts; fmt)`** — format any unix timestamp (`%Y-%m-%d %H:%M:%S` default).
- **`#parse_time(s; fmt)`** — parse a formatted string back to a timestamp (`%Y %m %d %H %M %S` subset, tolerant of separators).
- **`#uuid()`** — random UUID v4.
- **`#file_time(path)`** — last-modified time in unix seconds.
- **`#truncate(path; size)`** — truncate or extend a file to `size` bytes.
- **`#arch()`** — `"x64"` / `"arm64"` / `"x86"` / `"unknown"`.

Total: **183 builtins**.

### Example
```vesna
def handler(req)-
-path = req["path"],
-if path == "/"-
--back({code: '200'; body: "hello from vesna"; type: "text/html"}),
-back({code: '404'; body: "not found"}),
#http_server('8080'; "handler"),
```

### Verify
- Golden regression (tier3/binary/tier4/examples) byte-identical
- LSP smoke 8/8 PASS
- http_server tested end-to-end: /hello 200 text/plain, /json 200 application/json, /other 404

---

## 中文

Vesna 1.6.0 把语言向通用脚本语言推进：Web 服务、时间处理、文件工具——不改语法，全部为 `#` 前缀、小写下划线风格的内置。

### 新增（7 个内置，dispatch 177-183）
- **`#http_server(port; "handler")`** —— 原生 socket 的阻塞式 HTTP 服务。每个请求构造 `req = {method; path; headers; body}` 并按名调用 handler；handler 返回字符串（HTTP 200）或 `{code; body; type}` 控制状态与类型。适合微型 Web 服务 / 模拟服务器。
- **`#date_format(ts; fmt)`** —— 格式化任意 unix 时间戳（默认 `%Y-%m-%d %H:%M:%S`）。
- **`#parse_time(s; fmt)`** —— 把格式化字符串解析回时间戳（`%Y %m %d %H %M %S` 子集，容忍分隔符）。
- **`#uuid()`** —— 随机 UUID v4。
- **`#file_time(path)`** —— 文件最后修改时间（unix 秒）。
- **`#truncate(path; size)`** —— 将文件截断 / 扩展为 `size` 字节。
- **`#arch()`** —— `"x64"` / `"arm64"` / `"x86"` / `"unknown"`。

内置总数：**183**。

### 示例
```vesna
def handler(req)-
-path = req["path"],
-if path == "/"-
--back({code: '200'; body: "hello from vesna"; type: "text/html"}),
-back({code: '404'; body: "not found"}),
#http_server('8080'; "handler"),
```

### 验证
- Golden 回归（tier3/binary/tier4/examples）字节级一致
- LSP 冒烟 8/8 通过
- http_server 端到端实测：/hello 200 text/plain、/json 200 application/json、/other 404
