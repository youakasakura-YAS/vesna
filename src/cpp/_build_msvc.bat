@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d F:\Vesna\src\cpp
cl /nologo /std:c++17 /O2 /EHsc /utf-8 /D_CRT_SECURE_NO_WARNINGS /Fe:F:\Vesna\src\cpp\vesna_test.exe vesna.cpp lsp.cpp main.cpp /link ws2_32.lib advapi32.lib
