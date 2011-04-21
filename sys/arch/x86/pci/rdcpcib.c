/*	$NetBSD: rdcpcib.c,v 1.1.2.2 2011/04/21 01:41:32 rmind Exp $	*/

/*
 * Copyright (c) 2011 Manuel Bouyer.
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

/*
 * driver for the RDC vortex86/PMX-1000 SoC PCI-ISA bridge, which also drives
 * the watchdog timer
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rdcpcib.c,v 1.1.2.2 2011/04/21 01:41:32 rmind Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/timetc.h>
#include <sys/gpio.h>
#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/gpio/gpiovar.h>
#include <dev/sysmon/sysmonvar.h>

#include "pcibvar.h"

/*
 * special registers: iospace-registers for indirect access to timer and GPIO
 */
#define RDC_IND_BASE 0x22
#define RDC_IND_SIZE 0x2
#define RDC_IND_ADDR 0
#define RDC_IND_DATA 1

struct rdcpcib_softc {
	struct pcib_softc	rdc_pcib;
	
	/* indirect registers mapping */
	bus_space_tag_t		rdc_iot;
	bus_space_handle_t	rdc_ioh;

	/* Watchdog suppoprt */
	struct sysmon_wdog	rdc_smw;
};

static int rdcpcibmatch(device_t, cfdata_t, void *);
static void rdcpcibattach(device_t, device_t, void *);
static int rdcpcibdetach(device_t, int);

static uint8_t rdc_ind_read(struct rdcpcib_softc *, uint8_t);
static void rdc_ind_write(struct rdcpcib_softc *, uint8_t, uint8_t);

static void rdc_wdtimer_configure(device_t);
static int rdc_wdtimer_unconfigure(device_t, int);
static int rdc_wdtimer_setmode(struct sysmon_wdog *);
static int rdc_wdtimer_tickle(struct sysmon_wdog *);
static void rdc_wdtimer_stop(struct rdcpcib_softc *);
static void rdc_wdtimer_start(struct rdcpcib_softc *);

CFATTACH_DECL2_NEW(rdcpcib, sizeof(struct rdcpcib_softc),
    rdcpcibmatch, rdcpcibattach, rdcpcibdetach, NULL,
    pcibrescan, pcibchilddet);

static int
rdcpcibmatch(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_BRIDGE ||
	    PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_BRIDGE_ISA)
		return 0;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_RDC &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_RDC_PCIB)
			return 10;

	return 0;
}

static void
rdcpcibattach(device_t parent, device_t self, void *aux)
{
	struct rdcpcib_softc *sc = device_private(self);

	/* generic PCI/ISA bridge */
	pcibattach(parent, self, aux);

	/* map indirect registers */
	sc->rdc_iot = x86_bus_space_io;
	if (bus_space_map(sc->rdc_iot, RDC_IND_BASE, RDC_IND_SIZE, 0,
	    &sc->rdc_ioh) != 0) {
		aprint_error_dev(self, "couldn't map indirect registers\n");
		return;
	}

	/* Set up the watchdog. */
	rdc_wdtimer_configure(self);

	/* Install power handler XXX */
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
rdcpcibdetach(device_t self, int flags)
{
	struct rdcpcib_softc *sc = device_private(self);
	int rc;

	pmf_device_deregister(self);

	if ((rc = rdc_wdtimer_unconfigure(self, flags)) != 0)
		return rc;

	bus_space_unmap(sc->rdc_iot, sc->rdc_ioh, RDC_IND_SIZE);
	return pcibdetach(self, flags);
}

/* indirect registers read/write */
static uint8_t
rdc_ind_read(struct rdcpcib_softc *sc, uint8_t addr)
{
	bus_space_write_1(sc->rdc_iot, sc->rdc_ioh, RDC_IND_ADDR, addr);
	return bus_space_read_1(sc->rdc_iot, sc->rdc_ioh, RDC_IND_DATA);
}

static void
rdc_ind_write(struct rdcpcib_softc *sc, uint8_t addr, uint8_t data)
{
	bus_space_write_1(sc->rdc_iot, sc->rdc_ioh, RDC_IND_ADDR, addr);
	bus_space_write_1(sc->rdc_iot, sc->rdc_ioh, RDC_IND_DATA, data);
}

/*
 * watchdog timer registers
 */

/* control */
#define RDC_WDT0_CTRL	0x37
#define RDC_WDT0_CTRL_EN	0x40

/* signal select */
#define RDC_WDT0_SSEL	0x38
#define RDC_WDT0_SSEL_MSK	0xf0
#define RDC_WDT0_SSEL_NMI	0xc0
#define RDC_WDT0_SSEL_RST	0xd0

/* counter */
#define RDC_WDT0_CNTL	0x39
#define RDC_WDT0_CNTH	0x3A
#define RDC_WDT0_CNTU	0x3B
#define RDC_WDT0_FREQ		32768 /* Hz */
#define RDC_WDT0_PERIOD_MAX	(1 << 24)

/* clear counter */
#define RDC_WDT0_CTRL1	0x3c
#define RDC_WDT0_CTRL1_RELOAD	0x40
#define RDC_WDT0_CRTL1_FIRE	0x80


/*
 * Initialize the watchdog timer.
 */
static void
rdc_wdtimer_configure(device_t self)
{
	struct rdcpcib_softc *sc = device_private(self);
	uint8_t reg;

	/* Explicitly stop the timer. */
	rdc_wdtimer_stop(sc);

	/* 
	 * Register the driver with the sysmon watchdog framework.
	 */
	sc->rdc_smw.smw_name = device_xname(self);
	sc->rdc_smw.smw_cookie = sc;
	sc->rdc_smw.smw_setmode = rdc_wdtimer_setmode;
	sc->rdc_smw.smw_tickle = rdc_wdtimer_tickle;
	sc->rdc_smw.smw_period = RDC_WDT0_PERIOD_MAX / RDC_WDT0_FREQ;

	if (sysmon_wdog_register(&sc->rdc_smw)) {
		aprint_error_dev(self, "unable to register wdt"
		       "as a sysmon watchdog device.\n");
		return;
	}

	aprint_verbose_dev(self, "watchdog timer configured.\n");
	reg = rdc_ind_read(sc, RDC_WDT0_CTRL1);
	if (reg & RDC_WDT0_CRTL1_FIRE) {
		aprint_error_dev(self, "watchdog fired bit set, clearing\n");
		rdc_ind_write(sc, RDC_WDT0_CTRL1, reg & ~RDC_WDT0_CRTL1_FIRE);
	}
}

static int
rdc_wdtimer_unconfigure(device_t self, int flags)
{
	struct rdcpcib_softc *sc = device_private(self);
	int rc;

	if ((rc = sysmon_wdog_unregister(&sc->rdc_smw)) != 0) {
		if (rc == ERESTART)
			rc = EINTR;
		return rc;
	}

	/* Explicitly stop the timer. */
	rdc_wdtimer_stop(sc);

	return 0;
}


/*
 * Sysmon watchdog callbacks.
 */
static int
rdc_wdtimer_setmode(struct sysmon_wdog *smw)
{
	struct rdcpcib_softc *sc = smw->smw_cookie;
	unsigned int period;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		/* Stop the timer. */
		rdc_wdtimer_stop(sc);
	} else {
		period = smw->smw_period * RDC_WDT0_FREQ;
		if (period < 1 ||
		    period > RDC_WDT0_PERIOD_MAX)
			return EINVAL;
		period = period - 1;

		/* Stop the timer, */
		rdc_wdtimer_stop(sc);

		/* set the timeout, */
		rdc_ind_write(sc, RDC_WDT0_CNTL, (period >>  0) & 0xff);
		rdc_ind_write(sc, RDC_WDT0_CNTH, (period >>  8) & 0xff);
		rdc_ind_write(sc, RDC_WDT0_CNTU, (period >> 16) & 0xff);

		/* and start the timer again */
		rdc_wdtimer_start(sc);
	}
	return 0;
}

static int
rdc_wdtimer_tickle(struct sysmon_wdog *smw)
{
	struct rdcpcib_softc *sc = smw->smw_cookie;
	uint8_t reg;

	reg = rdc_ind_read(sc, RDC_WDT0_CTRL1);
	rdc_ind_write(sc, RDC_WDT0_CTRL1, reg | RDC_WDT0_CTRL1_RELOAD);
	return 0;
}

static void
rdc_wdtimer_stop(struct rdcpcib_softc *sc)
{
	uint8_t reg;
	reg = rdc_ind_read(sc, RDC_WDT0_CTRL);
	rdc_ind_write(sc, RDC_WDT0_CTRL, reg & ~RDC_WDT0_CTRL_EN);
}

static void
rdc_wdtimer_start(struct rdcpcib_softc *sc)
{
	uint8_t reg;
	rdc_ind_write(sc, RDC_WDT0_SSEL, RDC_WDT0_SSEL_RST);
	reg = rdc_ind_read(sc, RDC_WDT0_CTRL);
	rdc_ind_write(sc, RDC_WDT0_CTRL, reg | RDC_WDT0_CTRL_EN);
}
