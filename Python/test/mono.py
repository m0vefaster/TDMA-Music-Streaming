#!/usr/bin/env python

import wave # 1
import struct

ifile = wave.open("../files/8.wav","r")
ofile = wave.open("../files/noise.wav", "w")
ofile.setparams(ifile.getparams())

sampwidth = ifile.getsampwidth()
fmts = (None, "=B", "=h", None, "=l")
fmt = fmts[sampwidth]
dcs  = (None, 128, 0, None, 0)
dc = dcs[sampwidth]

l = []
for i in range(ifile.getnframes()):
    iframe = ifile.readframes(1)

    iframe = struct.unpack(fmt, iframe)[0]
    iframe -= dc
    l.append(iframe)

    oframe = iframe / 2;

    oframe += dc
   
    oframe = struct.pack(fmt, oframe)
    ofile.writeframes(oframe)

ifile.close()
ofile.close()

print l
