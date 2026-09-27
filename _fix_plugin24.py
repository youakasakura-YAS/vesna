# -*- coding: utf-8 -*-
import io, json

# 1) tmLanguage: 追加 7 个新内置（thread_id/sem_*/crc32/adler32）
p1 = r'F:\Vesna\vesna-vscode\syntaxes\vesna.tmLanguage.json'
s = io.open(p1, encoding='utf-8').read()
old = '|encrypt_file|decrypt_file|encrypt_dir|decrypt_dir|udp_open|udp_send|udp_recv|udp_close|dns_lookup)\\\\b"'
new = '|encrypt_file|decrypt_file|encrypt_dir|decrypt_dir|udp_open|udp_send|udp_recv|udp_close|dns_lookup|thread_id|sem_open|sem_wait|sem_post|sem_close|crc32|adler32)\\\\b"'
assert s.count(old) == 1, 'tmLanguage old not found (%d)' % s.count(old)
s = s.replace(old, new)
io.open(p1, 'w', encoding='utf-8', newline='\n').write(s)
print('tmLanguage builtins -> 205')

# 2) package.json: 版本 + 描述
p2 = r'F:\Vesna\vesna-vscode\package.json'
d = json.load(io.open(p2, encoding='utf-8'))
d['version'] = '2.4.0'
d['description'] = d['description'].replace('198 内置', '205 内置')
io.open(p2, 'w', encoding='utf-8', newline='\n').write(json.dumps(d, ensure_ascii=False, indent=2) + '\n')
print('package.json -> 2.4.0 / 205 内置')

# 3) README 双语同步
p3 = r'F:\Vesna\vesna-vscode\README.md'
s3 = io.open(p3, encoding='utf-8').read()
s3 = s3.replace('198 内置', '205 内置').replace('198 built-in', '205 built-in').replace('198 builtins', '205 builtins')
io.open(p3, 'w', encoding='utf-8', newline='\n').write(s3)
print('README synced')
