/*	$NetBSD: jazzio.c,v 1.4 2001/04/30 04:52:53 tsutsui Exp $	*/
/*	$OpenBSD: picabus.c,v 1.11 1999/01/11 05:11:10 millert Exp $	*/
/*	NetBSD: tc.c,v 1.2 1995/03/08 00:39:05 cgd Exp 	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * Author: Per Fogelstrom. (Mips R4x00)
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/cpu.h>
#include <machine/pio.h>
#include <machine/autoconf.h>

#include <arc/jazz/jazziovar.h>
#include <arc/jazz/pica.h>
#include <arc/jazz/rd94.h>
#include <arc/arc/arctype.h>
#include <arc/jazz/jazzdmatlbreg.h>
#include <arc/jazz/dma.h>

struct jazzio_softc {
	struct	device sc_dv;
	struct	abus sc_bus;
	struct	arc_bus_dma_tag sc_dmat;
	struct	pica_dev *sc_devs;
};

/* Definition of the driver for autoconfig. */
int	jazziomatch(struct device *, struct cfdata *, void *);
void	jazzioattach(struct device *, struct device *, void *);
int	jazzioprint(void *, const char *);

struct cfattach jazzio_ca = {
	sizeof(struct jazzio_softc), jazziomatch, jazzioattach
};
extern struct cfdriver jazzio_cd;

void	jazzio_intr_establish(int, int (*)(void *), void *);
void	jazzio_intr_disestablish(int);
int	pica_iointr(unsigned int, struct clockframe *);
int	pica_clkintr(unsigned int, struct clockframe *);
int	rd94_iointr(unsigned int, struct clockframe *);
int	rd94_clkintr(unsigned int, struct clockframe *);

intr_handler_t pica_clock_handler;

/*
 *  Interrupt dispatch table.
 */
struct pica_int_desc int_table[] = {
	{0, pica_intrnull, (void *)NULL, 0 },  /*  0 */
	{0, pica_intrnull, (void *)NULL, 0 },  /*  1 */
	{0, pica_intrnull, (void *)NULL, 0 },  /*  2 */
	{0, pica_intrnull, (void *)NULL, 0 },  /*  3 */
	{0, pica_intrnull, (void *)NULL, 0 },  /*  4 */
	{0, pica_intrnull, (void *)NULL, 0 },  /*  5 */
	{0, pica_intrnull, (void *)NULL, 0 },  /*  6 */
	{0, pica_intrnull, (void *)NULL, 0 },  /*  7 */
	{0, pica_intrnull, (void *)NULL, 0 },  /*  8 */
	{0, pica_intrnull, (void *)NULL, 0 },  /*  9 */
	{0, pica_intrnull, (void *)NULL, 0 },  /* 10 */
	{0, pica_intrnull, (void *)NULL, 0 },  /* 11 */
	{0, pica_intrnull, (void *)NULL, 0 },  /* 12 */
	{0, pica_intrnull, (void *)NULL, 0 },  /* 13 */
	{0, pica_intrnull, (void *)NULL, 0 },  /* 14 */
	{0, pica_intrnull, (void *)NULL, 0 },  /* 15 */
};

struct pica_dev {
	struct confargs	ps_ca;
	u_int		ps_mask;
	intr_handler_t	ps_handler;
	caddr_t		ps_base;
};

struct pica_dev acer_pica_61_cpu[] = {
	{{ "dallas_rtc",0, 0, },
	   0,			 pica_intrnull, (void *)PICA_SYS_CLOCK, },
	{{ "lpt",	1, 0, },
	   PICA_SYS_LB_IE_PAR1,	 pica_intrnull, (void *)PICA_SYS_PAR1, },
	{{ "fdc",	2, 0, },
	   PICA_SYS_LB_IE_FLOPPY,pica_intrnull, (void *)PICA_SYS_FLOPPY, },
	{{ NULL,	3, NULL, },
	   0, pica_intrnull, (void *)NULL, },
	{{ "vga",	4, NULL, },
	   0, pica_intrnull, (void *)PICA_V_LOCAL_VIDEO, },
	{{ "sonic",	5, 0, },
	   PICA_SYS_LB_IE_SONIC, pica_intrnull, (void *)PICA_SYS_SONIC, },
	{{ "asc",	6, 0, },
	   PICA_SYS_LB_IE_SCSI,  pica_intrnull, (void *)PICA_SYS_SCSI, },
	{{ "pckbd",	7, 0, },
	   PICA_SYS_LB_IE_KBD,	 pica_intrnull, (void *)PICA_SYS_KBD, },
	{{ "pms",	8, NULL, },
	   PICA_SYS_LB_IE_MOUSE, pica_intrnull, (void *)PICA_SYS_KBD, },
	{{ "com",	9, 0, },
	   PICA_SYS_LB_IE_COM1,	 pica_intrnull, (void *)PICA_SYS_COM1, },
	{{ "com",      10, 0, },
	   PICA_SYS_LB_IE_COM2,	 pica_intrnull, (void *)PICA_SYS_COM2, },
	{{ NULL,       -1, NULL, },
	   0, NULL, (void *)NULL, },
};

struct pica_dev mips_magnum_r4000_cpu[] = {
	{{ "dallas_rtc",0, 0, },
	   0,			 pica_intrnull, (void *)PICA_SYS_CLOCK, },
	{{ "lpt",	1, 0, },
	   PICA_SYS_LB_IE_PAR1,	 pica_intrnull, (void *)PICA_SYS_PAR1, },
	{{ "fdc",	2, 0, },
	   PICA_SYS_LB_IE_FLOPPY,pica_intrnull, (void *)PICA_SYS_FLOPPY, },
	{{ NULL,	3, NULL, },
	   0, pica_intrnull, (void *)NULL, },
	{{ "vxl",       4, 0, },
	   PICA_SYS_LB_IE_VIDEO, pica_intrnull, (void *)PICA_V_LOCAL_VIDEO, },
	{{ "sonic",	5, 0, },
	   PICA_SYS_LB_IE_SONIC, pica_intrnull, (void *)PICA_SYS_SONIC, },
	{{ "asc",	6, 0, },
	   PICA_SYS_LB_IE_SCSI,  pica_intrnull, (void *)PICA_SYS_SCSI, },
	{{ "pckbd",	7, 0, },
	   PICA_SYS_LB_IE_KBD,	 pica_intrnull, (void *)PICA_SYS_KBD, },
	{{ "pms",	8, NULL, },
	   PICA_SYS_LB_IE_MOUSE, pica_intrnull, (void *)PICA_SYS_KBD, },
	{{ "com",	9, 0, },
	   PICA_SYS_LB_IE_COM1,	 pica_intrnull, (void *)PICA_SYS_COM1, },
	{{ "com",      10, 0, },
	   PICA_SYS_LB_IE_COM2,	 pica_intrnull, (void *)PICA_SYS_COM2, },
	{{ NULL,       -1, NULL, },
	   0, NULL, (void *)NULL, },
};

struct pica_dev nec_rd94_cpu[] = {
	{{ "dallas_rtc",0, 0, },
	   0,			 pica_intrnull,	(void *)RD94_SYS_CLOCK, },
	{{ "lpt",	1, 0, },
	   RD94_SYS_LB_IE_PAR1,  pica_intrnull,	(void *)RD94_SYS_PAR1, },
	{{ "fdc",	2, 0, },
	   RD94_SYS_LB_IE_FLOPPY,pica_intrnull,	(void *)RD94_SYS_FLOPPY, },
	{{ NULL,	3, NULL, },
	   0, pica_intrnull, (void *)NULL, },
	{{ "sonic",	4, 0, },
	   RD94_SYS_LB_IE_SONIC, pica_intrnull,	(void *)RD94_SYS_SONIC, },
	{{ NULL,	5, NULL, },
	   0, pica_intrnull, (void *)NULL, },
	{{ NULL,	6, NULL, },
	   0, pica_intrnull, (void *)NULL, },
	{{ "pckbd",	7, 0, },
	   RD94_SYS_LB_IE_KBD,	 pica_intrnull,	(void *)RD94_SYS_KBD, },
	{{ "pms",	8, NULL, },
	   RD94_SYS_LB_IE_MOUSE, pica_intrnull,	(void *)RD94_SYS_KBD, },
	{{ "com",	9, 0, },
	   RD94_SYS_LB_IE_COM1,	 pica_intrnull,	(void *)RD94_SYS_COM1, },
	{{ "com",      10, 0, },
	   RD94_SYS_LB_IE_COM2,	 pica_intrnull,	(void *)RD94_SYS_COM2, },
	{{ NULL,       -1, NULL, },
	   0, NULL, (void *)NULL, },
};

struct pica_dev nec_jc94_cpu[] = {
	{{ "dallas_rtc",0, 0, },
	   0,			 pica_intrnull,	(void *)RD94_SYS_CLOCK, },
	{{ "lpt",	1, 0, },
	   RD94_SYS_LB_IE_PAR1,  pica_intrnull,	(void *)RD94_SYS_PAR1, },
	{{ "fdc",	2, 0, },
	   RD94_SYS_LB_IE_FLOPPY,pica_intrnull,	(void *)RD94_SYS_FLOPPY, },
	{{ NULL,	3, NULL, },
	   0, pica_intrnull, (void *)NULL, },
	{{ "sonic",	4, 0, },
	   RD94_SYS_LB_IE_SONIC, pica_intrnull,	(void *)RD94_SYS_SONIC, },
	{{ "osiop",	5, 0, },
	   RD94_SYS_LB_IE_SCSI0, pica_intrnull, (void *)RD94_SYS_SCSI0, },
	{{ "osiop",	6, 0, },
	   RD94_SYS_LB_IE_SCSI1, pica_intrnull, (void *)RD94_SYS_SCSI1, },
	{{ "pckbd",	7, 0, },
	   RD94_SYS_LB_IE_KBD,	 pica_intrnull,	(void *)RD94_SYS_KBD, },
	{{ "pms",	8, NULL, },
	   RD94_SYS_LB_IE_MOUSE, pica_intrnull,	(void *)RD94_SYS_KBD, },
	{{ "com",	9, 0, },
	   RD94_SYS_LB_IE_COM1,	 pica_intrnull,	(void *)RD94_SYS_COM1, },
	{{ "com",      10, 0, },
	   RD94_SYS_LB_IE_COM2,	 pica_intrnull,	(void *)RD94_SYS_COM2, },
	{{ NULL,       -1, NULL, },
	   0, NULL, (void *)NULL, },
};

struct pica_dev *pica_cpu_devs[] = {
        NULL,                   /* Unused */
        acer_pica_61_cpu,       /* Acer PICA */
	mips_magnum_r4000_cpu,	/* Mips MAGNUM R4000 */
	nec_rd94_cpu,		/* NEC-R94 */
	nec_rd94_cpu,		/* NEC-RA'94 */
	nec_rd94_cpu,		/* NEC-RD94 */
	nec_rd94_cpu,		/* NEC-R96 */
	NULL,
	NULL,
	NULL,
	NULL,
	nec_jc94_cpu,		/* NEC-JC94 */
};
int npica_cpu_devs = sizeof pica_cpu_devs / sizeof pica_cpu_devs[0];

int jazzio_found = 0;
int local_int_mask = 0;	/* Local interrupt enable mask */

extern struct arc_bus_space	pica_bus;

int
jazziomatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct confargs *ca = aux;

        /* Make sure that we're looking for a PICA. */
        if (strcmp(ca->ca_name, jazzio_cd.cd_name) != 0)
                return (0);

        /* Make sure that unit exists. */
	if (jazzio_found ||
	    cputype > npica_cpu_devs || pica_cpu_devs[cputype] == NULL)
		return (0);

	return (1);
}

void
jazzioattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct jazzio_softc *sc = (struct jazzio_softc *)self;
	struct jazzio_attach_args ja;
	int i;

	printf("\n");

	jazzio_found = 1;

	/* keep our CPU device description handy */
	sc->sc_devs = pica_cpu_devs[cputype];

	/* set up interrupt handlers */
	switch (cputype) {
	case ACER_PICA_61:
	case MAGNUM:
		set_intr(MIPS_INT_MASK_1, pica_iointr, 2);
		break;
	case NEC_R94:
	case NEC_RAx94:
	case NEC_RD94:
	case NEC_R96:
	case NEC_JC94:
		set_intr(MIPS_INT_MASK_1, rd94_iointr, 2);
		break;
	}

	sc->sc_bus.ab_dv = (struct device *)sc;

	/* Initialize PICA Dma */
	picaDmaInit();

	/* Create bus_dma_tag */
	jazz_bus_dma_tag_init(&sc->sc_dmat);

	/* Try to configure each PICA attached device */
	for (i = 0; sc->sc_devs[i].ps_ca.ca_slot >= 0; i++) {

		if (sc->sc_devs[i].ps_ca.ca_name == NULL)
			continue; /* Empty slot */

		ja.ja_name = sc->sc_devs[i].ps_ca.ca_name;
		ja.ja_bus = &sc->sc_bus;
		ja.ja_bust = &pica_bus;
		ja.ja_dmat = &sc->sc_dmat;
		ja.ja_addr = (bus_addr_t)sc->sc_devs[i].ps_base;
		ja.ja_intr = sc->sc_devs[i].ps_ca.ca_slot;
		ja.ja_dma = 0;

		/* Tell the autoconfig machinery we've found the hardware. */
		config_found(self, &ja, jazzioprint);
	}
}

int
jazzioprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct jazzio_attach_args *ja = aux;

        if (pnp)
                printf("%s at %s", ja->ja_name, pnp);
        printf(" addr 0x%lx intr %d", ja->ja_addr, ja->ja_intr);
        return (UNCONF);
}

void
jazzio_intr_establish(slot, handler, val)
	int slot;
	intr_handler_t handler;
	void *val;
{
	struct jazzio_softc *sc = jazzio_cd.cd_devs[0];

	if (slot == 0) {	/* Slot 0 is special, clock */
		pica_clock_handler = handler;
		switch (cputype) {
		case ACER_PICA_61:
		case MAGNUM:
			set_intr(MIPS_INT_MASK_4, pica_clkintr, 1);
			break;
		case NEC_R94:
		case NEC_RAx94:
		case NEC_RD94:
		case NEC_R96:
		case NEC_JC94:
			set_intr(MIPS_INT_MASK_3, rd94_clkintr, 1);
			break;
		}
	}

	if (int_table[slot].int_mask != 0) {
		panic("pica intr already set");
	} else {
		int_table[slot].int_mask = sc->sc_devs[slot].ps_mask;
		local_int_mask |= int_table[slot].int_mask;
		int_table[slot].int_hand = handler;
		int_table[slot].param = val;
	}

	switch (cputype) {
	case ACER_PICA_61:
	case MAGNUM:
		out16(PICA_SYS_LB_IE, local_int_mask);
		break;

	case NEC_R94:
	case NEC_R96:
		out16(RD94_SYS_LB_IE2, local_int_mask);
		break;

	case NEC_RAx94:
	case NEC_RD94:
	case NEC_JC94:
		/* XXX: I don't know why, but firmware does. */
		if (in32(0xe0000560) != 0)
			out16(RD94_SYS_LB_IE2, local_int_mask);
		else
			out16(RD94_SYS_LB_IE1, local_int_mask);
		break;
	}
}

void
jazzio_intr_disestablish(slot)
	int slot;
{
	if (slot != 0)		 {	/* Slot 0 is special, clock */
		local_int_mask &= ~int_table[slot].int_mask;
		int_table[slot].int_mask = 0;
		int_table[slot].int_hand = pica_intrnull;
		int_table[slot].param = (void *)NULL;
	}
}

int
pica_intrnull(val)
	void *val;
{
	panic("uncaught PICA intr for slot %p", val);
}

/*
 *   Handle pica i/o interrupt.
 */
int
pica_iointr(mask, cf)
	unsigned mask;
	struct clockframe *cf;
{
	int vector;

	while ((vector = inb(PVIS) >> 2) != 0) {
		(*int_table[vector].int_hand)(int_table[vector].param);
	}
	return (~0);  /* Dont reenable */
}

/*
 * Handle pica interval clock interrupt.
 */
int
pica_clkintr(mask, cf)
	unsigned mask;
	struct clockframe *cf;
{
	int temp;

	temp = inw(R4030_SYS_IT_STAT);
	(*pica_clock_handler)(cf);

	/* Re-enable clock interrupts */
	splx(MIPS_INT_MASK_4 | MIPS_SR_INT_IE);

	return (~MIPS_INT_MASK_4); /* Keep clock interrupts enabled */
}

/*
 *   Handle NEC-RD94 i/o interrupt.
 */
int
rd94_iointr(mask, cf)
	unsigned mask;
	struct clockframe *cf;
{
	int vector;

	while ((vector = inb(RD94_SYS_INTSTAT1) >> 2) != 0) {
		(*int_table[vector].int_hand)(int_table[vector].param);
	}
	return (~0);  /* Dont reenable */
}

/*
 * Handle NEC-RD94 interval clock interrupt.
 */
int
rd94_clkintr(mask, cf)
	unsigned mask;
	struct clockframe *cf;
{
	int temp;

	temp = in32(RD94_SYS_INTSTAT3);
	(*pica_clock_handler)(cf);

	/* Re-enable clock interrupts */
	splx(MIPS_INT_MASK_3 | MIPS_SR_INT_IE);

	return (~MIPS_INT_MASK_3); /* Keep clock interrupts enabled */
}
