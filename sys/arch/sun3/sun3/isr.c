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
 *
 *	$Id: isr.c,v 1.11 1994/09/20 16:52:27 gwr Exp $
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

/*
 * Justification:
 *
 * the reason the interrupts are not initialized earlier is because of
 * concerns that if it was done earlier then this might screw up the monitor.
 * this may just be lame.
 * 
 */

volatile u_char *interrupt_reg;

extern void level0intr(), level1intr(), level2intr(), level3intr(),
    level4intr(), level5intr(), level6intr(), level7intr();

struct level_intr {
    void (*level_intr)();
    int used;
} level_intr_array[NISR] = {
    { level0intr, 0 },		/* this is really the spurious interrupt */
    { level1intr, 0 },
    { level2intr, 0 }, 
    { level3intr, 0 },
    { level4intr, 0 }, 
    { level5intr, 0 },
    { level6intr, 0 },
    { level7intr, 0 }
};

     
struct isr *isr_array[NISR];

void isr_init()
{
	int i;

	for (i = 0; i < NISR; i++)
		isr_array[i] = NULL;

	interrupt_reg = obio_find_mapping(OBIO_INTERREG, 1);
	if (!interrupt_reg)
		mon_panic("interrupt reg VA not found\n");
}

void isr_add_custom(level, handler)
     int level;
     void (*handler)();
{
    level_intr_array[level].used++;
    set_vector_entry(VEC_INTERRUPT_BASE + level, handler);
}


void isr_activate(level)
     int level;
{
    level_intr_array[level].used++;
    set_vector_entry(VEC_INTERRUPT_BASE + level,
		     level_intr_array[level].level_intr);
}

void isr_cleanup()
{
    int i;

    for (i = 0; i <NISR; i++) {
	if (level_intr_array[i].used) continue;
	isr_activate(i);
    }
}

void isr_add(level, handler, arg)
     int level;
     int (*handler)();
     int arg;
{
    struct isr *new_isr;
    int first_isr;

    first_isr = 0;
    if ((level < 0) || (level >= NISR))
	panic("isr_add: attempt to add handler for bad level");
    new_isr = (struct isr *)
	malloc(sizeof(struct isr), M_DEVBUF, M_NOWAIT);
    if (!new_isr)
	panic("isr_add: allocation of new 'isr' failed");
    new_isr->isr_arg = arg;
    new_isr->isr_ipl = level;
    new_isr->isr_intr = handler;
    new_isr->isr_back = NULL;
    new_isr->isr_forw = isr_array[level];
    if (!isr_array[level]) first_isr++;
    isr_array[level] = new_isr;
    if (first_isr) 
	isr_activate(level);
}


void isr_soft_request(level)
     int level;
{
    u_char bit, reg_val;
    int s;

    if ((level < 1) || (level > 3))
	panic("isr_soft_request");
    s = splhigh();
    reg_val = *interrupt_reg;
    *interrupt_reg &= ~IREG_ALL_ENAB;
    switch(level) {
    case 1:
	bit = IREG_SOFT_ENAB_1;
	break;
    case 2:
	bit = IREG_SOFT_ENAB_2;
	break;
    case 3:
	bit = IREG_SOFT_ENAB_3;
	break;
    }
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
    s = splhigh();
    reg_val = *interrupt_reg;
    *interrupt_reg &= ~IREG_ALL_ENAB;
    switch(level) {
    case 1:
	bit = IREG_SOFT_ENAB_1;
	break;
    case 2:
	bit = IREG_SOFT_ENAB_2;
	break;
    case 3:
	bit = IREG_SOFT_ENAB_3;
	break;
    }
    *interrupt_reg &= ~bit;
    *interrupt_reg |= IREG_ALL_ENAB;
    splx(s);
}

extern int intrcnt[];
void intrhand(ipl)
	int ipl;
{
    struct isr *isr;
    int found;

	intrcnt[ipl]++;
	cnt.v_intr++;

    isr = isr_array[ipl];
    if (!isr) {
	printf("intrhand: unexpected ipl %d\n", ipl);
	return;
    }
    if (!ipl) {
	printf("intrhand: spurious interrupt\n");
	return;
    }
    found = 0;
    for (; isr != NULL; isr = isr->isr_forw)
	if (isr->isr_intr(isr->isr_arg)) {
	    found++;
	    break;
	}
    if (!found)
	printf("intrhand: stray interrupt, ipl %d\n", ipl);
}

/*
 * XXX - This really belongs in some common file, like
 * src/sys/net/netisr.c  -gwr
 */
netintr()
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
int
soft1intr(fp)
	void *fp;
{
	isr_soft_clear(1);
	if (sun3sir.sir_any) {
		cnt.v_soft++;
		if (sun3sir.sir_which[SIR_NET]) {
			sun3sir.sir_which[SIR_NET] = 0;
			netintr();
		}
		if (sun3sir.sir_which[SIR_CLOCK]) {
			sun3sir.sir_which[SIR_CLOCK] = 0;
			softclock();
		}
		if (sun3sir.sir_which[SIR_SPARE2]) {
			sun3sir.sir_which[SIR_SPARE2] = 0;
			/* spare2intr(); */
		}
		if (sun3sir.sir_which[SIR_SPARE3]) {
			sun3sir.sir_which[SIR_SPARE3] = 0;
			/* spare3intr(); */
			/* XXX - For testing (db> w sun3sir 1) */
			sun3_rom_abort();
		}
	}
	return (1);
}
