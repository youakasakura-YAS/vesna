# Vesna

<img src="icon.png" width="128" alt="Vesna">

> *Scripts of spring*
>
> **文档站**：https://youakasakura-YAS.github.io/vesna-docs/ （中文 | English）

[English](README.md) | [中文](README.zh-CN.md)

![version](https://img.shields.io/badge/version-1.5.0-blue)
![license](https://img.shields.io/badge/license-MIT-green)
![platform](https://img.shields.io/badge/platform-Windows-x64-lightgrey)

**本仓库是 Vesna 的 C++ 高性能实现** —— 一门轻量级文本处理脚本语言（文件后缀 `.ves`）。

- **定位**：文本处理 / 数据转换脚本
- **对标**：`awk`、`sed`、`jq`、`perl`
- **语言规范**：语义权威在 [Python 参考实现](https://github.com/youakasakura-YAS/vesna-py)（独立仓库）
- **VSCode 插件**：[vesna-vscode-extension](https://github.com/youakasakura-YAS/vesna-vscode-extension)（语法高亮 + LSP）
- **性能**：处理 10 万行日志约 240ms；183 个内置，含 HTTP 服务（`#http_server`）
- **语法**：缩进用 `-`，内置函数用 `#` 前缀，语句以 `,` 结尾

---

## 快速开始

### 1. 下载

从 [Releases](../../releases) 下载 `vesna-1.5.0-windows-x64.zip` 并解压到任意目录。

| 文件 | 说明 |
|---|---|
| `bin\vesna.exe` | C++ 解释器（静态链接，无 DLL 依赖） |
| `lib\` | 标准库（csv / json / text / stat） |
| `examples\` | 示例脚本 |
| `docs\` | 文档 |
| `README.md` / `CHANGELOG.md` / `LICENSE` | 发布说明 |

### 2. 安装（可选）

默认安装到 `C:\Vesna`：

```bat
vesna.exe --install
```

自定义路径：

```bat
vesna.exe --install D:\MyVesna
```

安装后任意目录都能运行 `vesna` 命令，并注册 `.ves` 文件关联。

### 3. 运行脚本

```bat
vesna hello.ves
vesna wc.ves sample.txt
vesna grep.ves "ERROR" log.txt
```

### 4. 调试脚本

```bat
vesna --debug hello.ves
```

交互命令：`c`/`continue` 继续，`n`/`next` 下一行，`s`/`step` 步入函数，`q`/`quit` 退出，`b <行>` 设断点，`del <行>` 删断点，`p <表达式>` 求值，`vars` 列变量，`bt` 调用栈回溯，`list` 显示源码，`help` 帮助。

### 5. 包管理器（vpm）

```bat
vesna --pkg init                 # 生成 vesna-pkg.json
vesna --pkg registry             # 缓存索引（默认：Vesna 包花园）
vesna --pkg install <目录|zip|owner:repo|包名>
vesna --pkg remove <名称>
vesna --pkg list
vesna --pkg search <关键词>
vesna --pkg publish              # 打包 entry + 元数据为 <名称>-<版本>.zip、计算 sha256、写入 registry-entry.json
```

`install <包名>` 会在缓存的 registry 索引中查找并下载包 zip。默认使用官方 registry——[Vesna 包花园](https://youakasakura-YAS.github.io/vesna-pkg/)。安装的包通过 `import <名称>` 从 `<VESNA_HOME>\packages\<名称>\<entry>`（`vesna-pkg.json` 的 `entry` 字段，自 1.5.0）导入。
自 1.3 起加入校验：包元数据检查（`name` 须匹配 `^[a-z][a-z0-9_-]+$`，`version` 须为 `x.y.z`，`entry` 必须存在）；registry 条目可携带 `sha256`，安装前校验哈希；依赖自动安装（含版本检查与循环保护）；`remove` 会拒绝卸载仍被其他已装包依赖的包（`--force` 可强制）。

---

## Hello, World

```vesna
print("Hello, World"),
```

### 一个完整的例子

读 CSV 文件，输出 JSON：

```vesna
import csv,
import json,

text = #fread("data.csv"),
rows = csv_parse(text),

header = rows['1'],
result = [],

i = '2',
while i <= #len(rows)-
-row = rows[i]
-obj = {}
-j = '1'
-while j <= #len(header)-
--obj[header[j]] = row[j]
--j += '1'
-#append(result; obj)
-i += '1',

print(json_write(result)),
```

*运行*：

```
vesna csv2json.ves data.csv
```

---

## 工具集

`examples\` 目录自带一组命令行工具：

| 脚本 | 作用 |
|---|---|
| `wc.ves` | 行/词/字符统计 |
| `grep.ves` | 正则过滤 |
| `head.ves` / `tail.ves` | 前/后 N 行 |
| `sort.ves` / `uniq.ves` | 排序 / 去重 |
| `cut.ves` | 按分隔符取列 |
| `sed.ves` / `replace.ves` | 正则/字面替换 |
| `stat.ves` | 词频统计 |
| `logstat.ves` | 日志级别统计 |
| `csv2json.ves` | CSV → JSON |
| `extract.ves` | 提取邮箱/URL/IP |

---

---

## 并发 / 网络 / 二进制（1.1）

第二/三梯队内置，不改语法、小写下划线命名，详见 [docs/builtins](docs/builtins.md)：

- **并发**：`#thread("fn"; arg...)`（线程隔离全局变量副本）、`#thread_join(id)`、`#thread_count()`、`#lock("name")` / `#unlock("name")`（命名互斥锁）
- **网络**（需 PATH 中有 curl）：`#http_get(url)`、`#http_post(url; body)`、`#tcp_ping(host; port)`（原生 TCP 探测）
- **二进制**：`#bin_read(path)` / `#bin_write(path; bytes)`、`#bin_hex` / `#bin_unhex`、`#bin_base64_encode` / `#bin_base64_decode`
Tier 3 (1.2) — **Data / Crypto / Process / FFI**:
- **数据**：`#json_encode` / `#json_decode`、`#re_groups`（正则捕获组）
- **加密**：`#sha256`、`#aes_encrypt` / `#aes_decrypt`（AES-256-CBC + PKCS7）
- **进程**：`#proc_run(cmd)`（返回 `{exit; output}`）
- **FFI**：`#ffi_call("dll"; "func"; arg...)` 直接调用系统库函数
## 标准库

模块放在 `C:\Vesna\lib\`（或随本仓库分发），用 `import xxx` 使用：

| 模块 | 内容 |
|---|---|
| `csv` | `csv_parse`、`csv_write` |
| `json` | `json_parse`、`json_write` |
| `text` | `text_word_count`、`text_extract_emails`、`text_extract_urls`、`text_extract_ips` |
| `stat` | `stat_count_by`、`stat_unique` |

---

## 性能

`vesna.exe` 与 [Python 参考实现](https://github.com/youakasakura-YAS/vesna-py) 对比：

| 基准 | Python | C++ |
|---|---|---|
| grep 10 万行日志 | ~2.8s | ~0.24s（约 12 倍） |
| 循环 1 万次求和 | ~229ms | ~28ms（约 8 倍） |

C++ 版优化（详见 [CHANGELOG.md](CHANGELOG.md)）：正则缓存、字面量快速路径、`Value` 判别联合、标识符 intern、内置函数哈希分发、`-O3 -flto` 编译、关闭 IO 同步。

---

## 从源码构建

### Windows（MinGW-w64，g++ 11+，Windows 10+）

```bat
cd src\cpp
g++ -std=c++17 -O3 -flto -static -Wall -Wextra vesna.cpp main.cpp -o vesna.exe -ladvapi32
```

### 跨平台（CMake，Windows / Linux / macOS）

```bat
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

源码树可移植：`src/cpp/platform.h` 抽象了 UTF-8/UTF-16 转换、shell、cwd、chdir、环境变量与临时目录。注册表内置（`-regwrite` / `-regdelete` / `-regenv`）在非 Windows 平台上报"当前平台不支持"。

### 回归测试

输出须与 `src/cpp/tests/golden/` 下的 golden 基准逐行一致（再生成命令见其 README）：

```bat
vesna.exe tests\regression.ves
vesna.exe tests\fs_test.ves
vesna.exe tests\smoke04.ves
```

---

## 语法速览

```vesna
/* 注释 */

/* 变量 */
name = "Vesna",
age = '1',

/* 列表、组、字典 */
items = ['1'; '2'; '3'],
point = ('1'; '2'),
config = {"host": "localhost"; "port": '8080'},

/* 条件 */
if age > '18'-
-print("adult"),
-elif age > '12'-
-print("teen"),
-else-
-print("child"),

/* 循环 */
for i in items-
-print(i),

i = '0',
while i < '3'-
-print(i),
-i += '1',

/* 函数 */
def greet(who)-
-back("Hello, " + who)
print(greet("Vesna")),

/* 模块 */
import json,
print(json_write(items)),
```

---

## 目录结构

```
src\
  cpp\              C++ 实现（vesna.hpp / vesna.cpp / main.cpp）
  cpp\tests\        回归测试资产
lib\                标准库（.ves）
examples\           示例脚本
docs\               文档
bench\              性能基准
```

Python 参考实现位于独立仓库：[youakasakura-YAS/vesna-py](https://github.com/youakasakura-YAS/vesna-py)。

---

## 文档

- [docs/syntax.md](docs/syntax.md) — 语法参考
- [docs/builtins.md](docs/builtins.md) — 内置函数参考
- [CHANGELOG.md](CHANGELOG.md) — 版本历史

---

## 设计原则

1. **文本优先** — 字符串、正则、文件读写是一等公民
2. **显式前缀** — 内置函数用 `#`，一眼区分用户函数
3. **缩进统一** — 块用 `-`，每层加一个
4. **语句清晰** — 语句用 `,` 分隔，块用 `-` 结尾
5. **小而够用** — 不追求大而全，聚焦文本处理

---

## 许可

[MIT](LICENSE)

名称来源：[薇斯纳](https://baike.mihoyo.com/ys/obc/content/509803/detail?bbs_presentation_style=no_header&visit_device=pc)
