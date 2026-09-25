# Vesna 语法

[English](syntax.md) | [中文](syntax.zh-CN.md)

## 目录

1. [字面量](#字面量)
2. [变量与赋值](#变量与赋值)
3. [运算符](#运算符)
4. [控制流](#控制流)
5. [函数](#函数)
6. [异常](#异常)
7. [模块](#模块)
8. [注释](#注释)
9. [语句与块](#语句与块)

---

## 字面量

### 字符串

用双引号：

```text
s = "hello",
```

转义：

```text
print("换行:\n制表:\t结束"),
print("引号: \" 反斜杠: \\"),
```

### 数字

用单引号包裹，支持整数、浮点、负数：

```text
a = '42',
b = '3.14',
c = '-5',
d = '-3.14',
```

**注意**：数字必须带单引号，`42` 会报「未知字符」。

### 布尔

```text
t = true,
f = false,
```

### 空值

```text
n = none,
```

### 列表

用 `;` 分隔元素，索引从 1 开始：

```text
a = ['1'; '2'; '3'],
a['1']         /* '1' */
a[-1]          /* '3'，负索引从末尾 */
```

### 组

类似列表，但**不可变**：

```text
p = ('1'; '2'),
```

### 字典

键值用 `:` 分隔，键值对用 `;` 分隔：

```text
d = {"name": "Tom"; "age": '18'},
d["name"]      /* "Tom" */
d["age"] = '19',
```

---

## 变量与赋值

```text
a = "abc",
b = '123',
c = a,
```

复合赋值：

```text
x = '10',
x += '5',      /* 15 */
x -= '3',      /* 12 */
x *= '2',      /* 24 */
x /= '4',      /* 6.0 */
```

下标赋值：

```text
a = ['1'; '2'; '3'],
a['1'] = '9',

d = {"k": '1'},
d["k"] = '9',
```

---

## 运算符

### 算术

| 运算符 | 含义 |
|---|---|
| `+` | 加（字符串用 `+` 拼接） |
| `-` | 减 |
| `*` | 乘 |
| `/` | 除 |
| `./` | 取整数部分 |
| `/.` | 取小数部分 |
| `/-` | 取余 |

```text
'10' + '3'       /* 13 */
"a" + "b"        /* "ab" */
'10' ./ '3'      /* 3 */
'10' /. '3'      /* 0.333... */
'10' /- '3'      /* 1 */
```

### 比较

| 运算符 | 含义 |
|---|---|
| `==` | 等于 |
| `!=` | 不等于 |
| `<` `>` | 小于 / 大于 |
| `<=` `>=` | 小于等于 / 大于等于 |

返回 `true` / `false`。

### 逻辑

| 运算符 | 含义 |
|---|---|
| `and` | 与 |
| `or` | 或 |
| `not` | 非 |

```text
if a > '0' and not b-
-print("ok"),
```

---

## 控制流

### if / elif / else

`elif` 和 `else` 与 `if` **同层**：

```text
if a == '1'-
-print("one"),
-elif a == '2'-
-print("two"),
-else-
-print("other"),
```

### while

```text
i = '0',
while i < '5'-
-print(i),
-i += '1',
```

### for

```text
for i in ['1'; '2'; '3']-
-print(i),
```

### break / continue

```text
for i in ['1'; '2'; '3']-
-if i == '2'-
--continue
-print(i),
```

### try / catch

```text
try-
-n = #int("abc"),
-catch e
-print("失败: " + e),
```

---

## 函数

```text
def add(a; b)-
-back(a + b)

print(add('1'; '2')),
```

带默认值：

```text
def greet(name; prefix = "Hello")-
-back(prefix + ", " + name)

print(greet("Vesna")),
print(greet("Vesna"; "Hi")),
```

**参数之间用 `;` 分隔。**

---

## 模块

```text
import csv,
import json,

rows = csv_parse(#fread("data.csv")),
print(json_write(rows)),
```

`import xxx` 会找：

```text
<脚本目录>\xxx.ves
<脚本目录>\lib\xxx.ves
<VESNA_HOME>\lib\xxx.ves
```

导入后，模块里的**变量和函数**都进入当前作用域。

---

## 注释

块注释：

```text
/* 这是注释 */

/* 多行
   注释 */
```

注释不能被嵌套。

---

## 语句与块

### 语句分隔

**每条语句末尾用 `,`**：

```text
a = '1',
b = '2',
print(a + b),
```

一行可以写多条：

```text
a = '1', b = '2', print(a + b),
```

### 块

用 `-` 表示缩进。每嵌套一层，多一个 `-`：

```text
if a > '0'-
-print("positive"),
-if a > '10'-
--print("big"),
```

`if` / `elif` / `else` / `while` / `for` / `def` / `try` 的行末要加 `-`。

### 内置函数

内置函数用 `#` 前缀：

```text
#up("hello"),        /* "HELLO" */
#split("a,b,c"; ",")  /* ["a"; "b"; "c"] */
```

参数之间用 `;`。

### 起止符

`.ves` 文件里不需要。嵌入其它文件时：

```text
<vesna>
...
</vesna>
```

---

## 完整示例

```text
import csv,
import json,

def is_error(line)-
-back(#match(line; "ERROR"))

text = #fread("app.log"),
lines = #lines(text),
errors = #filter(lines; "is_error"),

print("错误行数: " + #str(#len(errors))),

for e in errors-
-print(#trim(e)),
```
