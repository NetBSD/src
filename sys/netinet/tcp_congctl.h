/*	$NetBSD: tcp_congctl.h,v 1.3 2006/10/21 10:24:47 yamt Exp $	*/

/*
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Rui Paulo.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _NETINET_TCP_CONGCTL_H
#define _NETINET_TCP_CONGCTL_H

/*
 * Congestion control function table.
 */
struct tcp_congctl {
	int  (*fast_retransmit)(struct tcpcb *, const struct tcphdr *);
	void (*slow_retransmit)(struct tcpcb *);
	void (*fast_retransmit_newack)(struct tcpcb *, const struct tcphdr *);
	void (*newack)(struct tcpcb *, const struct tcphdr *);
	void (*cong_exp)(struct tcpcb *);
	
	uint32_t refcnt;
};

extern struct tcp_congctl tcp_reno_ctl;
extern struct tcp_congctl tcp_newreno_ctl;

extern struct simplelock tcp_congctl_slock;

#define TCPCC_MAXLEN 12

/* currently selected global congestion control */
struct tcp_congctl *tcp_congctl_global;
char   tcp_congctl_global_name[TCPCC_MAXLEN];

/* available global congestion control algorithms */
char   tcp_congctl_avail[10 * TCPCC_MAXLEN];

void   tcp_congctl_init(void);
int    tcp_congctl_register(const char *, struct tcp_congctl *);
int    tcp_congctl_unregister(const char *);
int    tcp_congctl_select(struct tcpcb *, const char *);
const char *
       tcp_congctl_bystruct(const struct tcp_congctl *);

#endif /* _NETINET_TCP_CONGCTL_H */
