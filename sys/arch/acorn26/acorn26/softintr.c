/* $NetBSD: softintr.c,v 1.3 2003/07/14 22:48:20 lukem Exp $ */

/*
 * Copyright (c) 1999 Ben Harris.
 * Copyright (c) 1994-1998 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Soft interrupt and other generic interrupt functions.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: softintr.c,v 1.3 2003/07/14 22:48:20 lukem Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>

#include <machine/intr.h>

#include <uvm/uvm_extern.h>

#include <machine/machdep.h>
#include <machine/softintr.h>

#include <net/netisr.h>

struct softintr_handler {
	LIST_ENTRY(softintr_handler) sh_link;
	int	sh_ipl;
	void	(*sh_func)(void *);
	void	*sh_arg;
	int	sh_pending;
};

LIST_HEAD(sh_head, softintr_handler);

static struct sh_head sh_head;

static struct softintr_handler *sh_softnet;

static void dosoftnet(void *);

extern int hardsplx(int); /* XXX should be in a header somewhere */

void
softintr_init()
{

	LIST_INIT(&sh_head);
	sh_softnet = softintr_establish(IPL_SOFTNET, dosoftnet, NULL);
}

void
setsoftnet()
{

	softintr_schedule(sh_softnet);
}

/* Handle software interrupts */

void
dosoftints(int level)
{
	struct softintr_handler *sh;

again:
	for (sh = LIST_FIRST(&sh_head); sh != NULL && sh->sh_ipl > level;
	     sh = LIST_NEXT(sh, sh_link))
		if (sh->sh_pending) {
			hardsplx(sh->sh_ipl);
			uvmexp.softs++;
			sh->sh_pending = 0;
			sh->sh_func(sh->sh_arg);
			goto again;
		}
}

void *
softintr_establish(int ipl, void (*func)(void *), void *arg)
{
	struct softintr_handler *new, *sh;

	MALLOC(new, struct softintr_handler *, sizeof(*sh),
	       M_SOFTINTR, M_WAITOK);
	new->sh_ipl = ipl;
	new->sh_func = func;
	new->sh_arg = arg;
	new->sh_pending = 0;
	if (LIST_FIRST(&sh_head) == NULL ||
	    LIST_FIRST(&sh_head)->sh_ipl <= ipl)
		/* XXX This shouldn't need to be a special case */
		LIST_INSERT_HEAD(&sh_head, new, sh_link);
	else {
		for (sh = LIST_FIRST(&sh_head);
		     LIST_NEXT(sh, sh_link) != NULL
			     && LIST_NEXT(sh, sh_link)->sh_ipl > ipl;
		     sh = LIST_NEXT(sh, sh_link));
		LIST_INSERT_AFTER(sh, new, sh_link);
	}
	return new;
}

void
softintr_disestablish(void *cookie)
{

	panic("softintr_disestablish not implemented");
}

void
softintr_schedule(void *cookie)
{
	struct softintr_handler *sh = cookie;

	sh->sh_pending = 1;
}

static void
dosoftnet(void *arg)
{

#define DONETISR(bit, fn) do {						\
	if (netisr & (1 << (bit))) {					\
		atomic_clear_bit(&netisr, 1 << (bit));			\
		fn();							\
	}								\
} while (0)
#include <net/netisr_dispatch.h>
#undef DONETISR
}
