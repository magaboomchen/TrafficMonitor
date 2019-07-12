#include <pcap.h>

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
#include <netinet/ether.h> // ETH_P_ALL
#include <netpacket/packet.h> // struct sockaddr_ll
#include <linux/ip.h> //iphdr
#include <arpa/inet.h> // inet_ntoa, in_addr

#include "global.h"

using namespace std;


void *myTimer(void *pInterval){
	float interval = *(int*)pInterval;
	unsigned int sumTime=0;
	while(1){
		sleep(interval);

		if((++sumTime>RUNTIME && RUNTIME !=0) || (myPacketCount >= MAXPACKET && MAXPACKET !=0) ){break;}
		// calculate the traffic volume
		printf("---------------------------------------------------------------------\n");
        //printf("packetCount:%d\n",myPacketCount);

		for(int i=0;i<ipListLen;i++){
			struct in_addr in;
			inet_aton(ipList[i],&in);
			unsigned int index = ntohl(in.s_addr)  &0xFFFF;
			////printf("myTimer index:%u\n",index);

			// find the matched table
			struct vDesktop * vListTmp=vDesktopHash[index];
			while(vListTmp->next!=NULL){
				////printf("myTimer ip:%s\n", inet_ntoa(vListTmp->addr));
				if(vListTmp->addr.s_addr == in.s_addr){	
					// match!
					float upLink=vListTmp->upLink/interval;
					////float downLink=vListTmp->downLink/interval;

					if(vListTmp->upLink<=0){vListTmp=vListTmp->next;continue;}

					// print traffic volume results
					printf("Server/VM : %s:%u\tClient : %s:%u\n\t",
						vListTmp->ipAdd,ntohs(vListTmp->th_sport),inet_ntoa(vListTmp->clientAddr),ntohs(vListTmp->th_dport));
					if(upLink/1000.0/1000.0*8.0>1000){
						printf("upLink:%f Gbps\t",upLink/1000.0/1000.0/1000.0*8.0);
					}else{
						printf("upLink:%f Mbps\t",upLink/1000.0/1000.0*8.0);
					}

					////printf("start time: %d.%d\t end time: %d.%d\n",vListTmp->ts.tv_sec,vListTmp->ts.tv_usec, vListTmp->te.tv_sec, vListTmp->te.tv_usec);
					////printf("desired ack: %u\n",vListTmp->th_seq);

					/*  use ping to measure rtt, may be dropped by firewall
					if(vListTmp->clientAddr.s_addr!=0 && upLink>0){
						// ping for rtt
						char pingRule[256];
						sprintf(pingRule,"ping -c1 %s ",inet_ntoa(vListTmp->clientAddr));

						FILE   *stream;
						char   buf[1024];
						memset( buf, '\0', sizeof(buf) );
						stream = popen( pingRule, "r" );
						fread( buf, sizeof(char), sizeof(buf), stream);
						////printf("buf: %s\n",buf);
						pclose( stream );

						// get rtt
						vListTmp->rtt= parsePingRtt(buf);
					}
					*/

					// print rtt results
					if(vListTmp->te.tv_sec!=0){
						vListTmp->rtt= ((vListTmp->te.tv_sec-vListTmp->ts.tv_sec)*1000000+vListTmp->te.tv_usec-vListTmp->ts.tv_usec)/1000.0;
						printf("rtt: %f ms\n",vListTmp->rtt);
					}else{
						vListTmp->rtt = -1;
						printf("rtt: NULL ms\n");
					}

					// reset
					vListTmp->state=vListTmp->th_seq=vListTmp->upLink=vListTmp->downLink=vListTmp->rtt=0;
					memset(&(vListTmp->ts),0,sizeof(struct timeval));
					memset(&(vListTmp->te),0,sizeof(struct timeval));
					break;
				}
				vListTmp=vListTmp->next;
			}
		}

		//printf("trafficAmount:%u Bytes, %f Mbps\n",trafficAmount,trafficAmount/interval/1000.0/1000.0*8);
		//trafficAmount=0;
	}
    freeMalloc();
}