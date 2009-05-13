/*	$NetBSD: ctl.c,v 1.1.6.2 2009/05/13 17:18:17 jym Exp $	 */

/*
 * Copyright (c) 2009 Stephen M. Rumble
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
 * 3. The name of the author may not be used to endorse or promote products
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
__KERNEL_RCSID(0, "$NetBSD: ctl.c,v 1.1.6.2 2009/05/13 17:18:17 jym Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/callout.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/machtype.h>
#include <machine/sysconf.h>

#include <sgimips/dev/ctlreg.h>

struct ctl_softc {
	struct device   	sc_dev;

	bus_space_tag_t		iot;
	bus_space_handle_t	ioh;

};

static int      ctl_match(struct device *, struct cfdata *, void *);
static void     ctl_attach(struct device *, struct device *, void *);
static void	ctl_bus_reset(void);
static void	ctl_bus_error(uint32_t, uint32_t, uint32_t, uint32_t);
static void	ctl_watchdog_enable(void);
static void	ctl_watchdog_disable(void);
static void	ctl_watchdog_tickle(void);

#if defined(BLINK)
static callout_t ctl_blink_ch;
static void	ctl_blink(void *);
#endif

CFATTACH_DECL(ctl, sizeof(struct ctl_softc),
    ctl_match, ctl_attach, NULL, NULL);

static struct ctl_softc csc;

static int
ctl_match(struct device * parent, struct cfdata * match, void *aux)
{
	/*
	 * CTL exists on IP6/IP10 systems.
	 */
	if (mach_type == MACH_SGI_IP6 || mach_type == MACH_SGI_IP10)
		return 1;
	else
		return 0;
}

static void
ctl_attach(struct device * parent, struct device * self, void *aux)
{
	struct mainbus_attach_args *ma = aux;

#ifdef BLINK
	callout_init(&ctl_blink_ch, 0);
#endif

	csc.iot = SGIMIPS_BUS_SPACE_NORMAL;
	if (bus_space_map(csc.iot, ma->ma_addr, 0,
	    BUS_SPACE_MAP_LINEAR, &csc.ioh))
		panic("ctl_attach: could not allocate memory\n");

	platform.bus_reset = ctl_bus_reset;
	platform.intr5 = ctl_bus_error;
	platform.watchdog_enable = ctl_watchdog_enable;
	platform.watchdog_disable = ctl_watchdog_disable;
	platform.watchdog_reset = ctl_watchdog_tickle;

	bus_space_write_2(csc.iot, csc.ioh, CTL_CPUCTRL,
	    (CTL_CPUCTRL_PARITY | CTL_CPUCTRL_SLAVE));

	printf("\n");

	ctl_bus_reset();

#if defined(BLINK)
	ctl_blink(&csc);
#endif
}

static void
ctl_bus_reset(void)
{

	bus_space_read_1(csc.iot, csc.ioh, CTL_LAN_PAR_CLR);
	bus_space_read_1(csc.iot, csc.ioh, CTL_DMA_PAR_CLR);
	bus_space_read_1(csc.iot, csc.ioh, CTL_CPU_PAR_CLR);
	bus_space_read_1(csc.iot, csc.ioh, CTL_VME_PAR_CLR);
}

static void
ctl_bus_error(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{

	printf("ctl0: bus error\n");
	ctl_bus_reset();
}

static void
ctl_watchdog_enable(void)
{
	uint32_t reg;

	/* XXX- doesn't seem to work properly */
	return;

	reg = bus_space_read_2(csc.iot, csc.ioh, CTL_CPUCTRL);
	reg |= CTL_CPUCTRL_WDOG;
	bus_space_write_2(csc.iot, csc.ioh, CTL_CPUCTRL, reg);
}

static void
ctl_watchdog_disable(void)
{
	uint16_t reg;

	/* XXX- doesn't seem to work properly */
	return;

	reg = bus_space_read_2(csc.iot, csc.ioh, CTL_CPUCTRL_WDOG);
	reg &= ~(CTL_CPUCTRL_WDOG);
	bus_space_write_2(csc.iot, csc.ioh, CTL_CPUCTRL, reg);
}

static void
ctl_watchdog_tickle(void)
{

	ctl_watchdog_disable();
	ctl_watchdog_enable();
}

#if defined(BLINK)
static void
ctl_blink(void *self)
{
	struct ctl_softc *sc = (struct ctl_softc *) self;
	int s, value;

	s = splhigh();

	value = bus_space_read_1(sc->iot, sc->ioh, CTL_AUX_CPUCTRL);
	value ^= CTL_AUX_CPUCTRL_CONSLED;
	bus_space_write_1(sc->iot, sc->ioh, CTL_AUX_CPUCTRL, value);
	splx(s);

	/*
	 * Blink rate is:
	 *      full cycle every second if completely idle (loadav = 0)
	 *      full cycle every 2 seconds if loadav = 1
	 *      full cycle every 3 seconds if loadav = 2
	 * etc.
	 */
	s = (((averunnable.ldavg[0] + FSCALE) * hz) >> (FSHIFT + 1));
	callout_reset(&ctl_blink_ch, s, ctl_blink, sc);
}
#endif
