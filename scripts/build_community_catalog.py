#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 QuadraJ and contributors.
"""Reproduce original complete SFZ collections, preserving every upstream patch/sample.
Explicit --download required (~9.4 GB for both). Never run by application/build.
Hashes verified against pinned Git blobs; resulting SHA-256 catalogs drive native installer.
"""
import argparse, concurrent.futures, hashlib, json, re, time, urllib.request, urllib.parse
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--download',action='store_true');p.add_argument('--collection',choices=['vsco','vcsl','all'],default='all');a=p.parse_args()
if not a.download:raise SystemExit('Use --download to consent to full source library downloads (~9.4 GB).')
ROOT=Path(__file__).resolve().parents[1]
SPECS={'vsco':('sgossner/VSCO-2-CE','6dd651d55dde97fd4028699be9d4481f26917891','VSCO 2 Community Edition','https://versilian-studios.com/vsco-community/'),'vcsl':('sgossner/VCSL','dfcf4a4918771eee884b96ad4493de82ef84daf6','Versilian Community Sample Library','https://versilian-studios.com/vcsl/')}
def fetch(url):
 for attempt in range(5):
  try:
   with urllib.request.urlopen(url,timeout=120) as r:return r.read()
  except Exception:
   if attempt==4:raise
   time.sleep(1+attempt)
def classify(path,collection):
 stem=Path(path).stem
 if collection=='vsco':
  groups=[('Bassoon','Bassoon','Woodwinds','wind'),('CelloEns','Cello Ensemble','Strings','cello'),('Clarinet','Clarinet','Woodwinds','wind'),('Contrabass','Contrabass','Strings','bass'),('FHorn','French Horn','Brass','brass'),('Flute','Concert Flute','Woodwinds','wind'),('GM-StylePerc','GM Percussion','Drums & Percussion','drums'),('Glockenspiel','Glockenspiel','Drums & Percussion','mallet'),('Harp','Concert Harp','Orchestral','harp'),('Marimba','Marimba','Drums & Percussion','mallet'),('Oboe','Oboe','Woodwinds','wind'),('Organ','Pipe Organ','Piano & Keys','synth'),('Piccolo','Piccolo','Woodwinds','wind'),('SViolin','Solo Violin','Strings','strings'),('Timpani','Timpani','Drums & Percussion','drums'),('Trombone','Trombone','Brass','brass'),('Trumpet','Trumpet','Brass','brass'),('Tuba','Tuba','Brass','brass'),('TubularBells','Tubular Bells','Drums & Percussion','mallet'),('VSUpright1','Yamaha Upright','Piano & Keys','upright'),('UprightPiano','Knight Upright','Piano & Keys','upright'),('ViolaEns','Viola Ensemble','Strings','strings'),('ViolinEns','Violin Ensemble','Strings','strings'),('Xylophone','Xylophone','Drums & Percussion','mallet')]
  for prefix,name,category,family in groups:
   if stem.startswith(prefix):return name,stem[len(prefix):].strip('- ') or 'Natural',category,family
  raise RuntimeError('Unknown VSCO mapping '+path)
 name=stem.split(' - ')[0];patch=stem.split(' - ',1)[1] if ' - ' in stem else 'Natural'
 if path.startswith('Aerophones'):
  if 'Organ' in name:return name,patch,'Piano & Keys','synth'
  return name,patch,'Woodwinds','wind'
 if path.startswith('Chordophones'):
  if any(v in name for v in ['Piano','Harpsichord']):return name,patch,'Piano & Keys','upright' if 'Upright' in name else 'grand'
  return name,patch,'World Instruments','harp' if 'Harp' in name else 'acoustic-guitar'
 if path.startswith('Electrophones'):return name,patch,'Piano & Keys','synth'
 return name,patch,'Drums & Percussion','drums' if path.startswith('Membranophones') else 'mallet'
for collection in ([a.collection] if a.collection!='all' else SPECS):
 repo,rev,title,source=SPECS[collection];out=ROOT/'out'/'community04'/collection;out.mkdir(parents=True,exist_ok=True)
 treeFile=ROOT/'out'/'inventory'/(collection+'-sfz-tree.json')
 tree=json.loads(treeFile.read_text()) if treeFile.exists() else json.loads(fetch(f'https://api.github.com/repos/{repo}/git/trees/{rev}?recursive=1'))
 selected=[x for x in tree['tree'] if x['type']=='blob' and (x['path'].lower().endswith(('.sfz','.wav','.flac','.aiff','.aif','.ogg','.mp3','.txt','.md')) or Path(x['path']).name.upper() in ['LICENSE','COPYING'])]
 # Graphics/editor files are not audio dependencies. Every SFZ and every audio recording is retained.
 print(collection,'files',len(selected),'bytes',sum(x.get('size',0) for x in selected),flush=True)
 def download(x):
  sourcePath=x['path'];path=sourcePath.replace('Chordophones/Zithers/Harpsichord, Italian/Psaltery, Bowed and Plucked/','Chordophones/Zithers/Psaltery, Bowed and Plucked/') if collection=='vcsl' else sourcePath;url=f'https://raw.githubusercontent.com/{repo}/{rev}/'+urllib.parse.quote(sourcePath);dest=out/path;dest.parent.mkdir(parents=True,exist_ok=True)
  if not dest.exists() or dest.stat().st_size!=x['size']:
   data=fetch(url)
   if len(data)!=x['size'] or hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()!=x['sha']:raise RuntimeError('Pinned blob mismatch '+path)
   temp=dest.with_name(dest.name+'.download');temp.write_bytes(data);temp.replace(dest)
  h=hashlib.sha256();git=hashlib.sha1(b'blob '+str(dest.stat().st_size).encode()+b'\0')
  with dest.open('rb') as f:
   while chunk:=f.read(1024*1024):h.update(chunk);git.update(chunk)
  if git.hexdigest()!=x['sha']:raise RuntimeError('Cached blob mismatch '+path)
  return dict(path=path,size=x['size'],sha256=h.hexdigest(),url=url,gitBlob=x['sha'],sourcePath=sourcePath)
 files=[]
 with concurrent.futures.ThreadPoolExecutor(max_workers=12) as pool:
  for i,f in enumerate(pool.map(download,selected)):
   files.append(f)
   if i%100==0:print(collection,i,'/',len(selected),flush=True)
 instruments=[]
 for f in files:
  if not f['path'].lower().endswith('.sfz'):continue
  name,patch,category,family=classify(f['path'],collection);slug=re.sub('[^a-z0-9]+','-',name.lower()).strip('-');ident='org.versilian.'+collection+'.'+slug
  text=(out/f['path']).read_text(errors='replace');vel=sorted(set(re.findall(r'lovel\s*=\s*(\d+)',text)))
  instruments.append(dict(id=ident+'.'+hashlib.sha256(f['path'].encode()).hexdigest()[:12],instrumentId=ident,patchId=f['path'],name=name,patchName=patch,category=category,family=family,articulation=patch,format='sfizz',file=f['path'],tags='original complete recorded '+collection,regionCount=len(re.findall('<region>',text)),velocityLowValues=[int(v) for v in vel]))
 manifest=dict(id='org.versilian.'+collection,version=rev[:12],name=title,author='Versilian Studios / Sam Gossner and credited contributors; original SFZ mappings',source=source,license='CC0-1.0',upstreamRepository=repo,upstreamRevision=rev,installSize=sum(f['size'] for f in files),files=files,instruments=instruments,mappingCorrections=(["Relocated accidentally nested Psaltery samples to match original SFZ paths; bytes preserved"] if collection=="vcsl" else []))
 (out/'pack.json').write_text(json.dumps(manifest,indent=2)+'\n');catalog=ROOT/'assets'/'sounds'/'catalog';catalog.mkdir(exist_ok=True);(catalog/(collection+'.json')).write_text(json.dumps(manifest,separators=(',',':'))+'\n')
 print(collection,'DONE',len({x['instrumentId'] for x in instruments}),'instruments',len(instruments),'patches',manifest['installSize'],'bytes',flush=True)
