# -*- coding: utf-8 -*-
# LSP 2.5 冒烟：textDocument/formatting + textDocument/references
import subprocess, json, sys

EXE = r'F:\Vesna\src\cpp\vesna_test.exe'
p = subprocess.Popen([EXE, '--lsp'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

def send(obj):
    body = json.dumps(obj, ensure_ascii=False)
    p.stdin.write(('Content-Length: %d\r\n\r\n%s' % (len(body.encode('utf-8')), body)).encode('utf-8'))
    p.stdin.flush()

def recv():
    headers = {}
    while True:
        line = p.stdout.readline()
        if not line: return None
        line = line.decode('utf-8', 'replace').rstrip('\r\n')
        if line == '': break
        k, _, v = line.partition(':')
        headers[k.strip().lower()] = v.strip()
    n = int(headers.get('content-length', 0))
    if n <= 0: return None
    body = p.stdout.read(n).decode('utf-8', 'replace')
    return json.loads(body)

results = []
def check(name, cond):
    results.append((name, cond))
    print(('PASS ' if cond else 'FAIL ') + name)

# 1) initialize
send({"jsonrpc":"2.0","id":1,"method":"initialize","params":{}})
r = recv()
cap = r.get('result', {}).get('capabilities', {})
check('capabilities formatting', cap.get('documentFormattingProvider') is True)
check('capabilities references', cap.get('referencesProvider') is True)
check('serverInfo 2.5.0', r.get('result', {}).get('serverInfo', {}).get('version') == '2.5.0')

# 2) didOpen（含行首尾空白的文档）
doc = "/* test */\ndef  f ( a )-\n  -return a ,  \n\nx = f ( '1' ),\nprint ( x ) ,\n"
uri = "file:///t.ves"
send({"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":uri,"languageId":"vesna","version":1,"text":doc}}})
recv()  # diagnostics notification

# 3) formatting
send({"jsonrpc":"2.0","id":2,"method":"textDocument/formatting","params":{"textDocument":{"uri":uri},"options":{"tabSize":4,"insertSpaces":True}}})
r = recv()
edits = r.get('result', [])
check('formatting returns edit', isinstance(edits, list) and len(edits) == 1)
if isinstance(edits, list) and edits:
    new_text = edits[0].get('newText', '')
    check('formatting trims line-edge spaces', '  -return' not in new_text and '-return a ,  ' not in new_text)
    check('formatting collapses blank lines', '\n\n\n' not in new_text)
    check('formatting keeps tokens', 'f ( a )' in new_text)
    check('formatting keeps def indent', 'def  f ( a )-' in new_text)

# 4) references（文档含 3 处 x）
doc2 = "x = '1',\ny = x + '2',\nprint(x),\n"
uri2 = "file:///r.ves"
send({"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":uri2,"languageId":"vesna","version":1,"text":doc2}}})
recv()
send({"jsonrpc":"2.0","id":3,"method":"textDocument/references","params":{"textDocument":{"uri":uri2},"position":{"line":1,"character":4},"context":{"includeDeclaration":True}}})
r = recv()
refs = r.get('result', [])
check('references count 3', isinstance(refs, list) and len(refs) == 3)
if isinstance(refs, list):
    lines = sorted(x.get('range', {}).get('start', {}).get('line', -1) for x in refs)
    check('references lines 0,1,2', lines == [0, 1, 2])

# 5) shutdown + exit
send({"jsonrpc":"2.0","id":99,"method":"shutdown","params":{}})
recv()
send({"jsonrpc":"2.0","method":"exit","params":{}})
p.stdin.close()
p.wait(timeout=5)

fails = [n for n, c in results if not c]
print('---')
print('ok=%d fail=%d' % (len(results) - len(fails), len(fails)))
sys.exit(1 if fails else 0)
