#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h> 
#include <netdb.h>

#define len 598528

void getSamples(unsigned char *samples) {
  char *service = "7000"; // First arg:  local port/service

  // Construct the server address structure
  struct addrinfo addrCriteria;                   // Criteria for address
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = AF_UNSPEC;             // Any address family
  addrCriteria.ai_flags = AI_PASSIVE;             // Accept on any address/port
  addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram socket
  addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP socket

  struct addrinfo *servAddr; // List of server addresses
  int rtnVal = getaddrinfo(NULL, service, &addrCriteria, &servAddr);
  if (rtnVal != 0){
    printf("getaddrinfo() failed");
    exit(1);
   }

  // Create socket for incoming connections
  int sock = socket(servAddr->ai_family, servAddr->ai_socktype,
      servAddr->ai_protocol);
  if (sock < 0){
	printf("Socket() failed");
	exit(1);
  }

  // Bind to the local address
  if (bind(sock, servAddr->ai_addr, servAddr->ai_addrlen) < 0){
	printf("bind() failed");
  	exit(1);
  }

  // Free address list allocated by getaddrinfo()
  freeaddrinfo(servAddr);


  int i;
  for (i = 0; i < len; i++) {
	struct sockaddr_in clntAddr; // Client address
    	// Set Length of client address structure (in-out parameter)
    	socklen_t clntAddrLen = sizeof(clntAddr);

    	// Block until receive message from a client
    	int numBytesRcvd = recvfrom(sock, &samples[i], sizeof(unsigned char), 0,
        	(struct sockaddr *) &clntAddr, &clntAddrLen);
	int port = ntohs(clntAddr.sin_port);
	printf("\nThe source port was:%d", port);
	port = 20000-port;
	samples[i] = port;
	if (numBytesRcvd < 0){
		printf("\n Received: %d \n",numBytesRcvd);
		exit(1);
        }
	//printf("\nNumber of bytes received is %d", numBytesRcvd);

	//printf("\n%d %hu", i+1,samples[i]);
    }

	printf("\n Exiting getSamples"); 
	return;
}

int main(int argc, char *argv[])
{
	printf("Wav Write Test\n");
	if (argc != 2) {
		fprintf(stderr, "Expecting wav file as argument\n");
		return 1;
	}

	unsigned char samples[len];
  	memset(samples, 0, len);
	printf("\n Getting the Samples");
	getSamples(samples);
        printf("\n Got the Samples. Writing to File\n");	
	// Write to a new file
	SF_INFO sfinfo_w ;
    	sfinfo_w.channels = 1;
    	sfinfo_w.samplerate = 44100;
    	sfinfo_w.format = SF_FORMAT_WAV | SF_FORMAT_PCM_U8;
	
	SNDFILE *sndFile_w = sf_open(argv[1], SFM_WRITE, &sfinfo_w);
	sf_count_t count_w = sf_write_raw(sndFile_w, samples, len) ;
    	sf_write_sync(sndFile_w);
    	sf_close(sndFile_w);

	return 0;
}
