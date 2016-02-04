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

#define len 120001

void HandleTCPClient(int clntSocket, short *samples) {
  int i;
  for (i = 0; i < len; i++) {
        int bytesreceived = recv(clntSocket, &samples[i], sizeof(short), /* flags */ 0);

        if (bytesreceived <= 0) {
		printf("\n Received: %d \n",bytesreceived);
		exit(1);
        }
	printf("\n%d %d", i+1,samples[i]);
    }
   printf("\n Exiting HandleTCPClient");
}

void getSamples(short *samples) {

  in_port_t servPort = atoi("7000"); // First arg:  local port

  // Create socket for incoming connections
  int servSock; // Socket descriptor for server
  if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	exit(1);

  // Construct local address structure
  struct sockaddr_in servAddr;                  // Local address
  memset(&servAddr, 0, sizeof(servAddr));       // Zero out structure
  servAddr.sin_family = AF_INET;                // IPv4 address family
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
  servAddr.sin_port = htons(servPort);          // Local port

  // Bind to the local address
  if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0){
	printf("\nBind failed\n");
	exit(1);
  }

  // Mark the socket so it will listen for incoming connections
  if (listen(servSock, 5) < 0){
	printf("\nListening on port failed\n");
	exit(1);
  }

  for (;;) { 
	// Run forever
    	struct sockaddr_in clntAddr; // Client address
	// Set length of client address structure (in-out parameter)
	socklen_t clntAddrLen = sizeof(clntAddr);

	// Wait for a client to connect
	int clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen);
	if (clntSock < 0){
		printf("\nClient Socket is %d\n",clntSock);
		exit(1);
	}

	// clntSock is connected to a client!

	char clntName[INET_ADDRSTRLEN]; // String to contain client address
	if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName,sizeof(clntName)) != NULL)
		printf("Handling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
	else
		puts("Unable to get client address");

	HandleTCPClient(clntSock, samples);
	printf("\n Exiting getSamples"); 
	return;
    }
}

int main(int argc, char *argv[])
{
	printf("Wav Write Test\n");
	if (argc != 2) {
		fprintf(stderr, "Expecting wav file as argument\n");
		return 1;
	}

	short samples[len];
  	memset(samples, 0, len);
	printf("\n Getting the Samples");
	getSamples(samples);
        printf("\n Got the Samples. Writing to File\n");	
	// Write to a new file
	SF_INFO sfinfo_w ;
    	sfinfo_w.channels = 1;
    	sfinfo_w.samplerate = 16000;
    	sfinfo_w.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	
	SNDFILE *sndFile_w = sf_open(argv[1], SFM_WRITE, &sfinfo_w);
	sf_count_t count_w = sf_write_short(sndFile_w, samples, len) ;
    	sf_write_sync(sndFile_w);
    	sf_close(sndFile_w);

	return 0;
}
