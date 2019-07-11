#include <arpa/inet.h>
#include <stdio.h>
using namespace std;

#define RUNTIME 0 // program running time, unit: second
#define MAXPACKET 1000*1000*1000/1500*RUNTIME // program capture MAXPACKET then exit
#define MAXIPLIST 64
#define INTERVAL 2

#define IPADDLEN 40	// max ipv4/ipv6 address str length
#define IPV4ADDLEN 32
#define HTLEN 256*256
#define RDPPORT 3389
#define VNCPORT 5900
#define MAXVDEKTOP 50

extern int myPacketCount;
//extern unsigned int trafficAmount;
extern char * ipList[MAXIPLIST];
extern unsigned int ipListLen;
extern bool closeSignal;
extern char ifName[IFNAMSIZ];

struct vDesktop{
	struct vDesktop *next;
	char ipAdd[IPADDLEN];	// VM ip address in string
	struct in_addr addr;		// VM ip address in u_32
	struct in_addr clientAddr;	// client ip address in u_32
	unsigned int upLink;		// Bytes
	unsigned int downLink;	// Bytes
	float rtt;
};

extern struct vDesktop **vDesktopHash;

int initial();
int parse_paras(int argc, char * argv[]);
float parsePingRtt(const char * buf);
void *myTimer(void *pInterval);
void freeMalloc();