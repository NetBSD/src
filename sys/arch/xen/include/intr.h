/*	$NetBSD: intr.h,v 1.43 2017/11/04 09:22:16 cherry Exp $	*/
/*	NetBSD intr.h,v 1.15 2004/10/31 10:39:34 yamt Exp	*/

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, and by Jason R. Thorpe.
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

#ifndef _XEN_INTR_H_
#define	_XEN_INTR_H_

#include <machine/intrdefs.h>

#ifndef _LOCORE
#include <xen/xen-public/xen.h>
#include <x86/intr.h>
#include <xen/xen.h>
#include <xen/hypervisor.h>
#include <machine/pic.h>
#include <sys/evcnt.h>

#include "opt_xen.h"


struct cpu_info;
/*
 * Struct describing an event channel. 
 */

struct evtsource {
	int ev_maxlevel;		/* max. IPL for this source */
	uint32_t ev_imask;		/* interrupt mask */
	struct intrhand *ev_handlers;	/* handler chain */
	struct evcnt ev_evcnt;		/* interrupt counter */
	struct cpu_info *ev_cpu;        /* cpu on which this event is bound */
	char ev_evname[32];		/* event counter name */
};

extern struct intrstub xenev_stubs[];
extern int irq2vect[256];
extern int vect2irq[256];
extern int irq2port[NR_EVENT_CHANNELS];

#ifdef MULTIPROCESSOR
int xen_intr_biglock_wrapper(void *);
#endif

int xen_intr_map(int *, int);
struct pic *intr_findpic(int);
void intr_add_pcibus(struct pcibus_attach_args *);
#if defined(DOM0OPS) || NPCI > 0
int xen_pirq_alloc(intr_handle_t *, int);
#endif /* defined(DOM0OPS) || NPCI > 0 */

#ifdef MULTIPROCESSOR
void xen_ipi_init(void);
int xen_send_ipi(struct cpu_info *, uint32_t);
void xen_broadcast_ipi(uint32_t);
#else
#define xen_ipi_init(_1) ((void) 0) /* nothing */
#define xen_send_ipi(_i1, _i2) (0) /* nothing */
#define xen_broadcast_ipi(_i1) ((void) 0) /* nothing */
#endif /* MULTIPROCESSOR */
#endif /* !_LOCORE */

#endif /* _XEN_INTR_H_ */
