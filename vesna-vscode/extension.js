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
    };
    client = new LanguageClient('vesna', 'Vesna LSP', serverOptions, clientOptions);
    client.start();
}

function deactivate() {
    if (!client) return undefined;
    return client.stop();
}

module.exports = { activate, deactivate };
