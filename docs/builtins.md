# Vesna Builtin Functions

[English](builtins.md) | [中文](builtins.zh-CN.md)

All builtins use the `#` prefix; arguments are separated by `;`.

```text
#up("hello"),           /* "HELLO" */
#split("a,b,c"; ","),   /* ["a"; "b"; "c"] */
```

---

## Strings

| Function | Description |
|---|---|
| `#len(s)` | length |
| `#up(s)` | uppercase |
| `#down(s)` | lowercase |
| `#title(s)` | capitalize each word |
| `#capitalize(s)` | first letter upper, rest lower |
| `#trim(s)` | strip both sides |
| `#lstrip(s)` | strip left |
| `#rstrip(s)` | strip right |
| `#sub(s; start; end)` | substring, 1-based index |
| `#split(s; sep)` | split by separator |
| `#join(list; sep)` | join list |
| `#find(s; sub)` | find position, 0 if absent |
| `#rfind(s; sub)` | find from the right |
| `#replace(s; old; new)` | literal replace |
| `#count(s; sub)` | count occurrences |
| `#startswith(s; p)` | starts with p? |
| `#endswith(s; p)` | ends with p? |
| `#repeat(s; n)` | repeat n times |
| `#lines(s)` | split by lines |
| `#char_at(s; i)` | i-th char, 1-based |
| `#ord(c)` | char → code |
| `#chr(n)` | code → char |

```text
#sub("Hello"; '1'; '3'),        /* "He" */
#find("Hello"; "l"),            /* 3 */
#replace("abc"; "b"; "X"),      /* "aXc" */
#char_at("Hello"; '1'),         /* "H" */
#ord("A"),                      /* 65 */
#chr('66'),                     /* "B" */
```

---

## Type conversion

| Function | Description |
|---|---|
| `#str(x)` | to string |
| `#int(x)` | to int |
| `#float(x)` | to float |
| `#bool(x)` | to bool |
| `#into(type; x)` | generic conversion |

The type argument of `#into` is a **keyword**, not a string:

```text
#into(int; "42"),
#into(str; '5'),
#into(float; "3.14"),
#into(list; "abc"),
```

---

## Type checks

| Function | Description |
|---|---|
| `#type(x)` | type name |
| `#is_digit(c)` | is a digit char |
| `#is_alpha(c)` | is a letter |
| `#is_alnum(c)` | letter or digit |
| `#is_space(c)` | whitespace |

```text
#type('5'),        /* "int" */
#type("a"),        /* "str" */
#type([1; 2]),     /* "list" */
```

---

## Lists

| Function | Description |
|---|---|
| `#len(a)` | length |
| `#append(a; x)` | append element |
| `#pop(a)` | pop from the end |
| `#sort(a)` | sort (returns a new list) |
| `#reverse(a)` | reverse |
| `#slice(a; start; end)` | slice |
| `#contains(a; x)` | contains? |

```text
a = ['3'; '1'; '2'],
#sort(a),              /* ['1'; '2'; '3'] */
#contains(a; '2'),     /* true */
```

---

## Dicts

| Function | Description |
|---|---|
| `#len(d)` | number of keys |
| `#keys(d)` | key list |
| `#values(d)` | value list |
| `#has_key(d; k)` | has key? |

```text
d = {"a": '1'; "b": '2'},
#keys(d),              /* ["a"; "b"] */
#has_key(d; "a"),      /* true */
```

---

## Math

| Function | Description |
|---|---|
| `#min(a)` | minimum |
| `#max(a)` | maximum |
| `#sum(a)` | sum |
| `#abs(x)` | absolute value |
| `#round(x; n)` | round to n digits |
| `#pow(a; b)` | power |

```text
#min([3; 1; 4]),       /* 1 */
#max([3; 1; 4]),       /* 4 */
#sum([3; 1; 4]),       /* 8 */
#abs('-5'),            /* 5 */
#round('3.14159'; '2'),/* 3.14 */
#pow('2'; '10'),       /* 1024 */
```

---

## Regex

| Function | Description |
|---|---|
| `#match(s; pat)` | matches? |
| `#findall(s; pat)` | extract all matches |
| `#gsub(s; pat; repl)` | replace |
| `#search(s; pat)` | search, returns groups |

```text
#match("2026-09-12"; "\\d{4}"),       /* true */
#findall("a1 b2 c3"; "[a-z]"),        /* ["a"; "b"; "c"] */
#gsub("a1b2"; "[0-9]"; ""),           /* "ab" */
```

---

## Files & directories

| Function | Description |
|---|---|
| `#fread(path)` | read file |
| `#fwrite(path; text)` | write file |
| `#fappend(path; text)` | append |
| `#fexists(path)` | file exists? |
| `#ls(dir)` | list directory |
| `#glob(pat)` | glob match |

```text
#fread("data.txt"),
#fwrite("out.txt"; "hello\n"),
#fappend("log.txt"; "new line\n"),
#fexists("data.txt"),       /* true */
#ls("."),                   /* file list */
#glob("*.ves"),             /* all .ves files */
```

---

## System

| Function | Description |
|---|---|
| `#args()` | command-line args |
| `#stdin()` | read standard input |
| `#exit(code)` | exit program |

```text
args = #args(),
path = args['1'],
```

---

## Functional

| Function | Description |
|---|---|
| `#map(list; "fn")` | call fn on each element |
| `#filter(list; "fn")` | keep elements where fn is truthy |
| `#reduce(list; "fn"; init)` | fold |

**Function names are strings**:

```text
def is_error(line)-
-back(#match(line; "ERROR"))

errors = #filter(lines; "is_error"),
```

`#map` and `#filter` call `fn` with one argument. `#reduce` calls `fn` with two: accumulator and current element.

```text
def add(a; b)-
-back(a + b)

total = #reduce(nums; "add"; '0'),
```

---

## Interpolated strings

`#f"..."` replaces `(var)` in the string with the variable's value:

```text
name = "Vesna",
age = '1',
print(#f"name=(name), age=(age)"),
/* prints: name=Vesna, age=1 */
```

`(name)` can only hold a variable name, not an expression.

---


---

## 0.4 General-purpose expansion

Added in 0.4.0. `#rand`, `#randint`, `#choice`, `#shuffle`, `#now`, `#date`, `#sleep`, `#ticks`, `#platform`, `#temp_dir` are non-deterministic or environment-dependent; the rest are deterministic.

### Advanced math

| Function | Description |
|---|---|
| `#sqrt(x)` | square root |
| `#floor(x)` / `#ceil(x)` | floor / ceiling |
| `#exp(x)` | e^x |
| `#log(x)` / `#log10(x)` | natural / base-10 log (x > 0) |
| `#sin(x)` / `#cos(x)` / `#tan(x)` | trigonometry (radians) |
| `#sign(x)` | -1 / 0 / 1 |
| `#clamp(x; lo; hi)` | clamp into [lo; hi] |
| `#rand()` | random float in [0; 1) |
| `#randint(a; b)` | random int in [a; b] |
| `#choice(a)` | random element of a list |
| `#shuffle(a)` | shuffled copy of a list |

### Number bases

| Function | Description |
|---|---|
| `#hex(n)` | to hexadecimal (no prefix, lowercase) |
| `#bin(n)` | to binary |
| `#oct(n)` | to octal |

### Strings

| Function | Description |
|---|---|
| `#pad(s; w; c)` | center-pad s to width w with c |
| `#lpad(s; w; c)` | right-align (pad left) |
| `#rpad(s; w; c)` | left-align (pad right) |
| `#format(fmt; ...)` | C-style formatting: `%s` `%d` `%f` `%.2f` `%%` |
| `#hash(s)` | deterministic FNV-1a 64-bit hash (decimal string) |

```text
#format("%s-%d-%.2f"; "v"; '42'; '3.14159'),  /* "v-42-3.14" */
#hash("hello"),                               /* deterministic 64-bit number */
#lpad("ab"; '5'; "0"),                        /* "000ab" */
```

### Lists

| Function | Description |
|---|---|
| `#range(start; end; step)` | integer sequence `[start; end)` |
| `#first(a)` / `#last(a)` | first / last element, none if empty |
| `#take(a; n)` / `#drop(a; n)` | keep / drop first n elements |
| `#set(a)` | unique elements, order kept |
| `#flatten(a)` | flatten one level |
| `#zip(a; b)` | pair elements into groups |
| `#insert(a; i; x)` | new list with x inserted at 1-based i |
| `#remove(a; i)` | new list without the 1-based i-th element |
| `#index_of(a; x)` | 1-based position, 0 if absent |
| `#enumerate(a)` | `[(i; v); ...]` with 1-based index |
| `#concat(a; b)` | concatenate two lists |

```text
#range('1'; '5'),          /* ['1';'2';'3';'4'] */
#zip(['1';'2']; ["a";"b"]),/* [('1';a);('2';b)] */
#set(['1';'1';'2']),       /* ['1';'2'] */
```

### Dicts

| Function | Description |
|---|---|
| `#get(d; k; default)` | value for key, or default (none) |
| `#items(d)` | `[(k; v); ...]` |
| `#pop_key(d; k)` | remove key, return old value (or none) |

### Type checks

`#is_str` `#is_int` `#is_float` `#is_bool` `#is_list` `#is_dict` `#is_none` `#is_group`

### Time & system

| Function | Description |
|---|---|
| `#now()` | unix timestamp (int) |
| `#date(fmt)` | formatted local time (default `%Y-%m-%d %H:%M:%S`) |
| `#sleep(ms)` | sleep milliseconds |
| `#ticks()` | monotonic milliseconds since start |
| `#platform()` | `"windows"` |
| `#temp_dir()` | system temp directory |

### Files

| Function | Description |
|---|---|
| `#fremove(path)` | delete file (missing is fine) |
| `#fmove(src; dst)` | move file |
| `#fsize(path)` | file size in bytes |
| `#is_dir(path)` / `#is_file(path)` | path type checks |
| `#mkdirs(path)` | create directories recursively |

### Encoding

| Function | Description |
|---|---|
| `#base64_encode(s)` / `#base64_decode(s)` | base64 encode / decode |
| `#url_encode(s)` / `#url_decode(s)` | URL percent-encoding (space → `+`) |

### Functional

| Function | Description |
|---|---|
| `#each(list; "fn")` | call fn for side effects, returns the list |
| `#all(list; "fn")` | every element truthy? |
| `#any(list; "fn")` | any element truthy? |
| `#find_first(list; "fn")` | first truthy element, none if absent |
| `#sort_by(list; "fn")` | stable sort by fn key |

### Exceptions

| Function | Description |
|---|---|
| `#throw(msg)` | raise an error |
| `#assert(cond; msg)` | raise if cond is falsy |

## Full example

```text
import json,

args = #args(),
path = args['1'],

if not #fexists(path)-
-print("file not found: " + path),
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
