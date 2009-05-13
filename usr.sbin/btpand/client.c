/*	$NetBSD: client.c,v 1.2.2.1 2009/05/13 19:20:19 jym Exp $	*/

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
__RCSID("$NetBSD: client.c,v 1.2.2.1 2009/05/13 19:20:19 jym Exp $");

#include <bluetooth.h>
#include <errno.h>
#include <sdp.h>
#include <unistd.h>

#include "btpand.h"
#include "bnep.h"

static void client_down(channel_t *);
static void client_query(void);

void
client_init(void)
{
	struct sockaddr_bt sa;
	channel_t *chan;
	socklen_t len;
	int fd;
	uint16_t mru, mtu;

	if (bdaddr_any(&remote_bdaddr))
		return;

	if (service_type)
		client_query();

	fd = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if (fd == -1) {
		log_err("Could not open L2CAP socket: %m");
		exit(EXIT_FAILURE);
	}

	memset(&sa, 0, sizeof(sa));
	sa.bt_family = AF_BLUETOOTH;
	sa.bt_len = sizeof(sa);
	bdaddr_copy(&sa.bt_bdaddr, &local_bdaddr);
	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
		log_err("Could not bind client socket: %m");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(fd, BTPROTO_L2CAP, SO_L2CAP_LM,
	    &l2cap_mode, sizeof(l2cap_mode)) == -1) {
		log_err("Could not set link mode (0x%4.4x): %m", l2cap_mode);
		exit(EXIT_FAILURE);
	}

	mru = BNEP_MTU_MIN;
	if (setsockopt(fd, BTPROTO_L2CAP, SO_L2CAP_IMTU,
	    &mru, sizeof(mru)) == -1) {
		log_err("Could not set L2CAP IMTU (%d): %m", mru);
		exit(EXIT_FAILURE);
	}

	log_info("Opening connection to service 0x%4.4x at %s",
	    service_class, bt_ntoa(&remote_bdaddr, NULL));

	sa.bt_psm = l2cap_psm;
	bdaddr_copy(&sa.bt_bdaddr, &remote_bdaddr);
	if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
		log_err("Could not connect: %m");
		exit(EXIT_FAILURE);
	}

	len = sizeof(mru);
	if (getsockopt(fd, BTPROTO_L2CAP, SO_L2CAP_IMTU, &mru, &len) == -1) {
		log_err("Could not get IMTU: %m");
		exit(EXIT_FAILURE);
	}
	if (mru < BNEP_MTU_MIN) {
		log_err("L2CAP IMTU too small (%d)", mru);
		exit(EXIT_FAILURE);
	}

	len = sizeof(mtu);
	if (getsockopt(fd, BTPROTO_L2CAP, SO_L2CAP_OMTU, &mtu, &len) == -1) {
		log_err("Could not get L2CAP OMTU: %m");
		exit(EXIT_FAILURE);
	}
	if (mtu < BNEP_MTU_MIN) {
		log_err("L2CAP OMTU too small (%d)", mtu);
		exit(EXIT_FAILURE);
	}

	chan = channel_alloc();
	if (chan == NULL)
		exit(EXIT_FAILURE);

	chan->send = bnep_send;
	chan->recv = bnep_recv;
	chan->down = client_down;
	chan->mru = mru;
	chan->mtu = mtu;
	b2eaddr(chan->raddr, &remote_bdaddr);
	b2eaddr(chan->laddr, &local_bdaddr);
	chan->state = CHANNEL_WAIT_CONNECT_RSP;
	channel_timeout(chan, 10);
	if (!channel_open(chan, fd))
		exit(EXIT_FAILURE);

	bnep_send_control(chan, BNEP_SETUP_CONNECTION_REQUEST,
	    2, service_class, SDP_SERVICE_CLASS_PANU);
}

static void
client_down(channel_t *chan)
{

	log_err("Client connection shut down, exiting");
	exit(EXIT_FAILURE);
}

static void
client_query(void)
{
	uint8_t buf[12];	/* enough for SSP and AIL both */
	sdp_session_t ss;
	sdp_data_t ssp, ail, rsp, rec, value, pdl, seq;
	uintmax_t psm;
	uint16_t attr;
	bool rv;

	ss = sdp_open(&local_bdaddr, &remote_bdaddr);
	if (ss == NULL) {
		log_err("%s: %m", service_type);
		exit(EXIT_FAILURE);
	}

	log_info("Searching for %s service at %s",
	    service_type, bt_ntoa(&remote_bdaddr, NULL));

	seq.next = buf;
	seq.end = buf + sizeof(buf);

	/*
	 * build ServiceSearchPattern (9 bytes)
	 *
	 *	uuid16	"service_class"
	 *	uuid16	L2CAP
	 *	uuid16	BNEP
	 */
	ssp.next = seq.next;
	sdp_put_uuid16(&seq, service_class);
	sdp_put_uuid16(&seq, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_uuid16(&seq, SDP_UUID_PROTOCOL_BNEP);
	ssp.end = seq.next;

	/*
	 * build AttributeIDList (3 bytes)
	 *
	 *	uint16	ProtocolDescriptorList
	 */
	ail.next = seq.next;
	sdp_put_uint16(&seq, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	ail.end = seq.next;

	rv = sdp_service_search_attribute(ss, &ssp, &ail, &rsp);
	if (!rv) {
		log_err("%s: %m", service_type);
		exit(EXIT_FAILURE);
	}

	/*
	 * we expect the response to contain a list of records
	 * containing a ProtocolDescriptorList. Find the first
	 * one containing L2CAP and BNEP protocols and extract
	 * the PSM.
	 */
	rv = false;
	while (!rv && sdp_get_seq(&rsp, &rec)) {
		if (!sdp_get_attr(&rec, &attr, &value)
		    || attr != SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST)
			continue;

		sdp_get_alt(&value, &value);	/* drop any alt header */
		while (!rv && sdp_get_seq(&value, &pdl)) {
			if (sdp_get_seq(&pdl, &seq)
			    && sdp_match_uuid16(&seq, SDP_UUID_PROTOCOL_L2CAP)
			    && sdp_get_uint(&seq, &psm)
			    && sdp_get_seq(&pdl, &seq)
			    && sdp_match_uuid16(&seq, SDP_UUID_PROTOCOL_BNEP))
				rv = true;
		}
	}

	sdp_close(ss);

	if (!rv) {
		log_err("%s query failed", service_type);
		exit(EXIT_FAILURE);
	}

	l2cap_psm = (uint16_t)psm;
	log_info("Found PSM %u for service %s", l2cap_psm, service_type);
}
