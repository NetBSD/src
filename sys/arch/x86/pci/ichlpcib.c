/*	$NetBSD: ichlpcib.c,v 1.5 2007/11/23 11:21:14 xtraeme Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * Intel I/O Controller Hub (ICHn) LPC Interface Bridge driver
 *
 *  LPC Interface Bridge is basically a pcib (PCI-ISA Bridge), but has
 *  some power management and monitoring functions.
 *  Currently we support the watchdog timer, SpeedStep (on some systems)
 *  and the power management timer.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ichlpcib.c,v 1.5 2007/11/23 11:21:14 xtraeme Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/ic/i82801lpcreg.h>
#include <dev/ic/acpipmtimer.h>

struct lpcib_softc {
	/* Device object. */
	struct device		sc_dev;

	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_pcitag;

	/* Watchdog variables. */
	struct sysmon_wdog	sc_smw;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	
	/* Power management */
	void			*sc_powerhook;
	struct pci_conf_state	sc_pciconf;
	pcireg_t		sc_pirq[8];
};

static int lpcibmatch(struct device *, struct cfdata *, void *);
static void lpcibattach(struct device *, struct device *, void *);
static void lpcib_powerhook(int, void *);

static void pmtimer_configure(struct lpcib_softc *, struct pci_attach_args *);

static void tcotimer_configure(struct lpcib_softc *, struct pci_attach_args *);
static int tcotimer_setmode(struct sysmon_wdog *);
static int tcotimer_tickle(struct sysmon_wdog *);
static void tcotimer_stop(struct lpcib_softc *);
static void tcotimer_start(struct lpcib_softc *);
static void tcotimer_status_reset(struct lpcib_softc *);
static int  tcotimer_disable_noreboot(struct lpcib_softc *, bus_space_tag_t,
				      bus_space_handle_t);

static void speedstep_configure(struct lpcib_softc *, struct pci_attach_args *);
static int speedstep_sysctl_helper(SYSCTLFN_ARGS);

struct lpcib_softc *speedstep_cookie;	/* XXX */
static int lpcib_ich6 = 0;

/* Defined in arch/.../pci/pcib.c. */
extern void pcibattach(struct device *, struct device *, void *);

CFATTACH_DECL(ichlpcib, sizeof(struct lpcib_softc),
    lpcibmatch, lpcibattach, NULL, NULL);

/*
 * Autoconf callbacks.
 */
static int
lpcibmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	/* We are ISA bridge, of course */
	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_BRIDGE ||
	    PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_BRIDGE_ISA)
		return 0;

	/* Matches only Intel ICH */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL) {
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_82801AA_LPC:	/* ICH */
		case PCI_PRODUCT_INTEL_82801AB_LPC:	/* ICH0 */
		case PCI_PRODUCT_INTEL_82801BA_LPC:	/* ICH2 */
		case PCI_PRODUCT_INTEL_82801BAM_LPC:	/* ICH2-M */
		case PCI_PRODUCT_INTEL_82801CA_LPC:	/* ICH3-S */
		case PCI_PRODUCT_INTEL_82801CAM_LPC:	/* ICH3-M */
		case PCI_PRODUCT_INTEL_82801DB_LPC:	/* ICH4 */
		case PCI_PRODUCT_INTEL_82801DB_ISA:	/* ICH4-M */
		case PCI_PRODUCT_INTEL_82801EB_LPC:	/* ICH5 */
			return 10;
		case PCI_PRODUCT_INTEL_82801FB_LPC:	/* ICH6 */
		case PCI_PRODUCT_INTEL_82801FBM_LPC:	/* ICH6-M */
		case PCI_PRODUCT_INTEL_82801G_LPC:	/* ICH7 */
		case PCI_PRODUCT_INTEL_82801GBM_LPC:	/* ICH7-M */
		case PCI_PRODUCT_INTEL_82801GHM_LPC:	/* ICH7-M DH */
		case PCI_PRODUCT_INTEL_82801H_LPC:	/* ICH8 */
		case PCI_PRODUCT_INTEL_82801HH_LPC:	/* ICH8 DH */
		case PCI_PRODUCT_INTEL_82801HO_LPC:	/* ICH8 DO */
		case PCI_PRODUCT_INTEL_82801HBM_LPC:    /* iCH8-M */
		case PCI_PRODUCT_INTEL_82801IH_LPC:	/* ICH9 */
		case PCI_PRODUCT_INTEL_82801IR_LPC:	/* ICH9-R */
		case PCI_PRODUCT_INTEL_82801IB_LPC:	/* ICH9 ? */
			lpcib_ich6 = 1;
			return 10;	/* prior to pcib */
		}
	}

	return 0;
}

static void
lpcibattach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct lpcib_softc *sc = (void*) self;

	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;

	pcibattach(parent, self, aux);

	/*
	 * Part of our I/O registers are used as ACPI PM regs.
	 * Since our ACPI subsystem accesses the I/O space directly so far,
	 * we do not have to bother bus_space I/O map confliction.
	 */
	if (pci_mapreg_map(pa, LPCIB_PCI_PMBASE, PCI_MAPREG_TYPE_IO, 0,
			   &sc->sc_iot, &sc->sc_ioh, NULL, NULL)) {
		aprint_error("%s: can't map power management i/o space",
		       sc->sc_dev.dv_xname);
		return;
	}

	/* Set up the power management timer. */
	pmtimer_configure(sc, pa);

	/* Set up the TCO (watchdog). */
	tcotimer_configure(sc, pa);

	/* Set up SpeedStep. */
	speedstep_configure(sc, pa);

	/* Install powerhook */
	sc->sc_powerhook = powerhook_establish(sc->sc_dev.dv_xname,
	    lpcib_powerhook, sc);
	if (sc->sc_powerhook == NULL)
		aprint_error("%s: can't establish powerhook\n",
		    sc->sc_dev.dv_xname);
}

static void
lpcib_powerhook(int why, void *opaque)
{
	struct lpcib_softc *sc;
	pci_chipset_tag_t pc;
	pcitag_t tag;

	sc = (struct lpcib_softc *)opaque;
	pc = sc->sc_pc;
	tag = sc->sc_pcitag;

	switch (why) {
	case PWR_SUSPEND:
		pci_conf_capture(pc, tag, &sc->sc_pciconf);

		/* capture PIRQ routing control registers */
		sc->sc_pirq[0] = pci_conf_read(pc, tag, LPCIB_PCI_PIRQA_ROUT);
		sc->sc_pirq[1] = pci_conf_read(pc, tag, LPCIB_PCI_PIRQB_ROUT);
		sc->sc_pirq[2] = pci_conf_read(pc, tag, LPCIB_PCI_PIRQC_ROUT);
		sc->sc_pirq[3] = pci_conf_read(pc, tag, LPCIB_PCI_PIRQD_ROUT);
		sc->sc_pirq[4] = pci_conf_read(pc, tag, LPCIB_PCI_PIRQE_ROUT);
		sc->sc_pirq[5] = pci_conf_read(pc, tag, LPCIB_PCI_PIRQF_ROUT);
		sc->sc_pirq[6] = pci_conf_read(pc, tag, LPCIB_PCI_PIRQG_ROUT);
		sc->sc_pirq[7] = pci_conf_read(pc, tag, LPCIB_PCI_PIRQH_ROUT);

		break;

	case PWR_RESUME:
		pci_conf_restore(pc, tag, &sc->sc_pciconf);

		/* restore PIRQ routing control registers */
		pci_conf_write(pc, tag, LPCIB_PCI_PIRQA_ROUT, sc->sc_pirq[0]);
		pci_conf_write(pc, tag, LPCIB_PCI_PIRQB_ROUT, sc->sc_pirq[1]);
		pci_conf_write(pc, tag, LPCIB_PCI_PIRQC_ROUT, sc->sc_pirq[2]);
		pci_conf_write(pc, tag, LPCIB_PCI_PIRQD_ROUT, sc->sc_pirq[3]);
		pci_conf_write(pc, tag, LPCIB_PCI_PIRQE_ROUT, sc->sc_pirq[4]);
		pci_conf_write(pc, tag, LPCIB_PCI_PIRQF_ROUT, sc->sc_pirq[5]);
		pci_conf_write(pc, tag, LPCIB_PCI_PIRQG_ROUT, sc->sc_pirq[6]);
		pci_conf_write(pc, tag, LPCIB_PCI_PIRQH_ROUT, sc->sc_pirq[7]);

		break;
	}
}

/*
 * Initialize the power management timer.
 */
static void
pmtimer_configure(struct lpcib_softc *sc, struct pci_attach_args *pa)
{
	pcireg_t control;

	/* 
	 * Check if power management I/O space is enabled and enable the ACPI_EN
	 * bit if it's disabled.
	 */
	control = pci_conf_read(pa->pa_pc, pa->pa_tag, LPCIB_PCI_ACPI_CNTL);
	if ((control & LPCIB_PCI_ACPI_CNTL_EN) == 0) {
		control |= LPCIB_PCI_ACPI_CNTL_EN;
		pci_conf_write(pa->pa_pc, pa->pa_tag, LPCIB_PCI_ACPI_CNTL,
		    control);
	}

	/* Attach our PM timer with the generic acpipmtimer function */
	acpipmtimer_attach(&sc->sc_dev, sc->sc_iot, sc->sc_ioh,
	    LPCIB_PM1_TMR, 0);
}

/*
 * Initialize the watchdog timer.
 */
static void
tcotimer_configure(struct lpcib_softc *sc, struct pci_attach_args *pa)
{
	bus_space_handle_t gcs_memh;
	pcireg_t pcireg;
	uint32_t ioreg;
	unsigned int period;

	/*
	 * Map the memory space necessary for GCS (General Control
	 * and Status Register). This is where the No Reboot (NR) bit
	 * lives on ICH6 and newer.
	 */
	if (lpcib_ich6) {
		pcireg = pci_conf_read(pa->pa_pc, pa->pa_tag, LPCIB_RCBA);
		pcireg &= 0xffffc000;
		if (bus_space_map(pa->pa_memt, pcireg + LPCIB_GCS_OFFSET,
		    		  LPCIB_GCS_SIZE, 0, &gcs_memh)) {
			aprint_error("%s: can't map GCS memory space; "
			    "TCO timer disabled\n", sc->sc_dev.dv_xname);
			return;
		}
	}

	/* 
	 * Clear the No Reboot (NR) bit. If this fails, enabling the TCO_EN bit
	 * in the SMI_EN register is the last chance.
	 */
	if (tcotimer_disable_noreboot(sc, pa->pa_memt, gcs_memh)) {
		ioreg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, LPCIB_SMI_EN);
		ioreg |= LPCIB_SMI_EN_TCO_EN;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, LPCIB_SMI_EN, ioreg);
	}

	/* Reset the watchdog status registers. */
	tcotimer_status_reset(sc);

	/* Explicitly stop the TCO timer. */
	tcotimer_stop(sc);

	/* 
	 * Register the driver with the sysmon watchdog framework.
	 */
	sc->sc_smw.smw_name = sc->sc_dev.dv_xname;
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = tcotimer_setmode;
	sc->sc_smw.smw_tickle = tcotimer_tickle;
	if (lpcib_ich6)
		period = LPCIB_TCOTIMER2_MAX_TICK;
	else
		period = LPCIB_TCOTIMER_MAX_TICK;
	sc->sc_smw.smw_period = lpcib_tcotimer_tick_to_second(period);

	if (sysmon_wdog_register(&sc->sc_smw)) {
		aprint_error("%s: unable to register TCO timer"
		       "as a sysmon watchdog device.\n",
		       sc->sc_dev.dv_xname);
		return;
	}

	aprint_verbose("%s: TCO (watchdog) timer configured.\n",
	    sc->sc_dev.dv_xname);
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
		 * ICH5 or older are limited to 4s min and 39s max.
		 * ICH6 or newer are limited to 2s min and 613s max.
		 */
		if (!lpcib_ich6) {
			if (smw->smw_period < LPCIB_TCOTIMER_MIN_TICK ||
			    smw->smw_period > LPCIB_TCOTIMER_MAX_TICK)
				return EINVAL;
		} else {
			if (smw->smw_period < LPCIB_TCOTIMER2_MIN_TICK ||
			    smw->smw_period > LPCIB_TCOTIMER2_MAX_TICK)
				return EINVAL;
		}
		period = lpcib_tcotimer_second_to_tick(smw->smw_period);
		
		/* Stop the TCO timer, */
		tcotimer_stop(sc);

		/* set the timeout, */
		if (lpcib_ich6) {
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
	if (!lpcib_ich6)
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, LPCIB_TCO_RLD, 1);
	else
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO_RLD, 1);

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
tcotimer_disable_noreboot(struct lpcib_softc *sc, bus_space_tag_t gcs_memt,
			  bus_space_handle_t gcs_memh)
{
	pcireg_t pcireg;
	uint16_t status = 0;

	if (!lpcib_ich6) {
		pcireg = pci_conf_read(sc->sc_pc, sc->sc_pcitag, 
				       LPCIB_PCI_GEN_STA);
		if (pcireg & LPCIB_PCI_GEN_STA_NO_REBOOT) {
			/* TCO timeout reset is disabled; try to enable it */
			pcireg &= ~LPCIB_PCI_GEN_STA_NO_REBOOT;
			pci_conf_write(sc->sc_pc, sc->sc_pcitag,
				       LPCIB_PCI_GEN_STA, pcireg);
			if (pcireg & LPCIB_PCI_GEN_STA_NO_REBOOT)
				goto error;
		}
	} else {
		status = bus_space_read_4(gcs_memt, gcs_memh, 0);
		status &= ~LPCIB_GCS_NO_REBOOT;
		bus_space_write_4(gcs_memt, gcs_memh, 0, status);
		status = bus_space_read_4(gcs_memt, gcs_memh, 0);
		bus_space_unmap(gcs_memt, gcs_memh, LPCIB_GCS_SIZE);
		if (status & LPCIB_GCS_NO_REBOOT)
			goto error;
	}

	return 0;
error:
	aprint_error("%s: TCO timer reboot disabled by hardware; "
	    "hope SMBIOS properly handles it.\n", sc->sc_dev.dv_xname);
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
 */
static int
speedstep_bad_hb_check(struct pci_attach_args *pa)
{

	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82815_FULL_HUB &&
	    PCI_REVISION(pa->pa_class) < 5)
		return 1;

	return 0;
}

static void
speedstep_configure(struct lpcib_softc *sc, struct pci_attach_args *pa)
{
	const struct sysctlnode	*node, *ssnode;
	int rv;

	/* Supported on ICH2-M, ICH3-M and ICH4-M.  */
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801DB_ISA ||
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801CAM_LPC ||
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82801BAM_LPC &&
	     pci_find_device(pa, speedstep_bad_hb_check) == 0)) {
		uint8_t pmcon;

		/* Enable SpeedStep if it isn't already enabled. */
		pmcon = pci_conf_read(pa->pa_pc, pa->pa_tag,
				      LPCIB_PCI_GEN_PMCON_1);
		if ((pmcon & LPCIB_PCI_GEN_PMCON_1_SS_EN) == 0)
			pci_conf_write(pa->pa_pc, pa->pa_tag,
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
		aprint_verbose("%s: SpeedStep enabled\n", sc->sc_dev.dv_xname);
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
