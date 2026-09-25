# -*- coding: utf-8 -*-
"""1.8.0 收尾：vsix + release + 备份 + 推送 + docs 同步"""
import shutil, glob, os, datetime, subprocess, time

# ---- 1) vsix 0.8.0 ----
vsix_dir = r'F:\Vesna\vesna-vscode'
r = subprocess.run(['npx', '--no-install', '@vscode/vsce', 'package', '--out', 'vesna-0.8.0.vsix'],
                   cwd=vsix_dir, capture_output=True, text=True, shell=True)
print('vsce rc=%d' % r.returncode, (r.stdout + r.stderr)[-120:].replace('\n', ' | '))
for old in glob.glob(os.path.join(vsix_dir, 'vesna-0.7.0.vsix')):
    os.remove(old)
print('old vsix removed')

# ---- 2) release 1.8.0 ----
src_rel = r'F:\Vesna\release\vesna-1.7.0-windows-x64'
dst = r'F:\Vesna\release\vesna-1.8.0-windows-x64'
shutil.rmtree(dst, ignore_errors=True)
shutil.copytree(src_rel, dst)
shutil.copy(r'F:\Vesna\bin\vesna.exe', os.path.join(dst, 'bin', 'vesna.exe'))
for f in glob.glob(r'F:\Vesna\lib\*.ves'):
    shutil.copy(f, os.path.join(dst, 'lib', os.path.basename(f)))
for f in ['builtins.md', 'builtins.zh-CN.md', 'syntax.md', 'syntax.zh-CN.md']:
    shutil.copy(os.path.join(r'F:\Vesna\docs', f), os.path.join(dst, 'docs', f))
shutil.rmtree(os.path.join(dst, 'examples'), ignore_errors=True)
shutil.copytree(r'F:\Vesna\examples', os.path.join(dst, 'examples'))
for f in ['README.md', 'README.zh-CN.md', 'CHANGELOG.md', 'CHANGELOG.zh-CN.md',
          'CONTRIBUTING.md', 'CONTRIBUTING.zh-CN.md', 'LICENSE', 'VERSION']:
    s = os.path.join(r'F:\Vesna', f)
    if os.path.exists(s):
        shutil.copy(s, os.path.join(dst, f))
shutil.copy(r'F:\Vesna\RELEASE-NOTES-1.8.0.md', os.path.join(dst, 'RELEASE-NOTES-1.8.0.md'))
if os.path.exists(os.path.join(dst, 'RELEASE-NOTES-1.7.0.md')):
    os.remove(os.path.join(dst, 'RELEASE-NOTES-1.7.0.md'))
shutil.copy(os.path.join(vsix_dir, 'vesna-0.8.0.vsix'), os.path.join(dst, 'vesna-0.8.0.vsix'))
if os.path.exists(os.path.join(dst, 'vesna-0.7.0.vsix')):
    os.remove(os.path.join(dst, 'vesna-0.7.0.vsix'))
zipf = r'F:\Vesna\release\vesna-1.8.0-windows-x64.zip'
if os.path.exists(zipf):
    os.remove(zipf)
import zipfile
with zipfile.ZipFile(zipf, 'w', zipfile.ZIP_DEFLATED) as z:
    for root, _, files in os.walk(dst):
        for f in files:
            full = os.path.join(root, f)
            z.write(full, os.path.relpath(full, dst))
r = subprocess.run([os.path.join(dst, 'bin', 'vesna.exe'), '--version'], capture_output=True, timeout=20)
print('release version:', r.stdout.decode('utf-8', 'ignore').strip())
print('release files:', sum(len(fs) for _, _, fs in os.walk(dst)), 'zip bytes:', os.path.getsize(zipf))

# ---- 3) 备份 ----
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
          'RELEASE-NOTES-1.8.0.md']:
    s = os.path.join(r'F:\Vesna', f)
    if os.path.exists(s):
        shutil.copy(s, os.path.join(snap, f))
snaps = sorted(glob.glob(os.path.join(bk_root, 'snap-*')))
for old in snaps[:-3]:
    shutil.rmtree(old, ignore_errors=True)
    print('removed old snap:', os.path.basename(old))
print('kept snaps:', [os.path.basename(x) for x in sorted(glob.glob(os.path.join(bk_root, 'snap-*')))])

# ---- 4) 主仓库提交推送 ----
repo = r'F:\Vesna'
r = subprocess.run(['git', '-C', repo, 'status', '--porcelain'], capture_output=True, text=True)
print('--- git status ---')
print(r.stdout.strip()[:600])
if r.stdout.strip():
    subprocess.run(['git', '-C', repo, 'add', '-A'], check=True)
    subprocess.run(['git', '-C', repo, 'commit', '-m', '喵~'], check=True)
    for i in range(4):
        p = subprocess.run(['git', '-C', repo, 'push', 'origin', 'main'], capture_output=True, text=True)
        if p.returncode == 0:
            print('main push OK'); break
        print('push %d failed' % (i+1), p.stderr.strip()[-150:])
        time.sleep(8)

# ---- 5) docs 站同步 ----
for s, d in [(r'F:\Vesna\CHANGELOG.md', r'F:\Vesna-docs\docs\en\guide\changelog.md'),
             (r'F:\Vesna\CHANGELOG.zh-CN.md', r'F:\Vesna-docs\docs\guide\changelog.md'),
             (r'F:\Vesna\docs\builtins.md', r'F:\Vesna-docs\docs\en\guide\builtins.md'),
             (r'F:\Vesna\docs\builtins.zh-CN.md', r'F:\Vesna-docs\docs\guide\builtins.md')]:
    shutil.copy(s, d)
print('docs sources synced')
r = subprocess.run(['npm', 'run', 'docs:build'], cwd=r'F:\Vesna-docs', capture_output=True, text=True, shell=True)
print('docs build rc=%d' % r.returncode)
if r.returncode != 0:
    print(r.stdout[-300:]); print(r.stderr[-300:]); raise SystemExit('build failed')
deploy = r'F:\Vesna-docs\.deploy'
for item in os.listdir(deploy):
    if item == '.git':
        continue
    p2 = os.path.join(deploy, item)
    if os.path.isdir(p2):
        shutil.rmtree(p2, ignore_errors=True)
    else:
        os.remove(p2)
dist = r'F:\Vesna-docs\docs\.vitepress\dist'
for item in os.listdir(dist):
    s = os.path.join(dist, item)
    d = os.path.join(deploy, item)
    if os.path.isdir(s):
        shutil.copytree(s, d)
    else:
        shutil.copy(s, d)
print('deploy copied')
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
