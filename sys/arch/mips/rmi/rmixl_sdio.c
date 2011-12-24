/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/param.h>

__KERNEL_RCSID(1, "$NetBSD: rmixl_sdio.c,v 1.1.2.1 2011/12/24 01:57:54 matt Exp $");

#include <sys/device.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <mips/rmi/rmixlvar.h>

#include <mips/rmi/rmixl_sdhcvar.h>

#include "locators.h"

static int xlsdio_match(device_t, cfdata_t, void *);
static void xlsdio_attach(device_t, device_t, void *);
static int xlsdio_print(void *aux, const char *);
static int xlsdio_intr(void *);

CFATTACH_DECL_NEW(xlsdio, sizeof(struct xlsdio_softc),
	xlsdio_match, xlsdio_attach, 0, 0);

static int
xlsdio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args * const pa = aux;
	if (pa->pa_id == PCI_ID_CODE(PCI_VENDOR_NETLOGIC, PCI_PRODUCT_NETLOGIC_XLP_SDHC))
		return 1;

	return 0;
}

static void
xlsdio_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct pci_attach_args * const pa = aux;
	struct xlsdio_softc * const sc = device_private(self);
	pci_intr_handle_t pcih;
	pcireg_t r;

	aprint_normal(": eMMC/SD/SDIO Interface\n");

	pci_intr_map(pa, &pcih);

	if (pci_intr_establish(pa->pa_pc, pcih, IPL_SDMMC, xlsdio_intr, sc) == NULL) {
		aprint_error_dev(self, "failed to establish interrupt\n");
	} else {
		const char * const intrstr = pci_intr_string(pa->pa_pc, pcih);
		aprint_normal_dev(self, "interrupting at %s\n", intrstr);
	}

	r = pci_conf_read(pa->pa_pc, pa->pa_tag, RMIXLP_MMC_SYSCTRL);
	r &= ~RMIXLP_MMC_SYSCTRL_RST;
	pci_conf_write(pa->pa_pc, pa->pa_tag, RMIXLP_MMC_SYSCTRL, r);
	DELAY(1000);
	r |= RMIXLP_MMC_SYSCTRL_CA;	/* Cache Allocate */
	r |= RMIXLP_MMC_SYSCTRL_EN0;	/* Enable Slot 0 */
	r |= RMIXLP_MMC_SYSCTRL_EN1;	/* Enable Slot 1 */
	r &= ~RMIXLP_MMC_SYSCTRL_CLK_DIS; /* Don't Disable Clock */
	pci_conf_write(pa->pa_pc, pa->pa_tag, RMIXLP_MMC_SYSCTRL, r);

	for (size_t slot = 0; slot < __arraycount(sc->sc_slots); slot++) {
		bus_size_t offset = RMIXLP_MMC_SLOTSIZE * (slot + 1);
		struct xlsdio_attach_args xaa = {
			.xa_bst = &rcp->rc_pci_ecfg_eb_memt,
			.xa_bsh = rcp->rc_pci_ecfg_eb_memh,
			.xa_dmat = pa->pa_dmat,
			.xa_addr = pa->pa_tag + offset, 
			.xa_size = RMIXLP_MMC_SLOTSIZE,
			.xa_slot = slot,
		};
		pci_conf_write(pa->pa_pc, pa->pa_tag, 
		    offset + SDHC_NINTR_STATUS_EN, 0);
		pci_conf_write(pa->pa_pc, pa->pa_tag,
		    offset + SDHC_NINTR_STATUS_EN, 0);
		pci_conf_write(pa->pa_pc, pa->pa_tag,
		    offset + SDHC_NINTR_STATUS, 0xffffffff);

		config_found(self, &xaa, xlsdio_print);
	}
}

static int
xlsdio_print(void *aux, const char *pnp)
{
	struct xlsdio_attach_args *xa = aux;

	if (pnp != NULL)
		aprint_normal("%s:", pnp);
	if (xa->xa_slot != XLSDIOCF_SLOT_DEFAULT)
		aprint_normal(" slot %d", xa->xa_slot);

	return UNCONF;
}

static int
xlsdio_intr(void *v)
{
	struct xlsdio_softc * const sc = v;
	int rv = 0;

	for (size_t slot = 0; slot < __arraycount(sc->sc_slots); slot++) {
		void *arg = sc->sc_slots[slot];
		if (arg != NULL) {
			int nrv = sdhc_intr(arg);
			if (nrv && !rv)
				rv = nrv;
		}
	}
	return rv;
}
