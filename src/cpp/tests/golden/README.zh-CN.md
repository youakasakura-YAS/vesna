# Golden 基准

本目录是 C++ 实现逐行一致性对照的权威基准。

- `regression.out` / `fs_test.out` / `smoke04.out`：由冻结的 **Python 参考实现 1.0.0**（youakasakura-YAS/vesna-py）生成，仅用于回归对照。
- `examples/*.out`：由 **C++ 实现**（权威）生成；py 停更后 `#fread` + `#len` 语义存在分歧（py 按字符数、C++ 按 UTF-8 字节数），故以 C++ 为准。

## 生成命令

```bat
set PYTHONIOENCODING=utf-8
set VESNA_HOME=repo根目录

cd src\cpp
python repo根\src\vesna.py tests\regression.ves > tests\golden\regression.out
python repo根\src\vesna.py tests\fs_test.ves    > tests\golden\fs_test.out
python repo根\src\vesna.py tests\smoke04.ves    > tests\golden\smoke04.out

cd examples
repo根\bin\vesna.exe csv2json.ves sample.csv          > ..\src\cpp\tests\golden\examples\csv2json.out
repo根\bin\vesna.exe cut.ves "," "2" sample.csv       > ..\src\cpp\tests\golden\examples\cut.out
repo根\bin\vesna.exe extract.ves sample.txt           > ..\src\cpp\tests\golden\examples\extract.out
repo根\bin\vesna.exe grep.ves "ERROR" sample.txt      > ..\src\cpp\tests\golden\examples\grep.out
repo根\bin\vesna.exe head.ves "3" sample.txt          > ..\src\cpp\tests\golden\examples\head.out
repo根\bin\vesna.exe logstat.ves sample.txt           > ..\src\cpp\tests\golden\examples\logstat.out
repo根\bin\vesna.exe replace.ves "a" "X" sample.txt   > ..\src\cpp\tests\golden\examples\replace.out
repo根\bin\vesna.exe sed.ves "\d+" "N" sample.txt     > ..\src\cpp\tests\golden\examples\sed.out
repo根\bin\vesna.exe sort.ves sample.txt              > ..\src\cpp\tests\golden\examples\sort.out
repo根\bin\vesna.exe stat.ves sample.txt              > ..\src\cpp\tests\golden\examples\stat.out
repo根\bin\vesna.exe tail.ves "3" sample.txt          > ..\src\cpp\tests\golden\examples\tail.out
repo根\bin\vesna.exe uniq.ves sample.txt              > ..\src\cpp\tests\golden\examples\uniq.out
repo根\bin\vesna.exe wc.ves sample.txt                > ..\src\cpp\tests\golden\examples\wc.out
```

**注意**：
- 输出重定向到文件时需 `PYTHONIOENCODING=utf-8`，与 C++ 版 UTF-8 输出保持一致。
- `fs_test.ves` 会创建并自清理临时文件，生成 golden 时 cwd 须在 `src\cpp`。
- 仅当权威实现语义变更时才重新生成本目录。
