#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 QuadraJ and contributors.
"""Build a verified, versioned optional release archive. Does not upload/download anything."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile
p = argparse.ArgumentParser()
p.add_argument('pack', type=Path)
p.add_argument('destination', type=Path)
p.add_argument('--base-url', required=True, help='Actual release assets URL prefix chosen by publisher')
a = p.parse_args()
root = a.pack.resolve()
j = json.loads((root / 'pack.json').read_text())
name = f"{j['id']}-{j['version']}.zip"
if any(c in name for c in '/\\') or '..' in name:
    raise SystemExit('Invalid pack identity')
files = []
for entry in j['files']:
    relative = Path(entry['path'])
    if relative.is_absolute() or '..' in relative.parts:
        raise SystemExit('Invalid inventory path')
    source = (root / relative).resolve()
    if not source.is_relative_to(root) or not source.is_file():
        raise SystemExit('Invalid pack payload path')
    data = source.read_bytes()
    if len(data) != entry['size'] or hashlib.sha256(data).hexdigest() != entry['sha256']:
        raise SystemExit(f"Integrity failed: {entry['path']}")
    files.append((entry['path'], data))
if sum(len(data) for _, data in files) != j['installSize']:
    raise SystemExit('Install size mismatch')
a.destination.mkdir(parents=True, exist_ok=True)
target = a.destination / name
if target.exists():
    raise SystemExit('Versioned archive already exists; do not overwrite published versions')
with zipfile.ZipFile(target, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
    for filename, data in sorted(files + [('pack.json', (root / 'pack.json').read_bytes())]):
        info = zipfile.ZipInfo(j['id'] + '/' + filename, date_time=(2026, 1, 1, 0, 0, 0))
        info.compress_type = zipfile.ZIP_DEFLATED
        archive.writestr(info, data)
sha = hashlib.sha256(target.read_bytes()).hexdigest()
record = dict(id=j['id'], version=j['version'], name=j['name'], license=j['license'],
              source=j['source'], downloadSize=target.stat().st_size, installSize=j['installSize'],
              sha256=sha, format='unpacked-sfz-pack-v1', url=a.base_url.rstrip('/') + '/' + name)
(a.destination / (name + '.catalog.json')).write_text(json.dumps(record, indent=2) + '\n')
(a.destination / (name + '.sha256')).write_text(sha + '  ' + name + '\n')
print(target)
