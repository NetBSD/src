/* $NetBSD: irqhandler.c,v 1.14 1997/04/02 21:52:19 christos Exp $ */

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * irqhandler.c
 *
 * IRQ/FIQ initialisation, claim, release and handler routines
 *
 * Created      : 30/09/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <vm/vm.h>
#include <net/netisr.h>

#include <machine/irqhandler.h>
#include <machine/cpu.h>
#include <machine/iomd.h>
#include <machine/katelib.h>

#include "podulebus.h"

irqhandler_t *irqhandlers[NIRQS];
fiqhandler_t *fiqhandlers;

int current_intr_depth = 0;
u_int irqmasks[IRQ_LEVELS];
u_int current_mask;
u_int actual_mask;
u_int disabled_mask = 0;
u_int spl_mask;
u_int soft_interrupts;
extern u_int intrcnt[];

u_int irqblock[NIRQS];

typedef struct {
    vm_offset_t physical;
    vm_offset_t virtual;
} pv_addr_t;
             
extern pv_addr_t systempage;
extern char *_intrnames;

/* Prototypes */

int podule_irqhandler	__P((void));
extern void zero_page_readonly	__P((void));
extern void zero_page_readwrite	__P((void));
extern int fiq_setregs		__P((fiqhandler_t *));
extern int fiq_getregs		__P((fiqhandler_t *));
extern void set_spl_masks	__P((void));

extern void arpintr	__P((void));
extern void ipintr	__P((void));
extern void atintr	__P((void));
extern void pppintr	__P((void));
extern void plipintr	__P((void));

/*
 * void irq_init(void)
 *
 * Initialise the IRQ/FIQ sub system
 */

void
irq_init()
{
	int loop;

	/* Clear all the IRQ handlers and the irq block masks */

	for (loop = 0; loop < NIRQS; ++loop) {
		irqhandlers[loop] = NULL;
		irqblock[loop] = 0;
	}

	/* Clear the FIQ handler */

	fiqhandlers = NULL;

	/* Clear the IRQ/FIQ masks in the IOMD */

	WriteByte(IOMD_IRQMSKA, 0x00);
	WriteByte(IOMD_IRQMSKB, 0x00);

#ifdef CPU_ARM7500
	WriteByte(IOMD_IRQMSKC, 0x00);
	WriteByte(IOMD_IRQMSKD, 0x00);
#endif	/* CPU_ARM7500 */

	WriteByte(IOMD_FIQMSK, 0x00);
	WriteByte(IOMD_DMAMSK, 0x00);

	/*
	 * Setup the irqmasks for the different Interrupt Priority Levels
	 * We will start with no bits set and these will be updated as handlers
	 * are installed at different IPL's.
	 */

	irqmasks[IPL_BIO]   = 0x00000000;
	irqmasks[IPL_NET]   = 0x00000000;
	irqmasks[IPL_TTY]   = 0x00000000;
	irqmasks[IPL_CLOCK] = 0x00000000;
	irqmasks[IPL_IMP]   = 0x00000000;
	irqmasks[IPL_NONE]   = 0x00000000;

	current_mask = 0x00000000;
	actual_mask = 0x00000000;
	spl_mask = 0x00000000;
	soft_interrupts = 0x00000000;

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
	 * Find the lowest interrupt priority on the irq chain.
	 * Interrupt is allowable at priorities lower than this.
	 * If ih_level is out of range then don't bother to update
	 * the masks.
	 */

	if (handler->ih_level >= 0 && handler->ih_level < IRQ_LEVELS) {
		irqhandler_t *ptr;

		ptr = irqhandlers[irq];
		if (ptr) {
			level = ptr->ih_level - 1;
			while (ptr) {
				if (ptr->ih_level - 1 < level)
					level = ptr->ih_level - 1;
				ptr = ptr->ih_next;
			}
			while (level >= 0) {
				irqmasks[level] |= (1 << irq);
				--level;
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

#if NPODULEBUS > 0
	/*
	 * Is this an expansion card IRQ and is there a PODULE IRQ handler
	 * installed ?
	 * If not panic as the podulebus irq handler should have been installed
	 * when the podulebus was attached.
	 *
	 * The podule IRQ's need to be fixed ASAP
	 */

	if (irq >= IRQ_EXPCARD0 && irqhandlers[IRQ_PODULE] == NULL)
		panic("Podule IRQ %d claimed but no podulebus handler installed\n",
		    irq);
#endif	/* NPODULEBUS */

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
	extern char *_intrnames;

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
		prehand = &irqhand;
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
  
	if (handler->ih_level >= 0 && handler->ih_level < IRQ_LEVELS) {
		irqhandler_t *ptr;

	/* Clean the bit from all the masks */

		for (level = 0; level < IRQ_LEVELS; ++level)
			irqmasks[level] &= ~(1 << irq);

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
			while (level >= 0) {
				irqmasks[level] |= (1 << irq);
				--level;
			}
		}
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


void
intr_release(arg)
	void *arg;
{
	irqhandler_t *ih = (irqhandler_t *)arg;

	if (irq_release(ih->ih_num, ih) == 0)
		free(ih, M_DEVBUF);
}


u_int
disable_interrupts(mask)
	u_int mask;
{
	register u_int cpsr;

	cpsr = SetCPSR(mask, mask);
	return(cpsr);
}


u_int
restore_interrupts(old_cpsr)
	u_int old_cpsr;
{
	register int mask = I32_bit | F32_bit;
	return(SetCPSR(mask, old_cpsr & mask));
}


u_int
enable_interrupts(mask)
	u_int mask;
{
	return(SetCPSR(mask, 0));
}


/*
 * void disable_irq(int irq)
 *
 * Disables a specific irq. The irq is removed from the master irq mask
 */

void
disable_irq(irq)
	int irq;
{
	register int oldirqstate; 

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
enable_irq(irq)
	int irq;
{
	register u_int oldirqstate; 

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
stray_irqhandler(mask)
	u_int mask;
{
	static u_int stray_irqs = 0;

	if (++stray_irqs <= 8)
		log(LOG_ERR, "Stray interrupt %08x%s\n", mask,
		    stray_irqs >= 8 ? ": stopped logging" : "");
}


/* Handle software interrupts */

void
dosoftints()
{
	register u_int softints;
	int s;

	softints = soft_interrupts & spl_mask;
	if (softints == 0) return;

	if (current_intr_depth > 1)
		return;

	s = splsoft();

	/*
	 * Software clock interrupts
	 */

	if (softints & IRQMASK_SOFTCLOCK) {
		++cnt.v_soft;
		++intrcnt[IRQ_SOFTCLOCK];
		atomic_clear_bit(&soft_interrupts, IRQMASK_SOFTCLOCK);
		softclock();
	}

#if defined(INET) && defined(PLIP) && defined(notyet)
	if (softints & IRQMASK_SOFTPLIP) {
		++cnt.v_soft;
		++intrcnt[IRQ_SOFTPLIP];
		atomic_clear_bit(&soft_interrupts, IRQMASK_SOFTPLIP);
		plipintr();
	}
#endif

	/*
	 * Network software interrupts
	 */

	if (softints & IRQMASK_SOFTNET) {
		++cnt.v_soft;
		++intrcnt[IRQ_SOFTNET];
		atomic_clear_bit(&soft_interrupts, IRQMASK_SOFTNET);

#ifdef INET
#include "ether.h"
#if NETHER > 0
		if (netisr & (1 << NETISR_ARP)) {
			atomic_clear_bit(&netisr, (1 << NETISR_ARP));
			arpintr();
		}
#endif
		if (netisr & (1 << NETISR_IP)) {
			atomic_clear_bit(&netisr, (1 << NETISR_IP));
			ipintr();
		}
#endif
#ifdef NETATALK
		if (netisr & (1 << NETISR_ATALK)) {
			atomic_clear_bit(&netisr, (1 << NETISR_ATALK));
			atintr();
		}
#endif
#ifdef NS
		if (netisr & (1 << NETISR_NS)) {
			atomic_clear_bit(&netisr, (1 << NETISR_NS));
			nsintr();
		}
#endif
#ifdef IMP
		if (netisr & (1 << NETISR_IMP)) {
			atomic_clear_bit(&netisr, (1 << NETISR_IMP));
			impintr();
		}
#endif
#ifdef ISO
		if (netisr & (1 << NETISR_ISO)) {
			atomic_clear_bit(&netisr, (1 << NETISR_ISO));
			clnlintr();
		}
#endif
#ifdef CCITT
		if (netisr & (1 << NETISR_CCITT)) {
			atomic_clear_bit(&netisr, (1 << NETISR_CCITT));
			ccittintr();
		}
#endif
#include "ppp.h"
#if NPPP > 0
		if (netisr & (1 << NETISR_PPP)) {
			atomic_clear_bit(&netisr, (1 << NETISR_PPP));
			pppintr();
		}
#endif
	}
	(void)splx(s);
}


/*
 * int fiq_claim(fiqhandler_t *handler)
 *
 * Claim FIQ's and install a handler for them.
 */

int
fiq_claim(handler)
	fiqhandler_t *handler;
{
	/* Fail if the FIQ's are already claimed */

	if (fiqhandlers)
		return(-1);

	if (handler->fh_size > 0xc0)
		return(-1);

	/* Install the handler */

	fiqhandlers = handler;

	/* Now we have to actually install the FIQ handler */

	/* Eventually we will copy this down but for the moment ... */

	zero_page_readwrite();

	WriteWord(0x0000003c, (u_int) handler->fh_func);
    
	zero_page_readonly();
    
	/* We must now set up the FIQ registers */

	fiq_setregs(handler);

	/* Set up the FIQ mask */

	WriteWord(IOMD_FIQMSK, handler->fh_mask);
    
	/* Make sure that the FIQ's are enabled */
    
	enable_interrupts(F32_bit);
	return(0);
}


/*
 * int fiq_release(fiqhandler_t *handler)
 *
 * Release FIQ's and remove a handler for them.
 */

int
fiq_release(handler)
	fiqhandler_t *handler;
{
	/* Fail if the handler is wrong */

	if (fiqhandlers != handler)
		return(-1);

	/* Disable FIQ interrupts */
      
	disable_interrupts(F32_bit);

	/* Clear up the FIQ mask */

	WriteWord(IOMD_FIQMSK, 0x00);

	/* Retrieve the FIQ registers */

	fiq_getregs(handler);

	/* Remove the handler */

	fiqhandlers = NULL;
	return(0);
}

/* End of irqhandler.c */
