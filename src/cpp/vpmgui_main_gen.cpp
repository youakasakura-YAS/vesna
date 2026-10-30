// vpmgui_main.cpp — vpm-gui 独立可执行入口（嵌入 json.ves + gui.ves + vpm-gui.ves）
#include "vesna.hpp"
#include "platform.h"

#include <iostream>
#include <string>
#include <vector>

namespace vesna {

static const char* VPMGUI_SCRIPT = R"VES(/* vpm-gui self-contained bundle: json.ves + gui.ves + vpm-gui.ves */
/* Vesna 标准库：json.ves */

def json_parse(text)-
-chars = []
-for c in text-
--#append(chars; c)
-state = [chars; '1'; #len(chars)]
-back(json_value(state))

def json_peek(st)-
-if st['2'] > st['3']-
--back("")
-back(st['1'][st['2']])

def json_next(st)-
-st['2'] += '1'

def json_skip_ws(st)-
-while st['2'] <= st['3']-
--c = st['1'][st['2']]
--if c == " " or c == "\t" or c == "\n" or c == "\r"-
---st['2'] += '1'
--else-
---break

def json_value(st)-
-json_skip_ws(st)
-c = json_peek(st)
-if c == "{"-
--back(json_object(st))
-if c == "["-
--back(json_array(st))
-if c == "\""-
--back(json_string(st))
-if c == "t"-
--json_literal(st; "true")
--back(true)
-if c == "f"-
--json_literal(st; "false")
--back(false)
-if c == "n"-
--json_literal(st; "null")
--back(none)
-back(json_number(st))

def json_literal(st; word)-
-i = '1'
-while i <= #len(word)-
--json_next(st)
--i += '1'

def json_string(st)-
-json_skip_ws(st)
-json_next(st)
-out = ""
-while st['2'] <= st['3']-
--c = json_peek(st)
--json_next(st)
--if c == "\""-
---back(out)
--if c == "\\"-
---e = json_peek(st)
---json_next(st)
---if e == "n"-
----out = out + "\n"
---elif e == "t"-
----out = out + "\t"
---elif e == "r"-
----out = out + "\r"
---elif e == "\""-
----out = out + "\""
---elif e == "\\"-
----out = out + "\\"
---else-
----out = out + e
--else-
---out = out + c
-back(out)

def json_number(st)-
-json_skip_ws(st)
-start = st['2']
-while st['2'] <= st['3']-
--c = st['1'][st['2']]
--if c == "-" or c == "+" or c == "." or c == "e" or c == "E"-
---st['2'] += '1'
--elif c >= "0" and c <= "9"-
---st['2'] += '1'
--else-
---break
-num_str = ""
-i = start
-while i < st['2']-
--num_str = num_str + st['1'][i]
--i += '1'
-if #find(num_str; ".") > '0'-
--back(#float(num_str))
-if #find(num_str; "e") > '0'-
--back(#float(num_str))
-back(#int(num_str))

def json_object(st)-
-json_skip_ws(st)
-json_next(st)
-obj = {}
-json_skip_ws(st)
-if json_peek(st) == "}"-
--json_next(st)
--back(obj)
-while true-
--json_skip_ws(st)
--k = json_string(st)
--json_skip_ws(st)
--json_next(st)
--v = json_value(st)
--obj[k] = v
--json_skip_ws(st)
--c = json_peek(st)
--if c == ","-
---json_next(st)
---continue
--json_next(st)
--back(obj)

def json_array(st)-
-json_skip_ws(st)
-json_next(st)
-arr = []
-json_skip_ws(st)
-if json_peek(st) == "]"-
--json_next(st)
--back(arr)
-while true-
--v = json_value(st)
--#append(arr; v)
--json_skip_ws(st)
--c = json_peek(st)
--if c == ","-
---json_next(st)
---continue
--json_next(st)
--back(arr)

def json_write(v)-
-back(json_ser(v))

def json_ser(v)-
-t = #type(v)
-if t == "none"-
--back("null")
-if t == "bool"-
--if v-
---back("true")
--back("false")
-if t == "int"-
--back(#str(v))
-if t == "float"-
--back(#str(v))
-if t == "str"-
--back(json_esc(v))
-if t == "list"-
--back(json_arr(v))
-if t == "group"-
--back(json_arr(v))
-if t == "dict"-
--back(json_obj(v))
-back("null")

def json_esc(s)-
-out = "\""
-for c in s-
--if c == "\""-
---out = out + "\\\""
--elif c == "\\"-
---out = out + "\\\\"
--elif c == "\n"-
---out = out + "\\n"
--elif c == "\t"-
---out = out + "\\t"
--elif c == "\r"-
---out = out + "\\r"
--else-
---out = out + c
-back(out + "\"")

def json_arr(v)-
-out = "["
-first = true
-for item in v-
--if not first-
---out = out + ","
--first = false
--out = out + json_ser(item)
-back(out + "]")

def json_obj(d)-
-out = "{"
-first = true
-ks = #keys(d)
-for k in ks-
--if not first-
---out = out + ","
--first = false
--out = out + json_esc(k) + ":" + json_ser(d[k])
-back(out + "}")
/* ============================================================
   vesna-gui 2.0.0 - Web GUI framework (pure Vesna, zero new builtins)
   Web 图形化操作平台框架（纯 Vesna，零新增内置、零第三方库）

   Usage / 用法:
     import vesna-gui,
     routes = {"GET /": "page_home"; "POST /api/todo": "api_add"},
     assets = {"/static/app.js": js},
     gui_serve('8080'; routes; assets),

   2.0 新增能力 / New in 2.0:
     - 方法与路径参数路由   method + path-param routing   ("GET /api/u/:id")
     - 块读高性能请求解析   chunked request parsing (4096B)
     - query / cookie / 表单 / multipart 文件上传解析
     - 响应：重定向 / JSON / 静态文件 / 下载 / SSE 推送 / Set-Cookie
     - Cookie 会话（服务端存储）  cookie sessions
     - 模板引擎 {{var}} {%for%} {%if%}   template engine
     - UI 组件库：页面/表格/表单/输入/按钮/卡片/徽章   UI components
   ============================================================ */

/* ---- 1 内部工具 / internals ---- */

def _lower(s)-
-if #is_str(s) == false-
--back(""),
-,
-out = "",
-for i in #range('1'; #len(s) + '1')-
--ch = #char_at(s; i),
--o = #ord(ch),
--if o >= '65' and o <= '90'-
---out = out + #chr(o + '32'),
--else-
---out = out + ch,
--,
-,
-back(out),
,

/* 字符串（任意字节）转字节列表 0-255 */
def _str_to_bytes(s)-
-out = [],
-for i in #range('1'; #len(s) + '1')-
--out = #append(out; #ord(#char_at(s; i))),
-,
-back(out),
,

/* 字节列表 0-255 转字符串 */
def _bytes_to_str(bytes)-
-out = "",
-for b in bytes-
--out = out + #chr(b),
-,
-back(out),
,

/* ---- 2 请求解析 / request parsing ---- */

/* 解析查询串 "a=1&b=2" -> {"a": "1"; "b": "2"}（url_decode 值） */
def _parse_query(q)-
-d = {},
-if #len(q) == '0'-
--back(d),
-,
-for kv in #split(q; "&")-
--p = #split(kv; "="),
--k = #url_decode(p['1']),
--v = "",
--if #len(p) >= '2'-
---v = #url_decode(p['2']),
--,
--d[k] = v,
-,
-back(d),
,

/* 解析 Cookie 头 "a=1; b=2" -> {"a": "1"; "b": "2"} */
def _parse_cookies(h)-
-d = {},
-if #is_str(h) == false-
--back(d),
-,
-if #len(h) == '0'-
--back(d),
-,
-for kv in #split(h; ";")-
--kv = #trim(kv),
--if #len(kv) > '0'-
---p = #split(kv; "="),
---k = _lower(#trim(p['1'])),
---v = "",
---if #len(p) >= '2'-
----v = #trim(p['2']),
---,
---d[k] = v,
--,
-,
-back(d),
,

/* 解析请求头块 -> {"content-type": ...; "content-length": ...}（键小写） */
def _parse_headers(raw)-
-d = {},
-for l in #split(raw; "\r\n")-
--if #len(l) == '0'-
---break,
--,
--c2 = #find(l; ":"),
--if c2 > '0'-
---k = _lower(#trim(#sub(l; '1'; c2 - '1'))),
---v = #trim(#sub(l; c2 + '1'; #len(l))),
---d[k] = v,
--,
-,
-back(d),
,

/* 解析 multipart/form-data body，返回 {"form": {...}; "files": {...}} */
def _parse_multipart(body; boundary)-
-form = {},
-files = {},
-parts = #split(body; "--" + boundary),
-for pt in parts-
--pt = #trim(pt),
--if #len(pt) == '0' or pt == "--"-
---continue,
--,
--hdr_end = #find(pt; "\r\n\r\n"),
--if hdr_end <= '0'-
---continue,
--,
--hdr = #sub(pt; '1'; hdr_end - '1'),
--content = #sub(pt; hdr_end + '4'; #len(pt)),
--name = "",
--filename = "",
--for l in #split(hdr; "\r\n")-
---c2 = #find(l; ":"),
---if c2 <= '0'-
----continue,
---,
---k = _lower(#trim(#sub(l; '1'; c2 - '1'))),
---v = #trim(#sub(l; c2 + '1'; #len(l))),
---if k == "content-disposition"-
----nq = #find(v; "name=\""),
----if nq > '0'-
-----name = #sub(v; nq + '6'; #len(v)),
-----eq = #find(name; "\""),
-----if eq > '0'-
------name = #sub(name; '1'; eq - '1'),
-----,
----,
----fq = #find(v; "filename=\""),
----if fq > '0'-
-----filename = #sub(v; fq + '10'; #len(v)),
-----eq2 = #find(filename; "\""),
-----if eq2 > '0'-
------filename = #sub(filename; '1'; eq2 - '1'),
-----,
----,
--,
--if #len(filename) > '0'-
---files[name] = {"name": filename; "b64": #base64_encode(content); "size": #len(content)},
--else-
---form[name] = content,
-,
-back({"form": form; "files": files}),
,

/* 读一个请求：块读（4096B）头 + Content-Length body，返回原始字符串 */
def _read_conn(c)-
-buf = "",
-while true-
--b = #tcp_recv(c; '4096'),
--if #is_str(b) == false-
---break,
--,
--if #len(b) == '0'-
---break,
--,
--buf = buf + b,
--if #contains(buf; "\r\n\r\n")-
---break,
--,
--if #len(buf) > '65536'-
---break,
-,
-,
-back(buf),
,

/* 原始请求串 -> 请求 dict */
def _parse_req(raw)-
-hb = #find(raw; "\r\n\r\n"),
-if hb <= '0'-
--back({"method": "GET"; "path": "/"; "query": {}; "headers": {}; "cookies": {}; "body": ""; "form": {}; "files": {}; "params": {}}),
-,
-head = #sub(raw; '1'; hb - '1'),
-body = #sub(raw; hb + '4'; #len(raw)),
-lines = #split(head; "\r\n"),
-first = lines['1'],
-parts = #split(first; " "),
-method = "GET",
-path = "/",
-if #len(parts) >= '3'-
--method = parts['1'],
--path = parts['2'],
-,
-query = "",
-raw_path = path,
-qp = #find(path; "?"),
-if qp > '0'-
--query = #sub(path; qp + '1'; #len(path)),
--path = #sub(path; '1'; qp - '1'),
-,
-hs = _parse_headers(head),
-cookies = _parse_cookies(#get(hs; "cookie")),
-req = {"method": method; "path": path; "raw_path": raw_path; "query": _parse_query(query); "headers": hs; "cookies": cookies; "body": body; "form": {}; "files": {}; "params": {}},
-ct = _lower(#get(hs; "content-type")),
-if #len(body) > '0'-
--if #contains(ct; "application/x-www-form-urlencoded")-
---req["form"] = _parse_query(body),
--else-
---if #contains(ct; "multipart/form-data")-
----bq = #find(ct; "boundary="),
----if bq > '0'-
-----bnd = #sub(ct; bq + '9'; #len(ct)),
-----mp = _parse_multipart(body; bnd),
-----req["form"] = mp["form"],
-----req["files"] = mp["files"],
----,
--,
-,
-back(req),
,

/* 路由匹配：routes 键形如 "GET /p/:id"、"/prefix/*"、"POST /x"；返回 {"fn"; "params"} 或空 */
def _route_match(routes; method; path)-
-for key in #keys(routes)-
--km = #split(key; " "),
--kmethod = "ANY",
--kpath = key,
--if #len(km) >= '2'-
---kmethod = km['1'],
---kpath = km['2'],
--,
--if kmethod != "ANY" and kmethod != method-
---continue,
--,
--kp = #split(kpath; "/"),
--pp = #split(path; "/"),
--if #len(kp) == #len(pp)-
---params = {},
---ok = true,
---for i in #range('1'; #len(kp) + '1')-
----a = kp[i],
----b = pp[i],
----if #startswith(a; ":")-
-----params[#sub(a; '2'; #len(a))] = #url_decode(b),
----else-
-----if a != b-
------ok = false,
-----,
----,
---,
---if ok-
----back({"fn": routes[key]; "params": params}),
---,
-,
-,
-back(""),
,

/* ---- 3 响应 / responses ---- */

def _resp(code; type; body; extra)-
-h = "HTTP/1.1 " + #str(code) + " " + _reason(code) + "\r\nContent-Type: " + type + "\r\nContent-Length: " + #str(#len(body)) + "\r\n",
-if #has_key(extra; "location")-
--h = h + "Location: " + #get(extra; "location") + "\r\n",
-,
-if #has_key(extra; "set_cookie")-
--h = h + "Set-Cookie: " + #get(extra; "set_cookie") + "\r\n",
-,
-h = h + "Connection: close\r\n\r\n",
-back(h + body),
,

def _reason(code)-
-if code == '200'-
--back("OK"),
-,
-if code == '302'-
--back("Found"),
-,
-if code == '400'-
--back("Bad Request"),
-,
-if code == '404'-
--back("Not Found"),
-,
-if code == '500'-
--back("Internal Server Error"),
-,
-back("Error"),
,

/* MIME 类型（按扩展名） */
def _mime(path)-
-e = "",
-dot = #rfind(path; "."),
-if dot > '0'-
--e = _lower(#sub(path; dot + '1'; #len(path))),
-,
-if e == "html" or e == "htm"-
--back("text/html; charset=utf-8"),
-,
-if e == "css"-
--back("text/css; charset=utf-8"),
-,
-if e == "js" or e == "mjs"-
--back("text/javascript; charset=utf-8"),
-,
-if e == "json"-
--back("application/json; charset=utf-8"),
-,
-if e == "png"-
--back("image/png"),
-,
-if e == "jpg" or e == "jpeg"-
--back("image/jpeg"),
-,
-if e == "gif"-
--back("image/gif"),
-,
-if e == "svg"-
--back("image/svg+xml"),
-,
-if e == "ico"-
--back("image/x-icon"),
-,
-if e == "webp"-
--back("image/webp"),
-,
-if e == "txt" or e == "md" or e == "log"-
--back("text/plain; charset=utf-8"),
-,
-if e == "pdf"-
--back("application/pdf"),
-,
-if e == "zip"-
--back("application/zip"),
-,
-if e == "woff2"-
--back("font/woff2"),
-,
-back("application/octet-stream"),
,

/* 结果 dict -> HTTP 响应（含静态文件/下载/JSON/重定向/SSE） */
def _apply_result(c; r; req)-
-if #type(r) == "str"-
--_tcp_send_resp(c; _resp('200'; "text/html; charset=utf-8"; r; {})),
--back("ok"),
-,
-if #type(r) != "dict"-
--_tcp_send_resp(c; _resp('500'; "text/plain; charset=utf-8"; "handler must return str or dict"; {})),
--back("ok"),
-,
-code = '200',
-type = "text/html; charset=utf-8",
-body = "",
-extra = {},
-extra["set_cookie"] = "",
-if #has_key(r; "redirect")-
--extra["location"] = #get(r; "redirect"),
--code = '302',
--body = "",
-,
-if #has_key(r; "json")-
--type = "application/json; charset=utf-8",
--body = #json_encode(#get(r; "json")),
-,
-if #has_key(r; "file")-
--fp = #get(r; "file"),
--if #fexists(fp) and #is_file(fp)-
---type = _mime(fp),
---body = #fread(fp),
--else-
---code = '404',
---body = "file not found: " + fp,
-,
-,
-if #has_key(r; "download")-
--fp = #get(r; "download"),
--if #fexists(fp) and #is_file(fp)-
---body = #fread(fp),
---type = "application/octet-stream",
---extra["disposition"] = "attachment; filename=\"" + #get(r; "name") + "\"",
--else-
---code = '404',
---body = "file not found: " + fp,
-,
-,
-if #has_key(r; "set_cookie")-
--extra["set_cookie"] = #get(r; "set_cookie"),
-,
-if #has_key(r; "sse")-
--fn = #get(r; "sse"),
--_sse_loop(c; fn; req),
--back("ok"),
-,
-if #len(body) == '0' and code == '200' and not #has_key(r; "redirect") and not #has_key(r; "json") and not #has_key(r; "file") and not #has_key(r; "download")-
--body = #get(r; "body"),
--if #has_key(r; "code")-
---code = #get(r; "code"),
-,
--if #has_key(r; "type")-
---type = #get(r; "type"),
-,
-,
-if #has_key(r; "code") and #has_key(r; "body") and not #has_key(r; "redirect") and not #has_key(r; "json") and not #has_key(r; "file") and not #has_key(r; "download")-
--code = #get(r; "code"),
--body = #get(r; "body"),
--if #has_key(r; "type")-
---type = #get(r; "type"),
-,
-,
-_tcp_send_resp(c; _resp(code; type; body; extra)),
-back("ok"),
,

/* SSE 长连接（轮询式）：fn(req) 每次被调用返回一行 data，返回 "STOP" 结束 */
def _sse_loop(c; fn; req)-
-h = "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nCache-Control: no-cache\r\nConnection: keep-alive\r\n\r\n",
-#tcp_send(c; h),
-try-
--while true-
---d = #call(fn; req),
---if d == "STOP"-
----break,
---,
---if #len(d) > '0'-
----#tcp_send(c; "data: " + d + "\n\n"),
---,
---#sleep('400'),
--,
-catch e-
--#tcp_send(c; "data: error: " + #str(e) + "\n\n"),
-,
-#tcp_close(c),
-back("ok"),
,

def _tcp_send_resp(c; msg)-
-try-
--#tcp_send(c; msg),
-catch e-
--, 
-#tcp_close(c),
,

/* ---- 4 会话 / sessions ---- */

_sessions = {},
_sid_lock = "vesna_gui_sid",

/* 读请求会话数据（无会话返回空 dict） */
def gui_session_data(req)-
-cookies = #get(req; "cookies"),
-sid = #get(cookies; "vesna_sid"),
-if #is_str(sid) == false or #len(sid) == '0'-
--back({}),
-,
-#lock(_sid_lock),
-d = #get(_sessions; sid),
-#unlock(_sid_lock),
-if #type(d) == "dict"-
--back(d),
-,
-back({}),
,

/* 在会话中写一个键值；返回带 Set-Cookie 的响应（首次创建会话） */
def gui_session_set(req; k; v)-
-cookies = #get(req; "cookies"),
-sid = #get(cookies; "vesna_sid"),
-#lock(_sid_lock),
-if #is_str(sid) == false or #len(sid) == '0' or #type(#get(_sessions; sid)) != "dict"-
--sid = #uuid(),
--_sessions[sid] = {},
-,
-_sessions[sid][k] = v,
-#unlock(_sid_lock),
-back({"code": '200'; "type": "text/plain; charset=utf-8"; "body": "ok"; "set_cookie": "vesna_sid=" + sid + "; Path=/; HttpOnly"}),
,

/* 删除会话 */
def gui_session_clear(req)-
-cookies = #get(req; "cookies"),
-sid = #get(cookies; "vesna_sid"),
-if #is_str(sid) and #len(sid) > '0'-
--#lock(_sid_lock),
--if #has_key(_sessions; sid)-
---#pop_key(_sessions; sid),
--,
--#unlock(_sid_lock),
-,
-back({"code": '200'; "type": "text/plain; charset=utf-8"; "body": "ok"; "set_cookie": "vesna_sid=; Path=/; Max-Age=0"}),
,

/* ---- 5 模板引擎 / template engine ---- */

/* {{name}} 转义；{{!name}} 不转义；{{list}} 为数组时 join 逗号 */
def _tpl_var(text; vars)-
-out = "",
-i = '1',
-n = #len(text),
-while i <= n-
--ls = #find(#sub(text; i; n); "{{"),
--if ls <= '0'-
---out = out + #sub(text; i; n),
---break,
--,
--out = out + #sub(text; i; i + ls - '2'),
--st = i + ls - '1',
--rs = #find(#sub(text; st; n); "}}"),
--if rs <= '0'-
---out = out + #sub(text; st; n),
---break,
--,
--expr = #trim(#sub(text; st + '2'; st + rs - '2')),
--raw = false,
--if #startswith(expr; "!")-
---raw = true,
---expr = #sub(expr; '2'; #len(expr)),
--,
--v = #get(vars; expr),
--if #type(v) == "list"-
---v = #join(v; ", "),
--,
--if #type(v) == "dict"-
---v = #json_encode(v),
--,
--if #type(v) == "none"-
---v = "",
--,
--if #type(v) == "str" and raw == false-
---v = gui_esc(v),
--else-
---if #type(v) != "str"-
----v = #str(v),
---,
--,
--out = out + v,
--i = st + rs + '1',
-,
-back(out),
,

/* {%for x in list%}...{%end%}（单层）；{%if k%}...{%else%}...{%end%} */
def _tpl_blocks(text; vars)-
-out = "",
-i = '1',
-n = #len(text),
-while i <= n-
--fs = #find(#sub(text; i; n); "{%"),
--if fs <= '0'-
---out = out + #sub(text; i; n),
---break,
--,
--out = out + #sub(text; i; i + fs - '2'),
--st = i + fs - '1',
--rs = #find(#sub(text; st; n); "%}"),
--if rs <= '0'-
---out = out + #sub(text; st; n),
---break,
--,
--tag = #trim(#sub(text; st + '2'; st + rs - '2')),
--body_start = st + rs + '1',
--if #startswith(tag; "for ")-
---rest = #sub(tag; '5'; #len(tag)),
---eq = #find(rest; " in "),
---if eq > '0'-
----x = #trim(#sub(rest; '1'; eq - '1')),
----lname = #trim(#sub(rest; eq + '4'; #len(rest))),
----lst = #get(vars; lname),
----if #type(lst) != "list"-
-----lst = [],
----,
----en = #find(#sub(text; body_start; n); "{%end%}"),
----if en > '0'-
-----inner = #sub(text; body_start; body_start + en - '2'),
-----nxt = body_start + en + '6',
-----acc = "",
-----for item in lst-
------nv = vars,
------nv[x] = item,
------acc = acc + _tpl_var(inner; nv),
-----,
-----out = out + acc,
-----i = nxt,
-----continue,
----,
---,
--,
--if #startswith(tag; "if ")-
---k = #trim(#sub(tag; '4'; #len(tag))),
---en = #find(#sub(text; body_start; n); "{%end%}"),
---if en > '0'-
----nxt = body_start + en + '6',
----inner = #sub(text; body_start; body_start + en - '2'),
----else_i = #find(inner; "{%else%}"),
----tt = "",
----ff = "",
----if else_i > '0'-
-----tt = #sub(inner; '1'; else_i - '1'),
-----ff = #sub(inner; else_i + '9'; #len(inner)),
----else-
-----tt = inner,
----,
----vv = #get(vars; k),
----if vv == false or vv == "" or #type(vv) == "none"-
-----out = out + _tpl_var(ff; vars),
----else-
-----out = out + _tpl_var(tt; vars),
----,
----i = nxt,
----continue,
---,
--,
--out = out + "{%" + tag + "%}",
--i = body_start,
-,
-back(out),
,

/* 模板渲染：先块级（for/if）再行级（变量） */
def gui_tpl(text; vars)-
-back(_tpl_var(_tpl_blocks(text; vars); vars)),
,

/* ---- 6 UI 组件 / UI components ---- */

def _css()-
-back("body{font-family:system-ui,-apple-system,'Segoe UI',sans-serif;margin:0;background:#f3f4f6;color:#111827;font-size:14px}.topbar{background:#111827;color:#fff;padding:12px 20px;font-size:17px;font-weight:600;display:flex;align-items:center;gap:12px}.topbar .brand{flex:1}.topbar a{color:#c7d2fe;text-decoration:none;font-size:13px;font-weight:400}.wrap{display:flex;gap:16px;padding:16px;align-items:flex-start;max-width:1200px;margin:0 auto}.side{width:200px;background:#fff;border-radius:10px;padding:10px;box-shadow:0 1px 3px rgba(0,0,0,.08);flex-shrink:0}.side a{display:block;padding:8px 12px;color:#374151;text-decoration:none;border-radius:6px;margin-bottom:2px}.side a.active{background:#eef2ff;color:#4f46e5;font-weight:600}.main{flex:1;background:#fff;border-radius:10px;padding:20px;box-shadow:0 1px 3px rgba(0,0,0,.08);min-width:0}.main h3{margin-top:0}.main textarea{width:100%;height:260px;font-family:ui-monospace,Consolas,monospace;font-size:13px;border:1px solid #e5e7eb;border-radius:6px;padding:10px;box-sizing:border-box}.row{display:flex;gap:10px;margin-top:10px;flex-wrap:wrap;align-items:center}.row button,.main button{padding:7px 16px;border:0;border-radius:6px;background:#4f46e5;color:#fff;cursor:pointer;font-size:13px}.row button.ghost,.main button.ghost{background:#e5e7eb;color:#374151}.main table{width:100%;border-collapse:collapse}.main th,.main td{padding:8px 10px;border-bottom:1px solid #e5e7eb;text-align:left;font-size:14px}.main th{color:#6b7280;font-weight:600}.main input[type=text],.main input[type=password],.main select{width:60%;padding:8px 10px;border:1px solid #e5e7eb;border-radius:6px;box-sizing:border-box}.main input[type=file]{padding:6px 0}.toast{position:fixed;bottom:24px;left:50%;transform:translateX(-50%);background:#111827;color:#fff;padding:10px 18px;border-radius:8px;font-size:14px;display:none;z-index:99}.card{background:#f9fafb;border:1px solid #e5e7eb;border-radius:8px;padding:14px 16px;margin-bottom:12px}.badge{display:inline-block;padding:2px 10px;border-radius:999px;font-size:12px;background:#eef2ff;color:#4f46e5;margin-right:6px}.badge.green{background:#ecfdf5;color:#059669}.badge.red{background:#fef2f2;color:#dc2626}.badge.gray{background:#f3f4f6;color:#6b7280}.kv{display:grid;grid-template-columns:auto 1fr;gap:4px 14px;font-size:13px}.kv b{color:#6b7280;font-weight:500}.main pre{background:#f9fafb;border:1px solid #e5e7eb;border-radius:6px;padding:10px;overflow:auto;font-size:12px;font-family:ui-monospace,Consolas,monospace}"),
,

/* 完整页面：opts 支持 {"nav": [{"href"; "name"}]; "current": "/"; "scripts": "js"} */
def gui_page(title; inner; opts)-
-nav = "",
-scripts = "",
-cur = "",
-if #type(opts) == "dict"-
--cur = #get(opts; "current"),
--scripts = #get(opts; "scripts"),
--items = #get(opts; "nav"),
--if #type(items) == "list"-
---for it in items-
----href = #get(it; "href"),
----nm = #get(it; "name"),
----cls = "",
----if href == cur-
-----cls = " class=\"active\"",
----,
----nav = nav + "<a" + cls + " href=\"" + href + "\">" + gui_esc(nm) + "</a>",
---,
--,
-,
-side = "",
-if #len(nav) > '0'-
--side = "<nav class=\"side\">" + nav + "</nav>",
-,
-scr = "",
-if #is_str(scripts) and #len(scripts) > '0'-
--scr = "<script>" + scripts + "</script>",
-,
-back("<!DOCTYPE html><html lang=\"zh-CN\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>" + gui_esc(title) + "</title><style>" + _css() + "</style></head><body><div class=\"topbar\"><span class=\"brand\">" + gui_esc(title) + "</span></div><div class=\"wrap\">" + side + "<div class=\"main\" id=\"main\">" + inner + "</div></div><div class=\"toast\" id=\"toast\"></div>" + scr + "</body></html>"),
,

/* HTML 转义 */
def gui_esc(s)-
-if #is_str(s) == false-
--back(""),
-,
-s = #replace(s; "&"; "&amp;"),
-s = #replace(s; "<"; "&lt;"),
-s = #replace(s; ">"; "&gt;"),
-s = #replace(s; "\""; "&quot;"),
-back(s),
,

/* JSON 响应 */
def gui_json(v)-
-back({"code": '200'; "type": "application/json; charset=utf-8"; "body": #json_encode(v)}),
,

/* 重定向 */
def gui_redirect(path)-
-back({"redirect": path}),
,

/* 静态文件（磁盘） */
def gui_file(path)-
-back({"file": path}),
,

/* 下载（附件） */
def gui_download(path; name)-
-back({"download": path; "name": name}),
,

/* 404 */
def gui_404(msg)-
-back({"code": '404'; "type": "text/plain; charset=utf-8"; "body": "404 " + msg}),
,

/* 表格 */
def gui_table(headers; rows)-
-h = "<table><thead><tr>",
-for i in headers-
--h = h + "<th>" + gui_esc(i) + "</th>",
-,
-h = h + "</tr></thead><tbody>",
-for r in rows-
--h = h + "<tr>",
--for i in r-
---h = h + "<td>" + gui_esc(i) + "</td>",
--,
--h = h + "</tr>",
-,
-h = h + "</tbody></table>",
-back(h),
,

/* 表单：fields = [{"name"; "label"; "type"(text/password); "value"}] */
def gui_form(action; method; fields; submit)-
-h = "<form method=\"" + method + "\" action=\"" + action + "\" enctype=\"application/x-www-form-urlencoded\">",
-for f in fields-
--nm = #get(f; "name"),
--lb = #get(f; "label"),
--tp = #get(f; "type"),
--vl = #get(f; "value"),
--if #len(tp) == '0'-
---tp = "text",
--,
--h = h + "<div class=\"row\"><label style=\"width:90px\">" + gui_esc(lb) + "</label><input type=\"" + tp + "\" name=\"" + nm + "\" value=\"" + gui_esc(vl) + "\"></div>",
-,
-h = h + "<div class=\"row\"><button type=\"submit\">" + gui_esc(submit) + "</button></div></form>",
-back(h),
,

/* multipart 上传表单 */
def gui_upload_form(action; fields)-
-h = "<form method=\"post\" action=\"" + action + "\" enctype=\"multipart/form-data\">",
-for f in fields-
--nm = #get(f; "name"),
--lb = #get(f; "label"),
--tp = #get(f; "type"),
--if #len(tp) == '0'-
---tp = "file",
--,
--h = h + "<div class=\"row\"><label style=\"width:90px\">" + gui_esc(lb) + "</label><input type=\"" + tp + "\" name=\"" + nm + "\"></div>",
-,
-h = h + "<div class=\"row\"><button type=\"submit\">上传</button></div></form>",
-back(h),
,

/* 单输入 */
def gui_input(name; label; type; value)-
-back("<div class=\"row\"><label style=\"width:90px\">" + gui_esc(label) + "</label><input type=\"" + type + "\" name=\"" + name + "\" value=\"" + gui_esc(value) + "\"></div>"),
,

/* 按钮 */
def gui_button(label; onclick)-
-if #len(onclick) == '0'-
--back("<button>" + gui_esc(label) + "</button>"),
-,
-back("<button onclick=\"" + onclick + "\">" + gui_esc(label) + "</button>"),
,

/* 卡片 */
def gui_card(title; body)-
-back("<div class=\"card\"><b>" + gui_esc(title) + "</b><div style=\"margin-top:6px\">" + body + "</div></div>"),
,

/* 徽章 */
def gui_badge(text; kind)-
-back("<span class=\"badge " + kind + "\">" + gui_esc(text) + "</span>"),
,

/* 链接 */
def gui_link(href; name)-
-back("<a href=\"" + href + "\">" + gui_esc(name) + "</a>"),
,

/* 键值展示 */
def gui_kv(pairs)-
-h = "<div class=\"kv\">",
-for p in pairs-
--h = h + "<b>" + gui_esc(p['0']) + "</b><span>" + gui_esc(p['1']) + "</span>",
-,
-h = h + "</div>",
-back(h),
,

/* ---- 7 服务入口 / server ---- */

/* 单连接处理（线程入口） */
def _serve_conn(c; routes; assets)-
-raw = _read_conn(c),
-req = _parse_req(raw),
-r = _handle_req(req; routes; assets),
-_apply_result(c; r; req),
-back("ok"),
,

/* 路由与静态资源分发 */
def _handle_req(req; routes; assets)-
-m = #get(req; "method"),
-p = #get(req; "path"),
-mt = _route_match(routes; m; p),
-if #type(mt) == "dict"-
--fn = #get(mt; "fn"),
--req["params"] = #get(mt; "params"),
--try-
---back(#call(fn; req)),
--catch e-
---back({"code": '500'; "type": "text/plain; charset=utf-8"; "body": "handler error: " + #str(e)}),
--,
-,
-if #has_key(assets; p)-
--a = assets[p],
--if #type(a) == "str"-
---back({"code": '200'; "type": "text/javascript; charset=utf-8"; "body": a}),
--,
--if #type(a) == "dict" and #has_key(a; "file")-
---fp = #get(a; "file"),
---if #fexists(fp) and #is_file(fp)-
----back({"code": '200'; "type": _mime(fp); "body": #fread(fp)}),
---,
---back({"code": '404'; "type": "text/plain; charset=utf-8"; "body": "asset not found"}),
--,
-,
-if #has_key(assets; "dir")-
--dir = #get(assets; "dir"),
--fp = dir + p,
--fp = #path_clean(fp),
--if #fexists(fp) and #is_file(fp) and #contains(fp; "..") == false-
---back({"code": '200'; "type": _mime(fp); "body": #fread(fp)}),
-,
-,
-back({"code": '404'; "type": "text/plain; charset=utf-8"; "body": "404 not found: " + p}),
,

/* 启动服务（阻塞）：每连接一线程 */
def gui_serve(port; routes; assets)-
-srv = #tcp_listen(port),
-print("vesna-gui 2.0 listening on http://localhost:" + #str(port)),
-print("routes: " + #join(#keys(routes); ", ")),
-while true-
--c = #tcp_accept(srv),
--#thread("_serve_conn"; c; routes; assets),
-,
,

/* ============================================================
   vpm-gui 2.0.0 - Vesna 包管理器图形化界面 (pure Vesna)
   Vesna Package Manager GUI — 全部用 Vesna 编写

   运行:  vesna vpm-gui.ves [端口]     默认端口 18082
   依赖:  vesna-gui (已装) + json 标准库 + vesna.exe --pkg
   ============================================================ */


/* ---------- 环境与工具 ---------- */

def _home()-
-h = #getenv("VESNA_HOME"),
-if #is_str(h) == false or #len(h) == '0'-
--back("C:\\Vesna"),
-back(h),
,

def _vesna_exe()-
-p = _home() + "\\bin\\vesna.exe",
-if #fexists(p)-
--back(p),
-back("vesna"),
,

def _read_json(path)-
-if not #fexists(path)-
--back(none),
-t = #fread(path),
-try-
--back(json_parse(t)),
-catch e-
--back(none),
,

def _meta_of(pkg_dir)-
-m = _read_json(pkg_dir + "\\vesna-pkg.json"),
-if #type(m) != "dict"-
--back({}),
-back(m),
,

def _installed_list()-
-out = [],
-pkgs_dir = _home() + "\\packages",
-if not #is_dir(pkgs_dir)-
--back(out),
-names = #ls(pkgs_dir),
-for nm in names-
--if nm == "_tmp" or nm == "_cache" or nm == "_pub"-
---continue,
--meta_file = pkgs_dir + "\\" + nm + "\\vesna-pkg.json",
--if not #fexists(meta_file)-
---continue,
--m = _meta_of(pkgs_dir + "\\" + nm),
--out = #append(out; {"name": #get(m; "name"); "version": #get(m; "version"); "description": #get(m; "description"); "entry": #get(m; "entry"); "dir": pkgs_dir + "\\" + nm}),
-,
-back(out),
,

def _registry_list()-
-r = _read_json(_home() + "\\registry.json"),
-if #type(r) != "list"-
--back([]),
-back(r),
,

def _run_vpm(cmd_args)-
-exe = _vesna_exe(),
-cmd = "\"" + exe + "\" --pkg " + cmd_args,
-r = #proc_run(cmd),
-exit_code = '0',
-out_text = "",
-if #type(r) == "dict"-
--exit_code = #get(r; "exit"),
--out_text = #get(r; "output"),
-,
-if #is_str(out_text) == false-
--out_text = "",
-,
-back({"exit": exit_code; "output": out_text}),
,

def _esc_field(v)-
-if #is_str(v) == false-
--back(""),
-back(v),
,

/* ---------- 页面 ---------- */

nav = [],
nav = #append(nav; {"href": "/"; "name": "仪表盘"}),
nav = #append(nav; {"href": "/installed"; "name": "已安装包"}),
nav = #append(nav; {"href": "/browse"; "name": "包花园"}),

def _page(title; inner; req; extra)-
-opts = {"nav": nav; "current": #get(req; "path"); "scripts": #get(extra; "scripts")},
-back(gui_page(title; inner; opts)),
,

def page_home(req)-
-/* 统计 */
-installed = _installed_list(),
-reg = _registry_list(),
-n_inst = #str(#len(installed)),
-n_reg = #str(#len(reg)),
-ves = #version(),
-/* 卡片 */
-card1 = gui_card("已安装包"; "<div class=\"kv\"><b>数量</b><span>" + n_inst + "</span><b>目录</b><span>" + _home() + "\\packages</span></div>"),
-card2 = gui_card("包花园"; "<div class=\"kv\"><b>可获取</b><span>" + n_reg + "</span><b>索引</b><span>" + _home() + "\\registry.json</span></div>"),
-card3 = gui_card("运行时"; "<div class=\"kv\"><b>版本</b><span>" + _esc_field(ves) + "</span><b>命令</b><span>vesna --pkg</span></div>"),
-inner = "<h3>包管理器总览</h3>" + card1 + card2 + card3 + "<p><a href=\"/installed\">管理已安装包 →</a> <a href=\"/browse\">浏览包花园 →</a></p>",
-back(_page("包管理器"; inner; req; {})),
,

def page_installed(req)-
-rows = [],
-for p in _installed_list()-
--nm = #get(p; "name"),
--ver = #get(p; "version"),
--desc = #get(p; "description"),
--if #is_str(ver) == false-
---ver = "-",
--,
--if #is_str(desc) == false or #len(desc) == '0'-
---desc = "（无描述）",
--,
--rows = #append(rows; [nm; ver; desc; gui_link("/op?do=remove&name=" + nm; "卸载")]),
-,
-inner = "<h3>已安装包</h3><p>共 " + #str(#len(rows)) + " 个（卸载需无其他包依赖，或使用 --force）</p>",
-if #len(rows) == '0'-
--inner = inner + "<p>暂无已安装包。</p>",
-else-
--inner = inner + gui_table(["名称"; "版本"; "描述"; "操作"]; rows),
-,
-back(_page("已安装包"; inner; req; {})),
,

def page_browse(req)-
-q = #get(#get(req; "query"); "q"),
-if #is_str(q) == false-
--q = "",
-,
-q = #down(q),
-reg = _registry_list(),
-rows = [],
-for p in reg-
--if #type(p) == "dict"-
---nm = #get(p; "name"),
---ver = #get(p; "version"),
---desc = #get(p; "description"),
---if #is_str(nm) == false-
----continue,
---,
---if #is_str(ver) == false-
----ver = "-",
---,
---if #is_str(desc) == false-
----desc = "",
---,
---hit = "true",
---if #len(q) > '0'-
----hit = "false",
----if #find(#down(nm); q) != '0' or #find(#down(desc); q) != '0'-
-----hit = "true",
----,
---,
---if hit == "true"-
----rows = #append(rows; [nm; ver; desc; gui_link("/op?do=install&name=" + nm; "安装")]),
---,
-,
-/* 搜索表单 */
-search_form = "<form method=\"get\" action=\"/browse\" class=\"row\"><input type=\"text\" name=\"q\" value=\"" + gui_esc(q) + "\" placeholder=\"搜索包名或描述\"><button type=\"submit\">搜索</button>" + "<a class=\"ghost\" href=\"/op?do=registry\"><button type=\"button\" class=\"ghost\">刷新索引</button></a></form>",
-inner = "<h3>包花园</h3>" + search_form,
-if #len(rows) == '0'-
--inner = inner + "<p>没有匹配的包（可点击“刷新索引”更新 registry 缓存）。</p>",
-else-
--inner = inner + "<p>匹配 " + #str(#len(rows)) + " 个</p>" + gui_table(["名称"; "版本"; "描述"; "操作"]; rows),
-,
-back(_page("包花园"; inner; req; {})),
,

def page_op(req)-
-q = #get(req; "query"),
-do = #get(q; "do"),
-nm = #get(q; "name"),
-if #is_str(do) == false-
--do = "",
-,
-if #is_str(nm) == false-
--nm = "",
-,
-/* 执行 */
-res = {},
-title = "操作",
-msg = "",
-if do == "install"-
--title = "安装 " + nm,
--res = _run_vpm("install " + nm),
-elif do == "remove"-
--title = "卸载 " + nm,
--res = _run_vpm("remove " + nm),
-elif do == "registry"-
--title = "刷新索引",
--res = _run_vpm("registry"),
-else-
--res = {"exit": '-1'; "output": "未知操作: " + do},
-,
-exit_code = #get(res; "exit"),
-out_text = #get(res; "output"),
-badge = gui_badge("完成"; "green"),
-if exit_code != '0'-
--badge = gui_badge("失败"; "red"),
-,
-pre = "<pre style=\"white-space:pre-wrap;word-break:break-all\">" + gui_esc(out_text) + "</pre>",
-inner = "<h3>" + title + "</h3><p>退出码 " + #str(exit_code) + " " + badge + "</p>" + pre + "<p><a href=\"/installed\">← 返回已安装包</a> <a href=\"/browse\">← 返回包花园</a></p>",
-back(_page(title; inner; req; {})),
,

/* ---------- 启动 ---------- */

args = #args(),
port = "18082",
if #len(args) >= '1'-
--port = args['1'],
,
routes = {
  "GET /": "page_home";
  "GET /installed": "page_installed";
  "GET /browse": "page_browse";
  "GET /op": "page_op"
},
assets = {},
gui_serve(#int(port); routes; assets),
)VES";

int mainCli(int argc, char** argv) {
    setConsoleUtf8();
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    if (argc >= 2 && std::string(argv[1]) == "--version") {
        std::cout << "vpm-gui 2.0.0 (Vesna " << VERSION << ")" << std::endl;
        return 0;
    }
    if (argc >= 2 && std::string(argv[1]) == "--help") {
        std::cout << "vpm-gui — Vesna 包管理器图形化界面 (纯 Vesna)\n"
                  << "用法: vpm-gui.exe [端口]   默认 18082\n"
                  << "      vpm-gui.exe --version\n";
        return 0;
    }

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
    try {
        return runSource(VPMGUI_SCRIPT, args, ".", "<vpm-gui>", true);
    } catch (VesnaError& e) {
        std::cerr << "vpm-gui 错误: " << e.str() << std::endl;
        return 1;
    }
}

}  // namespace vesna

int main(int argc, char** argv) {
    return vesna::mainCli(argc, argv);
}
