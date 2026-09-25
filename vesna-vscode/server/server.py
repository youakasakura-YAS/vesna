#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Vesna LSP Server"""

import sys
import os
import re
import json

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    from vesna import preprocess, Parser, VesnaError
except ImportError as e:
    sys.stderr.write(f"无法导入 vesna: {e}\n")
    sys.exit(1)


DOCS = {}  # uri -> text


# ---------- JSON-RPC over stdio ----------

def read_message(stdin):
    headers = {}
    while True:
        line = stdin.readline()
        if not line:
            return None
        if line in (b'\r\n', b'\n'):
            break
        k, _, v = line.decode('utf-8').partition(':')
        headers[k.strip()] = v.strip()
    if 'Content-Length' not in headers:
        return None
    length = int(headers['Content-Length'])
    body = stdin.read(length)
    if not body:
        return None
    return json.loads(body)


def write_message(stdout, obj):
    body = json.dumps(obj, ensure_ascii=False).encode('utf-8')
    header = f"Content-Length: {len(body)}\r\n\r\n".encode('utf-8')
    stdout.write(header + body)
    stdout.flush()


def make_response(id_, result):
    return {"jsonrpc": "2.0", "id": id_, "result": result}


def make_notification(method, params):
    return {"jsonrpc": "2.0", "method": method, "params": params}


# ---------- 诊断 ----------

def diagnose(text):
    from vesna import preprocess, Parser, VesnaError, lex_expr

    errors = []
    seen = set()

    def add(line_no, msg):
        line = max(0, (line_no or 1) - 1)
        key = (line, msg)
        if key in seen:
            return
        seen.add(key)
        errors.append({
            "range": {
                "start": {"line": line, "character": 0},
                "end": {"line": line, "character": 9999},
            },
            "severity": 1,
            "source": "vesna",
            "message": msg,
        })

    # 1. 预处理
    try:
        lines = preprocess(text)
    except VesnaError as e:
        add(e.line or 1, e.msg)
        return errors
    except Exception as e:
        add(1, f"内部错误: {e}")
        return errors

    # 2. 解析，Parser 现在会收集所有错误
    try:
        parser = Parser(lines)
        parser.parse()
        for line_no, msg in parser.errors:
            add(line_no, msg)
    except Exception as e:
        add(1, f"内部错误: {e}")

    # 3. 逐行词法检查，补上解析器跳过的行
    for ln in lines:
        try:
            lex_expr(ln.content, ln.line_no)
        except VesnaError as e:
            add(e.line or ln.line_no, e.msg)

    return errors


# ---------- 补全 ----------

COMPLETION_KEYWORDS = [
    ('if', '条件判断'), ('elif', 'else if 分支'), ('else', '否则分支'),
    ('while', 'while 循环'), ('for', 'for 循环'), ('in', '遍历'),
    ('break', '跳出循环'), ('continue', '继续下一次循环'),
    ('def', '定义函数'), ('back', '函数返回'),
    ('try', '异常捕获'), ('catch', '捕获异常'), ('import', '导入模块'),
    ('and', '逻辑与'), ('or', '逻辑或'), ('not', '逻辑非'),
    ('true', '真'), ('false', '假'),
]

COMPLETION_BUILTINS = [
    ('print', '输出'), ('input', '读入'),
    ('#up', '转大写'), ('#down', '转小写'), ('#into', '类型转换'), ('#f', '插值字符串'),
    ('#len', '长度'), ('#sub', '取子串'), ('#split', '分割'), ('#join', '拼接'),
    ('#find', '查找'), ('#replace', '替换'),
    ('#append', '列表追加'), ('#pop', '列表弹出'),
    ('#keys', '字典键'), ('#values', '字典值'),
    ('#type', '类型'), ('#args', '命令行参数'),
    ('#fread', '读文件'), ('#fwrite', '写文件'), ('#fappend', '追加文件'),
    ('#fexists', '文件存在'),
    ('#trim', '去空白'), ('#lines', '按行切'),
    ('#startswith', '前缀'), ('#endswith', '后缀'), ('#repeat', '重复'),
    ('#sort', '排序'), ('#reverse', '反转'), ('#slice', '切片'),
    ('#map', '映射'), ('#filter', '过滤'), ('#reduce', '归约'),
    ('#match', '正则匹配'), ('#findall', '正则提取'),
    ('#gsub', '正则替换'), ('#search', '正则搜索'),
    ('#ls', '列目录'), ('#glob', '通配'), ('#stdin', '读标准输入'),
    ('#has_key', '键存在'),
    ('#str', '转字符串'), ('#int', '转整数'),
    ('#float', '转浮点'), ('#bool', '转布尔'),
    ('#char_at', '第 n 个字符'), ('#ord', '字符码'), ('#chr', '码转字符'),
    ('#is_digit', '是数字'), ('#is_alpha', '是字母'),
    ('#is_alnum', '字母数字'), ('#is_space', '是空白'),
    ('#lstrip', '去左空白'), ('#rstrip', '去右空白'),
    ('#title', '标题化'), ('#capitalize', '首字母大写'),
    ('#count', '计数'), ('#rfind', '从右查找'),
    ('#min', '最小'), ('#max', '最大'), ('#sum', '求和'),
    ('#abs', '绝对值'), ('#round', '四舍五入'), ('#pow', '幂'),
    ('#contains', '包含'),
    ('#mkdir', '建目录'), ('#copy', '复制'),
    ('#rmdir', '删目录'), ('#rename', '重命名'),
    ('#getenv', '读环境变量'), ('#setenv', '写环境变量'),
    ('#cwd', '当前目录'), ('#chdir', '切换目录'),
    ('#exit', '退出'),
    ('#regwrite', '写注册表'), ('#regdelete', '删注册表'),
    ('#shell', '执行命令'), ('#path_clean', '路径清理'),
]

COMPLETION_TYPES = [
    ('int', '整数类型'), ('str', '字符串类型'), ('float', '浮点类型'),
    ('list', '列表类型'), ('dict', '字典类型'), ('bool', '布尔类型'),
]


def collect_identifiers(text):
    ids = set()
    for line in text.split('\n'):
        i = line.find('/*')
        if i >= 0:
            j = line.find('*/', i)
            line = line[:i] + (line[j + 2:] if j >= 0 else '')
        for tok in re.finditer(r'[a-zA-Z_][a-zA-Z0-9_]*', line):
            ids.add(tok.group())
    return ids


def item(label, detail, kind):
    return {"label": label, "detail": detail, "kind": kind, "insertText": label}


def completions(text):
    out = []
    for k, d in COMPLETION_KEYWORDS:
        out.append(item(k, d, 14))    # Keyword
    for k, d in COMPLETION_BUILTINS:
        out.append(item(k, d, 3))     # Function
    for k, d in COMPLETION_TYPES:
        out.append(item(k, d, 25))    # TypeParameter
    for name in collect_identifiers(text):
        if name in ('true', 'false'):
            continue
        out.append(item(name, '变量', 6))  # Variable
    return out

def line_end(text, line_no):
    lines = text.split('\n')
    if line_no >= len(lines):
        return {"line": line_no, "character": 0}
    return {"line": line_no, "character": len(lines[line_no])}


def make_insert_action(title, text, line_no, insert_text):
    end = line_end(text, line_no)
    return {
        "title": title,
        "kind": "quickfix",
        "edit": {
            "changes": {
                # uri 会在外面替换
            }
        },
        "_insert": insert_text,
        "_pos": end,
    }


def code_actions(uri, text, diags):
    actions = []

    for d in diags:
        msg = d.get('message', '')
        line_no = d['range']['start']['line']
        lines = text.split('\n')
        line_text = lines[line_no] if line_no < len(lines) else ''
        end = {"line": line_no, "character": len(line_text)}

        suggestion = None
        if '数字未闭合' in msg:
            suggestion = ("补上单引号 '", "'")
        elif '插值字符串未闭合' in msg:
            suggestion = ('补上双引号 "', '"')
        elif '字符串未闭合' in msg:
            suggestion = ('补上双引号 "', '"')
        elif '期望 LPAREN' in msg:
            suggestion = ('补上括号 ()', '()')
        elif '期望 LBRACK' in msg:
            suggestion = ('补上中括号 []', '[]')
        elif '期望 LBRACE' in msg:
            suggestion = ('补上大括号 {}', '{}')
        elif '期望 SEMI' in msg:
            suggestion = ('补上分号 ;', ';')
        elif '期望 RPAREN' in msg:
            suggestion = ('补上右括号 )', ')')
        elif '期望 RBRACK' in msg:
            suggestion = ('补上右中括号 ]', ']')
        elif '期望 RBRACE' in msg:
            suggestion = ('补上右大括号 }', '}')
        elif '实际 EOF' in msg:
            suggestion = ('补上右括号 )', ')')

        if suggestion:
            title,insert_text = suggestion
            edit = {
                "range": {"start": end, "end": end},
                "newText": insert_text,
            }
            actions.append({
                "title": title,
                "kind": "quickfix",
                "diagnostics": [d],
                "isPreferred": True,
                "edit": {
                    "changes": {
                        uri: [edit]
                    }
                },
            })

    return actions


# ---------- 主循环 ----------

def main():
    stdin = sys.stdin.buffer
    stdout = sys.stdout.buffer

    while True:
        try:
            msg = read_message(stdin)
        except Exception:
            break
        if msg is None:
            break

        method = msg.get('method')
        id_ = msg.get('id')
        params = msg.get('params') or {}

        if method == 'initialize':
            write_message(stdout, make_response(id_, {
                "capabilities": {
                    "textDocumentSync": 1,
                    "completionProvider": {"triggerCharacters": []},
                    "hoverProvider": True,
                    "codeActionProvider": True,       
                },
                "serverInfo": {"name": "vesna-lsp", "version": "0.3.0"},
            }))

        elif method == 'initialized':
            pass

        elif method == 'textDocument/didOpen':
            uri = params['textDocument']['uri']
            text = params['textDocument']['text']
            DOCS[uri] = text
            write_message(stdout, make_notification(
                'textDocument/publishDiagnostics',
                {"uri": uri, "diagnostics": diagnose(text)},
            ))

        elif method == 'textDocument/didChange':
            uri = params['textDocument']['uri']
            changes = params.get('contentChanges') or []
            if changes:
                DOCS[uri] = changes[-1]['text']
            text = DOCS.get(uri, '')
            write_message(stdout, make_notification(
                'textDocument/publishDiagnostics',
                {"uri": uri, "diagnostics": diagnose(text)},
            ))

        elif method == 'textDocument/didClose':
            uri = params['textDocument']['uri']
            DOCS.pop(uri, None)
            write_message(stdout, make_notification(
                'textDocument/publishDiagnostics',
                {"uri": uri, "diagnostics": []},
            ))

        elif method == 'textDocument/codeAction':
            uri = params['textDocument']['uri']
            text = DOCS.get(uri, '')
            diags = params.get('context', {}).get('diagnostics', [])
            write_message(stdout, make_response(id_, code_actions(uri, text, diags)))

        elif method == 'textDocument/completion':
            uri = params['textDocument']['uri']
            text = DOCS.get(uri, '')
            write_message(stdout, make_response(id_, completions(text)))

        elif method == 'textDocument/hover':
            write_message(stdout, make_response(id_, None))

        elif method == 'shutdown':
            write_message(stdout, make_response(id_, None))

        elif method == 'exit':
            break

        else:
            if id_ is not None:
                write_message(stdout, make_response(id_, None))


if __name__ == '__main__':
    main()