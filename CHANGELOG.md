# Changelog

[English](CHANGELOG.md) | [中文](CHANGELOG.zh-CN.md)

## 1.7.0

### Changed
- **Single-file distribution** — the standard library (`csv.ves` / `json.ves` / `pkg.ves` / `stat.ves` / `text.ves`, ~19 KB) is now embedded into the executable. `vesna.exe` runs standalone with no `lib/` directory, no `VESNA_HOME` and no other files — copy one exe and everything works (including `--pkg`). External `lib/` files still take precedence, so users can still override modules by dropping a `.ves` next to their script or in `lib/`.
- Version bump to 1.7.0.

## 1.6.0


### Added
- **`#http_server(port; "handler")`** — a blocking HTTP server (native sockets, no dependencies). Each request builds `req = {method; path; headers; body}` and calls the handler by name; the handler returns a string (200) or `{code; body; type}` for full control
- **Time builtins**: `#date_format(ts; fmt)` formats any unix timestamp (default `%Y-%m-%d %H:%M:%S`); `#parse_time(s; fmt)` parses a formatted string back to a timestamp (`%Y %m %d %H %M %S` subset, separator-tolerant)
- **`#uuid()`** — random UUID v4 (36 chars, RFC-4122 variant/version bits)
- **File builtins**: `#file_time(path)` returns the last-modified unix time; `#truncate(path; size)` truncates or extends a file to the given byte size
- **`#arch()`** — `"x64"` / `"arm64"` / `"x86"` / `"unknown"` (runtime platform detection is `#platform()` since 0.4)
- Syntax highlighting extended to all 183 builtins

## 1.5.0


### Added
- **vpm publish**: `vesna --pkg publish` — one command to publish a package: validates `vesna-pkg.json`, packs `entry` + metadata into `<name>-<version>.zip` (PowerShell `Compress-Archive` on Windows / `zip` elsewhere), computes the zip `sha256`, writes `registry-entry.json`, and updates a local `registry.json` if present
- **`#call(fname; arg...)` builtin** (dispatch 176): dynamic function invocation by name string — enables package frameworks like the new `vesna-test` test framework
- **`vesna-test` package 1.0.0**: unit test framework (`test_case(name; "fn")`, `test_run()`, `test_eq`, `test_assert`, `test_true`, `test_count`) — published to the vesna-pkg registry
- **Import respects package `entry`**: `import` now reads `vesna-pkg.json`'s `entry` field for vpm packages (previously only `<package-name>.ves` was tried); imported package globals can also see caller script functions via the parent env chain
- **`vesna --fmt <file>`**: source formatter — normalizes indentation (`-` runs), trims trailing whitespace, collapses blank lines (syntax-preserving, line-level only)
- **Syntax highlighting 1.1-1.4**: `vesna.tmLanguage.json` builtin patterns extended to all 176 builtins (thread/lock/http/bin/json/crypto/proc/ffi/csv/ini/xml/call)
- **REPL Tab completion** (Windows): type `#js<Tab>` etc. — unique match auto-completes, multiple matches list candidates

## 1.4.0


### Added
- **Native C++ LSP**: `vesna.exe --lsp` — a built-in Language Server (stdio JSON-RPC) with diagnostics (reuses the C++ `Parser` error collection), completion (all builtins from the single name table plus document identifiers, `#` trigger), hover (builtin/keyword docs + function defs), `documentSymbol` (functions) and `foldingRange` (indent blocks)
- **VSCode extension 0.5.0**: `vesna-0.5.0.vsix` — server now spawns `vesna --lsp` (no Python dependency); the Python LSP server directory was removed
- **Tier-4 builtins** (dispatch 170-175): `#csv_parse(s)` → row/field list, `#csv_build(rows)` → CSV text (RFC-4180 quoting), `#ini_read(path)` / `#ini_write(path; data)` (section/key/value, `;` `#` comments), `#xml_parse(s)` → simple DOM `{tag; attrs; children; text}` (attributes, nesting, self-closing tags, `&amp;` `&lt;` `&gt;` `&quot;` `&apos;`), `#ffi_call_s(dll; func; arg...)` → `char*` result as string
- **CI fixes**: compile now links `-lws2_32` (1.1 networking builtins); golden regression extended with tier3 / binary / tier4; concurrency run check; vpm validation smoke; LSP smoke drives the compiled `vesna.exe --lsp`; `release.yml` aligned to 1.4.0 (`vesna-0.5.0.vsix`, `-lws2_32`, bilingual README/CHANGELOG/CONTRIBUTING + RELEASE-NOTES)
- **Single source of truth for builtins**: the 175-entry name table `g_builtinNames` now drives the lexer's builtin set, the dispatch map, and LSP completion (previously three hand-maintained tables)

## 1.3.1

### Fixed
- `--install` now works when run from the `bin\` directory of a release package (the installer auto-detects a trailing `\bin` in the current directory and walks up to the package root). Previously it errored with "找不到 bin\vesna.exe".

## 1.3.0## 1.3.0

### Added
- **vpm validation** (`lib/pkg.ves`): metadata checks (`name` `^[a-z][a-z0-9_-]+$`, `version` `x.y.z`, `entry` must exist), optional `sha256` integrity verification from registry entries, automatic dependency installation with version checks and loop protection, and `remove` dependency guard (`--force` overrides). `init [name]` now validates the package name.
- **LSP 0.4.0** (`vesna-vscode/server`): parser synced to the frozen 1.0.0 reference (full 0.4 syntax diagnostics); completion covers all 169 builtins (1.1/1.2 tiers included); hover with builtin/keyword docs; `documentSymbol` (functions); `foldingRange` (indent blocks); `#` completion trigger.
- **VSCode extension 0.4.0**: `vesna-0.4.0.vsix`, serverInfo 0.4.0, new description.

## 1.2.0## 1.2.0

### Added
- **Data builtins**: `#json_encode(v)` / `#json_decode(s)` (native JSON, dicts keep insertion order), `#re_groups(s; pattern)` (regex capture groups, unmatched groups are `none`)
- **Crypto builtins**: `#sha256(s)` (verified against FIPS 180-4 vectors), `#aes_encrypt(data; key)` / `#aes_decrypt(b64; key)` (AES-256-CBC + PKCS7, SHA-256-derived key, base64 output, verified against FIPS-197); new pure-C++ `src/cpp/crypto.h` with `crypto_selftest.cpp` vector checks
- **Process builtin**: `#proc_run(cmd)` returns `{"exit": code; "output": stdout}`
- **FFI**: `#ffi_call("dll"; "func"; arg...)` — call C functions from shared libraries (Windows x64 LoadLibrary/GetProcAddress; Linux/macOS dlopen/dlsym); args: int / string (`char*`), up to 6 args, 64-bit integer result
- New smoke test `tests/tier3.ves` (+ `tier3_data.json`) with C++-generated golden

## 1.1.0## 1.1.0

### Added
- **Concurrency builtins**: `#thread("fn"; arg...)` (run a function in a new thread with an isolated copy of global variables; returns a thread id), `#thread_join(id)` (wait and collect the `back` value, rethrowing the thread's error), `#thread_count()`, `#lock("name")` / `#unlock("name")` (named mutexes for shared resources); `print` output is serialized and unjoined threads are joined at exit
- **Networking builtins** (require `curl` in PATH): `#http_get(url)`, `#http_post(url; body)`, `#tcp_ping(host; port)` (native TCP probe via Winsock on Windows / BSD sockets elsewhere)
- **Binary builtins**: `#bin_read(path)` / `#bin_write(path; bytes)` (byte lists), `#bin_hex(bytes)` / `#bin_unhex(s)`, `#bin_base64_encode(bytes)` / `#bin_base64_decode(s)` — bytes are plain ints so existing list ops apply
- New smoke tests `tests/concurrency.ves` / `tests/binary.ves` with C++-generated golden baselines (the frozen Python reference has no 1.1 builtins)

## 1.0.0## 1.0.0

### Added
- **Package manager (vpm)**: `vesna --pkg` — init / install (`dir` | `zip` | `owner:repo` | registry package name) / remove / list / search / registry; packages live in `<VESNA_HOME>\packages\<name>\<name>.ves` and are imported via `import <name>`; `install <name>` resolves the package from the cached registry index; default registry is the [Vesna Package Garden](https://youakasakura-YAS.github.io/vesna-pkg/)
- **Debugger**: `vesna --debug` — breakpoints (`b` / `del`), continue / next / step, expression eval (`p`), variables (`vars`), backtrace (`bt`), source listing (`list`); pauses on the first line by default
- New builtin `#cpdir(src; dst)`: recursively copy a directory
- **Cross-platform**: new `src/cpp/platform.h` abstraction (UTF-8/16 conversion, shell, cwd, chdir, environment, temp dir); CMake build for Windows / Linux / macOS; registry builtins (`-regwrite` / `-regdelete` / `-regenv`) report "not supported" on non-Windows; `#platform` returns `windows` / `linux` / `mac`
- Examples golden baselines are now generated by the C++ implementation (authoritative; the Python reference is frozen for regression only)
- **General-purpose expansion**: 70 new builtins
  - Math: `#sqrt` `#floor` `#ceil` `#exp` `#log` `#log10` `#sin` `#cos` `#tan` `#sign` `#clamp` `#rand` `#randint` `#choice` `#shuffle`
  - Number bases: `#hex` `#bin` `#oct`
  - Strings: `#pad` `#lpad` `#rpad` `#format` `#hash`
  - Lists: `#range` `#first` `#last` `#take` `#drop` `#set` `#flatten` `#zip` `#insert` `#remove` `#index_of` `#enumerate` `#concat`
  - Dicts: `#get` `#items` `#pop_key`
  - Type checks: `#is_str` `#is_int` `#is_float` `#is_bool` `#is_list` `#is_dict` `#is_none` `#is_group`
  - Time & system: `#now` `#date` `#sleep` `#ticks` `#platform` `#temp_dir`
  - Files: `#fremove` `#fmove` `#fsize` `#is_dir` `#is_file` `#mkdirs`
  - Encoding: `#base64_encode` `#base64_decode` `#url_encode` `#url_decode`
  - Functional: `#each` `#all` `#any` `#find_first` `#sort_by`
  - Exceptions: `#throw` `#assert`
- CI adds a golden comparison step for the 0.4 builtin smoke test (`tests/smoke04.ves`)
- New builtin `#regenv(name)`: read a user environment variable (`HKCU\Environment`)
- `vesna --uninstall`: removes the install directory, cleans `VESNA_HOME` / `PATH`, and deletes the `.ves` / `VesnaScript` registry association
- `--install` now also writes the `OpenWithProgids` entry (previously only in `install-assoc.reg`), fully integrating the association into the installer
- `#regdelete` now deletes the whole key tree recursively

### Changed
- Version bumped to 1.0.0
- `#format` supports precision (`%.2f`) in addition to `%s` `%d` `%f` `%%`

## 0.3.0

### Added
- Builtins `#regwrite`, `#regdelete`, `#shell`, `#path_clean`
- `vesna --install` with `.ves` file-association registration
- `vesna --version`, `vesna --help`
- File-type icons, "Run with Vesna" context menu, New-file menu
- **C++ implementation** (`src/cpp/`, statically linked, line-by-line compatible with the Python reference)

### Fixed
- Version bumped to 0.3.0
- Comment parsing no longer strips `/* */` inside strings
- `if`/`while` conditions now use truthiness (0/empty is falsy)
- File/registry/shell builtins throw catchable errors via `try/catch`
- Trailing tokens after assignment statements are rejected (previously silently ignored)
- Removed dead code branches; added argument boundary checks

### Changed
- Installer is written entirely in Vesna
- C++ performance optimizations (grep 100k lines: ~2s → ~240ms):
  - Regex cache and literal fast paths
  - Zero-copy variable lookup (`Env::getRef`)
  - `Value` as `std::variant` (~104 → ~40 bytes)
  - Identifier interning (names → integer IDs)
  - Builtin dispatch: if-chain → hash map + switch
  - `-O3 -flto` build; iostream/C-stdio sync disabled

## 0.2.0

### Added
- Bytecode: none yet, currently AST evaluation
- 70+ builtins
- Standard library: csv, json, text, stat
- Tool set: wc, grep, head, tail, sort, uniq, cut, sed, replace, stat, logstat, csv2json, extract
- VSCode syntax highlighting + LSP
- Standalone `vesna.exe`

### Changed
- Builtin prefix changed from `-` to `#`
- `elif` / `else` at the same level as `if`
- `-push` renamed to `-append`

## 0.1.0

Initial version.
