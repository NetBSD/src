/*	$NetBSD: evt.h,v 1.2 2005/08/20 00:57:06 manu Exp $	*/

/* Id: evt.h,v 1.3 2004/11/29 23:30:39 manubsd Exp */

/*
 * Copyright (C) 2004 Emmanuel Dreyfus
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

struct evt {
	struct evtdump *dump;
	TAILQ_ENTRY(evt) next;
};

TAILQ_HEAD(evtlist, evt);

#define EVTLIST_MAX	32

#ifdef ENABLE_ADMINPORT
struct evtdump *evt_pop(void);
vchar_t *evt_dump(void);
void evt_push(struct sockaddr *, struct sockaddr *, int, vchar_t *);
#endif

#ifdef ENABLE_ADMINPORT
#define EVT_PUSH(src, dst, type, optdata) evt_push(src, dst, type, optdata);
#else
#define EVT_PUSH(src, dst, type, optdata) ;
#endif

#endif /* _EVT_H */
