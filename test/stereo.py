#!/usr/bin/env python

import wave # 1
import struct

ifile = wave.open("../files/1.wav","r")
ofile = wave.open("../files/noise.wav", "w")
ofile.setparams(ifile.getparams())

sampwidth = ifile.getsampwidth()
fmts = (None, "=B", "=h", None, "=l")
fmt = fmts[sampwidth]
dcs  = (None, 128, 0, None, 0)
dc = dcs[sampwidth]


for i in range(ifile.getnframes()): 
	iframe=ifile.readframes(1) 
	(left,right)=struct.unpack(fmt,iframe) 
	left="0" 
	oframe=struct.pack(fmt, left,right) 
	ofile.writeframes(oframe)

ifile.close()
ofile.close()
