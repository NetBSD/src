/*	$NetBSD: events.c,v 1.4 2004/04/24 19:32:37 cl Exp $	*/

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: events.c,v 1.4 2004/04/24 19:32:37 cl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>

#include <machine/intrdefs.h>

#include <machine/xen.h>
#include <machine/hypervisor.h>
#include <machine/events.h>

struct pic xenev_pic = {
	.pic_dev = {
		.dv_xname = "xen_fakepic",
	},
	.pic_type = PIC_XEN,
	.pic_lock = __SIMPLELOCK_UNLOCKED,
};

typedef struct ev_vector {
	ev_handler_t	ev_handler;
} ev_vector_t;

static ev_vector_t ev_vectors[MAXEVENTS];

int xen_debug_handler(void *);
int xen_die_handler(void *);

void
init_events()
{
	int i;

	for (i = 0; i < MAXEVENTS; i++)
		ev_vectors[i].ev_handler = NULL;

	event_set_handler(_EVENT_DIE, &xen_die_handler, NULL, IPL_DIE);
	hypervisor_enable_event(_EVENT_DIE);

	event_set_handler(_EVENT_DEBUG, &xen_debug_handler, NULL, IPL_DEBUG);
	hypervisor_enable_event(_EVENT_DEBUG);
}

unsigned int
do_event(int num, struct trapframe *regs)
{
	ev_vector_t  *ev;
	struct cpu_info *ci;

	if (num >= MAXEVENTS) {
#ifdef DIAGNOSTIC
		printf("event number %d > MAXEVENTS\n", num);
#endif
		return ENOENT;
	}

	if (0 && num == 4) {
		ci = &cpu_info_primary;
		printf("do_event %d called, ilevel %d\n", num, ci->ci_ilevel);
	}

	ev = &ev_vectors[num];

	if (ev->ev_handler == NULL) {
		printf("no handler for %d\n", num);
		goto out;
	}
    
	ci = &cpu_info_primary;

	hypervisor_acknowledge_event(num);
	__asm__ __volatile__ (
		"   movl $1f,%%esi	;"
		"   jmp  *%%eax		;"
		"1:			"
		: : "a" (ci->ci_isources[num]->is_recurse),
		"b" (ci->ci_ilevel)
		: "esi", "ecx", "edx", "memory");

	if (0 && num == 4)
		printf("do_event %d done, ipending %08x\n", num,
		    ci->ci_ipending);

 out:
	return 0;
}

int
event_set_handler(int num, ev_handler_t handler, void *arg, int level)
{
	struct intrsource *isp;
	struct intrhand *ih;
	struct cpu_info *ci;

	if (num >= MAXEVENTS) {
#ifdef DIAGNOSTIC
		printf("event number %d > MAXEVENTS\n", num);
#endif
		return ENOENT;
	}

	MALLOC(isp, struct intrsource *, sizeof (struct intrsource), M_DEVBUF,
	    M_WAITOK|M_ZERO);
	if (isp == NULL)
		panic("can't allocate fixed interrupt source");
	MALLOC(ih, struct intrhand *, sizeof (struct intrhand), M_DEVBUF,
	    M_WAITOK|M_ZERO);
	if (ih == NULL)
		panic("can't allocate fixed interrupt source");

	ci = &cpu_info_primary;

	isp->is_recurse = xenev_stubs[num].ist_recurse;
	isp->is_resume = xenev_stubs[num].ist_resume;
	ih->ih_level = level;
	ih->ih_fun = handler;
	ih->ih_arg = arg;
	ih->ih_next = NULL;
	isp->is_handlers = ih;
	isp->is_pic = &xenev_pic;
	ci->ci_isources[num] = isp;
	evcnt_attach_dynamic(&isp->is_evcnt, EVCNT_TYPE_INTR, NULL,
	    ci->ci_dev->dv_xname, "xenev");

	intr_calculatemasks(ci);

	ev_vectors[num].ev_handler = xenev_stubs[num].ist_entry;
	return 0;
}

int
xen_die_handler(void *arg)
{
	printf("die event\n");
	return 0;
}

int
xen_debug_handler(void *arg)
{
	printf("die event\n");
	return 0;
}

