# -*- coding: utf-8 -*-
"""备份快照（保留最近 3）+ git 提交推送"""
import shutil, glob, os, datetime, subprocess

ts = datetime.datetime.now().strftime('%Y%m%d-%H%M')
bk_root = r'F:\Vesna\_backup'
snap = os.path.join(bk_root, 'snap-' + ts)
os.makedirs(bk_root, exist_ok=True)

# 备份核心（源码+bin+文档+插件+发布）
dirs = ['src', 'bin', 'docs', 'lib', 'examples', 'vesna-vscode', 'tests']
os.makedirs(snap, exist_ok=True)
for d in dirs:
    s = os.path.join(r'F:\Vesna', d)
    if os.path.exists(s):
        shutil.copytree(s, os.path.join(snap, d))
for f in ['README.md', 'README.zh-CN.md', 'CHANGELOG.md', 'CHANGELOG.zh-CN.md',
          'CONTRIBUTING.md', 'CONTRIBUTING.zh-CN.md', 'LICENSE', 'VERSION',
          'RELEASE-NOTES-1.6.0.md']:
    s = os.path.join(r'F:\Vesna', f)
    if os.path.exists(s):
        shutil.copy(s, os.path.join(snap, f))
print('snapshot:', snap)

# 保留最近 3
snaps = sorted(glob.glob(os.path.join(bk_root, 'snap-*')))
for old in snaps[:-3]:
    shutil.rmtree(old, ignore_errors=True)
    print('removed old:', old)
print('kept snaps:', [os.path.basename(x) for x in sorted(glob.glob(os.path.join(bk_root, 'snap-*')))])

# git 提交推送（主仓库）
repo = r'F:\Vesna'
r = subprocess.run(['git', '-C', repo, 'status', '--porcelain'], capture_output=True, text=True)
print('--- git status ---')
print(r.stdout.strip())
if r.stdout.strip():
    subprocess.run(['git', '-C', repo, 'add', '-A'], check=True)
    subprocess.run(['git', '-C', repo, 'commit', '-m', '喵~'], check=True)
    for i in range(3):
        p = subprocess.run(['git', '-C', repo, 'push', 'origin', 'main'], capture_output=True, text=True)
        if p.returncode == 0:
            print('push OK')
            break
        print('push attempt %d failed: %s' % (i + 1, p.stderr.strip()[-200:]))
        import time; time.sleep(5)
else:
    print('nothing to commit')
