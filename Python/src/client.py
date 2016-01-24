from audiotools import *
import sys
import io
import struct
import wave
from argparse import ArgumentParser
import socket
import pickle

def get_info(audio_file, main_args):
    """
    create a dictionary of information for the audiofile object.

    """
    info = {}
    info["channels"] = audio_file.channels()
    info["channel_mask"] = audio_file.channel_mask()
    info["bits"] = audio_file.bits_per_sample()
    info["sample_rate"] = audio_file.sample_rate()
    info["frames"] = audio_file.total_frames()
    info["length"] = audio_file.seconds_length()
    info["seekable"] = audio_file.seekable()
    info["verified"] = audio_file.verify()
    info["chunks"] = audio_file.has_foreign_wave_chunks()
    #info["available"] = audio_file.available(BIN)
    info["header"], info["footer"] = audio_file.wave_header_footer()

    if main_args.verbose:
        print "No. of Channels:\t\t", info["channels"]
        print "Channel mask:\t\t\t", info["channel_mask"]
        print "Bits per sample:\t\t", info["bits"], "BIT"
        print "Sample Rate:\t\t\t", (info["sample_rate"]/1000.0), "k"
        print "Number of Frames:\t\t", info["frames"]
        print "Audio Length:\t\t\t", info["length"], "seconds"
        print "Audio File Seekable?:\t\t", info["seekable"]
        print "File has foreign chunks?:\t", info["chunks"]
        #print "Correct Binaries present?:\t", info["available"]
    return info

def main():
    parser = ArgumentParser()
    parser.add_argument(
        "-v",
        "--verbose",
        help = "Run program verbosely",
        default = False,
        action = "store_true",
    )

    main_args = parser.parse_args()
    
    ifile = wave.open("../files/9.wav","r")
    sampwidth = ifile.getsampwidth()
    fmts = (None, "=B", "=h", None, "=l")
    fmt = fmts[sampwidth]
    dcs  = (None, 128, 0, None, 0)
    dc = dcs[sampwidth]

    output_framelist = []
    for i in range(ifile.getnframes()):
	iframe = ifile.readframes(1)
	iframe = struct.unpack(fmt, iframe)[0]
	iframe -= dc
	output_framelist.append(iframe)

    ifile.close()

    s = socket.socket()
    host = 'localhost'
    port = 50007

    data = pickle.dumps(list(output_framelist))
    s.connect((host, port))
    length = len(data)
    s.sendall(struct.pack('!I', length))
    s.sendall(data)
    #s.close()

if __name__ == "__main__":
    main()
