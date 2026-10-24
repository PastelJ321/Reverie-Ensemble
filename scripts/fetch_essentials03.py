#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 QuadraJ and contributors.
# Developer-only provenance reproduction. Never invoked by the application/build.
import sys
if '--download' not in sys.argv:
    raise SystemExit('Explicit download required: python3 scripts/fetch_essentials03.py --download (about 40 MiB)')
import json,urllib.request,urllib.parse,re,hashlib
from pathlib import Path
out=Path('assets/sounds/essentials03');(out/'samples').mkdir(parents=True,exist_ok=True)
existing=out/'pack.json'
expected={f.get('sourceUrl'):f['sha256'] for f in json.loads(existing.read_text())['files'] if f.get('sourceUrl')} if existing.exists() else {}
files=[];instruments=[]
def fetch(repo,rev,path,local):
 url='https://raw.githubusercontent.com/'+repo+'/'+rev+'/'+urllib.parse.quote(path)
 dest=out/local
 if not dest.exists():dest.write_bytes(urllib.request.urlopen(url,timeout=60).read())
 data=dest.read_bytes()
 if url in expected and hashlib.sha256(data).hexdigest()!=expected[url]:raise RuntimeError('Existing raw sample hash mismatch: '+local)
 files.append(dict(path=local,size=len(data),sha256=hashlib.sha256(data).hexdigest(),sourceUrl=url,sourcePath=path,repository=repo,revision=rev));return local

def patch(id,name,category,family,rows,release=.25):
 text='<global> ampeg_attack=0.002 ampeg_decay=0.2 ampeg_sustain=100 ampeg_release='+str(release)+'\n'
 for row in rows:text+='<region> '+' '.join(str(k)+'='+str(v) for k,v in row.items())+'\n'
 local=id+'.sfz';(out/local).write_text(text);data=text.encode();files.append(dict(path=local,size=len(data),sha256=hashlib.sha256(data).hexdigest()))
 instruments.append(dict(id='quadraj.essentials.'+id,name=name,category=category,family=family,instrumentId='quadraj.instrument.'+id,patchId=id+'.natural',patchName='Natural',articulation='Sustain' if family!='drums' else 'GM one-shots',tags='recorded '+family,format='sfz',file=local))
repo='sfzinstruments/karoryfer.fashionbass';rev='0703973b300a6bacb837602be9b588ab64732a72';rows=[]
for ni,(note,root) in enumerate([('ab1',32),('ab2',44),('ab3',56)]):
 for vel,lv,hv in [('p',0,78),('f',79,127)]:
  for rr in [1,2]:
   path='notes/'+note+'_'+vel+'_rr'+str(rr)+'.wav';local=fetch(repo,rev,path,'samples/bass-'+note+'-'+vel+'-'+str(rr)+'.wav');rows.append(dict(sample=local,lokey=[12,39,51][ni],hikey=[38,50,96][ni],pitch_keycenter=root,lovel=lv,hivel=hv,seq_length=2,seq_position=rr))
fetch(repo,rev,'LICENSE','Fashionbass-CC0.txt');fetch(repo,rev,'readme.txt','Fashionbass-readme.txt');patch('electric-bass','Electric Bass','Guitar & Bass','bass',rows,.18)
repo='sfzinstruments/karoryfer.black-and-green-guitars';rev='b3b3249d37dc977a1a297bd2dc053e6d9b6b805c';rows=[]
# Original mapping uses C3=48 (MIDI convention C4=60).
for ni,(note,root) in enumerate([('e3',40),('c4',48),('c5',60),('c6',72)]):
 for vel,lv,hv in [('p',0,78),('f',79,127)]:
  for rr in [1,2]:
   path='Samples/black/ord/twang_'+note+'_'+vel+'_rr'+str(rr)+'.wav';local=fetch(repo,rev,path,'samples/guitar-'+note+'-'+vel+'-'+str(rr)+'.wav');rows.append(dict(sample=local,lokey=[28,45,55,67][ni],hikey=[44,54,66,96][ni],pitch_keycenter=root,lovel=lv,hivel=hv,seq_length=2,seq_position=rr))
fetch(repo,rev,'LICENSE','Guitar-CC0.txt');fetch(repo,rev,'readme.txt','Guitar-readme.txt');mapping=urllib.request.urlopen('https://raw.githubusercontent.com/'+repo+'/'+rev+'/Programs/modules/maps_black/ord.sfz').read().decode();assert 'pitch_keycenter=40' in mapping
patch('electric-guitar','Hollowbody Electric Guitar','Guitar & Bass','guitar',rows,.22)
repo='sgossner/VCSL';rev='c1ea7bcc3c7309650ab0da9d15c9cd1fbc4a4c7e';tree=json.load(urllib.request.urlopen('https://api.github.com/repos/sgossner/VCSL/git/trees/'+rev+'?recursive=1'));paths=[t['path'] for t in tree['tree'] if t['type']=='blob'];rows=[]
for ni,(note,root) in enumerate([('C2',48),('C4',72),('C6',96)]):
 for vel,lv,hv in [(1,0,78),(3,79,127)]:
  path='Chordophones/Zithers/Grand Piano, Kawai/Sustains/GPiano_sus_'+note+'_v'+str(vel)+'_rr1_Player.wav';local=fetch(repo,rev,path,'samples/grand-'+note+'-'+str(vel)+'.wav');rows.append(dict(sample=local,lokey=[21,61,85][ni],hikey=[60,84,108][ni],pitch_keycenter=root,lovel=lv,hivel=hv))
patch('grand','Concert Grand Piano','Piano & Keys','grand',rows,.4)
rows=[]
patterns=[(36,'Bass Drum 1/BDrumNew_hit_v2_rr'),(38,'Snare Drum, Modern 3/Snare4_HitSN_v2_rr'),(42,'Hi-Hat Cymbal/HiHat_Close_rr'),(46,'Hi-Hat Cymbal/HiHat_HitO_rr'),(48,'Tom 1/Stick/TomH_HitS_v2_rr'),(45,'Tom 2/Stick/TomL_HitS_v2_rr')]
for key,part in patterns:
 for rr in [1,2]:
  hits=[p for p in paths if part+str(rr)+'_' in p and p.endswith('.wav')];assert hits,(key,part)
  path=hits[0];local=fetch(repo,rev,path,'samples/drum-'+str(key)+'-'+str(rr)+'.wav');rows.append(dict(sample=local,key=key,loop_mode='one_shot',seq_length=2,seq_position=rr))
patch('acoustic-drums','Acoustic Drum Kit','Drums & Percussion','drums',rows,.08)
fetch(repo,rev,'README.md','VCSL-readme.md')
# Staccato is a distinct recorded articulation of the same guitar instrument.
repo='sfzinstruments/karoryfer.black-and-green-guitars';rev='b3b3249d37dc977a1a297bd2dc053e6d9b6b805c';rows=[]
for ni,(note,root) in enumerate([('e3',40),('c4',48),('c5',60),('c6',72)]):
 path='Samples/black/stac/staccato_'+note+'_rr1.wav';local=fetch(repo,rev,path,'samples/guitar-staccato-'+note+'.wav');rows.append(dict(sample=local,lokey=[28,45,55,67][ni],hikey=[44,54,66,96][ni],pitch_keycenter=root))
patch('guitar-staccato','Hollowbody Electric Guitar','Guitar & Bass','guitar',rows,.06)
instruments[-1].update(instrumentId='quadraj.instrument.electric-guitar',patchId='electric-guitar.staccato',patchName='Staccato',articulation='Staccato')
cc0=Path('assets/sounds/factory/CC0-1.0.txt').read_bytes();(out/'CC0-1.0.txt').write_bytes(cc0);files.append(dict(path='CC0-1.0.txt',size=len(cc0),sha256=hashlib.sha256(cc0).hexdigest()))
m=dict(id='quadraj.ensemble-essentials',version='1.0.0',name='Ensemble Essentials',author='Karoryfer Samples / D. Smolken; Versilian Studios / contributors; QuadraJ selection and mapping',source='https://shop.karoryfer.com/pages/free-samples',license='CC0-1.0',installSize=sum(f['size'] for f in files),sha256=hashlib.sha256(''.join(f['sha256'] for f in files).encode()).hexdigest(),instruments=instruments,files=files)
# Rerun reverie-calibrate --apply after regenerating the pack.
# Source SFZ gain corrections for the quiet Kawai velocity-1 recordings.
grand=out/'grand.sfz';text=grand.read_text()
for note,vel,db,offset in [('C2',1,30.304520,13),('C2',3,7.765060,0),('C4',1,36.000888,0),('C4',3,8.582979,0),('C6',1,36.458918,0),('C6',3,2.975836,0)]:
 text=text.replace('sample=samples/grand-'+note+'-'+str(vel)+'.wav','sample=samples/grand-'+note+'-'+str(vel)+'.wav volume='+str(db)+' offset='+str(offset))
grand.write_text(text)
for f in files:
 if f['path']=='grand.sfz':f.update(size=grand.stat().st_size,sha256=hashlib.sha256(grand.read_bytes()).hexdigest())
m['instruments'][2]['rootMappingSource']='https://github.com/sgossner/VCSL/blob/dfcf4a4918771eee884b96ad4493de82ef84daf6/Chordophones/Zithers/Grand%20Piano,%20Kawai.sfz'
m['installSize']=sum(f['size'] for f in files);m['sha256']=hashlib.sha256(''.join(f['sha256'] for f in files).encode()).hexdigest()
(out/'pack.json').write_text(json.dumps(m,indent=2)+'\n');print(len(files),m['installSize'],len(instruments))
