/*	$NetBSD: ioasic.c,v 1.7 2000/01/10 03:24:41 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ioasic.c,v 1.7 2000/01/10 03:24:41 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>

#include <pmax/pmax/pmaxtype.h>
#include <pmax/pmax/asic.h>
#include <pmax/pmax/kmin.h>
#include <pmax/pmax/maxine.h>
#include <pmax/pmax/kn03.h>
#include <pmax/pmax/turbochannel.h>	/* interrupt enable declaration */

#include "opt_dec_3min.h"
#include "opt_dec_maxine.h"
#include "opt_dec_3maxplus.h"

#define	C(x)		((void *)(x))
#define ARRAY_SIZEOF(x) (sizeof((x)) / sizeof((x)[0]))

#if defined(DEC_3MIN)
static struct ioasic_dev kmin_ioasic_devs[] = {
	{ "lance",	0x0C0000, C(KMIN_LANCE_SLOT),	IOASIC_INTR_LANCE, },
	{ "scc",	0x100000, C(KMIN_SCC0_SLOT),	IOASIC_INTR_SCC_0, },
	{ "scc",	0x180000, C(KMIN_SCC1_SLOT),	IOASIC_INTR_SCC_1, },
	{ "mc146818",	0x200000, C(-1),		0 },
	{ "asc",	0x300000, C(KMIN_SCSI_SLOT),	IOASIC_INTR_SCSI, },
};
static int kmin_builtin_ndevs = ARRAY_SIZEOF(kmin_ioasic_devs);
static int kmin_ioasic_ndevs = ARRAY_SIZEOF(kmin_ioasic_devs);
#endif

#if defined(DEC_MAXINE)
static struct ioasic_dev xine_ioasic_devs[] = {
	{ "lance",	0x0C0000, C(XINE_LANCE_SLOT),	IOASIC_INTR_LANCE },
	{ "scc",	0x100000, C(XINE_SCC0_SLOT),	IOASIC_INTR_SCC_0 },
	{ "mc146818",	0x200000, C(-1),		0 },
	{ "isdn",	0x240000, C(XINE_ISDN_SLOT),	XINE_INTR_ISDN, },
	{ "dtop",	0x280000, C(XINE_DTOP_SLOT),	XINE_INTR_DTOP, },
	{ "fdc",	0x2C0000, C(XINE_FLOPPY_SLOT),	0 },
	{ "asc",	0x300000, C(XINE_SCSI_SLOT),	IOASIC_INTR_SCSI, },
	{ "(TC0)",	0x0,	  C(0),			XINE_INTR_TC_0,	},
	{ "(TC1)",	0x0,	  C(1),			XINE_INTR_TC_1,	},
	{ "(TC2)",	0x0,	  C(2),			XINE_INTR_VINT,	},
};
static int xine_builtin_ndevs = ARRAY_SIZEOF(xine_ioasic_devs) - 3;
static int xine_ioasic_ndevs = ARRAY_SIZEOF(xine_ioasic_devs);
#endif

#if defined(DEC_3MAXPLUS)
static struct ioasic_dev kn03_ioasic_devs[] = {
	{ "lance",	0x0C0000, C(KN03_LANCE_SLOT),	IOASIC_INTR_LANCE, },
	{ "z8530   ",	0x100000, C(KN03_SCC0_SLOT),	IOASIC_INTR_SCC_0, },
	{ "z8530   ",	0x180000, C(KN03_SCC1_SLOT),	IOASIC_INTR_SCC_1, },
	{ "mc146818",	0x200000, C(-1),		0,		},
	{ "asc",	0x300000, C(KN03_SCSI_SLOT),	IOASIC_INTR_SCSI, },
	{ "(TC0)",	0x0,	  C(0),			KN03_INTR_TC_0,	},
	{ "(TC1)",	0x0,	  C(1),			KN03_INTR_TC_1,	},
	{ "(TC2)",	0x0,	  C(2),			KN03_INTR_TC_2,	},
};
static int kn03_builtin_ndevs = ARRAY_SIZEOF(kn03_ioasic_devs) - 3;
static int kn03_ioasic_ndevs = ARRAY_SIZEOF(kn03_ioasic_devs);
#endif

static int	ioasicmatch __P((struct device *, struct cfdata *, void *));
static void	ioasicattach __P((struct device *, struct device *, void *));

const struct cfattach ioasic_ca = {
	sizeof(struct ioasic_softc), ioasicmatch, ioasicattach
};

tc_addr_t	ioasic_base = 0;

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

	if (cfdata->cf_unit > 0)
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
	int i, imsk;

	sc->sc_bst = ta->ta_memt;
	if (bus_space_map(ta->ta_memt, ta->ta_addr,
			0x400000, 0, &sc->sc_bsh)) {
		printf("%s: unable to map device\n", sc->sc_dv.dv_xname);
		return;
	}
	sc->sc_dmat = ta->ta_dmat;
	sc->sc_cookie = ta->ta_cookie;

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

	/*
	 * Turn off all device interrupt bits.
	 * (This _does_ include TC option slot bits.
	 */
	imsk = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK);
	for (i = 0; i < ioasic_ndevs; i++)
		imsk &= ~ioasic_devs[i].iad_intrbits;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK, imsk);

	/*
	 * Try to configure each device.
	 */
	ioasic_attach_devs(sc, ioasic_devs, builtin_ndevs);
}

#if 1 /* XXX for now XXX */
void
ioasic_intr_establish(dev, cookie, level, handler, val)
	struct device *dev;
	void *cookie;
	int level;
	int (*handler) __P((void *));
	void *val;
{
	(*tc_enable_interrupt)((unsigned)cookie, handler, val, 1);
}

#else /* XXX eventually XXX */

void
ioasic_intr_establish(ioa, cookie, level, func, arg)
	struct device *ioa;
	void *cookie, *arg;
	int level;
	int (*func) __P((void *));
{
	struct ioasic_softc *sc = (void *)ioasic_cd.cd_devs[0];
	int i, intrbits;

	for (i = 0; i < ioasic_ndevs; i++) {
		if (ioasic_devs[i].iad_cookie == cookie)
			goto found;
	}
	panic("ioasic_intr_establish: invalid cookie %d", (int)cookie);
found:

	intrtab[(int)cookie].ih_func = func;
	intrtab[(int)cookie].ih_arg = arg;
	
	intrbits = ioasic_devs[i].iad_intrbits;
	i = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK);
	i |= intrbits;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK, i);
}

void
ioasic_intr_disestablish(ioa, cookie)
	struct device *ioa;
	void *cookie;
{
	panic("ioasic_intr_disestablish: cookie %d", (int)cookie);
}
#endif
