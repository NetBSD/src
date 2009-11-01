/*	$NetBSD: intr.h,v 1.29.12.2 2009/11/01 13:58:45 jym Exp $	*/
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
#include <xen/xen.h>
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <machine/cpu.h>
#include <machine/pic.h>
#include <sys/evcnt.h>

#include "opt_xen.h"

/*
 * Struct describing an event channel. 
 */

struct evtsource {
	int ev_maxlevel;		/* max. IPL for this source */
	uint32_t ev_imask;		/* interrupt mask */
	struct intrhand *ev_handlers;	/* handler chain */
	struct evcnt ev_evcnt;		/* interrupt counter */
	char ev_evname[32];		/* event counter name */
};

/*
 * Structure describing an interrupt level. struct cpu_info has an array of
 * IPL_MAX of theses. The index in the array is equal to the stub number of
 * the stubcode as present in vector.s
 */

struct intrstub {
#if 0
	void *ist_entry;
#endif
	void *ist_recurse; 
	void *ist_resume;
};

/* for x86 compatibility */
extern struct intrstub i8259_stubs[];
extern struct intrstub ioapic_edge_stubs[];
extern struct intrstub ioapic_level_stubs[];

struct iplsource {
	struct intrhand *ipl_handlers;   /* handler chain */
	void *ipl_recurse;               /* entry for spllower */
	void *ipl_resume;                /* entry for doreti */
	struct lwp *ipl_lwp;
	u_long ipl_evt_mask1;	/* pending events for this IPL */
	u_long ipl_evt_mask2[NR_EVENT_CHANNELS];
};



/*
 * Interrupt handler chains. These are linked in both the evtsource and
 * the iplsource.
 * The handler is called with its (single) argument.
 */

struct intrhand {
	int	(*ih_fun)(void *);
	void	*ih_arg;
	int	ih_level;
	int	(*ih_realfun)(void *);
	void	*ih_realarg;
	struct	intrhand *ih_ipl_next;
	struct	intrhand *ih_evt_next;
	struct cpu_info *ih_cpu;
};

struct xen_intr_handle {
	int pirq; /* also contains the  APIC_INT_* flags if NIOAPIC > 0 */
	int evtch;
};

extern struct intrstub xenev_stubs[];

#define IUNMASK(ci,level) (ci)->ci_iunmask[(level)]

extern void Xspllower(int);

int splraise(int);
void spllower(int);

#define SPL_ASSERT_BELOW(x) KDASSERT(curcpu()->ci_ilevel < (x))

/*
 * Miscellaneous
 */
#define	spl0()		spllower(IPL_NONE)
#define	splx(x)		spllower(x)

typedef uint8_t ipl_t;
typedef struct {
	ipl_t _ipl;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._ipl = ipl};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return splraise(icookie._ipl);
}

#include <sys/spl.h>

/*
 * Stub declarations.
 */

struct cpu_info;

struct pcibus_attach_args;

#ifdef MULTIPROCESSOR
int intr_biglock_wrapper(void *);
#endif

void intr_default_setup(void);
int x86_nmi(void);
void intr_calculatemasks(struct evtsource *);

void *intr_establish(int, struct pic *, int, int, int, int (*)(void *), void *, bool);
void intr_disestablish(struct intrhand *);
const char *intr_string(int);
void cpu_intr_init(struct cpu_info *);
int xen_intr_map(int *, int);
#ifdef INTRDEBUG
void intr_printconfig(void);
#endif
int intr_find_mpmapping(int, int, struct xen_intr_handle *);
struct pic *intr_findpic(int);
void intr_add_pcibus(struct pcibus_attach_args *);

int x86_send_ipi(struct cpu_info *, int);
void x86_broadcast_ipi(int);
void x86_multicast_ipi(int, int);

#endif /* !_LOCORE */

#endif /* _XEN_INTR_H_ */
