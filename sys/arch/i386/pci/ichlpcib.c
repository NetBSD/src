/*	$NetBSD: ichlpcib.c,v 1.5 2004/04/04 15:16:38 kochi Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto.
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
 *  Currently we support the watchdog timer.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ichlpcib.c,v 1.5 2004/04/04 15:16:38 kochi Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/ic/i82801lpcreg.h>

struct lpcib_softc {
	/* Device object. */
	struct device		sc_dev;

	/* Watchdog variables. */
	struct sysmon_wdog	sc_smw;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

static int lpcibmatch(struct device *, struct cfdata *, void *);
static void lpcibattach(struct device *, struct device *, void *);

static void tcotimer_configure(struct lpcib_softc *,
				struct pci_attach_args *);
static int tcotimer_setmode(struct sysmon_wdog *);
static int tcotimer_tickle(struct sysmon_wdog *);

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

	pcibattach(parent, self, aux);

	/* Currently we use TCO timer feature only... */
	tcotimer_configure(sc, pa);
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
		printf("%s: unable to register TCO timer"
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
		printf("%s: can't map i/o space; "
		       "TCO timer disabled\n", sc->sc_dev.dv_xname);
		return;
	}
	
	/* Explicitly stop the TCO timer. */
	ioreg = bus_space_read_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO1_CNT);
	if ((ioreg & LPCIB_TCO1_CNT_TCO_TMR_HLT) == 0)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, LPCIB_TCO1_CNT,
				  ioreg | LPCIB_TCO1_CNT_TCO_TMR_HLT);

	/*
	 * Enable TCO timeout SMI. SMBIOS might ignore it, but...
	 */
	ioreg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, LPCIB_SMI_EN);
	if (ioreg & LPCIB_SMI_EN_GBL_SMI_EN)
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, LPCIB_SMI_EN,
				  ioreg | LPCIB_SMI_EN_TCO_EN);

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
		if (pcireg & LPCIB_PCI_GEN_STA_NO_REBOOT)
			printf("%s: TCO timer reboot disabled by hardware; "
			       "hope SMBIOS properly handles it.\n",
			       sc->sc_dev.dv_xname);
	}

	printf("%s: TCO (watchdog) timer configured.\n", sc->sc_dev.dv_xname);

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
