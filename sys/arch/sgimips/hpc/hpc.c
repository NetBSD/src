/*	$NetBSD: hpc.c,v 1.1.2.1 2001/08/25 06:15:49 thorpej Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
 * Copyright (c) 2001 Rafal K. Boni
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
 *          NetBSD Project.  See http://www.netbsd.org/ for
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/machtype.h>

#include <sgimips/gio/gioreg.h>
#include <sgimips/gio/giovar.h>

#include <sgimips/hpc/hpcvar.h>
#include <sgimips/hpc/hpcreg.h>
#include <sgimips/hpc/iocreg.h>

#include "locators.h"

struct hpc_softc {
	struct device 		sc_dev;

	bus_addr_t		sc_base;

	bus_space_tag_t		sc_ct;
	bus_space_handle_t	sc_ch;
};

extern int mach_type;		/* IPxx type */
extern int mach_subtype;	/* subtype: eg., Guiness/Fullhouse for IP22 */
extern int mach_boardrev;	/* machine board revision, in case it matters */

extern struct sgimips_bus_dma_tag sgimips_default_bus_dma_tag;

static int	hpc_match(struct device *, struct cfdata *, void *);
static void	hpc_attach(struct device *, struct device *, void *);
static int	hpc_print(void *, const char *);
static int	hpc_search(struct device *, struct cfdata *, void *);

static int	hpc_power_intr(void *);

struct cfattach hpc_ca = {
        sizeof(struct hpc_softc), hpc_match, hpc_attach 
};

static int
hpc_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;                                      
{
	struct gio_attach_args* ga = aux;

	/* Make sure it's actually there and readable */
	if (badaddr((void*)MIPS_PHYS_TO_KSEG1(ga->ga_addr), sizeof(u_int32_t)))
		return 0;

        return 1;
}

static void
hpc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct hpc_softc *sc = (struct hpc_softc *)self;
	struct gio_attach_args* ga = aux;
	struct hpc_attach_args ha;

	/*
	 * XXX
	 */
	printf("\n");

	sc->sc_ct = 1;
	sc->sc_ch = ga->ga_ioh;

	sc->sc_base = ga->ga_addr;
	config_search(hpc_search, self, &ha);

	/* 
	 * XXXrkb: only true for first HPC, but didn't know where else to
	 * shove it (ip22_intr is too early).  I suppose I should request
	 * a post-config callback or something, but where?
	 */
	cpu_intr_establish(9, IPL_NONE, hpc_power_intr, sc);
}

static int
hpc_print(aux, name)
	void *aux;
	const char *name;
{
	struct hpc_attach_args *ha = aux;

	if (name != 0)
		return QUIET;

	if (ha->ha_offset != HPCCF_OFFSET_DEFAULT)
		printf(" offset 0x%lx", ha->ha_offset);

	return UNCONF;
}

static int
hpc_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf; 
	void *aux;
{ 
	struct hpc_attach_args *haa = aux;
	struct hpc_softc *sc = (struct hpc_softc *)parent;

	do {
		haa->ha_name = cf->cf_driver->cd_name;
		haa->ha_offset = cf->cf_loc[HPCCF_OFFSET];

		haa->ha_iot = 1;	/* XXX */
		haa->ha_ioh = MIPS_PHYS_TO_KSEG1(sc->sc_base);
		haa->ha_dmat = &sgimips_default_bus_dma_tag;

		if ((*cf->cf_attach->ca_match)(parent, cf, haa) > 0)
			config_attach(parent, cf, haa, hpc_print);
	} while (cf->cf_fstate == FSTATE_STAR);

	return 0;
}

static int
hpc_power_intr(void * arg)
{
	u_int32_t pwr_reg;

	pwr_reg = *((volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fbd9850));
	*((volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fbd9850)) = pwr_reg;

	printf("hpc_power_intr: panel reg = %08x\n", pwr_reg);

	if (pwr_reg & 2)
	    cpu_reboot(RB_HALT, NULL);

	return 1;
}

