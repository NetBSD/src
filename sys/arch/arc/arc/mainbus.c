/*	$NetBSD: mainbus.c,v 1.11.2.1 2000/06/22 16:59:07 minoura Exp $	*/
/*	$OpenBSD: mainbus.c,v 1.4 1998/10/15 21:30:15 imp Exp $	*/
/*	NetBSD: mainbus.c,v 1.3 1995/06/28 02:45:10 cgd Exp 	*/

/*
 * Copyright (c) 1997 Per Fogelstrom.
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
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

#include <arc/arc/arctype.h>
#include <machine/autoconf.h>

struct mainbus_softc {
	struct	device sc_dv;
	struct	abus sc_bus;
};

/* Definition of the mainbus driver. */
static int	mbmatch __P((struct device *, struct cfdata *, void *));
static void	mbattach __P((struct device *, struct device *, void *));
static int	mbprint __P((void *, const char *));

struct cfattach mainbus_ca = {
	sizeof(struct mainbus_softc), mbmatch, mbattach
};

void	mb_intr_establish __P((struct confargs *, int (*)(void *), void *));
void	mb_intr_disestablish __P((struct confargs *));
caddr_t	mb_cvtaddr __P((struct confargs *));
int	mb_matchname __P((struct confargs *, char *));

static int mainbus_found;

static int
mbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	if (mainbus_found)
		return (0);

	return (1);
}

static void
mbattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_softc *sc = (struct mainbus_softc *)self;
	struct confargs nca;

	mainbus_found = 1;

	printf("\n");

	sc->sc_bus.ab_dv = (struct device *)sc;
	sc->sc_bus.ab_type = BUS_MAIN;
	sc->sc_bus.ab_intr_establish = mb_intr_establish;
	sc->sc_bus.ab_intr_disestablish = mb_intr_disestablish;
	sc->sc_bus.ab_cvtaddr = mb_cvtaddr;
	sc->sc_bus.ab_matchname = mb_matchname;

	/*
	 * Try to find and attach all of the CPUs in the machine.
	 * ( Right now only one CPU so code is simple )
	 */

	nca.ca_name = "cpu";
	nca.ca_slot = 0;
	nca.ca_offset = 0;
	nca.ca_bus = &sc->sc_bus;
	config_found(self, &nca, mbprint);

	switch (cputype) {
	case ACER_PICA_61:
	case MAGNUM:
	case NEC_R94:
	case NEC_RAx94:
	case NEC_RD94:
	case NEC_R96:
		nca.ca_name = "pica";
		nca.ca_slot = 0;
		nca.ca_offset = 0;
		nca.ca_bus = &sc->sc_bus;
		config_found(self, &nca, mbprint);
		break;

	case ALGOR_P4032:
	case ALGOR_P5064:
		nca.ca_name = "algor";
		nca.ca_slot = 0;
		nca.ca_offset = 0;
		nca.ca_bus = &sc->sc_bus;
		config_found(self, &nca, mbprint);
		break;
	}

	/* The following machines have a PCI bus */
	switch (cputype) {
	case NEC_RAx94:
	case NEC_RD94:
		nca.ca_name = "necpb";
		nca.ca_slot = 0;
		nca.ca_offset = 0;
		nca.ca_bus = &sc->sc_bus;
		config_found(self, &nca, mbprint);
		break;

	case ALGOR_P4032:
	case ALGOR_P5064:
		nca.ca_name = "pbcpcibr";
		nca.ca_slot = 0;
		nca.ca_offset = 0;
		nca.ca_bus = &sc->sc_bus;
		config_found(self, &nca, mbprint);
		break;
	}

	/* The following machines have an ISA bus */
	switch (cputype) {
	case ACER_PICA_61:
	case MAGNUM:
	case NEC_R94:
	case NEC_R96:
	case DESKSTATION_TYNE:
	case DESKSTATION_RPC44:
		nca.ca_name = "isabr";
		nca.ca_slot = 0;
		nca.ca_offset = 0;
		nca.ca_bus = &sc->sc_bus;
		config_found(self, &nca, mbprint);
		break;
	}
}

static int
mbprint(aux, pnp)
	void *aux;
	const char *pnp;
{

	if (pnp)
		return (QUIET);
	return (UNCONF);
}

void
mb_intr_establish(ca, handler, val)
	struct confargs *ca;
	int (*handler) __P((void *));
	void *val;
{

	panic("can never mb_intr_establish");
}

void
mb_intr_disestablish(ca)
	struct confargs *ca;
{

	panic("can never mb_intr_disestablish");
}

caddr_t
mb_cvtaddr(ca)
	struct confargs *ca;
{

	return (NULL);
}

int
mb_matchname(ca, name)
	struct confargs *ca;
	char *name;
{

	return (strcmp(name, ca->ca_name) == 0);
}
