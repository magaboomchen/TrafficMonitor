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
	int sumTime=0;
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
				if(vListTmp->addr.s_addr ==in.s_addr){	// match!
					// calculate upLink
					float upLink=vListTmp->upLink/interval;
					////float downLink=vListTmp->downLink/interval;

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

					// print results
					if(upLink/1000.0/1000.0*8.0>1000){
						printf("Server/VM ip: %s\tClient ip: %s\n\tupLink:%f Gbps\trtt: %f ms\n",vListTmp->ipAdd,inet_ntoa(vListTmp->clientAddr), upLink/1000.0/1000.0/1000.0*8.0,vListTmp->rtt);
					}else{
						printf("Server/VM ip: %s\tClient ip: %s\n\tupLink:%f Mbps\trtt: %f ms\n",vListTmp->ipAdd,inet_ntoa(vListTmp->clientAddr),upLink/1000.0/1000.0*8.0,vListTmp->rtt);
					}

					// reset
					vListTmp->upLink=vListTmp->downLink=vListTmp->rtt=0;
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
