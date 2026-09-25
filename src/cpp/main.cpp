// main.cpp — Vesna 1.0.0 C++ 命令行入口
#include "vesna.hpp"
#include "platform.h"

#include <iostream>
#include <string>
#include <vector>

namespace vesna {

// Vesna 内置安装程序（C++ 版）
static const char* INSTALL_SCRIPT = R"VES(
/* Vesna 内置安装程序 */

src = #cwd(),
args = #args(),

target = "C:\\Vesna",
if #len(args) >= '1'-
-target = #path_clean(args['1']),

print("=== Vesna 安装程序 ==="),
print("源目录: " + src),
print("目标目录: " + target),
print(""),

print("[1/6] 检查源文件..."),
prefix = #sub(target; '1'; '3'),
if not #fexists(prefix)-
-print("错误: 盘符不存在 " + prefix),
-#exit('1'),
if not #fexists(src + "\\bin\\vesna.exe")-
-print("  错误: 找不到 bin\\vesna.exe"),
-#exit('1'),
if not #fexists(src + "\\lib\\csv.ves")-
-print("  错误: 找不到 lib\\csv.ves"),
-#exit('1'),
print("  OK"),

print("[2/6] 创建目录..."),
#mkdir(target),
#mkdir(target + "\\bin"),
#mkdir(target + "\\lib"),
#mkdir(target + "\\examples"),
#mkdir(target + "\\docs"),
print("  OK"),

print("[3/6] 复制文件..."),
#copy(src + "\\bin\\vesna.exe"; target + "\\bin\\vesna.exe"),
if #fexists(src + "\\vesna.ico")-
-#copy(src + "\\vesna.ico"; target + "\\vesna.ico"),
print("  vesna.exe"),
files = #ls(src + "\\lib"),
for f in files-
-#copy(src + "\\lib\\" + f; target + "\\lib\\" + f),
print("  lib\\ (" + #str(#len(files)) + " 个文件)"),
files = #ls(src + "\\docs"),
for f in files-
-#copy(src + "\\docs\\" + f; target + "\\docs\\" + f),
print("  docs\\ (" + #str(#len(files)) + " 个文件)"),
files = #ls(src + "\\examples"),
for f in files-
-#copy(src + "\\examples\\" + f; target + "\\examples\\" + f),
print("  examples\\ (" + #str(#len(files)) + " 个文件)"),
if #fexists(src + "\\README.md")-
-#copy(src + "\\README.md"; target + "\\README.md"),
if #fexists(src + "\\LICENSE")-
-#copy(src + "\\LICENSE"; target + "\\LICENSE"),

print("[4/6] 设置环境变量..."),
#setenv("VESNA_HOME"; target),
print("  VESNA_HOME = " + target),
old_path = #regenv("PATH"),
bin_path = target + "\\bin",
if #find(old_path; bin_path) == '0'-
-#setenv("PATH"; old_path + ";" + bin_path),
-print("  PATH 已追加 " + bin_path),
else-
-print("  PATH 已包含 " + bin_path),
print("[5/6] 注册文件关联..."),
#regwrite("HKCU"; "Software\\Classes\\.ves"; ""; "VesnaScript"),
#regwrite("HKCU"; "Software\\Classes\\.ves\\ShellNew"; "NullFile"; ""),
#regwrite("HKCU"; "Software\\Classes\\VesnaScript"; ""; "Vesna 脚本"),
#regwrite("HKCU"; "Software\\Classes\\VesnaScript\\DefaultIcon"; ""; target + "\\vesna.ico"),
#regwrite("HKCU"; "Software\\Classes\\VesnaScript\\OpenWithProgids"; ""; ""),
#regwrite("HKCU"; "Software\\Classes\\VesnaScript\\shell\\run"; ""; "用 Vesna 运行"),
#regwrite("HKCU"; "Software\\Classes\\VesnaScript\\shell\\run\\command"; ""; "\"" + target + "\\bin\\vesna.exe\" \"%1\""),
print("  OK"),
print("[6/6] 安装完成"),
print(""),
print("请重开 cmd 后输入 vesna 测试。"),
)VES";

// Vesna 内置卸载程序（C++ 版）：删注册表关联 + 清环境变量 + 删安装目录
static const char* UNINSTALL_SCRIPT = R"VES(
/* Vesna 内置卸载程序 */

target = #getenv("VESNA_HOME"),
if target == ""-
-target = "C:\\Vesna",
args = #args(),
if #len(args) >= '1'-
-target = #path_clean(args['1']),

print("=== Vesna 卸载程序 ==="),
print("目标目录: " + target),
print(""),

print("[1/4] 删除文件关联..."),
#regdelete("HKCU"; "Software\\Classes\\VesnaScript"),
#regdelete("HKCU"; "Software\\Classes\\.ves"),
print("  OK"),

print("[2/4] 清理环境变量..."),
#shell("reg delete \"HKCU\\Environment\" /v VESNA_HOME /f"),
bin_path = target + "\\bin",
old_path = #regenv("PATH"),
new_path = #replace(old_path; ";" + bin_path; ""),
new_path = #replace(new_path; bin_path + ";"; ""),
new_path = #replace(new_path; bin_path; ""),
if new_path != old_path-
-#setenv("PATH"; new_path),
-print("  PATH 已移除 " + bin_path),
else-
-print("  PATH 未包含 " + bin_path),
print("  OK"),

print("[3/4] 删除安装目录..."),
if #fexists(target)-
-#rmdir(target),
-print("  已删除 " + target),
else-
-print("  目录不存在，跳过"),
print("  OK"),

print("[4/4] 卸载完成"),
print(""),
print("请重开 cmd 后生效。"),
)VES";

int mainCli(int argc, char** argv) {
    // 控制台 UTF-8，保证中文输出正确
    setConsoleUtf8();

    // 关闭 iostream 与 C stdio 的同步（提升输出吞吐）
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    if (argc < 2) { repl(); return 0; }

    std::string arg = argv[1];

    if (arg == "--install") {
        std::vector<std::string> args;
        for (int i = 2; i < argc; ++i) args.emplace_back(argv[i]);
        try {
            int code = runSource(INSTALL_SCRIPT, args, ".",
                                 "<install>", true);
            return code ? code : 0;
        } catch (VesnaError& e) {
            std::cerr << "安装失败: " << e.str() << std::endl;
            return 1;
        }
    }

    if (arg == "--uninstall") {
        std::vector<std::string> args;
        for (int i = 2; i < argc; ++i) args.emplace_back(argv[i]);
        try {
            int code = runSource(UNINSTALL_SCRIPT, args, ".",
                                 "<uninstall>", true);
            return code ? code : 0;
        } catch (VesnaError& e) {
            std::cerr << "卸载失败: " << e.str() << std::endl;
            return 1;
        }
    }

    if (arg == "--debug") {
        if (argc < 3) {
            std::cerr << "用法: vesna --debug <脚本.ves> [参数...]" << std::endl;
            return 1;
        }
        std::string path = argv[2];
        if (!fileExists(path)) {
            std::cerr << "找不到文件: " << path << std::endl;
            return 1;
        }
        std::vector<std::string> args;
        for (int i = 3; i < argc; ++i) args.emplace_back(argv[i]);
        std::cout << "Vesna 调试器 " << VERSION << " —— 输入 help 查看命令" << std::endl;
        try {
            return runFileDbg(path, args);
        } catch (VesnaError& e) {
            std::cerr << "错误: " << e.str() << std::endl;
            return 1;
        }
    }

    if (arg == "--pkg") {
        std::vector<std::string> args;
        for (int i = 2; i < argc; ++i) args.emplace_back(argv[i]);
        std::string pkg_script = findVesnaHome() + "\\lib\\pkg.ves";
        if (!fileExists(pkg_script)) {
            std::cerr << "找不到包管理器脚本: " << pkg_script << std::endl;
            return 1;
        }
        try {
            std::string src = readFileUtf8(pkg_script);
            return runSource(src, args, ".", pkg_script, true);
        } catch (VesnaError& e) {
            std::cerr << "包管理器错误: " << e.str() << std::endl;
            return 1;
        }
    }

    if (arg == "--version") {
        std::cout << "Vesna " << VERSION << std::endl;
        return 0;
    }

    if (arg == "--help") {
        std::cout << "用法:\n"
                  << "  vesna <脚本.ves> [参数...]   运行脚本\n"
                  << "  vesna                        进入 REPL\n"
                  << "  vesna --install              安装 Vesna\n"
                  << "  vesna --uninstall            卸载 Vesna（删目录+环境变量+注册表）\n"
                  << "  vesna --debug <脚本>         调试运行脚本（断点/单步/变量）\n"
                  << "  vesna --pkg <命令>           包管理器（init/install/remove/list/search）\n"
                  << "  vesna --version              显示版本\n"
                  << "  vesna --help                 显示帮助\n";
        return 0;
    }

    std::string path = arg;
    if (!fileExists(path)) {
        std::cerr << "找不到文件: " << path << std::endl;
        return 1;
    }
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) args.emplace_back(argv[i]);
    try {
        runFile(path, args);
    } catch (VesnaError& e) {
        std::cerr << "错误: " << e.str() << std::endl;
        return 1;
    }
    return 0;
}

}  // namespace vesna

int main(int argc, char** argv) {
    return vesna::mainCli(argc, argv);
}
