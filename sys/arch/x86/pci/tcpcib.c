/*	$NetBSD: tcpcib.c,v 1.1.2.2 2013/01/16 05:33:10 yamt Exp $	*/
/*	$OpenBSD: tcpcib.c,v 1.4 2012/10/17 22:32:01 deraadt Exp $	*/

/*
 * Copyright (c) 2012 Matt Dainty <matt@bodgit-n-scarper.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Intel Atom E600 series LPC bridge also containing HPET and watchdog
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tcpcib.c,v 1.1.2.2 2013/01/16 05:33:10 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/timetc.h>
#include <sys/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/i82801lpcvar.h>

#include <dev/sysmon/sysmonvar.h>

#include "pcibvar.h"

#define	E600_LPC_SMBA		0x40		/* SMBus Base Address */
#define	E600_LPC_GBA		0x44		/* GPIO Base Address */
#define	E600_LPC_WDTBA		0x84		/* WDT Base Address */

#define	E600_WDT_SIZE		64		/* I/O region size */
#define	E600_WDT_PV1		0x00		/* Preload Value 1 Register */
#define	E600_WDT_PV2		0x04		/* Preload Value 2 Register */
#define	E600_WDT_RR0		0x0c		/* Reload Register 0 */
#define	E600_WDT_RR1		0x0d		/* Reload Register 1 */
#define	E600_WDT_RR1_RELOAD	(1 << 0)	/* WDT Reload Flag */
#define	E600_WDT_RR1_TIMEOUT	(1 << 1)	/* WDT Timeout Flag */
#define	E600_WDT_WDTCR		0x10		/* WDT Configuration Register */
#define	E600_WDT_WDTCR_PRE	(1 << 2)	/* WDT Prescalar Select */
#define	E600_WDT_WDTCR_RESET	(1 << 3)	/* WDT Reset Select */
#define	E600_WDT_WDTCR_ENABLE	(1 << 4)	/* WDT Reset Enable */
#define	E600_WDT_WDTCR_TIMEOUT	(1 << 5)	/* WDT Timeout Output Enable */
#define	E600_WDT_DCR		0x14		/* Down Counter Register */
#define	E600_WDT_WDTLR		0x18		/* WDT Lock Register */
#define	E600_WDT_WDTLR_LOCK	(1 << 0)	/* Watchdog Timer Lock */
#define	E600_WDT_WDTLR_ENABLE	(1 << 1)	/* Watchdog Timer Enable */
#define	E600_WDT_WDTLR_TIMEOUT	(1 << 2)	/* WDT Timeout Configuration */

#define	E600_HPET_BASE		0xfed00000	/* HPET register base */
#define	E600_HPET_SIZE		0x00000400	/* HPET register size */

#define	E600_HPET_GCID		0x000		/* Capabilities and ID */
#define	E600_HPET_GCID_WIDTH	(1 << 13)	/* Counter Size */
#define	E600_HPET_PERIOD	0x004		/* Counter Tick Period */
#define	E600_HPET_GC		0x010		/* General Configuration */
#define	E600_HPET_GC_ENABLE	(1 << 0)	/* Overall Enable */
#define	E600_HPET_GIS		0x020		/* General Interrupt Status */
#define	E600_HPET_MCV		0x0f0		/* Main Counter Value */
#define	E600_HPET_T0C		0x100		/* Timer 0 Config and Capabilities */
#define	E600_HPET_T0CV		0x108		/* Timer 0 Comparator Value */
#define	E600_HPET_T1C		0x120		/* Timer 1 Config and Capabilities */
#define	E600_HPET_T1CV		0x128		/* Timer 1 Comparator Value */
#define	E600_HPET_T2C		0x140		/* Timer 2 Config and Capabilities */
#define	E600_HPET_T2CV		0x148		/* Timer 2 Comparator Value */

struct tcpcib_softc {
	/* we call pcibattach() which assumes this starts like this: */
	struct pcib_softc	sc_pcib;

	/* Watchdog interface */
	bool sc_wdt_valid;
	struct sysmon_wdog sc_wdt_smw;
	bus_space_tag_t sc_wdt_iot;
	bus_space_handle_t sc_wdt_ioh;

	/* High Precision Event Timer */
	device_t sc_hpetbus;
	bus_space_tag_t sc_hpet_memt;
};

static int	tcpcib_match(device_t, cfdata_t, void *);
static void	tcpcib_attach(device_t, device_t, void *);
static int	tcpcib_detach(device_t, int);
static int	tcpcib_rescan(device_t, const char *, const int *);
static void	tcpcib_childdet(device_t, device_t);
static bool	tcpcib_suspend(device_t, const pmf_qual_t *);
static bool	tcpcib_resume(device_t, const pmf_qual_t *);

static int	tcpcib_wdt_setmode(struct sysmon_wdog *);
static int	tcpcib_wdt_tickle(struct sysmon_wdog *);
static void	tcpcib_wdt_init(struct tcpcib_softc *, int);
static void	tcpcib_wdt_start(struct tcpcib_softc *);
static void	tcpcib_wdt_stop(struct tcpcib_softc *);

CFATTACH_DECL2_NEW(tcpcib, sizeof(struct tcpcib_softc),
    tcpcib_match, tcpcib_attach, tcpcib_detach, NULL,
    tcpcib_rescan, tcpcib_childdet);

static struct tcpcib_device {
	pcireg_t vendor, product;
} tcpcib_devices[] = {
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_E600_LPC }
};

static void
tcpcib_wdt_unlock(struct tcpcib_softc *sc)
{
	/* Register unlocking sequence */
	bus_space_write_1(sc->sc_wdt_iot, sc->sc_wdt_ioh, E600_WDT_RR0, 0x80);
	bus_space_write_1(sc->sc_wdt_iot, sc->sc_wdt_ioh, E600_WDT_RR0, 0x86);
}

static void
tcpcib_wdt_init(struct tcpcib_softc *sc, int period)
{
	uint32_t preload;

	/* Set new timeout */
	preload = (period * 33000000) >> 15;
	preload--;

	/*
	 * Set watchdog to perform a cold reset toggling the GPIO pin and the
	 * prescaler set to 1ms-10m resolution
	 */
	bus_space_write_1(sc->sc_wdt_iot, sc->sc_wdt_ioh, E600_WDT_WDTCR,
	    E600_WDT_WDTCR_ENABLE);
	tcpcib_wdt_unlock(sc);
	bus_space_write_4(sc->sc_wdt_iot, sc->sc_wdt_ioh, E600_WDT_PV1, 0);
	tcpcib_wdt_unlock(sc);
	bus_space_write_4(sc->sc_wdt_iot, sc->sc_wdt_ioh, E600_WDT_PV2,
	    preload);
	tcpcib_wdt_unlock(sc);
	bus_space_write_1(sc->sc_wdt_iot, sc->sc_wdt_ioh, E600_WDT_RR1,
	    E600_WDT_RR1_RELOAD);
}

static void
tcpcib_wdt_start(struct tcpcib_softc *sc)
{
	/* Enable watchdog */
	bus_space_write_1(sc->sc_wdt_iot, sc->sc_wdt_ioh, E600_WDT_WDTLR,
	    E600_WDT_WDTLR_ENABLE);
}

static void
tcpcib_wdt_stop(struct tcpcib_softc *sc)
{
	/* Disable watchdog, with a reload before for safety */
	tcpcib_wdt_unlock(sc);
	bus_space_write_1(sc->sc_wdt_iot, sc->sc_wdt_ioh, E600_WDT_RR1,
	    E600_WDT_RR1_RELOAD);
	bus_space_write_1(sc->sc_wdt_iot, sc->sc_wdt_ioh, E600_WDT_WDTLR, 0);
}

static int
tcpcib_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	unsigned int n;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_BRIDGE ||
	    PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_BRIDGE_ISA)
		return 0;

	for (n = 0; n < __arraycount(tcpcib_devices); n++) {
		if (PCI_VENDOR(pa->pa_id) == tcpcib_devices[n].vendor &&
		    PCI_PRODUCT(pa->pa_id) == tcpcib_devices[n].product)
			return 10;	/* beat pcib(4) */
	}

	return 0;
}

static void
tcpcib_attach(device_t parent, device_t self, void *aux)
{
	struct tcpcib_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	uint32_t reg, wdtbase;

	pmf_device_register(self, tcpcib_suspend, tcpcib_resume);

	/* Provide core pcib(4) functionality */
	pcibattach(parent, self, aux);

	/* High Precision Event Timer */
	sc->sc_hpet_memt = pa->pa_memt;
	tcpcib_rescan(self, "hpetichbus", NULL);

	/* Map Watchdog I/O space */
	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, E600_LPC_WDTBA);
	wdtbase = reg & 0xffff;
	sc->sc_wdt_iot = pa->pa_iot;
	if (reg & (1 << 31) && wdtbase) {
		if (PCI_MAPREG_IO_ADDR(wdtbase) == 0 ||
		    bus_space_map(sc->sc_wdt_iot, PCI_MAPREG_IO_ADDR(wdtbase),
		    E600_WDT_SIZE, 0, &sc->sc_wdt_ioh)) {
			aprint_error_dev(self, "can't map watchdog I/O space\n");
			return;
		}
		aprint_normal_dev(self, "watchdog");

		/* Check for reboot on timeout */
		reg = bus_space_read_1(sc->sc_wdt_iot, sc->sc_wdt_ioh,
		    E600_WDT_RR1);
		if (reg & E600_WDT_RR1_TIMEOUT) {
			aprint_normal(", reboot on timeout");

			/* Clear timeout bit */
			tcpcib_wdt_unlock(sc);
			bus_space_write_1(sc->sc_wdt_iot, sc->sc_wdt_ioh,
			    E600_WDT_RR1, E600_WDT_RR1_TIMEOUT);
		}

		/* Check it's not locked already */
		reg = bus_space_read_1(sc->sc_wdt_iot, sc->sc_wdt_ioh,
		    E600_WDT_WDTLR);
		if (reg & E600_WDT_WDTLR_LOCK) {
			aprint_error(", locked\n");
			return;
		}

		/* Disable watchdog */
		tcpcib_wdt_stop(sc);

		/* Register new watchdog */
		sc->sc_wdt_smw.smw_name = device_xname(self);
		sc->sc_wdt_smw.smw_cookie = sc;
		sc->sc_wdt_smw.smw_setmode = tcpcib_wdt_setmode;
		sc->sc_wdt_smw.smw_tickle = tcpcib_wdt_tickle;
		sc->sc_wdt_smw.smw_period = 600; /* seconds */
		if (sysmon_wdog_register(&sc->sc_wdt_smw)) {
			aprint_error(", unable to register wdog timer\n");
			return;
		}

		sc->sc_wdt_valid = true;
		aprint_normal("\n");
	}

}

static int
tcpcib_detach(device_t self, int flags)
{
	return pcibdetach(self, flags);
}

static int
tcpcib_rescan(device_t self, const char *ifattr, const int *locators)
{
	struct tcpcib_softc *sc = device_private(self);

	if (ifattr_match(ifattr, "hpetichbus") && sc->sc_hpetbus == NULL) {
		struct lpcib_hpet_attach_args hpet_arg;
		hpet_arg.hpet_mem_t = sc->sc_hpet_memt;
		hpet_arg.hpet_reg = E600_HPET_BASE;
		sc->sc_hpetbus = config_found_ia(self, "hpetichbus",
		    &hpet_arg, NULL);
	}

	return pcibrescan(self, ifattr, locators);
}

static void
tcpcib_childdet(device_t self, device_t child)
{
	struct tcpcib_softc *sc = device_private(self);

	if (sc->sc_hpetbus == child) {
		sc->sc_hpetbus = NULL;
		return;
	}

	pcibchilddet(self, child);
}

static bool
tcpcib_suspend(device_t self, const pmf_qual_t *qual)
{
	struct tcpcib_softc *sc = device_private(self);

	if (sc->sc_wdt_valid)
		tcpcib_wdt_stop(sc);

	return true;
}

static bool
tcpcib_resume(device_t self, const pmf_qual_t *qual)
{
	struct tcpcib_softc *sc = device_private(self);
	struct sysmon_wdog *smw = &sc->sc_wdt_smw;

	if (sc->sc_wdt_valid) {
		if ((smw->smw_mode & WDOG_MODE_MASK) != WDOG_MODE_DISARMED &&
		    smw->smw_period > 0) {
			tcpcib_wdt_init(sc, smw->smw_period);
			tcpcib_wdt_start(sc);
		} else {
			tcpcib_wdt_stop(sc);
		}
	}

	return true;
}

static int
tcpcib_wdt_setmode(struct sysmon_wdog *smw)
{
	struct tcpcib_softc *sc = smw->smw_cookie;
	unsigned int period;

	period = smw->smw_period;
	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		tcpcib_wdt_stop(sc);
	} else {
		/* 600 seconds is the maximum supported timeout value */
		if (period > 600)
			return EINVAL;

		tcpcib_wdt_stop(sc);
		tcpcib_wdt_init(sc, period);
		tcpcib_wdt_start(sc);
		tcpcib_wdt_tickle(smw);
	}

	return 0;
}

static int
tcpcib_wdt_tickle(struct sysmon_wdog *smw)
{
	struct tcpcib_softc *sc = smw->smw_cookie;

	/* Reset timer */
	tcpcib_wdt_unlock(sc);
	bus_space_write_1(sc->sc_wdt_iot, sc->sc_wdt_ioh,
	    E600_WDT_RR1, E600_WDT_RR1_RELOAD);

	return 0;
}
