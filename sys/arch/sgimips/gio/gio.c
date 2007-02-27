/*	$NetBSD: gio.c,v 1.26.2.1 2007/02/27 16:52:55 yamt Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gio.c,v 1.26.2.1 2007/02/27 16:52:55 yamt Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#define _SGIMIPS_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/machtype.h>
#include <machine/sysconf.h>

#include <sgimips/gio/gioreg.h>
#include <sgimips/gio/giovar.h>
#include <sgimips/gio/giodevs_data.h>

#include "locators.h"
#include "newport.h"
#include "grtwo.h"
#include "light.h"
#include "imc.h"
#include "pic.h"

#if (NNEWPORT > 0)
#include <sgimips/gio/newportvar.h>
#endif

#if (NGRTWO > 0)
#include <sgimips/gio/grtwovar.h>
#endif

#if (NLIGHT > 0)
#include <sgimips/gio/lightvar.h>
#endif

#if (NIMC > 0)
extern int imc_gio64_arb_config(int, uint32_t);
#endif

#if (NPIC > 0)
extern int pic_gio32_arb_config(int, uint32_t);
#endif


struct gio_softc {
	struct	device sc_dev;
};

static int	gio_match(struct device *, struct cfdata *, void *);
static void	gio_attach(struct device *, struct device *, void *);
static int	gio_print(void *, const char *);
static int	gio_search(struct device *, struct cfdata *,
			   const int *, void *);
static int	gio_submatch(struct device *, struct cfdata *,
			     const int *, void *);

CFATTACH_DECL(gio, sizeof(struct gio_softc),
    gio_match, gio_attach, NULL, NULL);

struct gio_probe {
	uint32_t slot;
	uint32_t base;
	uint32_t mach_type;
	uint32_t mach_subtype;
};

/*
 * Expansion Slot Base Addresses
 *
 * IP12, IP20 and IP24 have two GIO connectors: GIO_SLOT_EXP0 and
 * GIO_SLOT_EXP1.
 *
 * On IP24 these slots exist on the graphics board or the IOPLUS
 * "mezzanine" on Indy and Challenge S, respectively. The IOPLUS or
 * graphics board connects to the mainboard via a single GIO64 connector.
 *
 * IP22 has either three or four physical connectors, but only two
 * electrically distinct slots: GIO_SLOT_GFX and GIO_SLOT_EXP0.
 *
 * It should also be noted that DMA is (mostly) not supported in Challenge
 * S's GIO_SLOT_EXP1. See gio(4) for the story.
 */
static const struct gio_probe slot_bases[] = {
	{ GIO_SLOT_GFX,  0x1f000000, MACH_SGI_IP22, MACH_SGI_IP22_FULLHOUSE },

	{ GIO_SLOT_EXP0, 0x1f400000, MACH_SGI_IP12, -1 },
	{ GIO_SLOT_EXP0, 0x1f400000, MACH_SGI_IP20, -1 },
	{ GIO_SLOT_EXP0, 0x1f400000, MACH_SGI_IP22, -1 },

	{ GIO_SLOT_EXP1, 0x1f600000, MACH_SGI_IP12, -1 },
	{ GIO_SLOT_EXP1, 0x1f600000, MACH_SGI_IP20, -1 },
	{ GIO_SLOT_EXP1, 0x1f600000, MACH_SGI_IP22, MACH_SGI_IP22_GUINNESS },

	{ 0, 0, 0, 0 }
};

/*
 * Graphic Board Base Addresses
 *
 * Graphics boards are not treated like expansion slot cards. Their base
 * addresses do not necessarily correspond to GIO slot addresses and they
 * do not contain product identification words. 
 */
static const struct gio_probe gfx_bases[] = {
	/* grtwo, and newport on IP22 */
	{ -1, 0x1f000000, MACH_SGI_IP12, -1 },
	{ -1, 0x1f000000, MACH_SGI_IP20, -1 },
	{ -1, 0x1f000000, MACH_SGI_IP22, -1 },

	/* light */
	{ -1, 0x1f3f0000, MACH_SGI_IP12, -1 },
	{ -1, 0x1f3f0000, MACH_SGI_IP20, -1 },

	/* light (dual headed) */
	{ -1, 0x1f3f8000, MACH_SGI_IP12, -1 },
	{ -1, 0x1f3f8000, MACH_SGI_IP20, -1 },

	/* grtwo, and newport on IP22 */
	{ -1, 0x1f400000, MACH_SGI_IP12, -1 },
	{ -1, 0x1f400000, MACH_SGI_IP20, -1 },
	{ -1, 0x1f400000, MACH_SGI_IP22, -1 },

	/* grtwo */
	{ -1, 0x1f600000, MACH_SGI_IP12, -1 },
	{ -1, 0x1f600000, MACH_SGI_IP20, -1 },
	{ -1, 0x1f600000, MACH_SGI_IP22, -1 },

	/* newport */
	{ -1, 0x1f800000, MACH_SGI_IP22, MACH_SGI_IP22_FULLHOUSE },

	/* newport */
	{ -1, 0x1fc00000, MACH_SGI_IP22, MACH_SGI_IP22_FULLHOUSE },

	{ 0, 0, 0, 0 }
};

/* maximum number of graphics boards possible (arbitrarily large estimate) */
#define MAXGFX 8

static int
gio_match(struct device *parent, struct cfdata *match, void *aux)
{

	return 1;
}

static void
gio_attach(struct device *parent, struct device *self, void *aux)
{
	struct gio_attach_args ga;
	uint32_t gfx[MAXGFX];
	int i, j, ngfx;

	printf("\n");

	ngfx = 0;
	memset(gfx, 0, sizeof(gfx));

	/*
	 * Attach graphics devices first. They do not contain a Product
	 * Identification Word and have no slot number.
	 *
	 * Record addresses to which graphics devices attach so that
	 * we do not confuse them with expansion slots, should the
	 * addresses coincide.
	 */
	for (i = 0; gfx_bases[i].base != 0; i++) {
		/* skip slots that don't apply to us */
		if (gfx_bases[i].mach_type != mach_type)
			continue;

		if (gfx_bases[i].mach_subtype != -1 &&
		    gfx_bases[i].mach_subtype != mach_subtype)
			continue;

		ga.ga_slot = -1;
		ga.ga_addr = gfx_bases[i].base;
		ga.ga_iot = SGIMIPS_BUS_SPACE_NORMAL;
		ga.ga_ioh = MIPS_PHYS_TO_KSEG1(ga.ga_addr);
		ga.ga_dmat = &sgimips_default_bus_dma_tag;
		ga.ga_product = -1;

		if (platform.badaddr((void *)ga.ga_ioh, sizeof(uint32_t)))
			continue;
		
		if (config_found_sm_loc(self, "gio", NULL, &ga, gio_print,
		    gio_submatch)) {
			if (ngfx == MAXGFX)
				panic("gio_attach: MAXGFX");
			gfx[ngfx++] = gfx_bases[i].base;
		}
	}

	/*
	 * Now attach any GIO expansion cards.
	 *
	 * Be sure to skip any addresses to which a graphics device has
	 * already been attached.
	 */
	for (i = 0; slot_bases[i].base != 0; i++) {
		bool skip = false;

		/* skip slots that don't apply to us */
		if (slot_bases[i].mach_type != mach_type)
			continue;

		if (slot_bases[i].mach_subtype != -1 &&
		    slot_bases[i].mach_subtype != mach_subtype)
			continue;

		for (j = 0; j < ngfx; j++) {
			if (slot_bases[i].base == gfx[j]) {
				skip = true;
				break;
			}
		}
		if (skip)
			continue;

		ga.ga_slot = slot_bases[i].slot;
		ga.ga_addr = slot_bases[i].base;
		ga.ga_iot = SGIMIPS_BUS_SPACE_NORMAL;
		ga.ga_ioh = MIPS_PHYS_TO_KSEG1(ga.ga_addr);
		ga.ga_dmat = &sgimips_default_bus_dma_tag;

		if (platform.badaddr((void *)ga.ga_ioh, sizeof(uint32_t)))
			continue;

		ga.ga_product = bus_space_read_4(ga.ga_iot, ga.ga_ioh, 0);

		config_found_sm_loc(self, "gio", NULL, &ga, gio_print,
		    gio_submatch);
	}

	config_search_ia(gio_search, self, "gio", &ga);
}

static int
gio_print(void *aux, const char *pnp)
{
	struct gio_attach_args *ga = aux;
	int i = 0;

	/* gfx probe */
	if (ga->ga_product == -1)
		return (QUIET);

	if (pnp != NULL) {
	  	int product, revision;

		product = GIO_PRODUCT_PRODUCTID(ga->ga_product);

		if (GIO_PRODUCT_32BIT_ID(ga->ga_product))
			revision = GIO_PRODUCT_REVISION(ga->ga_product);
		else
			revision = 0;

		while (gio_knowndevs[i].productid != 0) {
			if (gio_knowndevs[i].productid == product) {
				aprint_normal("%s", gio_knowndevs[i].product);
				break;
			}
			i++;
		}

		if (gio_knowndevs[i].productid == 0)
			aprint_normal("unknown GIO card");

		aprint_normal(" (product 0x%02x revision 0x%02x) at %s",
		    product, revision, pnp);
	}

	if (ga->ga_slot != GIOCF_SLOT_DEFAULT)
		aprint_normal(" slot %d", ga->ga_slot);
	if (ga->ga_addr != (uint32_t) GIOCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%x", ga->ga_addr);

	return UNCONF;
}

static int
gio_search(struct device *parent, struct cfdata *cf,
	   const int *ldesc, void *aux)
{
	struct gio_attach_args *ga = aux;

	do {
		/* Handled by direct configuration, so skip here */
		if (cf->cf_loc[GIOCF_ADDR] == GIOCF_ADDR_DEFAULT)
			return 0;

		ga->ga_slot = cf->cf_loc[GIOCF_SLOT];
		ga->ga_addr = cf->cf_loc[GIOCF_ADDR];
		ga->ga_iot = 0;
		ga->ga_ioh = MIPS_PHYS_TO_KSEG1(ga->ga_addr);

		if (config_match(parent, cf, ga) > 0)
			config_attach(parent, cf, ga, gio_print);
	} while (cf->cf_fstate == FSTATE_STAR);

	return 0;
}

static int
gio_submatch(struct device *parent, struct cfdata *cf,
	     const int *ldesc, void *aux)
{
	struct gio_attach_args *ga = aux;

	if (cf->cf_loc[GIOCF_SLOT] != GIOCF_SLOT_DEFAULT &&
	    cf->cf_loc[GIOCF_SLOT] != ga->ga_slot)
		return 0;

	if (cf->cf_loc[GIOCF_ADDR] != GIOCF_ADDR_DEFAULT &&
	    cf->cf_loc[GIOCF_ADDR] != ga->ga_addr)
		return 0;

	return config_match(parent, cf, aux);
}

int
gio_cnattach()
{
	struct gio_attach_args ga;
	int i;

	for (i = 0; gfx_bases[i].base != 0; i++) {
		/* skip bases that don't apply to us */
		if (gfx_bases[i].mach_type != mach_type)
			continue;

		if (gfx_bases[i].mach_subtype != -1 &&
		    gfx_bases[i].mach_subtype != mach_subtype)
			continue;

		ga.ga_slot = -1;
		ga.ga_addr = gfx_bases[i].base;
		ga.ga_iot = SGIMIPS_BUS_SPACE_NORMAL;
		ga.ga_ioh = MIPS_PHYS_TO_KSEG1(ga.ga_addr);
		ga.ga_dmat = &sgimips_default_bus_dma_tag;
		ga.ga_product = -1;
		
		if (platform.badaddr((void *)ga.ga_ioh,sizeof(uint32_t)))
			continue;

#if (NGRTWO > 0)
		if (grtwo_cnattach(&ga) == 0)
			return 0;
#endif

#if (NLIGHT > 0)
		if (light_cnattach(&ga) == 0)
			return 0;
#endif

#if (NNEWPORT > 0)
		if (newport_cnattach(&ga) == 0)
			return 0;
#endif

	}

	return ENXIO;
}

/*
 * Devices living in the expansion slots must enable or disable some
 * GIO arbiter settings. This is accomplished via imc(4) or pic(4)
 * registers, depending on the machine in question.
 */
int
gio_arb_config(int slot, uint32_t flags) 
{

	if (flags == 0)
		return (EINVAL);

	if (flags & ~(GIO_ARB_RT | GIO_ARB_LB | GIO_ARB_MST | GIO_ARB_SLV |
	    GIO_ARB_PIPE | GIO_ARB_NOPIPE | GIO_ARB_32BIT | GIO_ARB_64BIT |
	    GIO_ARB_HPC2_32BIT | GIO_ARB_HPC2_64BIT))
		return (EINVAL);

	if (((flags & GIO_ARB_RT)   && (flags & GIO_ARB_LB))  ||
	    ((flags & GIO_ARB_MST)  && (flags & GIO_ARB_SLV)) ||
	    ((flags & GIO_ARB_PIPE) && (flags & GIO_ARB_NOPIPE)) ||
	    ((flags & GIO_ARB_32BIT) && (flags & GIO_ARB_64BIT)) ||
	    ((flags & GIO_ARB_HPC2_32BIT) && (flags & GIO_ARB_HPC2_64BIT)))
		return (EINVAL);

#if (NPIC > 0)
	if (mach_type == MACH_SGI_IP12)
		return (pic_gio32_arb_config(slot, flags));
#endif

#if (NIMC > 0)
	if (mach_type == MACH_SGI_IP20 || mach_type == MACH_SGI_IP22)
		return (imc_gio64_arb_config(slot, flags));
#endif

	return (EINVAL);
}

/*
 * Establish an interrupt handler for the specified slot.
 *
 * Indy and Challenge S have an interrupt per GIO slot. Indigo and Indigo2
 * share a single interrupt, however.
 */
void *
gio_intr_establish(int slot, int level, int (*func)(void *), void *arg)
{
	int intr;

	switch (mach_type) {
	case MACH_SGI_IP12:
	case MACH_SGI_IP20:
		if (slot == GIO_SLOT_GFX)
			panic("gio_intr_establish: slot %d", slot);
		intr = 6;
		break;

	case MACH_SGI_IP22:
		if (mach_subtype == MACH_SGI_IP22_FULLHOUSE) {
			if (slot == GIO_SLOT_EXP1)
				panic("gio_intr_establish: slot %d", slot);
			intr = 6;
		} else {
			if (slot == GIO_SLOT_GFX)
				panic("gio_intr_establish: slot %d", slot);
			intr = (slot == GIO_SLOT_EXP0) ? 22 : 23;
		}
		break;

	default:
		panic("gio_intr_establish: mach_type");
	}

	return (cpu_intr_establish(intr, level, func, arg));
}

const char *
gio_product_string(int prid)
{
	int i;

	for (i = 0; gio_knowndevs[i].product != NULL; i++)
		if (gio_knowndevs[i].productid == prid)
			return (gio_knowndevs[i].product);

	return (NULL);
}
