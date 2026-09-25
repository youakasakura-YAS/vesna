# lsp_cpp_smoke.py — 驱动 vesna.exe --lsp 的原生 LSP 冒烟测试（文件重定向模式，CI 可复现）
# 用法: python lsp_cpp_smoke.py [vesna.exe 路径]
import json
import subprocess
import sys
import os
import tempfile

EXE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "bin", "vesna.exe")
EXE = os.path.abspath(EXE)

def frame(msg):
    body = json.dumps(msg, ensure_ascii=False).encode("utf-8")
    return b"Content-Length: %d\r\n\r\n" % len(body) + body

def read_msg(buf, pos):
    end = buf.find(b"\r\n\r\n", pos)
    assert end > 0, "no header at %d" % pos
    n = int(buf[pos:end].split(b":", 1)[1].strip())
    body_start = end + 4
    body = buf[body_start:body_start + n]
    return json.loads(body.decode("utf-8")), body_start + n

tmp = tempfile.mkdtemp(prefix="vesna_lsp_")
inp = tmp + "\\in.bin"
outp = tmp + "\\out.txt"
errp = tmp + "\\err.txt"

reqs = [
    {"jsonrpc": "2.0", "id": 1, "method": "initialize",
     "params": {"capabilities": {}, "processId": None}},
    {"jsonrpc": "2.0", "method": "initialized", "params": {}},
]
good = "def add(a; b)-\n-\tback(a + b)\n-\n"
bad = "def add(a; b)-\n-\tback(a + b\n-\n"
uri = "file:///C:/t/good.ves"
uri2 = "file:///C:/t/bad.ves"
reqs.append({"jsonrpc": "2.0", "method": "textDocument/didOpen",
             "params": {"textDocument": {"uri": uri, "languageId": "vesna", "version": 1, "text": good}}})
reqs.append({"jsonrpc": "2.0", "method": "textDocument/didOpen",
             "params": {"textDocument": {"uri": uri2, "languageId": "vesna", "version": 1, "text": bad}}})
src = good + "x = #json_en\n"
reqs.append({"jsonrpc": "2.0", "id": 2, "method": "textDocument/completion",
             "params": {"textDocument": {"uri": uri}, "position": {"line": 3, "character": 11}}})
reqs.append({"jsonrpc": "2.0", "id": 3, "method": "textDocument/hover",
             "params": {"textDocument": {"uri": uri}, "position": {"line": 0, "character": 7}}})
reqs.append({"jsonrpc": "2.0", "id": 4, "method": "textDocument/documentSymbol",
             "params": {"textDocument": {"uri": uri}}})
reqs.append({"jsonrpc": "2.0", "id": 5, "method": "textDocument/foldingRange",
             "params": {"textDocument": {"uri": uri}}})
reqs.append({"jsonrpc": "2.0", "id": 6, "method": "shutdown", "params": None})
reqs.append({"jsonrpc": "2.0", "method": "exit", "params": None})

with open(inp, "wb") as f:
    for r in reqs:
        f.write(frame(r))

with open(inp, "rb") as fin, open(outp, "wb") as fout, open(errp, "wb") as ferr:
    proc = subprocess.run([EXE, "--lsp"], stdin=fin, stdout=fout, stderr=ferr, timeout=30)
assert proc.returncode == 0, "lsp exit %d stderr: %s" % (proc.returncode, open(errp, encoding="utf-8", errors="replace").read())

buf = open(outp, "rb").read()
pos = 0
msgs = []
while pos < len(buf):
    msg, pos = read_msg(buf, pos)
    msgs.append(msg)

by_id = {m.get("id"): m for m in msgs if "id" in m}
notifs = [m for m in msgs if "id" not in m and "method" in m]

# 1. initialize capabilities
caps = by_id[1]["result"]["capabilities"]
assert "completionProvider" in caps and caps["hoverProvider"] is True
print("1. initialize capabilities OK")

# 2. didOpen clean diagnostics
d1 = notifs[0]
assert d1["method"] == "textDocument/publishDiagnostics" and d1["params"]["diagnostics"] == []
print("2. didOpen clean diagnostics OK")

# 3. didOpen error diagnostics
d2 = notifs[1]
assert d2["params"]["diagnostics"], "bad doc should have diagnostics"
print("3. didOpen error diagnostics OK (%d diags)" % len(d2["params"]["diagnostics"]))

# 4. completion
labels = [i["label"] for i in by_id[2]["result"]]
for need in ["#json_encode", "#sha256", "#thread", "#ffi_call"]:
    assert need in labels, "completion missing %s" % need
assert any(i["label"] == "add" for i in by_id[2]["result"])
print("4. completion OK (%d items)" % len(labels))

# 5. hover def
val = by_id[3]["result"]["contents"]["value"]
assert "def add" in val, "hover: %r" % val
print("5. hover def OK")

# 6. documentSymbol
names = [s["name"] for s in by_id[4]["result"]]
assert "add" in names
print("6. documentSymbol OK")

# 7. foldingRange
fold = by_id[5]["result"]
assert isinstance(fold, list) and len(fold) >= 1
print("7. foldingRange OK (%d ranges)" % len(fold))

# 8. shutdown
assert by_id[6]["result"] is None
print("8. shutdown OK")

print("ALL C++ LSP TESTS PASSED")
