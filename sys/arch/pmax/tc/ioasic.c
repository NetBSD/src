/* $NetBSD: ioasic.c,v 1.1.2.7 1999/03/31 04:50:13 nisimura Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: ioasic.c,v 1.1.2.7 1999/03/31 04:50:13 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <pmax/pmax/pmaxtype.h>
#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>
#include <pmax/tc/ioasicreg.h>

#include "opt_dec_3min.h"
#include "opt_dec_maxine.h"
#include "opt_dec_3maxplus.h"

int	ioasicmatch __P((struct device *, struct cfdata *, void *));
void	ioasicattach __P((struct device *, struct device *, void *));

struct cfattach ioasic_ca = {
	sizeof(struct ioasic_softc), ioasicmatch, ioasicattach,
};

tc_addr_t ioasic_base;

/* XXX XXX XXX */
#define IOASIC_INTR_SCSI	0x000e0200
#define IOASIC_INTR_DTOP	0x00000001
#define IOASIC_INTR_FDC		0x00000090
#define XINE_INTR_TC_0		0x00001000
#define XINE_INTR_TC_1		0x00000020
#define KN03_INTR_TC_0		0x00000800
#define KN03_INTR_TC_1		0x00001000
#define KN03_INTR_TC_2		0x00002000
#define KMIN_INTR_CLOCK		0x00000020

extern u_int32_t iplmask[], oldiplmask[];
/* XXX XXX XXX */

#define C(x)	(void *)(x)

#if defined(DEC_MAXINE)
struct ioasic_dev xine_ioasic_devs[] = {
	{ "lance",	0x0c0000, C(SYS_DEV_LANCE), IOASIC_INTR_LANCE,	},
	{ "z8530   ",	0x100000, C(SYS_DEV_SCC0),  IOASIC_INTR_SCC_0,	},
	{ "mc146818",	0x200000, C(SYS_DEV_BOGUS), 0,			},
	{ "isdn",	0x240000, C(SYS_DEV_ISDN),  IOASIC_INTR_ISDN,	},
	{ "dtop",	0x280000, C(SYS_DEV_DTOP),  IOASIC_INTR_DTOP,	},
	{ "fdc",	0x2C0000, C(SYS_DEV_FDC),   IOASIC_INTR_FDC,	},
	{ "asc",	0x300000, C(SYS_DEV_SCSI),  IOASIC_INTR_SCSI	},
	{ "(TC0)",	0x0,	  C(SYS_DEV_OPT0),  XINE_INTR_TC_0	},
	{ "(TC1)",	0x0,	  C(SYS_DEV_OPT1),  XINE_INTR_TC_1	},
};
int xine_builtin_ndevs = 7;
int xine_ioasic_ndevs = sizeof(xine_ioasic_devs)/sizeof(xine_ioasic_devs[0]);
#endif

#if defined(DEC_3MIN) || defined(DEC_3MAXPLUS)
struct ioasic_dev kn03_ioasic_devs[] = {
	{ "lance",	0x0c0000, C(SYS_DEV_LANCE), IOASIC_INTR_LANCE,	},
	{ "z8530   ",	0x100000, C(SYS_DEV_SCC0),  IOASIC_INTR_SCC_0,	},
	{ "z8530   ",	0x180000, C(SYS_DEV_SCC1),  IOASIC_INTR_SCC_1,	},
	{ "mc146818",	0x200000, C(SYS_DEV_BOGUS), KMIN_INTR_CLOCK,	},
	{ "asc",	0x300000, C(SYS_DEV_SCSI),  IOASIC_INTR_SCSI	},
	{ "(TC0)",	0x0,	  C(SYS_DEV_OPT0),  KN03_INTR_TC_0	},
	{ "(TC1)",	0x0,	  C(SYS_DEV_OPT1),  KN03_INTR_TC_1	},
	{ "(TC2)",	0x0,	  C(SYS_DEV_OPT2),  KN03_INTR_TC_2	},
};
int kn03_builtin_ndevs = 5;
int kn03_ioasic_ndevs = sizeof(kn03_ioasic_devs)/sizeof(kn03_ioasic_devs[0]);
#endif

struct ioasic_dev *ioasic_devs;
int ioasic_ndevs, builtin_ndevs;

int
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

	switch (systype) {
#if defined(DEC_MAXINE)
	case DS_MAXINE:
		ioasic_devs = xine_ioasic_devs;
		ioasic_ndevs = xine_ioasic_ndevs;
		builtin_ndevs = xine_builtin_ndevs;
		break;
#endif
#if defined(DEC_3MIN) || defined(DS_3MAXPLUS)
	case DS_3MIN:
	case DS_3MAXPLUS:
		ioasic_devs = kn03_ioasic_devs;
		ioasic_ndevs = kn03_ioasic_ndevs;
		builtin_ndevs = kn03_builtin_ndevs;
		break;
#endif
	default:
		panic("ioasicmatch: how did we get here?");
	}

	return (1);
}

void
ioasicattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ioasic_softc *sc = (struct ioasic_softc *)self;
	struct tc_attach_args *ta = aux;
	int i;

	sc->sc_bst = ta->ta_memt;
	sc->sc_dmat = ta->ta_dmat;
	if (bus_space_map(ta->ta_memt, ta->ta_addr,
			0x400000, 0, &sc->sc_bsh)) {
		printf("%s: unable to map device\n", sc->sc_dv.dv_xname);
		return;
	}
	sc->sc_cookie = ta->ta_cookie;

	/* XXX XXX XXX */
	sc->sc_base = ta->ta_addr;

	printf("\n");

#if 1 /* !!! necessary? already all-0 upon booting as documented !!! */
	/*
	 * Turn off all device interrupt bits.
	 * (This _does_ include TC option slot bits.
	 */
	for (i = 0; i < ioasic_ndevs; i++)
		*(volatile u_int32_t *)(sc->sc_base + IOASIC_IMSK)
			&= ~ioasic_devs[i].iad_intrbits;
	tc_wmb();
#endif

#if 0
	/* XXX correct if_le_ioasic.c XXX */
	/*
	 * Set up the LANCE DMA area.
	 */
	ioasic_lance_dma_setup(sc);
#endif

	/*
	 * Try to configure each device.
	 */
	ioasic_attach_devs(sc, ioasic_devs, builtin_ndevs);
}

void
ioasic_intr_establish(ioa, cookie, level, func, arg)
	struct device *ioa;
	void *cookie, *arg;
	tc_intrlevel_t level;
	int (*func) __P((void *));
{
	struct ioasic_softc *sc = (void *)ioasic_cd.cd_devs[0];
	u_int dev, i, intrbits;

	dev = (u_long)cookie;

	for (i = 0; i < ioasic_ndevs; i++) {
		if (ioasic_devs[i].iad_cookie == cookie)
			goto found;
	}
	panic("ioasic_intr_establish: invalid cookie %d", dev);
found:

	intrtab[dev].ih_func = func;
	intrtab[dev].ih_arg = arg;
	
	intrbits = ioasic_devs[i].iad_intrbits;
	iplmask[level] |= intrbits;
	*(volatile u_int32_t *)(sc->sc_base + IOASIC_IMSK) |= intrbits;
	tc_wmb();
}

void
ioasic_intr_disestablish(ioa, cookie)
	struct device *ioa;
	void *cookie;
{
	printf("device %s with cookie %d: ", ioa->dv_xname, (int)cookie);
	panic("ioasic_intr_disestablish called");
}

char *
ioasic_lance_ether_address()
{

	return (char *)(ioasic_base + IOASIC_SLOT_2_START);
}

#if 0 /* Jason's new LANCE DMA region */
/*
 * DMA area for IOASIC LANCE.
 * XXX Should be done differently, but this is better than it used to be.
 */
#define LE_IOASIC_MEMSIZE	(128*1024)
#define LE_IOASIC_MEMALIGN	(128*1024)
caddr_t le_iomem;

void	ioasic_lance_dma_setup __P((struct ioasic_softc *));

void
ioasic_lance_dma_setup(sc)
	struct ioasic_softc *sc;
{
	bus_dma_tag_t dmat = sc->sc_dmat;
	bus_dma_segment_t seg;
	u_int32_t csr;
	tc_addr_t tca;
	int rseg;

	/*
	 * Allocate a DMA area for the chip.
	 */
	if (bus_dmamem_alloc(dmat, LE_IOASIC_MEMSIZE, LE_IOASIC_MEMALIGN,
	    0, &seg, 1, &rseg, BUS_DMA_NOWAIT)) {
		printf("%s: can't allocate DMA area for LANCE\n",
		    sc->sc_dv.dv_xname);
		return;
	}
	if (bus_dmamem_map(dmat, &seg, rseg, LE_IOASIC_MEMSIZE,
	    &le_iomem, BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) {
		printf("%s: can't map DMA area for LANCE\n",
		    sc->sc_dv.dv_xname);
		bus_dmamem_free(dmat, &seg, rseg);
		return;
	}

	/*
	 * Create and load the DMA map for the DMA area.
	 */
	if (bus_dmamap_create(dmat, LE_IOASIC_MEMSIZE, 1,
	    LE_IOASIC_MEMSIZE, 0, BUS_DMA_NOWAIT, &sc->sc_lance_dmam)) {
		printf("%s: can't create DMA map\n", sc->sc_dv.dv_xname);
		goto bad;
	}
	if (bus_dmamap_load(dmat, sc->sc_lance_dmam,
	    le_iomem, LE_IOASIC_MEMSIZE, NULL, BUS_DMA_NOWAIT)) {
		printf("%s: can't load DMA map\n", sc->sc_dv.dv_xname);
		goto bad;
	}

	tca = (tc_addr_t)sc->sc_lance_dmam->dm_segs[0].ds_addr;
	if (tca != sc->sc_lance_dmam->dm_segs[0].ds_addr) {
		printf("%s: bad LANCE DMA address\n", sc->sc_dv.dv_xname);
		bus_dmamap_unload(dmat, sc->sc_lance_dmam);
		goto bad;
	}
	bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		IOASIC_LANCE_DMAPTR,
		((tca << 3) & ~(tc_addr_t)0x1f) | ((tca >> 29) & 0x1f));
	csr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR);
	csr |= IOASIC_CSR_DMAEN_LANCE;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, csr);
	return;

 bad:
	bus_dmamem_unmap(dmat, le_iomem, LE_IOASIC_MEMSIZE);
	bus_dmamem_free(dmat, &seg, rseg);
	le_iomem = 0;
}
#else	/* old NetBSD/pmax code */
void	ioasic_lance_dma_setup __P((void *));

void
ioasic_lance_dma_setup(v)
	void *v;
{
	volatile u_int32_t *ldp;
	tc_addr_t tca;

	tca = (tc_addr_t)v;

	ldp = (volatile u_int *)IOASIC_REG_LANCE_DMAPTR(ioasic_base);
	*ldp = ((tca << 3) & ~(tc_addr_t)0x1f) | ((tca >> 29) & 0x1f);
	tc_wmb();

	*(volatile u_int32_t *)IOASIC_REG_CSR(ioasic_base) |=
	    IOASIC_CSR_DMAEN_LANCE;
	tc_wmb();
}
#endif

/*
 * spl(9) for IOASIC DECstations
 */

int _splraise_ioasic __P((int));
int _spllower_ioasic __P((int));
int _splx_ioasic __P((int));

int
_splraise_ioasic(lvl)
	int lvl;
{
	u_int32_t new;

	new = oldiplmask[lvl] = *(u_int32_t *)(ioasic_base + IOASIC_IMSK);
	new &= ~iplmask[lvl];
	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = new;
	tc_wmb();
	return lvl | _splraise(MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1);
}

int
_spllower_ioasic(mask)
{
	int s;

	s = IPL_NONE | _spllower(mask);
	oldiplmask[IPL_NONE] = *(u_int32_t *)(ioasic_base + IOASIC_IMSK);
	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = iplmask[IPL_HIGH];
	tc_wmb();
	return s;
}

int
_splx_ioasic(lvl)
	int lvl;
{
	(void)_splset(lvl & MIPS_INT_MASK);
	if (lvl & 0xff) {
		*(u_int32_t *)(ioasic_base + IOASIC_IMSK) =
			oldiplmask[lvl & 0xff];
		tc_wmb();
	}
	return 0;
}
