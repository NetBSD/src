/*	$NetBSD: interrupt.h,v 1.5.2.1 2014/08/10 06:54:04 tls Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

enum ipl_type {
	IPL_INTC,
	IPL_DMAC,
};

struct _ipl_dispatcher {
	int (*func)(void *);
	void *arg;
	int ipl;
	int channel;
	int bit;
	SLIST_ENTRY(_ipl_dispatcher) link;
};

struct _ipl_holder {
	u_int32_t mask;
};

struct _playstation2_evcnt {
	struct evcnt clock, sbus, dmac;
};
extern struct _playstation2_evcnt  _playstation2_evcnt;
extern struct clockframe playstation2_clockframe;

void interrupt_init_bootstrap(void);
void interrupt_init(void);
void softintr_dispatch(int);

/* for SIF BIOS */
void _sif_call_start(void);
void _sif_call_end(void);

/* SPL */
void md_ipl_register(enum ipl_type, struct _ipl_holder *);
void md_imask_update(void);
