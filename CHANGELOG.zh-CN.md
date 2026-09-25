# 更新日志

[English](CHANGELOG.md) | [中文](CHANGELOG.zh-CN.md)

## 1.5.0

### 新增
- **vpm publish**：`vesna --pkg publish` —— 一键发布包：校验 `vesna-pkg.json`、将 `entry` + 元数据打包为 `<name>-<version>.zip`（Windows 用 PowerShell `Compress-Archive`，其他平台用 `zip`）、计算 zip `sha256`、写入 `registry-entry.json`，若存在本地 `registry.json` 则自动追加条目
- **`#call(fname; arg...)` 内置**（dispatch 176）：按函数名字符串动态调用 —— 支撑包级框架（如新的 `vesna-test` 测试框架）
- **`vesna-test` 包 1.0.0**：单元测试框架（`test_case(name; "fn")`、`test_run()`、`test_eq`、`test_assert`、`test_true`、`test_count`）—— 已发布到 vesna-pkg registry
- **import 支持包的 `entry` 字段**：`import` 现在会读取 vpm 包 `vesna-pkg.json` 的 `entry`（此前只尝试 `<包名>.ves`）；导入包的全局环境通过 parent 链可查看到调用方脚本的函数
- **`vesna --fmt <file>`**：源码格式化器 —— 规范化缩进（`-` 层数）、去除行尾空白、压缩连续空行（纯行级，不改语法）
- **语法高亮 1.1-1.4**：`vesna.tmLanguage.json` 内置模式扩展至全部 176 个内置（thread/lock/http/bin/json/crypto/proc/ffi/csv/ini/xml/call）
- **REPL Tab 补全**（Windows）：输入 `#js<Tab>` 等 —— 唯一匹配自动补全，多匹配列出候选

## 1.4.0


### 新增
- **原生 C++ LSP**：`vesna.exe --lsp` —— 内置语言服务器（stdio JSON-RPC），诊断复用 C++ `Parser` 错误收集；补全覆盖全部内置（单一名字表 + 文档标识符，`#` 触发）；悬停（内置/关键字文档 + 函数定义）；`documentSymbol`（函数）；`foldingRange`（缩进块）
- **VSCode 插件 0.5.0**：`vesna-0.5.0.vsix` —— 服务器改为启动 `vesna --lsp`（不再依赖 Python）；Python LSP 服务器目录已移除
- **第四梯队内置**（dispatch 170-175）：`#csv_parse(s)` → 行列列表；`#csv_build(rows)` → CSV 文本（RFC-4180 引号）；`#ini_read(path)` / `#ini_write(path; data)`（节/键/值，`;` `#` 注释）；`#xml_parse(s)` → 简易 DOM `{tag; attrs; children; text}`（属性、嵌套、自闭合、`&amp;` `&lt;` `&gt;` `&quot;` `&apos;`）；`#ffi_call_s(dll; func; arg...)` → `char*` 结果转为字符串
- **CI 修复**：编译补 `-lws2_32`（1.1 网络内置）；golden 回归扩展 tier3 / binary / tier4；concurrency 运行检查；vpm 校验冒烟；LSP 冒烟直接驱动编译出的 `vesna.exe --lsp`；`release.yml` 对齐 1.4.0（`vesna-0.5.0.vsix`、`-lws2_32`、双语 README/CHANGELOG/CONTRIBUTING + RELEASE-NOTES）
- **内置单源事实**：175 项名字表 `g_builtinNames` 同时驱动词法内置集合、分发表与 LSP 补全（此前是三份手工维护的表）

## 1.3.1

### 修复
- `--install` 现在支持在发布包的 `bin\` 目录内直接运行（安装程序会自动识别当前目录末尾的 `\bin` 并上溯到包根目录）。此前会报「找不到 bin\vesna.exe」。

## 1.3.0## 1.3.0

### 新增
- **vpm 校验增强**（`lib/pkg.ves`）：元数据校验（`name` 须匹配 `^[a-z][a-z0-9_-]+$`、`version` 须为 `x.y.z`、`entry` 必须存在）、registry 条目可选 `sha256` 完整性校验、依赖自动安装（含版本检查与循环保护）、`remove` 依赖保护（`--force` 覆盖）。`init [名称]` 现在校验包名。
- **LSP 0.4.0**（`vesna-vscode/server`）：解析器同步至冻结的 1.0.0 参考实现（覆盖 0.4 全部语法诊断）；补全覆盖全部 169 个内置（含 1.1/1.2 梯队）；悬停显示内置/关键字文档；`documentSymbol`（函数）；`foldingRange`（缩进块）；`#` 补全触发器。
- **VSCode 插件 0.4.0**：`vesna-0.4.0.vsix`，serverInfo 0.4.0，更新描述。

## 1.2.0## 1.2.0

### 新增
- **数据内置**：`#json_encode(v)` / `#json_decode(s)`（原生 JSON，dict 保持插入顺序）、`#re_groups(s; pattern)`（正则捕获组，未匹配组为 `none`）
- **加密内置**：`#sha256(s)`（已按 FIPS 180-4 向量验证）、`#aes_encrypt(data; key)` / `#aes_decrypt(b64; key)`（AES-256-CBC + PKCS7，密钥经 SHA-256 派生，base64 输出，已按 FIPS-197 验证）；新增纯 C++ 的 `src/cpp/crypto.h` 与 `crypto_selftest.cpp` 向量校验
- **进程内置**：`#proc_run(cmd)` 返回 `{"exit": 退出码; "output": stdout}`
- **FFI**：`#ffi_call("dll"; "func"; arg...)` —— 从共享库调用 C 函数（Windows x64 用 LoadLibrary/GetProcAddress；Linux/macOS 用 dlopen/dlsym）；参数支持 int / 字符串（`char*`），最多 6 个，返回 64 位整数结果
- 新增冒烟测试 `tests/tier3.ves`（+ `tier3_data.json`），golden 由 C++ 生成

## 1.1.0## 1.1.0

### 新增
- **并发内置**：`#thread("fn"; arg...)`（在新线程中运行函数，全局变量按副本隔离，返回线程 id）、`#thread_join(id)`（等待并取回 `back` 值，线程出错则重新抛出）、`#thread_count()`、`#lock("name")` / `#unlock("name")`（命名互斥锁，用于共享资源保护）；`print` 输出已互斥，未 join 的线程在退出时自动等待
- **网络内置**（需系统 PATH 中有 curl）：`#http_get(url)`、`#http_post(url; body)`、`#tcp_ping(host; port)`（原生 TCP 探测：Windows 用 Winsock，其他平台用 BSD socket）
- **二进制内置**：`#bin_read(path)` / `#bin_write(path; bytes)`（字节列表）、`#bin_hex(bytes)` / `#bin_unhex(s)`、`#bin_base64_encode(bytes)` / `#bin_base64_decode(s)` —— 字节即普通整数，可直接套用现有列表操作
- 新增冒烟测试 `tests/concurrency.ves` / `tests/binary.ves`，golden 由 C++ 生成（冻结的 Python 参考实现不含 1.1 内置）

## 1.0.0## 1.0.0

### 新增
- **包管理器（vpm）**：`vesna --pkg` — init / install（`目录` | `zip` | `owner:repo` | registry 包名）/ remove / list / search / registry；包位于 `<VESNA_HOME>\packages\<名称>\<名称>.ves`，通过 `import <名称>` 导入；`install <包名>` 从缓存的 registry 索引解析包；默认 registry 为 [Vesna 包花园](https://youakasakura-YAS.github.io/vesna-pkg/)
- **调试器**：`vesna --debug` — 断点（`b` / `del`）、继续 / 下一行 / 步入、表达式求值（`p`）、变量（`vars`）、调用栈回溯（`bt`）、源码列表（`list`）；默认停在第一行
- 新内置 `#cpdir(src; dst)`：递归复制目录
- **跨平台**：新增 `src/cpp/platform.h` 抽象层（UTF-8/16 转换、shell、cwd、chdir、环境变量、临时目录）；CMake 构建支持 Windows / Linux / macOS；注册表内置（`-regwrite` / `-regdelete` / `-regenv`）在非 Windows 平台报"不支持"；`#platform` 返回 `windows` / `linux` / `mac`
- examples golden 基准改为由 C++ 实现生成（权威；Python 参考实现已冻结，仅用于回归）
- **通用语言扩充**：新增 70 个内置函数
  - 数学：`#sqrt` `#floor` `#ceil` `#exp` `#log` `#log10` `#sin` `#cos` `#tan` `#sign` `#clamp` `#rand` `#randint` `#choice` `#shuffle`
  - 进制：`#hex` `#bin` `#oct`
  - 字符串：`#pad` `#lpad` `#rpad` `#format` `#hash`
  - 列表：`#range` `#first` `#last` `#take` `#drop` `#set` `#flatten` `#zip` `#insert` `#remove` `#index_of` `#enumerate` `#concat`
  - 字典：`#get` `#items` `#pop_key`
  - 类型判断：`#is_str` `#is_int` `#is_float` `#is_bool` `#is_list` `#is_dict` `#is_none` `#is_group`
  - 时间/系统：`#now` `#date` `#sleep` `#ticks` `#platform` `#temp_dir`
  - 文件：`#fremove` `#fmove` `#fsize` `#is_dir` `#is_file` `#mkdirs`
  - 编码：`#base64_encode` `#base64_decode` `#url_encode` `#url_decode`
  - 函数式：`#each` `#all` `#any` `#find_first` `#sort_by`
  - 异常：`#throw` `#assert`
- CI 增加 0.4 内置冒烟测试 golden 对照（`tests/smoke04.ves`）
- 新增内置 `#regenv(name)`：读取用户环境变量（`HKCU\Environment`）
- `vesna --uninstall`：删除安装目录、清理 `VESNA_HOME`/`PATH`、删除 `.ves`/`VesnaScript` 文件关联
- `--install` 补写 `OpenWithProgids` 项（此前仅在 `install-assoc.reg` 中），文件关联完全集成进安装程序
- `#regdelete` 改为递归删除整个键树

### 变更
- 版本号统一为 1.0.0
- `#format` 在 `%s` `%d` `%f` `%%` 之外支持精度写法（`%.2f`）

## 0.3.0

### 新增
- 内置 `#regwrite`、`#regdelete`、`#shell`、`#path_clean`
- `vesna --install` 自动注册文件关联
- `vesna --version`、`vesna --help`
- 文件类型图标、右键"用 Vesna 运行"、新建菜单
- **C++ 实现**（`src/cpp/`，静态链接，与 Python 参考实现逐行一致）

### 修复
- 版本号统一为 0.3.0
- 注释解析不再误删字符串内的 `/* */`
- `if`/`while` 条件支持真值判断（数字 0/空为假，非 0/非空为真）
- 文件/注册表/命令类内置出错时抛出可被 `try/catch` 捕获的错误
- 拒绝赋值语句尾部多余内容（此前被静默忽略）
- 清理重复的无效代码分支，补充参数边界检查

### 变更
- 安装程序完全用 Vesna 写
- C++ 版性能优化（grep 10 万行：约 2s → 约 240ms）：
  - 正则缓存与字面量快速路径
  - 变量读取零拷贝（`Env::getRef`）
  - `Value` 判别联合（`std::variant`，约 104 → 约 40 字节）
  - 标识符 intern（变量/函数名 → 整数 ID）
  - 内置函数 if 链 → 哈希分发 + switch
  - `-O3 -flto` 编译、关闭 iostream 与 C stdio 同步

## 0.2.0

### 新增
- 字节码 → 暂无，当前是 AST 求值
- 内置函数 70+
- 标准库：csv、json、text、stat
- 工具集：wc、grep、head、tail、sort、uniq、cut、sed、replace、stat、logstat、csv2json、extract
- VSCode 语法高亮 + LSP
- 独立 `vesna.exe`

### 变更
- 内置函数前缀从 `-` 改为 `#`
- `elif` / `else` 与 `if` 同层
- `-push` 改名为 `-append`

## 0.1.0

初始版本。
