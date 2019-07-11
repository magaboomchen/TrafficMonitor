/*
reference
https://www.tcpdump.org/pcap.html
 */

#include <pcap.h>
#include <iostream>

#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>	// memset
#include <stdlib.h>	// malloc, free, exit

#include <pthread.h>

#include <unistd.h>	//close
#include <netinet/in.h>
#include <net/if.h>// struct ifreq
#include <sys/ioctl.h> // ioctl, SIOCGIFADDR
#include <sys/socket.h> // socket
#include <netinet/ether.h> // ETH_P_ALL, ETHER_ADDR_LEN
#include <netpacket/packet.h> // struct sockaddr_ll
#include <linux/ip.h> //iphdr
#include <arpa/inet.h> // inet_ntoa, in_addr

#include <signal.h> // ctrl-c, free RAM

#include "global.h"
#include "networkHeader.h"

using namespace std;

int myPacketCount = 0;
//unsigned int trafficAmount=0;
char * ipList[MAXIPLIST];
unsigned int ipListLen=0;
bool closeSignal=false;
char ifName[IFNAMSIZ];
struct vDesktop **vDesktopHash;

void freeMalloc(){
    // free vList
    for(int i=0;i<MAXIPLIST;i++){
        if(ipList[i]!=NULL){
            ////printf("free ip:%s\n",ipList[i]);
            free(ipList[i]);
        }
    }

    // free vDesktop
    for(int i=0;i<HTLEN;i++){
        free(vDesktopHash[i]);
    }
    free(vDesktopHash);
}

void ctrlc_message(int s) //ctrl+c
{
    printf("caught signal %d\n",s);
    freeMalloc();
    exit(1);
}

void packetHandler(u_char *userData, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{

    ////printf("%d packet(s) captured\n",++myPacketCount);
    //++myPacketCount;

    ////printf("Packet length: %d\n", pkthdr->len);
    ////printf("Number of bytes: %d\n", pkthdr->caplen);
    ////printf("Recieved time: %s", ctime((const time_t *)&pkthdr->ts.tv_sec));

    // The IP header
	const struct sniff_ip *ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
    ////printf("ip_src:%s\n",inet_ntoa(ip->ip_src));
    int index=ntohl(ip->ip_src.s_addr)&0xFFFF;

    // The TCP header
    size_ip = IP_HL(ip)*4;
    const struct sniff_tcp *tcp = (struct sniff_tcp*)(packet + SIZE_ETHERNET + size_ip);

    // find the matched table
    struct vDesktop * vListTmp=vDesktopHash[index];
    while(vListTmp->next!=NULL){
        //if(  vListTmp->addr.s_addr ==ip->ip_src.s_addr  && tcp->th_dport == htons(u_short(5201))  ){
        if(  vListTmp->addr.s_addr ==ip->ip_src.s_addr ){
            // match!
            vListTmp->upLink+=pkthdr->len;
            vListTmp->clientAddr=ip->ip_dst;
            break;
        }
        vListTmp=vListTmp->next;
    }

    //trafficAmount+=pkthdr->len;

}

int main(int argc, char * argv[])
{
    // initial
    initial();

    // parse input parameters
	if(parse_paras(argc, argv)<0){
		return -1;
	}

    // catch ctrl-c
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = ctrlc_message;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT,&sigIntHandler,NULL);

    // start myTimer
	pthread_t thread1;
	int interval = INTERVAL;	// unit: second
	pthread_create(&thread1,NULL, &myTimer, &interval);

    // set the interface, descriptor, error buffer
    char *dev;
    pcap_t *descr;
    char errbuf[PCAP_ERRBUF_SIZE];

    
    dev = pcap_lookupdev(errbuf);
    if (dev == NULL)
    {
        printf("pcap_lookupdev() failed: %s\n",errbuf);
        return 1;
    }
    

    // set the dev name
    dev=ifName;
    printf("dev: %s\n",dev);

    // sniff the dev
    descr = pcap_open_live(dev, BUFSIZ, 1, -1, errbuf);
    if (descr == NULL)
    {
        printf("pcap_open_live() failed: %s\n",errbuf);
        return 1;
    }

    // construct a filter
    struct bpf_program filter;
    char filterSyntax[256];
    sprintf(filterSyntax, "tcp portrange %u-%u || port %u", VNCPORT, VNCPORT+MAXVDEKTOP, RDPPORT);
    printf("filtlerSyntax: %s\n",filterSyntax);
    pcap_compile(descr, &filter, filterSyntax, 1, 0);
    pcap_setfilter(descr, &filter);

    // capture packets
    if (pcap_loop(descr, MAXPACKET, packetHandler, NULL) < 0)
    {
        printf("pcap_loop() failed: %s\n", pcap_geterr(descr));
        return 1;
    }

    printf("capture finished\n");

    pcap_close(descr);
	pthread_join(thread1,NULL);

    return 0;
}