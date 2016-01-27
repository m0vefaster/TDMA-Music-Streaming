#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void sendData(short *buffer,int len) {
  int sock;
  struct sockaddr_in servAddr;
  unsigned short servPort;
  char *servIP;
  char *fileName;
  int bytesRcvd, totalBytesRcvd;

  servIP = "127.0.0.1" ;
  servPort = atoi("7000");

  /* Create a reliable, stream socket using TCP */
  if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
	printf("\nFailed to form a socket\n");
	exit(1);
  }

  /* Construct the server address structure */
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = inet_addr(servIP);
  servAddr.sin_port = htons(servPort); /* Server port */

  /* Establish the connection */
  if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
	printf("\nFailed to connect to server\n");
	exit(1);
   }

  printf("\nConnected\n");

  /* Send the string to the server */

  for (int i=0;i<len;i++){
  	if (send(sock, &buffer[i],sizeof(short),0) != sizeof(short)){
		printf("\nNot enough data was sent\n");
		exit(1);
	}
      //printf("\nSent %d %d", (i+1), buffer[i]);
  }

  printf("\nNunber of samples sent is:%d", len);
 
  close(sock);
  exit(0);
}

int main(int argc, char *argv[])
{
	printf("Wav Read Test\n");
	if (argc != 2) {
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

	/*
	// Print Samples/Frames
	for (int i=0;i<numFrames;i++)
		printf("%d %hu\n", (i+1), buffer[i]);
	*/

	// Check correct number of samples loaded
	if (numFrames != sndInfo.frames) {
		fprintf(stderr, "Did not read enough frames for source\n");
		sf_close(sndFile);
		free(buffer);
		return 1;
	}

	// Output Info
	printf("Read %ld frames from %s, Sample rate: %d, Length: %fs",
		numFrames, argv[1], sndInfo.samplerate, (float)numFrames/sndInfo.samplerate);

	// Send the data	
	sendData(buffer, numFrames);	

	sf_close(sndFile);
	free(buffer);
	printf("\n");
	return 0;
}


