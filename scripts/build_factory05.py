#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Select complete original mappings, never thin their regions/velocities.
Requires the verified 0.4 source collections. Payload stays in ignored out/.
"""
from pathlib import Path
import json,re,hashlib,shutil,collections
ROOT=Path(__file__).resolve().parents[1]
out=ROOT/'out/factory05'; out.mkdir(parents=True,exist_ok=True)
required={'vsco':{'Bassoon','Clarinet','French Horn','Oboe','Trombone','Tuba','Concert Flute','Violin Ensemble','Pipe Organ'},'vcsl':{'TX81Z','Harpsichord, English','Vibraphone','Balafon','Kalimba, Kenya','Claps','Hi-Hat Cymbal','Snare Drum, Modern 1','Bass Drum 1'}}
files={}; selected=[]; inventory=[]; groups=[]
for collection in ('vsco','vcsl'):
 folder=ROOT/'out/community04'/collection;pack=json.loads((folder/'pack.json').read_text()); refs={f['path']:f for f in pack['files']};g=collections.defaultdict(list)
 def deps(sf):
  text=(folder/sf).read_text();text=re.sub(r'//[^\n]*','',text);base=Path(sf).parent;ret={sf}
  for key,value in re.findall(r'\b(default_path|sample)\s*=\s*([^\r\n<]*?)(?=\s+[\w]+\s*=|[\r\n<]|$)',text):
   value=value.strip().strip('"').replace('\\','/')
   if key=='default_path':base=Path(sf).parent/value
   elif not value.startswith('*'):
    path=(folder/base/value).resolve().relative_to(folder.resolve()).as_posix()
    if path not in refs:raise ValueError(path)
    ret.add(path)
  return ret
 for i in pack['instruments']:g[i['instrumentId']].append(i)
 for identity,patches in g.items():
  paths=set().union(*(deps(i['file']) for i in patches));size=sum(refs[p]['size'] for p in paths)
  groups.append((collection,patches,paths,size,folder,refs))
# Keep required core orchestra and keys, then the most compact complete instruments.
used=0
for collection,patches,paths,size,folder,refs in sorted(groups,key=lambda x:(x[1][0]['name'] not in required[x[0]],x[3]/len(x[1]))):
 name=patches[0]['name']; mandatory=name in required[collection]
 if name in {'Grand Piano, Kawai','Shaker, Large','Shaker, Small','Tambourine 4','Tambourine 5','Harmonica-Hohner-Special20-F'}:continue
 if not mandatory and (used+size>1300*1024**2 or len(inventory)>=48):continue
 # avoid duplicate recordings/families from overlapping VSCO/VCSL editions
 if any(x['name']==name for x in inventory):continue
 used+=size;inventory.append(dict(name=name,collection=collection,patches=len(patches),bytes=size))
 for path in sorted(paths):
  dest=out/collection/path;dest.parent.mkdir(parents=True,exist_ok=True)
  if not dest.exists():shutil.copyfile(folder/path,dest)
  data=refs[path];files[collection+'/'+path]={**data,'path':collection+'/'+path}
 for i in patches:selected.append({**i,'id':'factory05.'+i['id'],'instrumentId':'factory05.'+i['instrumentId'],'file':collection+'/'+i['file']})
# The three small complete Meta Ogg editions also ship in the factory artifact.
for folder in sorted((ROOT/'out/meta05/systems/instruments').glob('*')):
 if not (folder/'pack.json').exists():continue
 meta=json.loads((folder/'pack.json').read_text());prefix=folder.name
 for f in meta['files']:
  dest=out/prefix/f['path'];dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(folder/f['path'],dest)
  files[prefix+'/'+f['path']]={**f,'path':prefix+'/'+f['path']}
 for i in meta['instruments']:selected.append({**i,'id':'factory05.'+i['id'],'instrumentId':'factory05.'+i['instrumentId'],'file':prefix+'/'+i['file']})
 inventory.append(dict(name=meta['name'],collection='Meta Open Sound',patches=len(meta['instruments']),bytes=meta['installSize']))
licenseText='CC0-1.0. Original recordings: Versilian Studios / Sam Gossner and contributors, Meta and contributors. Source revisions and individual original URLs are preserved in pack.json. Original mappings and every dependent sample retained.\n'
(out/'LICENSE.txt').write_text(licenseText)
(ROOT/'assets/sounds/factory05/LICENSE.txt').write_text(licenseText)
b=licenseText.encode();files['LICENSE.txt']=dict(path='LICENSE.txt',size=len(b),sha256=hashlib.sha256(b).hexdigest())
j=dict(id='quadraj.factory05',name='Ensemble Factory Library',version='1.0.0',author='Versilian Studios, Meta and contributors; selection QuadraJ',source='https://versilian-studios.com/',license='CC0-1.0',installSize=sum(f['size'] for f in files.values()),instruments=selected,files=list(files.values()))
for stale in out.rglob('*'):
 if stale.is_file() and stale.relative_to(out).as_posix() not in files and stale.name!='pack.json':stale.unlink()
(out/'pack.json').write_text(json.dumps(j,indent=2)+'\n')
(ROOT/'assets/sounds/factory05/manifest.json').write_text(json.dumps(j,separators=(',',':'))+'\n')
summary=dict(instruments=len(inventory),patches=len(selected),bytes=j['installSize'],sampleFiles=sum(Path(f).suffix.lower() in ('.wav','.ogg','.mp3') for f in files),selection=inventory)
kinds=set()
for i in inventory:
 name=i['name']
 if name.startswith('Tambourine'):name='Tambourine'
 if name.startswith('Snare Drum') or name=='Legacy Snares':name='Snare Drum'
 kinds.add(name)
summary['conservativeKinds']=len(kinds)
summary['kindPolicy']='Numbered tambourines and snare collections are grouped as one type each; source FX/percussion remain explicitly named. Legacy compact overlap is not added to this count.'
(ROOT/'docs/measurements/0.5/factory-inventory.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps({k:v for k,v in summary.items() if k!='selection'}))
