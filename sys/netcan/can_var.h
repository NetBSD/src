/*	$NetBSD: can_var.h,v 1.2.10.2 2017/12/03 11:39:03 jdolecek Exp $	*/

/*-
 * Copyright (c) 2003, 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Robert Swindells and Manuel Bouyer
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

#ifndef _NETCAN_CAN_VAR_H_
#define _NETCAN_CAN_VAR_H_

#include <sys/queue.h>
#include <sys/device.h>
#include <netcan/can_link.h>

struct can_ifreq {
	char            cfr_name[IFNAMSIZ];	/* if name, e.g. "sja0" */
};

#ifdef _KERNEL
#include <sys/socketvar.h>

/*
 * common structure for CAN interface drivers. Should be at the start of
 * each driver's softc.
 */
struct canif_softc {
	device_t csc_dev;
	struct can_link_timecaps csc_timecaps; /* timing capabilities */
	struct can_link_timings csc_timings; /* operating timing values */
	uint32_t csc_linkmodes;
};

extern struct ifqueue canintrq;
extern struct domain candomain;

extern const struct pr_usrreqs can_usrreqs;

void can_ifattach(struct ifnet *);
void can_ifdetach(struct ifnet *);
void can_ifinit_timings(struct canif_softc *);
void can_mbuf_tag_clean(struct mbuf *);
void can_input(struct ifnet *, struct mbuf *);
void *can_ctlinput(int, struct sockaddr *, void *);
int can_ctloutput(int, struct socket *, struct sockopt *);
void can_init(void);
void canintr(void);
void can_bpf_mtap(struct ifnet *, struct mbuf *, bool);

#endif

#endif
