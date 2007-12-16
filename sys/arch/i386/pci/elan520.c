/*	$NetBSD: elan520.c,v 1.19 2007/12/16 21:14:22 dyoung Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Device driver for the AMD Elan SC520 System Controller.  This attaches
 * where the "pchb" driver might normally attach, and provides support for
 * extra features on the SC520, such as the watchdog timer and GPIO.
 *
 * Information about the GP bus echo bug work-around is from code posted
 * to the "soekris-tech" mailing list by Jasper Wallace.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: elan520.c,v 1.19 2007/12/16 21:14:22 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/mutex.h>
#include <sys/wdog.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>

#include <dev/pci/pcidevs.h>

#include "gpio.h"
#if NGPIO > 0
#include <dev/gpio/gpiovar.h>
#endif

#include <arch/i386/pci/elan520reg.h>

#include <dev/sysmon/sysmonvar.h>

struct elansc_softc {
	struct device sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	int sc_echobug;

	kmutex_t sc_mtx;

	bool sc_suspended;
	struct sysmon_wdog sc_smw;
#if NGPIO > 0
	/* GPIO interface */
	struct gpio_chipset_tag sc_gpio_gc;
	gpio_pin_t sc_gpio_pins[ELANSC_PIO_NPINS];
#endif
};

#if NGPIO > 0
static int	elansc_gpio_pin_read(void *, int);
static void	elansc_gpio_pin_write(void *, int, int);
static void	elansc_gpio_pin_ctl(void *, int, int);
#endif

static void
elansc_childdetached(device_t self, device_t child)
{
	/* elansc does not presently keep a pointer to children such
	 * as the gpio, so there is nothing to do.
	 */
}

static void
elansc_wdogctl_write(struct elansc_softc *sc, uint16_t val)
{
	uint8_t echo_mode = 0; /* XXX: gcc */

	KASSERT(mutex_owned(&sc->sc_mtx));

	/* Switch off GP bus echo mode if we need to. */
	if (sc->sc_echobug) {
		echo_mode = bus_space_read_1(sc->sc_memt, sc->sc_memh,
		    MMCR_GPECHO);
		bus_space_write_1(sc->sc_memt, sc->sc_memh,
		    MMCR_GPECHO, echo_mode & ~GPECHO_GP_ECHO_ENB);
	}

	/* Unlock the register. */
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_WDTMRCTL,
	    WDTMRCTL_UNLOCK1);
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_WDTMRCTL,
	    WDTMRCTL_UNLOCK2);

	/* Write the value. */
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_WDTMRCTL, val);

	/* Switch GP bus echo mode back. */
	if (sc->sc_echobug)
		bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_GPECHO,
		    echo_mode);
}

static void
elansc_wdogctl_reset(struct elansc_softc *sc)
{
	uint8_t echo_mode = 0/* XXX: gcc */;

	KASSERT(mutex_owned(&sc->sc_mtx));

	/* Switch off GP bus echo mode if we need to. */
	if (sc->sc_echobug) {
		echo_mode = bus_space_read_1(sc->sc_memt, sc->sc_memh, 
		    MMCR_GPECHO); 
		bus_space_write_1(sc->sc_memt, sc->sc_memh,
		    MMCR_GPECHO, echo_mode & ~GPECHO_GP_ECHO_ENB); 
	}

	/* Reset the watchdog. */
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_WDTMRCTL,
	    WDTMRCTL_RESET1);
	bus_space_write_2(sc->sc_memt, sc->sc_memh, MMCR_WDTMRCTL,
	    WDTMRCTL_RESET2);

	/* Switch GP bus echo mode back. */
	if (sc->sc_echobug)
		bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_GPECHO,
		    echo_mode);
}

static const struct {
	int	period;		/* whole seconds */
	uint16_t exp;		/* exponent select */
} elansc_wdog_periods[] = {
	{ 1,	WDTMRCTL_EXP_SEL25 },
	{ 2,	WDTMRCTL_EXP_SEL26 },
	{ 4,	WDTMRCTL_EXP_SEL27 },
	{ 8,	WDTMRCTL_EXP_SEL28 },
	{ 16,	WDTMRCTL_EXP_SEL29 },
	{ 32,	WDTMRCTL_EXP_SEL30 },
	{ 0,	0 },
};

static int
elansc_wdog_arm(struct elansc_softc *sc)
{
	struct sysmon_wdog *smw = &sc->sc_smw;
	int i;
	uint16_t exp_sel = 0; /* XXX: gcc */

	KASSERT(mutex_owned(&sc->sc_mtx));

	if (smw->smw_period == WDOG_PERIOD_DEFAULT) {
		smw->smw_period = 32;
		exp_sel = WDTMRCTL_EXP_SEL30;
	} else {
		for (i = 0; elansc_wdog_periods[i].period != 0; i++) {
			if (elansc_wdog_periods[i].period ==
			    smw->smw_period) {
				exp_sel = elansc_wdog_periods[i].exp;
				break;
			}
		}
		if (elansc_wdog_periods[i].period == 0)
			return EINVAL;
	}
	elansc_wdogctl_write(sc, WDTMRCTL_ENB |
	    WDTMRCTL_WRST_ENB | exp_sel);
	elansc_wdogctl_reset(sc);
	return 0;
}

static int
elansc_wdog_setmode(struct sysmon_wdog *smw)
{
	struct elansc_softc *sc = smw->smw_cookie;
	int rc = 0;

	mutex_enter(&sc->sc_mtx);

	if (!device_is_active(&sc->sc_dev))
		rc = ENXIO;
	else if (!device_has_power(&sc->sc_dev) || sc->sc_suspended)
		rc = EBUSY;
	else if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		elansc_wdogctl_write(sc,
		    WDTMRCTL_WRST_ENB | WDTMRCTL_EXP_SEL30);
	} else
		rc = elansc_wdog_arm(sc);

	mutex_exit(&sc->sc_mtx);
	return rc;
}

static int
elansc_wdog_tickle(struct sysmon_wdog *smw)
{
	struct elansc_softc *sc = smw->smw_cookie;

	mutex_enter(&sc->sc_mtx);
	elansc_wdogctl_reset(sc);
	mutex_exit(&sc->sc_mtx);
	return 0;
}

static int
elansc_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_AMD &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_AMD_SC520_SC)
		return (10);	/* beat pchb */

	return (0);
}

static const char *elansc_speeds[] = {
	"(reserved 00)",
	"100MHz",
	"133MHz",
	"(reserved 11)",
};

static bool
elansc_suspend(device_t dev)
{
	bool rc;
	struct elansc_softc *sc = device_private(dev);

	mutex_enter(&sc->sc_mtx);
	rc = ((sc->sc_smw.smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED);
	if (rc)
		sc->sc_suspended = true;
	mutex_exit(&sc->sc_mtx);
	if (!rc)
		aprint_debug_dev(dev, "watchdog enabled, suspend forbidden");
	return rc;
}

static bool
elansc_resume(device_t dev)
{
	struct elansc_softc *sc = device_private(dev);

	mutex_enter(&sc->sc_mtx);
	sc->sc_suspended = false;
	/* Set up the watchdog registers with some defaults. */
	elansc_wdogctl_write(sc, WDTMRCTL_WRST_ENB | WDTMRCTL_EXP_SEL30);

	/* ...and clear it. */
	elansc_wdogctl_reset(sc);
	mutex_exit(&sc->sc_mtx);

	return true;
}

static int
elansc_detach(device_t self, int flags)
{
	int rc;
	struct elansc_softc *sc = device_private(self);

	if ((rc = config_detach_children(self, flags)) != 0)
		return rc;

	pmf_device_deregister(self);

	if ((rc = sysmon_wdog_unregister(&sc->sc_smw)) != 0) {
		if (rc == ERESTART)
			rc = EINTR;
		return rc;
	}

	mutex_enter(&sc->sc_mtx);

	/* Set up the watchdog registers with some defaults. */
	elansc_wdogctl_write(sc, WDTMRCTL_WRST_ENB | WDTMRCTL_EXP_SEL30);

	/* ...and clear it. */
	elansc_wdogctl_reset(sc);

	bus_space_unmap(sc->sc_memt, sc->sc_memh, PAGE_SIZE);

	mutex_exit(&sc->sc_mtx);
	mutex_destroy(&sc->sc_mtx);
	return 0;
}

static void
elansc_attach(struct device *parent, struct device *self, void *aux)
{
	struct elansc_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	uint16_t rev;
	uint8_t ressta, cpuctl;
#if NGPIO > 0
	struct gpiobus_attach_args gba;
	int pin;
	int reg, shift;
	uint16_t data;
#endif

	aprint_naive(": System Controller\n");
	aprint_normal(": AMD Elan SC520 System Controller\n");

	sc->sc_memt = pa->pa_memt;
	if (bus_space_map(sc->sc_memt, MMCR_BASE_ADDR, PAGE_SIZE, 0,
	    &sc->sc_memh) != 0) {
		aprint_error("%s: unable to map registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_HIGH);

	rev = bus_space_read_2(sc->sc_memt, sc->sc_memh, MMCR_REVID);
	cpuctl = bus_space_read_1(sc->sc_memt, sc->sc_memh, MMCR_CPUCTL);

	aprint_normal("%s: product %d stepping %d.%d, CPU clock %s\n",
	    sc->sc_dev.dv_xname,
	    (rev & REVID_PRODID) >> REVID_PRODID_SHIFT,
	    (rev & REVID_MAJSTEP) >> REVID_MAJSTEP_SHIFT,
	    (rev & REVID_MINSTEP),
	    elansc_speeds[cpuctl & CPUCTL_CPU_CLK_SPD_MASK]);

	/*
	 * SC520 rev A1 has a bug that affects the watchdog timer.  If
	 * the GP bus echo mode is enabled, writing to the watchdog control
	 * register is blocked.
	 *
	 * The BIOS in some systems (e.g. the Soekris net4501) enables
	 * GP bus echo for various reasons, so we need to switch it off
	 * when we talk to the watchdog timer.
	 *
	 * XXX The step 1.1 (B1?) in my Soekris net4501 also has this
	 * XXX problem, so we'll just enable it for all Elan SC520s
	 * XXX for now.  --thorpej@NetBSD.org
	 */
	if (1 || rev == ((PRODID_ELAN_SC520 << REVID_PRODID_SHIFT) |
		    (0 << REVID_MAJSTEP_SHIFT) | (1)))
		sc->sc_echobug = 1;

	/*
	 * Determine cause of the last reset, and issue a warning if it
	 * was due to watchdog expiry.
	 */
	ressta = bus_space_read_1(sc->sc_memt, sc->sc_memh, MMCR_RESSTA);
	if (ressta & RESSTA_WDT_RST_DET)
		aprint_error(
		    "%s: WARNING: LAST RESET DUE TO WATCHDOG EXPIRATION!\n",
		    sc->sc_dev.dv_xname);
	bus_space_write_1(sc->sc_memt, sc->sc_memh, MMCR_RESSTA, ressta);

	/* Set up the watchdog registers with some defaults. */
	elansc_wdogctl_write(sc, WDTMRCTL_WRST_ENB | WDTMRCTL_EXP_SEL30);

	/* ...and clear it. */
	elansc_wdogctl_reset(sc);

	pmf_device_register(self, elansc_suspend, elansc_resume);

#if NGPIO > 0
	/* Initialize GPIO pins array */
	for (pin = 0; pin < ELANSC_PIO_NPINS; pin++) {
		sc->sc_gpio_pins[pin].pin_num = pin;
		sc->sc_gpio_pins[pin].pin_caps = GPIO_PIN_INPUT |
		    GPIO_PIN_OUTPUT;

		/* Read initial state */
		reg = (pin < 16 ? MMCR_PIODIR15_0 : MMCR_PIODIR31_16);
		shift = pin % 16;
		data = bus_space_read_2(sc->sc_memt, sc->sc_memh, reg);
		if ((data & (1 << shift)) == 0)
			sc->sc_gpio_pins[pin].pin_flags = GPIO_PIN_INPUT;
		else
			sc->sc_gpio_pins[pin].pin_flags = GPIO_PIN_OUTPUT;
		if (elansc_gpio_pin_read(sc, pin) == 0)
			sc->sc_gpio_pins[pin].pin_state = GPIO_PIN_LOW;
		else
			sc->sc_gpio_pins[pin].pin_state = GPIO_PIN_HIGH;
	}

	/* Create controller tag */
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = elansc_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = elansc_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = elansc_gpio_pin_ctl;

	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = ELANSC_PIO_NPINS;

	/* Attach GPIO framework */
	config_found_ia(&sc->sc_dev, "gpiobus", &gba, gpiobus_print);
#endif /* NGPIO */

	/*
	 * Hook up the watchdog timer.
	 */
	sc->sc_smw.smw_name = sc->sc_dev.dv_xname;
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = elansc_wdog_setmode;
	sc->sc_smw.smw_tickle = elansc_wdog_tickle;
	sc->sc_smw.smw_period = 32;	/* actually 32.54 */
	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error("%s: unable to register watchdog with sysmon\n",
		    sc->sc_dev.dv_xname);
}

CFATTACH_DECL2(elansc, sizeof(struct elansc_softc),
    elansc_match, elansc_attach, elansc_detach, NULL, NULL,
    elansc_childdetached);

#if NGPIO > 0
static int
elansc_gpio_pin_read(void *arg, int pin)
{
	struct elansc_softc *sc = arg;
	int reg, shift;
	uint16_t data;

	reg = (pin < 16 ? MMCR_PIODATA15_0 : MMCR_PIODATA31_16);
	shift = pin % 16;

	mutex_enter(&sc->sc_mtx);
	data = bus_space_read_2(sc->sc_memt, sc->sc_memh, reg);
	mutex_exit(&sc->sc_mtx);

	return ((data >> shift) & 0x1);
}

static void
elansc_gpio_pin_write(void *arg, int pin, int value)
{
	struct elansc_softc *sc = arg;
	int reg, shift;
	uint16_t data;

	reg = (pin < 16 ? MMCR_PIODATA15_0 : MMCR_PIODATA31_16);
	shift = pin % 16;

	mutex_enter(&sc->sc_mtx);
	data = bus_space_read_2(sc->sc_memt, sc->sc_memh, reg);
	if (value == 0)
		data &= ~(1 << shift);
	else if (value == 1)
		data |= (1 << shift);

	bus_space_write_2(sc->sc_memt, sc->sc_memh, reg, data);
	mutex_exit(&sc->sc_mtx);
}

static void
elansc_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct elansc_softc *sc = arg;
	int reg, shift;
	uint16_t data;

	reg = (pin < 16 ? MMCR_PIODIR15_0 : MMCR_PIODIR31_16);
	shift = pin % 16;
	mutex_enter(&sc->sc_mtx);
	data = bus_space_read_2(sc->sc_memt, sc->sc_memh, reg);
	if (flags & GPIO_PIN_INPUT)
		data &= ~(1 << shift);
	if (flags & GPIO_PIN_OUTPUT)
		data |= (1 << shift);

	bus_space_write_2(sc->sc_memt, sc->sc_memh, reg, data);
	mutex_exit(&sc->sc_mtx);
}
#endif /* NGPIO */
