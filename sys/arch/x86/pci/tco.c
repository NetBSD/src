/*	$NetBSD: tco.c,v 1.1.2.3 2015/09/22 12:05:54 skrll Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto and Matthew R. Green.
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

/*
 * Intel I/O Controller Hub (ICHn) watchdog timer
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tco.c,v 1.1.2.3 2015/09/22 12:05:54 skrll Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/timetc.h>
#include <sys/module.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/ic/i82801lpcreg.h>

#include <dev/sysmon/sysmonvar.h>

#include <arch/x86/pci/tco.h>

#include "pcibvar.h"

struct tco_softc{
	struct sysmon_wdog	sc_smw;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_space_tag_t		sc_rcbat;
	bus_space_handle_t	sc_rcbah;
	struct pcib_softc *	sc_pcib;
	int			sc_armed;
	unsigned int		sc_min_t;
	unsigned int		sc_max_t;
	int			sc_has_rcba;
};

static int tco_match(device_t, cfdata_t, void *);
static void tco_attach(device_t, device_t, void *);
static int tco_detach(device_t, int);

static bool tco_suspend(device_t, const pmf_qual_t *);

static int tcotimer_setmode(struct sysmon_wdog *);
static int tcotimer_tickle(struct sysmon_wdog *);
static void tcotimer_stop(struct tco_softc *);
static void tcotimer_start(struct tco_softc *);
static void tcotimer_status_reset(struct tco_softc *);
static int  tcotimer_disable_noreboot(device_t);

CFATTACH_DECL3_NEW(tco, sizeof(struct tco_softc),
    tco_match, tco_attach, tco_detach, NULL, NULL, NULL, 0);

/*
 * Autoconf callbacks.
 */
static int
tco_match(device_t parent, cfdata_t match, void *aux)
{
	struct lpcib_tco_attach_args *ta = aux;

	if (ta->ta_iot != 0)
		return 1;

	return 0;
}

static void
tco_attach(device_t parent, device_t self, void *aux)
{
	struct tco_softc *sc = device_private(self);
	struct lpcib_tco_attach_args *ta = aux;
	uint32_t ioreg;

	/* Retrieve bus info shared with parent/siblings */

	sc->sc_iot = ta->ta_iot;
	sc->sc_ioh = ta->ta_ioh;
	sc->sc_rcbat = ta->ta_rcbat;
	sc->sc_rcbah = ta->ta_rcbah;
	sc->sc_pcib = ta->ta_pcib;
	sc->sc_has_rcba = ta->ta_has_rcba;

	aprint_normal(": TCO (watchdog) timer configured.\n");
	aprint_naive("\n");

	/* Explicitly stop the TCO timer. */
	tcotimer_stop(sc);

	/*
	 * Enable TCO timeout SMI only if the hardware reset does not
	 * work. We don't know what the SMBIOS does.
	 */
	ioreg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, LPCIB_SMI_EN);
	ioreg &= ~LPCIB_SMI_EN_TCO_EN;

	/*
	 * Clear the No Reboot (NR) bit. If this fails, enabling the TCO_EN bit
	 * in the SMI_EN register is the last chance.
	 */
	if (tcotimer_disable_noreboot(self)) {
		ioreg |= LPCIB_SMI_EN_TCO_EN;
	}
	if ((ioreg & LPCIB_SMI_EN_GBL_SMI_EN) != 0) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, LPCIB_SMI_EN, ioreg);
	}

	/* Reset the watchdog status registers. */
	tcotimer_status_reset(sc);

	/*
	 * Register the driver with the sysmon watchdog framework.
	 */
	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = tcotimer_setmode;
	sc->sc_smw.smw_tickle = tcotimer_tickle;

	/*
	 * ICH6 or newer are limited to 2ticks min and 613ticks max.
	 *                              1sec           367secs
	 *
	 * ICH5 or older are limited to 4ticks min and 39ticks max.
	 *                              2secs          23secs
	 */
	if (sc->sc_has_rcba) {
		sc->sc_max_t = LPCIB_TCOTIMER2_MAX_TICK;
		sc->sc_min_t = LPCIB_TCOTIMER2_MIN_TICK;
	} else {
		sc->sc_max_t = LPCIB_TCOTIMER_MAX_TICK;
		sc->sc_min_t = LPCIB_TCOTIMER_MIN_TICK;
	}
	sc->sc_smw.smw_period = lpcib_tcotimer_tick_to_second(sc->sc_max_t);

	aprint_verbose_dev(self, "Min/Max interval %u/%u seconds\n",
		lpcib_tcotimer_tick_to_second(sc->sc_min_t),
		lpcib_tcotimer_tick_to_second(sc->sc_max_t));

	if (sysmon_wdog_register(&sc->sc_smw))
		aprint_error_dev(self, "unable to register TCO timer"
		       "as a sysmon watchdog device.\n");

	if (!pmf_device_register(self, tco_suspend, NULL))
		aprint_error_dev(self, "unable to register with pmf\n");
}

static int
tco_detach(device_t self, int flags)
{
	struct tco_softc *sc = device_private(self);
	int rc;

	if ((rc = sysmon_wdog_unregister(&sc->sc_smw)) != 0) {
		if (rc == ERESTART)
			rc = EINTR;
		return rc;
	}

	/* Explicitly stop the TCO timer. */
	tcotimer_stop(sc);

	/* XXX Set No Reboot? */

	pmf_device_deregister(self);

	return 0;
}

static bool
tco_suspend(device_t self, const pmf_qual_t *quals)
{
	struct tco_softc *sc = device_private(self);

	/* Allow suspend only if watchdog is not armed */

	return ((sc->sc_smw.smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED);
}

/*
 * Sysmon watchdog callbacks.
 */
static int
tcotimer_setmode(struct sysmon_wdog *smw)
{
	struct tco_softc *sc = smw->smw_cookie;
	unsigned int period;
	uint16_t ich6period = 0;
	uint8_t ich5period = 0;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		/* Stop the TCO timer. */
		tcotimer_stop(sc);
	} else {
		period = lpcib_tcotimer_second_to_tick(smw->smw_period);
		if (period < sc->sc_min_t || period > sc->sc_max_t)
			return EINVAL;

		/* Stop the TCO timer, */
		tcotimer_stop(sc);

		/* set the timeout, */
		if (sc->sc_has_rcba) {
			/* ICH6 or newer */
			ich6period = bus_space_read_2(sc->sc_iot, sc->sc_ioh,
						      LPCIB_TCO_TMR2);
			ich6period &= 0xfc00;
			bus_space_write_2(sc->sc_iot, sc->sc_ioh,
					  LPCIB_TCO_TMR2, ich6period | period);
		} else {
			/* ICH5 or older */
			ich5period = bus_space_read_1(sc->sc_iot, sc->sc_ioh,
						   LPCIB_TCO_TMR);
			ich5period &= 0xc0;
			bus_space_write_1(sc->sc_iot, sc->sc_ioh,
					  LPCIB_TCO_TMR, ich5period | period);
		}

		/* and start/reload the timer. */
		tcotimer_start(sc);
		tcotimer_tickle(smw);
	}

	return 0;
}

static int
tcotimer_tickle(struct sysmon_wdog *smw)
{
	struct tco_softc *sc = smw->smw_cookie;

	/* any value is allowed */
	if (sc->sc_has_rcba)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO_RLD, 1);
	else
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, LPCIB_TCO_RLD, 1);

	return 0;
}

static void
tcotimer_stop(struct tco_softc *sc)
{
	uint16_t ioreg;

	ioreg = bus_space_read_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO1_CNT);
	ioreg |= LPCIB_TCO1_CNT_TCO_TMR_HLT;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO1_CNT, ioreg);
}

static void
tcotimer_start(struct tco_softc *sc)
{
	uint16_t ioreg;

	ioreg = bus_space_read_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO1_CNT);
	ioreg &= ~LPCIB_TCO1_CNT_TCO_TMR_HLT;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO1_CNT, ioreg);
}

static void
tcotimer_status_reset(struct tco_softc *sc)
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO1_STS,
			  LPCIB_TCO1_STS_TIMEOUT);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO2_STS,
			  LPCIB_TCO2_STS_BOOT_STS);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO2_STS,
			  LPCIB_TCO2_STS_SECONDS_TO_STS);
}

/*
 * Clear the No Reboot (NR) bit, this enables reboots when the timer
 * reaches the timeout for the second time.
 */
static int
tcotimer_disable_noreboot(device_t self)
{
	struct tco_softc *sc = device_private(self);

	if (sc->sc_has_rcba) {
		uint32_t status;

		status = bus_space_read_4(sc->sc_rcbat, sc->sc_rcbah,
		    LPCIB_GCS_OFFSET);
		status &= ~LPCIB_GCS_NO_REBOOT;
		bus_space_write_4(sc->sc_rcbat, sc->sc_rcbah,
		    LPCIB_GCS_OFFSET, status);
		status = bus_space_read_4(sc->sc_rcbat, sc->sc_rcbah,
		    LPCIB_GCS_OFFSET);
		if (status & LPCIB_GCS_NO_REBOOT)
			goto error;
	} else {
		pcireg_t pcireg;

		pcireg = pci_conf_read(sc->sc_pcib->sc_pc, sc->sc_pcib->sc_tag,
				       LPCIB_PCI_GEN_STA);
		if (pcireg & LPCIB_PCI_GEN_STA_NO_REBOOT) {
			/* TCO timeout reset is disabled; try to enable it */
			pcireg &= ~LPCIB_PCI_GEN_STA_NO_REBOOT;
			pci_conf_write(sc->sc_pcib->sc_pc, sc->sc_pcib->sc_tag,
				       LPCIB_PCI_GEN_STA, pcireg);
			if (pcireg & LPCIB_PCI_GEN_STA_NO_REBOOT)
				goto error;
		}
	}

	return 0;
error:
	aprint_error_dev(self, "TCO timer reboot disabled by hardware; "
	    "hope SMBIOS properly handles it.\n");
	return EINVAL;
}

MODULE(MODULE_CLASS_DRIVER, tco, "sysmon_wdog");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
tco_modcmd(modcmd_t cmd, void *arg)
{
	int ret = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		ret = config_init_component(cfdriver_ioconf_tco,
					    cfattach_ioconf_tco,
					    cfdata_ioconf_tco);
#endif
		break;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		ret = config_fini_component(cfdriver_ioconf_tco,
					    cfattach_ioconf_tco,
					    cfdata_ioconf_tco);
#endif
		break;
	default:
		ret = ENOTTY;
	}

	return ret;
}
