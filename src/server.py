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

    noise_output = wave.open('../files/noise.wav', 'w')
    noise_output.setparams((2, 2, 44100, 0, 'NONE', 'not compressed'))

    for d in list(output_framelist):
        packed_value = struct.pack('h', d)
        noise_output.writeframes(packed_value)
        #noise_output.writeframes(packed_value)

if __name__ == "__main__":
    main()
