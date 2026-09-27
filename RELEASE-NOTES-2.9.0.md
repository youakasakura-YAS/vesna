# Vesna 2.9.0 Release Notes

## What's New

- **New builtin `#readline()`**: read one line from stdin (empty string on EOF) — enables line-based interaction for the resident process protocol and interactive tools.
- **`vesna-mc` 1.2.0 — resident process mode**: Minecraft bridge events no longer spawn short-lived processes each time. The mod starts a long-lived `vesna` child (`resident.ves`) and dispatches events over a stdin/stdout JSON line protocol — zero cold-start cost for high-frequency events such as `server_tick`. Enabled by `"resident": true` in `events.json` (default on); set it to `false` to fall back to one-shot mode.
- **VSCode extension 2.9.0**: syntax highlighting now covers 215 builtins.

## Verification

- Regression: 4/4 MATCH; LSP: 10/10.
- Resident mode: generator assertions 5/5; end-to-end script protocol 15/15; ResidentBridge javac syntax-level check passed.

## Files

- `bin/vesna.exe` — Vesna 2.9.0 (Windows x64, static)
- `vesna-2.9.0.vsix` — VSCode extension
- `docs/` — builtins (215) and syntax reference (bilingual)
- `examples/` — sample scripts

---
# Vesna 2.9.0 发布说明

## 新增

- **新内置 `#readline()`**：从 stdin 读取一行（EOF 返回空串）——为常驻进程协议与交互工具提供逐行输入。
- **`vesna-mc` 1.2.0 常驻进程模式**：Minecraft 桥接事件不再逐次启动短生命周期进程。模组拉起长驻 `vesna` 子进程（`resident.ves`），以 stdin/stdout JSON 行协议分发事件——`server_tick` 等高频事件零冷启动开销。`events.json` 置 `"resident": true` 启用（默认开）；改为 `false` 回到单次模式。
- **VSCode 插件 2.9.0**：语法高亮覆盖 215 个内置函数。

## 验证

- 回归 4/4 MATCH；LSP 10/10。
- 常驻模式：生成器断言 5/5；脚本协议端到端 15/15；ResidentBridge 本机 javac 语法级验证通过。

## 文件

- `bin/vesna.exe` — Vesna 2.9.0（Windows x64，静态）
- `vesna-2.9.0.vsix` — VSCode 扩展
- `docs/` — 内置函数（215）与语法参考（双语）
- `examples/` — 示例脚本
