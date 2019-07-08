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
#include <arpa/inet.h> // inet_ntoa

#define NIC "eth0"
#define IPADDLEN 40	// max ipv4/ipv6 address str length
#define IPV4ADDLEN 32
#define MAXIPLIST 32	// max virtual machine number under monitor
#define RUNTIME 5 // program running time, unit: second
using namespace std;

struct vDesktop{
	struct vDesktop *next;
	char ipAdd[IPADDLEN];
	in_addr addr;
	unsigned int upLink;		// Bytes
	unsigned int downLink;	// Bytes
	float delay;
};

struct vDesktop *vList;		// Initial state (empty list): pointer -> vDesktop -> NULL
char ifName[IFNAMSIZ];
bool closeSignal=false;

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
	vList->addr.s_addr=vList->upLink=vList->downLink=vList->delay=0;

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
			vListTmp->addr.s_addr=vListTmp->upLink=vListTmp->downLink=vListTmp->delay=0;

			// insert ip add
			strcpy(vListTmp->ipAdd,argv[++i]);
			inet_aton(vListTmp->ipAdd,&(vListTmp->addr));
			////printf("Test IP:%s\n",inet_ntoa(vListTmp->addr));
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
				////printf("add ip %s from file\n",vListTmp->ipAdd);
				inet_aton(vListTmp->ipAdd,&(vListTmp->addr));
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

void *myTimer(void *pInterval){
	float interval = *(int*)pInterval;
	int sumTime=0;
	while(1){
		sleep(interval);
		if(++sumTime>RUNTIME){closeSignal=true;break;}
		// calculate the traffic volume
		struct vDesktop * vListTmp=vList;
		printf("---------------------------------------------------------------------\n");
		while(vListTmp->next!=NULL){
			float upLink=vListTmp->upLink/interval;
			float downLink=vListTmp->downLink/interval;
			////printf("ip:%s\tupLink:%f Mbps\tdownlink:%f Mbps\n",vListTmp->ipAdd,upLink/1000.0/1000.0*8.0,downLink/1000.0/1000.0*8.0);
			printf("ip:%s\tupTransfer:%u Bytes\tupLink:%f Mbps\t\n",vListTmp->ipAdd,(unsigned int)upLink,upLink/1000.0/1000.0*8.0);
			vListTmp->upLink=vListTmp->downLink=vListTmp->delay=0;
			vListTmp=vListTmp->next;
		}
	}
}

int main(int argc, char * argv[]){

	if(parse_paras(argc, argv)<0){
		return -1;
	}

	pthread_t thread1;
	int interval = 1;	// unit: second
	pthread_create(&thread1,NULL, &myTimer, &interval);

	int sock=raw_init(ifName);

	unsigned char *buffer = (unsigned char *) malloc(65536); //to receive data
	memset(buffer,0,65536);
	struct iphdr *ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));
	struct sockaddr_in source, dest;
	memset(&source, 0, sizeof(source));
	memset(&dest, 0, sizeof(dest));
	char srcIP[IPADDLEN]={0};
	char dstIP[IPADDLEN]={0};

	printf("Start recvfrom:\n");
	struct vDesktop *vListTmpPre=vList;
	struct vDesktop *vListTmpLat=vList->next;
	while(1 && closeSignal==false){
		//Receive a network packet and copy in to buffer
		ssize_t buflen=recvfrom(sock,buffer,65536,0,NULL,NULL);
		if(buflen<0){
			perror("recvfrom: ");
			return -1;
		}
		////printf("Recv raw packet successfully.\n");

		// find the matched ip
		vListTmpPre=vList;
		vListTmpLat=vList->next;
		if(vListTmpLat==NULL){printf("vDesktop list empty.\n");continue;}// Empty vDesktop list
		while(vListTmpLat!=NULL){
			////printf("src:%s  -> dst:%s\n", srcIP, dstIP);
			// update vDesktop traffic statistics
			if(vListTmpPre->addr.s_addr==ip->saddr){
				vListTmpPre->upLink+=ntohs(ip->tot_len);
				////printf("ip:%s\tuplink:%u Bytes\n",vListTmpPre->ipAdd,vListTmpPre->upLink);
				break;
			}
			////else if( vListTmpPre->ipAdd==ip->daddr ){	
				////vListTmpPre->downLink+=ntohs(ip->tot_len);
				////printf("ip:%s\tdownlink:%u Bytes\n",vListTmpPre->ipAdd,vListTmpPre->downLink);
				////break;
			////}
			// continue find matched ip
			vListTmpPre=vListTmpLat;
			vListTmpLat=vListTmpLat->next;
		}

	}

	printf("finish!\n");
	close(sock);

	// free vDesktop list
	vListTmpPre=vList;
	vListTmpLat=vList->next;
	while(vListTmpLat!=NULL){
		printf("free vDesktop %s\n",vListTmpPre->ipAdd);
		free(vListTmpPre);
		vListTmpPre=vListTmpLat;
		vListTmpLat=vListTmpLat->next;
	}
	free(vListTmpPre);
	pthread_join(thread1,NULL);
	return 0;
}