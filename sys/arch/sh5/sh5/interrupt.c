/*	$NetBSD: interrupt.c,v 1.1 2002/07/05 13:32:05 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>	/* uvmexp.intrs */

#include <machine/intr.h>


/*
 * INTEVT to intrhand mapper.
 */
static int8_t intevt_to_ih[256];

struct intrhand {
	int	(*ih_func)(void *);
	void	*ih_arg;
	int	ih_type;
	int	ih_level;
	u_int	ih_intevt;
	int	ih_idx;
};

static struct intrhand *intrhand;
static int szintrhand;

#define	INTEVT_TO_MAP_INDEX(x)	((x) >> 5)
#define	INTEVT_TO_IH_INDEX(x)	intevt_to_ih[INTEVT_TO_MAP_INDEX(x)]
#define	INTEVT_IH(x)		(&intrhand[INTEVT_TO_IH_INDEX(x)])

static void (*intr_enable)(void *, u_int, int, int);
static void (*intr_disable)(void *, u_int);
static void *intr_arg;

struct evcnt _sh5_intr_events[NIPL];
static char *intr_names[NIPL] = {
	"softmisc", "softclock", "softnet", "softserial",
	"irq5", "irq6", "irq7", "irq8",
	"irq9", "irq10", "irq11", "irq12",
	"irq13", "irq14", "irq15"
};

void sh5_intr_dispatch(struct intrframe *);
static struct intrhand *alloc_ih(void);
static void free_ih(struct intrhand *);
static int spurious_interrupt(void *);


/*
 * SH5 Interrupt support.
 */
void
sh5_intr_init(int nhandles,
    void (*int_enable)(void *, u_int, int, int),
    void (*int_disable)(void *, u_int),
    void *arg)
{
	struct intrhand *ih;
	int i;

	ih = malloc(sizeof(*ih) * (nhandles + 1), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		panic("sh5_intr_init: Out of memory");

	intrhand = ih;
	szintrhand = nhandles + 1;
	intr_enable = int_enable;
	intr_disable = int_disable;
	intr_arg = arg;

	ih->ih_func = spurious_interrupt;
	ih->ih_arg = NULL;
	ih->ih_idx = 0;
	ih->ih_level = 0;
	ih->ih_intevt = 0;
	ih->ih_type = IST_NONE;

	for (i = 0; i < NIPL; i++)
		evcnt_attach_dynamic(&_sh5_intr_events[i], EVCNT_TYPE_INTR,
		    NULL, "intr", intr_names[i]);
}

void *
sh5_intr_establish(int intevt, int trigger, int level,
    int (*ih_func)(void *), void *ih_arg)
{
	struct intrhand *ih;

	KDASSERT(szintrhand != 0);

	ih = alloc_ih();
	ih->ih_func = ih_func;
	ih->ih_arg = ih_arg;
	ih->ih_level = level << SH5_CONREG_SR_IMASK_SHIFT;
	ih->ih_intevt = intevt;
	ih->ih_type = trigger;

	/* Map interrupt handler */
	INTEVT_TO_IH_INDEX(intevt) = ih->ih_idx;

	(*intr_enable)(intr_arg, intevt, trigger, level);

	return ((void *)ih);
}

void
sh5_intr_disestablish(void *cookie)
{
	struct intrhand *ih = cookie;

	(*intr_disable)(intr_arg, ih->ih_intevt);

	/* Unmap interrupt handler */
	INTEVT_TO_IH_INDEX(ih->ih_intevt) = 0;

	free_ih(ih);
}

struct evcnt *
sh5_intr_evcnt(void *cookie)
{
	struct intrhand *ih = cookie;

	return (&_sh5_intr_events[ih->ih_level - 1]);
}

void
sh5_intr_dispatch(struct intrframe *fr)
{
	struct intrhand *ih;

	KDASSERT(INTEVT_TO_MAP_INDEX(fr->if_state.sf_intevt) < 0x100);

	ih = INTEVT_IH(fr->if_state.sf_intevt);

	KDASSERT(ih->ih_func != NULL);

	if (ih->ih_level)
		_sh5_intr_events[ih->ih_level - 1].ev_count++;

	if ((ih->ih_func)(ih->ih_arg ? ih->ih_arg : (void *)fr))
		return;

	if (ih->ih_type == IST_LEVEL) {
		panic("sh5_intr_dispatch: Unclaimed level-triggered interrupt");
		/*NOTREACHED*/
	}

	printf("sh5_intr_dispatch: Unclaimed %s-triggered interrupt...\n",
	    (ih->ih_type == IST_PULSE) ? "Pulse" : "Edge");
	printf("sh5_intr_dispatch: INTEVT=0x%x, level=%d\n",
	    (int) fr->if_state.sf_intevt, ih->ih_level);
}

/*
 * Interrupt handle allocator.
 */
static struct intrhand *
alloc_ih()
{
	/* #0 is reserved for unregistered interrupt. */
	struct intrhand *ih = &intrhand[1];
	int i;

	for (i = 1; i < szintrhand; i++, ih++)
		if (ih->ih_idx == 0) {	/* no driver use this. */
			ih->ih_idx = i;	/* register myself */
			return (ih);
		}

	panic("intc_alloc_ih: Out of interrupt handles!");
	return (NULL);
}

static void
free_ih(struct intrhand *ih)
{

	memset(ih, 0, sizeof(*ih));
}

static int
spurious_interrupt(void *arg)
{
	struct intrframe *fr = arg;

	printf("Spurious Interrupt: INTEVT=0x%x\n",
	    (int) fr->if_state.sf_intevt);
	panic("oops");
	/* NOTREACHED */
	return (0);
}
