/*	$NetBSD: epsoc.c,v 1.11.2.1 2012/10/30 17:19:01 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: epsoc.c,v 1.11.2.1 2012/10/30 17:19:01 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/ep93xx/ep93xxreg.h> 
#include <arm/ep93xx/ep93xxvar.h>
#include <arm/ep93xx/epsocvar.h>

#include "locators.h"

static int	epsoc_match(device_t, cfdata_t, void *);
static void	epsoc_attach(device_t, device_t, void *);
static int	epsoc_search(device_t, cfdata_t, const int *, void *);
static int	epsoc_print(void *, const char *);
static int	epsoc_submatch(device_t, cfdata_t, const int *, void *);

CFATTACH_DECL_NEW(epsoc, sizeof(struct epsoc_softc),
    epsoc_match, epsoc_attach, NULL, NULL);

struct epsoc_softc *epsoc_sc = NULL;

static int
epsoc_match(device_t parent, cfdata_t match, void *aux)
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
epsoc_attach(device_t parent, device_t self, void *aux)
{
	struct epsoc_softc	*sc;
	u_int64_t		fclk, pclk, hclk;
	u_int32_t		id, clkset1;
	const char		*rev;
	int			locs[EPSOCCF_NLOCS];
	struct epsoc_attach_args sa;

	sc = device_private(self);

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
	printf("%s: fclk %lld.%02lld MHz hclk %lld.%02lld MHz pclk %lld.%02lld MHz\n", 
		device_xname(self),
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
	 *	some device is used by other system device. so attach first.
	 */
	sa.sa_iot = sc->sc_iot;
	sa.sa_dmat = sc->sc_dmat;
	sa.sa_hclk = sc->sc_hclk;
	sa.sa_pclk = sc->sc_pclk;
	locs[EPSOCCF_ADDR] = EP93XX_APB_HWBASE + EP93XX_APB_TIMERS;
	config_found_sm_loc(self, "epsoc", locs, &sa,
			    epsoc_print, epsoc_submatch);
	locs[EPSOCCF_ADDR] = EP93XX_APB_HWBASE + EP93XX_APB_GPIO;
	sa.sa_gpio = device_private(
	  config_found_sm_loc(self, "epsoc", locs, &sa,
			      epsoc_print, epsoc_submatch));
	config_search_ia(epsoc_search, self, "epsoc", &sa);
	
}

int
epsoc_submatch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct epsoc_attach_args *sa = aux;

	if (cf->cf_loc[EPSOCCF_ADDR] == ldesc[EPSOCCF_ADDR]) {
		sa->sa_addr = cf->cf_loc[EPSOCCF_ADDR];
		sa->sa_size = cf->cf_loc[EPSOCCF_SIZE];
		sa->sa_intr = cf->cf_loc[EPSOCCF_INTR];
		return (config_match(parent, cf, aux));
	} else
		return (0);
}

int
epsoc_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct epsoc_attach_args *sa = aux;

	sa->sa_addr = cf->cf_loc[EPSOCCF_ADDR];
	sa->sa_size = cf->cf_loc[EPSOCCF_SIZE];
	sa->sa_intr = cf->cf_loc[EPSOCCF_INTR];

	if (config_match(parent, cf, aux) > 0)
		config_attach(parent, cf, aux, epsoc_print);

	return (0);
}

static int
epsoc_print(void *aux, const char *name)
{
        struct epsoc_attach_args *sa = aux;

	if (sa->sa_size)
		aprint_normal(" addr 0x%lx", sa->sa_addr);
	if (sa->sa_size > 1)
		aprint_normal("-0x%lx", sa->sa_addr + sa->sa_size - 1);
	if (sa->sa_intr > 1)
		aprint_normal(" intr %d", sa->sa_intr);

	return (UNCONF);
}
