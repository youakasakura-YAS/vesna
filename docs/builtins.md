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
| `#rmdir(path)` | delete directory recursively (missing is fine) |

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
| `#setenv(name; value)` | set user environment variable (HKCU\Environment) |
| `#getenv(name)` | read process environment variable |
| `#regenv(name)` | read user environment variable (HKCU\Environment) |
| `#regwrite(root; path; name; value)` | write registry value (root: HKLM / HKCU) |
| `#regdelete(root; path)` | delete registry key tree (recursive) |
| `#shell(cmd)` | run shell command |
| `#path_clean(p)` | normalize path |
| `#cwd()` | current working directory |
| `#cpdir(src; dst)` | recursively copy a directory |

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

Added in 1.0.0. `#rand`, `#randint`, `#choice`, `#shuffle`, `#now`, `#date`, `#sleep`, `#ticks`, `#platform`, `#temp_dir` are non-deterministic or environment-dependent; the rest are deterministic.

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
| `#date_format(ts; fmt)` | format a timestamp (default `%Y-%m-%d %H:%M:%S`) |
| `#parse_time(s; fmt)` | parse a formatted string to timestamp (`%Y %m %d %H %M %S` supported) |
| `#sleep(ms)` | sleep milliseconds |
| `#ticks()` | monotonic milliseconds since start |
| `#platform()` | `"windows"` / `"linux"` / `"mac"` |
| `#arch()` | `"x64"` / `"arm64"` / `"x86"` / `"unknown"` |
| `#temp_dir()` | system temp directory |
| `#uuid()` | random UUID v4 string (36 chars) |

### Files

| Function | Description |
|---|---|
| `#fremove(path)` | delete file (missing is fine) |
| `#fmove(src; dst)` | move file |
| `#fsize(path)` | file size in bytes |
| `#file_time(path)` | file last-modified time (unix seconds) |
| `#truncate(path; size)` | truncate / extend a file to `size` bytes |
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
## 1.1 Concurrency / Networking / Binary

### Concurrency

| Function | Description |
|---|---|
| `#thread("fn"; arg...)` | run `fn` in a new thread with copies of global variables; returns thread id (int) |
| `#thread_join(id)` | wait for the thread and return its `back` value; rethrows the thread's error if any |
| `#thread_count()` | number of currently running (not yet joined) threads |
| `#lock("name")` | acquire the named mutex (blocking); created on first use |
| `#unlock("name")` | release the named mutex; error if never locked |

Notes: each thread runs on an independent copy of global state (functions are shared and read-only). Use `#lock`/`#unlock` around shared-file or shared-resource writes; `print` output is serialized. Joined threads are removed from the table; unjoined threads are joined at interpreter exit.

### Networking (requires `curl` in PATH)

| Function | Description |
|---|---|
| `#http_get(url)` | GET the URL, return the response body as string |
| `#http_post(url; body)` | POST form body, return the response body as string |
| `#tcp_ping(host; port)` | `0` if a TCP connection succeeds, `1` if refused/timeout, `-1` on error |
| `#http_server(port; "handler")` | blocking HTTP server; per request calls `handler(req)` where `req` is `{method; path; headers; body}`; handler returns a string (200) or `{code; body; type}` |

### Binary

| Function | Description |
|---|---|
| `#bin_read(path)` | read a file as a list of bytes (0-255 ints) |
| `#bin_write(path; bytes)` | write a list of bytes (each 0-255) to a file |
| `#bin_hex(bytes)` | bytes -> lowercase hex string |
| `#bin_unhex(s)` | hex string -> bytes list (even length required) |
| `#bin_base64_encode(bytes)` | bytes -> base64 string |
| `#bin_base64_decode(s)` | base64 string -> bytes list |

Bytes are plain integers, so existing list ops (`#len`, `#slice`, `#append`, ...) work on binary data directly.
## 1.2 Data / Crypto / Process / FFI

### Data

| Function | Description |
|---|---|
| `#json_encode(v)` | serialize any value to a JSON string (dicts keep insertion order) |
| `#json_decode(s)` | parse JSON to dict / list / int / float / string / bool / none; raises on invalid input |
| `#re_groups(s; pattern)` | first regex match as a group list: group 0 = whole match, unmatched groups are `none`; empty list if no match |

### Crypto

| Function | Description |
|---|---|
| `#sha256(s)` | SHA-256 hex digest (64 chars) — verified against FIPS 180-4 vectors |
| `#aes_encrypt(data; key)` | AES-256-CBC + PKCS7, key derived via SHA-256, returns base64 (verified against FIPS-197) |
| `#aes_decrypt(b64; key)` | decrypt base64 ciphertext produced by `#aes_encrypt` with the same key |

Note: the AES mode uses a fixed zero IV and a SHA-256-derived key — suitable for local tooling / storage at rest, not a substitute for authenticated encryption in high-security channels.

### Process

| Function | Description |
|---|---|
| `#proc_run(cmd)` | run a command, return dict `{"exit": code; "output": captured stdout}` |

### FFI (Windows x64; Linux/macOS via dlopen)

| Function | Description |
|---|---|
| `#ffi_call("dll"; "func"; arg...)` | load a shared library and call a C function; args support int / string (string passed as `char*`), max 6 args; returns the 64-bit integer result |

Examples: `#ffi_call("kernel32.dll"; "GetTickCount")`, `#ffi_call("kernel32.dll"; "GetModuleHandleA"; "kernel32.dll")`.

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
