/*	$NetBSD: ioasic.c,v 1.14 2000/07/11 04:10:25 nisimura Exp $	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Keith Bostic, Chris G. Demetriou, Jonathan Stone
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: ioasic.c,v 1.14 2000/07/11 04:10:25 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicreg.h>
#include <dev/tc/ioasicvar.h>

#include <machine/sysconf.h>

#include <pmax/pmax/pmaxtype.h>
#include <pmax/pmax/kmin.h>
#include <pmax/pmax/maxine.h>
#include <pmax/pmax/kn03.h>

#include "opt_dec_3min.h"
#include "opt_dec_maxine.h"
#include "opt_dec_3maxplus.h"

#define ARRAY_SIZEOF(x) (sizeof((x)) / sizeof((x)[0]))

#if defined(DEC_3MIN)
static struct ioasic_dev kmin_ioasic_devs[] = {
	{ "PMAD-BA ",	0x0C0000, C(SYS_DEV_LANCE),	},
	{ "scc",	0x100000, C(SYS_DEV_SCC0),	},
	{ "scc",	0x180000, C(SYS_DEV_SCC1),	},
	{ "mc146818",	0x200000, C(SYS_DEV_BOGUS),	},
	{ "asc",	0x300000, C(SYS_DEV_SCSI),	},
};
static int kmin_builtin_ndevs = ARRAY_SIZEOF(kmin_ioasic_devs);
static int kmin_ioasic_ndevs = ARRAY_SIZEOF(kmin_ioasic_devs);
#endif

#if defined(DEC_MAXINE)
static struct ioasic_dev xine_ioasic_devs[] = {
	{ "PMAD-BA ",	0x0C0000, C(SYS_DEV_LANCE),	},
	{ "scc",	0x100000, C(SYS_DEV_SCC0),	},
	{ "mc146818",	0x200000, C(SYS_DEV_BOGUS),	},
	{ "isdn",	0x240000, C(SYS_DEV_ISDN),	},
	{ "dtop",	0x280000, C(SYS_DEV_DTOP),	},
	{ "fdc",	0x2C0000, C(SYS_DEV_FDC),	},
	{ "asc",	0x300000, C(SYS_DEV_SCSI),	},
	{ "(TC0)",	0x0,	  C(SYS_DEV_OPT0),	},
	{ "(TC1)",	0x0,	  C(SYS_DEV_OPT1),	},
	{ "(TC2)",	0x0,	  C(SYS_DEV_OPT2),	},
};
static int xine_builtin_ndevs = ARRAY_SIZEOF(xine_ioasic_devs) - 3;
static int xine_ioasic_ndevs = ARRAY_SIZEOF(xine_ioasic_devs);
#endif

#if defined(DEC_3MAXPLUS)
static struct ioasic_dev kn03_ioasic_devs[] = {
	{ "PMAD-BA ",	0x0C0000, C(SYS_DEV_LANCE),	},
	{ "z8530   ",	0x100000, C(SYS_DEV_SCC0),	},
	{ "z8530   ",	0x180000, C(SYS_DEV_SCC1),	},
	{ "mc146818",	0x200000, C(SYS_DEV_BOGUS),	},
	{ "asc",	0x300000, C(SYS_DEV_SCSI),	},
	{ "(TC0)",	0x0,	  C(SYS_DEV_OPT0),	},
	{ "(TC1)",	0x0,	  C(SYS_DEV_OPT1),	},
	{ "(TC2)",	0x0,	  C(SYS_DEV_OPT2),	},
};
static int kn03_builtin_ndevs = ARRAY_SIZEOF(kn03_ioasic_devs) - 3;
static int kn03_ioasic_ndevs = ARRAY_SIZEOF(kn03_ioasic_devs);
#endif

static int	ioasicmatch __P((struct device *, struct cfdata *, void *));
static void	ioasicattach __P((struct device *, struct device *, void *));

const struct cfattach ioasic_ca = {
	sizeof(struct ioasic_softc), ioasicmatch, ioasicattach
};

tc_addr_t ioasic_base;	/* XXX XXX XXX */

/* There can be only one. */
int ioasicfound;

static int
ioasicmatch(parent, cfdata, aux)
	struct device *parent;
	struct cfdata *cfdata;
	void *aux;
{
	struct tc_attach_args *ta = aux;

	/* Make sure that we're looking for this type of device. */
	if (strncmp("IOCTL   ", ta->ta_modname, TC_ROM_LLEN))
		return (0);

	if (ioasicfound)
		return (0);

	return (1);
}

static void
ioasicattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ioasic_softc *sc = (struct ioasic_softc *)self;
	struct tc_attach_args *ta = aux;
	struct ioasic_dev *ioasic_devs;
	int ioasic_ndevs, builtin_ndevs;

	ioasicfound = 1;

	sc->sc_bst = ta->ta_memt;
	if (bus_space_map(ta->ta_memt, ta->ta_addr,
			0x400000, 0, &sc->sc_bsh)) {
		printf("%s: unable to map device\n", sc->sc_dv.dv_xname);
		return;
	}
	sc->sc_dmat = ta->ta_dmat;

	sc->sc_base = ta->ta_addr; /* XXX XXX XXX */

	printf("\n");

	switch (systype) {
#if defined(DEC_3MIN)
	case DS_3MIN:
		ioasic_devs = kmin_ioasic_devs;
		ioasic_ndevs = kmin_ioasic_ndevs;
		builtin_ndevs = kmin_builtin_ndevs;
		break;
#endif
#if defined(DEC_MAXINE)
	case DS_MAXINE:
		ioasic_devs = xine_ioasic_devs;
		ioasic_ndevs = xine_ioasic_ndevs;
		builtin_ndevs = xine_builtin_ndevs;
		break;
#endif
#if defined(DEC_3MAXPLUS)
	case DS_3MAXPLUS:
		ioasic_devs = kn03_ioasic_devs;
		ioasic_ndevs = kn03_ioasic_ndevs;
		builtin_ndevs = kn03_builtin_ndevs;
		break;
#endif
	default:
		panic("ioasicmatch: how did we get here?");
	}

#if 0 /* IMSK has been sanitized */
	/*
	 * Turn off all device interrupt bits.
	 * (This _does_ include TC option slot bits.)
	 */
	imsk = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK);
	for (i = 0; i < ioasic_ndevs; i++)
		imsk &= ~ioasic_devs[i].iad_intrbits;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK, imsk);
#endif

	/*
	 * Try to configure each device.
	 */
	ioasic_attach_devs(sc, ioasic_devs, builtin_ndevs);
}

const struct evcnt *
ioasic_intr_evcnt(dev, cookie)
	struct device *dev;
	void *cookie;
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void
ioasic_intr_establish(dev, cookie, level, handler, val)
	struct device *dev;
	void *cookie;
	int level;
	int (*handler) __P((void *));
	void *val;
{
	(*platform.intr_establish)(dev, cookie, level, handler, val);
}
