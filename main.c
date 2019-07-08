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

struct vDesktop{
	struct vDesktop *next;
	char ipAdd[IPADDLEN];
	unsigned int upLink;
	unsigned int downLink;
	float delay;
};

struct vDesktop *vList;		// Initial state (empty list): pointer -> vDesktop -> NULL
char ifName[IFNAMSIZ];
bool signal=false;

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

int parse_paras(int argc, char * argv[]){
	/*
	option				example
	-i		interface		eth0
	-ip		ip address	192.168.122.1
	-f		files			ip_list.txt
	*/	

	// Initial vDesktop list
	vList=(vDesktop *)malloc(sizeof(struct vDesktop));
	vList->next=NULL;
	memset(vList->ipAdd,0,IPADDLEN);
	vList->upLink=vList->downLink=vList->delay=0;

	FILE *fp;
	if(argc<=1 || (argc%2==0) ){
		printf("Input parameters number error!\n");
		return -1;
	}
	bool iFlag=false;
	for(int i=1;i<argc;i++){
		if(strcmp(argv[i],"-i")==0){
			strcpy(ifName,argv[++i]);
			iFlag = true;
		}
		else if(strcmp(argv[i],"-ip")==0){
			// find the back of list
			struct vDesktop * vListTmp=vList;
			while(vListTmp->next!=NULL){
				vListTmp=vListTmp->next;
			}
			// add new slow
			vListTmp->next=(vDesktop *)malloc(sizeof(struct vDesktop));
			vListTmp->next->next=NULL;
			memset(vListTmp->next->ipAdd,0,IPADDLEN);
			vListTmp->upLink=vListTmp->downLink=vListTmp->delay=0;

			// insert ip add
			strcpy(vListTmp->ipAdd,argv[++i]);
		}
		else if(strcmp(argv[i],"-f")==0){
			char *fileName=argv[++i];
			fp=fopen(fileName,"r");
			if(!fp){
				perror("file open error: ");
				return -1;			
			}

			// add ipAdd from file
			struct vDesktop * vListTmp=vList;
			while(!feof(fp)){
				fscanf(fp,"%s\n",vListTmp->ipAdd);
				printf("%s\n",vListTmp->ipAdd);
				vListTmp->next=(vDesktop *)malloc(sizeof(struct vDesktop));
				vListTmp->next->next=NULL;
				memset(vListTmp->next->ipAdd,0,IPADDLEN);
				vListTmp=vListTmp->next;
			}			
			fclose(fp);
		}
		else{
			printf("Input parameters error!\n");
			return -1;
		}
	}

	if(iFlag==true){return 0;}
	else{printf("Please designate an interface.\n");return -1;}
}

int main(int argc, char * argv[]){

	if(parse_paras(argc, argv)<0){
		return -1;
	}

	int sock=raw_init(ifName);

	unsigned char *buffer = (unsigned char *) malloc(65536); //to receive data
	memset(buffer,0,65536);

	int count = 128;
	printf("Start recvfrom:\n");
	while(1){
		if(signal==true){
			// calculate the traffic volume
			struct vDesktop * vListTmp=vList;
			while(vListTmp->next!=NULL){
				float upLink=vListTmp->upLink/INTERVAL/8;
				float downLink=vListTmp->downLink/INTERVAL/8;
				printf("ip:%s\tupLink:%u Mbps\tdownlink:%u Mbps\n",vListTmp->ipAdd,upLink,downLink);
				vListTmp=vListTmp->next;
			}
			signal = false;
		}

		//Receive a network packet and copy in to buffer
		ssize_t buflen=recvfrom(sock,buffer,65536,0,NULL,NULL);
		if(buflen<0){
			perror("recvfrom: ");
			return -1;
		}
		////printf("Recv raw packet successfully.\n");

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

		// find the matched ip
		struct vDesktop *vListTmpPre=vList;
		struct vDesktop *vListTmpLat=vList->next;
		if(vListTmpLat==NULL){printf("vDesktop list empty.\n");continue;}// Empty vDesktop list
		while(vListTmpLat!=NULL){
			////printf("src:%s  -> dst:%s\n", srcIP, dstIP);
			// update vDesktop traffic statistics
			if(strcmp(vListTmpPre->ipAdd, srcIP)==0 ){
				vListTmpPre->upLink+=ntohs(ip->tot_len);
				printf("ip:%s\tuplink:%u Bytes\n",vListTmpPre->ipAdd,vListTmpPre->upLink);
				break;
			}
			else if(strcmp(vListTmpPre->ipAdd, dstIP)==0 ){
				vListTmpPre->downLink+=ntohs(ip->tot_len);
				printf("ip:%s\tdownlink:%u Bytes\n",vListTmpPre->ipAdd,vListTmpPre->downLink);
				break;
			}
			// continue find matched ip
			else{
				vListTmpPre=vListTmpLat;
				vListTmpLat=vListTmpLat->next;
			}
		}

		count--;
		if(count<0){break;}
	}

	printf("finish!\n");
	close(sock);

	// free vDesktop list
	struct vDesktop *vListTmpPre=vList;
	struct vDesktop *vListTmpLat=vList->next;
	while(vListTmpLat!=NULL){
		printf("free vDesktop %s\n",vListTmpPre->ipAdd);
		free(vListTmpPre);
		vListTmpPre=vListTmpLat;
		vListTmpLat=vListTmpLat->next;
	}
	free(vListTmpPre);
	return 0;
}