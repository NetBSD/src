/*	$NetBSD: server.c,v 1.6 2009/10/25 19:28:45 plunky Exp $	*/

/*-
 * Copyright (c) 2008-2009 Iain Hibbert
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: server.c,v 1.6 2009/10/25 19:28:45 plunky Exp $");

#include <sys/ioctl.h>

#include <net/ethertypes.h>

#include <bluetooth.h>
#include <errno.h>
#include <sdp.h>
#include <unistd.h>

#include "btpand.h"
#include "bnep.h"

static struct event	server_ev;
static int		server_count;

static sdp_session_t	server_ss;
static uint32_t		server_handle;
static sdp_data_t	server_record;

static char *		server_ipv4_subnet;
static char *		server_ipv6_subnet;
static uint16_t		server_proto[] = { ETHERTYPE_IP, ETHERTYPE_ARP, ETHERTYPE_IPV6 };
static size_t		server_nproto = __arraycount(server_proto);

static void server_open(void);
static void server_read(int, short, void *);
static void server_down(channel_t *);
static void server_update(void);
static void server_mkrecord(void);

void
server_init(void)
{

	if (server_limit == 0)
		return;

	server_open();
	server_update();
}

/*
 * Start listening on server socket
 */
static void
server_open(void)
{
	struct sockaddr_bt sa;
	socklen_t len;
	uint16_t mru;
	int fd;

	fd = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if (fd == -1) {
		log_err("Could not open L2CAP socket: %m");
		exit(EXIT_FAILURE);
	}

	memset(&sa, 0, sizeof(sa));
	sa.bt_family = AF_BLUETOOTH;
	sa.bt_len = sizeof(sa);
	sa.bt_psm = l2cap_psm;
	bdaddr_copy(&sa.bt_bdaddr, &local_bdaddr);
	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
		log_err("Could not bind server socket: %m");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(fd, BTPROTO_L2CAP,
	    SO_L2CAP_LM, &l2cap_mode, sizeof(l2cap_mode)) == -1) {
		log_err("Could not set link mode (0x%4.4x): %m", l2cap_mode);
		exit(EXIT_FAILURE);
	}
	len = sizeof(l2cap_mode);
	getsockopt(fd, BTPROTO_L2CAP, SO_L2CAP_LM, &l2cap_mode, &len);

	mru = BNEP_MTU_MIN;
	if (setsockopt(fd, BTPROTO_L2CAP,
	    SO_L2CAP_IMTU, &mru, sizeof(mru)) == -1) {
		log_err("Could not set L2CAP IMTU (%d): %m", mru);
		exit(EXIT_FAILURE);
	}

	if (listen(fd, 0) == -1) {
		log_err("Could not listen on server socket: %m");
		exit(EXIT_FAILURE);
	}

	event_set(&server_ev, fd, EV_READ | EV_PERSIST, server_read, NULL);
	if (event_add(&server_ev, NULL) == -1) {
		log_err("Could not add server event: %m");
		exit(EXIT_FAILURE);
	}

	log_info("server socket open");
}

/*
 * handle connection request
 */
static void
server_read(int s, short ev, void *arg)
{
	struct sockaddr_bt ra, la;
	channel_t *chan;
	socklen_t len;
	int fd, n;
	uint16_t mru, mtu;

	assert(server_count < server_limit);

	len = sizeof(ra);
	fd = accept(s, (struct sockaddr *)&ra, &len);
	if (fd == -1)
		return;

	n = 1;
	if (ioctl(fd, FIONBIO, &n) == -1) {
		log_err("Could not set NonBlocking IO: %m");
		close(fd);
		return;
	}

	len = sizeof(mru);
	if (getsockopt(fd, BTPROTO_L2CAP, SO_L2CAP_IMTU, &mru, &len) == -1) {
		log_err("Could not get L2CAP IMTU: %m");
		close(fd);
		return;
	}
	if(mru < BNEP_MTU_MIN) {
		log_err("L2CAP IMTU too small (%d)", mru);
		close(fd);
		return;
	}

	len = sizeof(mtu);
	if (getsockopt(fd, BTPROTO_L2CAP, SO_L2CAP_OMTU, &mtu, &len) == -1) {
		log_err("Could not get L2CAP OMTU: %m");
		close(fd);
		return;
	}
	if (mtu < BNEP_MTU_MIN) {
		log_err("L2CAP OMTU too small (%d)", mtu);
		close(fd);
		return;
	}

	len = sizeof(n);
	if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &n, &len) == -1) {
		log_err("Could not get socket send buffer size: %m");
		close(fd);
		return;
	}

	if (n < (mtu * 2)) {
		n = mtu * 2;
		if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n)) == -1) {
			log_err("Could not set socket send buffer size (%d): %m", n);
			close(fd);
			return;
		}
	}

	n = mtu;
	if (setsockopt(fd, SOL_SOCKET, SO_SNDLOWAT, &n, sizeof(n)) == -1) {
		log_err("Could not set socket low water mark (%d): %m", n);
		close(fd);
		return;
	}

	len = sizeof(la);
	if (getsockname(fd, (struct sockaddr *)&la, &len) == -1) {
		log_err("Could not get socket address: %m");
		close(fd);
		return;
	}

	log_info("Accepted connection from %s", bt_ntoa(&ra.bt_bdaddr, NULL));

	chan = channel_alloc();
	if (chan == NULL) {
		close(fd);
		return;
	}

	chan->send = bnep_send;
	chan->recv = bnep_recv;
	chan->down = server_down;
	chan->mru = mru;
	chan->mtu = mtu;
	b2eaddr(chan->raddr, &ra.bt_bdaddr);
	b2eaddr(chan->laddr, &la.bt_bdaddr);
	chan->state = CHANNEL_WAIT_CONNECT_REQ;
	channel_timeout(chan, 10);
	if (!channel_open(chan, fd)) {
		chan->state = CHANNEL_CLOSED;
		channel_free(chan);
		close(fd);
		return;
	}

	if (++server_count == server_limit) {
		log_info("Server limit reached, closing server socket");
		event_del(&server_ev);
		close(s);
	}

	server_update();
}

/*
 * Shut down a server channel, we need to update the service record and
 * may want to restart accepting connections on the server socket
 */
static void
server_down(channel_t *chan)
{

	assert(server_count > 0);

	channel_close(chan);

	if (server_count-- == server_limit)
		server_open();

	server_update();
}

static void
server_update(void)
{
	bool rv;

	if (service_type == NULL)
		return;

	if (server_ss == NULL) {
		server_ss = sdp_open_local(control_path);
		if (server_ss == NULL) {
			log_err("failed to contact SDP server");
			return;
		}
	}

	server_mkrecord();

	if (server_handle == 0)
		rv = sdp_record_insert(server_ss, &local_bdaddr,
		    &server_handle, &server_record);
	else
		rv = sdp_record_update(server_ss, server_handle,
		    &server_record);

	if (!rv) {
		log_err("%s: %m", service_type);
		exit(EXIT_FAILURE);
	}
}

static void
server_mkrecord(void)
{
	static uint8_t data[256];	/* tis enough */
	sdp_data_t buf;
	size_t i;

	buf.next = data;
	buf.end = data + sizeof(data);

	sdp_put_uint16(&buf, SDP_ATTR_SERVICE_RECORD_HANDLE);
	sdp_put_uint32(&buf, 0x00000000);

	sdp_put_uint16(&buf, SDP_ATTR_SERVICE_CLASS_ID_LIST);
	sdp_put_seq(&buf, 3);
	sdp_put_uuid16(&buf, service_class);

	sdp_put_uint16(&buf, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	sdp_put_seq(&buf, 8 + 10 + 3 * server_nproto);
	sdp_put_seq(&buf, 6);
	sdp_put_uuid16(&buf, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_uint16(&buf, l2cap_psm);
	sdp_put_seq(&buf, 8 + 3 * server_nproto);
	sdp_put_uuid16(&buf, SDP_UUID_PROTOCOL_BNEP);
	sdp_put_uint16(&buf, 0x0100);	/* v1.0 */
	sdp_put_seq(&buf, 3 * server_nproto);
	for (i = 0; i < server_nproto; i++)
		sdp_put_uint16(&buf, server_proto[i]);

	sdp_put_uint16(&buf, SDP_ATTR_BROWSE_GROUP_LIST);
	sdp_put_seq(&buf, 3);
	sdp_put_uuid16(&buf, SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);

	sdp_put_uint16(&buf, SDP_ATTR_LANGUAGE_BASE_ATTRIBUTE_ID_LIST);
	sdp_put_seq(&buf, 9);
	sdp_put_uint16(&buf, 0x656e);	/* "en" */
	sdp_put_uint16(&buf, 106);	/* UTF-8 */
	sdp_put_uint16(&buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID);

	sdp_put_uint16(&buf, SDP_ATTR_SERVICE_AVAILABILITY);
	sdp_put_uint8(&buf, (UINT8_MAX - server_count * UINT8_MAX / server_limit));

	sdp_put_uint16(&buf, SDP_ATTR_BLUETOOTH_PROFILE_DESCRIPTOR_LIST);
	sdp_put_seq(&buf, 8);
	sdp_put_seq(&buf, 6);
	sdp_put_uuid16(&buf, service_class);
	sdp_put_uint16(&buf, 0x0100);	/* v1.0 */

	sdp_put_uint16(&buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID
	    + SDP_ATTR_SERVICE_NAME_OFFSET);
	sdp_put_str(&buf, service_name, -1);

	sdp_put_uint16(&buf, SDP_ATTR_PRIMARY_LANGUAGE_BASE_ID
	    + SDP_ATTR_SERVICE_DESCRIPTION_OFFSET);
	sdp_put_str(&buf, service_desc, -1);

	sdp_put_uint16(&buf, SDP_ATTR_SECURITY_DESCRIPTION);
	sdp_put_uint16(&buf, (l2cap_mode & L2CAP_LM_AUTH) ?  0x0001 : 0x0000);

	if (service_class == SDP_SERVICE_CLASS_NAP) {
		sdp_put_uint16(&buf, SDP_ATTR_NET_ACCESS_TYPE);
		sdp_put_uint16(&buf, 0x0004);	/* 10Mb Ethernet */

		sdp_put_uint16(&buf, SDP_ATTR_MAX_NET_ACCESS_RATE);
		sdp_put_uint32(&buf, IF_Mbps(10) / 8);	/* octets/second */
	}

	if (service_class == SDP_SERVICE_CLASS_NAP
	    || service_class == SDP_SERVICE_CLASS_GN) {
		if (server_ipv4_subnet) {
			sdp_put_uint16(&buf, SDP_ATTR_IPV4_SUBNET);
			sdp_put_str(&buf, server_ipv4_subnet, -1);
		}

		if (server_ipv6_subnet) {
			sdp_put_uint16(&buf, SDP_ATTR_IPV6_SUBNET);
			sdp_put_str(&buf, server_ipv6_subnet, -1);
		}
	}

	server_record.next = data;
	server_record.end = buf.next;
}
