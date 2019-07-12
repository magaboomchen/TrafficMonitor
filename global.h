#include <arpa/inet.h>
#include <stdio.h>

using namespace std;

#define RUNTIME 0 // program running time, unit: second
#define MAXPACKET 1000*1000*1000/1500*RUNTIME // program capture MAXPACKET then exit
#define MAXIPLIST 64
#define INTERVAL 1

#define IPADDLEN 40	// max ipv4/ipv6 address str length
#define IPV4ADDLEN 32
#define HTLEN 256*256
#define RDPPORT 3389
#define VNCPORT 5900
#define MAXVDEKTOP 50

// UNCONNECTED -> CONNECTED ----> WAITACK
// 					  ^		        |				
// 					  |_____________|
#define UNCONNECTED 0	// Haven't capture packets
#define CONNECTED 1		// Have capture packets, but haven't get a seq number; 1->2: capture a seq number
#define WAITACK 2		// Have got a seq number, ready to get the ack; 2->1: capture an ACK
#define PAUSE 4			// Pause the rtt measurement

extern int myPacketCount;
//extern unsigned int trafficAmount;
extern char * ipList[MAXIPLIST];
extern unsigned int ipListLen;
extern bool closeSignal;
extern char ifName[IFNAMSIZ];

struct vDesktop{
	struct vDesktop *next;
	////char ipAdd[IPADDLEN];		// VM ip address in string

	// TCP connection information
	struct in_addr addr;		// VM ip address in u_32
	struct in_addr clientAddr;	// client ip address in u_32
	u_short th_sport;			// VM tcp port
	u_short th_dport;			// client tcp port

	// rtt measurements
	int state;					// state machine: state of rtt measurment
	u_int th_seq;				// VM tcp sequence number
	struct timeval ts;			// time start
	struct timeval te;			// time end

	// measurement results
	unsigned int upLink;		// Bytes
	unsigned int downLink;		// Bytes
	float rtt;					// Round-trip time
};

extern struct vDesktop **vDesktopHash;

int initial();
int parse_paras(int argc, char * argv[]);
float parsePingRtt(const char * buf);
void *myTimer(void *pInterval);
void freeMalloc();