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
				if(vListTmp->addr.s_addr ==in.s_addr){
					// match!
					float upLink=vListTmp->upLink/interval;
					////float downLink=vListTmp->downLink/interval;
					printf("ip:%s\tupTransfer:%u KBytes\tupLink:%f Mbps\t\n",vListTmp->ipAdd,(unsigned int)(upLink/1000.0),upLink/1000.0/1000.0*8.0);
					vListTmp->upLink=vListTmp->downLink=vListTmp->delay=0;
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
