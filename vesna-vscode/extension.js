const path = require('path');
const vscode = require('vscode');
const { LanguageClient, TransportKind } = require('vscode-languageclient/node');

let client;

function findVesnaExe() {
    // 1) 显式配置
    const cfg = vscode.workspace.getConfiguration('vesna');
    const exe = cfg.get('executablePath');
    if (exe && exe.length > 0) return exe;
    // 2) PATH 中的 vesna
    return 'vesna';
}

function activate(context) {
    const serverOptions = {
        run: { command: findVesnaExe(), args: ['--lsp'], transport: TransportKind.stdio },
        debug: { command: findVesnaExe(), args: ['--lsp'], transport: TransportKind.stdio },
    };
    const clientOptions = {
        documentSelector: [{ scheme: 'file', language: 'vesna' }],
        // 1.8.0: 启动失败时给出可操作提示（未安装/未配置 vesna.exe 时避免静默失败）
        errorHandler: {
            error(error, message, count) {
                if (count > 3) {
                    vscode.window.showErrorMessage(
                        'Vesna LSP 启动失败：请确认已安装 vesna（或在工作区设置 vesna.executablePath 指向 vesna.exe）。'
                    );
                    return { action: vscode.ErrorAction.Shutdown };
                }
                return { action: vscode.ErrorAction.Continue };
            },
            closed() {
                return { action: vscode.ErrorAction.Restart };
            },
        },
    };
    client = new LanguageClient('vesna', 'Vesna LSP', serverOptions, clientOptions);
    client.start();
}

function deactivate() {
    if (!client) return undefined;
    return client.stop();
}

module.exports = { activate, deactivate };
