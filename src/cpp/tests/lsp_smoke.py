# -*- coding: utf-8 -*-
"""LSP 冒烟测试：驱动 server.py 子进程验证各能力"""
import subprocess
import json
import sys
import os

SERVER = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'vesna-vscode', 'server', 'server.py')
PY = sys.executable


def msg(obj):
    body = json.dumps(obj, ensure_ascii=False).encode('utf-8')
    return b'Content-Length: %d\r\n\r\n' % len(body) + body


def run():
    p = subprocess.Popen([PY, SERVER], stdin=subprocess.PIPE, stdout=subprocess.PIPE, cwd=os.path.dirname(SERVER))
    out = []

    def send(obj):
        p.stdin.write(msg(obj))
        p.stdin.flush()

    def recv():
        headers = {}
        while True:
            line = p.stdout.readline()
            if line in (b'\r\n', b'\n'):
                break
            k, _, v = line.decode().partition(':')
            headers[k.strip()] = v.strip()
        n = int(headers['Content-Length'])
        return json.loads(p.stdout.read(n))

    send({"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {"capabilities": {}}})
    r = recv()
    caps = r['result']['capabilities']
    assert caps.get('documentSymbolProvider') is True, 'no documentSymbolProvider'
    assert caps.get('foldingRangeProvider') is True, 'no foldingRangeProvider'
    assert r['result']['serverInfo']['version'] == '0.4.0', 'serverInfo version'
    out.append('initialize: serverInfo 0.4.0 + symbols/folding ok')

    send({"jsonrpc": "2.0", "method": "initialized", "params": {}})

    # 文档含：一个 def、一个错误（缺右括号）、若干 1.2 内置
    text = (
        "/* demo */\n"
        "def add(a; b)-\n"
        "-x = a + b,\n"
        "-back(x),\n"
        "print(#sha256(\"abc\")),\n"
        "d = #json_decode(\"{'a';'1'}\"),\n"
        "x = #ffi_call(\"kernel32.dll\"; \"GetTickCount\"),\n"
        "y = #int(\"abc\"\n"
    )
    uri = "file:///F:/Vesna/src/cpp/tests/demo.ves"
    send({"jsonrpc": "2.0", "method": "textDocument/didOpen",
          "params": {"textDocument": {"uri": uri, "text": text}}})
    r = recv()
    assert r['method'] == 'textDocument/publishDiagnostics', 'no diagnostics'
    diags = r['params']['diagnostics']
    msgs = [d['message'] for d in diags]
    assert len(msgs) > 0, 'expected diagnostics'
    out.append('didOpen diagnostics: %d 项（含 %r 等）' % (len(msgs), msgs[0][:24]))

    send({"jsonrpc": "2.0", "id": 2, "method": "textDocument/completion",
          "params": {"textDocument": {"uri": uri}, "position": {"line": 2, "character": 0}}})
    r = recv()
    labels = [c['label'] for c in r['result']]
    assert '#ffi_call' in labels, 'completion missing #ffi_call'
    assert '#sha256' in labels, 'completion missing #sha256'
    assert '#thread' in labels, 'completion missing #thread'
    assert 'def' in labels, 'completion missing keyword'
    out.append('completion: %d 项，含 1.2 内置' % len(labels))

    send({"jsonrpc": "2.0", "id": 3, "method": "textDocument/hover",
          "params": {"textDocument": {"uri": uri}, "position": {"line": 4, "character": 13}}})
    r = recv()
    md = r['result']['contents']['value']
    assert 'sha256' in md and 'FIPS' in md, 'hover bad: ' + md[:40]
    out.append('hover(#sha256): FIPS 文档返回')

    send({"jsonrpc": "2.0", "id": 4, "method": "textDocument/hover",
          "params": {"textDocument": {"uri": uri}, "position": {"line": 1, "character": 3}}})
    r = recv()
    md = r['result']['contents']['value']
    assert 'def' in md, 'hover keyword missing'
    out.append('hover(def): 关键字文档返回')

    send({"jsonrpc": "2.0", "id": 5, "method": "textDocument/documentSymbol",
          "params": {"textDocument": {"uri": uri}}})
    r = recv()
    names = [s['name'] for s in r['result']]
    assert 'add' in names, 'symbol missing add'
    out.append('documentSymbol: %s' % names)

    send({"jsonrpc": "2.0", "id": 6, "method": "textDocument/foldingRange",
          "params": {"textDocument": {"uri": uri}}})
    r = recv()
    fr = r['result']
    assert len(fr) >= 1, 'no folding ranges'
    out.append('foldingRange: %d 段' % len(fr))

    send({"jsonrpc": "2.0", "id": 7, "method": "shutdown", "params": {}})
    recv()
    send({"jsonrpc": "2.0", "method": "exit", "params": {}})
    p.wait(timeout=10)
    out.append('shutdown/exit ok')
    return out


if __name__ == '__main__':
    for line in run():
        print('PASS', line)
    print('ALL LSP TESTS PASSED')
