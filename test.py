from audiotools import *
import sys
import io
import struct
import wave
from argparse import ArgumentParser

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

    #open audio file as an AudioFile object
    audio_file =  open("files/2.wav")
    file_info = get_info(audio_file, main_args)

    #Creates a WaveReader object from the AudioFile Object
    pcm_data = audio_file.to_pcm()

    #Creates a FrameList object from WaveReader object. Currently reads all
    #frames in file
    frame_list = pcm_data.read(file_info["frames"])

    #Convert samples to floats (-1.0 - +1.0)
    float_frame_list = frame_list.to_float()

    #print float_frame_list.frame(0)
    f = pcm.from_list(frame_list,2,16,True)
    #print list(f)

    #eventually do some signal processing here...

    #Convert back to integer FrameList
    output_framelist = float_frame_list.to_int(file_info["bits"])
    #print list(output_framelist)
    #To convert back use f = from_list([-1,0,1,2],2,16,True) where the list is going to be the data received

    #now back to raw bytes
    #output_data = output_framelist.to_bytes(False, True)

    
    #noise_file = io.open('noise.wav', 'wb')

    #for d in list(output_framelist):
    #    packed_value = struct.pack('h', d)
    #    noise_file.write(packed_value)

    #noise_file.close()

    noise_output = wave.open('files/noise.wav', 'w')
    noise_output.setparams((2, 2, 44100, 0, 'NONE', 'not compressed'))

    for d in list(output_framelist):
        packed_value = struct.pack('h', d)
        noise_output.writeframes(packed_value)
        #noise_output.writeframes(packed_value)
	
if __name__ == "__main__":
    main()

