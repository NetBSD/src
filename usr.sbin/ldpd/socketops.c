/* $NetBSD: socketops.c,v 1.3 2010/12/30 11:29:21 kefren Exp $ */

/*-
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

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <stdio.h>
#include <ifaddrs.h>
#include <poll.h>

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

int             ls;			/* TCP listening socket on port 646 */
int             route_socket;		/* used to see when a route is added/deleted */
int		hello_socket;		/* hello multicast listener - transmitter */
int		command_socket;		/* Listening socket for interface command */
int             current_msg_id = 0x233;
int		command_port = LDP_COMMAND_PORT;
extern int      replay_index;
extern struct rt_msg replay_rt[REPLAY_MAX];
extern struct com_sock	csockets[MAX_COMMAND_SOCKETS];

int	ldp_hello_time = LDP_HELLO_TIME;
int	ldp_keepalive_time = LDP_KEEPALIVE_TIME;
int	ldp_holddown_time = LDP_HOLDTIME;

void	recv_pdu(int);
void	send_hello_alarm(int);
void	bail_out(int);
static int get_local_addr(struct sockaddr_dl *, struct in_addr *);

int 
create_hello_socket()
{
	struct ip_mreq  mcast_addr;
	int             s = socket(PF_INET, SOCK_DGRAM, 17);

	if (s < 0)
		return s;

	/*
	 * RFC3036 specifies we should listen to all subnet routers multicast
	 * group
	 */
	mcast_addr.imr_multiaddr.s_addr = inet_addr(ALL_ROUTERS);
	mcast_addr.imr_interface.s_addr = htonl(INADDR_ANY);

	socket_reuse_port(s);
	/* Bind it to port 646 on specific address */
	if (bind_socket(s, htonl(INADDR_ANY)) == -1) {
		warnp("Cannot bind hello socket\n");
		close(s);
		return -1;
	}
	/* We don't need to receive back our messages */
	if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &(uint8_t){0},
	    sizeof(uint8_t)) == -1) {
		fatalp("setsockopt: %s", strerror(errno));
		close(s);
		return -1;
	}
	/* Finally join the group */
        if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mcast_addr,
	    sizeof(mcast_addr)) == -1) {
                fatalp("setsockopt: %s", strerror(errno));
                close(s);
                return -1;
        }
	/* TTL:1, TOS: 0xc0 */
	if (set_mcast_ttl(s) == -1) {
		close(s);
		return -1;
	}
	if (set_tos(s) == -1) {
		fatalp("set_tos: %s", strerror(errno));
		close(s);
		return -1;
	}
	if (setsockopt(s, IPPROTO_IP, IP_RECVIF, &(uint32_t){1}, sizeof(uint32_t)) == -1) {
		fatalp("Cannot set IP_RECVIF\n");
		close(s);
		return -1;
	}
	hello_socket = s;
	return hello_socket;
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

/* Sets multicast TTL to 1 */
int
set_mcast_ttl(int s)
{
	int	ret;
	if ((ret = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &(int){1},
	    sizeof(int))) == -1)
		fatalp("set_mcast_ttl: %s", strerror(errno));
	return ret;
}

/* Sets TOS to 0xc0 aka IP Precedence 6 */
int 
set_tos(int s)
{
	int             ret;
	if ((ret = setsockopt(s, IPPROTO_IP, IP_TOS, &(int){0xc0},
	    sizeof(int))) == -1)
		fatalp("set_tos: %s", strerror(errno));
	return ret;
}

int 
socket_reuse_port(int s)
{
	int             ret;
	if ((ret = setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &(int){1},
	    sizeof(int))) == -1)
		fatalp("socket_reuse_port: %s", strerror(errno));
	return ret;
}

/* binds an UDP socket */
int 
bind_socket(int s, uint32_t addr)
{
	struct sockaddr_in sa;

	sa.sin_len = sizeof(sa);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(LDP_PORT);
	sa.sin_addr.s_addr = addr;
	if (bind(s, (struct sockaddr *) (&sa), sizeof(sa))) {
		fatalp("bind_socket: %s", strerror(errno));
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

	s = socket(PF_INET, SOCK_STREAM, 6);
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
	int sb = 0;			/* sent bytes */
	struct ifaddrs *ifa, *ifb;
	struct sockaddr_in *if_sa;
	char lastifname[20];

#define HELLO_MSG_SIZE (sizeof(struct ldp_pdu) + 	/* PDU */	\
			TLV_TYPE_LENGTH + MSGID_SIZE +	/* Hello TLV */	\
			/* Common Hello TLV */				\
			sizeof(struct common_hello_tlv) +		\
			/* IPv4 Transport Address */			\
			sizeof(struct transport_address_tlv))

	if ((v = calloc(1, HELLO_MSG_SIZE)) == NULL) {
		fatalp("malloc problem in send_hello()\n");
		return;
	}

	spdu = (struct ldp_pdu *)((char *)v);
	t = (struct hello_tlv *)(spdu + 1);
	cht = &t->ch;	/* Hello tlv struct includes CHT */
	trtlv = (struct transport_address_tlv *)(t + 1);

	/* Prepare PDU envelope */
	spdu->version = htons(LDP_VERSION);
	spdu->length = htons(HELLO_MSG_SIZE - PDU_VER_LENGTH);
	inet_aton(LDP_ID, &spdu->ldp_id);

	/* Prepare Hello TLV */
	t->type = htons(LDP_HELLO);
	t->length = htons(MSGID_SIZE +
			sizeof(struct common_hello_tlv) +
			sizeof(struct transport_address_tlv));
	/*
	 * I used ID 0 instead of htonl(get_message_id()) because I've
	 * seen hellos from a cisco router doing the same thing
	 */
	t->messageid = 0;

	/* Prepare Common Hello attributes */
	cht->type = htons(TLV_COMMON_HELLO);
	cht->length = htons(sizeof(cht->holdtime) + sizeof(cht->res));
	cht->holdtime = htons(ldp_holddown_time);
	cht->res = 0;

	/*
	 * Prepare Transport Address TLV RFC3036 says: "If this optional TLV
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
	inet_aton(ALL_ROUTERS, &sadest.sin_addr);

	if (getifaddrs(&ifa) == -1) {
		free(v);
		return;
	}
	
	lastifname[0] = '\0';
	for (ifb = ifa; ifb; ifb = ifb->ifa_next) {
		if_sa = (struct sockaddr_in *) ifb->ifa_addr;
		if (if_sa->sin_family != AF_INET)
			continue;
		if (ntohl(if_sa->sin_addr.s_addr) >> 24 == IN_LOOPBACKNET)
			continue;
		/* Send only once per interface, using master address */
		if (strcmp(ifb->ifa_name, lastifname) == 0)
			continue;
		debugp("Sending hello on %s\n", ifb->ifa_name);
		if (setsockopt(hello_socket, IPPROTO_IP, IP_MULTICAST_IF,
		    &if_sa->sin_addr, sizeof(struct in_addr)) == -1) {
			warnp("setsockopt failed: %s\n", strerror(errno));
			continue;
		}
		trtlv->address.s_addr = if_sa->sin_addr.s_addr;

		strlcpy(lastifname, ifb->ifa_name, sizeof(lastifname));

		/* Send to the wire */
		sb = sendto(hello_socket, v, HELLO_MSG_SIZE,
			    0, (struct sockaddr *) & sadest, sizeof(sadest));
		if (sb < (int)HELLO_MSG_SIZE)
		    fatalp("send: %s", strerror(errno));
		else
		    debugp("Send %d bytes (PDU: %d, Hello TLV: %d, CH: %d)\n",
			sb, (int) (sizeof(struct ldp_pdu) - PDU_VER_LENGTH),
		       (int) (TLV_TYPE_LENGTH + MSGID_SIZE),
		       (int) (sizeof(struct common_hello_tlv)));

	}
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
	struct sockaddr_in fromsa;
	struct sockaddr_dl *sdl = NULL;
	struct in_addr my_ldp_addr, local_addr;
	struct cmsghdr *cmptr;
	union {
		struct cmsghdr cm;
		char control[1024];
	} control_un;

	debugp("Entering RECV_PDU\n");

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	msg.msg_flags = 0;
	msg.msg_name = &fromsa;
	msg.msg_namelen = sizeof(fromsa);
	iov[0].iov_base = recvspace;
	iov[0].iov_len = sizeof(recvspace);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	c = recvmsg(sock, &msg, MSG_WAITALL);
	debugp("Incoming PDU size: %d\n", c);

	debugp("PDU from: %s\n", inet_ntoa(fromsa.sin_addr));

	/* Check to see if this is larger than MIN_PDU_SIZE */
	if (c < MIN_PDU_SIZE)
		return;

	/* Read the PDU */
	i = get_pdu(recvspace, &rpdu);

	/* We currently understand Version 1 */
	if (rpdu.version != LDP_VERSION) {
		fatalp("recv_pdu: Version mismatch\n");
		return;
	}

	/* Maybe it's our hello */
	inet_aton(LDP_ID, &my_ldp_addr);
	if (rpdu.ldp_id.s_addr == my_ldp_addr.s_addr) {
		fatalp("Received our PDU..\n");	/* it should be not looped */
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
	run_ldp_hello(&rpdu, t, &fromsa.sin_addr, &local_addr, sock);
}

void 
send_hello_alarm(int unused)
{
	struct ldp_peer *p;
	struct hello_info *hi, *hinext;
	time_t          t = time(NULL);
	int             olderrno = errno;

	/* Send hellos */
	if (!(t % ldp_hello_time))
		send_hello();

	/* Timeout -- */
	SLIST_FOREACH(p, &ldp_peer_head, peers)
		p->timeout--;

	/* Check for timeout */
check_peer:
	SLIST_FOREACH(p, &ldp_peer_head, peers)
		if (p->timeout < 1)
			switch (p->state) {
			case LDP_PEER_HOLDDOWN:
				debugp("LDP holddown expired for peer %s\n",
				       inet_ntoa(p->ldp_id));
				ldp_peer_delete(p);
				goto check_peer;
			case LDP_PEER_ESTABLISHED:
			case LDP_PEER_CONNECTED:
				send_notification(p, 0,
				    NOTIF_KEEP_ALIVE_TIMER_EXPIRED);
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

	/* Decrement hello info keepalives */
	SLIST_FOREACH(hi, &hello_info_head, infos)
		hi->keepalive--;

	/* Check hello keepalives */
	SLIST_FOREACH_SAFE(hi, &hello_info_head, infos, hinext)
		if (hi->keepalive < 1)
			SLIST_REMOVE(&hello_info_head, hi, hello_info, infos);

	/* Set the alarm again and bail out */
	alarm(1);
	errno = olderrno;
}

void 
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
void 
the_big_loop(void)
{
	int		sock_error;
	uint32_t	i;
	socklen_t       sock_error_size = sizeof(int);
	struct ldp_peer *p;
	struct com_sock	*cs;
	struct pollfd	pfd[MAX_POLL_FDS];

	SLIST_INIT(&hello_info_head);

	signal(SIGALRM, send_hello_alarm);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, bail_out);
	send_hello_alarm(1);

	route_socket = socket(PF_ROUTE, SOCK_RAW, AF_UNSPEC);

	if (bind_current_routes() != LDP_E_OK)
		fatalp("Cannot get current routes\n");

	for (;;) {
		nfds_t pollsum = 4;

		pfd[0].fd = ls;
		pfd[0].events = POLLRDNORM;
		pfd[0].revents = 0;

		pfd[1].fd = route_socket;
		pfd[1].events = POLLRDNORM;
		pfd[1].revents = 0;

		pfd[2].fd = command_socket;
		pfd[2].events = POLLRDNORM;
		pfd[2].revents = 0;

		/* Hello socket */
		pfd[3].fd = hello_socket;
		pfd[3].events = POLLIN;
		pfd[3].revents = 0;

		/* Command sockets */
		for (i=0; i < MAX_COMMAND_SOCKETS; i++)
			if (csockets[i].socket != -1) {
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
				pfd[pollsum].fd = p->socket;
				pfd[pollsum].events = POLLRDNORM;
				pfd[pollsum].revents = 0;
				pollsum++;
				break;
			    case LDP_PEER_CONNECTING:
				pfd[pollsum].fd = p->socket;
				pfd[pollsum].events = POLLWRNORM;
				pfd[pollsum].revents = 0;
				pollsum++;
				break;
			}
		}

		if (pollsum >= MAX_POLL_FDS) {
			fatalp("Too many sockets. Increase MAX_POLL_FDS\n");
			return;
			}
		if (poll(pfd, pollsum, INFTIM) < 0) {
			if (errno != EINTR)
				fatalp("poll: %s", strerror(errno));
			continue;
			}

		for (i = 0; i < pollsum; i++) {
			if ((pfd[i].revents & POLLRDNORM) ||
			    (pfd[i].revents & POLLIN)) {
				if(pfd[i].fd == ls) {
					new_peer_connection();
				} else if (pfd[i].fd == route_socket) {
					struct rt_msg xbuf;
					int l, to_read;
					do {
					    l = recv(route_socket, &xbuf,
					      sizeof(struct rt_msg), MSG_PEEK);
					} while ((l == -1) && (errno == EINTR));

					if (l == -1)
						break;

					to_read = l;
					l = 0;
					do {
					    l += recv(route_socket, &xbuf,
						to_read - l, MSG_WAITALL);
					} while (l != to_read);

					check_route(&xbuf, to_read);

				} else if (pfd[i].fd == hello_socket) {
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
				if ((getsockopt(pfd[i].fd, SOL_SOCKET, SO_ERROR,
					&sock_error, &sock_error_size) != 0) ||
					    (sock_error)) {
						ldp_peer_holddown(p);
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
	struct sockaddr_in sa, sin_me;
	int             s;

	s = accept(ls, (struct sockaddr *) & sa,
		& (socklen_t) { sizeof(struct sockaddr_in) } );
	if (s < 0) {
		fatalp("accept: %s", strerror(errno));
		return;
	}

	if (get_ldp_peer(&sa.sin_addr) != NULL) {
		close(s);
		return;
	}

	warnp("Accepted a connection from %s\n", inet_ntoa(sa.sin_addr));

	if (getsockname(s, (struct sockaddr *)&sin_me,
	    & (socklen_t) { sizeof(struct sockaddr_in) } )) {
		fatalp("new_peer_connection(): cannot getsockname\n");
		close(s);
		return;
	}

	if (ntohl(sa.sin_addr.s_addr) < ntohl(sin_me.sin_addr.s_addr)) {
		fatalp("Peer %s: connect from lower ID\n",
		    inet_ntoa(sa.sin_addr));
		close(s);
		return;
	}
	/* XXX: sa.sin_addr ain't peer LDP ID ... */
	ldp_peer_new(&sa.sin_addr, &sa.sin_addr, NULL, ldp_holddown_time, s);

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
	struct label_tlv *__packed labeltlv;
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
		    ntohs(ttmp->type) != LDP_KEEPALIVE)
			break;
		/* The big switch */
		switch (ntohs(ttmp->type)) {
		case LDP_INITIALIZE:
			itlv = (struct init_tlv *)ttmp;
			/* Check size */
			if (ntohs(itlv->length) <
			    sizeof(struct init_tlv) - TLV_TYPE_LENGTH) {
				send_notification(p, 0,
				    NOTIF_BAD_PDU_LEN | NOTIF_FATAL);
				ldp_peer_holddown(p);
				break;
			}
			/* Check version */
			if (ntohs(itlv->cs_version) != LDP_VERSION) {
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
		 * RFC 3036, Section 3.5.9.1
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
