#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>	//memset
#include <stdlib.h>

#include <unistd.h>	//close
#include <netinet/in.h>
#include <net/if.h>// struct ifreq
#include <sys/ioctl.h> // ioctl, SIOCGIFADDR
#include <sys/socket.h> // socket
#include <netinet/ether.h> // ETH_P_ALL
#include <netpacket/packet.h> // struct sockaddr_ll
#include <linux/ip.h> //iphdr
#include <arpa/inet.h> // inet_ntoa

#define NIC "eth0"
#define IPADDLEN 40	// max ipv4/ipv6 address str length
#define IPV4ADDLEN 32
#define MAXIPLIST 32	// max virtual machine number under monitor
using namespace std;

char *ipList[MAXIPLIST];

int raw_init (const char *device)
{
    int raw_socket;

    /* Open A Raw Socket */
    if ((raw_socket = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 1)
    {
        printf ("ERROR: Could not open socket, Got #?\n");
        return -1;
    }

	struct ifreq ifr;
    memset (&ifr, 0, sizeof (struct ifreq));

    /* Set the device to use */
    strcpy (ifr.ifr_name, device);

    /* Get the current flags that the device might have */
    if (ioctl (raw_socket, SIOCGIFFLAGS, &ifr) == -1)
    {
        perror ("Error: Could not retrive the flags from the device.\n");
        return -1;
    }

	if( ifr.ifr_flags & IFF_PROMISC != IFF_PROMISC){
		printf("IFF_PROMISC not set.\n");
	}

    /* Set the old flags plus the IFF_PROMISC flag */
    ifr.ifr_flags |= IFF_PROMISC;
    if (ioctl (raw_socket, SIOCSIFFLAGS, &ifr) == -1)
    {
        perror ("Error: Could not set flag IFF_PROMISC");
        return -1;
    }
    
	if( ifr.ifr_flags & IFF_PROMISC == IFF_PROMISC){
		printf ("Entering promiscuous mode\n");
	}


    /* Configure the device */

    if (ioctl (raw_socket, SIOCGIFINDEX, &ifr) < 0)
    {
        perror ("Error: Error getting the device index.\n");
        return -1;
    }

    return raw_socket;
}

int main(int argc, char * argv[]){
	/*
	option				example
	-i		interface		eth0
	-ip		ip address	192.168.122.1
	-f		files			ip_list.txt
	*/

	memset(ipList,0,sizeof(ipList));

	char ifName[IFNAMSIZ];
	FILE *fp;
	if(argc<=1 || (argc%2==0) ){
		printf("Input parameters number error!\n");
		return -1;
	}
	for(int i=1;i<argc;i++){
		if(strcmp(argv[i],"-i")==0){
			strcpy(ifName,argv[++i]);	
		}
		else if(strcmp(argv[i],"-ip")==0){
			for(int j=0;j<MAXIPLIST;j++){
				if(ipList[j]==0){
					ipList[j]=(char *)malloc(IPADDLEN*sizeof(char));
					memset(ipList[j],0,IPADDLEN);
					strcpy(ipList[j],argv[++i]);
					break;
				}
			}
		}
		else if(strcmp(argv[i],"-f")==0){
			char *fileName=argv[++i];
			fp=fopen(fileName,"r");
			if(!fp){
				perror("file open error: ");
				return -1;			
			}
			while(!feof(fp)){
				char ipAdd[IPADDLEN];
				memset(ipAdd,0,IPADDLEN);
				fscanf(fp,"%s\n",&ipAdd);
				printf("%s\n",ipAdd);				
			}			
			fclose(fp);
			printf("Todo: Read file.\n");
			return 1;
		}
		else{
			printf("Input parameters error!\n");
			return -1;
		}
	}

	//int sock=raw_init("eth0");
	int sock=raw_init(ifName);

	unsigned char *buffer = (unsigned char *) malloc(65536); //to receive data
	memset(buffer,0,65536);

	int count = 10;
	while(1){
		//Receive a network packet and copy in to buffer
		printf("Start recvfrom:\n");
		ssize_t buflen=recvfrom(sock,buffer,65536,0,NULL,NULL);
		if(buflen<0){
			perror("recvfrom: ");
			return -1;
		}
		printf("Recv raw packet successfully.\n");

		//unsigned short iphdrlen;
		struct iphdr *ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));
		struct sockaddr_in source, dest;
		memset(&source, 0, sizeof(source));
		source.sin_addr.s_addr = ip->saddr;
		memset(&dest, 0, sizeof(dest));
		dest.sin_addr.s_addr = ip->daddr;

		char srcIP[IPADDLEN]={0};
		char dstIP[IPADDLEN]={0};
		strcpy(srcIP, inet_ntoa(source.sin_addr));
		strcpy(dstIP, inet_ntoa(dest.sin_addr));

		for(int j=0;j<MAXIPLIST;j++){
			if( strcmp(ipList[j],srcIP)==0 || strcmp(ipList[j],dstIP)==0  || ipList[j]==0){
				printf("src:%s  -> dst:%s\n",srcIP,dstIP);
				break;
			}
		}

		count--;
		if(count<0){break;}
	}

	printf("finish!\n");
	close(sock);
	for(int j=0;j<MAXIPLIST;j++){
		if(ipList[j]!=0){
			printf("free pointer: %s\n",ipList[j]);
			free(ipList[j]);
		}
	}
	return 0;
}
