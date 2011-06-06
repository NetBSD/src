/*	$NetBSD: jazzio.c,v 1.20.24.1 2011/06/06 09:05:00 jruoho Exp $	*/
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: jazzio.c,v 1.20.24.1 2011/06/06 09:05:00 jruoho Exp $");

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

#include "ioconf.h"

void arc_sysreset(bus_addr_t, bus_size_t);

struct jazzio_softc {
	device_t sc_dev;
	struct	arc_bus_dma_tag sc_dmat;
	struct	pica_dev *sc_devs;
};

/* Definition of the driver for autoconfig. */
static int	jazziomatch(device_t, cfdata_t, void *);
static void	jazzioattach(device_t, device_t, void *);
static int	jazzioprint(void *, const char *);

CFATTACH_DECL_NEW(jazzio, sizeof(struct jazzio_softc),
    jazziomatch, jazzioattach, NULL, NULL);

static uint32_t jazzio_intr(uint32_t, struct clockframe *);
static int	jazzio_no_handler(void *);

/*
 *  Interrupt dispatch table for jazz i/o bus.
 */
struct jazzio_intrhand {
	intr_handler_t	ih_func;	/* Interrupt handler */
	void		*ih_arg;	/* Parameter to send to handler */
	struct evcnt	ih_evcnt;	/* interrupt counter */
	char		ih_evname[32];	/* event counter name */
};

static struct jazzio_intrhand jazzio_intrtab[16];


struct jazzio_config *jazzio_conf = NULL;
struct pica_dev *jazzio_devconfig = NULL;
int jazzio_int_mask = 0;	/* jazz i/o interrupt enable mask */
struct arc_bus_space jazzio_bus;

static int jazzio_found = 0;

static int
jazziomatch(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

        /* Make sure that we're looking for a jazzio bus. */
        if (strcmp(ca->ca_name, jazzio_cd.cd_name) != 0)
                return 0;

	return 1;
}

static void
jazzioattach(device_t parent, device_t self, void *aux)
{
	struct jazzio_softc *sc = device_private(self);
	struct jazzio_attach_args ja;
	int i;

	sc->sc_dev = self;

	if (jazzio_conf == NULL)
		panic("jazzio_conf isn't initialized");
	if (jazzio_devconfig == NULL)
		panic("jazzio_devconfig isn't initialized");

	jazzio_found = 1;

	aprint_normal("\n");

	/* keep our CPU device description handy */
	sc->sc_devs = jazzio_devconfig;

	/* initialize interrupt handler table */
	for (i = 0; i < __arraycount(jazzio_intrtab); i++) {
		jazzio_intrtab[i].ih_func = jazzio_no_handler;
		jazzio_intrtab[i].ih_arg = NULL;
	}

	/* set up interrupt handlers */
	(*platform->set_intr)(MIPS_INT_MASK_1, jazzio_intr, ARC_INTPRI_JAZZ);

	/* Initialize jazzio DMA mapping register area and pool */
	jazz_dmatlb_init(&jazzio_bus, jazzio_conf->jc_dmatlbreg);

	/* Create bus_dma_tag */
	jazz_bus_dma_tag_init(&sc->sc_dmat);

	/* Try to configure each jazzio attached device */
	for (i = 0; sc->sc_devs[i].ps_ca.ca_name != NULL; i++) {

		ja.ja_name = sc->sc_devs[i].ps_ca.ca_name;
		ja.ja_bust = &jazzio_bus;
		ja.ja_dmat = &sc->sc_dmat;
		ja.ja_addr = (bus_addr_t)sc->sc_devs[i].ps_base;
		ja.ja_intr = sc->sc_devs[i].ps_ca.ca_slot;
		ja.ja_dma = 0;

		/* Tell the autoconfig machinery we've found the hardware. */
		config_found(self, &ja, jazzioprint);
	}
}

static int
jazzioprint(void *aux, const char *pnp)
{
	struct jazzio_attach_args *ja = aux;

	if (pnp)
		aprint_normal("%s at %s", ja->ja_name, pnp);
	aprint_normal(" addr 0x%lx", ja->ja_addr);
	if (ja->ja_intr != -1)
		aprint_normal(" intr %d", ja->ja_intr);
	return UNCONF;
}

void
jazzio_intr_establish(int intr, intr_handler_t handler, void *val)
{
	struct jazzio_intrhand *jirp;

	if (intr < 0 || intr >= __arraycount(jazzio_intrtab))
		panic("jazzio intr %d out of range", intr);
	jirp = &jazzio_intrtab[intr];
	if (jirp->ih_func != jazzio_no_handler)
		panic("jazzio intr %d already set to %p", intr, jirp->ih_func);

	jazzio_int_mask |= 1 << intr;
	jirp->ih_func = handler;
	jirp->ih_arg = val;
	snprintf(jirp->ih_evname, sizeof(jirp->ih_evname), "intr %d", intr);
	evcnt_attach_dynamic(&jirp->ih_evcnt, EVCNT_TYPE_INTR,
	    NULL, "jazzio", jirp->ih_evname);

	(*jazzio_conf->jc_set_iointr_mask)(jazzio_int_mask);
}

void
jazzio_intr_disestablish(int intr)
{
	struct jazzio_intrhand *jirp;

	jazzio_int_mask &= ~(1 << intr);
	jirp = &jazzio_intrtab[intr];
	jirp->ih_func = jazzio_no_handler;
	jirp->ih_arg = NULL;
	evcnt_detach(&jirp->ih_evcnt);

	(*jazzio_conf->jc_set_iointr_mask)(jazzio_int_mask);
}

static int
jazzio_no_handler(void *arg)
{

	panic("uncaught jazzio interrupt with arg %p", arg);
}

/*
 *   Handle jazz i/o interrupt.
 */
uint32_t
jazzio_intr(uint32_t mask, struct clockframe *cf)
{
	unsigned int vector;
	struct jazzio_intrhand *jirp;

	while ((vector = inb(jazzio_conf->jc_iointr_status_reg)) != 0) {
		jirp = &jazzio_intrtab[(vector >> 2) - 1];
		(*jirp->ih_func)(jirp->ih_arg);
		jirp->ih_evcnt.ev_count++;
	}
	return MIPS_INT_MASK_1;
}

void
jazzio_reset(void)
{

	arc_sysreset(PICA_SYS_KBD, JAZZIO_KBCMDP);
}
