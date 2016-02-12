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

#define numFrames 53499 
void process_packet(u_char *, const struct pcap_pkthdr *, const u_char *);
int getSourceIP(const u_char * , int);
void writeToFile();
 
struct sockaddr_in source,dest;
int total=0,i,j; 
unsigned char samples[numFrames];
char *fileName;
unsigned char prev=0;
 
int main(int argc, char *argv[])
{
    pcap_t *handle; //Handle of the device that shall be sniffed
    char errbuf[100] , *devname , devs[100][100];
    int count = 1 , n;
    if  (argc!=3){
	printf("Usage: ./capture <Interface_Name> <FileName>\n");
	exit(1);
    } 

    devname = argv[1];
    fileName = argv[2];
 
    //Open the device for sniffing
    printf("Opening device %s for sniffing ... " , devname);
    handle = pcap_open_live(devname , 65536 , 1 , 0 , errbuf);
     
    if (handle == NULL) 
    {
        fprintf(stderr, "Couldn't open device %s : %s\n" , devname , errbuf);
        exit(1);
    }
    printf("Done\n");

    //Clearing the values
    memset(samples, 0, numFrames);     

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
	int octet = getSourceIP(buffer,size);
	/*Hack for consequtive samples*/
	if(octet!=prev){
		printf("\n\n");
		samples[total] = octet;
		total++;
		prev = octet;
		if (total%100==0)
			printf("\nReceived %d", total);
	}
    } 

    if (total==numFrames){
	writeToFile();
	exit(1);
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
    
    printf("\n%d", last_octet);
    return last_octet;
}
 
void writeToFile(){

        printf("\n Got the Samples. Writing to File\n");	
	// Write to a new file
	SF_INFO sfinfo_w ;
    	sfinfo_w.channels = 1;
    	sfinfo_w.samplerate = 8000;
    	sfinfo_w.format = SF_FORMAT_WAV | SF_FORMAT_PCM_U8;
	
	SNDFILE *sndFile_w = sf_open(fileName, SFM_WRITE, &sfinfo_w);
	sf_count_t count_w = sf_write_raw(sndFile_w, samples, numFrames) ;
    	sf_write_sync(sndFile_w);
    	sf_close(sndFile_w);
	int i;
	for (i=0;i<numFrames;i++)
		printf("\nFrame %d is : %d", (i+1), samples[i]);
}
