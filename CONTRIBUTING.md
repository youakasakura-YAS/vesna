# 贡献指南

欢迎为 Vesna 贡献代码、文档或示例。

## 项目结构

```
src\
  vesna.py         Python 参考实现（语言语义的权威版本）
  cpp\              C++ 实现（vesna.hpp / vesna.cpp / main.cpp）
lib\               标准库（.ves）
examples\          示例脚本
vesna-vscode\      VSCode 插件（语法高亮 + LSP）
docs\              文档
bench\             性能基准
```

## 开发流程

1. **Fork 本仓库**，从 `main` 分支新建功能分支
2. **改 Python 版**时：`src\vesna.py` 是语义权威，改动需同时更新 `CHANGELOG.md`
3. **改 C++ 版**时：必须保持与 Python 版**逐行输出一致**（见下方验证）
4. **提交前验证**（Windows / MinGW）：

```bat
cd src\cpp
g++ -std=c++17 -O3 -flto -static -Wall -Wextra vesna.cpp main.cpp -o vesna_cpp.exe -ladvapi32
vesna_cpp.exe tests\regression.ves > cpp.out
python ..\vesna.py tests\regression.ves > py.out
fc cpp.out py.out            :: 必须无差异
```

5. 提交信息用中文或英文均可，描述清楚改动意图

## 回归测试资产

- `src\cpp\tests\regression.ves` — 35 项功能回归（R1–R35）
- `src\cpp\tests\fs_test.ves` — 文件系统内置测试（FS1–FS8，脚本自清理）
- `examples\` — 13 个示例脚本，全部要求 C++ 版与 Python 版输出一致
- `bench\loop.ves` — 循环性能基准

## 性能优化

C++ 版性能优化（正则缓存、Value 判别联合、标识符 intern 等）的每一次改动都要求：

1. 全量回归通过（regression + 示例 + fs_test）
2. 提供 `bench` 实测对比（用 `cmd /c "... > nul 2>&1"` 重定向计时，避免管道干扰）
3. 更新 `CHANGELOG.md`

## 许可

贡献的代码默认采用与本仓库相同的 [MIT License](LICENSE)。
