# Vesna 1.5.0 Release Notes / 发布说明

> 中文见下半部分。English first.

## English

Vesna 1.5.0 focuses on the **package ecosystem** and **developer tooling**.

### What's new
- **`vesna --pkg publish`** — publish a package in one command: validate metadata, pack `entry` + `vesna-pkg.json` into `<name>-<version>.zip`, compute zip `sha256`, write `registry-entry.json`, and append to a local `registry.json` if present.
- **`#call(fname; arg...)`** (builtin 176) — dynamic invocation by function name; enables frameworks inside packages.
- **`vesna-test` 1.0.0** — unit test framework package: `test_case(name; "fn")`, `test_run()`, `test_eq`, `test_assert`, `test_true`, `test_count`. Published to the vesna-pkg registry (`vesna --pkg install vesna-test`).
- **Import honors package `entry`** — `import <pkg>` reads `vesna-pkg.json`'s `entry`; imported globals can see caller functions (parent env chain).
- **`vesna --fmt <file>`** — syntax-preserving formatter (indent, trailing whitespace, blank lines).
- **Syntax highlighting** extended to all 176 builtins (tmLanguage).
- **REPL Tab completion** on Windows — `#js<Tab>` completes to `json_encode`/`json_decode` etc.

### Files
- `bin\vesna.exe` — interpreter (static, no runtime deps)
- `lib\pkg.ves` — package manager (init/install/remove/list/search/registry/publish)
- `vesna-0.6.0.vsix` — VSCode extension (spawns `vesna --lsp`)
- bilingual README / CHANGELOG / RELEASE-NOTES

### Verify
- Golden regression: tier3/binary/tier4/examples all byte-identical
- LSP smoke: 8/8 assertions pass
- vpm loop: init → publish → install(zip) → import → call verified

---

## 中文

Vesna 1.5.0 聚焦**包生态**与**开发者工具链**。

### 新增
- **`vesna --pkg publish`** —— 一条命令发布包：校验元数据、打包 `entry` + `vesna-pkg.json` 为 `<name>-<version>.zip`、计算 zip `sha256`、写入 `registry-entry.json`，若存在本地 `registry.json` 自动追加。
- **`#call(fname; arg...)`**（内置 176）—— 按函数名动态调用，支撑包内框架。
- **`vesna-test` 1.0.0** —— 单元测试框架包：`test_case(name; "fn")`、`test_run()`、`test_eq`、`test_assert`、`test_true`、`test_count`。已发布至 vesna-pkg registry（`vesna --pkg install vesna-test`）。
- **import 支持包 `entry` 字段** —— `import <pkg>` 读取 `vesna-pkg.json` 的 `entry`；导入包全局经 parent 链可见调用方函数。
- **`vesna --fmt <file>`** —— 纯语法保持的格式化器（缩进、行尾空白、空行）。
- **语法高亮**扩展至全部 176 个内置（tmLanguage）。
- **REPL Tab 补全**（Windows）—— `#js<Tab>` 补全为 `json_encode`/`json_decode` 等。

### 文件
- `bin\vesna.exe` —— 解释器（静态链接，无运行时依赖）
- `lib\pkg.ves` —— 包管理器（init/install/remove/list/search/registry/publish）
- `vesna-0.6.0.vsix` —— VSCode 插件（spawn `vesna --lsp`）
- 双语 README / CHANGELOG / RELEASE-NOTES

### 验证
- Golden 回归：tier3/binary/tier4/examples 全字节一致
- LSP 冒烟：8/8 断言通过
- vpm 闭环：init → publish → install(zip) → import → 调用 已验证
