# Vesna

<img src="icon.png" width="128" alt="Vesna">

> *Scripts of spring*

![version](https://img.shields.io/badge/version-0.3.0-blue)
![license](https://img.shields.io/badge/license-MIT-green)
![platform](https://img.shields.io/badge/platform-Windows-x64-lightgrey)

Vesna 是一门轻量级文本处理脚本语言，文件后缀 `.ves`。

- **定位**：文本处理 / 数据转换脚本
- **对标**：`awk`、`sed`、`jq`、`perl`
- **双实现**：Python 参考实现 + C++ 高性能实现（同一语言、同一语义）
- **性能**：C++ 版处理 10 万行日志约 240ms，比 Python 版快约 12 倍
- **语法**：缩进用 `-`，内置函数用 `#` 前缀，语句用 `,` 分隔

---

## 快速开始

### 1. 下载

从 [Releases](../../releases) 下载 `vesna-0.3.0-windows-x64.zip` 并解压到任意目录。

包内包含：

| 文件 | 说明 |
|---|---|
| `vesna.exe` | Python 版解释器（PyInstaller 打包，依赖自带） |
| `vesna_cpp.exe` | C++ 版解释器（静态链接，无 DLL 依赖，推荐） |
| `lib\` | 标准库（csv / json / text / stat） |
| `examples\` | 示例脚本 |
| `docs\` | 文档 |
| `README.md` / `CHANGELOG.md` / `LICENSE` | 发布说明 |

### 2. 安装（可选）

默认安装到 `C:\Vesna`：

```bat
vesna_cpp.exe --install
```

装到自定义路径：

```bat
vesna_cpp.exe --install D:\MyVesna
```

安装后任意目录都能直接运行 `vesna` 命令，并注册 `.ves` 文件关联。

### 3. 运行脚本

```bat
vesna_cpp hello.ves
vesna_cpp wc.ves sample.txt
vesna_cpp grep.ves "ERROR" log.txt
```

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
vesna_cpp csv2json.ves data.csv
```

---

## 工具集

Vesna 自带一组命令行工具（`examples\` 目录）：

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

## 标准库

放在 `C:\Vesna\lib\`（或随仓库分发），用 `import xxx` 使用：

| 模块 | 内容 |
|---|---|
| `csv` | `csv_parse`、`csv_write` |
| `json` | `json_parse`、`json_write` |
| `text` | `text_word_count`、`text_extract_emails`、`text_extract_urls`、`text_extract_ips` |
| `stat` | `stat_count_by`、`stat_unique` |

---

## 性能

C++ 版（`vesna_cpp.exe`）相对 Python 版（`vesna.exe`）的性能对比：

| 基准 | Python 版 | C++ 版 |
|---|---|---|
| grep 10 万行日志 | ~2.8s | ~0.24s（约 12 倍） |
| 循环 1 万次求和 | ~229ms | ~28ms（约 8 倍） |

C++ 版累计优化（详见 [CHANGELOG.md](CHANGELOG.md)）：正则缓存、字面量快速路径、Value 判别联合、标识符 intern、内置函数哈希分发、`-O3 -flto` 编译、IO 同步关闭。

---

## 从源码构建（C++ 版）

需要 [MinGW-w64](https://www.mingw-w64.org/)（g++ 11+，Windows 10+）：

```bat
cd src\cpp
g++ -std=c++17 -O3 -flto -static -Wall -Wextra vesna.cpp main.cpp -o vesna_cpp.exe -ladvapi32
```

回归测试（对照 Python 版逐行一致）：

```bat
vesna_cpp.exe tests\regression.ves
vesna_cpp.exe tests\fs_test.ves
python ..\vesna.py tests\regression.ves   # 输出应与 C++ 版一致
```

---

## VSCode 插件

`vesna-vscode\` 目录是 VSCode 扩展源码（语法高亮 + LSP 诊断/补全/快速修复）。

安装打包好的插件：

```bat
code --install-extension vesna-vscode\vesna-0.3.0.vsix
```

或在 VSCode 扩展面板 → `...` → **从 VSIX 安装**。LSP 需要系统 PATH 中有 `python`。

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
F:\Vesna\
  src\
    vesna.py         Python 参考实现
    cpp\             C++ 高性能实现（vesna.hpp / vesna.cpp / main.cpp）
    cpp\tests\       回归测试资产
  lib\               标准库（.ves）
  examples\          示例脚本和数据
  docs\              文档
  bench\             性能基准
  vesna-vscode\      VSCode 插件源码
  release\           发布包（zip）
```

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
