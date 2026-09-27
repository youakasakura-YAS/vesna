# Vesna 2.0.0 Release Notes / 发布说明

> 中文见下半部分。English first.

## English

Vesna 2.0.0 is the start of the 2.0 iteration line — kernel polish first. This release hardens the debugger, improves error reporting, folds string literals at parse time, and gives the REPL command history.

### Debugger (`vesna --debug`)
- **Conditional breakpoints**: `b <line> if <expr>` stops only when the expression is true
- **Watch expressions**: `watch <expr>` evaluates and prints on every stop; `watches` lists them, `unwatch <n>` removes
- **Modify variables**: `set <name> = <expr>` writes a new value into the current scope
- **Finish**: `finish` runs until the current function returns
- **Clear all**: `del all` removes every breakpoint
- Existing commands unchanged: `c/n/s`, `b`, `del`, `p`, `vars`, `bt`, `list`, `q`

### Better errors
- Runtime / parse errors now print the offending source line (and a `^` caret column when available)
- `VesnaError` carries an optional column; the plumbing is in place for column-accurate diagnostics

### Performance
- String-literal concatenation is folded at parse time: `"a" + "b"` becomes `"ab"` in the AST — no runtime cost, semantics unchanged

### REPL
- Up/Down arrows browse command history

### Verify
- All four golden suites byte-identical (`regression`, `binary`, `tier4`, `smoke04`)
- Debugger commands exercised end to end (conditional break, watch, set, finish)
- Error context output checked; string folding output checked

---

## 中文

Vesna 2.0.0 开启 2.0 迭代主线，先打磨内核：调试器更完整、错误报告带源码上下文、字符串字面量编译期折叠、REPL 支持命令历史。

### 调试器（`vesna --debug`）
- **条件断点**：`b <行号> if <条件>`，条件为真才停下
- **监视表达式**：`watch <表达式>` 每次停下自动求值并显示；`watches` 列出，`unwatch <序号>` 删除
- **修改变量**：`set <变量> = <表达式>` 直接改写当前作用域的值
- **运行到返回**：`finish` 一直运行到当前函数返回
- **清空断点**：`del all` 一次删除全部断点
- 原有命令不变：`c/n/s`、`b`、`del`、`p`、`vars`、`bt`、`list`、`q`

### 更好的错误
- 解析/运行错误现在会带出错的源码行（有列号时显示 `^` 定位）
- `VesnaError` 增加可选列号字段，为后续列级诊断铺路

### 性能
- 字符串字面量拼接在解析期折叠：`"a" + "b"` 在 AST 中直接成为 `"ab"`，运行时零开销，语义不变

### REPL
- 上下方向键浏览命令历史

### 验证
- 四套 golden 回归逐字节一致（`regression`、`binary`、`tier4`、`smoke04`）
- 调试器命令端到端实测（条件断点/watch/set/finish）
- 错误上下文输出、字符串折叠输出均已核对
