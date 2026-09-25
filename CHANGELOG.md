# Changelog

## 0.3.0

### Added
- 内置 `#regwrite`、`#regdelete`、`#shell`、`#path_clean`
- `vesna --install` 自动注册文件关联
- `vesna --version`、`vesna --help`
- 文件类型图标、右键"用 Vesna 运行"、新建菜单
- **C++ 高性能实现**（`src/cpp/`，静态链接无 DLL 依赖，与 Python 版语义逐行一致）
- VSCode 插件更新至 0.3.0（LSP 诊断/补全同步新内置函数）

### Fixed
- 版本号统一为 0.3.0
- 注释解析不再误删字符串内的 `/* */`
- `if`/`while` 条件支持真值判断（数字 0/空为假，非 0/非空为真）
- 文件/注册表/命令类内置出错时抛出可被 `try/catch` 捕获的错误
- 拒绝赋值语句尾部多余内容（此前被静默忽略）
- 清理重复的无效代码分支，补充参数边界检查

### Changed
- 安装程序完全用 Vesna 写
- C++ 版性能优化（grep 10 万行：约 2s → 约 240ms）：
  - 正则缓存与字面量快速路径
  - 变量读取零拷贝（`Env::getRef`）
  - `Value` 判别联合（`std::variant`，约 104 字节 → 约 40 字节）
  - 标识符 intern（变量/函数名 → 整数 ID）
  - 内置函数 if 链 → 哈希分发 + switch
  - `-O3 -flto` 编译、关闭 iostream 与 C stdio 同步

## 0.2.0

### Added
- 字节码 → 暂无，当前是 AST 求值
- 内置函数 70+
- 标准库：csv、json、text、stat
- 工具集：wc、grep、head、tail、sort、uniq、cut、sed、replace、stat、logstat、csv2json、extract
- VSCode 语法高亮 + LSP
- 独立 `vesna.exe`

### Changed
- 内置函数前缀从 `-` 改为 `#`
- `elif` / `else` 与 `if` 同层
- `-push` 改名为 `-append`

## 0.1.0

初始版本。
