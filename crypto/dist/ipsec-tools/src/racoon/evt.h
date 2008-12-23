/*	$NetBSD: evt.h,v 1.7 2008/12/23 14:03:12 tteras Exp $	*/

/* Id: evt.h,v 1.5 2006/01/19 10:24:09 fredsen Exp */

/*
 * Copyright (C) 2004 Emmanuel Dreyfus
 * Copyright (C) 2008 Timo Teras
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _EVT_H
#define _EVT_H

/*
 * Old style (deprecated) events which are polled.
 */

struct evtdump {
	size_t len;	
	struct sockaddr_storage src;
	struct sockaddr_storage dst;
	time_t timestamp;
	int type;
	/* 
	 * Optionnal list of struct isakmp_data 
	 * for type EVTT_ISAKMP_CFG_DONE
	 */
};

/* type */
#define EVTT_UNSEPC		0
#define EVTT_PHASE1_UP		1
#define EVTT_PHASE1_DOWN	2
#define EVTT_XAUTH_SUCCESS	3
#define EVTT_ISAKMP_CFG_DONE	4
#define EVTT_PHASE2_UP		5
#define EVTT_PHASE2_DOWN	6
#define EVTT_DPD_TIMEOUT	7
#define EVTT_PEER_NO_RESPONSE	8
#define EVTT_PEER_DELETE	9
#define EVTT_RACOON_QUIT	10
#define EVTT_XAUTH_FAILED	11
#define EVTT_OVERFLOW		12	/* Event queue overflowed */
#define EVTT_PEERPH1AUTH_FAILED	13
#define EVTT_PEERPH1_NOPROP	14	/* NO_PROPOSAL_CHOSEN & friends */
#define EVTT_NO_ISAKMP_CFG	15	/* no need to wait for mode_cfg */

/*
 * New style, asynchronous events.
 */

struct evt_async {
	uint32_t ec_type;
	time_t ec_timestamp;

	struct sockaddr_storage ec_ph1src;
	struct sockaddr_storage ec_ph1dst;
	u_int32_t ec_ph2msgid;

	/*
	 * Optionnal list of struct isakmp_data
	 * for type EVTT_ISAKMP_CFG_DONE
	 */
};

/* type */
#define EVT_RACOON_QUIT			0x0001

#define EVT_PHASE1_UP			0x0100
#define EVT_PHASE1_DOWN			0x0101
#define EVT_PHASE1_NO_RESPONSE		0x0102
#define EVT_PHASE1_NO_PROPOSAL		0x0103
#define EVT_PHASE1_AUTH_FAILED		0x0104
#define EVT_PHASE1_DPD_TIMEOUT		0x0105
#define EVT_PHASE1_PEER_DELETED		0x0106
#define EVT_PHASE1_MODE_CFG		0x0107
#define EVT_PHASE1_XAUTH_SUCCESS	0x0108
#define EVT_PHASE1_XAUTH_FAILED		0x0109

#define EVT_PHASE2_NO_PHASE1		0x0200
#define EVT_PHASE2_UP			0x0201
#define EVT_PHASE2_DOWN			0x0202
#define EVT_PHASE2_NO_RESPONSE		0x0203

#ifdef ENABLE_ADMINPORT

struct ph1handle;
struct ph2handle;

struct evt_listener {
	LIST_ENTRY(evt_listener) ll_chain;
	int fd;
};
LIST_HEAD(evt_listener_list, evt_listener);
#define EVT_LISTENER_LIST(x) struct evt_listener_list x

void evt_generic __P((int type, vchar_t *optdata));
void evt_phase1 __P((const struct ph1handle *ph1, int type, vchar_t *optdata));
void evt_phase2 __P((const struct ph2handle *ph2, int type, vchar_t *optdata));
vchar_t *evt_dump __P((void));

int  evt_subscribe __P((struct evt_listener_list *list, int fd));
void evt_list_init __P((struct evt_listener_list *list));
void evt_list_cleanup __P((struct evt_listener_list *list));

#else

#define EVT_LISTENER_LIST(x)

#define evt_generic(type, optdata) ;
#define evt_phase1(ph1, type, optdata) ;
#define evt_phase2(ph2, type, optdata) ;

#define evt_subscribe(eventlist, fd) ;
#define evt_list_init(eventlist) ;
#define evt_list_cleanup(eventlist) ;
#define evt_get_fdmask(nfds, fdset) nfds
#define evt_handle_fdmask(fdset) ;

#endif /* ENABLE_ADMINPORT */

#endif /* _EVT_H */
