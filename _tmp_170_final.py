# -*- coding: utf-8 -*-
"""1.7.0 收尾：备份 + 主仓库提交 + docs 站同步"""
import shutil, glob, os, datetime, subprocess, time

# ---- 1) 备份 ----
ts = datetime.datetime.now().strftime('%Y%m%d-%H%M')
bk_root = r'F:\Vesna\_backup'
snap = os.path.join(bk_root, 'snap-' + ts)
os.makedirs(bk_root, exist_ok=True)
os.makedirs(snap, exist_ok=True)
for d in ['src', 'bin', 'docs', 'lib', 'examples', 'vesna-vscode', 'tests', 'tools']:
    s = os.path.join(r'F:\Vesna', d)
    if os.path.exists(s):
        shutil.copytree(s, os.path.join(snap, d))
for f in ['README.md', 'README.zh-CN.md', 'CHANGELOG.md', 'CHANGELOG.zh-CN.md',
          'CONTRIBUTING.md', 'CONTRIBUTING.zh-CN.md', 'LICENSE', 'VERSION',
          'RELEASE-NOTES-1.7.0.md']:
    s = os.path.join(r'F:\Vesna', f)
    if os.path.exists(s):
        shutil.copy(s, os.path.join(snap, f))
snaps = sorted(glob.glob(os.path.join(bk_root, 'snap-*')))
for old in snaps[:-3]:
    shutil.rmtree(old, ignore_errors=True)
    print('removed old snap:', os.path.basename(old))
print('kept snaps:', [os.path.basename(x) for x in sorted(glob.glob(os.path.join(bk_root, 'snap-*')))])

# ---- 2) 主仓库提交推送 ----
repo = r'F:\Vesna'
r = subprocess.run(['git', '-C', repo, 'status', '--porcelain'], capture_output=True, text=True)
print('--- git status ---')
print(r.stdout.strip()[:800])
if r.stdout.strip():
    subprocess.run(['git', '-C', repo, 'add', '-A'], check=True)
    subprocess.run(['git', '-C', repo, 'commit', '-m', '喵~'], check=True)
    for i in range(4):
        p = subprocess.run(['git', '-C', repo, 'push', 'origin', 'main'], capture_output=True, text=True)
        if p.returncode == 0:
            print('main push OK'); break
        print('push attempt %d failed' % (i+1), p.stderr.strip()[-150:])
        time.sleep(8)

# ---- 3) docs 站同步 ----
shutil.copy(r'F:\Vesna\CHANGELOG.md', r'F:\Vesna-docs\docs\en\guide\changelog.md')
shutil.copy(r'F:\Vesna\CHANGELOG.zh-CN.md', r'F:\Vesna-docs\docs\guide\changelog.md')
print('changelog synced')
r = subprocess.run(['npm', 'run', 'docs:build'], cwd=r'F:\Vesna-docs',
                   capture_output=True, text=True, shell=True)
print('docs build rc=%d' % r.returncode)
if r.returncode != 0:
    print(r.stdout[-400:]); print(r.stderr[-300:]); raise SystemExit('build failed')
deploy = r'F:\Vesna-docs\.deploy'
for item in os.listdir(deploy):
    if item == '.git':
        continue
    p = os.path.join(deploy, item)
    if os.path.isdir(p):
        shutil.rmtree(p, ignore_errors=True)
    else:
        os.remove(p)
dist = r'F:\Vesna-docs\docs\.vitepress\dist'
for item in os.listdir(dist):
    s = os.path.join(dist, item)
    d = os.path.join(deploy, item)
    if os.path.isdir(s):
        shutil.copytree(s, d)
    else:
        shutil.copy(s, d)
print('deploy copied:', sum(len(fs) for _, _, fs in os.walk(deploy)))
def git(cwd, *args):
    return subprocess.run(['git', '-C', cwd] + list(args), capture_output=True, text=True)
git(r'F:\Vesna-docs', 'add', '-A')
git(r'F:\Vesna-docs', 'commit', '-m', '喵~')
for i in range(4):
    p = git(r'F:\Vesna-docs', 'push', 'origin', 'main')
    if p.returncode == 0:
        print('docs main push OK'); break
    print('docs main push %d failed' % (i+1), p.stderr.strip()[-150:])
    time.sleep(8)
# gh-pages 独立仓重建
tmp = r'F:\Vesna-docs\_ghpages_tmp'
shutil.rmtree(tmp, ignore_errors=True)
os.makedirs(tmp, exist_ok=True)
for item in os.listdir(deploy):
    if item == '.git':
        continue
    s = os.path.join(deploy, item)
    d = os.path.join(tmp, item)
    if os.path.isdir(s):
        shutil.copytree(s, d)
    else:
        shutil.copy(s, d)
git(tmp, 'init', '-b', 'gh-pages')
git(tmp, 'remote', 'add', 'origin', 'https://github.com/youakasakura-YAS/vesna-docs.git')
git(tmp, 'add', '-A')
git(tmp, 'commit', '-m', '喵~')
for i in range(4):
    p = git(tmp, 'push', 'origin', 'gh-pages', '--force')
    if p.returncode == 0:
        print('gh-pages push OK'); break
    print('gh-pages push %d failed' % (i+1), p.stderr.strip()[-150:])
    time.sleep(8)
shutil.rmtree(tmp, ignore_errors=True)
print('ALL DONE')
