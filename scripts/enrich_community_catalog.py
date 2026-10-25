#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 QuadraJ and contributors.
"""Enrich already verified local full libraries without any network access."""
import json,re,csv,math
from pathlib import Path
root=Path(__file__).resolve().parents[1]
for collection in ['vsco','vcsl']:
 folder=root/'out/community04'/collection;file=folder/'pack.json'
 if not file.exists():continue
 j=json.loads(file.read_text())
 j['version']=j['upstreamRevision'][:12]+'+ensemble04.1'
 audit=root/'docs/measurements/0.4'/(collection+'-raw.csv')
 measured={r['file']:r for r in csv.DictReader(audit.open())} if audit.exists() else {}
 for i in j['instruments']:
  text=(folder/i['file']).read_text(errors='replace');roots=[int(v) for v in re.findall(r'\b(?:pitch_keycenter|key)\s*=\s*(\d+)',text) if 0<=int(v)<=127]
  roots=sorted(set(roots));i['auditionKey']=min(roots,key=lambda n:abs(n-60)) if roots else 60
  i['midiLow']=min(roots) if roots else 0;i['midiHigh']=max(roots) if roots else 127
  i['sampleCount']=len(set(re.findall(r'\bsample\s*=\s*([^\r\n]+)',text)))
  i['velocityLayers']=len(set(re.findall(r'\blovel\s*=\s*(\d+)',text))) or 1
  if collection=='vcsl' and i['name'] in ['Trumpet','Tenor Trombone','Tuba','French Horn']:i['category']='Brass';i['family']='brass'
  if i['file'] in measured:
   row=measured[i['file']];peak=float(row['peak']);rms=float(row['rms']);target=.045 if i['category']=='Piano & Keys' else .035
   gain=min(4,max(.25,(.3/max(peak,1e-9)) if i['category']=='Drums & Percussion' else target/max(rms,1e-9)),.6/max(peak,1e-9))
   i.update(calibrationVersion=2,calibrationDb=20*math.log10(gain),auditionDb=0,calibrationPolicy='Ensemble 0.4 family target, 3 roots × 3 velocities, +12dB max, -4.44dBFS strong-note headroom; fixed gain only',unsupportedOpcodes=row['unknown_opcodes'])
  i.update(artworkId=i['family'],artworkUri='asset://instruments/'+i['family'],thumbnailAsset='asset://instruments/'+i['family'],artworkLicense='CC0-1.0 (QuadraJ generated illustration rights)',artworkAuthor='QuadraJ / OpenAI image generation',imageAspectRatio=1)
 file.write_text(json.dumps(j,indent=2)+'\n');(root/'assets/sounds/catalog'/(collection+'.json')).write_text(json.dumps(j,separators=(',',':'))+'\n')
 print(collection,len({i['instrumentId'] for i in j['instruments']}),len(j['instruments']))
