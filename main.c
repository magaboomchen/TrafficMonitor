#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>	//memset
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
#define IPADDLEN 40
#define IPV4ADDLEN 32

using namespace std;


/*
int main(){
	int sock_r = socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
	if(sock_r<0){
		perror("Create raw socket: ");
		return -1;
	}
	printf("Create raw socket successfully.\n");

	unsigned char *buffer = (unsigned char *) malloc(65536); //to receive data
	memset(buffer,0,65536);
	//struct sockaddr saddr;
	//int saddr_len = sizeof (saddr);
	
	//https://blog.csdn.net/tennysonsky/article/details/44676377
	// NIC IFF_PROMISC

	struct ifreq req;	
	strncpy(req.ifr_name, NIC, IFNAMSIZ);	
	if(-1 == ioctl(sock_r, SIOCGIFINDEX, &req))	
	{
		perror("ioctl SIOCGIFINDEX,: ");
		close(sock_r);
		return -1;
	}
	req.ifr_flags |= IFF_PROMISC;

	if(-1 == ioctl(sock_r, SIOCSIFFLAGS, &req))	
	{
		perror("ioctl SIOCSIFFLAGS: ");
		close(sock_r);
		return -1;
	}

	if (ioctl (sock_r, SIOCGIFINDEX, &req) < 0)
	{
		perror ("Error: Error getting the device index.");
		close(sock_r);
		return -1;
	}

	printf("Open IFF_PROMISC successfully.\n");
	
	return 0;




	int count = 10;
	while(1){
		//Receive a network packet and copy in to buffer
		//buflen=recvfrom(sock_r,buffer,65536,0,&saddr,(socklen_t *)&saddr_len);
		printf("Start recvfrom:\n");
		ssize_t buflen=recvfrom(sock_r,buffer,65536,0,NULL,NULL);
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
		////memcpy(srcIP, inet_ntoa(source.sin_addr),strlen(inet_ntoa(source.sin_addr)));
		////memcpy(dstIP, inet_ntoa(dest.sin_addr),strlen(inet_ntoa(dest.sin_addr)));

		//char * srcIP = inet_ntoa(source.sin_addr);
		//char * dstIP = inet_ntoa(dest.sin_addr);
		printf("src:%s  -> dst:%s\n",srcIP,dstIP);



		//fprintf(log_txt, \t|-Version : %d\n,(unsigned int)ip->version);
		 
		//fprintf(log_txt , \t|-Internet Header Length : %d DWORDS or %d Bytes\n,(unsigned int)ip->ihl,((unsigned int)(ip->ihl))*4);
		 
		//fprintf(log_txt , \t|-Type Of Service : %d\n,(unsigned int)ip->tos);
		 
		//fprintf(log_txt , \t|-Total Length : %d Bytes\n,ntohs(ip->tot_len));
		 
		//fprintf(log_txt , \t|-Identification : %d\n,ntohs(ip->id));
		 
		//fprintf(log_txt , \t|-Time To Live : %d\n,(unsigned int)ip->ttl);
		 
		//fprintf(log_txt , \t|-Protocol : %d\n,(unsigned int)ip->protocol);
		 
		//fprintf(log_txt , \t|-Header Checksum : %d\n,ntohs(ip->check));
		 
		//fprintf(log_txt , \t|-Source IP : %s\n, inet_ntoa(source.sin_addr));
		 
		//fprintf(log_txt , \t|-Destination IP : %s\n,inet_ntoa(dest.sin_addr));


		count--;
		if(count<0){break;}
	}

	close(sock_r);
	return 0;
	
}
*/



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



int main(){
	int sock=raw_init("eth0");
	printf("finish!\n");
	close(sock);
	return 0;
}
