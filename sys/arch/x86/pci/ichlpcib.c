/*	$NetBSD: ichlpcib.c,v 1.14.8.1 2009/05/13 17:18:44 jym Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
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
 * Intel I/O Controller Hub (ICHn) LPC Interface Bridge driver
 *
 *  LPC Interface Bridge is basically a pcib (PCI-ISA Bridge), but has
 *  some power management and monitoring functions.
 *  Currently we support the watchdog timer, SpeedStep (on some systems)
 *  and the power management timer.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ichlpcib.c,v 1.14.8.1 2009/05/13 17:18:44 jym Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/timetc.h>
#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/ic/acpipmtimer.h>
#include <dev/ic/i82801lpcreg.h>
#include <dev/ic/hpetreg.h>
#include <dev/ic/hpetvar.h>

#include "hpet.h"
#include "pcibvar.h"

struct lpcib_softc {
	/* we call pcibattach() which assumes this starts like this: */
	struct pcib_softc	sc_pcib;

	struct pci_attach_args	sc_pa;
	int			sc_has_rcba;
	int			sc_has_ich5_hpet;

	/* RCBA */
	bus_space_tag_t		sc_rcbat;
	bus_space_handle_t	sc_rcbah;
	pcireg_t		sc_rcba_reg;

	/* Watchdog variables. */
	struct sysmon_wdog	sc_smw;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

#if NHPET > 0
	/* HPET variables. */
	uint32_t		sc_hpet_reg;
#endif

	/* Speedstep */
	pcireg_t		sc_pmcon_orig;

	/* Power management */
	pcireg_t		sc_pirq[2];
	pcireg_t		sc_pmcon;
	pcireg_t		sc_fwhsel2;
};

static int lpcibmatch(device_t, cfdata_t, void *);
static void lpcibattach(device_t, device_t, void *);
static bool lpcib_suspend(device_t PMF_FN_PROTO);
static bool lpcib_resume(device_t PMF_FN_PROTO);
static bool lpcib_shutdown(device_t, int);

static void pmtimer_configure(device_t);

static void tcotimer_configure(device_t);
static int tcotimer_setmode(struct sysmon_wdog *);
static int tcotimer_tickle(struct sysmon_wdog *);
static void tcotimer_stop(struct lpcib_softc *);
static void tcotimer_start(struct lpcib_softc *);
static void tcotimer_status_reset(struct lpcib_softc *);
static int  tcotimer_disable_noreboot(device_t);

static void speedstep_configure(device_t);
static int speedstep_sysctl_helper(SYSCTLFN_ARGS);

#if NHPET > 0
static void lpcib_hpet_configure(device_t);
#endif

struct lpcib_softc *speedstep_cookie;	/* XXX */

CFATTACH_DECL_NEW(ichlpcib, sizeof(struct lpcib_softc),
    lpcibmatch, lpcibattach, NULL, NULL);

static struct lpcib_device {
	pcireg_t vendor, product;
	int has_rcba;
	int has_ich5_hpet;
} lpcib_devices[] = {
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801AA_LPC, 0, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801BA_LPC, 0, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801BAM_LPC, 0, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801CA_LPC, 0, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801CAM_LPC, 0, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801DB_LPC, 0, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801DB_ISA, 0, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801EB_LPC, 0, 1 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801FB_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801FBM_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801G_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801GBM_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801GHM_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801H_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801HEM_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801HH_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801HO_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801HBM_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801IH_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801IO_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801IR_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801IEM_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801IB_LPC, 1, 0 },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_63XXESB_LPC, 1, 0 }, 

	{ 0, 0, 0, 0 },
};

/*
 * Autoconf callbacks.
 */
static int
lpcibmatch(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct lpcib_device *lpcib_dev;

	/* We are ISA bridge, of course */
	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_BRIDGE ||
	    PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_BRIDGE_ISA)
		return 0;

	for (lpcib_dev = lpcib_devices; lpcib_dev->vendor; ++lpcib_dev) {
		if (PCI_VENDOR(pa->pa_id) == lpcib_dev->vendor &&
		    PCI_PRODUCT(pa->pa_id) == lpcib_dev->product)
			return 10;
	}

	return 0;
}

static void
lpcibattach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct lpcib_softc *sc = device_private(self);
	struct lpcib_device *lpcib_dev;

	sc->sc_pa = *pa;

	for (lpcib_dev = lpcib_devices; lpcib_dev->vendor; ++lpcib_dev) {
		if (PCI_VENDOR(pa->pa_id) != lpcib_dev->vendor ||
		    PCI_PRODUCT(pa->pa_id) != lpcib_dev->product)
			continue;
		sc->sc_has_rcba = lpcib_dev->has_rcba;
		sc->sc_has_ich5_hpet = lpcib_dev->has_ich5_hpet;
		break;
	}

	pcibattach(parent, self, aux);

	/*
	 * Part of our I/O registers are used as ACPI PM regs.
	 * Since our ACPI subsystem accesses the I/O space directly so far,
	 * we do not have to bother bus_space I/O map confliction.
	 */
	if (pci_mapreg_map(pa, LPCIB_PCI_PMBASE, PCI_MAPREG_TYPE_IO, 0,
			   &sc->sc_iot, &sc->sc_ioh, NULL, NULL)) {
		aprint_error_dev(self, "can't map power management i/o space");
		return;
	}

	sc->sc_pmcon_orig = pci_conf_read(sc->sc_pcib.sc_pc, sc->sc_pcib.sc_tag,
	    LPCIB_PCI_GEN_PMCON_1);

	/* For ICH6 and later, always enable RCBA */
	if (sc->sc_has_rcba) {
		pcireg_t rcba;

		sc->sc_rcbat = sc->sc_pa.pa_memt;

		rcba = pci_conf_read(sc->sc_pcib.sc_pc, sc->sc_pcib.sc_tag,
		     LPCIB_RCBA);
		if ((rcba & LPCIB_RCBA_EN) == 0) {
			aprint_error_dev(self, "RCBA is not enabled");
			return;
		}
		rcba &= ~LPCIB_RCBA_EN;

		if (bus_space_map(sc->sc_rcbat, rcba, LPCIB_RCBA_SIZE, 0,
				  &sc->sc_rcbah)) {
			aprint_error_dev(self, "RCBA could not be mapped");
			return;
		}
	}

	/* Set up the power management timer. */
	pmtimer_configure(self);

	/* Set up the TCO (watchdog). */
	tcotimer_configure(self);

	/* Set up SpeedStep. */
	speedstep_configure(self);

#if NHPET > 0
	/* Set up HPET. */
	lpcib_hpet_configure(self);
#endif

	/* Install power handler */
	if (!pmf_device_register1(self, lpcib_suspend, lpcib_resume,
	    lpcib_shutdown))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static bool
lpcib_shutdown(device_t dv, int howto)
{
	struct lpcib_softc *sc = device_private(dv);

	pci_conf_write(sc->sc_pcib.sc_pc, sc->sc_pcib.sc_tag,
	    LPCIB_PCI_GEN_PMCON_1, sc->sc_pmcon_orig);

	return true;
}

static bool
lpcib_suspend(device_t dv PMF_FN_ARGS)
{
	struct lpcib_softc *sc = device_private(dv);
	pci_chipset_tag_t pc = sc->sc_pcib.sc_pc;
	pcitag_t tag = sc->sc_pcib.sc_tag;

	/* capture PIRQ routing control registers */
	sc->sc_pirq[0] = pci_conf_read(pc, tag, LPCIB_PCI_PIRQA_ROUT);
	sc->sc_pirq[1] = pci_conf_read(pc, tag, LPCIB_PCI_PIRQE_ROUT);

	sc->sc_pmcon = pci_conf_read(pc, tag, LPCIB_PCI_GEN_PMCON_1);
	sc->sc_fwhsel2 = pci_conf_read(pc, tag, LPCIB_PCI_GEN_STA);

	if (sc->sc_has_rcba) {
		sc->sc_rcba_reg = pci_conf_read(pc, tag, LPCIB_RCBA);
#if NHPET > 0
		sc->sc_hpet_reg = bus_space_read_4(sc->sc_rcbat, sc->sc_rcbah,
		    LPCIB_RCBA_HPTC);
#endif
	} else if (sc->sc_has_ich5_hpet) {
#if NHPET > 0
		sc->sc_hpet_reg = pci_conf_read(pc, tag, LPCIB_PCI_GEN_CNTL);
#endif
	}

	return true;
}

static bool
lpcib_resume(device_t dv PMF_FN_ARGS)
{
	struct lpcib_softc *sc = device_private(dv);
	pci_chipset_tag_t pc = sc->sc_pcib.sc_pc;
	pcitag_t tag = sc->sc_pcib.sc_tag;

	/* restore PIRQ routing control registers */
	pci_conf_write(pc, tag, LPCIB_PCI_PIRQA_ROUT, sc->sc_pirq[0]);
	pci_conf_write(pc, tag, LPCIB_PCI_PIRQE_ROUT, sc->sc_pirq[1]);

	pci_conf_write(pc, tag, LPCIB_PCI_GEN_PMCON_1, sc->sc_pmcon);
	pci_conf_write(pc, tag, LPCIB_PCI_GEN_STA, sc->sc_fwhsel2);

	if (sc->sc_has_rcba) {
		pci_conf_write(pc, tag, LPCIB_RCBA, sc->sc_rcba_reg);
#if NHPET > 0
		bus_space_write_4(sc->sc_rcbat, sc->sc_rcbah, LPCIB_RCBA_HPTC,
		    sc->sc_hpet_reg);
#endif
	} else if (sc->sc_has_ich5_hpet) {
#if NHPET > 0
		pci_conf_write(pc, tag, LPCIB_PCI_GEN_CNTL, sc->sc_hpet_reg);
#endif
	}

	return true;
}

/*
 * Initialize the power management timer.
 */
static void
pmtimer_configure(device_t self)
{
	struct lpcib_softc *sc = device_private(self);
	pcireg_t control;

	/* 
	 * Check if power management I/O space is enabled and enable the ACPI_EN
	 * bit if it's disabled.
	 */
	control = pci_conf_read(sc->sc_pcib.sc_pc, sc->sc_pcib.sc_tag,
	    LPCIB_PCI_ACPI_CNTL);
	if ((control & LPCIB_PCI_ACPI_CNTL_EN) == 0) {
		control |= LPCIB_PCI_ACPI_CNTL_EN;
		pci_conf_write(sc->sc_pcib.sc_pc, sc->sc_pcib.sc_tag,
		    LPCIB_PCI_ACPI_CNTL, control);
	}

	/* Attach our PM timer with the generic acpipmtimer function */
	acpipmtimer_attach(self, sc->sc_iot, sc->sc_ioh,
	    LPCIB_PM1_TMR, 0);
}

/*
 * Initialize the watchdog timer.
 */
static void
tcotimer_configure(device_t self)
{
	struct lpcib_softc *sc = device_private(self);
	uint32_t ioreg;
	unsigned int period;

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
	if (sc->sc_has_rcba)
		period = LPCIB_TCOTIMER2_MAX_TICK;
	else
		period = LPCIB_TCOTIMER_MAX_TICK;
	sc->sc_smw.smw_period = lpcib_tcotimer_tick_to_second(period);

	if (sysmon_wdog_register(&sc->sc_smw)) {
		aprint_error_dev(self, "unable to register TCO timer"
		       "as a sysmon watchdog device.\n");
		return;
	}

	aprint_verbose_dev(self, "TCO (watchdog) timer configured.\n");
}

/*
 * Sysmon watchdog callbacks.
 */
static int
tcotimer_setmode(struct sysmon_wdog *smw)
{
	struct lpcib_softc *sc = smw->smw_cookie;
	unsigned int period;
	uint16_t ich6period = 0;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		/* Stop the TCO timer. */
		tcotimer_stop(sc);
	} else {
		/* 
		 * ICH6 or newer are limited to 2s min and 613s max.
		 * ICH5 or older are limited to 4s min and 39s max.
		 */
		if (sc->sc_has_rcba) {
			if (smw->smw_period < LPCIB_TCOTIMER2_MIN_TICK ||
			    smw->smw_period > LPCIB_TCOTIMER2_MAX_TICK)
				return EINVAL;
		} else {
			if (smw->smw_period < LPCIB_TCOTIMER_MIN_TICK ||
			    smw->smw_period > LPCIB_TCOTIMER_MAX_TICK)
				return EINVAL;
		}
		period = lpcib_tcotimer_second_to_tick(smw->smw_period);
		
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
			period |= bus_space_read_1(sc->sc_iot, sc->sc_ioh,
						   LPCIB_TCO_TMR);
			period &= 0xc0;
			bus_space_write_1(sc->sc_iot, sc->sc_ioh,
					  LPCIB_TCO_TMR, period);
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
	struct lpcib_softc *sc = smw->smw_cookie;

	/* any value is allowed */
	if (sc->sc_has_rcba)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO_RLD, 1);
	else
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, LPCIB_TCO_RLD, 1);

	return 0;
}

static void
tcotimer_stop(struct lpcib_softc *sc)
{
	uint16_t ioreg;

	ioreg = bus_space_read_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO1_CNT);
	ioreg |= LPCIB_TCO1_CNT_TCO_TMR_HLT;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO1_CNT, ioreg);
}

static void
tcotimer_start(struct lpcib_softc *sc)
{
	uint16_t ioreg;

	ioreg = bus_space_read_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO1_CNT);
	ioreg &= ~LPCIB_TCO1_CNT_TCO_TMR_HLT;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO1_CNT, ioreg);
}

static void
tcotimer_status_reset(struct lpcib_softc *sc)
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
	struct lpcib_softc *sc = device_private(self);

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

		pcireg = pci_conf_read(sc->sc_pcib.sc_pc, sc->sc_pcib.sc_tag, 
				       LPCIB_PCI_GEN_STA);
		if (pcireg & LPCIB_PCI_GEN_STA_NO_REBOOT) {
			/* TCO timeout reset is disabled; try to enable it */
			pcireg &= ~LPCIB_PCI_GEN_STA_NO_REBOOT;
			pci_conf_write(sc->sc_pcib.sc_pc, sc->sc_pcib.sc_tag,
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


/*
 * Intel ICH SpeedStep support.
 */
#define SS_READ(sc, reg) \
	bus_space_read_1((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define SS_WRITE(sc, reg, val) \
	bus_space_write_1((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

/*
 * Linux driver says that SpeedStep on older chipsets cause
 * lockups on Dell Inspiron 8000 and 8100.
 * It should also not be enabled on systems with the 82855GM
 * Hub, which typically have an EST-enabled CPU.
 */
static int
speedstep_bad_hb_check(struct pci_attach_args *pa)
{

	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82815_FULL_HUB &&
	    PCI_REVISION(pa->pa_class) < 5)
		return 1;

	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82855GM_MCH)
		return 1;

	return 0;
}

static void
speedstep_configure(device_t self)
{
	struct lpcib_softc *sc = device_private(self);
	const struct sysctlnode	*node, *ssnode;
	int rv;

	/* Supported on ICH2-M, ICH3-M and ICH4-M.  */
	if (PCI_PRODUCT(sc->sc_pa.pa_id) == PCI_PRODUCT_INTEL_82801DB_ISA ||
	    PCI_PRODUCT(sc->sc_pa.pa_id) == PCI_PRODUCT_INTEL_82801CAM_LPC ||
	    (PCI_PRODUCT(sc->sc_pa.pa_id) == PCI_PRODUCT_INTEL_82801BAM_LPC &&
	     pci_find_device(&sc->sc_pa, speedstep_bad_hb_check) == 0)) {
		uint8_t pmcon;

		/* Enable SpeedStep if it isn't already enabled. */
		pmcon = pci_conf_read(sc->sc_pcib.sc_pc, sc->sc_pcib.sc_tag,
				      LPCIB_PCI_GEN_PMCON_1);
		if ((pmcon & LPCIB_PCI_GEN_PMCON_1_SS_EN) == 0)
			pci_conf_write(sc->sc_pcib.sc_pc, sc->sc_pcib.sc_tag,
				       LPCIB_PCI_GEN_PMCON_1,
				       pmcon | LPCIB_PCI_GEN_PMCON_1_SS_EN);

		/* Put in machdep.speedstep_state (0 for low, 1 for high). */
		if ((rv = sysctl_createv(NULL, 0, NULL, &node,
		    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
		    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL)) != 0)
			goto err;

		/* CTLFLAG_ANYWRITE? kernel option like EST? */
		if ((rv = sysctl_createv(NULL, 0, &node, &ssnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT, "speedstep_state", NULL,
		    speedstep_sysctl_helper, 0, NULL, 0, CTL_CREATE,
		    CTL_EOL)) != 0)
			goto err;

		/* XXX save the sc for IO tag/handle */
		speedstep_cookie = sc;
		aprint_verbose_dev(self, "SpeedStep enabled\n");
	}

	return;

err:
	aprint_normal("%s: sysctl_createv failed (rv = %d)\n", __func__, rv);
}

/*
 * get/set the SpeedStep state: 0 == low power, 1 == high power.
 */
static int
speedstep_sysctl_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode	node;
	struct lpcib_softc 	*sc = speedstep_cookie;
	uint8_t			state, state2;
	int			ostate, nstate, s, error = 0;

	/*
	 * We do the dance with spl's to avoid being at high ipl during
	 * sysctl_lookup() which can both copyin and copyout.
	 */
	s = splserial();
	state = SS_READ(sc, LPCIB_PM_SS_CNTL);
	splx(s);
	if ((state & LPCIB_PM_SS_STATE_LOW) == 0)
		ostate = 1;
	else
		ostate = 0;
	nstate = ostate;

	node = *rnode;
	node.sysctl_data = &nstate;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto out;

	/* Only two states are available */
	if (nstate != 0 && nstate != 1) {
		error = EINVAL;
		goto out;
	}

	s = splserial();
	state2 = SS_READ(sc, LPCIB_PM_SS_CNTL);
	if ((state2 & LPCIB_PM_SS_STATE_LOW) == 0)
		ostate = 1;
	else
		ostate = 0;

	if (ostate != nstate) {
		uint8_t cntl;

		if (nstate == 0)
			state2 |= LPCIB_PM_SS_STATE_LOW;
		else
			state2 &= ~LPCIB_PM_SS_STATE_LOW;

		/*
		 * Must disable bus master arbitration during the change.
		 */
		cntl = SS_READ(sc, LPCIB_PM_CTRL);
		SS_WRITE(sc, LPCIB_PM_CTRL, cntl | LPCIB_PM_SS_CNTL_ARB_DIS);
		SS_WRITE(sc, LPCIB_PM_SS_CNTL, state2);
		SS_WRITE(sc, LPCIB_PM_CTRL, cntl);
	}
	splx(s);
out:
	return error;
}

#if NHPET > 0
struct lpcib_hpet_attach_arg {
	bus_space_tag_t hpet_mem_t;
	uint32_t hpet_reg;
};

static int
lpcib_hpet_match(device_t parent, cfdata_t match, void *aux)
{
	struct lpcib_hpet_attach_arg *arg = aux;
	bus_space_tag_t tag;
	bus_space_handle_t handle;

	tag = arg->hpet_mem_t;

	if (bus_space_map(tag, arg->hpet_reg, HPET_WINDOW_SIZE, 0, &handle)) {
		aprint_verbose_dev(parent, "HPET window not mapped, skipping\n");
		return 0;
	}
	bus_space_unmap(tag, handle, HPET_WINDOW_SIZE);

	return 1;
}

static void
lpcib_hpet_attach(device_t parent, device_t self, void *aux)
{
	struct hpet_softc *sc = device_private(self);
	struct lpcib_hpet_attach_arg *arg = aux;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_memt = arg->hpet_mem_t;

	if (bus_space_map(sc->sc_memt, arg->hpet_reg, HPET_WINDOW_SIZE, 0,
			  &sc->sc_memh)) {
		aprint_error_dev(self,
		    "HPET memory window could not be mapped");
		return;
	}

	hpet_attach_subr(self);
}

CFATTACH_DECL_NEW(ichlpcib_hpet, sizeof(struct hpet_softc), lpcib_hpet_match,
    lpcib_hpet_attach, NULL, NULL);

static void
lpcib_hpet_configure(device_t self)
{
	struct lpcib_softc *sc = device_private(self);
	struct lpcib_hpet_attach_arg arg;
	uint32_t hpet_reg, val;

	if (sc->sc_has_ich5_hpet) {
		val = pci_conf_read(sc->sc_pcib.sc_pc, sc->sc_pcib.sc_tag,
		    LPCIB_PCI_GEN_CNTL);
		switch (val & LPCIB_ICH5_HPTC_WIN_MASK) {
		case LPCIB_ICH5_HPTC_0000:
			hpet_reg = LPCIB_ICH5_HPTC_0000_BASE;
			break;
		case LPCIB_ICH5_HPTC_1000:
			hpet_reg = LPCIB_ICH5_HPTC_1000_BASE;
			break;
		case LPCIB_ICH5_HPTC_2000:
			hpet_reg = LPCIB_ICH5_HPTC_2000_BASE;
			break;
		case LPCIB_ICH5_HPTC_3000:
			hpet_reg = LPCIB_ICH5_HPTC_3000_BASE;
			break;
		default:
			return;
		}
		val |= sc->sc_hpet_reg | LPCIB_ICH5_HPTC_EN;
		pci_conf_write(sc->sc_pcib.sc_pc, sc->sc_pcib.sc_tag,
		    LPCIB_PCI_GEN_CNTL, val);
	} else if (sc->sc_has_rcba) {
		val = bus_space_read_4(sc->sc_rcbat, sc->sc_rcbah,
		    LPCIB_RCBA_HPTC);
		switch (val & LPCIB_RCBA_HPTC_WIN_MASK) {
		case LPCIB_RCBA_HPTC_0000:
			hpet_reg = LPCIB_RCBA_HPTC_0000_BASE;
			break;
		case LPCIB_RCBA_HPTC_1000:
			hpet_reg = LPCIB_RCBA_HPTC_1000_BASE;
			break;
		case LPCIB_RCBA_HPTC_2000:
			hpet_reg = LPCIB_RCBA_HPTC_2000_BASE;
			break;
		case LPCIB_RCBA_HPTC_3000:
			hpet_reg = LPCIB_RCBA_HPTC_3000_BASE;
			break;
		default:
			return;
		}
		val |= LPCIB_RCBA_HPTC_EN;
		bus_space_write_4(sc->sc_rcbat, sc->sc_rcbah, LPCIB_RCBA_HPTC,
		    val);
	} else {
		/* No HPET here */
		return;
	}

	arg.hpet_mem_t = sc->sc_pa.pa_memt;
	arg.hpet_reg = hpet_reg;

	config_found_ia(self, "hpetichbus", &arg, NULL);
}
#endif
