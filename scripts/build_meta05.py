#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Explicit acquisition of pinned Meta instrument Ogg/SFZ source, no SFX corpus."""
from pathlib import Path
import json,urllib.request,urllib.parse,hashlib,concurrent.futures,time
import argparse
p=argparse.ArgumentParser();p.add_argument('--download',action='store_true');a=p.parse_args()
if not a.download:raise SystemExit('Pass --download to acquire all three complete Meta instrument collections (~92 MB).')
ROOT=Path(__file__).resolve().parents[1];rev='251c0be57d5d1b3242b91f53e0cde62ab1cb42d9';treeFile=ROOT/'out/meta-tree05.json';treeFile.parent.mkdir(parents=True,exist_ok=True)
if not treeFile.exists():treeFile.write_bytes(urllib.request.urlopen('https://api.github.com/repos/facebookincubator/Meta-open-sound/git/trees/'+rev+'?recursive=1').read())
tree=json.loads(treeFile.read_text())
if tree['sha']!=rev:raise SystemExit('Unexpected Meta source revision')
def fetch(x):
 url='https://raw.githubusercontent.com/facebookincubator/Meta-open-sound/'+rev+'/'+urllib.parse.quote(x['path'])
 dest=ROOT/'out/meta05'/x['path'];dest.parent.mkdir(parents=True,exist_ok=True)
 for n in range(5):
  try:
   b=dest.read_bytes() if dest.exists() else urllib.request.urlopen(url,timeout=60).read()
   if hashlib.sha1(b'blob '+str(len(b)).encode()+b'\0'+b).hexdigest()!=x['sha']:raise ValueError('Git blob mismatch')
   dest.write_bytes(b);return x,b,url
  except Exception:
   if n==4:raise
   time.sleep(n+1)
for key in ('meta-piano','meta-nylon-guitar','meta-steel-guitar'):
 prefix='systems/instruments/'+key+'/'
 rows=[x for x in tree['tree'] if x['type']=='blob' and x['path'].startswith(prefix)]
 results=list(concurrent.futures.ThreadPoolExecutor(max_workers=8).map(fetch,rows));folder=ROOT/'out/meta05'/prefix
 files=[dict(path=x['path'][len(prefix):],size=len(b),sha256=hashlib.sha256(b).hexdigest(),url=url,gitBlob=x['sha']) for x,b,url in results]
 sfz=folder/(key+'.sfz');text=sfz.read_text();print(key,len(rows),sum(len(b) for x,b,url in results),'macro', '#define' in text,flush=True)
 mapped=key+'.sfz'
 if key!='meta-piano':
  import re
  # sfizz ignores lochan/hichan: retain only the exact original channel-one
  # automatic first-position string mapping; never layer the seven masters.
  start=text.index('<master>');end=text.find('<master>',start+8)
  derived=re.sub(r'\b(?:lochan|hichan|amp_random)=[^\s]+','',text[:start]+text[start:end if end!=-1 else len(text)])
  mapped=key+'-ensemble.sfz';derived='// Ensemble 0.5: channel-one first-position edition for sfizz.\n// Original full multichannel map remains in '+key+'.sfz; channels 2-7 are not supported by this edition.\n'+derived;b=derived.encode();(folder/mapped).write_bytes(b)
  asset=ROOT/'assets/sounds/mappings05'/mapped;asset.parent.mkdir(parents=True,exist_ok=True);asset.write_bytes(b)
  files.append(dict(path=mapped,size=len(b),sha256=hashlib.sha256(b).hexdigest(),url='https://raw.githubusercontent.com/PastelJ321/Reverie-Ensemble/43dce87a31132aa0ba8b799d8f61ed6abaf88d41/assets/sounds/mappings05/'+mapped))
 i=dict(id=key,instrumentId=key,patchId=key+'.original',name=key.replace('-',' ').title(),patchName='Original',category='Piano & Keys' if key=='meta-piano' else 'Guitar & Bass',family='grand' if key=='meta-piano' else 'acoustic-guitar',format='sfizz',backend='sfizz',file=mapped,source='https://github.com/facebookincubator/Meta-open-sound/tree/'+rev+'/'+prefix,articulation='Original four velocity layers' if key=='meta-piano' else 'First-position mapping; original velocity layers',sampleCount=sum(f['path'].endswith('.ogg') for f in files),velocityLayers=4 if key=='meta-piano' else 3)
 cal=ROOT/'assets/sounds/meta05-calibration.json'
 if cal.exists():i.update(json.loads(cal.read_text()).get(i['name'],{}))
 j=dict(id=key,name=i['name'],version=rev[:12]+'+ensemble05.1',source=i['source'],author='Meta and contributors',license='CC0-1.0',installSize=sum(f['size'] for f in files),instruments=[i],files=files,upstreamRevision=rev)
 (folder/'pack.json').write_text(json.dumps(j,indent=2)+'\n');(ROOT/'assets/sounds/catalog'/(key+'.json')).write_text(json.dumps(j,separators=(',',':'))+'\n')
