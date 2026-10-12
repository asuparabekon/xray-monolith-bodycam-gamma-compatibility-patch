"""Validate the build output and prepare release assets."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import zipfile

ROOT = Path(__file__).resolve().parents[1]
import configparser
config = configparser.ConfigParser(interpolation=None)
config.read(ROOT / 'meta.ini', encoding='utf-8')
version = config['General']['version'].strip('"')
name = config['General']['modName'].strip('"').replace(' ', '-')
parser = argparse.ArgumentParser()
parser.add_argument('--check-version', action='store_true')
args = parser.parse_args()
if not re.fullmatch(r'\d+\.\d+\.\d+', version):
    raise SystemExit('Version must be major.minor.patch')
if os.environ.get('GITHUB_REF_TYPE') == 'tag' and os.environ.get('GITHUB_REF_NAME') != 'v' + version:
    raise SystemExit('Tag must match the source version: v' + version)
if args.check_version:
    print('Version:', version)
    raise SystemExit(0)
output = ROOT / 'release'
output.mkdir(exist_ok=True)
import importlib.util
import shutil
spec = importlib.util.spec_from_file_location('mod_package', ROOT / 'tools/package.py')
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)
mod_name, _ = package.read_meta()
source = ROOT / 'dist' / f'{mod_name} {version}.zip'
archive = output / f'{name}-{version}.zip'
shutil.copy2(source, archive)
files = {rel: ROOT / rel for rel in package.mod_files()}
if not any(rel.startswith('gamedata/') for rel in files):
    raise SystemExit('MO2 package has no gamedata at its root')
with zipfile.ZipFile(archive) as z:
    if z.testzip() is not None or set(z.namelist()) != set(files) or len(z.namelist()) != len(files):
        raise SystemExit('Archive contents differ from the build')
    for rel, p in files.items():
        if z.read(rel) != p.read_bytes():
            raise SystemExit('Archive bytes differ: ' + rel)
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
(output / 'SHA256SUMS.txt').write_text(f'{digest}  {archive.name}\n', encoding='utf-8')
print(archive.name, digest)
