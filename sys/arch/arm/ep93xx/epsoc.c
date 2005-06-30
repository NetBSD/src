/*	$NetBSD: epsoc.c,v 1.3 2005/06/30 17:03:52 drochner Exp $	*/

/*
 * Copyright (c) 2004 Jesse Off
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: epsoc.c,v 1.3 2005/06/30 17:03:52 drochner Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/ep93xx/ep93xxreg.h> 
#include <arm/ep93xx/ep93xxvar.h>
#include <arm/ep93xx/epsocvar.h>

#include "locators.h"

static int	epsoc_match(struct device *, struct cfdata *, void *);
static void	epsoc_attach(struct device *, struct device *, void *);
static int	epsoc_search(struct device *, struct cfdata *,
			     const locdesc_t *, void *);
static int	epsoc_print(void *, const char *);

CFATTACH_DECL(epsoc, sizeof(struct epsoc_softc),
    epsoc_match, epsoc_attach, NULL, NULL);

struct epsoc_softc *epsoc_sc = NULL;

static int
epsoc_match(struct device *parent, struct cfdata *match, void *aux)
{
	bus_space_handle_t	ioh;
	u_int32_t		id, ret = 0;

	bus_space_map(&ep93xx_bs_tag, EP93XX_APB_HWBASE + EP93XX_APB_SYSCON,
		EP93XX_APB_SYSCON_SIZE, 0, &ioh);
	id = bus_space_read_4(&ep93xx_bs_tag, ioh, EP93XX_SYSCON_ChipID);
	if ((id & 0x01faffff) == 0x9213) ret = 1;
	bus_space_unmap(&ep93xx_bs_tag, ioh, EP93XX_APB_SYSCON_SIZE);
	return ret;
}

static void
epsoc_attach(struct device *parent, struct device *self, void *aux)
{
	struct epsoc_softc	*sc;
	u_int64_t		fclk, pclk, hclk;
	u_int32_t		id, clkset1;
	const char		*rev;


	sc = (struct epsoc_softc*) self;

	if (epsoc_sc == NULL)
		epsoc_sc = sc;

	sc->sc_iot = &ep93xx_bs_tag;
	bus_space_map(sc->sc_iot, EP93XX_APB_HWBASE + EP93XX_APB_SYSCON,
		EP93XX_APB_SYSCON_SIZE, 0, &sc->sc_ioh);
	id = bus_space_read_4(sc->sc_iot, sc->sc_ioh, EP93XX_SYSCON_ChipID);
	switch(id >> 28) {
	case 0:
		rev = "A";
		break;
	case 1:
		rev = "B";
		break;
	case 2:
		rev = "C";
		break;
	case 3:
		rev = "D0";
		break;
	case 4:
		rev = "D1";
		break;
	case 5:
		rev = "E0";
		break;
	case 6:
		rev = "E1";
		break;
	default:
		rev = ">E1";
		break;
	}
	printf(": Cirrus Logic EP93xx SoC rev %s\n", rev);

	clkset1 = bus_space_read_4(sc->sc_iot, sc->sc_ioh, 
		EP93XX_SYSCON_ClkSet1);
	{
		int fclkdiv, hclkdiv, pclkdiv, pll1_ps;
		int pll1x1fbd1, pll1x2fbd2, pll1x2ipd;
		int hclkdivisors[] = {1, 2, 4, 5, 6, 8, 16, 32};
		
		fclkdiv =    (clkset1 & 0x0e000000) >> 25;
		hclkdiv =    (clkset1 & 0x00700000) >> 20;
		pclkdiv =    (clkset1 & 0x000c0000) >> 18;
		pll1_ps =    (clkset1 & 0x00030000) >> 16;
		pll1x1fbd1 = (clkset1 & 0x0000f800) >> 11;
		pll1x2fbd2 = (clkset1 & 0x000007e0) >> 5;
		pll1x2ipd =  (clkset1 & 0x0000001f);
		fclk = 14745600ULL * ((pll1x1fbd1 + 1) * (pll1x2fbd2 + 1)) /
			((pll1x2ipd + 1) * (1 << pll1_ps));
		hclk = fclk / hclkdivisors[hclkdiv];
		pclk = hclk >> pclkdiv;
		if (fclkdiv > 4) fclkdiv = 0;
		if (clkset1 & 0x00800000)  /* nBYP1 bypasses PLL */
			fclk = fclk >> fclkdiv;
		else
			fclk = 14745600ULL;
	}
	printf("%s: fclk %lld.%02lld Mhz hclk %lld.%02lld Mhz pclk %lld.%02lld Mhz\n", 
		sc->sc_dev.dv_xname,
		fclk / 1000000, (fclk % 1000000 + 5000) / 10000, 
		hclk / 1000000, (hclk % 1000000 + 5000) / 10000, 
		pclk / 1000000, (pclk % 1000000 + 5000) / 10000);

	sc->sc_fclk = fclk;
	sc->sc_hclk = hclk;
	sc->sc_pclk = pclk;

        /* get busdma tag for the platform */
        sc->sc_dmat = ep93xx_bus_dma_init(&ep93xx_bus_dma);

	/*
	 *  Attach each devices
	 */
	config_search_ia(epsoc_search, self, "epsoc", NULL);
	
}

int
epsoc_search(parent, cf, ldesc, aux)
	struct device *parent;
	struct cfdata *cf;
	const locdesc_t *ldesc;
	void *aux;
{
	struct epsoc_softc *sc = (struct epsoc_softc *)parent;
	struct epsoc_attach_args sa;

	sa.sa_iot = sc->sc_iot;
	sa.sa_dmat = sc->sc_dmat;
	sa.sa_addr = cf->cf_loc[EPSOCCF_ADDR];
	sa.sa_size = cf->cf_loc[EPSOCCF_SIZE];
	sa.sa_intr = cf->cf_loc[EPSOCCF_INTR];
	sa.sa_hclk = sc->sc_hclk;
	sa.sa_pclk = sc->sc_pclk;

	if (config_match(parent, cf, &sa) > 0)
		config_attach(parent, cf, &sa, epsoc_print);

	return (0);
}

static int
epsoc_print(aux, name)
	void *aux;
	const char *name;
{
        struct epsoc_attach_args *sa = (struct epsoc_attach_args*)aux;

	if (sa->sa_size)
		aprint_normal(" addr 0x%lx", sa->sa_addr);
	if (sa->sa_size > 1)
		aprint_normal("-0x%lx", sa->sa_addr + sa->sa_size - 1);
	if (sa->sa_intr > 1)
		aprint_normal(" intr %d", sa->sa_intr);

	return (UNCONF);
}
