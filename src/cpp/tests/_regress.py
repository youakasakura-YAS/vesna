# -*- coding: utf-8 -*-
import subprocess, io, os, sys

EXE = r'F:\Vesna\src\cpp\vesna_test.exe'
TESTS_DIR = r'F:\Vesna\src\cpp\tests'
GOLD = os.path.join(TESTS_DIR, 'golden')

def run_case(name):
    src = os.path.join(TESTS_DIR, name + '.ves')
    gold = os.path.join(GOLD, name + '.out')
    if not os.path.exists(src) or not os.path.exists(gold):
        return ('SKIP', 'missing src/gold')
    p = subprocess.run([EXE, src], cwd=TESTS_DIR, capture_output=True, timeout=120)
    out = p.stdout.decode('utf-8', errors='replace').replace('\r\n', '\n').replace('\r', '\n')
    g = io.open(gold, encoding='utf-8', errors='replace').read().replace('\r\n', '\n').replace('\r', '\n')
    if out == g:
        return ('MATCH', '')
    # 只显示首处差异
    ol, gl = out.split('\n'), g.split('\n')
    for i in range(max(len(ol), len(gl))):
        a = ol[i] if i < len(ol) else '<EOF>'
        b = gl[i] if i < len(gl) else '<EOF>'
        if a != b:
            return ('DIFF', 'line %d:\n  got: %r\n  exp: %r' % (i + 1, a[:120], b[:120]))
    return ('MATCH', '')

for name in ['regression', 'binary', 'tier4', 'smoke04']:
    st, detail = run_case(name)
    print(name, st)
    if detail:
        print(detail)
