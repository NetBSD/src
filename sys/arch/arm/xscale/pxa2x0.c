/*	$NetBSD: pxa2x0.c,v 1.20.2.2 2013/01/16 05:32:50 yamt Exp $ */

/*
 * Copyright (c) 2002, 2005  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 *	This product includes software developed for the NetBSD Project by
 *	Genetec Corporation.
 * 4. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Autoconfiguration support for the Intel PXA2[15]0 application
 * processor. This code is derived from arm/sa11x0/sa11x0.c
 */

/*-
 * Copyright (c) 2001, The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro and Ichiro FUKUHARA.
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
/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pxa2x0.c,v 1.20.2.2 2013/01/16 05:32:50 yamt Exp $");

#include "pxaintc.h"
#include "pxagpio.h"
#if 0
#include "pxadmac.h"	/* Not yet */
#endif

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <machine/cpu.h>
#include <sys/bus.h>

#include <arm/cpufunc.h>
#include <arm/mainbus/mainbus.h>
#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/xscalereg.h>

struct pxaip_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bust;
	bus_dma_tag_t sc_dmat;
	bus_space_handle_t sc_bush_clk;
	bus_space_handle_t sc_bush_mem;
};

/* prototypes */
static int	pxaip_match(device_t, cfdata_t, void *);
static void	pxaip_attach(device_t, device_t, void *);
static int 	pxaip_search(device_t, cfdata_t, const int *, void *);
static void	pxaip_attach_critical(struct pxaip_softc *);
static int	pxaip_print(void *, const char *);

static int	pxaip_measure_cpuclock(struct pxaip_softc *);

#if defined(CPU_XSCALE_PXA250) && defined(CPU_XSCALE_PXA270)
# define SUPPORTED_CPU	"PXA250 and PXA270"
#elif defined(CPU_XSCALE_PXA250)
# define SUPPORTED_CPU	"PXA250"
#elif defined(CPU_XSCALE_PXA270)
# define SUPPORTED_CPU	"PXA270"
#else
# define SUPPORTED_CPU	"none of PXA2xx"
#endif

/* attach structures */
CFATTACH_DECL_NEW(pxaip, sizeof(struct pxaip_softc),
    pxaip_match, pxaip_attach, NULL, NULL);

static struct pxaip_softc *pxaip_sc;
static vaddr_t pxamemctl_regs;
#define MEMCTL_BOOTSTRAP_REG(reg) \
	(*((volatile uint32_t *)(pxamemctl_regs + (reg))))
static vaddr_t pxaclkman_regs;
#define CLKMAN_BOOTSTRAP_REG(reg) \
	(*((volatile uint32_t *)(pxaclkman_regs + (reg))))

static int
pxaip_match(device_t parent, cfdata_t match, void *aux)
{

#if	!defined(CPU_XSCALE_PXA270)
	if (__CPU_IS_PXA270)
		goto bad_config;
#endif

#if	!defined(CPU_XSCALE_PXA250)
	if (__CPU_IS_PXA250)
		goto bad_config;
#endif

	return 1;

#if	defined(CPU_XSCALE_PXA250) + defined(CPU_XSCALE_PXA270) != 2
 bad_config:
	aprint_error("Kernel is configured for %s, but CPU is %s\n",
		     SUPPORTED_CPU, __CPU_IS_PXA270 ? "PXA270" : "PXA250");
	return 0;
#endif
}

static void
pxaip_attach(device_t parent, device_t self, void *aux)
{
	struct pxaip_softc *sc = device_private(self);
	int cpuclock;

	pxaip_sc = sc;
	sc->sc_dev = self;
	sc->sc_bust = &pxa2x0_bs_tag;
	sc->sc_dmat = &pxa2x0_bus_dma_tag;

	aprint_normal(": Onchip Peripheral Bus\n");

	if (bus_space_map(sc->sc_bust, PXA2X0_CLKMAN_BASE, PXA2X0_CLKMAN_SIZE,
	    0, &sc->sc_bush_clk))
		panic("pxaip_attach: failed to map CLKMAN");

	if (bus_space_map(sc->sc_bust, PXA2X0_MEMCTL_BASE, PXA2X0_MEMCTL_SIZE,
	    0, &sc->sc_bush_mem))
		panic("pxaip_attach: failed to map MEMCTL");

	/*
	 * Calculate clock speed
	 * This takes 2 secs at most.
	 */
	cpuclock = pxaip_measure_cpuclock(sc) / 1000;
	printf("%s: CPU clock = %d.%03d MHz\n", device_xname(self),
	    cpuclock/1000, cpuclock%1000 );

	aprint_normal("%s: kernel is configured for " SUPPORTED_CPU
		      ", cpu type is %s\n",
		      device_xname(self),
		      __CPU_IS_PXA270 ? "PXA270" : "PXA250");

	/*
	 * Attach critical devices
	 */
	pxaip_attach_critical(sc);

	/*
	 * Attach all other devices
	 */
	config_search_ia(pxaip_search, self, "pxaip", sc);
}

static int
pxaip_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct pxaip_softc *sc = aux;
	struct pxaip_attach_args aa;

	aa.pxa_iot = sc->sc_bust;
	aa.pxa_dmat = sc->sc_dmat;
	aa.pxa_name = cf->cf_name;
	aa.pxa_addr = cf->cf_loc[PXAIPCF_ADDR];
	aa.pxa_size = cf->cf_loc[PXAIPCF_SIZE];
	aa.pxa_index = cf->cf_loc[PXAIPCF_INDEX];
	aa.pxa_intr = cf->cf_loc[PXAIPCF_INTR];

	if (config_match(parent, cf, &aa))
		config_attach(parent, cf, &aa, pxaip_print);

	return 0;
}

static void
pxaip_attach_critical(struct pxaip_softc *sc)
{
	struct pxaip_attach_args aa;

	aa.pxa_iot = sc->sc_bust;
	aa.pxa_dmat = sc->sc_dmat;
	aa.pxa_name = "pxaintc";
	aa.pxa_addr = PXA2X0_INTCTL_BASE;
	aa.pxa_size = PXA2X0_INTCTL_SIZE;
	aa.pxa_intr = PXAIPCF_INTR_DEFAULT;
	if (config_found(sc->sc_dev, &aa, pxaip_print) == NULL)
		panic("pxaip_attach_critical: failed to attach INTC!");

#if NPXAGPIO > 0
	aa.pxa_iot = sc->sc_bust;
	aa.pxa_dmat = sc->sc_dmat;
	aa.pxa_name = "pxagpio";
	aa.pxa_addr = PXA2X0_GPIO_BASE;
	aa.pxa_size = PXA2X0_GPIO_SIZE;
	aa.pxa_intr = PXAIPCF_INTR_DEFAULT;
	if (config_found(sc->sc_dev, &aa, pxaip_print) == NULL)
		panic("pxaip_attach_critical: failed to attach GPIO!");
#endif

#if NPXADMAC > 0
	aa.pxa_iot = sc->sc_bust;
	aa.pxa_dmat = sc->sc_dmat;
	aa.pxa_name = "pxaidmac";
	aa.pxa_addr = PXA2X0_DMAC_BASE;
	aa.pxa_size = PXA2X0_DMAC_SIZE;
	aa.pxa_intr = PXA2X0_INT_DMA;
	if (config_found(sc->sc_dev, &aa, pxaip_print) == NULL)
		panic("pxaip_attach_critical: failed to attach DMAC!");
#endif
}

static int
pxaip_print(void *aux, const char *name)
{
	struct pxaip_attach_args *sa = (struct pxaip_attach_args *)aux;

	if (sa->pxa_addr != PXAIPCF_ADDR_DEFAULT) {
		aprint_normal(" addr 0x%lx", sa->pxa_addr);
		if (sa->pxa_size > PXAIPCF_SIZE_DEFAULT)
			aprint_normal("-0x%lx", sa->pxa_addr + sa->pxa_size-1);
	}
	if (sa->pxa_intr != PXAIPCF_INTR_DEFAULT)
		aprint_normal(" intr %d", sa->pxa_intr);

	return (UNCONF);
}

static inline uint32_t
read_clock_counter_xsc1(void)
{
	uint32_t x;
	__asm volatile("mrc	p14, 0, %0, c1, c0, 0" : "=r" (x) );

	return x;
}

static inline uint32_t
read_clock_counter_xsc2(void)
{
	uint32_t x;
	__asm volatile("mrc	p14, 0, %0, c1, c1, 0" : "=r" (x) );

	return x;
}

static int
pxaip_measure_cpuclock(struct pxaip_softc *sc)
{
	uint32_t rtc0, rtc1, start, end;
	uint32_t pmcr_save;
	bus_space_handle_t ioh;
	int irq;
	int is_xsc2 = CPU_IS_PXA270;
#define	read_clock_counter()	(is_xsc2 ? read_clock_counter_xsc2() : \
					read_clock_counter_xsc1())

	if (bus_space_map(sc->sc_bust, PXA2X0_RTC_BASE, PXA2X0_RTC_SIZE, 0,
	    &ioh))
		panic("pxaip_measure_cpuclock: can't map RTC");

	irq = disable_interrupts(I32_bit|F32_bit);

	if (is_xsc2) {
		__asm volatile(
			"mrc p14, 0, %0, c0, c1, 0" : "=r" (pmcr_save));
		/* Enable clock counter */
		__asm volatile(
			"mcr p14, 0, %0, c0, c1, 0" : : "r" (PMNC_E|PMNC_C));
	}
	else {
		__asm volatile(
			"mrc p14, 0, %0, c0, c0, 0" : "=r" (pmcr_save));
		/* Enable clock counter */
		__asm volatile(
			"mcr p14, 0, %0, c0, c0, 0" : : "r" (PMNC_E|PMNC_C));
	}

	rtc0 = bus_space_read_4(sc->sc_bust, ioh, RTC_RCNR);
	/* Wait for next second starts */
	while ((rtc1 = bus_space_read_4(sc->sc_bust, ioh, RTC_RCNR)) == rtc0)
		;
	start = read_clock_counter();
	while(rtc1 == bus_space_read_4(sc->sc_bust, ioh, RTC_RCNR))
		;		/* Wait for 1sec */
	end = read_clock_counter();

	if (is_xsc2)
		__asm volatile(
			"mcr p14, 0, %0, c0, c1, 0" : : "r" (pmcr_save));
	else
		__asm volatile(
			"mcr p14, 0, %0, c0, c0, 0" : : "r" (pmcr_save));
	restore_interrupts(irq);

	bus_space_unmap(sc->sc_bust, ioh, PXA2X0_RTC_SIZE);

	return end - start;
}

void
pxa2x0_turbo_mode(int f)
{
	__asm volatile("mcr p14, 0, %0, c6, c0, 0" : : "r" (f));
}

void
pxa2x0_probe_sdram(vaddr_t memctl_va, paddr_t *start, paddr_t *size)
{
	uint32_t mdcnfg, dwid, dcac, drac, dnb;
	int i;

	mdcnfg = *((volatile uint32_t *)(memctl_va + MEMCTL_MDCNFG));

	/*
	 * Scan all 4 SDRAM banks
	 */
	for (i = 0; i < PXA2X0_SDRAM_BANKS; i++) {
		start[i] = 0;
		size[i] = 0;

		switch (i) {
		case 0:
		case 1:
			if ((i == 0 && (mdcnfg & MDCNFG_DE0) == 0) ||
			    (i == 1 && (mdcnfg & MDCNFG_DE1) == 0))
				continue;
			dwid = mdcnfg >> MDCNFD_DWID01_SHIFT;
			dcac = mdcnfg >> MDCNFD_DCAC01_SHIFT;
			drac = mdcnfg >> MDCNFD_DRAC01_SHIFT;
			dnb = mdcnfg >> MDCNFD_DNB01_SHIFT;
			break;

		case 2:
		case 3:
			if ((i == 2 && (mdcnfg & MDCNFG_DE2) == 0) ||
			    (i == 3 && (mdcnfg & MDCNFG_DE3) == 0))
				continue;
			dwid = mdcnfg >> MDCNFD_DWID23_SHIFT;
			dcac = mdcnfg >> MDCNFD_DCAC23_SHIFT;
			drac = mdcnfg >> MDCNFD_DRAC23_SHIFT;
			dnb = mdcnfg >> MDCNFD_DNB23_SHIFT;
			break;
		default:
			panic("pxa2x0_probe_sdram: impossible");
		}

		dwid = 2 << (1 - (dwid & MDCNFD_DWID_MASK));  /* 16/32 width */
		dcac = 1 << ((dcac & MDCNFD_DCAC_MASK) + 8);  /* 8-11 columns */
		drac = 1 << ((drac & MDCNFD_DRAC_MASK) + 11); /* 11-13 rows */
		dnb = 2 << (dnb & MDCNFD_DNB_MASK);	      /* # of banks */

		size[i] = (paddr_t)(dwid * dcac * drac * dnb);
		start[i] = PXA2X0_SDRAM0_START + (i * PXA2X0_SDRAM_BANK_SIZE);
	}
}

void
pxa2x0_memctl_bootstrap(vaddr_t va)
{

	pxamemctl_regs = va;
}

uint32_t
pxa2x0_memctl_read(int reg)
{
	struct pxaip_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	if (__predict_true(pxaip_sc != NULL)) {
		sc = pxaip_sc;
		iot = sc->sc_bust;
		ioh = sc->sc_bush_mem;
		return (bus_space_read_4(iot, ioh, reg));
	} else if (__predict_true(pxamemctl_regs != 0)) {
		return (MEMCTL_BOOTSTRAP_REG(reg));
	}
	panic("pxa2x0_memctl_read: not bootstrapped");
	/*NOTREACHED*/
}

void
pxa2x0_memctl_write(int reg, uint32_t val)
{
	struct pxaip_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	if (__predict_true(pxaip_sc != NULL)) {
		sc = pxaip_sc;
		iot = sc->sc_bust;
		ioh = sc->sc_bush_mem;
		bus_space_write_4(iot, ioh, reg, val);
	} else if (__predict_true(pxamemctl_regs != 0)) {
		MEMCTL_BOOTSTRAP_REG(reg) = val;
	} else {
		panic("pxa2x0_memctl_write: not bootstrapped");
	}
	return;
}

void
pxa2x0_clkman_bootstrap(vaddr_t va)
{

	pxaclkman_regs = va;
}

void
pxa2x0_clkman_config(u_int clk, bool enable)
{
	struct pxaip_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	uint32_t rv;

	if (__predict_true(pxaip_sc != NULL)) {
		sc = pxaip_sc;
		iot = sc->sc_bust;
		ioh = sc->sc_bush_clk;

		rv = bus_space_read_4(iot, ioh, CLKMAN_CKEN);
		rv &= ~clk;
		if (enable)
			rv |= clk;
		bus_space_write_4(iot, ioh, CLKMAN_CKEN, rv);
		return;
	} else if (__predict_true(pxaclkman_regs != 0)) {
		rv = CLKMAN_BOOTSTRAP_REG(CLKMAN_CKEN);
		rv &= ~clk;
		if (enable)
			rv |= clk;
		CLKMAN_BOOTSTRAP_REG(CLKMAN_CKEN) = rv;
		return;
	}
	panic("pxa2x0_clkman_config: not bootstrapped");
}
