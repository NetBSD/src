/*	$NetBSD: isa_irqhandler.c,v 1.27 2014/09/21 15:48:29 christos Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * IRQ/FIQ initialisation, claim, release and handler routines
 *
 *	from: irqhandler.c
 *
 * Created      : 30/09/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isa_irqhandler.c,v 1.27 2014/09/21 15:48:29 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <sys/intr.h>

#include <arm/locore.h>

#include <machine/irqhandler.h>

irqhandler_t *irqhandlers[NIRQS];

u_int current_mask;
u_int actual_mask;
u_int disabled_mask;
u_int irqmasks[NIPL];

/* Prototypes */

extern void set_spl_masks(void);
void irq_calculatemasks(void);
void stray_irqhandler(u_int);

#define WriteWord(a, b) *((volatile unsigned int *)(a)) = (b)

/*
 * void irq_init(void)
 *
 * Initialise the IRQ/FIQ sub system
 */

void
irq_init(void)
{
	int loop;

	/* Clear all the IRQ handlers and the irq block masks */
	for (loop = 0; loop < NIRQS; ++loop) {
		irqhandlers[loop] = NULL;
	}

	/*
	 * Setup the irqmasks for the different Interrupt Priority Levels
	 * We will start with no bits set and these will be updated as handlers
	 * are installed at different IPL's.
	 */
	for (loop = 0; loop < NIPL; ++loop)
		irqmasks[loop] = 0;

	current_mask = 0x00000000;
	disabled_mask = 0x00000000;
	actual_mask = 0x00000000;

	set_spl_masks();

	/* Enable IRQ's and FIQ's */
	enable_interrupts(I32_bit | F32_bit); 
}


/*
 * int irq_claim(int irq, irqhandler_t *handler)
 *
 * Enable an IRQ and install a handler for it.
 */

int
irq_claim(int irq, irqhandler_t *handler, const char *group, const char *name)
{

#ifdef DIAGNOSTIC
	/* Sanity check */
	if (handler == NULL)
		panic("NULL interrupt handler");
	if (handler->ih_func == NULL)
		panic("Interrupt handler does not have a function");
#endif	/* DIAGNOSTIC */

	/*
	 * IRQ_INSTRUCT indicates that we should get the irq number
	 * from the irq structure
	 */
	if (irq == IRQ_INSTRUCT)
		irq = handler->ih_num;
    
	/* Make sure the irq number is valid */
	if (irq < 0 || irq >= NIRQS)
		return(-1);

	/* Make sure the level is valid */
	if (handler->ih_level < 0 || handler->ih_level >= NIPL)
    	        return(-1);

	/* Attach evcnt */
	evcnt_attach_dynamic(&handler->ih_ev, EVCNT_TYPE_INTR, NULL,
	    group, name);

	/* Attach handler at top of chain */
	handler->ih_next = irqhandlers[irq];
	irqhandlers[irq] = handler;

	/*
	 * Reset the flags for this handler.
	 * As the handler is now in the chain mark it as active.
	 */
	handler->ih_flags = 0 | IRQ_FLAG_ACTIVE;

	/*
	 * Record the interrupt number for accounting.
	 * Done here as the accounting number may not be the same as the
	 * IRQ number though for the moment they are
	 */
	handler->ih_num = irq;

	irq_calculatemasks();

	enable_irq(irq);
	set_spl_masks();
	return(0);
}


/*
 * int irq_release(int irq, irqhandler_t *handler)
 *
 * Disable an IRQ and remove a handler for it.
 */

int
irq_release(int irq, irqhandler_t *handler)
{
	irqhandler_t *irqhand;
	irqhandler_t **prehand;

	/*
	 * IRQ_INSTRUCT indicates that we should get the irq number
	 * from the irq structure
	 */
	if (irq == IRQ_INSTRUCT)
		irq = handler->ih_num;

	/* Make sure the irq number is valid */
	if (irq < 0 || irq >= NIRQS)
		return(-1);

	/* Locate the handler */
	prehand = &irqhandlers[irq];
	irqhand = *prehand;
    
	while (irqhand && handler != irqhand) {
		prehand = &irqhand->ih_next;
		irqhand = *prehand;
	}

	/* Remove the handler if located */
	if (irqhand)
		*prehand = irqhand->ih_next;
	else
		return(-1);

	/* The handler has been removed from the chain so mark it as inactive */
	irqhand->ih_flags &= ~IRQ_FLAG_ACTIVE;

	/* Make sure the head of the handler list is active */
	if (irqhandlers[irq])
		irqhandlers[irq]->ih_flags |= IRQ_FLAG_ACTIVE;

	evcnt_detach(&irqhand->ih_ev);

	irq_calculatemasks();

	/*
	 * Disable the appropriate mask bit if there are no handlers left for
	 * this IRQ.
	 */
	if (irqhandlers[irq] == NULL)
		disable_irq(irq);

	set_spl_masks();
      
	return(0);
}

/* adapted from .../i386/isa/isa_machdep.c */
/*
 * Recalculate the interrupt masks from scratch.
 * We could code special registry and deregistry versions of this function that
 * would be faster, but the code would be nastier, and we don't expect this to
 * happen very much anyway.
 */
void
irq_calculatemasks(void)
{
	int          irq, level;
	irqhandler_t *ptr;
	int          irqlevel[NIRQS];

	/* First, figure out which levels each IRQ uses. */
	for (irq = 0; irq < NIRQS; irq++) {
		int levels = 0;
		for (ptr = irqhandlers[irq]; ptr; ptr = ptr->ih_next)
			levels |= 1 << ptr->ih_level;
		irqlevel[irq] = levels;
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		int irqs = 0;
		for (irq = 0; irq < NIRQS; irq++)
			if (irqlevel[irq] & (1 << level))
				irqs |= 1 << irq;
		irqmasks[level] = ~irqs;
	}

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	KASSERT(irqmasks[IPL_NONE] == ~0);
	irqmasks[IPL_SOFTCLOCK] &= irqmasks[IPL_NONE];
	irqmasks[IPL_SOFTBIO] &= irqmasks[IPL_SOFTCLOCK];
	irqmasks[IPL_SOFTNET] &= irqmasks[IPL_SOFTBIO];
	irqmasks[IPL_SOFTSERIAL] &= irqmasks[IPL_SOFTNET];
	irqmasks[IPL_VM] &= irqmasks[IPL_SOFTSERIAL];
	irqmasks[IPL_CLOCK] &= irqmasks[IPL_VM];
	irqmasks[IPL_HIGH] &= irqmasks[IPL_CLOCK];
}


void *
intr_claim(int irq, int level, int (*ih_func)(void *), void *ih_arg, const char *group, const char *name)
{
	irqhandler_t *ih;

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT | M_ZERO);
	if (!ih)
		panic("intr_claim(): Cannot malloc handler memory");

	ih->ih_level = level;
	ih->ih_func = ih_func;
	ih->ih_arg = ih_arg;
	ih->ih_flags = 0;

	if (irq_claim(irq, ih, group, name) != 0) {
		free(ih, M_DEVBUF);
		return(NULL);
	}

	return(ih);
}

int
intr_release(void *arg)
{
	irqhandler_t *ih = (irqhandler_t *)arg;

	if (irq_release(ih->ih_num, ih) == 0) {
		free(ih, M_DEVBUF);
		return(0);
	}
	return(1);
}


/*
 * void disable_irq(int irq)
 *
 * Disables a specific irq. The irq is removed from the master irq mask
 */

void
disable_irq(int irq)
{
	u_int oldirqstate; 

	oldirqstate = disable_interrupts(I32_bit);
	current_mask &= ~(1 << irq);
	irq_setmasks();
	restore_interrupts(oldirqstate);
}  


/*
 * void enable_irq(int irq)
 *
 * Enables a specific irq. The irq is added to the master irq mask
 * This routine should be used with caution. A handler should already
 * be installed.
 */

void
enable_irq(int irq)
{
	u_int oldirqstate; 

	oldirqstate = disable_interrupts(I32_bit);
	current_mask |= (1 << irq);
	irq_setmasks();
	restore_interrupts(oldirqstate);
}  


/*
 * void stray_irqhandler(u_int mask)
 *
 * Handler for stray interrupts. This gets called if a handler cannot be
 * found for an interrupt.
 */

void
stray_irqhandler(u_int mask)
{
	static u_int stray_irqs = 0;

	if (++stray_irqs <= 8)
		log(LOG_ERR, "Stray interrupt %08x%s\n", mask,
		    stray_irqs >= 8 ? ": stopped logging" : "");
}
