#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
using namespace std;

int main(){
	int sock_r = socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
	if(sock_r<0){
		perror("Create Raw Socket: ");
		return -1;
	}

	unsigned char *buffer = (unsigned char *) malloc(65536); //to receive data
	memset(buffer,0,65536);
	struct sockaddr saddr;
	int saddr_len = sizeof (saddr);
	
	int count = 10;

	while(1){
		//Receive a network packet and copy in to buffer
		buflen=recvfrom(sock_r,buffer,65536,0,&saddr,(socklen_t *)&saddr_len);
		if(buflen<0){
			perror("recvfrom: ");
			return -1;
		}

		unsigned short iphdrlen;
		struct iphdr *ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));
		memset(&source, 0, sizeof(source));
		source.sin_addr.s_addr = ip->saddr;
		memset(&dest, 0, sizeof(dest));
		dest.sin_addr.s_addr = ip->daddr;

		char * srcIP = inet_ntoa(source.sin_addr);
		char * dstIP = inet_ntoa(dest.sin_addr));
		printf("src:%s  -> dst:%s\n",srcIP,dstIP);
		
/*
		fprintf(log_txt, \t|-Version : %d\n,(unsigned int)ip->version);
		 
		fprintf(log_txt , \t|-Internet Header Length : %d DWORDS or %d Bytes\n,(unsigned int)ip->ihl,((unsigned int)(ip->ihl))*4);
		 
		fprintf(log_txt , \t|-Type Of Service : %d\n,(unsigned int)ip->tos);
		 
		fprintf(log_txt , \t|-Total Length : %d Bytes\n,ntohs(ip->tot_len));
		 
		fprintf(log_txt , \t|-Identification : %d\n,ntohs(ip->id));
		 
		fprintf(log_txt , \t|-Time To Live : %d\n,(unsigned int)ip->ttl);
		 
		fprintf(log_txt , \t|-Protocol : %d\n,(unsigned int)ip->protocol);
		 
		fprintf(log_txt , \t|-Header Checksum : %d\n,ntohs(ip->check));
		 
		fprintf(log_txt , \t|-Source IP : %s\n, inet_ntoa(source.sin_addr));
		 
		fprintf(log_txt , \t|-Destination IP : %s\n,inet_ntoa(dest.sin_addr));
*/

		count--;
		if(count<0){break;}
	}

	return 0;
	
}
