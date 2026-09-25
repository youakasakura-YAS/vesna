# Changelog

[English](CHANGELOG.md) | [中文](CHANGELOG.zh-CN.md)

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
