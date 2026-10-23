#!/usr/bin/env python3
"""Explicit developer-only fetch of a pinned CC0 VSCO subset; never run by app/build."""
import json, re, hashlib, urllib.request, urllib.parse
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
REV='440300901dfe9275fd84e0b7763af1f8443ae62e'
MAPPING_REV='6dd651d55dde97fd4028699be9d4481f26917891'
MAPPINGS=dict(upright='VSUpright1.sfz',violin='SViolinVib.sfz',strings='ViolinEnsSusVib.sfz',cello='CelloEnsSusVib.sfz',bass='ContrabassPizz.sfz',flute='FluteExpVib.sfz',trumpet='TrumpetSus.sfz',marimba='Marimba.sfz')
ROOT=Path(__file__).resolve().parents[1]/'assets/sounds/factory'
with urllib.request.urlopen('https://api.github.com/repos/sgossner/VSCO-2-CE/git/trees/'+REV+'?recursive=1') as response:
    TREE=json.load(response)['tree']
W=[x for x in TREE if x['path'].endswith('.wav')]
def choose(prefix,pattern,count=3):
    a=[x for x in W if x['path'].startswith(prefix) and re.search(pattern,x['path'])]
    if not a: raise RuntimeError((prefix,pattern))
    # Spread original recordings across the available pitch range, never duplicate one WAV as different instruments.
    a.sort(key=lambda x:note(x['path']))
    return [a[round(i*(len(a)-1)/(count-1))] for i in range(count)] if count>1 else [min(a,key=lambda x:x['size'])]
def note(path):
    m=re.search(r'_([A-G])(#?)(-?\d)(?:_|\.)',path)
    if not m:raise RuntimeError(path)
    return (int(m[3])+1)*12+{'C':0,'D':2,'E':4,'F':5,'G':7,'A':9,'B':11}[m[1]]+bool(m[2])
spec=[('upright','Upright Piano','Piano & Keys','Keys/Upright Nr1/',r'_mf_RR[12]\.wav$'),
('violin','Solo Violin','Strings','Strings/Solo Violin/Arco Vib/',r'_f\.wav$'),
('strings','Violin Ensemble','Orchestral','Strings/Violin Section/susVib/',r'_v1\.wav$'),
('cello','Cello Ensemble','Strings','Strings/Cello Section/susvib/',r'_v1_1\.wav$'),
('bass','Pizzicato Contrabass','Guitar & Bass','Strings/Solo Contrabass/Pizz/',r'_v1_rr1\.wav$'),
('flute','Concert Flute','Woodwinds','Woodwinds/Flute/expvib/',r'_v1_1\.wav$'),
('trumpet','Concert Trumpet','Brass','Brass/Trumpet/sus/',r'_v3_rr1\.wav$'),
('marimba','Marimba','Melodic Percussion','Percussion/Marimba/',r'\.wav$')]
files=[];instruments=[];jobs=[]
for ident,name,category,prefix,pattern in spec:
    samples=choose(prefix,pattern,4);lines=['<global> ampeg_attack=0.003 ampeg_release=0.25 ampeg_sustain=100']
    keys=[note(x['path'])+(0 if ident in ('upright','violin') else 12) for x in samples]
    for i,(x,key) in enumerate(zip(samples,keys)):
        low=0 if i==0 else (keys[i-1]+key)//2+1
        high=127 if i==len(keys)-1 else (key+keys[i+1])//2
        dest=f'samples/{ident}-{note(x["path"])}.wav';jobs.append((x,dest));lines.append(f'<region> sample={dest} lokey={low} hikey={high} pitch_keycenter={key}')
    ROOT.mkdir(parents=True,exist_ok=True);mapping=ROOT/(ident+'.sfz');mapping.write_text('\n'.join(lines)+'\n');instruments.append(dict(id='quadraj.cc0.'+ident,name=name,category=category,tags='acoustic recorded multisample '+name.lower(),format='sfz',file=mapping.name))
# Three independently recorded percussion samples, mapped to GM notes.
drum=[]
for key,prefix,pattern in [(36,'Percussion/',r'BDrumNewhit_v3_rr1_Sum.wav$'),(38,'VSCO 1 Percussion/drums/snare/OldSnare/',r'/snare_f.wav$'),(42,'VSCO 1 Percussion/',r'(?i)(hihat|hi.hat).*\.wav$')]:
    matches=[x for x in W if x['path'].startswith(prefix) and re.search(pattern,x['path'])]
    if not matches and key==42:matches=[x for x in W if x['path'].startswith('Percussion/') and re.search(r'susCymb1-hitstick_mp_rr1.wav$',x['path'])]
    x=min(matches,key=lambda x:x['size']);dest=f'samples/drums-{key}.wav';jobs.append((x,dest));drum.append(f'<region> sample={dest} key={key} loop_mode=one_shot')
(ROOT/'percussion.sfz').write_text('<global> ampeg_attack=0.001 ampeg_release=0.1\n'+'\n'.join(drum)+'\n');instruments.append(dict(id='quadraj.cc0.percussion',name='Recorded Percussion Kit',category='Drums & Percussion',tags='recorded bass drum snare cymbal GM',format='sfz',file='percussion.sfz'))
def download(job):
    x,dest=job;p=ROOT/dest;p.parent.mkdir(parents=True,exist_ok=True)
    url='https://raw.githubusercontent.com/sgossner/VSCO-2-CE/'+REV+'/'+urllib.parse.quote(x['path'])
    if not p.exists():
        with urllib.request.urlopen(url,timeout=90) as f:data=f.read()
        if len(data)!=x['size']:raise RuntimeError('Size mismatch '+dest)
        p.write_bytes(data)
    return dict(path=dest,size=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest(),source=url,original=x['path'])
with ThreadPoolExecutor(max_workers=4) as pool:files=list(pool.map(download,jobs))
with urllib.request.urlopen('https://raw.githubusercontent.com/sgossner/VSCO-2-CE/'+REV+'/LICENSE') as f:(ROOT/'CC0-1.0.txt').write_bytes(f.read())
for p in sorted(ROOT.glob('*.sfz')):
    files.append(dict(path=p.name,size=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
p=ROOT/'CC0-1.0.txt';files.append(dict(path=p.name,size=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
for instrument in instruments:
    ident=instrument['file'].removesuffix('.sfz')
    if ident in MAPPINGS:instrument['rootMappingSource']=f'https://github.com/sgossner/VSCO-2-CE/blob/{MAPPING_REV}/{MAPPINGS[ident]}'
manifest=dict(id='quadraj.community-essentials',version='1.0.0',name='Community Essentials',author='Versilian Studios: Sam Gossner; Ivy Audio: Simon Dalzell; sample editing: Elan Hickler / Soundemote; QuadraJ mappings',source='https://versilian-studios.com/vsco-community/',license='CC0-1.0',upstreamRevision=REV,installSize=sum(x['size'] for x in files),sha256=hashlib.sha256(''.join(x['sha256'] for x in files).encode()).hexdigest(),instruments=instruments,files=files)
(ROOT/'pack.json').write_text(json.dumps(manifest,indent=2)+'\n');print(len(instruments),'instruments,',manifest['installSize'],'bytes,',len(jobs),'original WAVs')
