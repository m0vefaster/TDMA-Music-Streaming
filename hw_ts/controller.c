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
#include <time.h>

// recv and send for len 
// exit and close socket
// Source port instead of source ip
// sys times for start time and absolute--->Done 
// libpcap to get in timeslots

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif


static inline __attribute__((always_inline))
uint64_t get_time_ns(void)
{
    struct timespec tp;

    if (unlikely(clock_gettime(CLOCK_REALTIME, &tp) != 0))
        return -1;

    return (1000000000) * (uint64_t)tp.tv_sec + tp.tv_nsec;
}

void receiveFromClient(int clntSocket, unsigned char *val, int len, char *type){

	if (recv(clntSocket, &val, len , 0)!=len){
		printf("\nMessage Received Failed for %s\n", type);
		exit(1);
        }

}

void receiveOkay(int clntSocket, char *type){
	unsigned char okay;

	if (recv(clntSocket, &okay, 1 , 0)!=1){
		printf("\nMessage Received Failed for %s\n", type);
		exit(1);
        }

	if (okay!=1){
		printf("\nOkay was not returned: Returned %hu\n", okay);
                exit(1);
	}
}

void sendOkay(int clntSocket, char *type){
	unsigned char ch = '1';	
	if (send(clntSocket, &ch , 1, 0) != 1){
                printf("\nMessage Sending Failed for %s\n", type);
                exit(1);
        }	

}

void sendToClientCh(int clntSocket, unsigned char *ch, int len,char *type){

	if (send(clntSocket, ch, len, 0) != len){
                printf("\nMessage Sending Failed for %s\n", type);
                exit(1);
        }	
}

void sendToClientI32(int clntSocket, uint32_t *ch, int len,char *type){

	if (send(clntSocket, ch, len, 0) != len){
                printf("\nMessage Sending Failed for %s\n", type);
                exit(1);
        }	
}

void sendToClientI64(int clntSocket, uint64_t *ch, int len,char *type){

	if (send(clntSocket, ch, len, 0) != len){
                printf("\nMessage Sending Failed for %s\n", type);
                exit(1);
        }	
}

void HandleTCPClient(int clntSocket, unsigned char *buffer,int len, int sampleRate) {
	int i;
	unsigned char src_num, SCHED_QUEUE_SIZE;
	
	unsigned char num_hosts=255, host_ids[num_hosts];
	uint32_t fixed_quanta[num_hosts*sizeof(uint32_t)];
	
	uint64_t start_time , end_time, now, offset = 2000000000;
	uint64_t time_slot =  (1000000000 / sampleRate); 

	for(i=0;i<=num_hosts;i++)
		host_ids[i] = i;

	memset(fixed_quanta,0, num_hosts*sizeof(uint32_t));
	receiveFromClient(clntSocket, &src_num, sizeof(unsigned char), "register_client");
	receiveFromClient(clntSocket, &SCHED_QUEUE_SIZE,sizeof(unsigned char), "register_client");
	sendOkay(clntSocket, "register_client");
		
	sendToClientCh(clntSocket, &num_hosts, sizeof(unsigned char), "receive_initial_data");
	sendToClientCh(clntSocket, host_ids, sizeof(unsigned char) * num_hosts, "receive_initial_data");
	sendToClientI32(clntSocket, fixed_quanta, sizeof(uint32_t) * num_hosts, "receive_initial data");	
	receiveOkay(clntSocket, "receive_initial_data");

	
	for(i=0;i<len;i++){
		memset(fixed_quanta,0, num_hosts*sizeof(uint32_t));
		fixed_quanta[buffer[i]] = 1;
		now = get_time_ns();
		start_time  = now + offset;
		end_time = now + time_slot + offset;

	 	sendToClientI64(clntSocket, &start_time, sizeof(uint64_t), "get_single_timeslot");
	 	sendToClientI64(clntSocket, &end_time, sizeof(uint64_t), "get_single_timeslot");
		sendToClientI32(clntSocket, fixed_quanta, sizeof(uint32_t) * num_hosts, "receive_initial data");	

  		start_time += time_slot+1;	

		/*Ignoring tx_update*/
		int max_len=100;
		char temp[max_len];
		recv(clntSocket, &temp, max_len , MSG_DONTWAIT);
	}		
}

void startServer(unsigned char *buffer,int len, int sampleRate) {

	in_port_t servPort = atoi("8000"); // First arg:  local port

	// Create socket for incoming connections
	int servSock; // Socket descriptor for server
	if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		exit(1);

	// Construct local address structure
	struct sockaddr_in servAddr;                  // Local address
	memset(&servAddr, 0, sizeof(servAddr));       // Zero out structure
	servAddr.sin_family = AF_INET;                // IPv4 address family
	servAddr.sin_addr.s_addr = inet_addr("192.168.1.100"); // Any incoming interface
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
		printf("\nHandling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
	else
		puts("Unable to get client address");

	HandleTCPClient(clntSock,buffer, len,sampleRate);
	return;
}

int main(int argc, char *argv[])
{
	printf("Wav Write Controller\n");
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


	// Check channels - mono
	if (sndInfo.channels != 1) {
		fprintf(stderr, "Wrong number of channels\n");
		sf_close(sndFile);
		return 1;
	}

	// Allocate memory
	unsigned char *buffer = malloc(sndInfo.frames * sizeof(unsigned char));
	if (buffer == NULL) {
		fprintf(stderr, "Could not allocate memory for file\n");
		sf_close(sndFile);
		return 1;
	}

	// Load data
	long numFrames = sf_read_raw(sndFile, buffer, sndInfo.frames);

	// Print Samples/Frames
  	/*int i;	
	for (i=0;i<numFrames;i++){
		printf("%d %u\n", (i+1), (unsigned int)buffer[i]);
	}*/

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
	printf("\nStarting the server..\n");
	startServer(buffer, numFrames, sndInfo.samplerate);

	sf_close(sndFile);
	free(buffer);
	printf("\n");

	return 0;
}
