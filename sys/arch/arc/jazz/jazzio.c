/*	$NetBSD: jazzio.c,v 1.9 2003/02/10 11:43:29 tsutsui Exp $	*/
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
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/autoconf.h>
#include <machine/platform.h>

#include <arc/jazz/jazziovar.h>
#include <arc/jazz/pica.h>
#include <arc/jazz/jazzdmatlbreg.h>
#include <arc/jazz/jazzdmatlbvar.h>
#include <arc/jazz/dma.h>
#include <arc/jazz/pckbc_jazzioreg.h>

void arc_sysreset __P((bus_addr_t, bus_size_t));

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

CFATTACH_DECL(jazzio, sizeof(struct jazzio_softc),
    jazziomatch, jazzioattach, NULL, NULL);
extern struct cfdriver jazzio_cd;

void	jazzio_intr_establish(int, int (*)(void *), void *);
void	jazzio_intr_disestablish(int);
int	jazzio_intr(unsigned, struct clockframe *);
int	jazzio_no_handler __P((void *));

/*
 *  Interrupt dispatch table for jazz i/o bus.
 */
struct jazzio_intr_registry {
	intr_handler_t	int_hand;	/* Interrupt handler */
	void		*param;		/* Parameter to send to handler */
};

struct jazzio_intr_registry jazzio_intrtab[] = {
	{ jazzio_no_handler, (void *)NULL, },  /*  0 */
	{ jazzio_no_handler, (void *)NULL, },  /*  1 */
	{ jazzio_no_handler, (void *)NULL, },  /*  2 */
	{ jazzio_no_handler, (void *)NULL, },  /*  3 */
	{ jazzio_no_handler, (void *)NULL, },  /*  4 */
	{ jazzio_no_handler, (void *)NULL, },  /*  5 */
	{ jazzio_no_handler, (void *)NULL, },  /*  6 */
	{ jazzio_no_handler, (void *)NULL, },  /*  7 */
	{ jazzio_no_handler, (void *)NULL, },  /*  8 */
	{ jazzio_no_handler, (void *)NULL, },  /*  9 */
	{ jazzio_no_handler, (void *)NULL, },  /* 10 */
	{ jazzio_no_handler, (void *)NULL, },  /* 11 */
	{ jazzio_no_handler, (void *)NULL, },  /* 12 */
	{ jazzio_no_handler, (void *)NULL, },  /* 13 */
	{ jazzio_no_handler, (void *)NULL, },  /* 14 */
	{ jazzio_no_handler, (void *)NULL, },  /* 15 */
};


struct jazzio_config *jazzio_conf = NULL;
struct pica_dev *jazzio_devconfig = NULL;
int jazzio_found = 0;
int jazzio_int_mask = 0;	/* jazz i/o interrupt enable mask */

struct arc_bus_space jazzio_bus;

int
jazziomatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct confargs *ca = aux;

        /* Make sure that we're looking for a jazzio bus. */
        if (strcmp(ca->ca_name, jazzio_cd.cd_name) != 0)
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

	if (jazzio_conf == NULL)
		panic("jazzio_conf isn't initialized");
	if (jazzio_devconfig == NULL)
		panic("jazzio_devconfig isn't initialized");

	jazzio_found = 1;

	printf("\n");

	/* keep our CPU device description handy */
	sc->sc_devs = jazzio_devconfig;

	/* set up interrupt handlers */
	(*platform->set_intr)(MIPS_INT_MASK_1, jazzio_intr, 2);

	sc->sc_bus.ab_dv = (struct device *)sc;

	/* Initialize jazzio dma mapping register area and pool */
	jazz_dmatlb_init(&jazzio_bus, jazzio_conf->jc_dmatlbreg);

	/* Create bus_dma_tag */
	jazz_bus_dma_tag_init(&sc->sc_dmat);

	/* Try to configure each jazzio attached device */
	for (i = 0; sc->sc_devs[i].ps_ca.ca_name != NULL; i++) {

		ja.ja_name = sc->sc_devs[i].ps_ca.ca_name;
		ja.ja_bus = &sc->sc_bus;
		ja.ja_bust = &jazzio_bus;
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
		aprint_normal("%s at %s", ja->ja_name, pnp);
	aprint_normal(" addr 0x%lx", ja->ja_addr);
	if (ja->ja_intr != -1)
		aprint_normal(" intr %d", ja->ja_intr);
	return (UNCONF);
}

void
jazzio_intr_establish(intr, handler, val)
	int intr;
	intr_handler_t handler;
	void *val;
{
	if (intr < 0 ||
	    intr >= sizeof(jazzio_intrtab)/sizeof(jazzio_intrtab[0])) {
		panic("jazzio intr %d out of range", intr);
	} else if (jazzio_intrtab[intr].int_hand != jazzio_no_handler) {
		panic("jazzio intr %d already set to %p", intr,
		    jazzio_intrtab[intr].int_hand);
	} else {
		jazzio_int_mask |= 1 << intr;
		jazzio_intrtab[intr].int_hand = handler;
		jazzio_intrtab[intr].param = val;
	}

	(*jazzio_conf->jc_set_iointr_mask)(jazzio_int_mask);
}

void
jazzio_intr_disestablish(intr)
	int intr;
{
	jazzio_int_mask &= ~(1 << intr);
	jazzio_intrtab[intr].int_hand = jazzio_no_handler;
	jazzio_intrtab[intr].param = (void *)NULL;

	(*jazzio_conf->jc_set_iointr_mask)(jazzio_int_mask);
}

int
jazzio_no_handler(arg)
	void *arg;
{
	panic("uncaught jazzio interrupt with arg %p", arg);
}

/*
 *   Handle jazz i/o interrupt.
 */
int
jazzio_intr(mask, cf)
	unsigned mask;
	struct clockframe *cf;
{
	unsigned int vector;
	struct jazzio_intr_registry *jirp;

	while ((vector = inb(jazzio_conf->jc_iointr_status_reg)) != 0) {
		jirp = &jazzio_intrtab[(vector >> 2) - 1];
		(*jirp->int_hand)(jirp->param);
	}
	return (~0);  /* Dont reenable */
}

void
jazzio_reset()
{
	arc_sysreset(PICA_SYS_KBD, JAZZIO_KBCMDP);
}
