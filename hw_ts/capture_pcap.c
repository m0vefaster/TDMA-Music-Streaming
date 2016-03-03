/*
    Packet sniffer using libpcap library
*/
//sudo apt-get install libpcap-dev
//Source: http://www.binarytides.com/packet-sniffer-code-c-libpcap-linux-sockets/
#include<pcap.h>
#include<stdio.h>
#include<stdlib.h> // for exit()
#include<string.h> //for memset
#include<math.h>
 
#include<sys/socket.h>
#include<arpa/inet.h> // for inet_ntoa()
#include<net/ethernet.h>
#include<netinet/ip_icmp.h>   //Provides declarations for icmp header
#include<netinet/udp.h>   //Provides declarations for udp header
#include<netinet/tcp.h>   //Provides declarations for tcp header
#include<netinet/ip.h>    //Provides declarations for ip header
#include <inttypes.h>

#include <sndfile.h>


unsigned char *samples;
uint64_t *timeStamps;
int total;

int multFactor;
int expected;
int frequency;
int timeSlot;

FILE *logfp;

int getSourceIP(const u_char *Buffer , int Size)
	{
	unsigned short iphdrlen;
	//Moving ahead to encapsulated packet which is 62 bytes ahead 
	struct iphdr *iph = (struct iphdr *)(Buffer + 62  + sizeof(struct ethhdr) );
	iphdrlen =iph->ihl*4;

	struct sockaddr_in dest; 
	dest.sin_addr.s_addr = iph->daddr;

	char *sourceIP = inet_ntoa(dest.sin_addr); 
	int last_octet=  atoi(strrchr(sourceIP,'.')+1);   

	//fprintf(logfp,"\n%d", last_octet);
	return last_octet;
	}

uint64_t getTimestamp(const u_char *Buffer , int Size){
	char val[4];
	memcpy(val, Buffer + 46,4);
	uint32_t x;
	memcpy(&x, val,4);
	float f = ((x>>16)&0x7fff); // whole part
	f += (x&0xffff) / 65536.0f; // fraction
	if (((x>>31)&0x1) == 0x1) { f = -f; } // sign bit set
	float timeStampNs  = ntohl(x);

	memcpy(val, Buffer + 58, 4);
	memcpy(&x, val,4);
	uint32_t timeStampS  = ntohl(x);

	uint64_t timeStamp= timeStampS * pow(10,9) + timeStampNs;

	return timeStamp;
}


void getERSPANTimestamps(const unsigned char *buffer, unsigned int size) {

	//Get the IP Header part of this packet , excluding the ethernet header
	struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));

	if (iph->protocol==47){

		int octet = getSourceIP(buffer,size);
		uint64_t timestamp = getTimestamp(buffer,size);
		
		samples[total] = octet;
		timeStamps[total] = timestamp;
		total++;
	} 

}

void writeToFile(char *output_file){
	
        printf("\n Got the Samples. Writing to File\n");	
	fprintf(logfp,"\n\nSamples....");
	
	int totalSamples =0;
	int i;
	uint64_t prev = 0;
	unsigned char *validSamples;
	int expectedValid = expected/multFactor;
	validSamples = (unsigned char *) malloc(expected * sizeof(unsigned char));
	uint64_t start = timeStamps[1];
	int nightTimePackets = 0;
	int slot = 0;

	fprintf(logfp, "\nTimeslot is: %d", timeSlot);
	fprintf(logfp, "\nstart is");
	fprintf(logfp,"%" PRIu64 "\n", start);

	for(i=2;i<expected;i++){
		
		validSamples[totalSamples++]= samples[i];

		fprintf(logfp,"\nSlot %d has value %u\n",slot, samples[i]);
		fprintf(logfp,"%" PRIu64 "\n", timeStamps[i]);
		fprintf(logfp,"%" PRIu64 "\n", (start+timeSlot));
		fprintf(logfp,"\nDay Time Packets:");
		while(timeStamps[i] < (start +timeSlot)){
			fprintf(logfp,"%u ", samples[i]);
			i++;	
		}
		
		start += timeSlot;

		//nightTime Packets
		fprintf(logfp,"\nNight Time Packets:");
		while(timeStamps[i] < (start + timeSlot)){
			fprintf(logfp,"%u ", samples[i]);
			nightTimePackets++;
			i++;
		}

		if(totalSamples> expectedValid)
			break;

		fprintf(logfp,"\n\n");
		start += timeSlot;
		slot++;
	
	} 
	fprintf(logfp,"\nNight Time Packets are %d", nightTimePackets);
	printf("\nFound all valid Samples...");

	// Write to a new file
	SF_INFO sfinfo_w ;
    	sfinfo_w.channels = 1;
    	sfinfo_w.samplerate = frequency;
    	sfinfo_w.format = SF_FORMAT_WAV | SF_FORMAT_PCM_U8;
	
	SNDFILE *sndFile_w = sf_open(output_file, SFM_WRITE, &sfinfo_w);
	sf_count_t count_w = sf_write_raw(sndFile_w, validSamples, expectedValid);
    	sf_write_sync(sndFile_w);
    	sf_close(sndFile_w);

	printf("\nDone.....");
 		
}
int main(int argc, char *argv[]){

	pcap_t *pcap;
	const unsigned char *packet;
	char errbuf[PCAP_ERRBUF_SIZE];
	struct pcap_pkthdr header;

	if ( argc != 4 ) {
		printf("Usage: ./capture <erspan_pcap_file> <Audio Output FileName> <Audio Reference File>\n");
		exit(1);
	}

	char *pcap_file = argv[1];
	char *output_file = argv[2];
	char *ref_file = argv[3];

	SF_INFO sndInfo;
    	SNDFILE *sndFile = sf_open(ref_file, SFM_READ, &sndInfo);
    	if (sndFile == NULL) {
		fprintf(stderr, "Error reading source file '%s': %s\n", ref_file, sf_strerror(sndFile));
        return 1;
    }

	multFactor = (8000*80)/sndInfo.samplerate;
	timeSlot = (125000/sndInfo.samplerate) * 8000;
	expected = sndInfo.frames;
	expected = expected * multFactor;
	frequency = sndInfo.samplerate;

    	samples = (unsigned char *) malloc(expected * sizeof(unsigned char));
    	timeStamps = (uint64_t *) malloc(expected * sizeof(uint64_t));
	memset (samples, 0, expected);
	memset (timeStamps,0, expected);	

	logfp = fopen("capture.log","w");

	pcap = pcap_open_offline(pcap_file, errbuf);

	if (pcap == NULL){
		fprintf(stderr, "error reading pcap file: %s\n", errbuf);
		exit(1);
	}

	/* Now just loop through extracting packets as long as we have
	 * some to read.
	 */
	while ((packet = pcap_next(pcap, &header)) != NULL)
		getERSPANTimestamps(packet, header.caplen);	
 	
	writeToFile(output_file);
		
	return 0;
}
