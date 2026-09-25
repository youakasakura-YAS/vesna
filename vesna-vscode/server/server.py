#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Vesna LSP Server 0.4.0"""

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
    ('#regenv', '刷新环境变量'), ('#cpdir', '复制目录'),
    ('#sqrt', '平方根'), ('#floor', '向下取整'), ('#ceil', '向上取整'),
    ('#exp', '指数'), ('#log', '自然对数'), ('#log10', '常用对数'),
    ('#sin', '正弦'), ('#cos', '余弦'), ('#tan', '正切'),
    ('#sign', '符号'), ('#clamp', '夹取'), ('#rand', '随机小数'),
    ('#randint', '随机整数'), ('#choice', '随机选择'), ('#shuffle', '打乱'),
    ('#hex', '转十六进制'), ('#bin', '转二进制'), ('#oct', '转八进制'),
    ('#pad', '填充'), ('#lpad', '左填充'), ('#rpad', '右填充'),
    ('#format', '格式化'), ('#hash', '哈希'),
    ('#range', '整数序列'), ('#first', '首元素'), ('#last', '末元素'),
    ('#take', '取前 n'), ('#drop', '丢前 n'), ('#set', '去重'),
    ('#flatten', '展平'), ('#zip', '并行配对'), ('#insert', '插入'),
    ('#remove', '删除元素'), ('#index_of', '索引'), ('#enumerate', '带序号'),
    ('#concat', '合并'), ('#get', '取键值'), ('#items', '键值对'),
    ('#pop_key', '弹键'), ('#is_str', '是字符串'), ('#is_int', '是整数'),
    ('#is_float', '是浮点'), ('#is_bool', '是布尔'), ('#is_list', '是列表'),
    ('#is_dict', '是字典'), ('#is_none', '是 none'), ('#is_group', '是分组'),
    ('#now', '当前时间'), ('#date', '日期'), ('#sleep', '休眠'),
    ('#ticks', '毫秒计数'), ('#platform', '平台'), ('#temp_dir', '临时目录'),
    ('#fremove', '删文件'), ('#fmove', '移动文件'), ('#fsize', '文件大小'),
    ('#is_dir', '是目录'), ('#is_file', '是文件'), ('#mkdirs', '建目录树'),
    ('#base64_encode', 'Base64 编码'), ('#base64_decode', 'Base64 解码'),
    ('#url_encode', 'URL 编码'), ('#url_decode', 'URL 解码'),
    ('#each', '遍历'), ('#all', '全真'), ('#any', '任一真'),
    ('#find_first', '查首个'), ('#sort_by', '按键排序'), ('#throw', '抛异常'),
    ('#assert', '断言'),
    ('#thread', '开线程'), ('#thread_join', '等线程'), ('#thread_count', '线程数'),
    ('#lock', '加锁'), ('#unlock', '解锁'),
    ('#http_get', 'HTTP GET'), ('#http_post', 'HTTP POST'), ('#tcp_ping', 'TCP 探测'),
    ('#bin_read', '读二进制'), ('#bin_write', '写二进制'),
    ('#bin_hex', '转十六进制'), ('#bin_unhex', '十六进制还原'),
    ('#bin_base64_encode', '二进制 Base64'), ('#bin_base64_decode', 'Base64 还原'),
    ('#json_encode', 'JSON 序列化'), ('#json_decode', 'JSON 解析'),
    ('#re_groups', '正则分组'), ('#sha256', 'SHA-256 摘要'),
    ('#aes_encrypt', 'AES 加密'), ('#aes_decrypt', 'AES 解密'),
    ('#proc_run', '运行进程'), ('#ffi_call', '调用 C 函数'),
]

COMPLETION_TYPES = [
    ('int', '整数类型'), ('str', '字符串类型'), ('float', '浮点类型'),
    ('list', '列表类型'), ('dict', '字典类型'), ('bool', '布尔类型'),
]

# ---------- hover ----------

HOVER_KEYWORDS = {
    'if': '条件分支：`if 条件-` 后接单层缩进块，可配 `elif` / `else`。',
    'elif': '`elif 条件-`：前一个 `if` 未命中时的分支判断。',
    'else': '`else-`：前面条件全部不满足时执行的块。',
    'while': '`while 条件-`：条件为真时循环执行块体。',
    'for': '`for 变量 in 列表-`：遍历列表（索引从 1 开始）。',
    'in': '配合 `for` 使用：`for x in lst-`。',
    'break': '跳出当前循环。',
    'continue': '跳过本轮，进入下一轮循环。',
    'def': '`def 名(参数)-` 定义函数，块内用 `back(值)` 返回。',
    'back': '函数返回：`back(值)` 或 `back()`。',
    'try': '`try-` 开始异常捕获块，需配 `catch e-`。',
    'catch': '`catch e-`：捕获异常，变量 `e` 为错误信息。',
    'import': '`import 模块`：加载 `lib` 或 `packages` 中的模块。',
    'and': '逻辑与。', 'or': '逻辑或。', 'not': '逻辑非。',
    'true': '布尔真。', 'false': '布尔假。',
}

HOVER_BUILTINS = {
    '#json_encode': '`#json_encode(v)` → JSON 字符串。dict 保持插入顺序。',
    '#json_decode': '`#json_decode(s)` → dict/list/int/float/str/bool/none；非法输入报错。',
    '#re_groups': '`#re_groups(s; pattern)` → 首个正则匹配的捕获组列表；组 0 为整段，未匹配组为 none。',
    '#sha256': '`#sha256(s)` → 64 位十六进制 SHA-256 摘要（FIPS 180-4 验证）。',
    '#aes_encrypt': '`#aes_encrypt(data; key)` → AES-256-CBC+PKCS7 加密的 base64（密钥经 SHA-256 派生）。',
    '#aes_decrypt': '`#aes_decrypt(b64; key)` → 用相同密钥解密 base64 密文。',
    '#proc_run': '`#proc_run(cmd)` → `{"exit": 码, "output": stdout}`。',
    '#ffi_call': '`#ffi_call("dll"; "func"; arg...)` → 调用共享库 C 函数，返回 64 位整数。',
    '#thread': '`#thread(函数)` → 新线程执行函数，返回线程 id。',
    '#thread_join': '`#thread_join(id)` → 等待线程结束并返回其返回值。',
    '#thread_count': '`#thread_count()` → 存活线程数。',
    '#lock': '`#lock(name)` → 获取命名互斥锁。',
    '#unlock': '`#unlock(name)` → 释放命名互斥锁。',
    '#http_get': '`#http_get(url)` → GET 请求响应文本。',
    '#http_post': '`#http_post(url; body)` → POST 请求响应文本。',
    '#tcp_ping': '`#tcp_ping(host; port)` → TCP 连通性测试。',
    '#bin_read': '`#bin_read(path)` → 字节列表。',
    '#bin_write': '`#bin_write(path; bytes)` → 写入字节列表。',
    '#bin_hex': '`#bin_hex(bytes)` → 十六进制字符串。',
    '#bin_unhex': '`#bin_unhex(s)` → 十六进制字符串还原为字节列表。',
    '#bin_base64_encode': '`#bin_base64_encode(bytes)` → base64。',
    '#bin_base64_decode': '`#bin_base64_decode(s)` → base64 还原为字节列表。',
    '#print': '`print(a; b; ...)` → 输出并以空格分隔。',
    '#fread': '`#fread(path)` → 读取整个文件文本。',
    '#fwrite': '`#fwrite(path; text)` → 覆盖写入文件。',
    '#fappend': '`#fappend(path; text)` → 追加写入文件。',
    '#shell': '`#shell(cmd)` → 执行系统命令并返回输出。',
    '#len': '`#len(v)` → 字符串/列表/dict 的长度。',
    '#type': '`#type(v)` → 值的类型名。',
    '#str': '`#str(v)` → 转字符串。', '#int': '`#int(v)` → 转整数。',
    '#float': '`#float(v)` → 转浮点。', '#bool': '`#bool(v)` → 转布尔。',
}


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


def hover(text, line, character):
    lines = text.split('\n')
    if line >= len(lines):
        return None
    line_text = lines[line]
    left = line_text[:character]
    m = re.search(r'[a-zA-Z_#][a-zA-Z0-9_#]*$', left)
    if not m:
        return None
    word = m.group(0)
    md = None
    if word.startswith('#'):
        md = HOVER_BUILTINS.get(word)
    elif word in HOVER_KEYWORDS:
        md = HOVER_KEYWORDS[word]
    elif re.match(r'^\s*def\s+' + re.escape(word) + r'\b', line_text):
        md = '函数 `' + word + '`：' + line_text.strip()
    if md is None:
        return None
    return {
        "contents": {
            "kind": "markdown",
            "value": "**`" + word + "`**\n\n" + md,
        },
        "range": {
            "start": {"line": line, "character": max(0, character - len(word))},
            "end": {"line": line, "character": character},
        },
    }


def document_symbols(text):
    symbols = []
    for i, line in enumerate(text.split('\n')):
        m = re.match(r'^\s*def\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(?:\(|-)', line)
        if m:
            name = m.group(1)
            symbols.append({
                "name": name,
                "kind": 12,  # Function
                "range": {
                    "start": {"line": i, "character": 0},
                    "end": {"line": i, "character": len(line)},
                },
                "selectionRange": {
                    "start": {"line": i, "character": 0},
                    "end": {"line": i, "character": len(line)},
                },
            })
    return symbols


def folding_ranges(text):
    lines = text.split('\n')
    ranges = []
    depth = 0
    start = None
    for i, line in enumerate(lines):
        d = 0
        for ch in line:
            if ch == '-':
                d += 1
            else:
                break
        if d > depth and start is None:
            start = i
            depth = d
        elif d < depth:
            if start is not None and i - 1 > start:
                ranges.append({
                    "startLine": start,
                    "startCharacter": 0,
                    "endLine": i - 1,
                    "endCharacter": len(lines[i - 1]),
                })
            depth = d
            start = i if d > 0 else None
    if start is not None and len(lines) - 1 > start:
        ranges.append({
            "startLine": start,
            "startCharacter": 0,
            "endLine": len(lines) - 1,
            "endCharacter": len(lines[-1]),
        })
    return ranges


def line_end(text, line_no):
    lines = text.split('\n')
    if line_no >= len(lines):
        return {"line": line_no, "character": 0}
    return {"line": line_no, "character": len(lines[line_no])}


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
            title, insert_text = suggestion
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
                    "completionProvider": {"triggerCharacters": ['#']},
                    "hoverProvider": True,
                    "codeActionProvider": True,
                    "documentSymbolProvider": True,
                    "foldingRangeProvider": True,
                },
                "serverInfo": {"name": "vesna-lsp", "version": "0.4.0"},
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
            uri = params['textDocument']['uri']
            text = DOCS.get(uri, '')
            pos = params.get('position', {})
            write_message(stdout, make_response(id_, hover(text, pos.get('line', 0), pos.get('character', 0))))

        elif method == 'textDocument/documentSymbol':
            uri = params['textDocument']['uri']
            text = DOCS.get(uri, '')
            write_message(stdout, make_response(id_, document_symbols(text)))

        elif method == 'textDocument/foldingRange':
            uri = params['textDocument']['uri']
            text = DOCS.get(uri, '')
            write_message(stdout, make_response(id_, folding_ranges(text)))

        elif method == 'shutdown':
            write_message(stdout, make_response(id_, None))

        elif method == 'exit':
            break

        else:
            if id_ is not None:
                write_message(stdout, make_response(id_, None))


if __name__ == '__main__':
    main()
