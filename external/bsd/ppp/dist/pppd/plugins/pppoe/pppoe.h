/***********************************************************************
*
* pppoe.h
*
* Declaration of various PPPoE constants
*
* Copyright (C) 2000 Roaring Penguin Software Inc.
*
* This program may be distributed according to the terms of the GNU
* General Public License, version 2 or (at your option) any later version.
*
* Id: pppoe.h,v 1.4 2008/06/15 04:35:50 paulus Exp 
*
***********************************************************************/

#include "config.h"

#include <stdio.h>		/* For FILE */
#include <sys/types.h>		/* For pid_t */
#include <ctype.h>
#include <string.h>

#include "pppd/pppd.h"		/* For error */

/* How do we access raw Ethernet devices? */
#undef USE_LINUX_PACKET
#undef USE_BPF

#if defined(HAVE_NETPACKET_PACKET_H) || defined(HAVE_LINUX_IF_PACKET_H)
#define USE_LINUX_PACKET 1
#elif defined(HAVE_SYS_DLPI_H)
#define USE_DLPI
#elif defined(HAVE_NET_BPF_H)
#define USE_BPF 1
#endif

/* Sanity check */
#if !defined(USE_BPF) && !defined(USE_LINUX_PACKET) && !defined(USE_DLPI)
#error Unknown method for accessing raw Ethernet frames
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

/* This has to be included before Linux 4.8's linux/in.h
 * gets dragged in. */
#include <netinet/in.h>

/* Ugly header files on some Linux boxes... */
#if defined(HAVE_LINUX_IF_H)
#include <linux/if.h>
#elif defined(HAVE_NET_IF_H)
#include <net/if.h>
#endif

#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif

#define BPF_BUFFER_IS_EMPTY 1
#define BPF_BUFFER_HAS_DATA 0

/* Define various integer types -- assumes a char is 8 bits */
#if SIZEOF_UNSIGNED_SHORT == 2
typedef unsigned short UINT16_t;
#elif SIZEOF_UNSIGNED_INT == 2
typedef unsigned int UINT16_t;
#else
#error Could not find a 16-bit integer type
#endif

#if SIZEOF_UNSIGNED_SHORT == 4
typedef unsigned short UINT32_t;
#elif SIZEOF_UNSIGNED_INT == 4
typedef unsigned int UINT32_t;
#elif SIZEOF_UNSIGNED_LONG == 4
typedef unsigned long UINT32_t;
#else
#error Could not find a 32-bit integer type
#endif

#ifdef HAVE_LINUX_IF_ETHER_H
#include <linux/if_ether.h>
#else

#ifdef HAVE_NETINET_IF_ETHER_H
#include <sys/types.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifndef HAVE_SYS_DLPI_H
#include <netinet/if_ether.h>
#endif
#endif
#endif

/* Ethernet frame types according to RFC 2516 */
#define ETH_PPPOE_DISCOVERY 0x8863
#define ETH_PPPOE_SESSION   0x8864

/* But some brain-dead peers disobey the RFC, so frame types are variables */
extern UINT16_t Eth_PPPOE_Discovery;
extern UINT16_t Eth_PPPOE_Session;

/* PPPoE codes */
#define CODE_PADI           0x09
#define CODE_PADO           0x07
#define CODE_PADR           0x19
#define CODE_PADS           0x65
#define CODE_PADT           0xA7

/* Extensions from draft-carrel-info-pppoe-ext-00 */
/* I do NOT like PADM or PADN, but they are here for completeness */
#define CODE_PADM           0xD3
#define CODE_PADN           0xD4

#define CODE_SESS           0x00

/* PPPoE Tags */
#define TAG_END_OF_LIST        0x0000
#define TAG_SERVICE_NAME       0x0101
#define TAG_AC_NAME            0x0102
#define TAG_HOST_UNIQ          0x0103
#define TAG_AC_COOKIE          0x0104
#define TAG_VENDOR_SPECIFIC    0x0105
#define TAG_RELAY_SESSION_ID   0x0110
#define TAG_PPP_MAX_PAYLOAD    0x0120
#define TAG_SERVICE_NAME_ERROR 0x0201
#define TAG_AC_SYSTEM_ERROR    0x0202
#define TAG_GENERIC_ERROR      0x0203

/* Extensions from draft-carrel-info-pppoe-ext-00 */
/* I do NOT like these tags one little bit */
#define TAG_HURL               0x111
#define TAG_MOTM               0x112
#define TAG_IP_ROUTE_ADD       0x121

/* Discovery phase states */
#define STATE_SENT_PADI     0
#define STATE_RECEIVED_PADO 1
#define STATE_SENT_PADR     2
#define STATE_SESSION       3
#define STATE_TERMINATED    4

/* How many PADI/PADS attempts? */
#define MAX_PADI_ATTEMPTS 3

/* Initial timeout for PADO/PADS */
#define PADI_TIMEOUT 5

/* States for scanning PPP frames */
#define STATE_WAITFOR_FRAME_ADDR 0
#define STATE_DROP_PROTO         1
#define STATE_BUILDING_PACKET    2

/* Special PPP frame characters */
#define FRAME_ESC    0x7D
#define FRAME_FLAG   0x7E
#define FRAME_ADDR   0xFF
#define FRAME_CTRL   0x03
#define FRAME_ENC    0x20

#define IPV4ALEN     4
#define SMALLBUF   256

/* There are other fixed-size buffers preventing
   this from being increased to 16110. The buffer
   sizes would need to be properly de-coupled from
   the default MRU. For now, getting up to 1500 is
   enough. */
#define ETH_JUMBO_LEN 1508

/* A PPPoE Packet, including Ethernet headers */
typedef struct PPPoEPacketStruct {
    struct ethhdr ethHdr;	/* Ethernet header */
    unsigned int vertype:8;	/* PPPoE Version and Type (must both be 1) */
    unsigned int code:8;	/* PPPoE code */
    unsigned int session:16;	/* PPPoE session */
    unsigned int length:16;	/* Payload length */
    unsigned char payload[ETH_JUMBO_LEN]; /* A bit of room to spare */
} PPPoEPacket;

#define PPPOE_VER(vt)		((vt) >> 4)
#define PPPOE_TYPE(vt)		((vt) & 0xf)
#define PPPOE_VER_TYPE(v, t)	(((v) << 4) | (t))

/* Header size of a PPPoE packet */
#define PPPOE_OVERHEAD 6  /* type, code, session, length */
#define HDR_SIZE (sizeof(struct ethhdr) + PPPOE_OVERHEAD)
#define MAX_PPPOE_PAYLOAD (ETH_JUMBO_LEN - PPPOE_OVERHEAD)
#define PPP_OVERHEAD 2  /* protocol */
#define MAX_PPPOE_MTU (MAX_PPPOE_PAYLOAD - PPP_OVERHEAD)
#define TOTAL_OVERHEAD (PPPOE_OVERHEAD + PPP_OVERHEAD)
#define ETH_PPPOE_MTU (ETH_DATA_LEN - TOTAL_OVERHEAD)

/* PPPoE Tag */

typedef struct PPPoETagStruct {
    unsigned int type:16;	/* tag type */
    unsigned int length:16;	/* Length of payload */
    unsigned char payload[ETH_JUMBO_LEN]; /* A LOT of room to spare */
} PPPoETag;
/* Header size of a PPPoE tag */
#define TAG_HDR_SIZE 4

/* Chunk to read from stdin */
#define READ_CHUNK 4096

/* Function passed to parsePacket */
typedef void ParseFunc(UINT16_t type,
		       UINT16_t len,
		       unsigned char *data,
		       void *extra);

#define PPPINITFCS16    0xffff  /* Initial FCS value */

/* Keep track of the state of a connection -- collect everything in
   one spot */

typedef struct PPPoEConnectionStruct {
    int discoveryState;		/* Where we are in discovery */
    int discoverySocket;	/* Raw socket for discovery frames */
    int sessionSocket;		/* Raw socket for session frames */
    unsigned char myEth[ETH_ALEN]; /* My MAC address */
    unsigned char peerEth[ETH_ALEN]; /* Peer's MAC address */
    unsigned char req_peer_mac[ETH_ALEN]; /* required peer MAC address */
    unsigned char req_peer;	/* require mac addr to match req_peer_mac */
    UINT16_t session;		/* Session ID */
    char *ifName;		/* Interface name */
    char *serviceName;		/* Desired service name, if any */
    char *acName;		/* Desired AC name, if any */
    int synchronous;		/* Use synchronous PPP */
    PPPoETag hostUniq;		/* Use Host-Uniq tag */
    int printACNames;		/* Just print AC names */
    FILE *debugFile;		/* Debug file for dumping packets */
    int numPADOs;		/* Number of PADO packets received */
    PPPoETag cookie;		/* We have to send this if we get it */
    PPPoETag relayId;		/* Ditto */
    int error;			/* Error packet received */
    int debug;			/* Set to log packets sent and received */
    int discoveryTimeout;       /* Timeout for discovery packets */
    int discoveryAttempts;      /* Number of discovery attempts */
    int seenMaxPayload;
    int mtu;			/* Stored MTU */
    int mru;			/* Stored MRU */
} PPPoEConnection;

/* Structure used to determine acceptable PADO or PADS packet */
struct PacketCriteria {
    PPPoEConnection *conn;
    int acNameOK;
    int serviceNameOK;
    int seenACName;
    int seenServiceName;
};

/* Function Prototypes */
UINT16_t etherType(PPPoEPacket *packet);
int openInterface(char const *ifname, UINT16_t type, unsigned char *hwaddr);
int sendPacket(PPPoEConnection *conn, int sock, PPPoEPacket *pkt, int size);
int receivePacket(int sock, PPPoEPacket *pkt, int *size);
void fatalSys(char const *str);
void dumpPacket(FILE *fp, PPPoEPacket *packet, char const *dir);
void dumpHex(FILE *fp, unsigned char const *buf, int len);
int parsePacket(PPPoEPacket *packet, ParseFunc *func, void *extra);
void parseLogErrs(UINT16_t typ, UINT16_t len, unsigned char *data, void *xtra);
void syncReadFromPPP(PPPoEConnection *conn, PPPoEPacket *packet);
void asyncReadFromPPP(PPPoEConnection *conn, PPPoEPacket *packet);
void asyncReadFromEth(PPPoEConnection *conn, int sock, int clampMss);
void syncReadFromEth(PPPoEConnection *conn, int sock, int clampMss);
char *strDup(char const *str);
void sendPADT(PPPoEConnection *conn, char const *msg);
void sendSessionPacket(PPPoEConnection *conn,
		       PPPoEPacket *packet, int len);
void initPPP(void);
void clampMSS(PPPoEPacket *packet, char const *dir, int clampMss);
UINT16_t computeTCPChecksum(unsigned char *ipHdr, unsigned char *tcpHdr);
UINT16_t pppFCS16(UINT16_t fcs, unsigned char *cp, int len);
void discovery(PPPoEConnection *conn);
unsigned char *findTag(PPPoEPacket *packet, UINT16_t tagType,
		       PPPoETag *tag);

void pppoe_printpkt(PPPoEPacket *packet,
		    void (*printer)(void *, char *, ...), void *arg);
void pppoe_log_packet(const char *prefix, PPPoEPacket *packet);

static inline int parseHostUniq(const char *uniq, PPPoETag *tag)
{
    unsigned i, len = strlen(uniq);

#define hex(x) \
    (((x) <= '9') ? ((x) - '0') : \
        (((x) <= 'F') ? ((x) - 'A' + 10) : \
            ((x) - 'a' + 10)))

    if (!len || len % 2 || len / 2 > sizeof(tag->payload))
        return 0;

    for (i = 0; i < len; i += 2) {
        if (!isxdigit(uniq[i]) || !isxdigit(uniq[i+1]))
            return 0;

        tag->payload[i / 2] = (char)(hex(uniq[i]) << 4 | hex(uniq[i+1]));
    }

#undef hex

    tag->type = htons(TAG_HOST_UNIQ);
    tag->length = htons(len / 2);
    return 1;
}

#define SET_STRING(var, val) do { if (var) free(var); var = strDup(val); } while(0);

#define CHECK_ROOM(cursor, start, len) \
do {\
    if (((cursor)-(start))+(len) > MAX_PPPOE_PAYLOAD) { \
	error("Would create too-long packet");	\
        return; \
    } \
} while(0)

/* True if Ethernet address is broadcast or multicast */
#define NOT_UNICAST(e) ((e[0] & 0x01) != 0)
#define BROADCAST(e) ((e[0] & e[1] & e[2] & e[3] & e[4] & e[5]) == 0xFF)
#define NOT_BROADCAST(e) ((e[0] & e[1] & e[2] & e[3] & e[4] & e[5]) != 0xFF)
