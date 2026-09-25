# Vesna 1.7.0 Release Notes / 发布说明

> 中文见下半部分。English first.

## English

Vesna 1.7.0 makes the interpreter **a single file**. The standard library (csv / json / pkg / stat / text, ~19 KB total) is now compiled into the executable, so `vesna.exe` works standalone: no `lib/` directory, no `VESNA_HOME`, no side files.

### What changed
- Standard library embedded: `import csv`, `import json`, `import text`, `import stat`, and the built-in package manager (`--pkg`) all work from the bare exe.
- External files still win: if a `lib/*.ves` (or a script-adjacent `.ves`) exists, it is loaded instead of the embedded copy — keep your overrides.
- Version bump 1.6.0 → 1.7.0. Binary size grows by ~19 KB.

### Usage
```bash
# 单文件：把 vesna.exe 拷到任何位置直接运行
vesna.exe script.ves
vesna.exe --pkg install hello_vesna
```

### Verify
- Standalone test: exe copied to an empty folder (no lib, no VESNA_HOME) — `import csv/json/text/stat`, `--pkg list` all work; LSP smoke PASS.
- Full golden regression byte-identical.

---

## 中文

Vesna 1.7.0 让解释器变成**单文件**。标准库（csv / json / pkg / stat / text，共约 19 KB）已编译进可执行文件，`vesna.exe` 独立可用：无需 `lib/` 目录、无需 `VESNA_HOME`、无需任何旁挂文件。

### 变更内容
- 标准库内嵌：`import csv`、`import json`、`import text`、`import stat` 以及内置包管理器（`--pkg`）全部在裸 exe 上可用。
- 外部文件仍优先：若存在 `lib/*.ves`（或脚本旁的 `.ves`），优先加载外部副本——自定义覆盖仍然有效。
- 版本 1.6.0 → 1.7.0。二进制体积增加约 19 KB。

### 用法
```bash
# 把 vesna.exe 拷到任何位置直接运行
vesna.exe script.ves
vesna.exe --pkg install hello_vesna
```

### 验证
- 单文件实测：exe 拷到空目录（无 lib、无 VESNA_HOME）——`import csv/json/text/stat`、`--pkg list` 全部可用；LSP 冒烟通过。
- 全量 golden 回归字节级一致。
