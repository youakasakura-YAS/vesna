# 贡献指南

[English](CONTRIBUTING.md) | [中文](CONTRIBUTING.zh-CN.md)

欢迎为 Vesna 贡献代码、文档或示例。

## 目录结构

```
src\cpp\            C++ 实现（vesna.hpp / vesna.cpp / main.cpp）
src\cpp\tests\      回归测试资产
lib\                标准库（.ves）
examples\           示例脚本
docs\               文档
bench\              性能基准
```

Python 参考实现位于独立仓库：[youakasakura-YAS/vesna-py](https://github.com/youakasakura-YAS/vesna-py)。

## 开发流程

1. Fork 本仓库，从 `main` 分支新建功能分支
2. C++ 改动必须保持与 Python 参考实现**逐行输出一致**（见下方验证）
3. 用户可见的改动需同步更新 `CHANGELOG.md` 与 `CHANGELOG.zh-CN.md`
4. 提交前验证（Windows / MinGW）：

```bat
cd src\cpp
g++ -std=c++17 -O3 -flto -static -Wall -Wextra vesna.cpp main.cpp -o vesna_cpp.exe -ladvapi32
vesna_cpp.exe tests\regression.ves > cpp.out
git clone https://github.com/youakasakura-YAS/vesna-py
python vesna-py\src\vesna.py tests\regression.ves > py.out
fc cpp.out py.out            :: 必须无差异
```

5. 提交信息用中文或英文均可，描述清楚改动意图

## 回归测试资产

- `src\cpp\tests\regression.ves` — 35 项功能回归（R1–R35）
- `src\cpp\tests\fs_test.ves` — 文件系统内置测试（FS1–FS8，脚本自清理）
- `examples\` — 13 个示例脚本，输出须与 Python 参考实现一致
- `bench\loop.ves` — 循环性能基准

## 性能优化

C++ 版的每一次性能改动都要求：

1. 全量回归通过（regression + 示例 + fs_test）
2. 提供 `bench` 实测（用 `cmd /c "... > nul 2>&1"` 重定向计时，避免管道干扰）
3. 更新 `CHANGELOG.md`

## 许可

贡献的代码默认采用与本仓库相同的 [MIT License](LICENSE)。
