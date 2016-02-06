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

#include <errno.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <asm/types.h>
#include <linux/net_tstamp.h>
#include <linux/errqueue.h>

#ifndef SO_TIMESTAMPING
# define SO_TIMESTAMPING         37
# define SCM_TIMESTAMPING        SO_TIMESTAMPING
#endif

#ifndef SO_TIMESTAMPNS
# define SO_TIMESTAMPNS 35
#endif

#ifndef SIOCGSTAMPNS
# define SIOCGSTAMPNS 0x8907
#endif

#ifndef SIOCSHWTSTAMP
# define SIOCSHWTSTAMP 0x89b0
#endif


static void usage(const char *error)
{
	if (error)
		printf("invalid option: %s\n", error);
	printf("timestamping interface option*\n\n"
	       "Options:\n"
	       "  IP_MULTICAST_LOOP - looping outgoing multicasts\n"
	       "  SO_TIMESTAMP - normal software time stamping, ms resolution\n"
	       "  SO_TIMESTAMPNS - more accurate software time stamping\n"
	       "  SOF_TIMESTAMPING_TX_HARDWARE - hardware time stamping of outgoing packets\n"
	       "  SOF_TIMESTAMPING_TX_SOFTWARE - software fallback for outgoing packets\n"
	       "  SOF_TIMESTAMPING_RX_HARDWARE - hardware time stamping of incoming packets\n"
	       "  SOF_TIMESTAMPING_RX_SOFTWARE - software fallback for incoming packets\n"
	       "  SOF_TIMESTAMPING_SOFTWARE - request reporting of software time stamps\n"
	       "  SOF_TIMESTAMPING_RAW_HARDWARE - request reporting of raw HW time stamps\n"
	       "  SIOCGSTAMP - check last socket time stamp\n"
	       "  SIOCGSTAMPNS - more accurate socket time stamp\n");
	exit(1);
}

static void bail(const char *error)
{
	printf("%s\n", error);
	exit(1);
}

void sendData(short *buffer,int numFrames,int argc, char *argv[]) {
	int sock;
	char *servPort;
	char *servIP;
	char *fileName;
	int bytesRcvd, totalBytesRcvd;

	int so_timestamping_flags = 0;
	int so_timestamp = 0;
	int so_timestampns = 0;
	int siocgstamp = 0;
	int siocgstampns = 0;

	char *interface;
	struct ifreq device;
	struct ifreq hwtstamp;
	struct hwtstamp_config hwconfig, hwconfig_requested;
	int enabled = 1;
	int val;
	socklen_t len;

	servIP = "192.168.1.33" ;
	servPort = "7000";

	if (argc < 2)
	usage(0);

	interface = argv[2];
	
	int i;
	for (i = 3; i < argc; i++) {
		if (!strcasecmp(argv[i], "SO_TIMESTAMP"))
			so_timestamp = 1;
		else if (!strcasecmp(argv[i], "SO_TIMESTAMPNS"))
			so_timestampns = 1;
		else if (!strcasecmp(argv[i], "SIOCGSTAMP"))
			siocgstamp = 1;
		else if (!strcasecmp(argv[i], "SIOCGSTAMPNS"))
			siocgstampns = 1;
		else if (!strcasecmp(argv[i], "SOF_TIMESTAMPING_TX_HARDWARE"))
			so_timestamping_flags |= SOF_TIMESTAMPING_TX_HARDWARE;
		else if (!strcasecmp(argv[i], "SOF_TIMESTAMPING_TX_SOFTWARE"))
			so_timestamping_flags |= SOF_TIMESTAMPING_TX_SOFTWARE;
		else if (!strcasecmp(argv[i], "SOF_TIMESTAMPING_RX_HARDWARE"))
			so_timestamping_flags |= SOF_TIMESTAMPING_RX_HARDWARE;
		else if (!strcasecmp(argv[i], "SOF_TIMESTAMPING_RX_SOFTWARE"))
			so_timestamping_flags |= SOF_TIMESTAMPING_RX_SOFTWARE;
		else if (!strcasecmp(argv[i], "SOF_TIMESTAMPING_SOFTWARE"))
			so_timestamping_flags |= SOF_TIMESTAMPING_SOFTWARE;
		else if (!strcasecmp(argv[i], "SOF_TIMESTAMPING_RAW_HARDWARE"))
			so_timestamping_flags |= SOF_TIMESTAMPING_RAW_HARDWARE;
		else
			usage(argv[i]);
	}

	printf("\nInterface selected is :%s", interface);

	// Tell the system what kind(s) of address info we want
	struct addrinfo addrCriteria;                   // Criteria for address match
	memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
	addrCriteria.ai_family = PF_INET;             // Any address family
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


	memset(&device, 0, sizeof(device));
	strncpy(device.ifr_name, interface, sizeof(device.ifr_name));
	if (ioctl(sock, SIOCGIFADDR, &device) < 0)
		bail("getting interface IP address");

	memset(&hwtstamp, 0, sizeof(hwtstamp));
	strncpy(hwtstamp.ifr_name, interface, sizeof(hwtstamp.ifr_name));
	hwtstamp.ifr_data = (void *)&hwconfig;
	memset(&hwconfig, 0, sizeof(hwconfig));
	hwconfig.tx_type =
		(so_timestamping_flags & SOF_TIMESTAMPING_TX_HARDWARE) ?
		HWTSTAMP_TX_ON : HWTSTAMP_TX_OFF;
	printf("\nhwconfig.tx_type:%d",hwconfig.tx_type);
	hwconfig.rx_filter =
		(so_timestamping_flags & SOF_TIMESTAMPING_RX_HARDWARE) ?
		HWTSTAMP_FILTER_PTP_V1_L4_SYNC : HWTSTAMP_FILTER_NONE;
	hwconfig_requested = hwconfig;
	if (ioctl(sock, SIOCSHWTSTAMP, &hwtstamp) < 0) {
		if ((errno == EINVAL || errno == ENOTSUP) &&
		    hwconfig_requested.tx_type == HWTSTAMP_TX_OFF &&
		    hwconfig_requested.rx_filter == HWTSTAMP_FILTER_NONE)
			printf("SIOCSHWTSTAMP: disabling hardware time stamping not possible\n");
		else
			bail("SIOCSHWTSTAMP");
	}
	printf("SIOCSHWTSTAMP: tx_type %d requested, got %d; rx_filter %d requested, got %d\n",
	       hwconfig_requested.tx_type, hwconfig.tx_type,
	       hwconfig_requested.rx_filter, hwconfig.rx_filter);

	/* set socket options for time stamping */
	if (so_timestamp &&
		setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP,
			   &enabled, sizeof(enabled)) < 0)
		bail("setsockopt SO_TIMESTAMP");

	if (so_timestampns &&
		setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPNS,
			   &enabled, sizeof(enabled)) < 0)
		bail("setsockopt SO_TIMESTAMPNS");

	if (so_timestamping_flags &&
		setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING,
			   &so_timestamping_flags,
			   sizeof(so_timestamping_flags)) < 0)
		bail("setsockopt SO_TIMESTAMPING");

	/* request IP_PKTINFO for debugging purposes */
	if (setsockopt(sock, SOL_IP, IP_PKTINFO,
		       &enabled, sizeof(enabled)) < 0)
		printf("%s: %s\n", "setsockopt IP_PKTINFO", strerror(errno));

	/* verify socket options */
	len = sizeof(val);
	if (getsockopt(sock, SOL_SOCKET, SO_TIMESTAMP, &val, &len) < 0)
		printf("%s: %s\n", "getsockopt SO_TIMESTAMP", strerror(errno));
	else
		printf("SO_TIMESTAMP %d\n", val);

	if (getsockopt(sock, SOL_SOCKET, SO_TIMESTAMPNS, &val, &len) < 0)
		printf("%s: %s\n", "getsockopt SO_TIMESTAMPNS",
		       strerror(errno));
	else
		printf("SO_TIMESTAMPNS %d\n", val);

	if (getsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING, &val, &len) < 0) {
		printf("%s: %s\n", "getsockopt SO_TIMESTAMPING",
		       strerror(errno));
	} else {
		printf("SO_TIMESTAMPING %d\n", val);
		if (val != so_timestamping_flags)
			printf("   not the expected value %d\n",
			       so_timestamping_flags);
	}
	/* Send the string to the server */
	for (i=0;i<numFrames;i++){
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

	printf("\nNunber of samples sent is:%d", numFrames);
	close(sock);
	exit(0);
}

int main(int argc, char *argv[])
{
	printf("Wav Read Test\n");
	if (argc <=2) {
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
  	//int i;	
	//for (i=0;i<numFrames;i++)
	//	printf("%d %hu\n", (i+1), buffer[i]);

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
	printf("\nNumber of Frames is:%d", numFrames);
	sendData(buffer, numFrames,argc,argv);	

	sf_close(sndFile);
	free(buffer);
	printf("\n");
	return 0;
}


