from audiotools import *
import sys
import io
import struct
import wave
import socket
import pickle

def recvall(sock, count):
    buf = b''
    while count:
        newbuf = sock.recv(count)
        if not newbuf: return None
        buf += newbuf
        count -= len(newbuf)
    return buf

def main():
    HOST = 'localhost'
    PORT = 50007
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((HOST, PORT))
    s.listen(1)
    conn, addr = s.accept()
    print 'Connected by', addr

    lengthbuf = recvall(conn, 4)
    length, = struct.unpack('!I', lengthbuf)
    data = recvall(conn, length)
    output_framelist = pickle.loads(data)
    #conn.close()

    ifile = wave.open("../files/9.wav","r")
    ofile = wave.open("../files/noise.wav", "w")
    ofile.setparams(ifile.getparams())

    sampwidth = ifile.getsampwidth()
    fmts = (None, "=B", "=h", None, "=l")
    fmt = fmts[sampwidth]
    dcs  = (None, 128, 0, None, 0)
    dc = dcs[sampwidth]

    for iframe in output_framelist:
	    oframe = iframe / 2;
	    oframe += dc
	    oframe = struct.pack(fmt, oframe)
	    ofile.writeframes(oframe)

    ifile.close()
    ofile.close()

if __name__ == "__main__":
    main()
