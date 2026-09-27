# Vesna

Vesna language support for Visual Studio Code — native C++ LSP (diagnostics / completion / hover / symbols / folding / go-to-definition / rename / signature help / workspace symbols) plus syntax highlighting for 205 builtins.

## Features

- LSP via `vesna --lsp` (stdio)
- Syntax highlighting: keywords, builtins (`#name`), types, booleans, numbers (`'1'`), strings (`"..."`), `#f"..."` interpolation, comments
- Snippets for common constructs
- Hover documentation for builtins and keywords
- Go to definition / rename for user identifiers

## Requirements

- [Vesna](https://github.com/youakasakura-YAS/vesna) installed and available as `vesna` on PATH, or set `vesna.executablePath` in workspace settings to the full path of `vesna.exe`.

## Usage

1. Install this extension (`.vsix` → Extensions panel → Install from VSIX).
2. Open a `.ves` file. The LSP client starts automatically.
3. If `vesna` is not on PATH, set `vesna.executablePath`.

---

# Vesna（中文）

Vesna 语言的 VS Code 支持扩展 — 原生 C++ LSP（诊断/补全/悬停/符号/折叠/跳转定义/重命名/签名提示/工作区符号）+ 205 内置函数语法高亮。

## 功能

- 通过 `vesna --lsp`（stdio）提供 LSP
- 语法高亮：关键字、内置（`#名称`）、类型、布尔、数字（`'1'`）、字符串（`"..."`）、`#f"..."` 插值、注释
- 常用结构代码片段
- 内置与关键字悬停文档
- 用户标识符跳转定义 / 重命名

## 要求

- 已安装 [Vesna](https://github.com/youakasakura-YAS/vesna) 且 `vesna` 在 PATH 中；或在工作区设置 `vesna.executablePath` 指向 `vesna.exe` 完整路径。

## 使用

1. 安装本扩展（`.vsix` → 扩展面板 → 从 VSIX 安装）。
2. 打开 `.ves` 文件，LSP 客户端自动启动。
3. `vesna` 不在 PATH 时，设置 `vesna.executablePath`。
