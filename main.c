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
    size_ip = IP_HL(ip)*4;
	if (size_ip < 20) {
		printf("   * Invalid IP header length: %u bytes\n", size_ip);
		return;
	}
    // The TCP header
    const struct sniff_tcp *tcp = (struct sniff_tcp*)(packet + SIZE_ETHERNET + size_ip);
    size_tcp = TH_OFF(tcp)*4;
	if (size_tcp < 20) {
		printf("   * Invalid TCP header length: %u bytes\n", size_tcp);
		return;
	}

    ////printf("tcp_sport:%u\t tcp_dport:%u\n",ntohs(tcp->th_sport),ntohs(tcp->th_dport));

    // find the matched table
    ////printf("ip_src:%s\n",inet_ntoa(ip->ip_src));

    // upLink
    u_short port = ntohs(tcp->th_sport);
    if(  port == RDPPORT || (port >= VNCPORT && port <= VNCPORT+MAXVDEKTOP) ){
        unsigned int index=ntohl(ip->ip_src.s_addr)&0xFFFF;
        struct vDesktop * vListTmp=vDesktopHash[index];
        bool fFlag = false;
        while(vListTmp->next!=NULL){
            if(  0 == memcmp(&vListTmp->addr,&ip->ip_src,sizeof(struct in_addr)*2) && 0 == memcmp(&vListTmp->th_sport,&tcp->th_sport,sizeof(u_short)*2) ){
                // match
                fFlag=true;
                // analyses
                vListTmp->upLink+=pkthdr->len;
                if(vListTmp->state==UNCONNECTED){
                    // first capture the tcp segment
                    ////vListTmp->clientAddr=ip->ip_dst;
                    ////vListTmp->th_sport=tcp->th_sport;
                    ////vListTmp->th_dport=tcp->th_dport;
                    vListTmp->state=CONNECTED;
                }else if(vListTmp->state==CONNECTED){
                    ////printf("captured tcp seq:%u\n",ntohl(tcp->th_seq) );
                    vListTmp->th_seq=ntohl(tcp->th_seq) + pkthdr->len - SIZE_ETHERNET - size_ip - size_tcp;
                    ////printf("desired ack seq:%u\n",vListTmp->th_seq );
                    vListTmp->ts=pkthdr->ts;
                    ////printf("ts:%d,%d\n",vListTmp->ts.tv_sec,vListTmp->ts.tv_usec);
                    vListTmp->state=WAITACK;
                }else if(vListTmp->state==WAITACK){
                    // do nothing
                }else if(vListTmp->state!=PAUSE){
                    printf("Error capture state.\n");
                    vListTmp->state=UNCONNECTED;
                }else{
                    // different port
                }
                break;
            }else{
                vListTmp=vListTmp->next;
            }
        }
        if(fFlag==false){
            // add new block
			vListTmp->next=(struct vDesktop *)malloc(sizeof(struct vDesktop));
            memset(vListTmp->next,0,sizeof(struct vDesktop));
            vListTmp->next->next=NULL;

            // fill in information
            memcpy(&vListTmp->addr,&ip->ip_src,sizeof(struct in_addr)*2);
            memcpy(&vListTmp->th_sport,&tcp->th_sport,sizeof(u_short)*2);
        }
    }

    // downLink
    port = ntohs(tcp->th_dport);
    if( port == RDPPORT || (port >= VNCPORT && port <= VNCPORT+MAXVDEKTOP) ){
        unsigned int index=ntohl(ip->ip_dst.s_addr)&0xFFFF;
        struct vDesktop *vListTmp=vDesktopHash[index];
        bool fFlag = false;
        while(vListTmp->next!=NULL){
            if( vListTmp->addr.s_addr == ip->ip_dst.s_addr && vListTmp->clientAddr.s_addr == ip->ip_src.s_addr && vListTmp->th_sport == tcp->th_dport && vListTmp->th_dport == tcp->th_sport ){             
                // match
                fFlag=true;
                // analyses
                vListTmp->downLink+=pkthdr->len;
                if(vListTmp->state==WAITACK){
                    ////printf("get downLink packet, ack: %u\n",htonl(tcp->th_ack));
                    if(tcp->th_ack == htonl(vListTmp->th_seq)){
                        ////printf("get ack\n");
                        vListTmp->te=pkthdr->ts;
                        ////printf("te:%d,%d\n",vListTmp->te.tv_sec,vListTmp->te.tv_usec);
                        vListTmp->state=PAUSE;
                    }
                }
                break;
            }else{
                vListTmp=vListTmp->next;
            }
        }
        if(fFlag==false){
            
            // add new block
			vListTmp->next=(struct vDesktop *)malloc(sizeof(struct vDesktop));
            memset(vListTmp->next,0,sizeof(struct vDesktop));
            vListTmp->next->next=NULL;

            // fill in information
            vListTmp->addr.s_addr = ip->ip_dst.s_addr;
            vListTmp->clientAddr.s_addr = ip->ip_src.s_addr;
            vListTmp->th_sport = tcp->th_dport;
            vListTmp->th_dport = tcp->th_sport;
            
        }
    }

/*
    // upLink
    int index=ntohl(ip->ip_src.s_addr)&0xFFFF;
    struct vDesktop * vListTmp=vDesktopHash[index];
    bool fFlag=false;
    while(vListTmp->next!=NULL){
        if(  (ip->ip_src.s_addr == vListTmp->addr.s_addr && ip->ip_dst.s_addr == vListTmp->clientAddr.s_addr) || vListTmp->clientAddr.s_addr == 0 ){
            // match
            ////printf("uplink\n");

            vListTmp->upLink+=pkthdr->len;

            if(vListTmp->state==UNCONNECTED){
                // first capture the tcp segment
                vListTmp->clientAddr=ip->ip_dst;
                vListTmp->th_sport=tcp->th_sport;
                vListTmp->th_dport=tcp->th_dport;
                vListTmp->state=CONNECTED;
            }else if(vListTmp->state==CONNECTED && tcp->th_sport == vListTmp->th_sport && tcp->th_dport == vListTmp->th_dport){
                ////printf("captured tcp seq:%u\n",ntohl(tcp->th_seq) );
                vListTmp->th_seq=ntohl(tcp->th_seq) + pkthdr->len - SIZE_ETHERNET - size_ip - size_tcp;
                ////printf("desired ack seq:%u\n",vListTmp->th_seq );
                vListTmp->ts=pkthdr->ts;
                ////printf("ts:%d,%d\n",vListTmp->ts.tv_sec,vListTmp->ts.tv_usec);
                vListTmp->state=WAITACK;
            }else if(vListTmp->state==WAITACK && tcp->th_sport == vListTmp->th_sport && tcp->th_dport == vListTmp->th_dport){
                // do nothing
            }else if(vListTmp->state!=PAUSE && tcp->th_sport == vListTmp->th_sport && tcp->th_dport == vListTmp->th_dport){
                printf("Error capture state.\n");
                vListTmp->state=UNCONNECTED;
            }else{
                // different port
            }

            fFlag=true;
            break;
        }else{
            vListTmp=vListTmp->next;
        }
    }

    // downLink
    if(fFlag==false){
        index=ntohl(ip->ip_dst.s_addr)&0xFFFF;
        vListTmp=vDesktopHash[index];
        while(vListTmp->next!=NULL){
            if(  (ip->ip_dst.s_addr == vListTmp->addr.s_addr && ip->ip_src.s_addr == vListTmp->clientAddr.s_addr) ){
                // match
                ////printf("downlink\n");

                vListTmp->downLink+=pkthdr->len;

                if(vListTmp->state==WAITACK && tcp->th_sport == vListTmp->th_dport && tcp->th_dport == vListTmp->th_sport){
                    ////printf("get downLink packet, ack: %u\n",htonl(tcp->th_ack));
                    if(tcp->th_ack == htonl(vListTmp->th_seq)){
                        ////printf("get ack\n");
                        vListTmp->te=pkthdr->ts;
                        ////printf("te:%d,%d\n",vListTmp->te.tv_sec,vListTmp->te.tv_usec);
                        vListTmp->state=PAUSE;
                    }
                }

                fFlag=true;
                break;
            }else{
                vListTmp=vListTmp->next;
            }
        }
    }

    // Mismatching
    //if(fFlag==false){
    //    printf("Mismatching.\n");
    //}

    ////printf("end\n");
    //trafficAmount+=pkthdr->len;
*/
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
    sprintf(filterSyntax, "portrange %u-%u || port %u", VNCPORT, VNCPORT+MAXVDEKTOP, RDPPORT);
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