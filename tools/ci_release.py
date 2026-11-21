"""Validate the release tag, the selected source and the packaged artifact (stdlib only)."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parent.parent
PLUGIN_ID = 'ra9fael.sysinfo'


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT, text=True, encoding='utf-8').strip()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as handle:
        for block in iter(lambda: handle.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def validate_source(event, name):
    manifest = json.loads((ROOT / 'plugin.json').read_text(encoding='utf-8-sig'))
    version = manifest['version']
    if not re.fullmatch(r'\d+\.\d+\.\d+', version):
        raise ValueError('Version must be X.Y.Z')
    if manifest.get('id') != PLUGIN_ID:
        raise ValueError('plugin.json id mismatch')
    if name != 'push':
        raise ValueError('Only a new version tag can trigger a release')
    ref = event.get('ref', '')
    if not event.get('created') or event.get('deleted') or not re.fullmatch(r'refs/tags/\d+\.\d+\.\d+', ref):
        raise ValueError('Only newly created version tags can publish automatically')
    tag = ref.removeprefix('refs/tags/')
    if tag != version:
        raise ValueError(f'Tag {tag} does not match plugin.json version {version}')
    sha = git('rev-parse', 'HEAD')
    tag_ref = f'refs/tags/{tag}'
    if git('rev-parse', f'{tag_ref}^{{commit}}') != sha:
        raise ValueError(f'Tag {tag} does not point at the selected source')
    remote = dict(line.split()[::-1] for line in git('ls-remote', 'origin', tag_ref, f'{tag_ref}^{{}}').splitlines())
    if remote.get(f'{tag_ref}^{{}}', remote.get(tag_ref)) != sha:
        raise ValueError(f'Remote tag {tag} does not point at the selected source')
    print(f'::group::source ok v{version} tag={tag}')
    print('::endgroup::')
    return sha, tag


def validate_package(dist: Path, version: str):
    package = dist / f'{PLUGIN_ID}-{version}.wsplugin'
    if not package.is_file():
        raise ValueError(f'Missing package {package.name}')
    checksum = dist / f'{package.name}.sha256'
    if not checksum.is_file():
        raise ValueError('Missing .sha256 sidecar')
    expected = checksum.read_text(encoding='ascii').split()[0]
    if sha256_file(package) != expected:
        raise ValueError('SHA256 mismatch')
    with zipfile.ZipFile(package) as archive:
        names = set(archive.namelist())
        manifest = json.loads(archive.read('plugin.json'))
    if manifest != json.loads((ROOT / 'plugin.json').read_text(encoding='utf-8-sig')):
        raise ValueError('Package manifest differs from the checked-out source')
    program = str(manifest.get('runtime', {}).get('program', ''))
    for required in ('plugin.json', 'README.md', 'README.en.md', 'licenses/phosphor.txt', program):
        if required not in names:
            raise ValueError(f'Package is missing {required}')
    print(f'::group::package ok {package.name} {package.stat().st_size} bytes')
    print('::endgroup::')


def main():
    dist = Path(sys.argv[1]) if len(sys.argv) > 1 else None
    if dist is None and 'GITHUB_EVENT_PATH' in os.environ:
        event = json.loads(Path(os.environ['GITHUB_EVENT_PATH']).read_text(encoding='utf-8'))
        name = os.environ['GITHUB_EVENT_NAME']
        sha, tag = validate_source(event, name)
        with Path(os.environ['GITHUB_OUTPUT']).open('a', encoding='utf-8') as output:
            output.write(f'sha={sha}\ntag={tag}\n')
        return 0
    manifest = json.loads((ROOT / 'plugin.json').read_text(encoding='utf-8-sig'))
    validate_package(Path(dist or (ROOT / 'dist')), manifest['version'])
    return 0


if __name__ == '__main__':
    sys.exit(main())
