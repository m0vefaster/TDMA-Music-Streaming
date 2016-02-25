/*
    Packet sniffer using libpcap library
*/
//sudo apt-get install libpcap-dev
//Source: http://www.binarytides.com/packet-sniffer-code-c-libpcap-linux-sockets/
#include<pcap.h>
#include<stdio.h>
#include<stdlib.h> // for exit()
#include<string.h> //for memset
 
#include<sys/socket.h>
#include<arpa/inet.h> // for inet_ntoa()
#include<net/ethernet.h>
#include<netinet/ip_icmp.h>   //Provides declarations for icmp header
#include<netinet/udp.h>   //Provides declarations for udp header
#include<netinet/tcp.h>   //Provides declarations for tcp header
#include<netinet/ip.h>    //Provides declarations for ip header

#include <sndfile.h>

#define numFrames 200000 

FILE *logfp;
void process_packet(u_char *, const struct pcap_pkthdr *, const u_char *);
int getSourceIP(const u_char * , int);
void writeToFile();
 
struct sockaddr_in source,dest;
int total=0,i,j,super=0; 
char *fileName;
unsigned char prev=0;
struct pcap_stat stat;
int written = 0;
int expected; 
int frequency ;
unsigned char *samples;
int main(int argc, char *argv[])
{
    pcap_t *handle; //Handle of the device that shall be sniffed
    char errbuf[100] , *devname , devs[100][100];
    int count = 1 , n;
    char *ref_file;
    if  (argc!=4){
	printf("Usage: ./capture <Interface_Name> <Output FileName> <Reference File>\n");
	exit(1);
    } 

    devname = argv[1];
    fileName = argv[2];
    ref_file = argv[3];

    SF_INFO sndInfo;
    SNDFILE *sndFile = sf_open(ref_file, SFM_READ, &sndInfo);
    if (sndFile == NULL) {
	fprintf(stderr, "Error reading source file '%s': %s\n", argv[1], sf_strerror(sndFile));
        return 1;
    }

    expected = sndInfo.frames;
    frequency = sndInfo.samplerate;
    samples = (unsigned char *) malloc(expected * sizeof(unsigned char));

    logfp = fopen("capture.log","w");
    //Open the device for sniffing
    printf("Opening device %s for sniffing ... " , devname);
    handle = pcap_open_live(devname , 65536 , 1 , 0 , errbuf);
     
    if (handle == NULL) 
    {
        fprintf(stderr, "Couldn't open device %s : %s\n" , devname , errbuf);
        exit(1);
    }
    printf("Done\n");


    /*
    pcap_set_snaplen(handle, 65535);
    pcap_set_promisc(handle, 0);
    pcap_set_timeout(handle, 1000);
    pcap_set_buffer_size(handle, 16<<26); // 16MB
    pcap_activate(handle);

    char filter_exp[] = "udp";
    struct bpf_program fp;	
    bpf_u_int32 mask;
    bpf_u_int32 net;

    if (pcap_lookupnet(devname, &net, &mask, errbuf) == -1) {
		fprintf(stderr, "Couldn't get netmask for device %s: %s\n",
		    devname, errbuf);
		net = 0;
		mask = 0;
    }

    printf("Device: %s\n", devname);
    printf("Filter expression: %s\n", filter_exp);

    if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n",
		    filter_exp, pcap_geterr(handle));
		exit(EXIT_FAILURE);
    }

     if (pcap_setfilter(handle, &fp) == -1) {
		fprintf(stderr, "Couldn't install filter %s: %s\n",
		    filter_exp, pcap_geterr(handle));
		exit(EXIT_FAILURE);
    }

    if(pcap_stats(handle, &stat) < 0){
	    fprintf(stderr,"\nError setting the mode.\n");
	    pcap_close(handle);
	    return;
    }
    */

    //Clearing the values
    memset(samples, 0, expected);     

    //Put the device in sniff loop
    pcap_loop(handle , -1 , process_packet , NULL);
     
    return 0;   
}
 
void process_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *buffer)
{
    int size = header->len;
     
    //Get the IP Header part of this packet , excluding the ethernet header
    struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
    if (iph->protocol==17){
	super++;
	int octet = getSourceIP(buffer,size);
	/*Hack for consequtive samples*/

	/*
	if (super%10000==0){
			//printf("\nReceivedL %d", stat.ps_recv);
			//printf("\nReceived %d", total);
			//printf("\nDrop is %d", stat.ps_drop); 
			//printf("\nIDrop is%d", stat.ps_ifdrop);
			//fprintf(logfp,"\nSuper is%d", super);
    			fprintf(logfp,"\n%d\n", total);
			fflush(logfp);
	}
	*/
	if(octet!=prev){
		samples[total] = octet;
		total++;
		prev = octet;
    		//fprintf(logfp,"\n%d", octet);
	}
    } 

    if (total==expected && written==0){//numFrames){
	printf("\nTotal is:%d", total);
	writeToFile();
	written=1;
	int j;
	for(j=0;j<expected;j++)
		fprintf(logfp,"%d %u\n", j,samples[j]);
	//exit(1);
    }
}
 
int getSourceIP(const u_char *Buffer , int Size)
{
    unsigned short iphdrlen;
         
    struct iphdr *iph = (struct iphdr *)(Buffer  + sizeof(struct ethhdr) );
    iphdrlen =iph->ihl*4;
     
    source.sin_addr.s_addr = iph->daddr;

    char *sourceIP = inet_ntoa(source.sin_addr); 
    int last_octet=  atoi(strrchr(sourceIP,'.')+1);   
    
    //fprintf(logfp,"\n%d", last_octet);
    return last_octet;
}
 
void writeToFile(){

        printf("\n Got the Samples. Writing to File\n");	
	// Write to a new file
	SF_INFO sfinfo_w ;
    	sfinfo_w.channels = 1;
    	sfinfo_w.samplerate = frequency;
    	sfinfo_w.format = SF_FORMAT_WAV | SF_FORMAT_PCM_U8;
	
	SNDFILE *sndFile_w = sf_open(fileName, SFM_WRITE, &sfinfo_w);
	sf_count_t count_w = sf_write_raw(sndFile_w, samples, total);//numFrames) ;
    	sf_write_sync(sndFile_w);
    	sf_close(sndFile_w);
	/*
	int i;
	for (i=0;i<numFrames;i++)
		printf("\nFrame %d is : %d", (i+1), samples[i]);
	*/
}
