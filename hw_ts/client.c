#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unistd.h>
#include <netdb.h>


void sendData(short *buffer,int len) {
  int sock;
  char *servPort;
  char *servIP;
  char *fileName;
  int bytesRcvd, totalBytesRcvd;

  servIP = "192.168.1.33" ;
  servPort = "7000";

  // Tell the system what kind(s) of address info we want
  struct addrinfo addrCriteria;                   // Criteria for address match
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = AF_UNSPEC;             // Any address family
  // For the following fields, a zero value means "don't care"
  addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram sockets
  addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP protocol

  // Get address(es)
  struct addrinfo *servAddr; // List of server addresses
  int rtnVal = getaddrinfo(servIP, servPort, &addrCriteria, &servAddr);
  if (rtnVal != 0){
	printf("getaddrinfo() failed");
	exit(1);
  }

  // Create a datagram/UDP socket
  sock = socket(servAddr->ai_family, servAddr->ai_socktype,
      servAddr->ai_protocol); // Socket descriptor for client
  if (sock < 0){
	printf("socket() failed");
  }

  /* Send the string to the server */
  int i;
  for (i=0;i<len;i++){

      //printf("\nSent %d %d", (i+1), buffer[i]);
      // Send the string to the server
      ssize_t numBytes = sendto(sock, &buffer[i], sizeof(short) , 0,
      servAddr->ai_addr, servAddr->ai_addrlen);
      if (numBytes < 0){
	printf("\nNot enough data was sent\n");  
                exit(1);
     }
    //TODO: Remove sleep and check what is wrong
    usleep(1);
  }

  printf("\nNunber of samples sent is:%d", len);
  sleep(100); 
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

	// Print Samples/Frames
  	int i;	
	for (i=0;i<numFrames;i++)
		printf("%d %hu\n", (i+1), buffer[i]);

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


