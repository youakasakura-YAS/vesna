# Vesna 2.8.0 Release Notes / 发布说明

> 中文见下半部分。English first.

## English

Vesna 2.8.0 grows the Minecraft story: `vesna-mc` 1.1.0 turns the bridge mod into a real modding surface — events, actions and timers driven entirely by Vesna scripts.

### `vesna-mc` 1.1.0 — Minecraft modding via Vesna
- **Event system (11)**: `server_started` / `server_stopped` / `player_join` / `player_leave` / `player_death` / `player_kill` / `block_break` / `block_place` / `player_chat` / `player_advancement` / `server_tick` (every second)
- **Timers**: `events.json` `"timers"` array declares periodic scripts (e.g. broadcast every 30 s)
- **Action system (8 types)**: scripts return `{"actions":[...]}` and the mod executes `message` (broadcast / `player:` private), `command`, `give`, `kick`, `effect`, `tp`, `sound`, `log`; `{"message":"..."}` is a quick broadcast
- 10 new example script templates (death / kill / break / place / chat reply / advancement / tick / timer, …)
- Generator fix: all 11 event scripts + timer scripts are now copied into the generated project (previously only 3)
- Platform entrypoints updated for Fabric / Forge / NeoForge (MC 1.20.1+); bridge classes stay pure Java

### Verification
- Generator: 6/6 pass (3 platforms × files + bridge v2 + entry v2)
- Bridge classes: javac zero errors
- Script protocol: end-to-end 11/11 (subprocess argv, same as the mod's ProcessBuilder)
- Language regression: 4/4 golden byte-identical; LSP 10/10

### Official packages
- `vesna-mc` 1.1.0 (updated in the package registry; install with `vesna --pkg install vesna-mc`)
- `vesna-java` / `vesna-android` / `vesna-dev` 1.0.0 (unchanged)

---

## 中文

Vesna 2.8.0 继续讲 Minecraft 的故事：`vesna-mc` 1.1.0 把桥接模组升级为真正的模组开发面——事件、动作、定时任务全部由 Vesna 脚本驱动。

### `vesna-mc` 1.1.0 —— 用 Vesna 写模组功能
- **事件系统（11 个）**：`server_started` / `server_stopped` / `player_join` / `player_leave` / `player_death` / `player_kill` / `block_break` / `block_place` / `player_chat` / `player_advancement` / `server_tick`（每秒）
- **定时任务**：`events.json` 的 `timers` 数组声明周期脚本（如每 30 秒广播）
- **动作系统（8 类）**：脚本返回 `{"actions":[...]}`，模组执行 `message`（广播 / `player:` 私聊）、`command`、`give`、`kick`、`effect`、`tp`、`sound`、`log`；`{"message":"..."}` 快捷广播
- 新增 10 个示例脚本模板（死亡 / 击杀 / 破坏 / 放置 / 聊天回复 / 进度 / 节拍 / 计时器等）
- 生成器修复：生成项目现在会复制全部 11 个事件脚本与定时脚本（此前只复制 3 个）
- Fabric / Forge / NeoForge（MC 1.20.1+）平台入口全部更新；桥接类保持纯 Java

### 验证
- 生成器：6/6 通过（三平台 × 文件 + 桥 v2 + 入口 v2）
- 桥接类：javac 零错误
- 脚本协议：端到端 11/11（subprocess argv，与模组 ProcessBuilder 一致）
- 语言回归：4/4 golden 逐字节一致；LSP 10/10

### 官方包
- `vesna-mc` 1.1.0（registry 已更新；`vesna --pkg install vesna-mc` 安装）
- `vesna-java` / `vesna-android` / `vesna-dev` 1.0.0（未变）
