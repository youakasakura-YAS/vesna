# Vesna 内置函数

[English](builtins.md) | [中文](builtins.zh-CN.md)

所有内置函数用 `#` 前缀，参数用 `;` 分隔。

```text
#up("hello"),           /* "HELLO" */
#split("a,b,c"; ","),   /* ["a"; "b"; "c"] */
```

---

## 字符串

| 函数 | 说明 |
|---|---|
| `#len(s)` | 长度 |
| `#up(s)` | 转大写 |
| `#down(s)` | 转小写 |
| `#title(s)` | 每个单词首字母大写 |
| `#capitalize(s)` | 首字母大写，其余小写 |
| `#trim(s)` | 去两边空白 |
| `#lstrip(s)` | 去左空白 |
| `#rstrip(s)` | 去右空白 |
| `#sub(s; start; end)` | 子串，索引从 1 开始 |
| `#split(s; sep)` | 按分隔符切分 |
| `#join(list; sep)` | 拼接列表 |
| `#find(s; sub)` | 查找位置，找不到返回 0 |
| `#rfind(s; sub)` | 从右查找 |
| `#replace(s; old; new)` | 字面替换 |
| `#count(s; sub)` | 子串出现次数 |
| `#startswith(s; p)` | 是否以 p 开头 |
| `#endswith(s; p)` | 是否以 p 结尾 |
| `#repeat(s; n)` | 重复 n 次 |
| `#lines(s)` | 按行切分 |
| `#char_at(s; i)` | 第 i 个字符，从 1 开始 |
| `#ord(c)` | 字符 → 码 |
| `#chr(n)` | 码 → 字符 |

```text
#sub("Hello"; '1'; '3'),        /* "He" */
#find("Hello"; "l"),            /* 3 */
#replace("abc"; "b"; "X"),      /* "aXc" */
#char_at("Hello"; '1'),         /* "H" */
#ord("A"),                      /* 65 */
#chr('66'),                     /* "B" */
```

---

## 类型转换

| 函数 | 说明 |
|---|---|
| `#str(x)` | 转字符串 |
| `#int(x)` | 转整数 |
| `#float(x)` | 转浮点 |
| `#bool(x)` | 转布尔 |
| `#into(type; x)` | 通用转换 |

`#into` 的类型参数是**关键字**，不是字符串：

```text
#into(int; "42"),
#into(str; '5'),
#into(float; "3.14"),
#into(list; "abc"),
```

---

## 类型判断

| 函数 | 说明 |
|---|---|
| `#type(x)` | 返回类型名 |
| `#is_digit(c)` | 是否数字字符 |
| `#is_alpha(c)` | 是否字母 |
| `#is_alnum(c)` | 是否字母或数字 |
| `#is_space(c)` | 是否空白 |

```text
#type('5'),        /* "int" */
#type("a"),        /* "str" */
#type([1; 2]),     /* "list" */
```

---

## 列表

| 函数 | 说明 |
|---|---|
| `#len(a)` | 长度 |
| `#append(a; x)` | 追加元素 |
| `#pop(a)` | 弹出末尾 |
| `#sort(a)` | 排序（返回新列表） |
| `#reverse(a)` | 反转 |
| `#slice(a; start; end)` | 切片 |
| `#contains(a; x)` | 是否包含 |

```text
a = ['3'; '1'; '2'],
#sort(a),              /* ['1'; '2'; '3'] */
#contains(a; '2'),     /* true */
```

---

## 字典

| 函数 | 说明 |
|---|---|
| `#len(d)` | 键的数量 |
| `#keys(d)` | 键列表 |
| `#values(d)` | 值列表 |
| `#has_key(d; k)` | 是否包含键 |

```text
d = {"a": '1'; "b": '2'},
#keys(d),              /* ["a"; "b"] */
#has_key(d; "a"),      /* true */
```

---

## 数学

| 函数 | 说明 |
|---|---|
| `#min(a)` | 最小 |
| `#max(a)` | 最大 |
| `#sum(a)` | 求和 |
| `#abs(x)` | 绝对值 |
| `#round(x; n)` | 四舍五入，保留 n 位 |
| `#pow(a; b)` | 幂 |

```text
#min([3; 1; 4]),       /* 1 */
#max([3; 1; 4]),       /* 4 */
#sum([3; 1; 4]),       /* 8 */
#abs('-5'),            /* 5 */
#round('3.14159'; '2'),/* 3.14 */
#pow('2'; '10'),       /* 1024 */
```

---

## 正则

| 函数 | 说明 |
|---|---|
| `#match(s; pat)` | 是否匹配 |
| `#findall(s; pat)` | 提取所有匹配 |
| `#gsub(s; pat; repl)` | 替换 |
| `#search(s; pat)` | 搜索，返回分组 |

```text
#match("2026-09-12"; "\\d{4}"),       /* true */
#findall("a1 b2 c3"; "[a-z]"),        /* ["a"; "b"; "c"] */
#gsub("a1b2"; "[0-9]"; ""),           /* "ab" */
```

---

## 文件与目录

| 函数 | 说明 |
|---|---|
| `#fread(path)` | 读文件 |
| `#fwrite(path; text)` | 写文件 |
| `#fappend(path; text)` | 追加 |
| `#fexists(path)` | 文件是否存在 |
| `#ls(dir)` | 列目录 |
| `#glob(pat)` | 通配匹配 |

```text
#fread("data.txt"),
#fwrite("out.txt"; "hello\n"),
#fappend("log.txt"; "new line\n"),
#fexists("data.txt"),       /* true */
#ls("."),                   /* 文件列表 */
#glob("*.ves"),             /* 所有 .ves 文件 */
```

---

## 系统

| 函数 | 说明 |
|---|---|
| `#args()` | 命令行参数列表 |
| `#stdin()` | 读标准输入 |
| `#exit(code)` | 退出程序 |

```text
args = #args(),
path = args['1'],
```

---

## 函数式

| 函数 | 说明 |
|---|---|
| `#map(list; "fn")` | 对每个元素调用 fn |
| `#filter(list; "fn")` | 保留 fn 返回真的元素 |
| `#reduce(list; "fn"; init)` | 归约 |

**函数名用字符串**：

```text
def is_error(line)-
-back(#match(line; "ERROR"))

errors = #filter(lines; "is_error"),
```

`#map` 和 `#filter` 的 `fn` 接受一个参数。`#reduce` 接受两个参数：累加器和当前元素。

```text
def add(a; b)-
-back(a + b)

total = #reduce(nums; "add"; '0'),
```

---

## 插值字符串

`#f"..."` 把字符串里的 `(var)` 替换为变量值：

```text
name = "Vesna",
age = '1',
print(#f"name=(name), age=(age)"),
/* 输出：name=Vesna, age=1 */
```

`(name)` 里只能是变量名，不能是表达式。

---

## 完整例子

```text
import json,

args = #args(),
path = args['1'],

if not #fexists(path)-
-print("找不到文件: " + path),
-#exit('1'),

content = #fread(path),
lines = #lines(content),

def is_error(line)-
-back(#match(line; "ERROR"))

errors = #filter(lines; "is_error"),

result = {
    "file": path;
    "total": #len(lines);
    "errors": #len(errors)
},

print(json_write(result)),
```
