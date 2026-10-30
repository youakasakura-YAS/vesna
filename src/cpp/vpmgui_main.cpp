// vpmgui_main.cpp — vpm-gui 独立可执行入口（嵌入 json.ves + gui.ves + vpm-gui.ves）
#include "vesna.hpp"
#include "platform.h"

#include <iostream>
#include <string>
#include <vector>

namespace vesna {

static const char* VPMGUI_SCRIPT = R"VES(__VESNA_APP_SCRIPT__)VES";

int mainCli(int argc, char** argv) {
    setConsoleUtf8();
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    if (argc >= 2 && std::string(argv[1]) == "--version") {
        std::cout << "vpm-gui 2.0.0 (Vesna " << VERSION << ")" << std::endl;
        return 0;
    }
    if (argc >= 2 && std::string(argv[1]) == "--help") {
        std::cout << "vpm-gui — Vesna 包管理器图形化界面 (纯 Vesna)\n"
                  << "用法: vpm-gui.exe [端口]   默认 18082\n"
                  << "      vpm-gui.exe --version\n";
        return 0;
    }

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
    try {
        return runSource(VPMGUI_SCRIPT, args, ".", "<vpm-gui>", true);
    } catch (VesnaError& e) {
        std::cerr << "vpm-gui 错误: " << e.str() << std::endl;
        return 1;
    }
}

}  // namespace vesna

int main(int argc, char** argv) {
    return vesna::mainCli(argc, argv);
}
