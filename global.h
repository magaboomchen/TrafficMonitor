#include <arpa/inet.h>
#include <stdio.h>
using namespace std;

#define RUNTIME 0 // program running time, unit: second
#define MAXPACKET 1000*1000*1000/1500*5 // program capture MAXPACKET then exit
#define MAXIPLIST 64

#define IPADDLEN 40	// max ipv4/ipv6 address str length
#define IPV4ADDLEN 32
#define HTLEN 256*256

extern int myPacketCount;
//extern unsigned int trafficAmount;
extern char * ipList[MAXIPLIST];
extern unsigned int ipListLen;
extern bool closeSignal;
extern char ifName[IFNAMSIZ];

struct vDesktop{
	struct vDesktop *next;
	char ipAdd[IPADDLEN];
	struct in_addr addr;
	struct in_addr clientAddr;
	unsigned int upLink;		// Bytes
	unsigned int downLink;	// Bytes
	float delay;
};

extern struct vDesktop **vDesktopHash;

int initial();
int parse_paras(int argc, char * argv[]);
void *myTimer(void *pInterval);
void freeMalloc();