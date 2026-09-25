# Contributing

[English](CONTRIBUTING.md) | [中文](CONTRIBUTING.zh-CN.md)

Welcome! Contributions of code, docs, and examples are appreciated.

## Repository layout

```
src\cpp\            C++ implementation (vesna.hpp / vesna.cpp / main.cpp)
src\cpp\tests\      regression assets
lib\                standard library (.ves)
examples\           example scripts
docs\               documentation
bench\              benchmarks
```

The Python reference implementation lives in its own repository: [youakasakura-YAS/vesna-py](https://github.com/youakasakura-YAS/vesna-py).

## Workflow

1. Fork this repo, create a feature branch from `main`
2. C++ changes must keep **line-by-line output compatibility** with the Python reference (see verification below)
3. Update `CHANGELOG.md` (and `CHANGELOG.zh-CN.md`) for user-visible changes
4. Verify before committing (Windows / MinGW):

```bat
cd src\cpp
g++ -std=c++17 -O3 -flto -static -Wall -Wextra vesna.cpp main.cpp -o vesna.exe -ladvapi32
vesna.exe tests\regression.ves > cpp.out
git clone https://github.com/youakasakura-YAS/vesna-py
python vesna-py\src\vesna.py tests\regression.ves > py.out
fc cpp.out py.out          :: must show no differences
```

5. Commit messages in Chinese or English, describing the change clearly

## Regression assets

- `src\cpp\tests\regression.ves` — 35 functional checks (R1–R35)
- `src\cpp\tests\fs_test.ves` — filesystem builtin checks (FS1–FS8, self-cleaning)
- `examples\` — 13 example scripts, all must match the Python reference output
- `bench\loop.ves` — loop performance benchmark

## Performance work

Every performance change to the C++ implementation must:

1. Pass full regression (regression + examples + fs_test)
2. Provide `bench` measurements (use `cmd /c "... > nul 2>&1"` redirection to avoid pipeline noise)
3. Update `CHANGELOG.md`

## License

Contributions default to the same [MIT License](LICENSE).
