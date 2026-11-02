#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Fetch only the manifest-selected factory payload, with explicit consent."""
import argparse,json,hashlib,urllib.request,concurrent.futures,time
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--download',action='store_true');a=p.parse_args()
if not a.download:raise SystemExit('Pass --download to fetch the factory payload (~1.38 GB).')
r=Path(__file__).resolve().parents[1];j=json.loads((r/'assets/sounds/factory05/manifest.json').read_text());out=r/'out/factory05';out.mkdir(parents=True,exist_ok=True)
def get(f):
 dest=out/f['path'];dest.parent.mkdir(parents=True,exist_ok=True)
 if dest.exists() and dest.stat().st_size==f['size'] and hashlib.sha256(dest.read_bytes()).hexdigest()==f['sha256']:return
 if 'url' not in f:
  b=(r/'assets/sounds/factory05'/f['path']).read_bytes()
  if hashlib.sha256(b).hexdigest()!=f['sha256']:raise RuntimeError('Generated factory metadata mismatch')
  dest.write_bytes(b);return
 for n in range(5):
  try:
   b=urllib.request.urlopen(f['url'],timeout=120).read()
   if len(b)!=f['size'] or hashlib.sha256(b).hexdigest()!=f['sha256']:raise RuntimeError('Factory hash mismatch: '+f['path'])
   temp=dest.with_suffix(dest.suffix+'.download');temp.write_bytes(b);temp.replace(dest);return
  except Exception:
   if n==4:raise
   time.sleep(n+1)
with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:list(pool.map(get,j['files']))
(out/'pack.json').write_text(json.dumps(j,indent=2)+'\n');print('Factory payload verified:',j['installSize'],'bytes')
