/*
 * Copyright (c) 1995 Per Fogelstrom
 * Copyright (c) 1993, 1994 Charles M. Hannum.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>  
#include <sys/device.h> 
#include <sys/malloc.h>
#include <vm/vm.h>  

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/intr.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#define IO_ICU1		0x020
#define IO_ICU2		0x0a0

#define	IRQ_SLAVE	2
#define ICU_LEN		16

u_int8_t isa_inb(int);
void isa_outb(int, u_int8_t);

__inline u_int8_t isa_inb(int x)
{
	return (*((volatile unsigned char *) (0xb0000000 | (x))));
}

__inline void isa_outb(int x, u_int8_t y)
{
	*((volatile unsigned char *) (0xb0000000 | x)) = y;
	DELAY(100);
}

typedef int isa_chipset_tag_t;

void *	intr_establish(isa_chipset_tag_t, int, int, int, int (*)(void *),
				void *);
void	intr_disestablish(isa_chipset_tag_t, void*);
int	iointr(unsigned int, struct clockframe *);
void	initicu(void);
void	intr_calculatemasks(void);
int	fakeintr(void *a);
char *	isa_intr_typename(int);

#define LEGAL_IRQ(x)    ((x) >= 0 && (x) < ICU_LEN && (x) != 2)

struct intrhand {
        struct  intrhand *ih_next;
        int     (*ih_fun) __P((void *));
        void    *ih_arg;
        u_long  ih_count;
        int     ih_level;
        int     ih_irq;
        char    *ih_what;
};

int	imen;
int	intrtype[ICU_LEN], intrmask[ICU_LEN], intrlevel[ICU_LEN];
struct	intrhand *intrhand[ICU_LEN];

int fakeintr(a)
	void *a;
{
	return 0;
}

/*
 * Recalculate the interrupt masks from scratch.
 * We could code special registry and deregistry versions of this function that
 * would be faster, but the code would be nastier, and we don't expect this to
 * happen very much anyway.
 */
void
intr_calculatemasks()
{
	int irq, level;
	struct intrhand *q;

	/* First, figure out which levels each IRQ uses. */
	for (irq = 0; irq < ICU_LEN; irq++) {
		register int levels = 0;
		for (q = intrhand[irq]; q; q = q->ih_next)
			levels |= 1 << q->ih_level;
		intrlevel[irq] = levels;
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < 5; level++) {
		register int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++)
			if (intrlevel[irq] & (1 << level))
				irqs |= 1 << irq;
		imask[level] = irqs | SIR_ALLMASK;
	}

	/*
	 * There are tty, network and disk drivers that use free() at interrupt
	 * time, so imp > (tty | net | bio).
	 */
	imask[IPL_IMP] |= imask[IPL_TTY] | imask[IPL_NET] | imask[IPL_BIO];

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	imask[IPL_TTY] |= imask[IPL_NET] | imask[IPL_BIO];
	imask[IPL_NET] |= imask[IPL_BIO];

	/*
	 * These are pseudo-levels.
	 */
	imask[IPL_NONE] = 0x00000000;
	imask[IPL_HIGH] = 0xffffffff;

	/* And eventually calculate the complete masks. */
	for (irq = 0; irq < ICU_LEN; irq++) {
		register int irqs = 1 << irq;
		for (q = intrhand[irq]; q; q = q->ih_next)
			irqs |= imask[q->ih_level];
		intrmask[irq] = irqs | SIR_ALLMASK;
	}

	/* Lastly, determine which IRQs are actually in use. */
	{
		register int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++)
			if (intrhand[irq])
				irqs |= 1 << irq;
		if (irqs >= 0x100) /* any IRQs >= 8 in use */
			irqs |= 1 << IRQ_SLAVE;
		imen = ~irqs;
		isa_outb(IO_ICU1 + 1, imen);
		isa_outb(IO_ICU2 + 1, imen >> 8);
	}
}

/*
 *	Establish a ISA bus interrupt.
 */
void *   
intr_establish(ic, irq, type, level, ih_fun, ih_arg)
        isa_chipset_tag_t ic;
        int irq;
        int type;
        int level;
        int (*ih_fun) __P((void *));
        void *ih_arg;
{
	struct intrhand **p, *q, *ih;
	static struct intrhand fakehand = {NULL, fakeintr};
	extern int cold;

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("pci_intr_establish: can't malloc handler info");

	if (!LEGAL_IRQ(irq) || type == IST_NONE)
		panic("intr_establish: bogus irq or type");

	switch (intrtype[irq]) {
	case IST_EDGE:
	case IST_LEVEL:
		if (type == intrtype[irq])
			break;
	case IST_PULSE:
		if (type != IST_NONE)
			panic("intr_establish: can't share %s with %s",
			    isa_intr_typename(intrtype[irq]),
			    isa_intr_typename(type));
		break;
	}

	/*
	 * Figure out where to put the handler.
	 * This is O(N^2), but we want to preserve the order, and N is
	 * generally small.
	 */
	for (p = &intrhand[irq]; (q = *p) != NULL; p = &q->ih_next)
		;

	/*
	 * Actually install a fake handler momentarily, since we might be doing
	 * this with interrupts enabled and don't want the real routine called
	 * until masking is set up.
	 */
	fakehand.ih_level = level;
	*p = &fakehand;

	intr_calculatemasks();

	/*
	 * Poke the real handler in now.
	 */
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_count = 0;
	ih->ih_next = NULL;
	ih->ih_level = level;
	ih->ih_irq = irq;
	ih->ih_what = ""; /* XXX - should be eliminated */
	*p = ih;

	return (ih);
}

void                    
intr_disestablish(ic, arg)
        isa_chipset_tag_t ic;
        void *arg;      
{               
	return;
}

/*
 *	Process an interrupt from the ISA bus.
 */
int
iointr(mask, cf)
        unsigned mask;
        struct clockframe *cf;
{
	struct intrhand *ih;
	int isa_vector;
	int o_imen;
	char vector;

	(void) &isa_vector;	/* shut off gcc unused-variable warnings */

#if 1
		isa_outb(IO_ICU1, 0x0f);	/* Poll */
		vector = isa_inb(IO_ICU1);
		if(vector > 0 || (isa_vector = vector & 7) == 2) { 
			isa_outb(IO_ICU2, 0x0f);
			vector = isa_inb(IO_ICU2);
			if(vector > 0)
				return(~0);
			isa_vector = (vector & 7) | 8;
		}
#endif

	o_imen = imen;
	imen |= 1 << (isa_vector & (ICU_LEN - 1));
	if(isa_vector & 0x08) {
		isa_inb(IO_ICU2 + 1);
		isa_outb(IO_ICU2 + 1, imen >> 8);
		isa_outb(IO_ICU2, 0x60 + (isa_vector & 7));
		isa_outb(IO_ICU1, 0x60 + IRQ_SLAVE);
	}
	else {
		isa_inb(IO_ICU1 + 1);
		isa_outb(IO_ICU1 + 1, imen);
		isa_outb(IO_ICU1, 0x60 + isa_vector);
	}
	ih = intrhand[isa_vector];
	while(ih) {
		(*ih->ih_fun)(ih->ih_arg);
		ih = ih->ih_next;
	}
	imen = o_imen;
	isa_inb(IO_ICU1 + 1);
	isa_inb(IO_ICU2 + 1);
	isa_outb(IO_ICU1 + 1, imen);
	isa_outb(IO_ICU2 + 1, imen >> 8);

	return(~0);  /* Dont reenable */
}

/* 
 * Initialize the interrupt controller logic.
 */
void
initicu()
{  

	isa_outb(IO_ICU1, 0x11);	/* reset; program device, four bytes */
	isa_outb(IO_ICU1+1, 0);		/* starting at this vector index */
	isa_outb(IO_ICU1+1, 1 << IRQ_SLAVE); /* slave on line 2 */
	isa_outb(IO_ICU1+1, 1);		/* 8086 mode */
	isa_outb(IO_ICU1+1, 0xff);	/* leave interrupts masked */
	isa_outb(IO_ICU1, 0x68);	/* special mask mode (if available) */
	isa_outb(IO_ICU1, 0x0a);	/* Read IRR by default. */

	isa_outb(IO_ICU2, 0x11);	/* reset; program device, four bytes */
	isa_outb(IO_ICU2+1, 8);		/* staring at this vector index */
	isa_outb(IO_ICU2+1, IRQ_SLAVE);
	isa_outb(IO_ICU2+1, 1);		/* 8086 mode */
	isa_outb(IO_ICU2+1, 0xff);	/* leave interrupts masked */
	isa_outb(IO_ICU2, 0x68);	/* special mask mode (if available) */
	isa_outb(IO_ICU2, 0x0a);	/* Read IRR by default. */

	/*
	 * Initialize the VT82C586.
	 * XXX
	 */

	isa_outb(0x20, 0x10);
	isa_outb(0x21, 0x00);
	isa_outb(0x21, 0x00);

	isa_outb(0xa0, 0x10);
	isa_outb(0xa1, 0x00);
	isa_outb(0xa1, 0x00);
}	       

char *
isa_intr_typename(type)
	int type;
{

	switch (type) {
        case IST_NONE :
		return ("none");
        case IST_PULSE:
		return ("pulsed");
        case IST_EDGE:
		return ("edge-triggered");
        case IST_LEVEL:
		return ("level-triggered");
	default:
		panic("isa_intr_typename: invalid type %d", type);
	}
}
