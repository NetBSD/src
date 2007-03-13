/*	$NetBSD: interrupt.c,v 1.10.30.1 2007/03/13 16:50:05 ad Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.10.30.1 2007/03/13 16:50:05 ad Exp $");

#include <sys/param.h>
#include <sys/pool.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>	/* uvmexp.intrs */

#include <machine/intr.h>
#include <machine/cacheops.h>


/*
 * SH5 interrupt handlers are tracked using instances of the following
 * structure.
 */
struct intrhand {
	int	(*ih_func)(void *);
	void	*ih_arg;
	int	ih_type;
	int	ih_level;
	u_int	ih_intevt;
};

/*
 * The list of all possible interrupt handlers, indexed by INTEVT
 */
static struct intrhand *intrhand[256];

#define	INTEVT_TO_IH_INDEX(x)	((int)((x) >> 5) & 0xff)
#define	INTEVT_IH(x)		(intrhand[INTEVT_TO_IH_INDEX(x)])

/*
 * Pool of interrupt handles.
 * This is used by all native SH5 interrupt drivers. It ensures the
 * handle is allocated from KSEG0, which avoids taking multiple TLB
 * before the real interrupt handler is even called.
 */
static struct pool intrhand_pool;

/*
 * By default, each interrupt handle is 64-bytes in size. This allows
 * some room for future growth.
 */
#define	INTRHAND_SIZE		64


static void (*intr_enable)(void *, u_int, int, int);
static void (*intr_disable)(void *, u_int);
static void *intr_arg;

struct evcnt _sh5_intr_events[NIPL];
extern const char intrnames[];		/* Defined in port-specific code */

void sh5_intr_dispatch(struct intrframe *);

void
sh5_intr_init(void (*int_enable)(void *, u_int, int, int),
    void (*int_disable)(void *, u_int),
    void *arg)
{
	const char *iname;
	int i;

	pool_init(&intrhand_pool, INTRHAND_SIZE, SH5_CACHELINE_SIZE,
	    0, 0, NULL, NULL, IPL_NONE);

	intr_enable = int_enable;
	intr_disable = int_disable;
	intr_arg = arg;

	iname = intrnames;
	for (i = 0; i < NIPL; i++) {
		evcnt_attach_dynamic(&_sh5_intr_events[i], EVCNT_TYPE_INTR,
		    NULL, "intr", iname);
		iname = &iname[strlen(iname) + 1];
	}
}

void *
sh5_intr_establish(int intevt, int trigger, int level,
    int (*ih_func)(void *), void *ih_arg)
{
	struct intrhand *ih;
	int idx;

	KDASSERT(intr_enable != NULL);
	KDASSERT(level > 0 && level < NIPL);

	idx = INTEVT_TO_IH_INDEX(intevt);
	KDASSERT(idx < (sizeof(intrhand) / sizeof(struct intrhand *)));

	if (intrhand[idx] != NULL)
		return (NULL);		/* Perhaps panic? */

	if ((ih = sh5_intr_alloc_handle(sizeof(*ih))) == NULL)
		return (NULL);

	ih->ih_func = ih_func;
	ih->ih_arg = ih_arg;
	ih->ih_level = level;
	ih->ih_intevt = intevt;
	ih->ih_type = trigger;

	/* Map interrupt handler */
	intrhand[idx] = ih;

	(*intr_enable)(intr_arg, intevt, trigger, level);

	return ((void *)ih);
}

void
sh5_intr_disestablish(void *cookie)
{
	struct intrhand *ih = cookie;
	int idx;

	idx = INTEVT_TO_IH_INDEX(ih->ih_intevt);
	KDASSERT(idx < (sizeof(intrhand) / sizeof(struct intrhand *)));
	KDASSERT(intrhand[idx] == ih);

	(*intr_disable)(intr_arg, ih->ih_intevt);

	/* Unmap interrupt handler */
	intrhand[idx] = NULL;

	sh5_intr_free_handle(ih);
}

struct evcnt *
sh5_intr_evcnt(void *cookie)
{
	struct intrhand *ih = cookie;

	return (&_sh5_intr_events[ih->ih_level]);
}

void *
sh5_intr_alloc_handle(size_t size)
{

	if (size > INTRHAND_SIZE)
		panic("sh5_intr_alloc_handle: size > %d", INTRHAND_SIZE);

	return (pool_get(&intrhand_pool, 0));
}

void
sh5_intr_free_handle(void *handle)
{

	pool_put(&intrhand_pool, handle);
}

void
sh5_intr_dispatch(struct intrframe *fr)
{
	extern u_long intrcnt[];
	struct intrhand *ih;
	int idx;

	idx = INTEVT_TO_IH_INDEX(fr->if_state.sf_intevt);
	KDASSERT(idx < (sizeof(intrhand) / sizeof(struct intrhand *)));

	if ((ih = intrhand[idx]) == NULL) {
		int level;
		__asm volatile("getcon sr, %0" : "=r"(level));
		printf(
		    "sh5_intr_dispatch: spurious level %d irq: intevt 0x%lx\n",
		    (level >> SH5_CONREG_SR_IMASK_SHIFT) &
		    SH5_CONREG_SR_IMASK_MASK,
		    (unsigned long)fr->if_state.sf_intevt);
		return;
	}

	_sh5_intr_events[ih->ih_level].ev_count++;
	intrcnt[ih->ih_level]++;
	uvmexp.intrs++;

	if ((ih->ih_func)(ih->ih_arg ? ih->ih_arg : (void *)fr))
		return;

	if (ih->ih_type == IST_LEVEL) {
		panic("sh5_intr_dispatch: Unclaimed level-triggered interrupt");
		/*NOTREACHED*/
	}

#if 0
	/* We don't support Edge or Pulse triggered interrupts at this time */

	printf("sh5_intr_dispatch: Unclaimed %s-triggered interrupt...\n",
	    (ih->ih_type == IST_PULSE) ? "Pulse" : "Edge");
	printf("sh5_intr_dispatch: INTEVT=0x%lx, level=%d\n",
	    (unsigned long) fr->if_state.sf_intevt, ih->ih_level);
#endif
}
