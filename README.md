# Vesna

<img src="icon.png" width="128" alt="Vesna">

> *Scripts of spring*


Vesna 是一门轻量级文本处理脚本语言。文件后缀 `.ves`。

- **定位**：文本处理 / 数据转换脚本
- **对标**：`awk`、`sed`、`jq`、`perl`
- **实现**：Python 编写的解释器
- **安装**：自带 `vesna` 命令，任何目录都能跑
- **语法**：缩进用 `-`，内置函数用 `#` 前缀，语句用 `,` 分隔

---

## 快速开始

### 安装

1. 把项目放到 `C:\Vesna\`
2. 设置环境变量：
   - `VESNA_HOME = C:\Vesna`
   - `PATH = %PATH%;C:\Vesna\bin`

### 运行脚本

```bat
vesna hello.ves
vesna wc.ves sample.txt
vesna grep.ves "ERROR" log.txt
```

---

### Hello, World

```vesna
print("Hello, World"),
```

### 一个完整的例子

##### 读 CSV 文件，输出 JSON：

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

### 工具集

vesna自带一部分没什么用的工具集

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

更多？ 我写那么多没用的干什么。

---

### 标准库

放在 ```C:\Vesna\lib\```，可以用 ```import xxx``` 使用

| 模块     | 内容 |
| ----------- | ----------- |
| ```csv```      | ```csv_parse```、```csv_write```       |
| ```json```   | ```json_parse```、```json_write```        |
| ```text```   | ```text_word_count```、```text_extract_emails``` 、  ```text_extract_urls```、```text_extract_ips```     |
| ```stat```   | ```stat_count_by```、```stat_unique```        |

---

### 语法速览

```
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

### 目录结构

```
C:\Vesna\
  bin\          命令行入口
  src\          解释器源码（vesna.py）
  lib\          标准库（.ves）
  examples\     示例脚本和数据
  docs\         文档
  README.md
```

---

### 文档

- 列表项
- 

---

### 设计原则

1.**文本优先** — 字符串、正则、文件读写是一等公民
2.**显式前缀** — 内置函数用 #，一眼区分用户函数
3.**缩进统一** — 块用 -，每层加一个
4.**语句清晰** — 语句用 , 分隔，块用 - 结尾
5.**小而够用** — 不追求大而全，聚焦文本处理

---

### 版本

当前 0.2.0

---

### 其他

- 名称来源 
- [薇斯纳](https://baike.mihoyo.com/ys/obc/content/509803/detail?bbs_presentation_style=no_header&visit_device=pc)
- 长得像python？
- 没错，因为这就是用python写的解释器


