# Vesna 1.8.0 Release Notes / 发布说明

> 中文见下半部分。English first.

## English

Vesna 1.8.0 hardens the package manager, upgrades the native LSP, and makes file hashing correct for binaries.

### Package manager (vpm) validation hardening
- **Zip path-traversal protection**: every extracted file is checked for `..` or absolute-path entries before install; malicious packages are rejected and cleaned up
- **`entry` lockdown**: `entry` in `vesna-pkg.json` must be a plain filename (no `..`, drive letter, or separators)
- **Registry cache validation**: `registry.json` is structure-validated on load; corrupt caches are reported, deleted and re-downloaded
- **Local cache / offline install**: downloaded packages are cached to `packages\_cache\<name>-<ver>.zip`; if the cache sha256 matches, install works fully offline

### LSP v1.8.0 (native C++)
- `textDocument/definition` — go to definition
- `textDocument/signatureHelp` — parameter hints for builtins and user functions (trigger `(`)
- `workspace/symbol` — cross-document symbols
- Server version bumped; VSCode extension 0.8.0 shows an actionable message when the LSP fails to start

### Core
- `#sha256` now accepts a byte list: `#sha256(#bin_read(path))` hashes real file bytes (previously `#fread` text-mangled binary data — registry sha256 now matches external tools)

### Verify
- Cache offline install, evil-zip rejection, entry traversal rejection, corrupt-registry rebuild — all verified
- LSP definition / signatureHelp / workspaceSymbol verified over stdio
- Full golden regression byte-identical; LSP smoke PASS

---

## 中文

Vesna 1.8.0 加固包管理器、升级原生 LSP，并修正二进制文件的真实哈希。

### 包管理器（vpm）校验增强
- **zip 路径穿越防护**：安装前逐个检查解压文件，含 `..` 或绝对路径条目即拒绝并清理
- **entry 锁定**：`vesna-pkg.json` 的 `entry` 必须是纯文件名（不允许 `..` / 盘符 / 分隔符）
- **registry 缓存校验**：`registry.json` 加载时做结构校验；损坏的缓存会提示、删除并重新下载
- **本地缓存 / 离线安装**：下载的包缓存到 `packages\_cache\<name>-<ver>.zip`；缓存 sha256 匹配时完全离线安装

### LSP v1.8.0（原生 C++）
- `textDocument/definition` —— 跳转定义
- `textDocument/signatureHelp` —— 内置与用户函数参数提示（`(` 触发）
- `workspace/symbol` —— 跨文档符号
- 服务端版本更新；VSCode 插件 0.8.0 在 LSP 启动失败时给出可操作提示

### 核心
- `#sha256` 现接受字节列表：`#sha256(#bin_read(path))` 对真实文件字节做哈希（此前 `#fread` 文本化会破坏二进制——registry sha256 现已与外部工具一致）

### 验证
- 缓存离线安装、恶意 zip 拒绝、entry 穿越拒绝、registry 损坏重建——全部实测通过
- LSP definition / signatureHelp / workspaceSymbol 经 stdio 实测通过
- 全量 golden 回归字节级一致；LSP 冒烟通过
