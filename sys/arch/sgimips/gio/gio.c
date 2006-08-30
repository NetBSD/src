/*	$NetBSD: gio.c,v 1.22 2006/08/30 23:48:55 rumble Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: gio.c,v 1.22 2006/08/30 23:48:55 rumble Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#define _SGIMIPS_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/machtype.h>

#include <sgimips/gio/gioreg.h>
#include <sgimips/gio/giovar.h>
#include <sgimips/gio/giodevs_data.h>

#include "locators.h"
#include "newport.h"
#include "grtwo.h"
#include "imc.h"
#include "pic.h"

#if (NNEWPORT > 0)
#include <sgimips/gio/newportvar.h>
#endif

#if (NGRTWO > 0)
#include <sgimips/gio/grtwovar.h>
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

static uint32_t gio_slot_addr[] = {
	0x1f400000,
	0x1f600000,
	0x1f000000,
	0
};

static int
gio_match(struct device *parent, struct cfdata *match, void *aux)
{

	return 1;
}

static void
gio_attach(struct device *parent, struct device *self, void *aux)
{
	struct gio_attach_args ga;
	int i;

	printf("\n");

	for (i=0; gio_slot_addr[i] != 0; i++) {
		ga.ga_slot = (gio_slot_addr[i] == 0x1f000000) ? GIO_SLOT_GFX :
		             (gio_slot_addr[i] == 0x1f400000) ? GIO_SLOT_EXP0 :
								GIO_SLOT_EXP1;
		ga.ga_addr = gio_slot_addr[i];
		ga.ga_iot = 0;
		ga.ga_ioh = MIPS_PHYS_TO_KSEG1(gio_slot_addr[i]);
		ga.ga_dmat = &sgimips_default_bus_dma_tag;
		
#if 0
		/* XXX */
		if (bus_space_peek_4(ga.ga_iot, ga.ga_ioh, 0, &ga.ga_product))
			continue;
#else
		if (badaddr((void *)ga.ga_ioh,sizeof(uint32_t)))
			continue;

		ga.ga_product = bus_space_read_4(ga.ga_iot, ga.ga_ioh, 0);
#endif

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
	
	/* XXX Duplicate code XXX */
	for (i=0; gio_slot_addr[i] != 0; i++) {
		ga.ga_slot = i;
		ga.ga_addr = gio_slot_addr[i];
		ga.ga_iot = 0;
		ga.ga_ioh = MIPS_PHYS_TO_KSEG1(gio_slot_addr[i]);
		
#if 0
		/* XXX */
		if (bus_space_peek_4(ga.ga_iot, ga.ga_ioh, 0, &ga.ga_product))
			continue;
#else

		if (badaddr((void *)ga.ga_ioh,sizeof(uint32_t)))
			continue;

		ga.ga_product = bus_space_read_4(ga.ga_iot, ga.ga_ioh, 0);
#endif

#if (NGRTWO > 0)
		if (grtwo_cnattach(&ga) == 0)
			return 0;
#endif

		/* XXX This probably attaches console to the wrong newport on
		 * dualhead Indys, as GFX slot is tried last not first */
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
	    GIO_ARB_PIPE | GIO_ARB_NOPIPE))
		return (EINVAL);

	if (((flags & GIO_ARB_RT)   && (flags & GIO_ARB_LB))  ||
	    ((flags & GIO_ARB_MST)  && (flags & GIO_ARB_SLV)) ||
	    ((flags & GIO_ARB_PIPE) && (flags & GIO_ARB_NOPIPE)))
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
		intr = (slot == GIO_SLOT_EXP0) ? 0 : 6;
		break;

	case MACH_SGI_IP22:
		if (mach_subtype == MACH_SGI_IP22_FULLHOUSE) {
			if (slot == GIO_SLOT_EXP1)
				panic("gio_intr_establish: slot %d", slot);
			intr = (slot == GIO_SLOT_EXP0) ? 0 : 6;
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
