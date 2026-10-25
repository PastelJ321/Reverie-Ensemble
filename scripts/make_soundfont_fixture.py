#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 QuadraJ and contributors.
"""Original deterministic test tone banks; no third-party recordings. SF3 needs ffmpeg."""
import math,struct,subprocess,tempfile,ctypes
from pathlib import Path
root=Path(__file__).resolve().parents[1]/'tests'/'fixtures';root.mkdir(exist_ok=True)
def chunk(name,data):
 data+=b'\0' if len(data)%2 else b''
 return name.encode()+struct.pack('<I',len(data))+data
def li(name,*children):return chunk('LIST',name.encode()+b''.join(children))
def name(s):return s.encode().ljust(20,b'\0')[:20]
rate=22050;count=22050;pcm=b''.join(struct.pack('<h',round(12000*math.sin(2*math.pi*261.625565*i/rate))) for i in range(count))
for sf3 in [False,True]:
 if sf3:
  lib=ctypes.CDLL('/opt/homebrew/lib/libsndfile.dylib')
  class Info(ctypes.Structure):_fields_=[('frames',ctypes.c_int64),('samplerate',ctypes.c_int),('channels',ctypes.c_int),('format',ctypes.c_int),('sections',ctypes.c_int),('seekable',ctypes.c_int)]
  lib.sf_open.argtypes=[ctypes.c_char_p,ctypes.c_int,ctypes.POINTER(Info)];lib.sf_open.restype=ctypes.c_void_p
  lib.sf_write_short.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_short),ctypes.c_int64];lib.sf_close.argtypes=[ctypes.c_void_p]
  with tempfile.TemporaryDirectory() as tmp:
   file=Path(tmp)/'tone.ogg';info=Info(0,rate,1,0x200060,0,0);handle=lib.sf_open(str(file).encode(),0x20,ctypes.byref(info));assert handle
   values=(ctypes.c_short*count).from_buffer_copy(pcm);lib.sf_write_short(handle,values,count);lib.sf_close(handle);samples=file.read_bytes()
  start=0;end=len(samples)
 else:samples=pcm+b'\0'*92;start=0;end=count
 info=li('INFO',chunk('ifil',struct.pack('<HH',3 if sf3 else 2,1)),chunk('isng',b'EMU8000\0'),chunk('INAM',b'QuadraJ Test Tone\0'),chunk('ICOP',b'Copyright 2026 QuadraJ; Apache-2.0\0'))
 phdr=b''.join(name(n)+struct.pack('<HHHIII',pr,bank,bag,0,0,0) for n,pr,bank,bag in [('Tone',0,0,0),('Bank Test',7,5,1),('EOP',0,0,2)])
 pbag=b''.join(struct.pack('<HH',i,0) for i in [0,1,2]);pgen=struct.pack('<HHHHHH',41,0,41,0,0,0)
 inst=name('Tone Instrument')+struct.pack('<H',0)+name('EOI')+struct.pack('<H',1)
 ibag=struct.pack('<HHHH',0,0,4,0)
 # release envelope then loop mode, overriding root, sampleID (sampleID must be last)
 igen=struct.pack('<HhHHHHHH',38,-1200,54,1,58,60,53,0)
 shdr=name('Tone')+struct.pack('<IIIIIBbHH',start,end,100,count-100,rate,60,0,0,0x11 if sf3 else 1)+name('EOS')+b'\0'*26
 pdta=li('pdta',chunk('phdr',phdr),chunk('pbag',pbag),chunk('pmod',b'\0'*10),chunk('pgen',pgen),chunk('inst',inst),chunk('ibag',ibag),chunk('imod',b'\0'*10),chunk('igen',igen+b'\0'*4),chunk('shdr',shdr))
 body=b'sfbk'+info+li('sdta',chunk('smpl',samples))+pdta
 (root/('tone.sf3' if sf3 else 'tone.sf2')).write_bytes(chunk('RIFF',body))
