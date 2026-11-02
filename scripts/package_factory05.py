#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Verify and package the existing factory artifact. Never downloads content."""
from pathlib import Path
import json,hashlib,tarfile,gzip
root=Path(__file__).resolve().parents[1];folder=root/'out/factory05';j=json.loads((root/'assets/sounds/factory05/manifest.json').read_text())
if json.loads((folder/'pack.json').read_text())!=j:raise SystemExit('Factory manifest differs from the selected edition')
for f in j['files']:
 p=folder/f['path'];resolved=p.resolve()
 if folder.resolve() not in resolved.parents or p.is_symlink() or p.stat().st_size!=f['size'] or hashlib.sha256(p.read_bytes()).hexdigest()!=f['sha256']:raise SystemExit('Invalid factory file: '+f['path'])
out=root/'out/dist';out.mkdir(parents=True,exist_ok=True);archive=out/'Ensemble-Factory-1.0.0.tar.gz'
with archive.open('wb') as raw, gzip.GzipFile(filename='',fileobj=raw,mode='wb',mtime=0,compresslevel=3) as compressed, tarfile.open(fileobj=compressed,mode='w',format=tarfile.PAX_FORMAT) as tar:
 for name in ['pack.json']+[f['path'] for f in j['files']]:
  p=folder/name;info=tar.gettarinfo(p,arcname='FactoryLibrary/'+name);info.uid=info.gid=0;info.uname=info.gname='';info.mtime=0;info.mode=0o644
  with p.open('rb') as file:tar.addfile(info,file)

h=hashlib.sha256()
with archive.open('rb') as file:
 while b:=file.read(1024*1024):h.update(b)
(archive.with_suffix(archive.suffix+'.sha256')).write_text(h.hexdigest()+'  '+archive.name+'\n');print(archive,archive.stat().st_size,h.hexdigest())
