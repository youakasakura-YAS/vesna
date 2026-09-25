# Vesna

<img src="icon.png" width="128" alt="Vesna">

> *Scripts of spring*

[English](README.md) | [中文](README.zh-CN.md)

![version](https://img.shields.io/badge/version-1.3.0-blue)
![license](https://img.shields.io/badge/license-MIT-green)
![platform](https://img.shields.io/badge/platform-Windows-x64-lightgrey)

**This repository hosts the C++ implementation of Vesna** — a lightweight text-processing scripting language (file extension `.ves`).

- **Positioning**: text processing / data transformation scripts
- **Alternatives**: `awk`, `sed`, `jq`, `perl`
- **Language spec**: authoritative semantics live in the [Python reference implementation](https://github.com/youakasakura-YAS/vesna-py)
- **VSCode extension**: [vesna-vscode-extension](https://github.com/youakasakura-YAS/vesna-vscode-extension) (syntax highlighting + LSP)
- **Performance**: processes 100k log lines in ~240ms
- **Syntax**: indentation uses `-`, builtins use `#` prefix, statements end with `,`

---

## Quick start

### 1. Download

Grab `vesna-1.3.0-windows-x64.zip` from [Releases](../../releases) and extract it anywhere.

| File | Description |
|---|---|
| `bin\vesna.exe` | C++ interpreter (statically linked, no DLL dependencies) |
| `lib\` | Standard library (csv / json / text / stat) |
| `examples\` | Example scripts |
| `docs\` | Documentation |
| `README.md` / `CHANGELOG.md` / `LICENSE` | Release notes |

### 2. Install (optional)

Installs to `C:\Vesna` by default:

```bat
vesna.exe --install
```

Custom path:

```bat
vesna.exe --install D:\MyVesna
```

After install, the `vesna` command works from any directory, and `.ves` file association is registered.

### 3. Run scripts

```bat
vesna hello.ves
vesna wc.ves sample.txt
vesna grep.ves "ERROR" log.txt
```

### 4. Debug scripts

```bat
vesna --debug hello.ves
```

Interactive commands: `c`/`continue` continue, `n`/`next` next line, `s`/`step` step into, `q`/`quit` quit, `b <line>` set breakpoint, `del <line>` delete breakpoint, `p <expr>` evaluate expression, `vars` list variables, `bt` backtrace, `list` show source, `help` help.

### 5. Package manager (vpm)

```bat
vesna --pkg init                 # scaffold vesna-pkg.json
vesna --pkg registry             # cache index (default: Vesna Package Garden)
vesna --pkg install <dir|zip|owner:repo|name>
vesna --pkg remove <name>
vesna --pkg list
vesna --pkg search <keyword>
```

`install <name>` looks the name up in the cached registry index and downloads the package zip. The official registry — [Vesna Package Garden](https://youakasakura-YAS.github.io/vesna-pkg/) — is used by default. Installed packages are imported with `import <name>` from `<VESNA_HOME>\packages\<name>\<name>.ves`.
Validation (since 1.3): package metadata is checked (`name` must be `^[a-z][a-z0-9_-]+$`, `version` must be `x.y.z`, `entry` must exist), registry entries may carry a `sha256` hash that is verified before install, dependencies are installed automatically (with version checks and loop protection), and `remove` refuses to uninstall a package other installed packages still depend on (`--force` overrides).

---

## Hello, World

```vesna
print("Hello, World"),
```

### A complete example

Read a CSV file, output JSON:

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

*Run*:

```
vesna csv2json.ves data.csv
```

---

## Bundled tools

The `examples\` directory ships a set of command-line tools:

| Script | Purpose |
|---|---|
| `wc.ves` | line/word/char counts |
| `grep.ves` | regex filter |
| `head.ves` / `tail.ves` | first/last N lines |
| `sort.ves` / `uniq.ves` | sort / dedupe |
| `cut.ves` | extract columns by separator |
| `sed.ves` / `replace.ves` | regex/literal replace |
| `stat.ves` | word frequency |
| `logstat.ves` | log level stats |
| `csv2json.ves` | CSV → JSON |
| `extract.ves` | extract emails/URLs/IPs |

---

---

## Concurrency / Networking / Binary (1.1)

Second/third-tier builtins, no syntax changes, snake_case naming, see [docs/builtins](docs/builtins.md):

- **Concurrency**: `#thread("fn"; arg...)` (isolated global copies per thread), `#thread_join(id)`, `#thread_count()`, `#lock("name")` / `#unlock("name")` (named mutexes)
- **Networking** (requires `curl` in PATH): `#http_get(url)`, `#http_post(url; body)`, `#tcp_ping(host; port)` (native TCP probe)
- **Binary**: `#bin_read(path)` / `#bin_write(path; bytes)`, `#bin_hex` / `#bin_unhex`, `#bin_base64_encode` / `#bin_base64_decode`
Tier 3 (1.2) — **Data / Crypto / Process / FFI**:
- **Data**: `#json_encode` / `#json_decode`, `#re_groups` (regex capture groups)
- **Crypto**: `#sha256`, `#aes_encrypt` / `#aes_decrypt` (AES-256-CBC + PKCS7)
- **Process**: `#proc_run(cmd)` (returns `{exit; output}`)
- **FFI**: `#ffi_call("dll"; "func"; arg...)` — call system library functions directly
## Standard library

Modules live in `C:\Vesna\lib\` (or ship with this repo), used via `import xxx`:

| Module | Contents |
|---|---|
| `csv` | `csv_parse`, `csv_write` |
| `json` | `json_parse`, `json_write` |
| `text` | `text_word_count`, `text_extract_emails`, `text_extract_urls`, `text_extract_ips` |
| `stat` | `stat_count_by`, `stat_unique` |

---

## Performance

`vesna.exe` vs the Python reference implementation ([vesna-py](https://github.com/youakasakura-YAS/vesna-py)):

| Benchmark | Python | C++ |
|---|---|---|
| grep 100k log lines | ~2.8s | ~0.24s (≈12×) |
| loop 10k sum | ~229ms | ~28ms (≈8×) |

C++ optimizations (details in [CHANGELOG.md](CHANGELOG.md)): regex cache, literal fast paths, `Value` variant, identifier interning, builtin hash dispatch, `-O3 -flto`, disabled iostream sync.

---

## Building from source

### Windows (MinGW-w64, g++ 11+, Windows 10+)

```bat
cd src\cpp
g++ -std=c++17 -O3 -flto -static -Wall -Wextra vesna.cpp main.cpp -o vesna.exe -ladvapi32
```

### Cross-platform (CMake, Windows / Linux / macOS)

```bat
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The source tree is portable: `src/cpp/platform.h` abstracts UTF-8/UTF-16 conversion, shell, cwd, chdir, environment and the temp dir. Registry builtins (`-regwrite` / `-regdelete` / `-regenv`) report "not supported on this platform" on non-Windows.

### Regression tests

Outputs must match the golden baselines in `src/cpp/tests/golden/` (see its README for regeneration commands):

```bat
vesna.exe tests\regression.ves
vesna.exe tests\fs_test.ves
vesna.exe tests\smoke04.ves
```

---

## Syntax tour

```vesna
/* comments */

/* variables */
name = "Vesna",
age = '1',

/* list, group, dict */
items = ['1'; '2'; '3'],
point = ('1'; '2'),
config = {"host": "localhost"; "port": '8080'},

/* condition */
if age > '18'-
-print("adult"),
-elif age > '12'-
-print("teen"),
-else-
-print("child"),

/* loops */
for i in items-
-print(i),

i = '0',
while i < '3'-
-print(i),
-i += '1',

/* functions */
def greet(who)-
-back("Hello, " + who)
print(greet("Vesna")),

/* modules */
import json,
print(json_write(items)),
```

---

## Repository layout

```
src\
  cpp\              C++ implementation (vesna.hpp / vesna.cpp / main.cpp)
  cpp\tests\        regression assets
lib\                standard library (.ves)
examples\           example scripts
docs\               documentation
bench\              benchmarks
```

The Python reference implementation lives in its own repository: [youakasakura-YAS/vesna-py](https://github.com/youakasakura-YAS/vesna-py).

---

## Documentation

- [docs/syntax.md](docs/syntax.md) — language syntax
- [docs/builtins.md](docs/builtins.md) — builtin functions
- [CHANGELOG.md](CHANGELOG.md) — version history

---

## Design principles

1. **Text first** — strings, regex, and file I/O are first-class citizens
2. **Explicit prefixes** — builtins use `#`, easy to tell from user functions
3. **Uniform indentation** — blocks use `-`, one per nesting level
4. **Clear statements** — statements end with `,`, blocks end with `-`
5. **Small and sufficient** — focused on text processing, not bloat

---

## License

[MIT](LICENSE)

Name origin: [薇斯纳](https://baike.mihoyo.com/ys/obc/content/509803/detail?bbs_presentation_style=no_header&visit_device=pc)
