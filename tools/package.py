"""Builds this mod's release zip and an MO2 mod folder from the repo.

    python tools/package.py                 # dist/<name> <version>.zip
    python tools/package.py --stage DIR     # also write the mod folder into DIR (used by deploy)

The zip holds meta.ini, README.md, PROVENANCE.md and gamedata/ at its root, the layout MO2 and the
PiP archives use. Entries are sorted and carry a fixed timestamp, so the same tree gives the same
zip. Name and version come from meta.ini ([General] modName, version).

--stage must be a new or empty directory outside this checkout, not an ancestor of it and not
overlapping the archive output; anything else is refused before a file is written."""
import argparse
import configparser
import hashlib
import os
import shutil
import sys
import tempfile
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIP_EXT = ('.bak', '.tmp', '.orig', '.rej')
DOCS = ('meta.ini', 'README.md', 'PROVENANCE.md')
ZIP_TIME = (1980, 1, 1, 0, 0, 0)


def mod_files():
    for doc in DOCS:
        if not os.path.isfile(os.path.join(ROOT, doc)):
            raise SystemExit(f'missing {doc}')
    files = list(DOCS)
    for dp, dn, fn in os.walk(os.path.join(ROOT, 'gamedata')):
        dn.sort()
        for f in sorted(fn):
            if f.lower().endswith(SKIP_EXT):
                continue
            files.append(os.path.relpath(os.path.join(dp, f), ROOT).replace(os.sep, '/'))
    return files


def read_meta():
    cp = configparser.ConfigParser(interpolation=None)
    cp.read(os.path.join(ROOT, 'meta.ini'), encoding='utf-8')
    g = cp['General']
    return g.get('modName', os.path.basename(ROOT)).strip('"'), g.get('version', '0').strip('"')


def real(p):
    return os.path.normcase(os.path.realpath(os.path.abspath(p)))


def inside(child, parent):
    return child == parent or child.startswith(parent.rstrip(os.sep) + os.sep)


def check_stage(stage, dist):
    s, root, d = real(stage), real(ROOT), real(dist)
    if os.path.dirname(s) == s:
        return 'is a filesystem root'
    if inside(s, root) or inside(root, s):
        return 'overlaps the checkout'
    if inside(s, d) or inside(d, s):
        return 'overlaps the archive output'
    if os.path.lexists(stage) and (not os.path.isdir(stage) or os.listdir(stage)):
        return 'exists and is not an empty directory'
    return None


def write_zip(zpath, files):
    fd, temporary = tempfile.mkstemp(prefix='.package-', suffix='.zip', dir=os.path.dirname(zpath))
    os.close(fd)
    try:
        with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED) as z:
            for rel in files:
                info = zipfile.ZipInfo(rel, ZIP_TIME)
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = 0o644 << 16
                with open(os.path.join(ROOT, rel), 'rb') as f:
                    z.writestr(info, f.read())
        os.replace(temporary, zpath)
    finally:
        if os.path.exists(temporary):
            os.remove(temporary)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--stage', help='write the mod folder into this new or empty directory')
    ap.add_argument('--dist', default=os.path.join(ROOT, 'dist'))
    a = ap.parse_args()
    if a.stage:
        why = check_stage(a.stage, a.dist)
        if why:
            raise SystemExit(f'refusing --stage {a.stage}: {why}')
    name, version = read_meta()
    files = mod_files()
    os.makedirs(a.dist, exist_ok=True)
    zpath = os.path.join(a.dist, f'{name} {version}.zip')
    write_zip(zpath, files)
    with open(zpath, 'rb') as archive:
        h = hashlib.file_digest(archive, 'sha256').hexdigest()
    print(f'{zpath}  {len(files)} files  sha256 {h}')
    if a.stage:
        for rel in files:
            dst = os.path.join(a.stage, rel)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copy2(os.path.join(ROOT, rel), dst)
        print(f'staged {a.stage}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
