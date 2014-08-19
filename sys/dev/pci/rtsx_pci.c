/*	$NetBSD: rtsx_pci.c,v 1.2.10.2 2014/08/20 00:03:48 tls Exp $	*/
/*	$OpenBSD: rtsx_pci.c,v 1.4 2013/11/06 13:51:02 stsp Exp $	*/

/*
 * Copyright (c) 2006 Uwe Stuehler <uwe@openbsd.org>
 * Copyright (c) 2012 Stefan Sperling <stsp@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtsx_pci.c,v 1.2.10.2 2014/08/20 00:03:48 tls Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/pmf.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/rtsxreg.h>
#include <dev/ic/rtsxvar.h>

#include <dev/sdmmc/sdmmcvar.h>

#define RTSX_PCI_BAR	0x10

struct rtsx_pci_softc {
	struct rtsx_softc sc;
	pci_chipset_tag_t sc_pc;
	void *sc_ih;
};

static int rtsx_pci_match(device_t , cfdata_t, void *);
static void rtsx_pci_attach(device_t, device_t, void *);
static int rtsx_pci_detach(device_t, int);

CFATTACH_DECL_NEW(rtsx_pci, sizeof(struct rtsx_pci_softc),
    rtsx_pci_match, rtsx_pci_attach, rtsx_pci_detach, NULL);

#ifdef RTSX_DEBUG
extern int rtsxdebug;
#define DPRINTF(n,s)	do { if ((n) <= rtsxdebug) printf s; } while (0)
#else
#define DPRINTF(n,s)	/**/
#endif

static int
rtsx_pci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args *pa = aux;

	/* 
	 * Explicitly match the UNDEFINED device class only. Some RTS5902
	 * devices advertise a SYSTEM/SDHC class in addition to the UNDEFINED
	 * device class. Let sdhc(4) handle the SYSTEM/SDHC ones.
	 */
	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_REALTEK ||
	    PCI_CLASS(pa->pa_class) != PCI_CLASS_UNDEFINED)
		return 0;

	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_REALTEK_RTS5209 ||
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_REALTEK_RTS5229)
		return 1;

	return 0;
}

static void
rtsx_pci_attach(device_t parent, device_t self, void *aux)
{
	struct rtsx_pci_softc *sc = device_private(self);
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pci_intr_handle_t ih;
	pcireg_t reg;
	char const *intrstr;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_size_t size;
	uint32_t flags;
	char intrbuf[PCI_INTRSTR_LEN];

	sc->sc.sc_dev = self;
	sc->sc_pc = pc;

	pci_aprint_devinfo(pa, NULL);

	if ((pci_conf_read(pc, tag, RTSX_CFG_PCI) & RTSX_CFG_ASIC) != 0) {
		aprint_error_dev(self, "no asic\n");
		return;
	}

	if (pci_mapreg_map(pa, RTSX_PCI_BAR, PCI_MAPREG_TYPE_MEM, 0,
	    &iot, &ioh, NULL, &size)) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	if (pci_intr_map(pa, &ih)) {
		aprint_error_dev(self, "couldn't map interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pc, ih, intrbuf, sizeof(intrbuf));
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_SDMMC, rtsx_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	/* Enable the device */
	reg = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	reg |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, reg);

	/* Power up the device */
	pci_set_powerstate(pc, tag, PCI_PMCSR_STATE_D0);

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_REALTEK_RTS5209:
		flags = RTSX_F_5209;
		break;
	case PCI_PRODUCT_REALTEK_RTS5229:
	default:
		flags = RTSX_F_5229;
		break;
	}

	if (rtsx_attach(&sc->sc, iot, ioh, size, pa->pa_dmat, flags) != 0) {
		aprint_error_dev(self, "couldn't initialize chip\n");
		return;
	}

	if (!pmf_device_register1(self, rtsx_suspend, rtsx_resume,
	    rtsx_shutdown))
		aprint_error_dev(self, "couldn't establish powerhook\n");
}

static int
rtsx_pci_detach(device_t self, int flags)
{
	struct rtsx_pci_softc *sc = device_private(self);
	int rv;

	rv = rtsx_detach(&sc->sc, flags);
	if (rv)
		return rv;

	pci_intr_disestablish(sc->sc_pc, sc->sc_ih);

	return 0;
}
