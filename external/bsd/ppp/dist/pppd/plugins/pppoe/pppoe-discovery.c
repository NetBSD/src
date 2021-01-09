/*
 * Perform PPPoE discovery
 *
 * Copyright (C) 2000-2001 by Roaring Penguin Software Inc.
 * Copyright (C) 2004 Marco d'Itri <md@linux.it>
 *
 * This program may be distributed according to the terms of the GNU
 * General Public License, version 2 or (at your option) any later version.
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "pppoe.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_NETPACKET_PACKET_H
#include <netpacket/packet.h>
#elif defined(HAVE_LINUX_IF_PACKET_H)
#include <linux/if_packet.h>
#endif

#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

char *xstrdup(const char *s);
void usage(void);

void die(int status)
{
	exit(status);
}

void error(char *fmt, ...)
{
    va_list pvar;
    va_start(pvar, fmt);
    vfprintf(stderr, fmt, pvar);
    va_end(pvar);
}

/* Initialize frame types to RFC 2516 values.  Some broken peers apparently
   use different frame types... sigh... */

UINT16_t Eth_PPPOE_Discovery = ETH_PPPOE_DISCOVERY;
UINT16_t Eth_PPPOE_Session   = ETH_PPPOE_SESSION;

/**********************************************************************
*%FUNCTION: etherType
*%ARGUMENTS:
* packet -- a received PPPoE packet
*%RETURNS:
* ethernet packet type (see /usr/include/net/ethertypes.h)
*%DESCRIPTION:
* Checks the ethernet packet header to determine its type.
* We should only be receveing DISCOVERY and SESSION types if the BPF
* is set up correctly.  Logs an error if an unexpected type is received.
* Note that the ethernet type names come from "pppoe.h" and the packet
* packet structure names use the LINUX dialect to maintain consistency
* with the rest of this file.  See the BSD section of "pppoe.h" for
* translations of the data structure names.
***********************************************************************/
UINT16_t
etherType(PPPoEPacket *packet)
{
    UINT16_t type = (UINT16_t) ntohs(packet->ethHdr.h_proto);
    if (type != Eth_PPPOE_Discovery && type != Eth_PPPOE_Session) {
	fprintf(stderr, "Invalid ether type 0x%x\n", type);
    }
    return type;
}

/**********************************************************************
*%FUNCTION: openInterface
*%ARGUMENTS:
* ifname -- name of interface
* type -- Ethernet frame type
* hwaddr -- if non-NULL, set to the hardware address
*%RETURNS:
* A raw socket for talking to the Ethernet card.  Exits on error.
*%DESCRIPTION:
* Opens a raw Ethernet socket
***********************************************************************/
int
openInterface(char const *ifname, UINT16_t type, unsigned char *hwaddr)
{
    int optval=1;
    int fd;
    struct ifreq ifr;
    int domain, stype;

#ifdef HAVE_STRUCT_SOCKADDR_LL
    struct sockaddr_ll sa;
#else
    struct sockaddr sa;
#endif

    memset(&sa, 0, sizeof(sa));

#ifdef HAVE_STRUCT_SOCKADDR_LL
    domain = PF_PACKET;
    stype = SOCK_RAW;
#else
    domain = PF_INET;
    stype = SOCK_PACKET;
#endif

    if ((fd = socket(domain, stype, htons(type))) < 0) {
	/* Give a more helpful message for the common error case */
	if (errno == EPERM) {
	    fatal("Cannot create raw socket -- pppoe must be run as root.");
	}
	fatalSys("socket");
    }

    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0) {
	fatalSys("setsockopt");
    }

    /* Fill in hardware address */
    if (hwaddr) {
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
	    fatalSys("ioctl(SIOCGIFHWADDR)");
	}
	memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
#ifdef ARPHRD_ETHER
	if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
	    fatal("Interface %.16s is not Ethernet", ifname);
	}
#endif
	if (NOT_UNICAST(hwaddr)) {
	    fatal("Interface %.16s has broadcast/multicast MAC address??", ifname);
	}
    }

    /* Sanity check on MTU */
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (ioctl(fd, SIOCGIFMTU, &ifr) < 0) {
	fatalSys("ioctl(SIOCGIFMTU)");
    }
    if (ifr.ifr_mtu < ETH_DATA_LEN) {
	fprintf(stderr, "Interface %.16s has MTU of %d -- should be %d.\n",
	      ifname, ifr.ifr_mtu, ETH_DATA_LEN);
	fprintf(stderr, "You may have serious connection problems.\n");
    }

#ifdef HAVE_STRUCT_SOCKADDR_LL
    /* Get interface index */
    sa.sll_family = AF_PACKET;
    sa.sll_protocol = htons(type);

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
	fatalSys("ioctl(SIOCFIGINDEX): Could not get interface index");
    }
    sa.sll_ifindex = ifr.ifr_ifindex;

#else
    strcpy(sa.sa_data, ifname);
#endif

    /* We're only interested in packets on specified interface */
    if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
	fatalSys("bind");
    }

    return fd;
}


/***********************************************************************
*%FUNCTION: sendPacket
*%ARGUMENTS:
* sock -- socket to send to
* pkt -- the packet to transmit
* size -- size of packet (in bytes)
*%RETURNS:
* 0 on success; -1 on failure
*%DESCRIPTION:
* Transmits a packet
***********************************************************************/
int
sendPacket(PPPoEConnection *conn, int sock, PPPoEPacket *pkt, int size)
{
#if defined(HAVE_STRUCT_SOCKADDR_LL)
    if (send(sock, pkt, size, 0) < 0) {
	fatalSys("send (sendPacket)");
	return -1;
    }
#else
    struct sockaddr sa;

    if (!conn) {
	fatal("relay and server not supported on Linux 2.0 kernels");
    }
    strcpy(sa.sa_data, conn->ifName);
    if (sendto(sock, pkt, size, 0, &sa, sizeof(sa)) < 0) {
	fatalSys("sendto (sendPacket)");
	return -1;
    }
#endif
    return 0;
}

/***********************************************************************
*%FUNCTION: receivePacket
*%ARGUMENTS:
* sock -- socket to read from
* pkt -- place to store the received packet
* size -- set to size of packet in bytes
*%RETURNS:
* >= 0 if all OK; < 0 if error
*%DESCRIPTION:
* Receives a packet
***********************************************************************/
int
receivePacket(int sock, PPPoEPacket *pkt, int *size)
{
    if ((*size = recv(sock, pkt, sizeof(PPPoEPacket), 0)) < 0) {
	fatalSys("recv (receivePacket)");
	return -1;
    }
    return 0;
}

/**********************************************************************
*%FUNCTION: parsePacket
*%ARGUMENTS:
* packet -- the PPPoE discovery packet to parse
* func -- function called for each tag in the packet
* extra -- an opaque data pointer supplied to parsing function
*%RETURNS:
* 0 if everything went well; -1 if there was an error
*%DESCRIPTION:
* Parses a PPPoE discovery packet, calling "func" for each tag in the packet.
* "func" is passed the additional argument "extra".
***********************************************************************/
int
parsePacket(PPPoEPacket *packet, ParseFunc *func, void *extra)
{
    UINT16_t len = ntohs(packet->length);
    unsigned char *curTag;
    UINT16_t tagType, tagLen;

    if (PPPOE_VER(packet->vertype) != 1) {
	fprintf(stderr, "Invalid PPPoE version (%d)\n",
		PPPOE_VER(packet->vertype));
	return -1;
    }
    if (PPPOE_TYPE(packet->vertype) != 1) {
	fprintf(stderr, "Invalid PPPoE type (%d)\n",
		PPPOE_TYPE(packet->vertype));
	return -1;
    }

    /* Do some sanity checks on packet */
    if (len > ETH_JUMBO_LEN - PPPOE_OVERHEAD) { /* 6-byte overhead for PPPoE header */
	fprintf(stderr, "Invalid PPPoE packet length (%u)\n", len);
	return -1;
    }

    /* Step through the tags */
    curTag = packet->payload;
    while(curTag - packet->payload < len) {
	/* Alignment is not guaranteed, so do this by hand... */
	tagType = (curTag[0] << 8) + curTag[1];
	tagLen = (curTag[2] << 8) + curTag[3];
	if (tagType == TAG_END_OF_LIST) {
	    return 0;
	}
	if ((curTag - packet->payload) + tagLen + TAG_HDR_SIZE > len) {
	    fprintf(stderr, "Invalid PPPoE tag length (%u)\n", tagLen);
	    return -1;
	}
	func(tagType, tagLen, curTag+TAG_HDR_SIZE, extra);
	curTag = curTag + TAG_HDR_SIZE + tagLen;
    }
    return 0;
}

/**********************************************************************
*%FUNCTION: parseForHostUniq
*%ARGUMENTS:
* type -- tag type
* len -- tag length
* data -- tag data.
* extra -- user-supplied pointer.  This is assumed to be a pointer to int.
*%RETURNS:
* Nothing
*%DESCRIPTION:
* If a HostUnique tag is found which matches our PID, sets *extra to 1.
***********************************************************************/
void
parseForHostUniq(UINT16_t type, UINT16_t len, unsigned char *data,
		 void *extra)
{
    PPPoETag *tag = extra;

    if (type == TAG_HOST_UNIQ && len == ntohs(tag->length))
	tag->length = memcmp(data, tag->payload, len);
}

/**********************************************************************
*%FUNCTION: packetIsForMe
*%ARGUMENTS:
* conn -- PPPoE connection info
* packet -- a received PPPoE packet
*%RETURNS:
* 1 if packet is for this PPPoE daemon; 0 otherwise.
*%DESCRIPTION:
* If we are using the Host-Unique tag, verifies that packet contains
* our unique identifier.
***********************************************************************/
int
packetIsForMe(PPPoEConnection *conn, PPPoEPacket *packet)
{
    PPPoETag hostUniq = conn->hostUniq;

    /* If packet is not directed to our MAC address, forget it */
    if (memcmp(packet->ethHdr.h_dest, conn->myEth, ETH_ALEN)) return 0;

    /* If we're not using the Host-Unique tag, then accept the packet */
    if (!conn->hostUniq.length) return 1;

    parsePacket(packet, parseForHostUniq, &hostUniq);
    return !hostUniq.length;
}

/**********************************************************************
*%FUNCTION: parsePADOTags
*%ARGUMENTS:
* type -- tag type
* len -- tag length
* data -- tag data
* extra -- extra user data.  Should point to a PacketCriteria structure
*          which gets filled in according to selected AC name and service
*          name.
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Picks interesting tags out of a PADO packet
***********************************************************************/
void
parsePADOTags(UINT16_t type, UINT16_t len, unsigned char *data,
	      void *extra)
{
    struct PacketCriteria *pc = (struct PacketCriteria *) extra;
    PPPoEConnection *conn = pc->conn;
    int i;

    switch(type) {
    case TAG_AC_NAME:
	pc->seenACName = 1;
	if (conn->printACNames) {
	    printf("Access-Concentrator: %.*s\n", (int) len, data);
	}
	if (conn->acName && len == strlen(conn->acName) &&
	    !strncmp((char *) data, conn->acName, len)) {
	    pc->acNameOK = 1;
	}
	break;
    case TAG_SERVICE_NAME:
	pc->seenServiceName = 1;
	if (conn->printACNames && len > 0) {
	    printf("       Service-Name: %.*s\n", (int) len, data);
	}
	if (conn->serviceName && len == strlen(conn->serviceName) &&
	    !strncmp((char *) data, conn->serviceName, len)) {
	    pc->serviceNameOK = 1;
	}
	break;
    case TAG_AC_COOKIE:
	if (conn->printACNames) {
	    printf("Got a cookie:");
	    /* Print first 20 bytes of cookie */
	    for (i=0; i<len && i < 20; i++) {
		printf(" %02x", (unsigned) data[i]);
	    }
	    if (i < len) printf("...");
	    printf("\n");
	}
	conn->cookie.type = htons(type);
	conn->cookie.length = htons(len);
	memcpy(conn->cookie.payload, data, len);
	break;
    case TAG_RELAY_SESSION_ID:
	if (conn->printACNames) {
	    printf("Got a Relay-ID:");
	    /* Print first 20 bytes of relay ID */
	    for (i=0; i<len && i < 20; i++) {
		printf(" %02x", (unsigned) data[i]);
	    }
	    if (i < len) printf("...");
	    printf("\n");
	}
	conn->relayId.type = htons(type);
	conn->relayId.length = htons(len);
	memcpy(conn->relayId.payload, data, len);
	break;
    case TAG_SERVICE_NAME_ERROR:
	if (conn->printACNames) {
	    printf("Got a Service-Name-Error tag: %.*s\n", (int) len, data);
	}
	break;
    case TAG_AC_SYSTEM_ERROR:
	if (conn->printACNames) {
	    printf("Got a System-Error tag: %.*s\n", (int) len, data);
	}
	break;
    case TAG_GENERIC_ERROR:
	if (conn->printACNames) {
	    printf("Got a Generic-Error tag: %.*s\n", (int) len, data);
	}
	break;
    }
}

/***********************************************************************
*%FUNCTION: sendPADI
*%ARGUMENTS:
* conn -- PPPoEConnection structure
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Sends a PADI packet
***********************************************************************/
void
sendPADI(PPPoEConnection *conn)
{
    PPPoEPacket packet;
    unsigned char *cursor = packet.payload;
    PPPoETag *svc = (PPPoETag *) (&packet.payload);
    UINT16_t namelen = 0;
    UINT16_t plen;

    if (conn->serviceName) {
	namelen = (UINT16_t) strlen(conn->serviceName);
    }
    plen = TAG_HDR_SIZE + namelen;
    CHECK_ROOM(cursor, packet.payload, plen);

    /* Set destination to Ethernet broadcast address */
    memset(packet.ethHdr.h_dest, 0xFF, ETH_ALEN);
    memcpy(packet.ethHdr.h_source, conn->myEth, ETH_ALEN);

    packet.ethHdr.h_proto = htons(Eth_PPPOE_Discovery);
    packet.vertype = PPPOE_VER_TYPE(1, 1);
    packet.code = CODE_PADI;
    packet.session = 0;

    svc->type = TAG_SERVICE_NAME;
    svc->length = htons(namelen);
    CHECK_ROOM(cursor, packet.payload, namelen+TAG_HDR_SIZE);

    if (conn->serviceName) {
	memcpy(svc->payload, conn->serviceName, strlen(conn->serviceName));
    }
    cursor += namelen + TAG_HDR_SIZE;

    /* If we're using Host-Uniq, copy it over */
    if (conn->hostUniq.length) {
	int len = ntohs(conn->hostUniq.length);
	CHECK_ROOM(cursor, packet.payload, len + TAG_HDR_SIZE);
	memcpy(cursor, &conn->hostUniq, len + TAG_HDR_SIZE);
	cursor += len + TAG_HDR_SIZE;
	plen += len + TAG_HDR_SIZE;
    }

    packet.length = htons(plen);

    sendPacket(conn, conn->discoverySocket, &packet, (int) (plen + HDR_SIZE));
    if (conn->debugFile) {
	dumpPacket(conn->debugFile, &packet, "SENT");
	fprintf(conn->debugFile, "\n");
	fflush(conn->debugFile);
    }
}

/**********************************************************************
*%FUNCTION: waitForPADO
*%ARGUMENTS:
* conn -- PPPoEConnection structure
* timeout -- how long to wait (in seconds)
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Waits for a PADO packet and copies useful information
***********************************************************************/
void
waitForPADO(PPPoEConnection *conn, int timeout)
{
    fd_set readable;
    int r;
    struct timeval tv;
    PPPoEPacket packet;
    int len;

    struct PacketCriteria pc;
    pc.conn          = conn;
    pc.acNameOK      = (conn->acName)      ? 0 : 1;
    pc.serviceNameOK = (conn->serviceName) ? 0 : 1;
    pc.seenACName    = 0;
    pc.seenServiceName = 0;
    conn->error = 0;
	
    do {
	if (BPF_BUFFER_IS_EMPTY) {
	    tv.tv_sec = timeout;
	    tv.tv_usec = 0;
	
	    FD_ZERO(&readable);
	    FD_SET(conn->discoverySocket, &readable);

	    while(1) {
		r = select(conn->discoverySocket+1, &readable, NULL, NULL, &tv);
		if (r >= 0 || errno != EINTR) break;
	    }
	    if (r < 0) {
		perror("select (waitForPADO)");
		return;
	    }
	    if (r == 0) return;        /* Timed out */
	}
	
	/* Get the packet */
	receivePacket(conn->discoverySocket, &packet, &len);

	/* Check length */
	if (ntohs(packet.length) + HDR_SIZE > len) {
	    fprintf(stderr, "Bogus PPPoE length field (%u)\n",
		   (unsigned int) ntohs(packet.length));
	    continue;
	}

#ifdef USE_BPF
	/* If it's not a Discovery packet, loop again */
	if (etherType(&packet) != Eth_PPPOE_Discovery) continue;
#endif

	if (conn->debugFile) {
	    dumpPacket(conn->debugFile, &packet, "RCVD");
	    fprintf(conn->debugFile, "\n");
	    fflush(conn->debugFile);
	}
	/* If it's not for us, loop again */
	if (!packetIsForMe(conn, &packet)) continue;

	if (packet.code == CODE_PADO) {
	    if (BROADCAST(packet.ethHdr.h_source)) {
		fprintf(stderr, "Ignoring PADO packet from broadcast MAC address\n");
		continue;
	    }
	    parsePacket(&packet, parsePADOTags, &pc);
	    if (conn->error)
		return;
	    if (!pc.seenACName) {
		fprintf(stderr, "Ignoring PADO packet with no AC-Name tag\n");
		continue;
	    }
	    if (!pc.seenServiceName) {
		fprintf(stderr, "Ignoring PADO packet with no Service-Name tag\n");
		continue;
	    }
	    conn->numPADOs++;
	    if (pc.acNameOK && pc.serviceNameOK) {
		memcpy(conn->peerEth, packet.ethHdr.h_source, ETH_ALEN);
		if (conn->printACNames) {
		    printf("AC-Ethernet-Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
			   (unsigned) conn->peerEth[0], 
			   (unsigned) conn->peerEth[1],
			   (unsigned) conn->peerEth[2],
			   (unsigned) conn->peerEth[3],
			   (unsigned) conn->peerEth[4],
			   (unsigned) conn->peerEth[5]);
		    printf("--------------------------------------------------\n");
		    continue;
		}
		conn->discoveryState = STATE_RECEIVED_PADO;
		break;
	    }
	}
    } while (conn->discoveryState != STATE_RECEIVED_PADO);
}

/**********************************************************************
*%FUNCTION: discovery
*%ARGUMENTS:
* conn -- PPPoE connection info structure
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Performs the PPPoE discovery phase
***********************************************************************/
void
discovery(PPPoEConnection *conn)
{
    int padiAttempts = 0;
    int timeout = conn->discoveryTimeout;

    conn->discoverySocket =
	openInterface(conn->ifName, Eth_PPPOE_Discovery, conn->myEth);

    do {
	padiAttempts++;
	if (padiAttempts > conn->discoveryAttempts) {
	    fprintf(stderr, "Timeout waiting for PADO packets\n");
	    close(conn->discoverySocket);
	    conn->discoverySocket = -1;
	    return;
	}
	sendPADI(conn);
	conn->discoveryState = STATE_SENT_PADI;
	waitForPADO(conn, timeout);
    } while (!conn->numPADOs);
}

int main(int argc, char *argv[])
{
    int opt;
    PPPoEConnection *conn;

    conn = malloc(sizeof(PPPoEConnection));
    if (!conn)
	fatalSys("malloc");

    memset(conn, 0, sizeof(PPPoEConnection));

    conn->printACNames = 1;
    conn->discoveryTimeout = PADI_TIMEOUT;
    conn->discoveryAttempts = MAX_PADI_ATTEMPTS;

    while ((opt = getopt(argc, argv, "I:D:VUQS:C:W:t:a:h")) > 0) {
	switch(opt) {
	case 'S':
	    conn->serviceName = xstrdup(optarg);
	    break;
	case 'C':
	    conn->acName = xstrdup(optarg);
	    break;
	case 't':
	    if (sscanf(optarg, "%d", &conn->discoveryTimeout) != 1) {
		fprintf(stderr, "Illegal argument to -t: Should be -t timeout\n");
		exit(EXIT_FAILURE);
	    }
	    if (conn->discoveryTimeout < 1) {
		conn->discoveryTimeout = 1;
	    }
	    break;
	case 'a':
	    if (sscanf(optarg, "%d", &conn->discoveryAttempts) != 1) {
		fprintf(stderr, "Illegal argument to -a: Should be -a attempts\n");
		exit(EXIT_FAILURE);
	    }
	    if (conn->discoveryAttempts < 1) {
		conn->discoveryAttempts = 1;
	    }
	    break;
	case 'U':
	    if(conn->hostUniq.length) {
		fprintf(stderr, "-U and -W are mutually exclusive\n");
		exit(EXIT_FAILURE);
	    } else {
		pid_t pid = getpid();
		conn->hostUniq.type = htons(TAG_HOST_UNIQ);
		conn->hostUniq.length = htons(sizeof(pid));
		memcpy(conn->hostUniq.payload, &pid, sizeof(pid));
	    }
	    break;
	case 'W':
	    if(conn->hostUniq.length) {
		fprintf(stderr, "-U and -W are mutually exclusive\n");
		exit(EXIT_FAILURE);
	    }
	    if (!parseHostUniq(optarg, &conn->hostUniq)) {
		fprintf(stderr, "Invalid host-uniq argument: %s\n", optarg);
		exit(EXIT_FAILURE);
            }
	    break;
	case 'D':
	    conn->debugFile = fopen(optarg, "w");
	    if (!conn->debugFile) {
		fprintf(stderr, "Could not open %s: %s\n",
			optarg, strerror(errno));
		exit(1);
	    }
	    fprintf(conn->debugFile, "pppoe-discovery from pppd %s\n", VERSION);
	    break;
	case 'I':
	    conn->ifName = xstrdup(optarg);
	    break;
	case 'Q':
	    conn->printACNames = 0;
	    break;
	case 'V':
	case 'h':
	    usage();
	    exit(0);
	default:
	    usage();
	    exit(1);
	}
    }

    /* default interface name */
    if (!conn->ifName)
	conn->ifName = strdup("eth0");

    conn->discoverySocket = -1;
    conn->sessionSocket = -1;

    discovery(conn);

    if (!conn->numPADOs)
	exit(1);
    else
	exit(0);
}

void fatal(char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

void fatalSys(char const *str)
{
    perror(str);
    exit(1);
}

char *xstrdup(const char *s)
{
    register char *ret = strdup(s);
    if (!ret)
	fatalSys("strdup");
    return ret;
}

void usage(void)
{
    fprintf(stderr, "Usage: pppoe-discovery [options]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "   -I if_name     -- Specify interface (default eth0)\n");
    fprintf(stderr, "   -D filename    -- Log debugging information in filename.\n");
    fprintf(stderr,
	    "   -t timeout     -- Initial timeout for discovery packets in seconds\n"
	    "   -a attempts    -- Number of discovery attempts\n"
	    "   -V             -- Print version and exit.\n"
	    "   -Q             -- Quit Mode: Do not print access concentrator names\n"
	    "   -S name        -- Set desired service name.\n"
	    "   -C name        -- Set desired access concentrator name.\n"
	    "   -U             -- Use Host-Unique to allow multiple PPPoE sessions.\n"
	    "   -W hexvalue    -- Set the Host-Unique to the supplied hex string.\n"
	    "   -h             -- Print usage information.\n");
    fprintf(stderr, "\npppoe-discovery from pppd " VERSION "\n");
}
