/* $NetBSD: socketops.c,v 1.26 2013/02/04 20:28:24 kefren Exp $ */

/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mihai Chelaru <kefren@NetBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>
#include <errno.h>
#include <ifaddrs.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include "conffile.h"
#include "fsm.h"
#include "ldp.h"
#include "ldp_command.h"
#include "tlv.h"
#include "ldp_peer.h"
#include "notifications.h"
#include "tlv_stack.h"
#include "mpls_interface.h"
#include "label.h"
#include "mpls_routes.h"
#include "ldp_errors.h"
#include "socketops.h"

int ls;				/* TCP listening socket on port 646 */
int route_socket;		/* used to see when a route is added/deleted */
int command_socket;		/* Listening socket for interface command */
int current_msg_id = 0x233;
int command_port = LDP_COMMAND_PORT;
extern int      replay_index;
extern struct rt_msg replay_rt[REPLAY_MAX];
extern struct com_sock	csockets[MAX_COMMAND_SOCKETS];

int	ldp_hello_time = LDP_HELLO_TIME;
int	ldp_keepalive_time = LDP_KEEPALIVE_TIME;
int	ldp_holddown_time = LDP_HOLDTIME;
int	no_default_route = 1;
int	loop_detection = 0;
bool	may_connect;

void	recv_pdu(int);
void	send_hello_alarm(int);
__dead static void bail_out(int);
static int bind_socket(int s, int stype);
static int set_tos(int); 
static int socket_reuse_port(int);
static int get_local_addr(struct sockaddr_dl *, struct in_addr *);
static int is_hello_socket(int);
static int is_passive_if(char *if_name);

int 
create_hello_sockets()
{
	struct ip_mreq  mcast_addr;
	int s, joined_groups;
	struct ifaddrs *ifa, *ifb;
	uint lastifindex;
#ifdef INET6
	struct ipv6_mreq mcast_addr6;
	struct sockaddr_in6 *if_sa6;
#endif
	struct hello_socket *hs;

	SLIST_INIT(&hello_socket_head);

	s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s < 0)
		return s;
	debugp("INET4 socket created (%d)\n", s);
	/*
	 * RFC5036 specifies we should listen to all subnet routers multicast
	 * group
	 */
	mcast_addr.imr_multiaddr.s_addr = htonl(INADDR_ALLRTRS_GROUP);

	if (socket_reuse_port(s) < 0)
		goto chs_error;
	/* Bind it to port 646 */
	if (bind_socket(s, AF_INET) == -1) {
		warnp("Cannot bind INET hello socket\n");
		goto chs_error;
	}

	/* We don't need to receive back our messages */
	if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &(u_char){0},
	    sizeof(u_char)) == -1) {
		fatalp("INET setsockopt IP_MCAST_LOOP: %s\n", strerror(errno));
		goto chs_error;
	}
	/* Finally join the group on all interfaces */
	if (getifaddrs(&ifa) == -1) {
		fatalp("Cannot iterate interfaces\n");
		return -1;
	}
	lastifindex = UINT_MAX;
	joined_groups = 0;
	for (ifb = ifa; ifb; ifb = ifb->ifa_next) {
		struct sockaddr_in *if_sa = (struct sockaddr_in *) ifb->ifa_addr;
		if (if_sa->sin_family != AF_INET || (!(ifb->ifa_flags & IFF_UP)) ||
		    (ifb->ifa_flags & IFF_LOOPBACK) ||
		    (!(ifb->ifa_flags & IFF_MULTICAST)) ||
		    (ntohl(if_sa->sin_addr.s_addr) >> 24 == IN_LOOPBACKNET) ||
		    is_passive_if(ifb->ifa_name) ||
		    lastifindex == if_nametoindex(ifb->ifa_name))
			continue;
		lastifindex = if_nametoindex(ifb->ifa_name);

		mcast_addr.imr_interface.s_addr = if_sa->sin_addr.s_addr;
		debugp("Join IPv4 mcast on %s\n", ifb->ifa_name);
        	if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mcast_addr,
		    sizeof(mcast_addr)) == -1) {
        	        fatalp("setsockopt ADD_MEMBER: %s\n", strerror(errno));
			goto chs_error;
        	}
		joined_groups++;
		if (joined_groups == IP_MAX_MEMBERSHIPS) {
			warnp("Maximum group memberships reached for INET socket\n");
			break;
		}
	}
	/* TTL:1 for IPv4 */
	if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &(int){1},
	    sizeof(int)) == -1) {
		fatalp("set mcast ttl: %s\n", strerror(errno));
		goto chs_error;
	}
	/* TOS :0xc0 for IPv4 */
	if (set_tos(s) == -1) {
		fatalp("set_tos: %s", strerror(errno));
		goto chs_error;
	}
	/* we need to get the input interface for message processing */
	if (setsockopt(s, IPPROTO_IP, IP_RECVIF, &(uint32_t){1},
	    sizeof(uint32_t)) == -1) {
		fatalp("Cannot set IP_RECVIF\n");
		goto chs_error;
	}

	hs = (struct hello_socket *)malloc(sizeof(*hs));
	if (hs == NULL) {
		fatalp("Cannot alloc hello_socket structure\n");
		goto chs_error;
	}
	hs->type = AF_INET;
	hs->socket = s;
	SLIST_INSERT_HEAD(&hello_socket_head, hs, listentry);

#ifdef INET6
	/*
	 * Now we do the same for IPv6
	 */
	s = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (s < 0) {
		fatalp("Cannot create INET6 socket\n");
		return -1;
	}
	debugp("INET6 socket created (%d)\n", s);

	if (socket_reuse_port(s) < 0)
		goto chs_error;

	if (bind_socket(s, AF_INET6) == -1) {
		fatalp("Cannot bind INET6 hello socket\n");
		goto chs_error;
	}

	if (setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
	    &(uint){0}, sizeof(uint)) == -1) {
		fatalp("INET6 setsocketopt IP_MCAST_LOOP: %s\n",
		    strerror(errno));
		goto chs_error;
	}

	lastifindex = UINT_MAX;
	mcast_addr6.ipv6mr_multiaddr = in6addr_linklocal_allrouters;
	for (ifb = ifa; ifb; ifb = ifb->ifa_next) {
		if_sa6 = (struct sockaddr_in6 *) ifb->ifa_addr;
		if (if_sa6->sin6_family != AF_INET6 ||
		    (!(ifb->ifa_flags & IFF_UP)) ||
		    (!(ifb->ifa_flags & IFF_MULTICAST)) ||
		    (ifb->ifa_flags & IFF_LOOPBACK) ||
		    is_passive_if(ifb->ifa_name) ||
		    IN6_IS_ADDR_LOOPBACK(&if_sa6->sin6_addr))
			continue;
		/*
		 * draft-ietf-mpls-ldp-ipv6-07 Section 5.1:
		 * Additionally, the link-local
		 * IPv6 address MUST be used as the source IP address in IPv6
		 * LDP Link Hellos.
		 */
		if (IN6_IS_ADDR_LINKLOCAL(&if_sa6->sin6_addr) == 0)
			continue;
		/* We should have only one LLADDR per interface, but... */
		if (lastifindex == if_nametoindex(ifb->ifa_name))
			continue;
		mcast_addr6.ipv6mr_interface = lastifindex =
		    if_nametoindex(ifb->ifa_name);

		debugp("Join IPv6 mcast on %s\n", ifb->ifa_name);
		if (setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP,
		    (char *)&mcast_addr6, sizeof(mcast_addr6)) == -1) {
			fatalp("INET6 setsockopt JOIN: %s\n", strerror(errno));
			goto chs_error;
		}
	}
	freeifaddrs(ifa);

	/* TTL: 255 for IPv6 - draft-ietf-mpls-ldp-ipv6-07 Section 9 */
	if (setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
	    &(int){255}, sizeof(int)) == -1) {
		fatalp("set mcast hops: %s\n", strerror(errno));
		goto chs_error;
	}
	if (setsockopt(s, IPPROTO_IPV6, IPV6_RECVPKTINFO,
	    &(uint32_t){1}, sizeof(uint32_t)) == -1)
		goto chs_error;

	hs = (struct hello_socket *)malloc(sizeof(*hs));
	if (hs == NULL) {
		fatalp("Memory alloc problem: hs\n");
		goto chs_error;
	}

	hs->type = AF_INET6;
	hs->socket = s;
	SLIST_INSERT_HEAD(&hello_socket_head, hs, listentry);
#endif
	return 0;
chs_error:
	close(s);
	return -1;
}

/* Check if parameter is a hello socket */
int
is_hello_socket(int s)
{
	struct hello_socket *hs;

	SLIST_FOREACH(hs, &hello_socket_head, listentry)
		if (hs->socket == s)
			return 1;
	return 0;
}

/* Check if interface is passive */
static int
is_passive_if(char *if_name)
{
	struct passive_if *pif;

	SLIST_FOREACH(pif, &passifs_head, listentry)
		if (strncasecmp(if_name, pif->if_name, IF_NAMESIZE) == 0)
			return 1;
	return 0;
}

/* Sets the TTL to 1 as we don't want to transmit outside this subnet */
int
set_ttl(int s)
{
	int             ret;
	if ((ret = setsockopt(s, IPPROTO_IP, IP_TTL, &(int){1}, sizeof(int)))
	    == -1)
		fatalp("set_ttl: %s", strerror(errno));
	return ret;
}

/* Sets TOS to 0xc0 aka IP Precedence 6 */
static int
set_tos(int s)
{
	int             ret;
	if ((ret = setsockopt(s, IPPROTO_IP, IP_TOS, &(int){0xc0},
	    sizeof(int))) == -1)
		fatalp("set_tos: %s", strerror(errno));
	return ret;
}

static int
socket_reuse_port(int s)
{
	int ret;
	if ((ret = setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &(int){1},
	    sizeof(int))) == -1)
		fatalp("socket_reuse_port: %s", strerror(errno));
	return ret;
}

/* binds an UDP socket */
static int
bind_socket(int s, int stype)
{
	union sockunion su;

	assert (stype == AF_INET || stype == AF_INET6);

	if (stype == AF_INET) {
		su.sin.sin_len = sizeof(su.sin);
		su.sin.sin_family = AF_INET;
		su.sin.sin_addr.s_addr = htonl(INADDR_ANY);
		su.sin.sin_port = htons(LDP_PORT);
	}
#ifdef INET6
	else if (stype == AF_INET6) {
		su.sin6.sin6_len = sizeof(su.sin6);
		su.sin6.sin6_family = AF_INET6;
		su.sin6.sin6_addr = in6addr_any;
		su.sin6.sin6_port = htons(LDP_PORT);
	}
#endif
	if (bind(s, &su.sa, su.sa.sa_len)) {
		fatalp("bind_socket: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

/* Create / bind the TCP socket */
int
create_listening_socket(void)
{
	struct sockaddr_in sa;
	int             s;

	sa.sin_len = sizeof(sa);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(LDP_PORT);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0)
		return s;
	if (bind(s, (struct sockaddr *) & sa, sizeof(sa))) {
		fatalp("bind: %s", strerror(errno));
		close(s);
		return -1;
	}
	if (listen(s, 10) == -1) {
		fatalp("listen: %s", strerror(errno));
		close(s);
		return -1;
	}
/*	if (set_tos(s) == -1) {
		fatalp("set_tos: %s", strerror(errno));
		close(s);
		return -1;
	}
*/	return s;
}

/*
 * It's ugly. We need a function to pass all tlvs and create pdu but since I
 * use UDP socket only to send hellos, I didn't bother
 */
void 
send_hello(void)
{
	struct hello_tlv *t;
	struct common_hello_tlv *cht;
	struct ldp_pdu  *spdu;
	struct transport_address_tlv *trtlv;
	void *v;
	struct sockaddr_in sadest;	/* Destination ALL_ROUTERS */
	ssize_t sb = 0;			/* sent bytes */
	struct ifaddrs *ifa, *ifb;
	struct sockaddr_in *if_sa;
	int ip4socket = -1;
	uint lastifindex;
	struct hello_socket *hs;
#ifdef INET6
	struct sockaddr_in6 sadest6;
	int ip6socket = -1;
#endif

#define BASIC_HELLO_MSG_SIZE (sizeof(struct ldp_pdu) + 	/* PDU */	\
			TLV_TYPE_LENGTH + MSGID_SIZE +	/* Hello TLV */	\
			/* Common Hello TLV */				\
			sizeof(struct common_hello_tlv))
#define GENERAL_HELLO_MSG_SIZE BASIC_HELLO_MSG_SIZE + 			\
			/* Transport Address */				\
			sizeof(struct transport_address_tlv)
#define IPV4_HELLO_MSG_SIZE BASIC_HELLO_MSG_SIZE + 4 + sizeof(struct in_addr)
#define IPV6_HELLO_MSG_SIZE BASIC_HELLO_MSG_SIZE + 4 + sizeof(struct in6_addr)

	if ((v = calloc(1, GENERAL_HELLO_MSG_SIZE)) == NULL) {
		fatalp("alloc problem in send_hello()\n");
		return;
	}

	spdu = (struct ldp_pdu *)((char *)v);
	t = (struct hello_tlv *)(spdu + 1);
	cht = &t->ch;	/* Hello tlv struct includes CHT */
	trtlv = (struct transport_address_tlv *)(t + 1);

	/* Prepare PDU envelope */
	spdu->version = htons(LDP_VERSION);
	spdu->length = htons(IPV4_HELLO_MSG_SIZE - PDU_VER_LENGTH);
	inet_aton(LDP_ID, &spdu->ldp_id);

	/* Prepare Hello TLV */
	t->type = htons(LDP_HELLO);
	t->length = htons(MSGID_SIZE +
			sizeof(struct common_hello_tlv) +
			IPV4_HELLO_MSG_SIZE - BASIC_HELLO_MSG_SIZE);
	/*
	 * kefren:
	 * I used ID 0 instead of htonl(get_message_id()) because I've
	 * seen hellos from Cisco routers doing the same thing
	 */
	t->messageid = 0;

	/* Prepare Common Hello attributes */
	cht->type = htons(TLV_COMMON_HELLO);
	cht->length = htons(sizeof(cht->holdtime) + sizeof(cht->res));
	cht->holdtime = htons(ldp_holddown_time);
	cht->res = 0;

	/*
	 * Prepare Transport Address TLV RFC5036 says: "If this optional TLV
	 * is not present the IPv4 source address for the UDP packet carrying
	 * the Hello should be used." But we send it because everybody seems
	 * to do so
	 */
	trtlv->type = htons(TLV_IPV4_TRANSPORT);
	trtlv->length = htons(sizeof(struct in_addr));
	/* trtlv->address will be set for each socket */

	/* Destination sockaddr */
	memset(&sadest, 0, sizeof(sadest));
	sadest.sin_len = sizeof(sadest);
	sadest.sin_family = AF_INET;
	sadest.sin_port = htons(LDP_PORT);
	sadest.sin_addr.s_addr = htonl(INADDR_ALLRTRS_GROUP);

	/* Find our socket */
	SLIST_FOREACH(hs, &hello_socket_head, listentry)
		if (hs->type == AF_INET) {
			ip4socket = hs->socket;
			break;
		}
	assert(ip4socket >= 0);

	if (getifaddrs(&ifa) == -1) {
		free(v);
		fatalp("Cannot enumerate interfaces\n");
		return;
	}
	
	lastifindex = UINT_MAX;
	/* Loop all interfaces in order to send IPv4 hellos */
	for (ifb = ifa; ifb; ifb = ifb->ifa_next) {
		if_sa = (struct sockaddr_in *) ifb->ifa_addr;
		if (if_sa->sin_family != AF_INET ||
		    (!(ifb->ifa_flags & IFF_UP)) ||
		    (ifb->ifa_flags & IFF_LOOPBACK) ||
		    (!(ifb->ifa_flags & IFF_MULTICAST)) ||
		    is_passive_if(ifb->ifa_name) ||
		    (ntohl(if_sa->sin_addr.s_addr) >> 24 == IN_LOOPBACKNET) ||
		    lastifindex == if_nametoindex(ifb->ifa_name))
			continue;

		/* Send only once per interface, using primary address */
		if (lastifindex == if_nametoindex(ifb->ifa_name))
			continue;
		lastifindex = if_nametoindex(ifb->ifa_name);

		if (setsockopt(ip4socket, IPPROTO_IP, IP_MULTICAST_IF,
		    &if_sa->sin_addr, sizeof(struct in_addr)) == -1) {
			warnp("setsockopt failed: %s\n", strerror(errno));
			continue;
		}
		trtlv->address.ip4addr.s_addr = if_sa->sin_addr.s_addr;

		/* Put it on the wire */
		sb = sendto(ip4socket, v, IPV4_HELLO_MSG_SIZE, 0,
			    (struct sockaddr *) & sadest, sizeof(sadest));
		if (sb < (ssize_t)(IPV4_HELLO_MSG_SIZE))
		    fatalp("send: %s", strerror(errno));
		else
		    debugp("Sent (IPv4) %zd bytes on %s"
			" (PDU: %d, Hello TLV: %d, CH: %d, TR: %d)\n",
			sb, ifb->ifa_name,
			ntohs(spdu->length), ntohs(t->length),
			ntohs(cht->length), ntohs(trtlv->length));
	}
#ifdef INET6
	/* Adjust lengths */
	spdu->length = htons(IPV6_HELLO_MSG_SIZE - PDU_VER_LENGTH);
	t->length = htons(MSGID_SIZE +
			sizeof(struct common_hello_tlv) +
			IPV6_HELLO_MSG_SIZE - BASIC_HELLO_MSG_SIZE);
	trtlv->length = htons(sizeof(struct in6_addr));
	trtlv->type = htons(TLV_IPV6_TRANSPORT);

	/* Prepare destination sockaddr */
	memset(&sadest6, 0, sizeof(sadest6));
	sadest6.sin6_len = sizeof(sadest6);
	sadest6.sin6_family = AF_INET6;
	sadest6.sin6_port = htons(LDP_PORT);
	sadest6.sin6_addr = in6addr_linklocal_allrouters;

	SLIST_FOREACH(hs, &hello_socket_head, listentry)
		if (hs->type == AF_INET6) {
			ip6socket = hs->socket;
			break;
		}

	lastifindex = UINT_MAX;
	for (ifb = ifa; ifb; ifb = ifb->ifa_next) {
		struct sockaddr_in6 * if_sa6 =
		    (struct sockaddr_in6 *) ifb->ifa_addr;
		if (if_sa6->sin6_family != AF_INET6 ||
		    (!(ifb->ifa_flags & IFF_UP)) ||
		    (!(ifb->ifa_flags & IFF_MULTICAST)) ||
		    (ifb->ifa_flags & IFF_LOOPBACK) ||
		    is_passive_if(ifb->ifa_name) ||
		    IN6_IS_ADDR_LOOPBACK(&if_sa6->sin6_addr))
			continue;
		/*
		 * draft-ietf-mpls-ldp-ipv6-07 Section 5.1:
		 * Additionally, the link-local
		 * IPv6 address MUST be used as the source IP address in IPv6
		 * LDP Link Hellos.
		 */
		if (IN6_IS_ADDR_LINKLOCAL(&if_sa6->sin6_addr) == 0)
			continue;
		/* We should have only one LLADDR per interface, but... */
		if (lastifindex == if_nametoindex(ifb->ifa_name))
			continue;
		lastifindex = if_nametoindex(ifb->ifa_name);

		if (setsockopt(ip6socket, IPPROTO_IPV6, IPV6_MULTICAST_IF,
		    &lastifindex, sizeof(int)) == -1) {
			fatalp("ssopt6 IPV6_MULTICAST_IF failed: %s for %s\n",
			    strerror(errno), ifb->ifa_name);
			continue;
		}

		memcpy(&trtlv->address.ip6addr, &if_sa6->sin6_addr,
		    sizeof(struct in6_addr));

		/* Put it on the wire */
		sb = sendto(ip6socket, v, IPV6_HELLO_MSG_SIZE,
			    0, (struct sockaddr *)&sadest6, sizeof(sadest6));
		if (sb < (ssize_t)(IPV6_HELLO_MSG_SIZE))
		    fatalp("send6: %s", strerror(errno));
		else
		    debugp("Sent (IPv6) %zd bytes on %s "
			"(PDU: %d, Hello TLV: %d, CH: %d TR: %d)\n",
			sb, ifb->ifa_name, htons(spdu->length),
			htons(t->length), htons(cht->length),
			htons(trtlv->length));
	}
#endif
	freeifaddrs(ifa);
	free(v);
}

int
get_message_id(void)
{
	current_msg_id++;
	return current_msg_id;
}

static int
get_local_addr(struct sockaddr_dl *sdl, struct in_addr *sin)
{
	struct ifaddrs *ifa, *ifb;
	struct sockaddr_in *sinet;

	if (sdl == NULL)
		return -1;

	if (getifaddrs(&ifa) == -1)
		return -1;
	for (ifb = ifa; ifb; ifb = ifb->ifa_next)
		if (ifb->ifa_addr->sa_family == AF_INET) {
			if (if_nametoindex(ifb->ifa_name) != sdl->sdl_index)
				continue;
			sinet = (struct sockaddr_in*) ifb->ifa_addr;
			sin->s_addr = sinet->sin_addr.s_addr;
			freeifaddrs(ifa);
			return 0;
		}
	freeifaddrs(ifa);
	return -1;
}

/* Receive PDUs on Multicast UDP socket */
void
recv_pdu(int sock)
{
	struct ldp_pdu  rpdu;
	int             c, i;
	struct msghdr msg;
	struct iovec iov[1];
	unsigned char recvspace[MAX_PDU_SIZE];
	struct hello_tlv *t;
	union sockunion sender;
	struct sockaddr_dl *sdl = NULL;
	struct in_addr my_ldp_addr, local_addr;
	struct cmsghdr *cmptr;
	union {
		struct cmsghdr cm;
		char control[1024];
	} control_un;

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	msg.msg_flags = 0;
	msg.msg_name = &sender;
	msg.msg_namelen = sizeof(sender);
	iov[0].iov_base = recvspace;
	iov[0].iov_len = sizeof(recvspace);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	c = recvmsg(sock, &msg, MSG_WAITALL);

	/* Check to see if this is larger than MIN_PDU_SIZE */
	if (c < MIN_PDU_SIZE)
		return;

	/* Read the PDU */
	i = get_pdu(recvspace, &rpdu);

	debugp("recv_pdu(%d): PDU(size: %d) from: %s\n", sock,
	    c, satos(&sender.sa));

	/* We currently understand Version 1 */
	if (rpdu.version != LDP_VERSION) {
		warnp("recv_pdu: Version mismatch\n");
		return;
	}

	/* Check if it's our hello */
	inet_aton(LDP_ID, &my_ldp_addr);
	if (rpdu.ldp_id.s_addr == my_ldp_addr.s_addr) {
		/* It should not be looped. We set MULTICAST_LOOP 0 */
		fatalp("Received our PDU. Ignoring it\n");
		return;
	}

	if (msg.msg_controllen < (socklen_t)sizeof(struct cmsghdr) ||
	    (msg.msg_flags & MSG_CTRUNC))
		local_addr.s_addr = my_ldp_addr.s_addr;
	else {
		for (cmptr = CMSG_FIRSTHDR(&msg); cmptr != NULL;
		    cmptr = CMSG_NXTHDR(&msg, cmptr))
			if (cmptr->cmsg_level == IPPROTO_IP &&
			    cmptr->cmsg_type == IP_RECVIF) {
				sdl = (struct sockaddr_dl *) CMSG_DATA(cmptr);
				break;
			}
		if (get_local_addr(sdl, &local_addr) != 0)
			local_addr.s_addr = my_ldp_addr.s_addr;
	}


	debugp("Read %d bytes from address %s Length: %.4d Version: %d\n",
	       c, inet_ntoa(rpdu.ldp_id), rpdu.length, rpdu.version);

	/* Fill the TLV messages */
	t = get_hello_tlv(recvspace + i, c - i);
	run_ldp_hello(&rpdu, t, &sender.sa, &local_addr, sock, may_connect);
}

void 
send_hello_alarm(int unused)
{
	struct ldp_peer *p, *ptmp;
	struct hello_info *hi, *hinext;
	time_t          t = time(NULL);
	int             olderrno = errno;

	if (may_connect == false)
		may_connect = true;
	/* Send hellos */
	if (!(t % ldp_hello_time))
		send_hello();

	/* Timeout -- */
	SLIST_FOREACH(p, &ldp_peer_head, peers)
		p->timeout--;

	/* Check for timeout */
	SLIST_FOREACH_SAFE(p, &ldp_peer_head, peers, ptmp)
		if (p->timeout < 1)
			switch (p->state) {
			case LDP_PEER_HOLDDOWN:
				debugp("LDP holddown expired for peer %s\n",
				       inet_ntoa(p->ldp_id));
				ldp_peer_delete(p);
				break;
			case LDP_PEER_ESTABLISHED:
			case LDP_PEER_CONNECTED:
				send_notification(p, 0,
				    NOTIF_FATAL|NOTIF_KEEP_ALIVE_TIMER_EXPIRED);
				warnp("Keepalive expired for %s\n",
				    inet_ntoa(p->ldp_id));
				ldp_peer_holddown(p);
				break;
			}	/* switch */

	/* send keepalives */
	if (!(t % ldp_keepalive_time)) {
		SLIST_FOREACH(p, &ldp_peer_head, peers)	
		    if (p->state == LDP_PEER_ESTABLISHED) {
			debugp("Sending KeepAlive to %s\n",
			    inet_ntoa(p->ldp_id));
			keep_alive(p);
		    }
	}

	/* Decrement and Check hello keepalives */
	SLIST_FOREACH_SAFE(hi, &hello_info_head, infos, hinext) {
		if (hi->keepalive != 0xFFFF)
			hi->keepalive--;
		if (hi->keepalive < 1)
			SLIST_REMOVE(&hello_info_head, hi, hello_info, infos);
	}

	/* Set the alarm again and bail out */
	alarm(1);
	errno = olderrno;
}

static void 
bail_out(int x)
{
	ldp_peer_holddown_all();
	flush_mpls_routes();
	exit(0);
}

/*
 * The big poll that catches every single event
 * on every socket.
 */
int
the_big_loop(void)
{
	int		sock_error;
	uint32_t	i;
	socklen_t       sock_error_size = sizeof(int);
	struct ldp_peer *p;
	struct com_sock	*cs;
	struct pollfd	pfd[MAX_POLL_FDS];
	struct hello_socket *hs;
	nfds_t pollsum;

	assert(MAX_POLL_FDS > 5);

	SLIST_INIT(&hello_info_head);

	signal(SIGALRM, send_hello_alarm);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, bail_out);
	signal(SIGTERM, bail_out);

	/* Send first hellos in 5 seconds. Avoid No hello notifications */
	may_connect = false;
	alarm(5);

	route_socket = socket(PF_ROUTE, SOCK_RAW, AF_UNSPEC);

	sock_error = bind_current_routes();
	if (sock_error != LDP_E_OK) {
		fatalp("Cannot get current routes\n");
		return sock_error;
	}

	for (;;) {
		pfd[0].fd = ls;
		pfd[0].events = POLLRDNORM;
		pfd[0].revents = 0;

		pfd[1].fd = route_socket;
		pfd[1].events = POLLRDNORM;
		pfd[1].revents = 0;

		pfd[2].fd = command_socket;
		pfd[2].events = POLLRDNORM;
		pfd[2].revents = 0;

		/* Hello sockets */
		pollsum = 3;
		SLIST_FOREACH(hs, &hello_socket_head, listentry) {
			pfd[pollsum].fd = hs->socket;
			pfd[pollsum].events = POLLIN;
			pfd[pollsum].revents = 0;
			pollsum++;
		}

		/* Command sockets */
		for (i=0; i < MAX_COMMAND_SOCKETS; i++)
			if (csockets[i].socket != -1) {
				if (pollsum >= MAX_POLL_FDS)
					break;
				pfd[pollsum].fd = csockets[i].socket;
				pfd[pollsum].events = POLLIN;
				pfd[pollsum].revents = 0;
				pollsum++;
			}

		/* LDP Peer sockets */
		SLIST_FOREACH(p, &ldp_peer_head, peers) {
			if (p->socket < 1)
				continue;
			switch (p->state) {
			    case LDP_PEER_CONNECTED:
			    case LDP_PEER_ESTABLISHED:
				if (pollsum >= MAX_POLL_FDS)
					break;
				pfd[pollsum].fd = p->socket;
				pfd[pollsum].events = POLLRDNORM;
				pfd[pollsum].revents = 0;
				pollsum++;
				break;
			    case LDP_PEER_CONNECTING:
				if (pollsum >= MAX_POLL_FDS)
					break;
				pfd[pollsum].fd = p->socket;
				pfd[pollsum].events = POLLWRNORM;
				pfd[pollsum].revents = 0;
				pollsum++;
				break;
			}
		}

		if (pollsum >= MAX_POLL_FDS) {
			fatalp("Too many sockets. Increase MAX_POLL_FDS\n");
			return LDP_E_TOO_MANY_FDS;
		}
		if (poll(pfd, pollsum, INFTIM) < 0) {
			if (errno != EINTR)
				fatalp("poll: %s", strerror(errno));
			continue;
		}

		for (i = 0; i < pollsum; i++) {
			if ((pfd[i].revents & POLLRDNORM) ||
			    (pfd[i].revents & POLLIN)) {
				if(pfd[i].fd == ls)
					new_peer_connection();
				else if (pfd[i].fd == route_socket) {
					struct rt_msg xbuf;
					int l;
					do {
						l = read(route_socket, &xbuf,
						    sizeof(xbuf));
					} while ((l == -1) && (errno == EINTR));

					if (l == -1)
						break;

					check_route(&xbuf, l);

				} else if (is_hello_socket(pfd[i].fd) == 1) {
					/* Receiving hello socket */
					recv_pdu(pfd[i].fd);
				} else if (pfd[i].fd == command_socket) {
					command_accept(command_socket);
				} else if ((cs = is_command_socket(pfd[i].fd))
						!= NULL) {
					command_dispatch(cs);
				} else {
					/* ldp peer socket */
					p = get_ldp_peer_by_socket(pfd[i].fd);
					if (p)
						recv_session_pdu(p);
				}
			} else if(pfd[i].revents & POLLWRNORM) {
				p = get_ldp_peer_by_socket(pfd[i].fd);
				if (!p)
					continue;
				if (getsockopt(pfd[i].fd, SOL_SOCKET, SO_ERROR,
				    &sock_error, &sock_error_size) != 0 ||
				    sock_error != 0) {
					ldp_peer_holddown(p);
					sock_error = 0;
				} else {
					p->state = LDP_PEER_CONNECTED;
					send_initialize(p);
				}
			}
		}

		for (int ri = 0; ri < replay_index; ri++) {
			debugp("Replaying: PID %d, SEQ %d\n",
				replay_rt[ri].m_rtm.rtm_pid,
				replay_rt[ri].m_rtm.rtm_seq);
			check_route(&replay_rt[ri], sizeof(struct rt_msg));
                }
		replay_index = 0;
	}	/* for (;;) */
}

void 
new_peer_connection()
{
	union sockunion peer_address, my_address;
	struct in_addr *peer_ldp_id = NULL;
	struct hello_info *hi;
	int             s;

	s = accept(ls, &peer_address.sa,
		& (socklen_t) { sizeof(union sockunion) } );
	if (s < 0) {
		fatalp("accept: %s", strerror(errno));
		return;
	}

	if (getsockname(s, &my_address.sa,
	    & (socklen_t) { sizeof(union sockunion) } )) {
		fatalp("new_peer_connection(): cannot getsockname\n");
		close(s);
		return;
	}

	if (peer_address.sa.sa_family == AF_INET)
		peer_address.sin.sin_port = 0;
	else if (peer_address.sa.sa_family == AF_INET6)
		peer_address.sin6.sin6_port = 0;
	else {
		fatalp("Unknown peer address family\n");
		close(s);
		return;
	}

	/* Already peered or in holddown ? */
	if (get_ldp_peer(&peer_address.sa) != NULL) {
		close(s);
		return;
	}

	warnp("Accepted a connection from %s\n", satos(&peer_address.sa));

	/* Verify if it should connect - XXX: no check for INET6 */
	if (peer_address.sa.sa_family == AF_INET &&
	    ntohl(peer_address.sin.sin_addr.s_addr) <
	    ntohl(my_address.sin.sin_addr.s_addr)) {
		fatalp("Peer %s: connect from lower ID\n",
		    satos(&peer_address.sa));
		close(s);
		return;
	}

	/* Match hello info in order to get ldp_id */
	SLIST_FOREACH(hi, &hello_info_head, infos) {
		if (sockaddr_cmp(&peer_address.sa,
		    &hi->transport_address.sa) == 0) {
			peer_ldp_id = &hi->ldp_id;
			break;
		}
	}
	if (peer_ldp_id == NULL) {
		fatalp("Got connection from %s, but no hello info exists\n",
		    satos(&peer_address.sa));
		close(s);
		return;
	} else
		ldp_peer_new(peer_ldp_id, &peer_address.sa, NULL,
		    ldp_holddown_time, s);

}

void 
send_initialize(struct ldp_peer * p)
{
	struct init_tlv ti;

	ti.type = htons(LDP_INITIALIZE);
	ti.length = htons(sizeof(struct init_tlv) - TLV_TYPE_LENGTH);
	ti.messageid = htonl(get_message_id());
	ti.cs_type = htons(TLV_COMMON_SESSION);
	ti.cs_len = htons(CS_LEN);
	ti.cs_version = htons(LDP_VERSION);
	ti.cs_keepalive = htons(2 * ldp_keepalive_time);
	ti.cs_adpvlim = 0;
	ti.cs_maxpdulen = htons(MAX_PDU_SIZE);
	ti.cs_peeraddress.s_addr = p->ldp_id.s_addr;
	ti.cs_peeraddrspace = 0;

	send_tlv(p, (struct tlv *) (void *) &ti);
}

void 
keep_alive(struct ldp_peer * p)
{
	struct ka_tlv   kt;

	kt.type = htons(LDP_KEEPALIVE);
	kt.length = htons(sizeof(kt.messageid));
	kt.messageid = htonl(get_message_id());

	send_tlv(p, (struct tlv *) (void *) &kt);

}

void 
recv_session_pdu(struct ldp_peer * p)
{
	struct ldp_pdu *rpdu;
	struct address_tlv *atlv;
	struct al_tlv  *altlv;
	struct init_tlv	*itlv;
	struct label_map_tlv *lmtlv;
	struct fec_tlv *fectlv;
	struct label_tlv *labeltlv;
	struct notification_tlv *nottlv;
	struct hello_info *hi;

	int             c;
	int32_t         wo = 0;
	struct tlv     *ttmp;
	unsigned char   recvspace[MAX_PDU_SIZE];

	memset(recvspace, 0, MAX_PDU_SIZE);

	c = recv(p->socket, (void *) recvspace, MAX_PDU_SIZE, MSG_PEEK);

	debugp("Ready to read %d bytes\n", c);

	if (c < 1) {		/* Session closed */
		warnp("Error in connection with %s\n", inet_ntoa(p->ldp_id));
		ldp_peer_holddown(p);
		return;
	}
	if (c > MAX_PDU_SIZE) {
		debugp("Incoming PDU size exceeds MAX_PDU_SIZE !\n");
		return;
	}
	if (c < MIN_PDU_SIZE) {
		debugp("PDU too small received from peer %s\n", inet_ntoa(p->ldp_id));
		return;
	}
	rpdu = (struct ldp_pdu *) recvspace;
	/* XXX: buggy messages may crash the whole thing */
	c = recv(p->socket, (void *) recvspace,
		ntohs(rpdu->length) + PDU_VER_LENGTH, MSG_WAITALL);
	rpdu = (struct ldp_pdu *) recvspace;

	/* Check if it's somehow OK... */
	if (check_recv_pdu(p, rpdu, c) != 0)
		return;

	debugp("Read %d bytes, PDU size: %d bytes\n", c, ntohs(rpdu->length));
	wo = sizeof(struct ldp_pdu);

	while (wo + TLV_TYPE_LENGTH < (uint)c) {

		ttmp = (struct tlv *) (&recvspace[wo]);

		if ((ntohs(ttmp->type) != LDP_KEEPALIVE) &&
		    (ntohs(ttmp->type) != LDP_LABEL_MAPPING)) {
			debugp("Got Type: 0x%.4X (Length: %d) from %s\n",
			    ntohs(ttmp->type), ntohs(ttmp->length),
			    inet_ntoa(p->ldp_id));
		} else
			debugp("Got Type: 0x%.4X (Length: %d) from %s\n",
			    ntohs(ttmp->type), ntohs(ttmp->length),
			    inet_ntoa(p->ldp_id));

		/* Should we get the message ? */
		if (p->state != LDP_PEER_ESTABLISHED &&
		    ntohs(ttmp->type) != LDP_INITIALIZE &&
		    ntohs(ttmp->type) != LDP_KEEPALIVE &&
		    ntohs(ttmp->type) != LDP_NOTIFICATION)
			break;
		/* The big switch */
		switch (ntohs(ttmp->type)) {
		case LDP_INITIALIZE:
			itlv = (struct init_tlv *)ttmp;
			/* Check size */
			if (ntohs(itlv->length) <
			    sizeof(struct init_tlv) - TLV_TYPE_LENGTH) {
				debugp("Bad size\n");
				send_notification(p, 0,
				    NOTIF_BAD_PDU_LEN | NOTIF_FATAL);
				ldp_peer_holddown(p);
				break;
			}
			/* Check version */
			if (ntohs(itlv->cs_version) != LDP_VERSION) {
				debugp("Bad version");
				send_notification(p, ntohl(itlv->messageid),
					NOTIF_BAD_LDP_VER | NOTIF_FATAL);
				ldp_peer_holddown(p);
				break;
			}
			/* Check if we got any hello from this one */
			SLIST_FOREACH(hi, &hello_info_head, infos)
				if (hi->ldp_id.s_addr == rpdu->ldp_id.s_addr)
					break;
			if (hi == NULL) {
			    debugp("No hello. Moving peer to holddown\n");
			    send_notification(p, ntohl(itlv->messageid),
				NOTIF_SESSION_REJECTED_NO_HELLO | NOTIF_FATAL);
			    ldp_peer_holddown(p);
			    break;
			}

			if (!p->master) {
				keep_alive(p);
				send_initialize(p);
			} else {
				p->state = LDP_PEER_ESTABLISHED;
				p->established_t = time(NULL);
				keep_alive(p);

				/*
				 * Recheck here ldp id because we accepted
				 * connection without knowing who is it for sure
				 */
				p->ldp_id.s_addr = rpdu->ldp_id.s_addr;

				fatalp("LDP neighbour %s is UP\n",
				    inet_ntoa(p->ldp_id));
				mpls_add_ldp_peer(p);
				send_addresses(p);
				send_all_bindings(p);
			}
			break;
		case LDP_KEEPALIVE:
			if ((p->state == LDP_PEER_CONNECTED) && (!p->master)) {
				p->state = LDP_PEER_ESTABLISHED;
				p->established_t = time(NULL);
				fatalp("LDP neighbour %s is UP\n",
				    inet_ntoa(p->ldp_id));
				mpls_add_ldp_peer(p);
				send_addresses(p);
				send_all_bindings(p);
			}
			p->timeout = p->holdtime;
			break;
		case LDP_ADDRESS:
			/* Add peer addresses */
			atlv = (struct address_tlv *) ttmp;
			altlv = (struct al_tlv *) (&atlv[1]);
			add_ifaddresses(p, altlv);
			print_bounded_addresses(p);
			break;
		case LDP_ADDRESS_WITHDRAW:
			atlv = (struct address_tlv *) ttmp;
			altlv = (struct al_tlv *) (&atlv[1]);
			del_ifaddresses(p, altlv);
			break;
		case LDP_LABEL_MAPPING:
			lmtlv = (struct label_map_tlv *) ttmp;
			fectlv = (struct fec_tlv *) (&lmtlv[1]);
			labeltlv = (struct label_tlv *)((unsigned char *)fectlv
				+ ntohs(fectlv->length) + TLV_TYPE_LENGTH);
			map_label(p, fectlv, labeltlv);
			break;
		case LDP_LABEL_REQUEST:
			lmtlv = (struct label_map_tlv *) ttmp;
			fectlv = (struct fec_tlv *) (&lmtlv[1]);
			switch (request_respond(p, lmtlv, fectlv)) {
			case LDP_E_BAD_FEC:
				send_notification(p, ntohl(lmtlv->messageid),
					NOTIF_UNKNOWN_TLV);
				break;
			case LDP_E_BAD_AF:
				send_notification(p, ntohl(lmtlv->messageid),
					NOTIF_UNSUPPORTED_AF);
				break;
			case LDP_E_NO_SUCH_ROUTE:
				send_notification(p, ntohl(lmtlv->messageid),
					NOTIF_NO_ROUTE);
				break;
			}
			break;
		case LDP_LABEL_WITHDRAW:
			lmtlv = (struct label_map_tlv *) ttmp;
			fectlv = (struct fec_tlv *) (&lmtlv[1]);
			if (withdraw_label(p, fectlv) == LDP_E_OK) {
				/* Send RELEASE */
				prepare_release(ttmp);
				send_tlv(p, ttmp);
				}
			break;
		case LDP_LABEL_RELEASE:
			/*
			 * XXX: we need to make a timed queue...
			 * For now I just assume peers are processing messages
			 * correctly so I just ignore confirmations
			 */
			wo = -1;	/* Ignore rest of message */
			break;
		case LDP_LABEL_ABORT:
		/* XXX: For now I pretend I can process everything
		 * RFC 5036, Section 3.5.9.1
		 * If an LSR receives a Label Abort Request Message after it
		 * has responded to the Label Request in question with a Label
		 * Mapping message or a Notification message, it ignores the
		 * abort request.
		 */
			wo = -1;
			break;
		case LDP_NOTIFICATION:
			nottlv = (struct notification_tlv *) ttmp;
			nottlv->st_code = ntohl(nottlv->st_code);
			fatalp("Got notification 0x%X from peer %s\n",
			    nottlv->st_code, inet_ntoa(p->ldp_id));
			if (nottlv->st_code >> 31) {
				fatalp("LDP peer %s signalized %s\n",
				    inet_ntoa(p->ldp_id),
				    NOTIF_STR[(nottlv->st_code << 1) >> 1]);
				ldp_peer_holddown(p);
				wo = -1;
			}
			break;
		case LDP_HELLO:
			/* No hellos should came on tcp session */
			wo = -1;
			break;
		default:
			warnp("Unknown TLV received from %s\n",
			    inet_ntoa(p->ldp_id));
			debug_tlv(ttmp);
			wo = -1;/* discard the rest of the message */
			break;
		}
		if (wo < 0) {
			debugp("Discarding the rest of the message\n");
			break;
		} else {
			wo += ntohs(ttmp->length) + TLV_TYPE_LENGTH;
			debugp("WORKED ON %u bytes (Left %d)\n", wo, c - wo);
		}
	}			/* while */

}

/* Sends a pdu, tlv pair to a connected peer */
int 
send_message(struct ldp_peer * p, struct ldp_pdu * pdu, struct tlv * t)
{
	unsigned char   sendspace[MAX_PDU_SIZE];

	/* Check if peer is connected */
	switch (p->state) {
	case LDP_PEER_CONNECTED:
	case LDP_PEER_ESTABLISHED:
		break;
	default:
		return -1;
	}

	/* Check length validity first */
	if (ntohs(pdu->length) !=
	    ntohs(t->length) + TLV_TYPE_LENGTH + PDU_PAYLOAD_LENGTH) {
		fatalp("LDP: TLV - PDU incompability. Message discarded\n");
		fatalp("LDP: TLV len %d - PDU len %d\n", ntohs(t->length),
		    ntohs(pdu->length));
		return -1;
	}
	if (ntohs(t->length) + PDU_VER_LENGTH > MAX_PDU_SIZE) {
		fatalp("Message to large discarded\n");
		return -1;
	}
	/* Arrange them in a buffer and send */
	memcpy(sendspace, pdu, sizeof(struct ldp_pdu));
	memcpy(sendspace + sizeof(struct ldp_pdu), t,
	    ntohs(t->length) + TLV_TYPE_LENGTH);

	/* Report keepalives only for DEBUG */
	if ((ntohs(t->type) != 0x201) && (ntohs(t->type) != 0x400)) {
		debugp("Sending message type 0x%.4X to %s (size: %d)\n",
		    ntohs(t->type), inet_ntoa(p->ldp_id), ntohs(t->length));
	} else
	/* downgraded from warnp to debugp for now */
		debugp("Sending message type 0x%.4X to %s (size: %d)\n",
		    ntohs(t->type), inet_ntoa(p->ldp_id), ntohs(t->length));

	/* Send it finally */
	return send(p->socket, sendspace,
		ntohs(pdu->length) + PDU_VER_LENGTH, 0);
}

/*
 * Encapsulates TLV into a PDU and sends it to a peer
 */
int 
send_tlv(struct ldp_peer * p, struct tlv * t)
{
	struct ldp_pdu  pdu;

	pdu.version = htons(LDP_VERSION);
	inet_aton(LDP_ID, &pdu.ldp_id);
	pdu.label_space = 0;
	pdu.length = htons(ntohs(t->length) + TLV_TYPE_LENGTH +
		PDU_PAYLOAD_LENGTH);

	return send_message(p, &pdu, t);
}


int 
send_addresses(struct ldp_peer * p)
{
	struct address_list_tlv *t;
	int             ret;

	t = build_address_list_tlv();

	ret = send_tlv(p, (struct tlv *) t);
	free(t);
	return ret;

}
