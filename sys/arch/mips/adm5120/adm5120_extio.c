/* $NetBSD: adm5120_extio.c,v 1.1.2.2 2007/03/24 14:54:49 yamt Exp $ */

/*-
 * Copyright (c) 2007 David Young.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adm5120_extio.c,v 1.1.2.2 2007/03/24 14:54:49 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <mips/cache.h>
#include <mips/cpuregs.h>

#include <mips/adm5120/include/adm5120reg.h>
#include <mips/adm5120/include/adm5120var.h>
#include <mips/adm5120/include/adm5120_mainbusvar.h>
#include <mips/adm5120/include/adm5120_extiovar.h>

#include "locators.h"

#ifdef EXTIO_DEBUG
int extio_debug = 1;
#define	EXTIO_DPRINTF(__fmt, ...)		\
do {						\
	if (extio_debug)			\
		printf((__fmt), __VA_ARGS__);	\
} while (/*CONSTCOND*/0)
#else /* !EXTIO_DEBUG */
#define	EXTIO_DPRINTF(__fmt, ...)	do { } while (/*CONSTCOND*/0)
#endif /* EXTIO_DEBUG */

static int	extio_match(struct device *, struct cfdata *, void *);
static void	extio_attach(struct device *, struct device *, void *);
static int	extio_submatch(struct device *, struct cfdata *,
			       const int *, void *);
static int	extio_print(void *, const char *);

CFATTACH_DECL(extio, sizeof(struct extio_softc),
    extio_match, extio_attach, NULL, NULL);

/* There can be only one. */
int	extio_found;

struct extiodev {
	const char	*ed_name;
	bus_addr_t	ed_addr;
	int		ed_irq;
	uint32_t	ed_gpio_mask;
	int		ed_cfio;
};

struct extiodev extiodevs[] = {
	{"wdc",		ADM5120_BASE_EXTIO0,	0,	__BIT(4),	1},
	{NULL,		0,			0,	0x0,		0},
};

static int
extio_match(struct device *parent, struct cfdata *match, void *aux)
{
	return !extio_found;
}

static void
extio_attach_args_create(struct extio_attach_args *ea, struct extiodev *ed,
    void *gpio, bus_space_tag_t st)
{
	ea->ea_name = ed->ed_name;
	ea->ea_addr = ed->ed_addr;
	ea->ea_irq = ed->ed_irq;
	ea->ea_st = st;
	ea->ea_gpio = gpio;
	ea->ea_gpio_mask = ed->ed_gpio_mask;
	ea->ea_cfio = ed->ed_cfio;
}

static void
extio_mpmc_dump(struct extio_softc *sc)
{
	EXTIO_DPRINTF("%s: regs:\n"
	    "  ctl 0x%08" PRIx32 "\n"
	    "  sts 0x%08" PRIx32 "\n"
	    "   sc 0x%08" PRIx32 "\n"
	    "  sww 0x%08" PRIx32 "\n"
	    "  swo 0x%08" PRIx32 "\n"
	    "  swr 0x%08" PRIx32 "\n"
	    "  swp 0x%08" PRIx32 "\n"
	    " swwr 0x%08" PRIx32 "\n"
	    "  swt 0x%08" PRIx32 "\n", __func__,
	    bus_space_read_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_CONTROL),
	    bus_space_read_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_STATUS),
	    bus_space_read_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_SC(2)),
	    bus_space_read_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_SWW(2)),
	    bus_space_read_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_SWO(2)),
	    bus_space_read_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_SWR(2)),
	    bus_space_read_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_SWP(2)),
	    bus_space_read_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_SWWR(2)),
	    bus_space_read_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_SWT(2)));
}

static void
extio_mpmc_init(struct extio_softc *sc)
{
	int i, s;
#if 0
	uint32_t control;
#endif
	uint32_t status;

	/* Map MultiPort Memory Controller */
	if (bus_space_map(sc->sc_obiot, ADM5120_BASE_MPMC, 0x280, 0,
	                  &sc->sc_mpmch) != 0) {
		printf("%s: unable to map MPMC\n", device_xname(&sc->sc_dev));
		return;
	}

	extio_mpmc_dump(sc);

#if 0
	control = bus_space_read_4(sc->sc_obiot, sc->sc_mpmch,
	    ADM5120_MPMC_CONTROL) | ADM5120_MPMC_CONTROL_DWB;
	bus_space_write_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_CONTROL,
	    control);
#endif

	s = splhigh();
	/* I wait for MPMC to become idle, and then I enter low-power mode
	 * so that I can safely set the static configuration.
	 */
	for (i = 1000; --i > 0; ) {
		status = bus_space_read_4(sc->sc_obiot, sc->sc_mpmch,
		    ADM5120_MPMC_STATUS);
		if ((status &
		     (ADM5120_MPMC_STATUS_WBS|ADM5120_MPMC_STATUS_BU)) == 0)
			break;
		delay(10);
	}

	if (i == 0) {
		printf("%s: timeout waiting for MPMC idle\n",
		    device_xname(&sc->sc_dev));
		splx(s);
		return;
	} else
		EXTIO_DPRINTF("%s: MPMC idle\n", device_xname(&sc->sc_dev));

#if 0
	control = bus_space_read_4(sc->sc_obiot, sc->sc_mpmch,
	    ADM5120_MPMC_CONTROL) | ADM5120_MPMC_CONTROL_ME |
	    ADM5120_MPMC_CONTROL_LPM;
	bus_space_write_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_CONTROL,
	    control);
#endif

	/*
	 * Configure external I/O to suit the CompactFlash card.
	 *
	 * Static Configuration 2
	 *
	 * 1 Enable 'async page mode four'.
	 * 2 'Byte lane state' bits for active low for both read & write.
	 * 3 No buffer, no write protection.
	 * 4 No extended wait.
	 * 5 Active low chip select.
	 * 7 8-bit memory width.
	 */
	bus_space_write_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_SC(2),
	    ADM5120_MPMC_SC_BLS|ADM5120_MPMC_SC_PM|ADM5120_MPMC_SC_MW_8B);

	/*
	 * Static Wait Wen 2: after asserting chip select, wait 3 HCLK cycles
	 * before asserting write enable.
	 */
	bus_space_write_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_SWW(2),
	    __SHIFTIN(2, ADM5120_MPMC_SWW_WWE));

	/*
	 * Static Wait Oen 2: after selecting chip select, wait 3 HCLK cycles
	 * before asserting output enable.
	 */
	bus_space_write_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_SWO(2),
	    __SHIFTIN(3, ADM5120_MPMC_SWO_WOE));

	/*
	 * Static Wait Rd 2: set wait state time to 27 HCLK cycles.
	 */
	bus_space_write_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_SWR(2),
	    __SHIFTIN(26, ADM5120_MPMC_SWR_NMRW));

	/*
	 * Static Wait Wait Page 2: set wait state time to 30 HCLK cycles.
	 */
	bus_space_write_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_SWP(2),
	    __SHIFTIN(29, ADM5120_MPMC_SWP_WPS));

	/*
	 * Static Wait Wait Wr 2: set wait state time to 22 HCLK cycles.
	 */
	bus_space_write_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_SWWR(2),
	    __SHIFTIN(20, ADM5120_MPMC_SWWR_WWS));

	/*
	 * Static Wait Wait Turn 2: 10 HCLK cycles for turnaround.
	 */
	bus_space_write_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_SWT(2),
	    __SHIFTIN(9, ADM5120_MPMC_SWT_WAITTURN));

#if 0
	/* Leave low-power mode. */
	control = bus_space_read_4(sc->sc_obiot, sc->sc_mpmch,
	    ADM5120_MPMC_CONTROL) &
	    ~(ADM5120_MPMC_CONTROL_LPM|ADM5120_MPMC_CONTROL_DWB);
	bus_space_write_4(sc->sc_obiot, sc->sc_mpmch, ADM5120_MPMC_CONTROL,
	    control);
	splx(s);
#endif

	extio_mpmc_dump(sc);
}

static void
extio_attach(struct device *parent, struct device *self, void *aux)
{
	struct extio_softc *sc = (struct extio_softc *)self;
	struct mainbus_attach_args *ma = (struct mainbus_attach_args *)aux;
	struct extio_attach_args ea;
	struct extiodev *ed;
	struct adm5120_config *admc = &adm5120_configuration;

	extio_found = 1;
	printf("\n");

	sc->sc_gpio = ma->ma_gpio;
	sc->sc_obiot = ma->ma_obiot;
	sc->sc_gpioh = ma->ma_gpioh;

	EXTIO_DPRINTF("%s: %d\n", __func__, __LINE__);

	sc->sc_pm.pm_map = &sc->sc_map[0];

	/* Map GPIO[0] (WAIT#) for input.
	 *
	 * If WAIT# is high (inactive), then enable WAIT# handshake for
	 * EXTIO0 accesses.  Otherwise, assume that WAIT# is
	 * stuck low (active), in which case all accesses would timeout
	 * if we enabled WAIT# handshake.
	 *
	 * Map GPIO[1:2].  Program 5120 to treat GPIO[1:2] as
	 * Chip Select / Interrupt pins for External I/O #0.
	 *
	 * Map GPIO[3:4].  Program 5120 to treat GPIO[3:4] as
	 * Chip Select / Interrupt pins for External I/O #1.
	 *
	 * Use GPIO[4] for interrupts.  (Not yet.)
	 */
	if (gpio_pin_map(sc->sc_gpio, 0, __BITS(0, 4), &sc->sc_pm) != 0) {
		printf("%s: failed to map GPIO[1:2]\n",
		    device_xname(&sc->sc_dev));
	}
	EXTIO_DPRINTF("%s: %d\n", __func__, __LINE__);
	gpio_pin_ctl(sc->sc_gpio, &sc->sc_pm, 0, GPIO_PIN_INPUT);
	gpio_pin_ctl(sc->sc_gpio, &sc->sc_pm, 1, GPIO_PIN_OUTPUT);
	gpio_pin_ctl(sc->sc_gpio, &sc->sc_pm, 2, GPIO_PIN_INPUT);
	gpio_pin_ctl(sc->sc_gpio, &sc->sc_pm, 3, GPIO_PIN_OUTPUT);
	gpio_pin_ctl(sc->sc_gpio, &sc->sc_pm, 4, GPIO_PIN_INPUT);
	gpio_pin_write(sc->sc_gpio, &sc->sc_pm, 1, 0);
	gpio_pin_write(sc->sc_gpio, &sc->sc_pm, 3, 0);

	EXTIO_DPRINTF("%s: %d\n", __func__, __LINE__);

	if (gpio_pin_read(sc->sc_gpio, &sc->sc_pm, 0) == GPIO_PIN_HIGH) {
		EXTIO_DPRINTF("%s: WAIT# inactive\n",
		    device_xname(&sc->sc_dev));
		bus_space_write_4(sc->sc_obiot, sc->sc_gpioh, ADM5120_GPIO2,
		    ADM5120_GPIO2_EW | ADM5120_GPIO2_CSX0 | ADM5120_GPIO2_CSX1);
	} else {
		printf("%s: WAIT# active; may be stuck\n",
		    device_xname(&sc->sc_dev));
		bus_space_write_4(sc->sc_obiot, sc->sc_gpioh, ADM5120_GPIO2,
		    ADM5120_GPIO2_CSX0 | ADM5120_GPIO2_CSX1);
	}
	EXTIO_DPRINTF("%s: %d\n", __func__, __LINE__);

	/* Map MultiPort Memory Controller */
	if (bus_space_map(sc->sc_obiot, ADM5120_BASE_MPMC, 0x280, 0,
	                  &sc->sc_mpmch) != 0) {
		printf("%s: unable to map MPMC\n", device_xname(&sc->sc_dev));
		return;
	}

	extio_mpmc_init(sc);
	EXTIO_DPRINTF("%s: %d\n", __func__, __LINE__);

	/* Program 5120 for level interrupts on GPIO[4] (INTX1).  (Not yet.)
	 *
	 * Map interrupt.  (Not yet.  In the mean time, use flags 0x1000 in
	 * kernel configuration so that wdc(4) will expect no interrupts.)
	 */

	cfio_bus_mem_init(&sc->sc_cfio, &admc->extio_space);

	for (ed = extiodevs; ed->ed_name != NULL; ed++) {
		EXTIO_DPRINTF("%s: %d\n", __func__, __LINE__);
		extio_attach_args_create(&ea, ed, sc->sc_gpio,
		    (ed->ed_cfio) ? &sc->sc_cfio : &admc->extio_space);
		EXTIO_DPRINTF("%s: %d\n", __func__, __LINE__);
		(void)config_found_sm_loc(self, "extio", NULL, &ea, extio_print,
		    extio_submatch);
	}
	EXTIO_DPRINTF("%s: %d\n", __func__, __LINE__);
	extio_mpmc_dump(sc);
}

static int
extio_submatch(struct device *parent, struct cfdata *cf,
	       const int *ldesc, void *aux)
{
	struct extio_attach_args *ea = aux;

	if (cf->cf_loc[EXTIOCF_CFIO] != EXTIOCF_CFIO_DEFAULT &&
	    cf->cf_loc[EXTIOCF_CFIO] != ea->ea_cfio)
		return 0;

	if (cf->cf_loc[EXTIOCF_GPIO_MASK] != EXTIOCF_GPIO_MASK_DEFAULT &&
	    cf->cf_loc[EXTIOCF_GPIO_MASK] != ea->ea_gpio_mask)
		return 0;

	if (cf->cf_loc[EXTIOCF_IRQ] != EXTIOCF_IRQ_DEFAULT &&
	    cf->cf_loc[EXTIOCF_IRQ] != ea->ea_irq)
		return 0;

	if (cf->cf_loc[EXTIOCF_ADDR] != EXTIOCF_ADDR_DEFAULT &&
	    cf->cf_loc[EXTIOCF_ADDR] != ea->ea_addr)
		return 0;

	return config_match(parent, cf, aux);
}

static int
extio_print(void *aux, const char *pnp)
{
	struct extio_attach_args *ea = aux;

	if (pnp != NULL)
		aprint_normal("%s at %s", ea->ea_name, pnp);
	if (ea->ea_cfio != EXTIOCF_CFIO_DEFAULT)
		aprint_normal(" cfio");
	if (ea->ea_addr != EXTIOCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%lx", ea->ea_addr);
	if (ea->ea_gpio_mask != EXTIOCF_GPIO_MASK_DEFAULT)
		aprint_normal(" gpio_mask 0x%02x", ea->ea_gpio_mask);

	return UNCONF;
}
