/*	$NetBSD: isr.c,v 1.17 1995/04/07 04:43:25 gwr Exp $	*/

/*
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
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
 *	This product includes software developed by Adam Glass.
 * 4. The name of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This handles multiple attach of autovectored interrupts,
 * and the handy software interrupt request register.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/vmmeter.h>

#include <net/netisr.h>

#include <machine/cpu.h>
#include <machine/mon.h>
#include <machine/obio.h>
#include <machine/isr.h>

#include "vector.h"
#include "interreg.h"

extern int intrcnt[];	/* statistics */

#define NISR 8

struct isr {
	struct	isr *isr_next;
	int	(*isr_intr)();
	void *isr_arg;
	int	isr_ipl;
};

static struct isr *isr_autovec_list[NISR];

volatile u_char *interrupt_reg;

void isr_init()
{
	interrupt_reg = obio_find_mapping(OBIO_INTERREG, 1);
	if (!interrupt_reg)
		mon_panic("interrupt reg VA not found\n");
	/* Turn off all interrupts until clock_attach */
	*interrupt_reg = 0;
}

void isr_add_custom(level, handler)
	int level;
	void (*handler)();
{
	set_vector_entry(AUTO_VECTOR_BASE + level, handler);
}

static int isr_soft_pending;
void isr_soft_request(level)
	int level;
{
	u_char bit, reg_val;
	int s;

	if ((level < 1) || (level > 3))
		panic("isr_soft_request");

	bit = 1 << level;

	/* XXX - Should do this in the callers... */
	if (isr_soft_pending & bit)
		return;

	s = splhigh();
	isr_soft_pending |= bit;
	reg_val = *interrupt_reg;
	*interrupt_reg &= ~IREG_ALL_ENAB;

	*interrupt_reg |= bit;
	*interrupt_reg |= IREG_ALL_ENAB;
	splx(s);
}

void isr_soft_clear(level)
	int level;
{
	u_char bit, reg_val;
	int s;

	if ((level < 1) || (level > 3))
		panic("isr_soft_clear");

	bit = 1 << level;

	s = splhigh();
	isr_soft_pending &= ~bit;
	reg_val = *interrupt_reg;
	*interrupt_reg &= ~IREG_ALL_ENAB;

	*interrupt_reg &= ~bit;
	*interrupt_reg |= IREG_ALL_ENAB;
	splx(s);
}

/*
 * XXX - This really belongs in some common file, like
 * src/sys/net/netisr.c  -gwr
 */
void netintr()
{
	int n, s;

	s = splhigh();
	n = netisr;
	netisr = 0;
	splx(s);

	/*
	 * Theo says:
	 * XXX	this is bogus: should just have a list of
	 *	routines to call, a la timeouts.  Mods to
	 *	netisr are not atomic and must be protected (gah).
	 * Gordon says:
	 * How about an array of characters (1 per protocol)?
	 */
#ifdef INET
	if (n & (1 << NETISR_ARP))
		arpintr();
	if (n & (1 << NETISR_IP))
		ipintr();
#endif
#ifdef NS
	if (n & (1 << NETISR_NS))
		nsintr();
#endif
#ifdef ISO
	if (n & (1 << NETISR_ISO))
		clnlintr();
#endif
#ifdef CCITT
	if (n & (1 << NETISR_CCITT)) {
		ccittintr();
	}
#endif
}


/*
 * Level 1 software interrupt.
 * Possible reasons:
 *	Network software interrupt
 *	Soft clock interrupt
 */
int soft1intr(fp)
	void *fp;
{
	union sun3sir sir;
	int n, s;

	s = splhigh();
	sir.sir_any = sun3sir.sir_any;
	sun3sir.sir_any = 0;
	isr_soft_clear(1);
	splx(s);

	if (sir.sir_any) {
		cnt.v_soft++;
		if (sir.sir_which[SIR_NET]) {
			sir.sir_which[SIR_NET] = 0;
			netintr();
		}
		if (sir.sir_which[SIR_CLOCK]) {
			sir.sir_which[SIR_CLOCK] = 0;
			softclock();
		}
		if (sir.sir_which[SIR_SPARE2]) {
			sir.sir_which[SIR_SPARE2] = 0;
			/* spare2intr(); */
		}
		if (sir.sir_which[SIR_SPARE3]) {
			sir.sir_which[SIR_SPARE3] = 0;
			/* spare3intr(); */
		}
		return (1);
	}
	return(0);
}


/*
 * This is called by the assembly routines
 * for handling auto-vectored interupts.
 */
void isr_autovec(ipl)
	int ipl;
{
	struct isr *isr;
	register int n;

	n = intrcnt[ipl];
	intrcnt[ipl] = n+1;
	cnt.v_intr++;

	isr = isr_autovec_list[ipl];
	if (isr == NULL) {
		if (n == 0)
			printf("isr_autovec: ipl %d unexpected\n", ipl);
		return;
	}

	/* Give all the handlers a chance. */
	n = 0;
	while (isr) {
		n |= isr->isr_intr(isr->isr_arg);
		isr = isr->isr_next;
	}
	if (!n)
		printf("isr_autovec: ipl %d not claimed\n", ipl);
}

/*
 * Establish an interrupt handler.
 * Called by driver attach functions.
 */
void isr_add_autovect(handler, arg, level)
	int (*handler)();
	void *arg;
	int level;
{
	struct isr *new_isr;

	if ((level < 0) || (level >= NISR))
		panic("isr_add: bad level=%d", level);
	new_isr = (struct isr *)
		malloc(sizeof(struct isr), M_DEVBUF, M_NOWAIT);
	if (!new_isr)
		panic("isr_add: malloc failed");

	new_isr->isr_intr = handler;
	new_isr->isr_arg = arg;
	new_isr->isr_ipl = level;
	new_isr->isr_next = isr_autovec_list[level];
	isr_autovec_list[level] = new_isr;
}

extern void badtrap();
struct vector_handler {
	int (*func)();
	void *arg;
};
static struct vector_handler isr_vector_handlers[192];

/*
 * This is called by the assembly glue
 * for handling vectored interupts.
 */
void
isr_vectored(evec)
	u_short evec;
{
	int ipl, vec = evec >> 4;
	struct vector_handler *vh;

	ipl = getsr();
	ipl = (ipl >> 8) & 7;

	intrcnt[ipl]++;
	cnt.v_intr++;

	if (vec < 64 || vec >= 256) {
		printf("isr_vectored: vector=0x%x (invalid)\n", vec);
		return;
	}
	vh = &isr_vector_handlers[vec - 64];
	if (vh->func == NULL) {
		printf("isr_vectored: vector=0x%x (nul func)\n", vec);
		set_vector_entry(vec, badtrap);
		return;
	}

	/* OK, call the isr function. */
	if (vh->func(vh->arg) == 0)
		printf("isr_vectored: vector=0x%x (not claimed)\n", vec);
}

/*
 * Establish an interrupt handler.
 * Called by driver attach functions.
 */
extern void vect_intr();
void isr_add_vectored(func, arg, level, vec)
	int (*func)();
	void *arg;
	int level, vec;
{
	struct vector_handler *vh;

	if (vec < 64 || vec >= 256) {
		printf("isr_add_vectored: vect=0x%x (invalid)\n", vec);
		return;
	}
	vh = &isr_vector_handlers[vec - 64];
	if (vh->func) {
		printf("isr_add_vectored: vect=0x%x (in use)\n", vec);
		return;
	}
	vh->func = func;
	vh->arg = arg;
	set_vector_entry(vec, vect_intr);
}
