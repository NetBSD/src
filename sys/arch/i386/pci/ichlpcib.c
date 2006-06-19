/*	$NetBSD: ichlpcib.c,v 1.12.8.1 2006/06/19 03:44:26 chap Exp $	*/

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
 *  Currently we support the watchdog timer, and SpeedStep on some
 *  systems.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ichlpcib.c,v 1.12.8.1 2006/06/19 03:44:26 chap Exp $");

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

static void tcotimer_configure(struct lpcib_softc *, struct pci_attach_args *);
static int tcotimer_setmode(struct sysmon_wdog *);
static int tcotimer_tickle(struct sysmon_wdog *);

static void speedstep_configure(struct lpcib_softc *,
				struct pci_attach_args *);
static int speedstep_sysctl_helper(SYSCTLFN_ARGS);

struct lpcib_softc *speedstep_cookie;	/* XXX */

/* Defined in arch/i386/pci/pcib.c. */
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
	    PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_BRIDGE_ISA) {
		return 0;
	}

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
		case PCI_PRODUCT_INTEL_82801FB_LPC:	/* ICH6 */
		case PCI_PRODUCT_INTEL_82801FBM_LPC:	/* ICH6-M */
		case PCI_PRODUCT_INTEL_82801G_LPC:	/* ICH7 */
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

	/* Set up the TCO (watchdog). */
	tcotimer_configure(sc, pa);

	/* Set up SpeedStep. */
	speedstep_configure(sc, pa);

	/* Install powerhook */
	sc->sc_powerhook = powerhook_establish(lpcib_powerhook, sc);
	if (sc->sc_powerhook == NULL)
		aprint_error("%s: can't establish powerhook\n",
		    sc->sc_dev.dv_xname);

	return;
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

	return;
}

/*
 * Initialize the watchdog timer.
 */
static void
tcotimer_configure(struct lpcib_softc *sc, struct pci_attach_args *pa)
{
	pcireg_t pcireg;
	unsigned int ioreg;

	sc->sc_smw.smw_name = sc->sc_dev.dv_xname;
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = tcotimer_setmode;
	sc->sc_smw.smw_tickle = tcotimer_tickle;
	sc->sc_smw.smw_period =
		lpcib_tcotimer_tick_to_second(LPCIB_TCOTIMER_MAX_TICK);

	if (sysmon_wdog_register(&sc->sc_smw)) {
		aprint_error("%s: unable to register TCO timer"
		       "as a sysmon watchdog device.\n",
		       sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Part of our I/O registers are used as ACPI PM regs.
	 * Since our ACPI subsystem accesses the I/O space directly so far,
	 * we do not have to bother bus_space I/O map confliction.
	 */
	if (pci_mapreg_map(pa, LPCIB_PCI_PMBASE, PCI_MAPREG_TYPE_IO, 0,
			   &sc->sc_iot, &sc->sc_ioh, NULL, NULL)) {
		sysmon_wdog_unregister(&sc->sc_smw);
		aprint_error("%s: can't map i/o space; "
		       "TCO timer disabled\n", sc->sc_dev.dv_xname);
		return;
	}
	
	/* Explicitly stop the TCO timer. */
	ioreg = bus_space_read_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO1_CNT);
	if ((ioreg & LPCIB_TCO1_CNT_TCO_TMR_HLT) == 0)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO1_CNT,
				  ioreg | LPCIB_TCO1_CNT_TCO_TMR_HLT);

	/*
	 * Enable TCO timeout SMI only if the hardware reset does not
	 * work. We don't know what the SMBIOS does.
	 */
	ioreg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, LPCIB_SMI_EN);
	ioreg &= ~LPCIB_SMI_EN_TCO_EN;

	/*
	 * And enable TCO timeout reset.
	 * This logic works when the TCO timer expires twice.
	 */
	pcireg = pci_conf_read(pa->pa_pc, pa->pa_tag, LPCIB_PCI_GEN_STA);
	if (pcireg & LPCIB_PCI_GEN_STA_NO_REBOOT) {
		/* TCO timeout reset is disabled; try to enable it */
		pci_conf_write(pa->pa_pc, pa->pa_tag, LPCIB_PCI_GEN_STA,
			       pcireg & (~LPCIB_PCI_GEN_STA_NO_REBOOT));
		pcireg = pci_conf_read(pa->pa_pc, pa->pa_tag,
					LPCIB_PCI_GEN_STA);
		if (pcireg & LPCIB_PCI_GEN_STA_NO_REBOOT) {
			aprint_error("%s: TCO timer reboot disabled by hardware; "
			       "hope SMBIOS properly handles it.\n",
			       sc->sc_dev.dv_xname);
			ioreg |= LPCIB_SMI_EN_TCO_EN;
		}
	}

	if (ioreg & LPCIB_SMI_EN_GBL_SMI_EN)
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, LPCIB_SMI_EN, ioreg);

	aprint_verbose("%s: TCO (watchdog) timer configured.\n",
	    sc->sc_dev.dv_xname);

	return;
}

/*
 * Sysmon watchdog callbacks.
 */
static int
tcotimer_setmode(struct sysmon_wdog *smw)
{
	struct lpcib_softc *sc = smw->smw_cookie;
	unsigned int ioreg, period;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		/* Stop the TCO timer. */
		ioreg = bus_space_read_2(sc->sc_iot, sc->sc_ioh,
					 LPCIB_TCO1_CNT);
		if ((ioreg & LPCIB_TCO1_CNT_TCO_TMR_HLT) == 0)
			bus_space_write_2(sc->sc_iot, sc->sc_ioh,
					  LPCIB_TCO1_CNT,
					  ioreg | LPCIB_TCO1_CNT_TCO_TMR_HLT);
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT) {
			period = LPCIB_TCOTIMER_MAX_TICK;
			smw->smw_period =
				lpcib_tcotimer_tick_to_second(period);
		} else
			period = lpcib_tcotimer_second_to_tick(smw->smw_period);
		if (period < LPCIB_TCOTIMER_MIN_TICK ||
		    period > LPCIB_TCOTIMER_MAX_TICK)
			return EINVAL;

		/* Stop the TCO timer, */
		ioreg = bus_space_read_2(sc->sc_iot, sc->sc_ioh,
					 LPCIB_TCO1_CNT);
		if ((ioreg & LPCIB_TCO1_CNT_TCO_TMR_HLT) == 0)
			bus_space_write_2(sc->sc_iot, sc->sc_ioh,
					  LPCIB_TCO1_CNT,
					  ioreg | LPCIB_TCO1_CNT_TCO_TMR_HLT);

		/* set the timeout, */
		period |= (bus_space_read_1(sc->sc_iot, sc->sc_ioh,
					 LPCIB_TCO_TMR) & ~LPCIB_TCO_TMR_MASK);
		bus_space_write_1(sc->sc_iot, sc->sc_ioh,
				  LPCIB_TCO_TMR, period);

		/* and restart the timer. */
		bus_space_write_2(sc->sc_iot, sc->sc_ioh,
				  LPCIB_TCO1_CNT,
				  ioreg & ~LPCIB_TCO1_CNT_TCO_TMR_HLT);

		tcotimer_tickle(smw);
	}

	return 0;
}

static int
tcotimer_tickle(struct sysmon_wdog *smw)
{
	struct lpcib_softc *sc = smw->smw_cookie;

	/* any value is allowed */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LPCIB_TCO_RLD, 0);

	return 0;
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
		return (1);

	return (0);
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
	} else
		aprint_verbose("%s: No SpeedStep\n", sc->sc_dev.dv_xname);

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
	struct lpcib_softc *sc = speedstep_cookie;
	uint8_t		state, state2;
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
	if (ostate != nstate)
	{
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
	return (error);
}
