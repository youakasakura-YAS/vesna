# Golden 基准

本目录由 **Python 参考实现 0.4.0**（youakasakura-YAS/vesna-py）生成，作为 C++ 实现逐行一致性对照的权威基准。

## 生成命令

```bat
set PYTHONIOENCODING=utf-8
set VESNA_HOME=repo根目录

cd src\cpp
python repo根\src\vesna.py tests\regression.ves > tests\golden\regression.out
python repo根\src\vesna.py tests\fs_test.ves    > tests\golden\fs_test.out
python repo根\src\vesna.py tests\smoke04.ves    > tests\golden\smoke04.out

cd examples
python repo根\src\vesna.py csv2json.ves sample.csv          > ..\src\cpp\tests\golden\examples\csv2json.out
python repo根\src\vesna.py cut.ves "," "2" sample.csv       > ..\src\cpp\tests\golden\examples\cut.out
python repo根\src\vesna.py extract.ves sample.txt           > ..\src\cpp\tests\golden\examples\extract.out
python repo根\src\vesna.py grep.ves "ERROR" sample.txt      > ..\src\cpp\tests\golden\examples\grep.out
python repo根\src\vesna.py head.ves "3" sample.txt          > ..\src\cpp\tests\golden\examples\head.out
python repo根\src\vesna.py logstat.ves sample.txt           > ..\src\cpp\tests\golden\examples\logstat.out
python repo根\src\vesna.py replace.ves "a" "X" sample.txt   > ..\src\cpp\tests\golden\examples\replace.out
python repo根\src\vesna.py sed.ves "\d+" "N" sample.txt     > ..\src\cpp\tests\golden\examples\sed.out
python repo根\src\vesna.py sort.ves sample.txt              > ..\src\cpp\tests\golden\examples\sort.out
python repo根\src\vesna.py stat.ves sample.txt              > ..\src\cpp\tests\golden\examples\stat.out
python repo根\src\vesna.py tail.ves "3" sample.txt          > ..\src\cpp\tests\golden\examples\tail.out
python repo根\src\vesna.py uniq.ves sample.txt              > ..\src\cpp\tests\golden\examples\uniq.out
python repo根\src\vesna.py wc.ves sample.txt                > ..\src\cpp\tests\golden\examples\wc.out
```

**注意**：
- 输出重定向到文件时需 `PYTHONIOENCODING=utf-8`，与 C++ 版 UTF-8 输出保持一致
- `fs_test.ves` 会创建并自清理临时文件，生成 golden 时 cwd 须在 `src\cpp`
- 仅当 Python 参考实现语义变更时才重新生成本目录
