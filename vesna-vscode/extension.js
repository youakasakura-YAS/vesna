const path = require('path');
const vscode = require('vscode');
const { LanguageClient, TransportKind } = require('vscode-languageclient/node');

let client;

function activate(context) {
    const serverModule = context.asAbsolutePath(path.join('server', 'server.py'));
    const serverOptions = {
        run: { command: 'python', args: [serverModule], transport: TransportKind.stdio },
        debug: { command: 'python', args: [serverModule], transport: TransportKind.stdio },
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