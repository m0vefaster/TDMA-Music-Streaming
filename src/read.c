#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>

int main(int argc, char *argv[])
{
	printf("Wav Read Test\n");
	if (argc != 3) {
		fprintf(stderr, "Expecting wav file as argument\n");
		return 1;
	}

	// Open sound file
	SF_INFO sndInfo;
	SNDFILE *sndFile = sf_open(argv[1], SFM_READ, &sndInfo);
	if (sndFile == NULL) {
		fprintf(stderr, "Error reading source file '%s': %s\n", argv[1], sf_strerror(sndFile));
		return 1;
	}

	// Check format - 16bit PCM
	if (sndInfo.format != (SF_FORMAT_WAV | SF_FORMAT_PCM_16)) {
		fprintf(stderr, "Input should be 16bit Wav\n");
		sf_close(sndFile);
		return 1;
	}

	// Check channels - mono
	if (sndInfo.channels != 1) {
		fprintf(stderr, "Wrong number of channels\n");
		sf_close(sndFile);
		return 1;
	}

	// Allocate memory
	short *buffer = malloc(sndInfo.frames * sizeof(short));
	if (buffer == NULL) {
		fprintf(stderr, "Could not allocate memory for file\n");
		sf_close(sndFile);
		return 1;
	}

	// Load data
	long numFrames = sf_read_short(sndFile, buffer, sndInfo.frames);

	// Print Samples/Frames
	/*
	for (int i=0;i<numFrames;i++)
		printf("%hu ", buffer[i]);
	*/

	// Check correct number of samples loaded
	if (numFrames != sndInfo.frames) {
		fprintf(stderr, "Did not read enough frames for source\n");
		sf_close(sndFile);
		free(buffer);
		return 1;
	}

	// Output Info
	printf("Read %ld frames from %s, Sample rate: %d, Length: %fs\n",
		numFrames, argv[1], sndInfo.samplerate, (float)numFrames/sndInfo.samplerate);
	

	// Write to a new file
	SF_INFO sfinfo_w ;
    	sfinfo_w.channels = 1;
    	sfinfo_w.samplerate = 16000;
    	sfinfo_w.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	
	SNDFILE *sndFile_w = sf_open(argv[2], SFM_WRITE, &sfinfo_w);
	sf_count_t count_w = sf_write_short(sndFile_w, buffer, sndInfo.frames) ;
    	sf_write_sync(sndFile_w);
    	sf_close(sndFile_w);

	sf_close(sndFile);
	free(buffer);

	return 0;
}
