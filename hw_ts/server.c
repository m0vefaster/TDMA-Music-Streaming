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

#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <net/if.h>


#include <asm/types.h>
#include <linux/net_tstamp.h>
#include <linux/errqueue.h>


#define numFrames 120001


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


static void printpacket(struct msghdr *msg, int res,
			char *data,
			int sock, int recvmsg_flags,
			int siocgstamp, int siocgstampns)
{
	struct sockaddr_in *from_addr = (struct sockaddr_in *)msg->msg_name;
	struct cmsghdr *cmsg;
	struct timeval tv;
	struct timespec ts;
	struct timeval now;

	gettimeofday(&now, 0);

	printf("%ld.%06ld: received %s data, %d bytes from %s, %zu bytes control messages\n",
	       (long)now.tv_sec, (long)now.tv_usec,
	       (recvmsg_flags & MSG_ERRQUEUE) ? "error" : "regular",
	       res,
	       inet_ntoa(from_addr->sin_addr),
	       msg->msg_controllen);
	for (cmsg = CMSG_FIRSTHDR(msg);
	     cmsg;
	     cmsg = CMSG_NXTHDR(msg, cmsg)) {
		printf("   cmsg len %zu: ", cmsg->cmsg_len);
		switch (cmsg->cmsg_level) {
		case SOL_SOCKET:
			printf("SOL_SOCKET ");
			switch (cmsg->cmsg_type) {
			case SO_TIMESTAMP: {
				struct timeval *stamp =
					(struct timeval *)CMSG_DATA(cmsg);
				printf("SO_TIMESTAMP %ld.%06ld",
				       (long)stamp->tv_sec,
				       (long)stamp->tv_usec);
				break;
			}
			case SO_TIMESTAMPNS: {
				struct timespec *stamp =
					(struct timespec *)CMSG_DATA(cmsg);
				printf("SO_TIMESTAMPNS %ld.%09ld",
				       (long)stamp->tv_sec,
				       (long)stamp->tv_nsec);
				break;
			}
			case SO_TIMESTAMPING: {
				struct timespec *stamp =
					(struct timespec *)CMSG_DATA(cmsg);
				printf("SO_TIMESTAMPING ");
				printf("SW %ld.%09ld ",
				       (long)stamp->tv_sec,
				       (long)stamp->tv_nsec);
				stamp++;
				/* skip deprecated HW transformed */
				stamp++;
				printf("HW raw %ld.%09ld",
				       (long)stamp->tv_sec,
				       (long)stamp->tv_nsec);
				break;
			}
			default:
				printf("type %d", cmsg->cmsg_type);
				break;
			}
			break;
		case IPPROTO_IP:
			printf("IPPROTO_IP ");
			switch (cmsg->cmsg_type) {
			case IP_RECVERR: {
				struct sock_extended_err *err =
					(struct sock_extended_err *)CMSG_DATA(cmsg);
				printf("\nError in IP_RECVERR");
				if (res < sizeof(sync))
					printf(" => truncated data?!");
				else if (!memcmp(sync, data + res - sizeof(sync),
							sizeof(sync)))
					printf(" => GOT OUR DATA BACK (HURRAY!)");
				break;
			}
			case IP_PKTINFO: {
				struct in_pktinfo *pktinfo =
					(struct in_pktinfo *)CMSG_DATA(cmsg);
				printf("IP_PKTINFO interface index %u",
					pktinfo->ipi_ifindex);
				break;
			}
			default:
				printf("type %d", cmsg->cmsg_type);
				break;
			}
			break;
		default:
			printf("level %d type %d",
				cmsg->cmsg_level,
				cmsg->cmsg_type);
			break;
		}
		printf("\n");
	}

	if (siocgstamp) {
		if (ioctl(sock, SIOCGSTAMP, &tv))
			printf("\n Error in siocgstamp");
		else
			printf("SIOCGSTAMP %ld.%06ld\n",
			       (long)tv.tv_sec,
			       (long)tv.tv_usec);
	}
	if (siocgstampns) {
		if (ioctl(sock, SIOCGSTAMPNS, &ts))
			printf("\n Error in siocgstamp");
		else
			printf("SIOCGSTAMPNS %ld.%09ld\n",
			       (long)ts.tv_sec,
			       (long)ts.tv_nsec);
	}
}


static short recvpacket(int sock, int recvmsg_flags,
		       int siocgstamp, int siocgstampns)
{
	//char data[256];
	short data;
	struct msghdr msg;
	struct iovec entry;
	struct sockaddr_in from_addr;
	struct {
		struct cmsghdr cm;
		char control[512];
	} control;
	int res;

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &entry;
	msg.msg_iovlen = 1;
	entry.iov_base = &data;
	entry.iov_len = sizeof(data);
	msg.msg_name = (caddr_t)&from_addr;
	msg.msg_namelen = sizeof(from_addr);
	msg.msg_control = &control;
	msg.msg_controllen = sizeof(control);

	res = recvmsg(sock, &msg, recvmsg_flags);
	if (res < 0) {
		printf("\n Error receiving the packet");
	} else {
		//printpacket(&msg, res, data, sock, recvmsg_flags,siocgstamp, siocgstampns);
		printf("\nPrint packet here");
		//short p = (data[1] << 8) + data[2];
		printf("\nData is :%hu\n", data);
		//printf("\nData is :%s",*entry.iov_base);
		if (recvmsg_flags==0)
			printpacket(&msg, res, "heello",sock, recvmsg_flags,siocgstamp, siocgstampns);
		return data;
	}
}

void getSamples(short *samples, int argc, char *argv[]) {
  char *service = "7000"; // First arg:  local port/service
	int so_timestamping_flags = 0;
	int so_timestamp = 0;
	int so_timestampns = 0;
	int siocgstamp = 0;
	int siocgstampns = 0;
	int ip_multicast_loop = 0;

	int enabled=1;
	int val;
	socklen_t len;
   	int i;
	char *interface;
	struct ifreq device;
	struct ifreq hwtstamp;
	struct hwtstamp_config hwconfig, hwconfig_requested;

		
	interface = argv[2];
   for (i = 3; i < argc; i++) {
		if (!strcasecmp(argv[i], "SO_TIMESTAMP"))
			so_timestamp = 1;
		else if (!strcasecmp(argv[i], "SO_TIMESTAMPNS"))
			so_timestampns = 1;
		else if (!strcasecmp(argv[i], "SIOCGSTAMP"))
			siocgstamp = 1;
		else if (!strcasecmp(argv[i], "SIOCGSTAMPNS"))
			siocgstampns = 1;
		else if (!strcasecmp(argv[i], "IP_MULTICAST_LOOP"))
			ip_multicast_loop = 1;
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
		printf("\nError:setsockopt IP_PKTINFO");

	/* verify socket options */
	len = sizeof(val);
	if (getsockopt(sock, SOL_SOCKET, SO_TIMESTAMP, &val, &len) < 0)
		printf("\n%s\n", "getsockopt SO_TIMESTAMP");
	else
		printf("SO_TIMESTAMP %d\n", val);

	if (getsockopt(sock, SOL_SOCKET, SO_TIMESTAMPNS, &val, &len) < 0)
		printf("\n%s\n", "getsockopt SO_TIMESTAMPNS");
	else
		printf("SO_TIMESTAMPNS %d\n", val);

	if (getsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING, &val, &len) < 0) {
		printf("\n%s\n", "getsockopt SO_TIMESTAMPING");
	} else {
		printf("SO_TIMESTAMPING %d\n", val);
		if (val != so_timestamping_flags)
			printf("   not the expected value %d\n",
			       so_timestamping_flags);
	}


  for (i = 0; i < numFrames; i++) {
	struct sockaddr_storage clntAddr; // Client address
    	// Set Length of client address structure (in-out parameter)
    	socklen_t clntAddrLen = sizeof(clntAddr);

	/*
    	// Block until receive message from a client
    	int numBytesRcvd = recvfrom(sock, &samples[i], sizeof(short), 0,
        	(struct sockaddr *) &clntAddr, &clntAddrLen);

	if (numBytesRcvd < 0){
		printf("\n Received: %d \n",numBytesRcvd);
		exit(1);
        }
	printf("\nNumber of bytes received is %d", numBytesRcvd);
	*/
	samples[i] = recvpacket(sock, 0, 1, 1);//siocgstamp,siocgstampns are 1 here

	printf("\n%d %hu", i+1,samples[i]);
    }

	printf("\n Exiting getSamples"); 
	return;
}

int main(int argc, char *argv[])
{
	printf("Wav Write Test\n");
	if (argc < 2) {
		fprintf(stderr, "Expecting wav file as argument\n");
		return 1;
	}

	short samples[numFrames];
  	memset(samples, 0, numFrames);
	printf("\n Getting the Samples");
	getSamples(samples,argc,argv);
        printf("\n Got the Samples. Writing to File\n");	
	// Write to a new file
	SF_INFO sfinfo_w ;
    	sfinfo_w.channels = 1;
    	sfinfo_w.samplerate = 16000;
    	sfinfo_w.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	
	SNDFILE *sndFile_w = sf_open(argv[1], SFM_WRITE, &sfinfo_w);
	sf_count_t count_w = sf_write_short(sndFile_w, samples, numFrames) ;
    	sf_write_sync(sndFile_w);
    	sf_close(sndFile_w);

	return 0;
}
