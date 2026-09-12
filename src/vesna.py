#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Vesna 0.3.0 — Scripts of spring"""

import sys
import os
import re

VERSION = "0.2.0"
INSTALL_SCRIPT = r'''
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
if not #fexists(src + "\\vesna.exe")-
-print("  错误: 找不到 vesna.exe"),
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
#copy(src + "\\vesna.exe"; target + "\\bin\\vesna.exe"),
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
old_path = #getenv("PATH"),
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
#regwrite("HKCU"; "Software\\Classes\\VesnaScript\\shell\\run"; ""; "用 Vesna 运行"),
#regwrite("HKCU"; "Software\\Classes\\VesnaScript\\shell\\run\\command"; ""; "\"" + target + "\\bin\\vesna.exe\" \"%1\""),
print("  OK"),
print("[6/6] 安装完成"),
print(""),
print("请重开 cmd 后输入 vesna 测试。"),
'''
def _find_vesna_home():
    env = os.environ.get("VESNA_HOME")
    if env:
        return env
    # PyInstaller 打包后，sys.frozen 为 True
    if getattr(sys, "frozen", False):
        exe_dir = os.path.dirname(os.path.abspath(sys.executable))
        # exe 在 bin\，lib 在上一级
        return os.path.dirname(exe_dir)
    # 源码运行
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.dirname(here)

VESNA_HOME = _find_vesna_home()
COMPOUND_OP = {'+': 'PLUS', '-': 'MINUS', '*': 'STAR', '/': 'SLASH'}


# ============================================================
# 错误
# ============================================================

class VesnaError(Exception):
    def __init__(self, msg, line=None):
        self.msg = msg
        self.line = line
        super().__init__(str(self))

    def __str__(self):
        if self.line is None:
            return self.msg
        return f"[第 {self.line} 行] {self.msg}"


class BreakSignal(Exception):
    pass


class ContinueSignal(Exception):
    pass


class ReturnSignal(Exception):
    def __init__(self, value):
        self.value = value


class ExitSignal(Exception):
    def __init__(self, code=0):
        self.code = code


# ============================================================
# 预处理
# ============================================================

def extract_vesna(src):
    i = src.find('<vesna>')
    if i == -1:
        return src
    j = src.find('</vesna>', i)
    if j == -1:
        return src[i + 7:]
    return src[i + 7:j]


def remove_comments(src):
    out = []
    i = 0
    n = len(src)
    while i < n:
        if src[i:i + 2] == '/*':
            j = src.find('*/', i + 2)
            if j == -1:
                out.extend(c for c in src[i:] if c == '\n')
                break
            out.extend(c for c in src[i:j + 2] if c == '\n')
            i = j + 2
        else:
            out.append(src[i])
            i += 1
    return ''.join(out)


class Line:
    __slots__ = ('indent', 'content', 'block_start', 'line_no')

    def __init__(self, indent, content, block_start, line_no):
        self.indent = indent
        self.content = content
        self.block_start = block_start
        self.line_no = line_no


def _split_logical_lines(src):
    """把源码切成逻辑行：括号或引号未闭合时，多行合并。
    返回 [(text, start_phys_line), ...]
    """
    phys_lines = src.split('\n')
    result = []
    buf = []
    buf_start = 1
    depth_paren = 0
    depth_brack = 0
    depth_brace = 0
    in_dq = False
    in_sq = False
    escape = False

    for phys_no, line in enumerate(phys_lines, 1):
        if not buf:
            buf_start = phys_no
        for c in line:
            if escape:
                escape = False
                buf.append(c)
                continue
            if in_dq:
                if c == '\\':
                    escape = True
                elif c == '"':
                    in_dq = False
                buf.append(c)
                continue
            if in_sq:
                if c == "'":
                    in_sq = False
                buf.append(c)
                continue
            if c == '"':
                in_dq = True
                buf.append(c)
            elif c == "'":
                in_sq = True
                buf.append(c)
            elif c == '(':
                depth_paren += 1
                buf.append(c)
            elif c == ')':
                depth_paren -= 1
                buf.append(c)
            elif c == '[':
                depth_brack += 1
                buf.append(c)
            elif c == ']':
                depth_brack -= 1
                buf.append(c)
            elif c == '{':
                depth_brace += 1
                buf.append(c)
            elif c == '}':
                depth_brace -= 1
                buf.append(c)
            else:
                buf.append(c)

        if depth_paren > 0 or depth_brack > 0 or depth_brace > 0 or in_dq or in_sq:
            buf.append(' ')
        else:
            result.append((''.join(buf), buf_start))
            buf = []

    if buf:
        result.append((''.join(buf), buf_start))
    return result


def _split_statements(text):
    result = []
    buf = []
    in_dq = False
    in_sq = False
    escape = False
    depth = 0
    for c in text:
        if escape:
            escape = False
            buf.append(c)
            continue
        if in_dq:
            if c == '\\':
                escape = True
            elif c == '"':
                in_dq = False
            buf.append(c)
            continue
        if in_sq:
            if c == "'":
                in_sq = False
            buf.append(c)
            continue
        if c == '"':
            in_dq = True
            buf.append(c)
            continue
        if c == "'":
            in_sq = True
            buf.append(c)
            continue
        if c in '([{':
            depth += 1
            buf.append(c)
            continue
        if c in ')]}':
            depth -= 1
            buf.append(c)
            continue
        if c == ',' and depth == 0:
            result.append(''.join(buf))
            buf = []
            continue
        buf.append(c)
    if buf:
        result.append(''.join(buf))
    return result


def preprocess(src):
    src = remove_comments(extract_vesna(src))
    lines = []
    for text, phys_no in _split_logical_lines(src):
        for stmt in _split_statements(text):
            s = stmt.strip()
            if not s:
                continue
            indent = 0
            while s.startswith('-'):
                indent += 1
                s = s[1:]
            s = s.lstrip()
            block_start = False
            if s.endswith('-'):
                block_start = True
                s = s[:-1].rstrip()
            if s:
                lines.append(Line(indent, s, block_start, phys_no))
    return lines


# ============================================================
# 词法
# ============================================================

KEYWORDS = {
    'if', 'elif', 'else', 'while', 'for', 'in',
    'def', 'back', 'break', 'continue',
    'try', 'catch', 'import', 'and', 'or', 'not',
}

BUILTINS = {
    'up', 'down', 'into', 'f',
    'len', 'sub', 'split', 'join', 'find', 'replace',
    'append', 'pop', 'keys', 'values', 'type',
    'args', 'fread', 'fwrite', 'fexists', 'exit',
    'trim', 'startswith', 'endswith', 'lines', 'repeat',
    'sort', 'reverse', 'map', 'filter', 'reduce', 'slice',
    'match', 'findall', 'gsub', 'search',
    'ls', 'glob', 'fappend', 'stdin',
    'has_key',
    'str', 'int', 'float', 'bool',
    'char_at', 'ord', 'chr',
    'is_digit', 'is_alpha', 'is_alnum', 'is_space',
    'lstrip', 'rstrip', 'title', 'capitalize',
    'count', 'rfind',
    'min', 'max', 'sum', 'abs', 'round', 'pow',
    'contains','contains',
    'mkdir', 'copy', 'rmdir', 'rename',
    'getenv', 'setenv', 'cwd', 'chdir',
    'regwrite', 'regdelete', 'shell', 'path_clean',
}

TYPE_KEYWORDS = {'int', 'str', 'float', 'list', 'dict', 'bool'}


class Token:
    __slots__ = ('kind', 'value', 'line')

    def __init__(self, kind, value, line):
        self.kind = kind
        self.value = value
        self.line = line

    def __repr__(self):
        return f"Token({self.kind}, {self.value!r})"


def unescape(s):
    out = []
    i = 0
    while i < len(s):
        if s[i] == '\\' and i + 1 < len(s):
            c = s[i + 1]
            m = {'n': '\n', 't': '\t', 'r': '\r', '\\': '\\', '"': '"', "'": "'", '0': '\0'}
            out.append(m.get(c, '\\' + c))
            i += 2
        else:
            out.append(s[i])
            i += 1
    return ''.join(out)


def lex_expr(s, ln):
    toks = []
    i = 0
    n = len(s)
    while i < n:
        c = s[i]
        if c.isspace():
            i += 1
            continue

        # -f"..." 插值字符串
        if c == '#' and i + 2 < n and s[i + 1] == 'f' and s[i + 2] == '"':
            j = i + 3
            buf = []
            while j < n and s[j] != '"':
                if s[j] == '\\' and j + 1 < n:
                    buf.append(s[j]); buf.append(s[j + 1]); j += 2
                else:
                    buf.append(s[j]); j += 1
            if j >= n:
                raise VesnaError("插值字符串未闭合", ln)
            toks.append(Token('INTERP', unescape(''.join(buf)), ln))
            i = j + 1
            continue

        # -xxx 内置函数
        if c == '#' and i + 1 < n and (s[i + 1].isalpha() or s[i + 1] == '_'):
            j = i + 1
            while j < n and (s[j].isalnum() or s[j] == '_'):
                j += 1
            w = s[i + 1:j]
            if w in BUILTINS:
                toks.append(Token('BUILTIN', w, ln))
                i = j
                continue

        # 字符串
        if c == '"':
            j = i + 1
            buf = []
            while j < n and s[j] != '"':
                if s[j] == '\\' and j + 1 < n:
                    buf.append(s[j]); buf.append(s[j + 1]); j += 2
                else:
                    buf.append(s[j]); j += 1
            if j >= n:
                raise VesnaError("字符串未闭合", ln)
            toks.append(Token('STRING', unescape(''.join(buf)), ln))
            i = j + 1
            continue

        # 数字 '...'
        if c == "'":
            j = s.find("'", i + 1)
            if j == -1:
                raise VesnaError("数字未闭合", ln)
            body = s[i + 1:j]
            if not re.fullmatch(r'-?\d+(\.\d+)?|-?\.\d+', body):
                raise VesnaError(f"无效数字: '{body}'", ln)
            toks.append(Token('NUMBER', body, ln))
            i = j + 1
            continue

        # 标识符 / 关键字
        if c.isalpha() or c == '_':
            j = i
            while j < n and (s[j].isalnum() or s[j] == '_'):
                j += 1
            w = s[i:j]
            if w == 'true':
                toks.append(Token('BOOL', True, ln))
            elif w == 'false':
                toks.append(Token('BOOL', False, ln))
            elif w == 'none':                                  
                toks.append(Token('NONE', None, ln))           
            elif w in KEYWORDS:
                toks.append(Token(w.upper(), w, ln))
            elif w in TYPE_KEYWORDS:
                toks.append(Token('TYPE', w, ln))
            else:
                toks.append(Token('IDENT', w, ln))
            i = j
            continue

        # 双字符
        two = s[i:i + 2]
        if two == '+=':
            toks.append(Token('PLUSEQ', '+', ln)); i += 2; continue
        if two == '-=':
            toks.append(Token('MINUSEQ', '-', ln)); i += 2; continue
        if two == '*=':
            toks.append(Token('STAREQ', '*', ln)); i += 2; continue
        if two == '/=':
            toks.append(Token('SLASHEQ', '/', ln)); i += 2; continue
        if two in ('./', '/.', '/-', '==', '!=', '>=', '<='):
            toks.append(Token(two, two, ln)); i += 2; continue

        # 单字符
        m = {
            '+': 'PLUS', '-': 'MINUS', '*': 'STAR', '/': 'SLASH',
            '=': 'ASSIGN', '<': 'LT', '>': 'GT',
            '(': 'LPAREN', ')': 'RPAREN',
            '[': 'LBRACK', ']': 'RBRACK',
            '{': 'LBRACE', '}': 'RBRACE',
            ',': 'COMMA', ';': 'SEMI', ':': 'COLON',
        }
        if c in m:
            toks.append(Token(m[c], c, ln))
            i += 1
            continue
        raise VesnaError(f"未知字符: {c}", ln)

    toks.append(Token('EOF', None, ln))
    return toks


# ============================================================
# 表达式解析
# ============================================================

class ExprParser:
    def __init__(self, toks):
        self.toks = toks
        self.pos = 0

    def peek(self):
        return self.toks[self.pos]

    def advance(self):
        t = self.toks[self.pos]
        self.pos += 1
        return t

    def expect(self, k):
        t = self.peek()
        if t.kind != k:
            raise VesnaError(f"期望 {k}，实际 {t.kind}", t.line)
        return self.advance()

    def parse(self):
        return self.or_expr()

    def or_expr(self):
        l = self.and_expr()
        while self.peek().kind == 'OR':
            self.advance()
            l = ('or', l, self.and_expr())
        return l

    def and_expr(self):
        l = self.not_expr()
        while self.peek().kind == 'AND':
            self.advance()
            l = ('and', l, self.not_expr())
        return l

    def not_expr(self):
        if self.peek().kind == 'NOT':
            self.advance()
            return ('not', self.not_expr())
        return self.comparison()

    def comparison(self):
        l = self.additive()
        while self.peek().kind in ('==', '!=', 'LT', 'GT', '>=', '<='):
            op = self.advance().kind
            l = ('bin', op, l, self.additive())
        return l

    def additive(self):
        l = self.mul()
        while self.peek().kind in ('PLUS', 'MINUS'):
            op = self.advance().kind
            l = ('bin', op, l, self.mul())
        return l

    def mul(self):
        l = self.unary()
        while self.peek().kind in ('STAR', 'SLASH', './', '/.', '/-'):
            op = self.advance().kind
            l = ('bin', op, l, self.unary())
        return l

    def unary(self):
        if self.peek().kind == 'MINUS':
            self.advance()
            return ('neg', self.unary())
        return self.postfix()

    def postfix(self):
        e = self.primary()
        while self.peek().kind == 'LBRACK':
            self.advance()
            idx = self.parse()
            self.expect('RBRACK')
            e = ('index', e, idx)
        return e

    def primary(self):
        t = self.peek()

        if t.kind == 'NUMBER':
            self.advance()
            v = t.value
            return ('num', float(v) if '.' in v else int(v))
        if t.kind == 'STRING':
            self.advance()
            return ('str', t.value)
        if t.kind == 'BOOL':
            self.advance()
            return ('bool', t.value)
        if t.kind == 'NONE':                        
            self.advance()                          
            return ('none',)                        
        if t.kind == 'INTERP':
            self.advance()
            return ('interp', t.value)

        if t.kind == 'BUILTIN':
            name = t.value
            self.advance()
            self.expect('LPAREN')
            if name == 'into':
                tt = self.peek()
                if tt.kind != 'TYPE':
                    raise VesnaError("-into 第一个参数必须是类型关键字", tt.line)
                self.advance()
                self.expect('SEMI')
                v = self.parse()
                self.expect('RPAREN')
                return ('into', tt.value, v)
            if name == 'args':
                self.expect('RPAREN')
                return ('builtin', name, [])
            args = []
            if self.peek().kind != 'RPAREN':
                args.append(self.parse())
                while self.peek().kind == 'SEMI':
                    self.advance()
                    args.append(self.parse())
            self.expect('RPAREN')
            return ('builtin', name, args)

        if t.kind == 'IDENT':
            name = t.value
            self.advance()
            if self.peek().kind == 'LPAREN':
                self.advance()
                args = []
                if self.peek().kind != 'RPAREN':
                    args.append(self.parse())
                    while self.peek().kind == 'SEMI':
                        self.advance()
                        args.append(self.parse())
                self.expect('RPAREN')
                return ('call', name, args)
            return ('var', name)

        if t.kind == 'LPAREN':
            self.advance()
            first = self.parse()
            if self.peek().kind == 'SEMI':
                elems = [first]
                while self.peek().kind == 'SEMI':
                    self.advance()
                    elems.append(self.parse())
                self.expect('RPAREN')
                return ('group', elems)
            self.expect('RPAREN')
            return first

        if t.kind == 'LBRACK':
            self.advance()
            elems = []
            if self.peek().kind != 'RBRACK':
                elems.append(self.parse())
                while self.peek().kind == 'SEMI':
                    self.advance()
                    elems.append(self.parse())
            self.expect('RBRACK')
            return ('list', elems)

        if t.kind == 'LBRACE':
            self.advance()
            pairs = []
            if self.peek().kind != 'RBRACE':
                k = self.parse()
                self.expect('COLON')
                v = self.parse()
                pairs.append((k, v))
                while self.peek().kind == 'SEMI':
                    self.advance()
                    k = self.parse()
                    self.expect('COLON')
                    v = self.parse()
                    pairs.append((k, v))
            self.expect('RBRACE')
            return ('dict', pairs)

        raise VesnaError(f"表达式不完整（可能是括号未闭合），实际 {t.kind}", t.line)


def parse_expr(s, ln):
    return ExprParser(lex_expr(s, ln)).parse()


# ============================================================
# 语句解析
# ============================================================

class Parser:
    def __init__(self, lines, filename='<stdin>'):
        self.lines = lines
        self.pos = 0
        self.filename = filename
        self.errors = []

    def parse(self):
        out = []
        while self.pos < len(self.lines):
            s = self.stmt(0)
            if s is None:
                break
            out.append(s)
        return out

    def stmt(self, min_indent):
        if self.pos >= len(self.lines):
            return None
        ln = self.lines[self.pos]
        if ln.indent < min_indent:
            return None
        start = self.pos
        try:
            return self._stmt_inner(min_indent)
        except VesnaError as e:
            if self.pos <= start:
                self.pos = start + 1
            line = e.line or ln.line_no
            self.errors.append((line, e.msg))
            return ('error', e.msg, line)

    def _stmt_inner(self, min_indent):
        ln = self.lines[self.pos]
        c = ln.content

        if c.startswith('if ') and ln.block_start:
            return self.parse_if(ln)
        if c.startswith('while ') and ln.block_start:
            return self.parse_while(ln)
        if c.startswith('for ') and ln.block_start:
            return self.parse_for(ln)
        if c.startswith('def ') and ln.block_start:
            return self.parse_def(ln)
        if c == 'try' and ln.block_start:
            return self.parse_try(ln)
        if c.startswith('import '):
            self.pos += 1
            return ('import', c[7:].strip())

        self.pos += 1

        if c == 'break':
            return ('break',)
        if c == 'continue':
            return ('continue',)
        if c.startswith('back(') and c.endswith(')'):
            return ('back', parse_expr(c[5:-1], ln.line_no))

        return self.parse_simple(c, ln.line_no)
    
    def parse_if(self, ln):
        cond = parse_expr(ln.content[3:], ln.line_no)
        self.pos += 1
        branches = []
        cur_cond = cond
        cur_body = []
        while True:
            # 收集当前分支的 body（缩进 = ln.indent + 1）
            while self.pos < len(self.lines):
                nxt = self.lines[self.pos]
                if nxt.indent != ln.indent + 1:
                    break
                s = self.stmt(ln.indent + 1)
                if s:
                    cur_body.append(s)
                else:
                    break
            if self.pos >= len(self.lines):
                break
            nxt = self.lines[self.pos]
            if nxt.indent != ln.indent:
                break
            c = nxt.content
            if c.startswith('elif '):
                branches.append((cur_cond, cur_body))
                cur_cond = parse_expr(c[5:], nxt.line_no)
                cur_body = []
                self.pos += 1
            elif c == 'else':
                branches.append((cur_cond, cur_body))
                cur_cond = None
                cur_body = []
                self.pos += 1
            else:
                break
        branches.append((cur_cond, cur_body))
        return ('if', branches)
    def parse_while(self, ln):
        cond = parse_expr(ln.content[6:], ln.line_no)
        self.pos += 1
        body = []
        while self.pos < len(self.lines):
            nxt = self.lines[self.pos]
            if nxt.indent != ln.indent + 1:
                break
            s = self.stmt(ln.indent + 1)
            if s:
                body.append(s)
        return ('while', cond, body)

    def parse_for(self, ln):
        m = re.match(r'for\s+(\w+)\s+in\s+(.+)$', ln.content)
        if not m:
            raise VesnaError("for 语法错误，应为: for 变量 in 表达式-", ln.line_no)
        var = m.group(1)
        iter_expr = parse_expr(m.group(2), ln.line_no)
        self.pos += 1
        body = []
        while self.pos < len(self.lines):
            nxt = self.lines[self.pos]
            if nxt.indent != ln.indent + 1:
                break
            s = self.stmt(ln.indent + 1)
            if s:
                body.append(s)
        return ('for', var, iter_expr, body)

    def parse_def(self, ln):
        m = re.match(r'def\s+(\w+)\s*\((.*)\)\s*$', ln.content)
        if not m:
            raise VesnaError("def 语法错误", ln.line_no)
        name = m.group(1)
        params = self._split_params(m.group(2), ln.line_no)
        self.pos += 1
        body = []
        while self.pos < len(self.lines):
            nxt = self.lines[self.pos]
            if nxt.indent != ln.indent + 1:
                break
            s = self.stmt(ln.indent + 1)
            if s:
                body.append(s)
        return ('def', name, params, body)

    def _split_params(self, s, ln):
        if not s.strip():
            return []
        parts = []
        depth = 0
        cur = []
        for ch in s:
            if ch in '([{':
                depth += 1; cur.append(ch)
            elif ch in ')]}':
                depth -= 1; cur.append(ch)
            elif ch == ';' and depth == 0:
                parts.append(''.join(cur).strip()); cur = []
            else:
                cur.append(ch)
        if cur:
            parts.append(''.join(cur).strip())

        out = []
        for p in parts:
            if '=' in p:
                name, default_str = p.split('=', 1)
                out.append((name.strip(), parse_expr(default_str.strip(), ln)))
            else:
                out.append((p, None))
        return out

    def parse_try(self, ln):
        self.pos += 1
        try_body = []
        while self.pos < len(self.lines):
            nxt = self.lines[self.pos]
            if nxt.indent != ln.indent + 1:
                break
            if nxt.content.startswith('catch '):
                break
            s = self.stmt(ln.indent + 1)
            if s:
                try_body.append(s)

        if self.pos >= len(self.lines) or not self.lines[self.pos].content.startswith('catch '):
            raise VesnaError("try 需要 catch", ln.line_no)

        catch_line = self.lines[self.pos]
        m = re.match(r'catch\s+(\w+)\s*$', catch_line.content)
        if not m:
            raise VesnaError("catch 语法错误，应为: catch 变量", catch_line.line_no)
        catch_var = m.group(1)
        self.pos += 1

        catch_body = []
        while self.pos < len(self.lines):
            nxt = self.lines[self.pos]
            if nxt.indent != ln.indent + 1:
                break
            s = self.stmt(ln.indent + 1)
            if s:
                catch_body.append(s)

        return ('try', try_body, catch_var, catch_body)

    def parse_simple(self, content, ln):
        toks = lex_expr(content, ln)
        depth = 0
        op_pos = -1
        op_kind = None
        for i, t in enumerate(toks):
            if t.kind in ('LPAREN', 'LBRACK', 'LBRACE'):
                depth += 1
            elif t.kind in ('RPAREN', 'RBRACK', 'RBRACE'):
                depth -= 1
            elif depth == 0 and t.kind in ('ASSIGN', 'PLUSEQ', 'MINUSEQ', 'STAREQ', 'SLASHEQ'):
                op_pos = i
                op_kind = 'ASSIGN' if t.kind == 'ASSIGN' else t.value
                break

        if op_pos >= 0:
            left = toks[:op_pos] + [Token('EOF', None, ln)]
            right = toks[op_pos + 1:]
            lv = self.parse_lvalue(left)
            val = ExprParser(right).parse()
            return ('assign', lv, op_kind, val)

        return ('expr', ExprParser(toks).parse())

    def parse_lvalue(self, toks):
        p = ExprParser(toks)
        t = p.peek()
        if t.kind != 'IDENT':
            raise VesnaError("赋值左侧必须是变量", t.line)
        name = t.value
        p.advance()
        if p.peek().kind == 'LBRACK':
            p.advance()
            idx = p.parse()
            p.expect('RBRACK')
            return ('index', name, idx)
        return ('var', name)


# ============================================================
# 运行时
# ============================================================

class Env:
    def __init__(self, parent=None):
        self.vars = {}
        self.funcs = {}
        self.parent = parent

    def get(self, name):
        if name in self.vars:
            return self.vars[name]
        if self.parent:
            return self.parent.get(name)
        return name  # 未定义变量当作字符串

    def set(self, name, val):
        self.vars[name] = val

    def get_func(self, name):
        if name in self.funcs:
            return self.funcs[name]
        if self.parent:
            return self.parent.get_func(name)
        raise VesnaError(f"未定义函数: {name}")

    def set_func(self, name, fn):
        self.funcs[name] = fn


class Function:
    def __init__(self, name, params, body, closure):
        self.name = name
        self.params = params  # [(name, default_expr_or_None)]
        self.body = body
        self.closure = closure


# ============================================================
# 运算符（字典分派 + 快速类型检查）
# ============================================================

def _is_num(t):
    return t is int or t is float


def _vesna_eq(a, b):
    ta, tb = type(a), type(b)
    if ta is not tb:
        if _is_num(ta) and _is_num(tb):
            return a == b
        return False
    return a == b


def _op_add(a, b):
    ta, tb = type(a), type(b)
    if ta is str and tb is str:
        return a + b
    if ta is bool or tb is bool:
        raise VesnaError("布尔值不能相加")
    if _is_num(ta) and _is_num(tb):
        return a + b
    raise VesnaError(f"无法相加: {fmt(a)} + {fmt(b)}")


def _num_only(a, b):
    ta, tb = type(a), type(b)
    if ta is bool or tb is bool:
        raise VesnaError("该运算符需要数字")
    if not _is_num(ta) or not _is_num(tb):
        raise VesnaError("该运算符需要数字")


def _op_sub(a, b):
    _num_only(a, b)
    return a - b


def _op_mul(a, b):
    _num_only(a, b)
    return a * b


def _op_div(a, b):
    _num_only(a, b)
    if b == 0:
        raise VesnaError("除数不能为 0")
    return a / b


def _op_idiv(a, b):
    _num_only(a, b)
    if b == 0:
        raise VesnaError("除数不能为 0")
    return int(a / b)


def _op_fdiv(a, b):
    _num_only(a, b)
    if b == 0:
        raise VesnaError("除数不能为 0")
    return a / b - int(a / b)


def _op_mod(a, b):
    _num_only(a, b)
    if b == 0:
        raise VesnaError("除数不能为 0")
    return a % b


def _op_eq(a, b):
    return _vesna_eq(a, b)


def _op_ne(a, b):
    return not _vesna_eq(a, b)


def _op_lt(a, b):
    try:
        return a < b
    except TypeError:
        raise VesnaError(f"无法比较: {fmt(a)} 和 {fmt(b)}")


def _op_gt(a, b):
    try:
        return a > b
    except TypeError:
        raise VesnaError(f"无法比较: {fmt(a)} 和 {fmt(b)}")


def _op_le(a, b):
    try:
        return a <= b
    except TypeError:
        raise VesnaError(f"无法比较: {fmt(a)} 和 {fmt(b)}")


def _op_ge(a, b):
    try:
        return a >= b
    except TypeError:
        raise VesnaError(f"无法比较: {fmt(a)} 和 {fmt(b)}")


_BINOP_FUNCS = {
    'PLUS': _op_add,
    'MINUS': _op_sub,
    'STAR': _op_mul,
    'SLASH': _op_div,
    './': _op_idiv,
    '/.': _op_fdiv,
    '/-': _op_mod,
    '==': _op_eq,
    '!=': _op_ne,
    'LT': _op_lt,
    'GT': _op_gt,
    '<=': _op_le,
    '>=': _op_ge,
}
class Interp:
    def __init__(self, argv=None, script_dir='.'):
        self.g = Env()
        self.argv = argv or []
        self.script_dir = script_dir

    def run(self, program):
        for s in program:
            self.exec(s, self.g)

    def exec(self, stmt, env):
        k = stmt[0]

        if k == 'assign':
            self.assign(stmt[1], stmt[2], self.eval(stmt[3], env), env)

        elif k == 'expr':
            self.eval(stmt[1], env)

        elif k == 'back':
            raise ReturnSignal(self.eval(stmt[1], env))

        elif k == 'break':
            raise BreakSignal()

        elif k == 'continue':
            raise ContinueSignal()

        elif k == 'if':
            for cond, body in stmt[1]:
                if cond is None or self.eval(cond, env) is True:
                    for s in body:
                        self.exec(s, env)
                    break

        elif k == 'while':
            while self.eval(stmt[1], env) is True:
                try:
                    for s in stmt[2]:
                        self.exec(s, env)
                except BreakSignal:
                    break
                except ContinueSignal:
                    continue

        elif k == 'for':
            var = stmt[1]
            iterable = self.eval(stmt[2], env)
            if isinstance(iterable, dict):
                items = list(iterable.keys())
            elif isinstance(iterable, (list, tuple)):
                items = list(iterable)
            elif isinstance(iterable, str):
                items = list(iterable)
            else:
                raise VesnaError(f"无法迭代: {fmt(iterable)}")
            for item in items:
                env.set(var, item)
                try:
                    for s in stmt[3]:
                        self.exec(s, env)
                except BreakSignal:
                    break
                except ContinueSignal:
                    continue

        elif k == 'def':
            env.set_func(stmt[1], Function(stmt[1], stmt[2], stmt[3], env))

        elif k == 'try':
            try:
                for s in stmt[1]:
                    self.exec(s, env)
            except VesnaError as e:
                env.set(stmt[2], str(e))
                for s in stmt[3]:
                    self.exec(s, env)

        elif k == 'import':
            self.do_import(stmt[1], env)
        
        elif k == 'error':
            raise VesnaError(stmt[1], stmt[2])

        else:
            raise VesnaError(f"未知语句 {k}")

    def assign(self, lv, op_kind, val, env):
        if lv[0] == 'var':
            name = lv[1]
            if op_kind != 'ASSIGN':
                val = self.binop(COMPOUND_OP[op_kind], env.get(name), val)
            env.set(name, val)
        else:
            name = lv[1]
            idx = self.eval(lv[2], env)
            target = env.get(name)
            if isinstance(target, list):
                py_idx = int(idx) - 1 if idx > 0 else int(idx)
                if op_kind != 'ASSIGN':
                    val = self.binop(COMPOUND_OP[op_kind], target[py_idx], val)
                target[py_idx] = val
            elif isinstance(target, dict):
                if op_kind != 'ASSIGN':
                    old = target.get(idx)
                    val = self.binop(COMPOUND_OP[op_kind], old, val)
                target[idx] = val
            else:
                raise VesnaError("只能对列表或字典下标赋值")

    def do_import(self, name, env):
        name = name.strip()
        paths = [
            os.path.join(self.script_dir, name + '.ves'),
            os.path.join(self.script_dir, 'lib', name + '.ves'),
            os.path.join(VESNA_HOME, 'lib', name + '.ves'),
        ]
        for p in paths:
            if os.path.exists(p):
                with open(p, encoding='utf-8') as f:
                    src = f.read()
                sub_parser = Parser(preprocess(src), p)
                program = sub_parser.parse()
                if sub_parser.errors:
                    line, msg = sub_parser.errors[0]
                    raise VesnaError(f"{name}.ves: {msg}", line)
                sub = Interp(self.argv, os.path.dirname(p))
                sub.run(program)
                for k, v in sub.g.vars.items():
                    env.set(k, v)
                for k, v in sub.g.funcs.items():
                    env.set_func(k, v)
                return
        raise VesnaError(f"找不到模块: {name}")

    def eval(self, e, env):
        k = e[0]

        if k == 'num': return e[1]
        if k == 'str': return e[1]
        if k == 'bool': return e[1]
        if k == 'none': return None                  
        if k == 'var': return env.get(e[1])
        if k == 'interp': return self.interp(e[1], env)

        if k == 'neg':
            v = self.eval(e[1], env)
            if isinstance(v, bool) or not isinstance(v, (int, float)):
                raise VesnaError("一元负号只能用于数字")
            return -v

        if k == 'not':
            return not self.truthy(self.eval(e[1], env))
        if k == 'and':
            if not self.truthy(self.eval(e[1], env)):
                return False
            return self.truthy(self.eval(e[2], env))
        if k == 'or':
            if self.truthy(self.eval(e[1], env)):
                return True
            return self.truthy(self.eval(e[2], env))

        if k == 'bin':
            return self.binop(e[1], self.eval(e[2], env), self.eval(e[3], env))

        if k == 'group': return tuple(self.eval(x, env) for x in e[1])
        if k == 'list': return [self.eval(x, env) for x in e[1]]
        if k == 'dict': return {self.eval(a, env): self.eval(b, env) for a, b in e[1]}

        if k == 'index':
            t = self.eval(e[1], env)
            i = self.eval(e[2], env)
            if isinstance(i, str):
                try:
                    return t[i]
                except (IndexError, KeyError):
                    raise VesnaError(f"键不存在: {i}")
            if isinstance(i, float) and i.is_integer():
                i = int(i)
            try:
                return t[i - 1 if i > 0 else i]
            except (IndexError, KeyError):
                raise VesnaError(f"下标越界: {i}")

        if k == 'builtin': return self.builtin(e[1], e[2], env)
        if k == 'into': return self.into(e[1], self.eval(e[2], env))
        if k == 'call': return self.call(e[1], e[2], env)

        raise VesnaError(f"未知表达式 {k}")

    def truthy(self, v):
        if v is None or v is False: return False
        if v is True: return True
        if isinstance(v, (int, float)): return v != 0
        if isinstance(v, (str, list, tuple, dict)): return len(v) > 0
        return True

    def binop(self, op, l, r, _table=_BINOP_FUNCS):
        f = _table.get(op)
        if f is None:
            raise VesnaError(f"未知运算符 {op}")
        return f(l, r)

    def eq(self, l, r):
        return _vesna_eq(l, r)

    def interp(self, tpl, env):
        out = []
        i = 0
        while i < len(tpl):
            if tpl[i] == '(':
                j = tpl.find(')', i + 1)
                if j == -1:
                    out.append(tpl[i:]); break
                name = tpl[i + 1:j]
                v = env.get(name)
                if isinstance(v, str): out.append(v)
                elif isinstance(v, bool): out.append('true' if v else 'false')
                elif isinstance(v, (int, float)): out.append(str(v))
                else: out.append(fmt(v))
                i = j + 1
            else:
                out.append(tpl[i]); i += 1
        return ''.join(out)

    def into(self, t, v):
        if t == 'int':
            if isinstance(v, bool): return 1 if v else 0
            if isinstance(v, int): return v
            if isinstance(v, float): return int(v)
            if isinstance(v, str):
                try: return int(v)
                except ValueError:
                    try: return int(float(v))
                    except ValueError: raise VesnaError(f"无法转成 int: {v}")
            raise VesnaError(f"无法转成 int: {fmt(v)}")
        if t == 'str':
            if isinstance(v, str): return v
            if isinstance(v, bool): return 'true' if v else 'false'
            if isinstance(v, (int, float)): return str(v)
            return fmt(v)
        if t == 'float':
            if isinstance(v, bool): return 1.0 if v else 0.0
            if isinstance(v, (int, float)): return float(v)
            if isinstance(v, str):
                try: return float(v)
                except ValueError: raise VesnaError(f"无法转成 float: {v}")
            raise VesnaError(f"无法转成 float: {fmt(v)}")
        if t == 'bool':
            return self.truthy(v)
        if t == 'list':
            if isinstance(v, list): return v
            if isinstance(v, tuple): return list(v)
            if isinstance(v, str): return list(v)
            raise VesnaError(f"无法转成 list: {fmt(v)}")
        if t == 'dict':
            if isinstance(v, dict): return v
            raise VesnaError(f"无法转成 dict: {fmt(v)}")
        raise VesnaError(f"未知类型 {t}")

    def builtin(self, name, args, env):
        def ev(i): return self.eval(args[i], env)

        if name == 'up':
            v = ev(0)
            if not isinstance(v, str): raise VesnaError("-up 需要字符串")
            return v.upper()
        if name == 'down':
            v = ev(0)
            if not isinstance(v, str): raise VesnaError("-down 需要字符串")
            return v.lower()
        if name == 'len':
            v = ev(0)
            if isinstance(v, (str, list, tuple, dict)): return len(v)
            raise VesnaError("-len 需要字符串/列表/组/字典")
        if name == 'sub':
            s, start, end = ev(0), ev(1), ev(2)
            if not isinstance(s, str): raise VesnaError("-sub 第一个参数需要字符串")
            return s[int(start) - 1:int(end)]
        if name == 'split':
            s = ev(0)
            sep = ev(1) if args[1:] else ' '
            if not isinstance(s, str): raise VesnaError("-split 第一个参数需要字符串")
            return s.split(sep)
        if name == 'join':
            lst = ev(0)
            sep = ev(1) if args[1:] else ''
            if not isinstance(lst, (list, tuple)): raise VesnaError("-join 第一个参数需要列表/组")
            return sep.join(x if isinstance(x, str) else fmt(x) for x in lst)
        if name == 'find':
            s, sub = ev(0), ev(1)
            if not isinstance(s, str): raise VesnaError("-find 需要字符串")
            i = s.find(sub)
            return i + 1 if i >= 0 else 0
        if name == 'replace':
            s, old, new = ev(0), ev(1), ev(2)
            if not isinstance(s, str): raise VesnaError("-replace 第一个参数需要字符串")
            return s.replace(old, new)
        if name == 'append':
            lst, v = ev(0), ev(1)
            if not isinstance(lst, list): raise VesnaError("-append 第一个参数需要列表")
            lst.append(v); return lst
        if name == 'pop':
            lst = ev(0)
            if not isinstance(lst, list): raise VesnaError("-pop 需要列表")
            if not lst: raise VesnaError("-pop 空列表")
            return lst.pop()
        if name == 'keys':
            d = ev(0)
            if not isinstance(d, dict): raise VesnaError("-keys 需要字典")
            return list(d.keys())
        if name == 'values':
            d = ev(0)
            if not isinstance(d, dict): raise VesnaError("-values 需要字典")
            return list(d.values())
        if name == 'type':
            return self.type_name(ev(0))
        if name == 'args':
            return list(self.argv)
        if name == 'fread':
            with open(ev(0), encoding='utf-8') as f:
                return f.read()
        if name == 'fwrite':
            path, content = ev(0), ev(1)
            with open(path, 'w', encoding='utf-8') as f:
                f.write(content if isinstance(content, str) else fmt(content))
            return None
        if name == 'fexists':
            return os.path.exists(ev(0))
        if name == 'exit':
            raise ExitSignal(int(ev(0)) if args else 0)
        if name == 'f':
            raise VesnaError("-f 必须紧接字符串: -f\"...\"")
                # ---- 字符串 ----
        if name == 'trim':
            v = ev(0)
            if not isinstance(v, str): raise VesnaError("-trim 需要字符串")
            return v.strip()

        if name == 'startswith':
            s, p = ev(0), ev(1)
            if not isinstance(s, str): raise VesnaError("-startswith 第一个参数需要字符串")
            return s.startswith(p)

        if name == 'endswith':
            s, p = ev(0), ev(1)
            if not isinstance(s, str): raise VesnaError("-endswith 第一个参数需要字符串")
            return s.endswith(p)

        if name == 'lines':
            v = ev(0)
            if not isinstance(v, str): raise VesnaError("-lines 需要字符串")
            return v.splitlines()

        if name == 'repeat':
            s, n = ev(0), ev(1)
            if not isinstance(s, str): raise VesnaError("-repeat 第一个参数需要字符串")
            return s * int(n)
        
        if name == 'has_key':
            d, k = ev(0), ev(1)
            if not isinstance(d, dict): raise VesnaError("-has_key 第一个参数需要字典")
            return k in d

        if name == 'str':
            return self.into('str', ev(0))

        if name == 'char_at':
            s = ev(0)
            i = ev(1)
            if not isinstance(s, str):
                raise VesnaError("-char_at 第一个参数需要字符串")
            i = int(i)
            if i < 1 or i > len(s):
                raise VesnaError(f"-char_at 下标越界: {i}")
            return s[i - 1]

        # ---- 列表 ----
        if name == 'sort':
            lst = ev(0)
            if not isinstance(lst, (list, tuple)): raise VesnaError("-sort 需要列表/组")
            return sorted(lst)

        if name == 'reverse':
            lst = ev(0)
            if not isinstance(lst, (list, tuple)): raise VesnaError("-reverse 需要列表/组")
            return list(reversed(lst))

        if name == 'slice':
            lst, start, end = ev(0), ev(1), ev(2)
            if not isinstance(lst, (list, tuple, str)): raise VesnaError("-slice 第一个参数需要列表/组/字符串")
            return lst[int(start) - 1:int(end)]

        if name in ('map', 'filter'):
            lst = ev(0)
            if not isinstance(lst, (list, tuple)): raise VesnaError(f"-{name} 第一个参数需要列表/组")
            fname = ev(1)
            if not isinstance(fname, str): raise VesnaError(f"-{name} 第二个参数需要函数名字符串")
            fn = env.get_func(fname)
            out = []
            for item in lst:
                local = Env(fn.closure)
                if len(fn.params) < 1: raise VesnaError(f"{fname} 需要 1 个参数")
                local.set(fn.params[0][0], item)
                try:
                    for s in fn.body:
                        self.exec(s, local)
                    result = None
                except ReturnSignal as rs:
                    result = rs.value
                if name == 'map':
                    out.append(result)
                else:
                    if self.truthy(result):
                        out.append(item)
            return out

        if name == 'reduce':
            lst = ev(0)
            if not isinstance(lst, (list, tuple)): raise VesnaError("-reduce 第一个参数需要列表/组")
            fname = ev(1)
            init = ev(2) if len(args) > 2 else None
            if not isinstance(fname, str): raise VesnaError("-reduce 第二个参数需要函数名字符串")
            fn = env.get_func(fname)
            acc = init
            for item in lst:
                local = Env(fn.closure)
                if len(fn.params) < 2: raise VesnaError(f"{fname} 需要 2 个参数")
                local.set(fn.params[0][0], acc)
                local.set(fn.params[1][0], item)
                try:
                    for s in fn.body:
                        self.exec(s, local)
                    acc = None
                except ReturnSignal as rs:
                    acc = rs.value
            return acc

        # ---- 正则 ----
        if name == 'match':
            s, pat = ev(0), ev(1)
            if not isinstance(s, str): raise VesnaError("-match 第一个参数需要字符串")
            return re.search(pat, s) is not None

        if name == 'search':
            s, pat = ev(0), ev(1)
            if not isinstance(s, str): raise VesnaError("-search 第一个参数需要字符串")
            m = re.search(pat, s)
            if m is None:
                return []
            return list(m.groups()) if m.groups() else [m.group(0)]

        if name == 'findall':
            s, pat = ev(0), ev(1)
            if not isinstance(s, str): raise VesnaError("-findall 第一个参数需要字符串")
            return [m if isinstance(m, str) else list(m) for m in re.findall(pat, s)]

        if name == 'gsub':
            s, pat, repl = ev(0), ev(1), ev(2)
            if not isinstance(s, str): raise VesnaError("-gsub 第一个参数需要字符串")
            return re.sub(pat, repl, s)

        # ---- 文件 ----
        if name == 'fappend':
            path, content = ev(0), ev(1)
            with open(path, 'a', encoding='utf-8') as f:
                f.write(content if isinstance(content, str) else fmt(content))
            return None

        if name == 'ls':
            d = ev(0) if args else '.'
            if not isinstance(d, str): d = '.'
            return sorted(os.listdir(d))

        if name == 'glob':
            import glob as _glob
            pat = ev(0)
            return sorted(_glob.glob(pat))

        if name == 'stdin':
            return sys.stdin.read()

         # ---- 类型转换简写 ----
        if name == 'str':
            return self.into('str', ev(0))
        if name == 'int':
            return self.into('int', ev(0))
        if name == 'float':
            return self.into('float', ev(0))
        if name == 'bool':
            return self.into('bool', ev(0))

        # ---- 字符 ----
        if name == 'char_at':
            s = ev(0)
            i = ev(1)
            if not isinstance(s, str):
                raise VesnaError("-char_at 第一个参数需要字符串")
            i = int(i)
            if i < 1 or i > len(s):
                raise VesnaError(f"-char_at 下标越界: {i}")
            return s[i - 1]

        if name == 'ord':
            c = ev(0)
            if not isinstance(c, str) or len(c) < 1:
                raise VesnaError("-ord 需要非空字符串")
            return ord(c[0])

        if name == 'chr':
            n = ev(0)
            n = int(n)
            try:
                return chr(n)
            except ValueError:
                raise VesnaError(f"-chr 无效码点: {n}")

        if name == 'is_digit':
            c = ev(0)
            return isinstance(c, str) and len(c) == 1 and c.isdigit()

        if name == 'is_alpha':
            c = ev(0)
            return isinstance(c, str) and len(c) == 1 and c.isalpha()

        if name == 'is_alnum':
            c = ev(0)
            return isinstance(c, str) and len(c) == 1 and c.isalnum()

        if name == 'is_space':
            c = ev(0)
            return isinstance(c, str) and len(c) == 1 and c.isspace()

        # ---- 字符串扩展 ----
        if name == 'lstrip':
            v = ev(0)
            if not isinstance(v, str):
                raise VesnaError("-lstrip 需要字符串")
            return v.lstrip()

        if name == 'rstrip':
            v = ev(0)
            if not isinstance(v, str):
                raise VesnaError("-rstrip 需要字符串")
            return v.rstrip()

        if name == 'title':
            v = ev(0)
            if not isinstance(v, str):
                raise VesnaError("-title 需要字符串")
            return v.title()

        if name == 'capitalize':
            v = ev(0)
            if not isinstance(v, str):
                raise VesnaError("-capitalize 需要字符串")
            return v.capitalize()

        if name == 'count':
            s = ev(0)
            sub = ev(1)
            if not isinstance(s, str):
                raise VesnaError("-count 第一个参数需要字符串")
            return s.count(sub)

        if name == 'rfind':
            s = ev(0)
            sub = ev(1)
            if not isinstance(s, str):
                raise VesnaError("-rfind 第一个参数需要字符串")
            i = s.rfind(sub)
            return i + 1 if i >= 0 else 0

        # ---- 数学 ----
        if name == 'min':
            v = ev(0)
            if isinstance(v, (list, tuple)):
                if len(v) == 0:
                    raise VesnaError("-min 不能是空列表")
                return min(v)
            return min(ev(i) for i in range(len(args)))

        if name == 'max':
            v = ev(0)
            if isinstance(v, (list, tuple)):
                if len(v) == 0:
                    raise VesnaError("-max 不能是空列表")
                return max(v)
            return max(ev(i) for i in range(len(args)))

        if name == 'sum':
            v = ev(0)
            if not isinstance(v, (list, tuple)):
                raise VesnaError("-sum 需要列表/组")
            return sum(v)

        if name == 'abs':
            v = ev(0)
            if isinstance(v, bool) or not isinstance(v, (int, float)):
                raise VesnaError("-abs 需要数字")
            return abs(v)

        if name == 'round':
            v = ev(0)
            if len(args) > 1:
                n = int(ev(1))
                return round(v, n)
            return round(v)

        if name == 'pow':
            a = ev(0)
            b = ev(1)
            return a ** b

        # ---- 容器 ----
        if name == 'contains':
            v = ev(0)
            x = ev(1)
            if isinstance(v, (list, tuple, str, dict)):
                return x in v
            raise VesnaError("-contains 需要列表/组/字符串/字典")

        # ---- 字典 ----
        if name == 'has_key':
            d = ev(0)
            k = ev(1)
            if not isinstance(d, dict):
                raise VesnaError("-has_key 第一个参数需要字典")
            return k in d 

        # ---- 文件系统 ----
        if name == 'mkdir':
            path = ev(0)
            if not isinstance(path, str):
                raise VesnaError("-mkdir 需要字符串")
            os.makedirs(path, exist_ok=True)
            return None

        if name == 'copy':
            import shutil
            src, dst = ev(0), ev(1)
            if not isinstance(src, str) or not isinstance(dst, str):
                raise VesnaError("-copy 参数需要字符串")
            if not os.path.exists(src):
                raise VesnaError(f"-copy 源不存在: {src}")
            if os.path.isdir(src):
                shutil.copytree(src, dst, dirs_exist_ok=True)
            else:
                d = os.path.dirname(dst)
                if d and not os.path.exists(d):
                    os.makedirs(d, exist_ok=True)
                shutil.copy2(src, dst)
            return None

        if name == 'rmdir':
            import shutil
            path = ev(0)
            if not isinstance(path, str):
                raise VesnaError("-rmdir 需要字符串")
            if not os.path.exists(path):
                return None
            if os.path.isdir(path):
                shutil.rmtree(path, ignore_errors=True)
            else:
                os.remove(path)
            return None

        if name == 'rename':
            src, dst = ev(0), ev(1)
            if not isinstance(src, str) or not isinstance(dst, str):
                raise VesnaError("-rename 参数需要字符串")
            os.rename(src, dst)
            return None

        if name == 'getenv':
            key = ev(0)
            if not isinstance(key, str):
                raise VesnaError("-getenv 需要字符串")
            return os.environ.get(key, "")

        if name == 'setenv':
            key, val = ev(0), ev(1)
            if not isinstance(key, str) or not isinstance(val, str):
                raise VesnaError("-setenv 参数需要字符串")
            if sys.platform == 'win32':
                import winreg
                try:
                    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, "Environment",
                                        0, winreg.KEY_SET_VALUE) as k:
                        winreg.SetValueEx(k, key, 0, winreg.REG_SZ, val)
                except Exception as e:
                    raise VesnaError(f"-setenv 失败: {e}")
            else:
                os.environ[key] = val
            return None

        if name == 'cwd':
            return os.getcwd()

        if name == 'chdir':
            path = ev(0)
            if not isinstance(path, str):
                raise VesnaError("-chdir 需要字符串")
            try:
                os.chdir(path)
            except FileNotFoundError:
                raise VesnaError(f"-chdir 目录不存在: {path}")
            return None

        if name == 'regwrite':
            import winreg
            root = ev(0)
            path = ev(1)
            key = ev(2)
            value = ev(3)
            roots = {
                'HKCU': winreg.HKEY_CURRENT_USER,
                'HKLM': winreg.HKEY_LOCAL_MACHINE,
            }
            h = roots.get(root)
            if h is None:
                raise VesnaError(f"-regwrite 未知根: {root}")
            with winreg.CreateKey(h, path) as k:
                winreg.SetValueEx(k, key, 0, winreg.REG_SZ, value)
            return None

        if name == 'regdelete':
            import winreg
            root = ev(0)
            path = ev(1)
            roots = {
                'HKCU': winreg.HKEY_CURRENT_USER,
                'HKLM': winreg.HKEY_LOCAL_MACHINE,
            }
            h = roots.get(root)
            if h is None:
                raise VesnaError(f"-regdelete 未知根: {root}")
            try:
                winreg.DeleteKey(h, path)
            except FileNotFoundError:
                pass
            return None

        if name == 'shell':
            import subprocess
            cmd = ev(0)
            if not isinstance(cmd, str):
                raise VesnaError("-shell 需要字符串")
            subprocess.run(cmd, shell=True, capture_output=True)
            return None

        if name == 'path_clean':
            import os as _os
            p = ev(0)
            if not isinstance(p, str):
                raise VesnaError("-path_clean 需要字符串")
            return _os.path.abspath(p)      

        raise VesnaError(f"未知内置 -{name}")

    def type_name(self, v):
        if v is None: return 'none'
        if isinstance(v, bool): return 'bool'
        if isinstance(v, int): return 'int'
        if isinstance(v, float): return 'float'
        if isinstance(v, str): return 'str'
        if isinstance(v, list): return 'list'
        if isinstance(v, tuple): return 'group'
        if isinstance(v, dict): return 'dict'
        return 'unknown'

    def call(self, name, args, env):
        if name == 'print':
            print(' '.join(fmt(self.eval(a, env)) for a in args))
            return None
        if name == 'input':
            p = ''
            if args:
                pv = self.eval(args[0], env)
                p = pv if isinstance(pv, str) else fmt(pv)
            return input(p)

        fn = env.get_func(name)
        if len(args) > len(fn.params):
            raise VesnaError(f"{name} 最多 {len(fn.params)} 个参数")

        local = Env(fn.closure)
        for i, (pname, pdefault) in enumerate(fn.params):
            if i < len(args):
                local.set(pname, self.eval(args[i], env))
            elif pdefault is not None:
                local.set(pname, self.eval(pdefault, fn.closure))
            else:
                raise VesnaError(f"{name} 缺少参数 {pname}")

        try:
            for s in fn.body:
                self.exec(s, local)
        except ReturnSignal as rs:
            return rs.value
        return None


# ============================================================
# 输出格式化
# ============================================================

def fmt(v):
    if v is None: return 'none'
    if isinstance(v, bool): return 'true' if v else 'false'
    if isinstance(v, str): return v
    if isinstance(v, (int, float)):
        if isinstance(v, float) and v.is_integer():
            return f"'{int(v)}'"
        return f"'{v}'"
    if isinstance(v, list):
        return '[' + ';'.join(fmt(x) for x in v) + ']'
    if isinstance(v, tuple):
        return '(' + ';'.join(fmt(x) for x in v) + ')'
    if isinstance(v, dict):
        return '{' + ','.join(f"{fmt(k)}:{fmt(vv)}" for k, vv in v.items()) + '}'
    return str(v)


# ============================================================
# 入口
# ============================================================

def run_source(src, argv=None, script_dir='.', filename='<stdin>', catch_exit=False):
    parser = Parser(preprocess(src), filename)
    program = parser.parse()
    if parser.errors:
        line, msg = parser.errors[0]
        raise VesnaError(msg, line)
    interp = Interp(argv or [], script_dir)
    try:
        interp.run(program)
    except ExitSignal as e:
        if catch_exit:
            return e.code
        sys.exit(e.code)
    except (BreakSignal, ContinueSignal):
        import traceback
        traceback.print_exc()
        raise VesnaError("break/continue 只能用于循环内")
    return 0


def run_file(path, argv=None):
    with open(path, encoding='utf-8') as f:
        src = f.read()
    return run_source(src, argv or [], os.path.dirname(os.path.abspath(path)), path)


def repl():
    print(f"Vesna {VERSION} — Scripts of spring")
    print("输入空行退出\n")
    buf = []
    interp = Interp()
    while True:
        try:
            line = input('>>> ' if not buf else '... ')
        except (EOFError, KeyboardInterrupt):
            print(); break
        if not line.strip() and not buf:
            break
        buf.append(line)
        try:
            program = Parser(preprocess('\n'.join(buf)), '<repl>').parse()
            for s in program:
                interp.exec(s, interp.g)
            buf = []
        except VesnaError as e:
            if '未闭合' in str(e):
                continue
            print(f"错误: {e}"); buf = []
        except (BreakSignal, ContinueSignal):
            print("错误: break/continue 只能用于循环内"); buf = []
        except ReturnSignal:
            print("错误: back 只能用于函数内"); buf = []


def main():
    if len(sys.argv) < 2:
        repl(); return

    arg = sys.argv[1]

    if arg == '--install':
        try:
            code = run_source(INSTALL_SCRIPT, sys.argv[2:], os.getcwd(), '<install>', catch_exit=True)
        except VesnaError as e:
            print(f"安装失败: {e}", file=sys.stderr)
            sys.exit(1)
        sys.exit(code or 0)

    if arg == '--version':
        print(f"Vesna {VERSION}")
        return

    if arg == '--help':
        print("用法:")
        print("  vesna <脚本.ves> [参数...]   运行脚本")
        print("  vesna                        进入 REPL")
        print("  vesna --install              安装 Vesna")
        print("  vesna --version              显示版本")
        print("  vesna --help                 显示帮助")
        return

    path = arg
    if not os.path.exists(path):
        print(f"找不到文件: {path}", file=sys.stderr)
        sys.exit(1)
    try:
        run_file(path, sys.argv[2:])
    except VesnaError as e:
        print(f"错误: {e}", file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError as e:
        print(f"找不到文件: {e.filename}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()