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

int initial(){
	// Initial ipList
	for(int i=0;i<MAXIPLIST;i++){
		ipList[i]=(char *)malloc(sizeof(char)*IPADDLEN);
		memset(ipList[i],0,sizeof(char)*IPADDLEN);
	}
	
    // Initial vDesktop hash table
    vDesktopHash=(struct vDesktop **)malloc(sizeof(struct vDesktop *)*HTLEN);
    memset(vDesktopHash,0,sizeof(struct vDesktop *)*HTLEN);
    
    for(int i=0;i<HTLEN;i++){
        vDesktopHash[i]=(struct vDesktop *)malloc(sizeof(struct vDesktop));
        memset(vDesktopHash[i],0,sizeof(struct vDesktop));
        vDesktopHash[i]->next=NULL;
	}

    return 0;
}

int parse_paras(int argc, char * argv[]){
	/*
	option				example
	-i		interface		eth0
	-ip		ip address	192.168.122.1
	-f		files			ip_list.txt
	*/

	FILE *fp;
	if(argc<=1 || (argc%2==0) ){
		printf("Input parameters number error! argc=%d\n",argc);
		return -1;
	}
	bool iFlag=false;
	for(int i=1;i<argc;i++){
		if(strcmp(argv[i],"-i")==0){
			strcpy(ifName,argv[++i]);
			iFlag = true;
		}
		else if(strcmp(argv[i],"-ip")==0){
            i++;
            // calculate the hash table index
            struct in_addr inp;
            inet_aton(argv[i],&inp);
            unsigned int index=htonl(inp.s_addr)&0xFFFF;
            ////printf("index:%u\n",index);
			
            // find the back of list
			struct vDesktop * vListTmp=vDesktopHash[index];
			while(vListTmp->next!=NULL){
				vListTmp=vListTmp->next;
			}

			// add new block
			vListTmp->next=(struct vDesktop *)malloc(sizeof(struct vDesktop));
            memset(vListTmp->next,0,sizeof(struct vDesktop));
            vListTmp->next->next=NULL;

			// insert ip add
			strcpy(vListTmp->ipAdd,argv[i]);
			strcpy(ipList[ipListLen++],argv[i]);
			inet_aton(vListTmp->ipAdd,&(vListTmp->addr));
			////printf("Test IP:%s\n",inet_ntoa(vListTmp->addr));
		}
		else if(strcmp(argv[i],"-f")==0){
            i++;
			char *fileName=argv[i];
			fp=fopen(fileName,"r");
			if(!fp){
				perror("file open error: ");
				return -1;			
			}

			// add ipAdd from file
			while(!feof(fp)){
                char charTmp[IPADDLEN];
                fscanf(fp,"%s\n",charTmp);

                // calculate the hash table index
                struct in_addr inp;
                inet_aton(charTmp,&inp);
                unsigned int index=htonl(inp.s_addr)&0xFFFF;
                ////printf("index:%u\n",index);
                
                // find the back of list
                struct vDesktop * vListTmp=vDesktopHash[index];
                while(vListTmp->next!=NULL){
                    vListTmp=vListTmp->next;
                }

                // add new block
                vListTmp->next=(struct vDesktop *)malloc(sizeof(struct vDesktop));
                memset(vListTmp->next,0,sizeof(struct vDesktop));
                vListTmp->next->next=NULL;

                // insert ip add
                strcpy(vListTmp->ipAdd,charTmp);
				strcpy(ipList[ipListLen++],charTmp);
                inet_aton(vListTmp->ipAdd,&(vListTmp->addr));
                ////printf("Test IP:%s\n",inet_ntoa(vListTmp->addr));
			}
			fclose(fp);
		}
		else{
			printf("Input parameters error! parameter [%d]:%s nonexists\n",i,argv[i]);
			return -1;
		}
	}

	if(iFlag==true){return 0;}
	else{printf("Please designate an interface.\n");return -1;}

}