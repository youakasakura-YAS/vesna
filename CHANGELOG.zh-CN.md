# 更新日志

[English](CHANGELOG.md) | [中文](CHANGELOG.zh-CN.md)

## 0.4.0

### 新增
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
- 版本号统一为 0.4.0
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
