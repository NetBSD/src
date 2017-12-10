/* $NetBSD: dwiic_pci.c,v 1.1 2017/12/10 17:12:54 bouyer Exp $ */

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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
 * Synopsys DesignWare I2C controller, PCI front-end
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dwiic_pci.c,v 1.1 2017/12/10 17:12:54 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_pci.h>
#include <dev/acpi/acpi_util.h>
#include <dev/acpi/acpi_i2c.h>

#include <dev/ic/dwiic_var.h>
#include <arch/x86/pci/lpssreg.h>

//#define DWIIC_DEBUG

#ifdef DWIIC_DEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

struct pci_dwiic_softc {
	struct dwiic_softc	sc_dwiic;
	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_ptag;
	struct acpi_devnode	*sc_acpinode;
};

static uint32_t
lpss_read(struct pci_dwiic_softc *sc, int offset)
{
	u_int32_t b = bus_space_read_4(sc->sc_dwiic.sc_iot, sc->sc_dwiic.sc_ioh,
	     offset);
	return b;
}

static void
lpss_write(struct pci_dwiic_softc *sc, int offset, uint32_t val)
{
	bus_space_write_4(sc->sc_dwiic.sc_iot, sc->sc_dwiic.sc_ioh,
	    offset, val);
}

static int	pci_dwiic_match(device_t, cfdata_t, void *);
static void	pci_dwiic_attach(device_t, device_t, void *);
static bool	dwiic_pci_power(struct dwiic_softc *, bool);

CFATTACH_DECL_NEW(pcidwiic, sizeof(struct pci_dwiic_softc),
    pci_dwiic_match, pci_dwiic_attach, dwiic_detach, NULL);


int
pci_dwiic_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL)
		return 0;

	if (PCI_PRODUCT(pa->pa_id) < PCI_PRODUCT_INTEL_100SERIES_LP_I2C_0 ||
	    PCI_PRODUCT(pa->pa_id) > PCI_PRODUCT_INTEL_100SERIES_LP_I2C_3)
		return 0;

	return 1;
}

void
pci_dwiic_attach(device_t parent, device_t self, void *aux)
{
	struct pci_dwiic_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	const char *intrstr;
	pci_intr_handle_t intrhandle;
	char intrbuf[PCI_INTRSTR_LEN];
	pcireg_t memtype;
	pcireg_t csr;
	uint32_t caps;

	sc->sc_dwiic.sc_dev = self;
	sc->sc_dwiic.sc_power = dwiic_pci_power;
	sc->sc_dwiic.sc_type = dwiic_type_sunrisepoint;

	sc->sc_pc = pa->pa_pc;
	sc->sc_ptag = pa->pa_tag;

	/* register access not enabled by BIOS */
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    csr | PCI_COMMAND_MEM_ENABLE);

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, PCI_BAR0);
	if (pci_mapreg_map(pa, PCI_BAR0, memtype, 0, &sc->sc_dwiic.sc_iot,
	    &sc->sc_dwiic.sc_ioh, NULL, NULL) != 0) {
		aprint_error(": can't map register space\n");
		goto out;
	}
	dwiic_pci_power(&sc->sc_dwiic, 1);

	caps = lpss_read(sc, LPSS_CAP);

	aprint_naive(": I2C controller\n");
	aprint_normal(": I2C controller instance %d\n",
	    (int)(caps & LPSS_CAP_INSTANCE));

	if (pci_intr_map(pa, &intrhandle)) {
		aprint_error_dev(self, "can't map interrupt\n");
		goto out;
	}
	intrstr = pci_intr_string(pa->pa_pc, intrhandle,
	    intrbuf, sizeof(intrbuf));

	sc->sc_dwiic.sc_ih = pci_intr_establish(pa->pa_pc, intrhandle,
	    IPL_VM, dwiic_intr, sc);
	if (sc->sc_dwiic.sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		goto out;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	lpss_write(sc, LPSS_RESET, LPSS_RESET_CTRL_REL);
	lpss_write(sc, LPSS_REMAP_LO,
	    pci_conf_read(sc->sc_pc, sc->sc_ptag, PCI_BAR0));
	lpss_write(sc, LPSS_REMAP_HI,
	    pci_conf_read(sc->sc_pc, sc->sc_ptag, PCI_BAR0 + 0x4));

	sc->sc_acpinode = acpi_pcidev_find(0 /*XXX segment*/,
	    pa->pa_bus, pa->pa_device, pa->pa_function);

	if (sc->sc_acpinode) {
		sc->sc_dwiic.sc_iba.iba_child_devices = 
		    acpi_enter_i2c_devs(sc->sc_acpinode);
	} else {
		aprint_verbose_dev(self, "no matching ACPI node\n");
	}

	dwiic_attach(&sc->sc_dwiic);

	pmf_device_register(self, dwiic_suspend, dwiic_resume);

out:
	return;
}

static bool
dwiic_pci_power(struct dwiic_softc *dwsc, bool power)
{
	struct pci_dwiic_softc *sc = (void *)dwsc;
	pcireg_t pmreg;

	printf("status 0x%x\n", pci_conf_read(sc->sc_pc, sc->sc_ptag, PCI_COMMAND_STATUS_REG));
	printf("reset 0x%x\n", lpss_read(sc, LPSS_RESET));
	printf("rlo 0x%x\n", lpss_read(sc, LPSS_REMAP_LO));
	printf("rho 0x%x\n", lpss_read(sc, LPSS_REMAP_HI));

	if (!power)
		lpss_write(sc, LPSS_CLKGATE, LPSS_CLKGATE_CTRL_OFF);
	if (pci_get_capability(sc->sc_pc, sc->sc_ptag, PCI_CAP_PWRMGMT,
	    &pmreg, NULL)) {
		DPRINTF(("%s: power status 0x%x", device_xname(dwsc->sc_dev),
		    pci_conf_read(sc->sc_pc, sc->sc_ptag, pmreg + PCI_PMCSR)));
		pci_conf_write(sc->sc_pc, sc->sc_ptag, pmreg + PCI_PMCSR,
		    power ? PCI_PMCSR_STATE_D0 : PCI_PMCSR_STATE_D3);
		DELAY(10000); /* 10 milliseconds */
		DPRINTF((" -> 0x%x\n", 
		    pci_conf_read(sc->sc_pc, sc->sc_ptag, pmreg + PCI_PMCSR)));
	}
	if (power) {
		lpss_write(sc, LPSS_CLKGATE, LPSS_CLKGATE_CTRL_ON);
	}
	return true;
}
