/*	$NetBSD: ifpga_irqhandler.c,v 1.1.4.3 2002/02/11 20:07:42 jdolecek Exp $ */

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
 * Copyright (c) 1994 Brini.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: iomd_irqhandler.c,v 1.16 $
 */

#include "opt_irqstats.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <evbarm/ifpga/irqhandler.h>	/* XXX XXX XXX */
#include <machine/cpu.h>

irqhandler_t *irqhandlers[NIRQS];

int current_intr_depth;		/* Depth of interrupt nesting */
u_int intr_claimed_mask;	/* Interrupts that are claimed */
u_int intr_disabled_mask;	/* Interrupts that are temporarily disabled */
u_int intr_current_mask;	/* Interrupts currently allowable */
u_int spl_mask;
u_int irqmasks[NIPL];
u_int irqblock[NIRQS];

extern u_int soft_interrupts;	/* Only so we can initialise it */

extern char *_intrnames;
extern void set_spl_masks	__P((void));

void
irq_init(void)
{
	int loop;

	/* Clear all the IRQ handlers and the irq block masks */
	for (loop = 0; loop < NIRQS; ++loop) {
		irqhandlers[loop] = NULL;
		irqblock[loop] = 0;
	}

	/*
	 * Setup the irqmasks for the different Interrupt Priority Levels
	 * We will start with no bits set and these will be updated as handlers
	 * are installed at different IPL's.
	 */
	for (loop = 0; loop < NIPL; ++loop)
		irqmasks[loop] = 0;

	current_intr_depth = 0;
	intr_claimed_mask = 0x00000000;
	intr_disabled_mask = 0x00000000;
	intr_current_mask = 0x00000000;
	spl_mask = 0x00000000;
	soft_interrupts = 0x00000000;

	set_spl_masks();
}

/* Run the irq initialization code that can't be done until the IRQ
   controller is mapped.  */
void
irq_postinit()
{
	irq_setmasks();

	/* Enable IRQ's and FIQ's */
	enable_interrupts(I32_bit | F32_bit);
}

void
stray_irqhandler()
{
	panic("stray irq\n");
}

/*
 * void disable_irq(int irq)
 *
 * Disables a specific irq. The irq is removed from the master irq mask
 *
 * Use of this function outside this file is deprecated.
 */

void
disable_irq(irq)
	int irq;
{
	int oldirqstate; 

	oldirqstate = disable_interrupts(I32_bit);
	intr_claimed_mask &= ~(1 << irq);
	intr_current_mask = intr_claimed_mask & ~intr_disabled_mask;
	irq_setmasks();
	restore_interrupts(oldirqstate);
}  


/*
 * void enable_irq(int irq)
 *
 * Enables a specific irq. The irq is added to the master irq mask
 * This routine should be used with caution. A handler should already
 * be installed.
 *
 * Use of this function outside this file is deprecated.
 */

void
enable_irq(irq)
	int irq;
{
	u_int oldirqstate; 

	oldirqstate = disable_interrupts(I32_bit);
	intr_claimed_mask |= (1 << irq);
	intr_current_mask = intr_claimed_mask & ~intr_disabled_mask;
	irq_setmasks();
	restore_interrupts(oldirqstate);
}  

/*
 * int irq_claim(int irq, irqhandler_t *handler)
 *
 * Enable an IRQ and install a handler for it.
 */

int
irq_claim(irq, handler)
	int irq;
	irqhandler_t *handler;
{
	int level;
	int loop;

#ifdef DIAGNOSTIC
	/* Sanity check */
	if (handler == NULL)
		panic("NULL interrupt handler\n");
	if (handler->ih_func == NULL)
		panic("Interrupt handler does not have a function\n");
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
	 * Done here as the accounting number may not be the same as the IRQ number
	 * though for the moment they are
	 */
	handler->ih_num = irq;

#ifdef IRQSTATS
	/* Get the interrupt name from the head of the list */
	if (handler->ih_name) {
		char *ptr = _intrnames + (irq * 14);
		strcpy(ptr, "             ");
		strncpy(ptr, handler->ih_name,
		    min(strlen(handler->ih_name), 13));
	} else {
		char *ptr = _intrnames + (irq * 14);
		sprintf(ptr, "irq %2d     ", irq);
	}
#endif	/* IRQSTATS */

	/*
	 * Update the irq masks.
	 * If ih_level is out of range then don't bother to update
	 * the masks.
	 */
	if (handler->ih_level >= 0 && handler->ih_level < NIPL) {
		irqhandler_t *ptr;

		/*
		 * Find the lowest interrupt priority on the irq chain.
		 * Interrupt is allowable at priorities lower than this.
		 */
		ptr = irqhandlers[irq];
		if (ptr) {
			level = ptr->ih_level - 1;
			while (ptr) {
				if (ptr->ih_level - 1 < level)
					level = ptr->ih_level - 1;
				ptr = ptr->ih_next;
			}
			for (loop = 0; loop < NIPL; ++loop) {
				if (level >= loop)
					irqmasks[loop] |= (1 << irq);
				else
					irqmasks[loop] &= ~(1 << irq);
			}
		}

#include "sl.h"
#include "ppp.h"
#if NSL > 0 || NPPP > 0
		/* In the presence of SLIP or PPP, splimp > spltty. */
		irqmasks[IPL_NET] &= irqmasks[IPL_TTY];
#endif
	}

	/*
	 * We now need to update the irqblock array. This array indicates
	 * what other interrupts should be blocked when interrupt is asserted
	 * This basically emulates hardware interrupt priorities e.g. by blocking
	 * all other IPL_BIO interrupts with an IPL_BIO interrupt is asserted.
	 * For each interrupt we find the highest IPL and set the block mask to
	 * the interrupt mask for that level.
	 */
	for (loop = 0; loop < NIRQS; ++loop) {
		irqhandler_t *ptr;

		ptr = irqhandlers[loop];
		if (ptr) {
			/* There is at least 1 handler so scan the chain */
 			level = ptr->ih_level;
			while (ptr) {
				if (ptr->ih_level > level)
					level = ptr->ih_level;
				ptr = ptr->ih_next;
			}
			irqblock[loop] = ~irqmasks[level];
		} else
			/* No handlers for this irq so nothing to block */
			irqblock[loop] = 0;
	}

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
irq_release(irq, handler)
	int irq;
	irqhandler_t *handler;
{
	int level;
	int loop;
	irqhandler_t *irqhand;
	irqhandler_t **prehand;
#ifdef IRQSTATS
	extern char *_intrnames;
#endif	/* IRQSTATS */
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
	irqhand = irqhandlers[irq];
	prehand = &irqhandlers[irq];
    
	while (irqhand && handler != irqhand) {
		prehand = &irqhand->ih_next;
		irqhand = irqhand->ih_next;
	}

	/* Remove the handler if located */
	if (irqhand)
		*prehand = irqhand->ih_next;
	else
		return(-1);

	/* Now the handler has been removed from the chain mark is as inactive */
	irqhand->ih_flags &= ~IRQ_FLAG_ACTIVE;

	/* Make sure the head of the handler list is active */
	if (irqhandlers[irq])
		irqhandlers[irq]->ih_flags |= IRQ_FLAG_ACTIVE;

#ifdef IRQSTATS
	/* Get the interrupt name from the head of the list */
	if (irqhandlers[irq] && irqhandlers[irq]->ih_name) {
		char *ptr = _intrnames + (irq * 14);
		strcpy(ptr, "             ");
		strncpy(ptr, irqhandlers[irq]->ih_name,
		    min(strlen(irqhandlers[irq]->ih_name), 13));
	} else {
		char *ptr = _intrnames + (irq * 14);
		sprintf(ptr, "irq %2d     ", irq);
	}
#endif	/* IRQSTATS */

	/*
	 * Update the irq masks.
	 * If ih_level is out of range then don't bother to update
	 * the masks.
	 */
	if (handler->ih_level >= 0 && handler->ih_level < NIPL) {
		irqhandler_t *ptr;

		/*
		 * Find the lowest interrupt priority on the irq chain.
		 * Interrupt is allowable at priorities lower than this.
		 */
		ptr = irqhandlers[irq];
		if (ptr) {
			level = ptr->ih_level - 1;
			while (ptr) {
				if (ptr->ih_level - 1 < level)
					level = ptr->ih_level - 1;
				ptr = ptr->ih_next;
			}
			for (loop = 0; loop < NIPL; ++loop) {
				if (level >= loop)
					irqmasks[loop] |= (1 << irq);
				else
					irqmasks[loop] &= ~(1 << irq);
			}
		}
	}

	/*
	 * We now need to update the irqblock array. This array indicates
	 * what other interrupts should be blocked when interrupt is asserted
	 * This basically emulates hardware interrupt priorities e.g. by
	 * blocking all other IPL_BIO interrupts with an IPL_BIO interrupt
	 * is asserted. For each interrupt we find the highest IPL and set
	 * the block mask to the interrupt mask for that level.
	 */
	for (loop = 0; loop < NIRQS; ++loop) {
		irqhandler_t *ptr;

		ptr = irqhandlers[loop];
		if (ptr) {
			/* There is at least 1 handler so scan the chain */
			level = ptr->ih_level;
			while (ptr) {
				if (ptr->ih_level > level)
					level = ptr->ih_level;
				ptr = ptr->ih_next;
			}
			irqblock[loop] = ~irqmasks[level];
		} else
			/* No handlers for this irq so nothing to block */
			irqblock[loop] = 0;
	}

	/*
	 * Disable the appropriate mask bit if there are no handlers left for
	 * this IRQ.
	 */
	if (irqhandlers[irq] == NULL)
		disable_irq(irq);

	set_spl_masks();
      
	return(0);
}


void *
intr_claim(irq, level, name, ih_func, ih_arg)
	int irq;
	int level;
	const char *name;
	int (*ih_func) __P((void *));
	void *ih_arg;
{
	irqhandler_t *ih;

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (!ih)
		panic("intr_claim(): Cannot malloc handler memory\n");

	ih->ih_level = level;
	ih->ih_name = name;
	ih->ih_func = ih_func;
	ih->ih_arg = ih_arg;
	ih->ih_flags = 0;

	if (irq_claim(irq, ih) != 0)
		return(NULL);
	return(ih);
}


int
intr_release(arg)
	void *arg;
{
	irqhandler_t *ih = (irqhandler_t *)arg;

	if (irq_release(ih->ih_num, ih) == 0) {
		free(ih, M_DEVBUF);
		return(0);
	}
	return(1);
}
