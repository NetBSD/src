/*	$NetBSD: sysasic.c,v 1.1 2001/04/24 19:43:23 marcus Exp $	*/

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
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <machine/shbvar.h>
#include <machine/sysasicvar.h>


static u_int32_t sysasicmasks[SYSASIC_IRQ_LEVEL_MAX+1][(SYSASIC_EVENT_MAX+1)/32];

void
sysasic_mask_irq(level)
	int level;
{
	volatile unsigned int *masks =
	  (volatile unsigned int *)(void *)(0xa05f6910+(level<<4));

	masks[0] = 0;
	masks[1] = 0;
}

void
sysasic_unmask_irq(level)
	int level;
{
	volatile unsigned int *masks =
	  (volatile unsigned int *)(void *)(0xa05f6910+(level<<4));

	masks[0] = sysasicmasks[level][0];
	masks[1] = sysasicmasks[level][1];
}

/*
 * Set up an interrupt handler to start being called.
 */
void *
sysasic_intr_establish(irq, event, level, ih_fun, ih_arg)
	int irq;
	int event;
	int level;
	int (*ih_fun) __P((void *));
	void *ih_arg;
{
	struct intrhand *ih;
	int mask_no, bit_set, bit_no;

	switch (irq) {
	case 9:
		mask_no = SYSASIC_IRQ_LEVEL_9;
		break;
	case 11:
		mask_no = SYSASIC_IRQ_LEVEL_11;
		break;
        case 13:
		mask_no = SYSASIC_IRQ_LEVEL_13;
		break;
	default:
		panic("invalid sysasic IRQ %d", irq);
	}

	if (event < 0 || event > SYSASIC_EVENT_MAX)
		panic("invalid sysasic event %d", event);

	bit_set = event >> 5;
	bit_no = event & 31;

	if (sysasicmasks[mask_no][bit_set] & (1<<bit_no))
		panic("multiple handlers for event %d", event);

	sysasicmasks[mask_no][bit_set] |= 1<<bit_no;

	ih = shb_intr_establish(irq, IST_LEVEL, level, ih_fun, ih_arg);

	/* XXX need to remember the event no so that we can disestablish
	   correctly.  This should probably be a field of its own in
	   struct intrhand, but by jamming it into the ih_irq field we
	   can avoid changing the generic sh3 struct... */

	ih->ih_irq |= (event << 8);

	return ih;
}


/*
 * Deregister an interrupt handler.
 */
void
sysasic_intr_disestablish(ic, arg)
	void *ic;
	void *arg;
{
	struct intrhand *ih = arg;
	int event, mask_no, bit_set, bit_no;

	/* XXX kluge, see sysasic_intr_establish() */
	event = ih->ih_irq >> 8;
	ih->ih_irq &= 0xff;

	switch (ih->ih_irq) {
	case 9:
		mask_no = SYSASIC_IRQ_LEVEL_9;
		break;
	case 11:
		mask_no = SYSASIC_IRQ_LEVEL_11;
		break;
        case 13:
		mask_no = SYSASIC_IRQ_LEVEL_13;
		break;
	default:
		panic("invalid sysasic IRQ %d", ih->ih_irq);
	}

	if (event < 0 || event > SYSASIC_EVENT_MAX)
		panic("invalid sysasic event %d", event);

	bit_set = event >> 5;
	bit_no = event & 31;

	sysasicmasks[mask_no][bit_set] &= ~(1<<bit_no);

	shb_intr_disestablish(ic, arg);
}
