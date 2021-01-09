/***********************************************************************
*
* if.c
*
* Implementation of user-space PPPoE redirector for Linux.
*
* Functions for opening a raw socket and reading/writing raw Ethernet frames.
*
* Copyright (C) 2000 by Roaring Penguin Software Inc.
*
* This program may be distributed according to the terms of the GNU
* General Public License, version 2 or (at your option) any later version.
*
***********************************************************************/

static char const RCSID[] =
"Id: if.c,v 1.2 2008/06/09 08:34:23 paulus Exp ";

#define _GNU_SOURCE 1
#include "pppoe.h"
#include "pppd/pppd.h"

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
	error("Invalid ether type 0x%x", type);
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
	error("Can't open socket for pppoe: %m");
	return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0) {
	error("Can't set socket options for pppoe: %m");
	close(fd);
	return -1;
    }

    /* Fill in hardware address */
    if (hwaddr) {
	strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
	    error("Can't get hardware address for %s: %m", ifname);
	    close(fd);
	    return -1;
	}
	memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
#ifdef ARPHRD_ETHER
	if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
	    warn("Interface %.16s is not Ethernet", ifname);
	}
#endif
	if (NOT_UNICAST(hwaddr)) {
	    fatal("Can't use interface %.16s: it has broadcast/multicast MAC address",
		  ifname);
	}
    }

    /* Sanity check on MTU */
    strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(fd, SIOCGIFMTU, &ifr) < 0) {
	error("Can't get MTU for %s: %m", ifname);
    } else if (ifr.ifr_mtu < ETH_DATA_LEN) {
	error("Interface %.16s has MTU of %d -- should be at least %d.",
	      ifname, ifr.ifr_mtu, ETH_DATA_LEN);
	error("This may cause serious connection problems.");
    }

#ifdef HAVE_STRUCT_SOCKADDR_LL
    /* Get interface index */
    sa.sll_family = AF_PACKET;
    sa.sll_protocol = htons(type);

    strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
	error("Could not get interface index for %s: %m", ifname);
	close(fd);
	return -1;
    }
    sa.sll_ifindex = ifr.ifr_ifindex;

#else
    strcpy(sa.sa_data, ifname);
#endif

    /* We're only interested in packets on specified interface */
    if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
	error("Failed to bind to interface %s: %m", ifname);
	close(fd);
	return -1;
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
    int err;

    if (debug)
	pppoe_log_packet("Send ", pkt);
#if defined(HAVE_STRUCT_SOCKADDR_LL)
    err = send(sock, pkt, size, 0);
#else
    struct sockaddr sa;

    strcpy(sa.sa_data, conn->ifName);
    err = sendto(sock, pkt, size, 0, &sa, sizeof(sa));
#endif
    if (err < 0) {
	error("error sending pppoe packet: %m");
	return -1;
    }
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
	error("error receiving pppoe packet: %m");
	return -1;
    }
    if (debug)
	pppoe_log_packet("Recv ", pkt);
    return 0;
}
