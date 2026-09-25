# Vesna Syntax

[English](syntax.md) | [中文](syntax.zh-CN.md)

## Contents

1. [Literals](#literals)
2. [Variables & assignment](#variables--assignment)
3. [Operators](#operators)
4. [Control flow](#control-flow)
5. [Functions](#functions)
6. [Exceptions](#exceptions)
7. [Modules](#modules)
8. [Comments](#comments)
9. [Statements & blocks](#statements--blocks)

---

## Literals

### Strings

Double quotes:

```text
s = "hello",
```

Escapes:

```text
print("newline:\ntab:\tend"),
print("quote: \" backslash: \\"),
```

### Numbers

Single quotes, integers / floats / negatives:

```text
a = '42',
b = '3.14',
c = '-5',
d = '-3.14',
```

**Note**: numbers must be single-quoted; bare `42` raises "unknown character".

### Booleans

```text
t = true,
f = false,
```

### None

```text
n = none,
```

### Lists

`;`-separated, 1-based indexing:

```text
a = ['1'; '2'; '3'],
a['1']         /* '1' */
a[-1]          /* '3', negative index from the end */
```

### Groups

Like lists but **immutable**:

```text
p = ('1'; '2'),
```

### Dicts

`:` separates key/value, `;` separates pairs:

```text
d = {"name": "Tom"; "age": '18'},
d["name"]      /* "Tom" */
d["age"] = '19',
```

---

## Variables & assignment

```text
a = "abc",
b = '123',
c = a,
```

Compound assignment:

```text
x = '10',
x += '5',      /* 15 */
x -= '3',      /* 12 */
x *= '2',      /* 24 */
x /= '4',      /* 6.0 */
```

Indexed assignment:

```text
a = ['1'; '2'; '3'],
a['1'] = '9',

d = {"k": '1'},
d["k"] = '9',
```

---

## Operators

### Arithmetic

| Operator | Meaning |
|---|---|
| `+` | add (also string concat) |
| `-` | subtract |
| `*` | multiply |
| `/` | divide |
| `./` | integer part |
| `/.` | fractional part |
| `/-` | modulo |

```text
'10' + '3'       /* 13 */
"a" + "b"        /* "ab" */
'10' ./ '3'      /* 3 */
'10' /. '3'      /* 0.333... */
'10' /- '3'      /* 1 */
```

### Comparison

| Operator | Meaning |
|---|---|
| `==` | equal |
| `!=` | not equal |
| `<` `>` | less / greater |
| `<=` `>=` | less-equal / greater-equal |

Returns `true` / `false`.

### Logic

| Operator | Meaning |
|---|---|
| `and` | and |
| `or` | or |
| `not` | not |

```text
if a > '0' and not b-
-print("ok"),
```

---

## Control flow

### if / elif / else

`elif` and `else` sit at the **same level** as `if`:

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
-print("failed: " + e),
```

---

## Functions

```text
def add(a; b)-
-back(a + b)

print(add('1'; '2')),
```

Default values:

```text
def greet(name; prefix = "Hello")-
-back(prefix + ", " + name)

print(greet("Vesna")),
print(greet("Vesna"; "Hi")),
```

**Parameters are separated by `;`.**

---

## Modules

```text
import csv,
import json,

rows = csv_parse(#fread("data.csv")),
print(json_write(rows)),
```

`import xxx` searches:

```text
<script dir>\xxx.ves
<script dir>\lib\xxx.ves
<VESNA_HOME>\lib\xxx.ves
```

After import, the module's **variables and functions** enter the current scope.

---

## Comments

Block comments:

```text
/* this is a comment */

/* multi-line
   comment */
```

Comments cannot be nested.

---

## Statements & blocks

### Statement separator

**Every statement ends with `,`**:

```text
a = '1',
b = '2',
print(a + b),
```

Multiple statements on one line:

```text
a = '1', b = '2', print(a + b),
```

### Blocks

`-` means indentation. Each nesting level adds one `-`:

```text
if a > '0'-
-print("positive"),
-if a > '10'-
--print("big"),
```

`if` / `elif` / `else` / `while` / `for` / `def` / `try` lines end with `-`.

### Builtins

Builtins use the `#` prefix:

```text
#up("hello"),        /* "HELLO" */
#split("a,b,c"; ",")  /* ["a"; "b"; "c"] */
```

Parameters are separated by `;`.

### Start/end tags

Not needed in `.ves` files. When embedding in other files:

```text
<vesna>
...
</vesna>
```

---

## Full example

```text
import csv,
import json,

def is_error(line)-
-back(#match(line; "ERROR"))

text = #fread("app.log"),
lines = #lines(text),
errors = #filter(lines; "is_error"),

print("error lines: " + #str(#len(errors))),

for e in errors-
-print(#trim(e)),
```
