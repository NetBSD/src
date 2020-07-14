/*	$NetBSD: ciss_pci.c,v 1.22 2020/07/14 17:23:58 jdolecek Exp $	*/
/*	$OpenBSD: ciss_pci.c,v 1.9 2005/12/13 15:56:01 brad Exp $	*/

/*
 * Copyright (c) 2005 Michael Shalayeff
 * All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ciss_pci.c,v 1.22 2020/07/14 17:23:58 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <sys/bus.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_disk.h>
#include <dev/scsipi/scsipiconf.h>

#include <dev/ic/cissreg.h>
#include <dev/ic/cissvar.h>

#define	CISS_BAR	0x10

int	ciss_pci_match(device_t, cfdata_t, void *);
void	ciss_pci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ciss_pci, sizeof(struct ciss_softc),
	ciss_pci_match, ciss_pci_attach, NULL, NULL);


static const struct {
	int vendor;
	int product;
	const char *name;
} ciss_pci_devices[] = {
#define CISS_PCI_DEVICE(v, p, d) { PCI_VENDOR_##v, PCI_PRODUCT_##v##_##p, d }
	CISS_PCI_DEVICE(COMPAQ, CSA532, "Compaq Smart Array 532"),
	CISS_PCI_DEVICE(COMPAQ, CSA5300, "Compaq Smart Array 5300 V1"),
	CISS_PCI_DEVICE(COMPAQ, CSA5300_2, "Compaq Smart Array 5300 V2"),
	CISS_PCI_DEVICE(COMPAQ, CSA5312, "Compaq Smart Array 5312"),
	CISS_PCI_DEVICE(COMPAQ, CSA5i, "Compaq Smart Array 5i"),
	CISS_PCI_DEVICE(COMPAQ, CSA5i_2, "Compaq Smart Array 5i V2"),
	CISS_PCI_DEVICE(COMPAQ, CSA6i, "Compaq Smart Array 6i"),
	CISS_PCI_DEVICE(COMPAQ, CSA641, "Compaq Smart Array 641"),
	CISS_PCI_DEVICE(COMPAQ, CSA642, "Compaq Smart Array 642"),
	CISS_PCI_DEVICE(COMPAQ, CSA6400, "Compaq Smart Array 6400"),
	CISS_PCI_DEVICE(COMPAQ, CSA6400EM, "Compaq Smart Array 6400EM"),
	CISS_PCI_DEVICE(COMPAQ, CSA6422, "Compaq Smart Array 6422"),
	CISS_PCI_DEVICE(COMPAQ, CSA64XX, "Compaq Smart Array 64XX"),
	CISS_PCI_DEVICE(HP, HPSAE200, "Smart Array E200"),
	CISS_PCI_DEVICE(HP, HPSAE200I_1, "HP Smart Array E200I-1"),
	CISS_PCI_DEVICE(HP, HPSAE200I_2, "HP Smart Array E200I-2"),
	CISS_PCI_DEVICE(HP, HPSAE200I_3, "HP Smart Array E200I-3"),
	CISS_PCI_DEVICE(HP, HPSAP600, "HP Smart Array P600"),
	CISS_PCI_DEVICE(HP, HPSAP800, "HP Smart Array P800"),
	CISS_PCI_DEVICE(HP, HPSAV100, "HP Smart Array V100"),
	CISS_PCI_DEVICE(HP, HPSA_1, "HP Smart Array 1"),
	CISS_PCI_DEVICE(HP, HPSA_2, "HP Smart Array 2"),
	CISS_PCI_DEVICE(HP, HPSA_3, "HP Smart Array 3"),
	CISS_PCI_DEVICE(HP, HPSA_4, "HP Smart Array 4"),
	CISS_PCI_DEVICE(HP, HPSA_5, "HP Smart Array 5"),
	CISS_PCI_DEVICE(HP, HPSA_6, "HP Smart Array 6"),
	CISS_PCI_DEVICE(HP, HPSA_7, "HP Smart Array 7"),
	CISS_PCI_DEVICE(HP, HPSA_8, "HP Smart Array 8"),
	CISS_PCI_DEVICE(HP, HPSA_9, "HP Smart Array 9"),
	CISS_PCI_DEVICE(HP, HPSA_10, "HP Smart Array 10"),
	CISS_PCI_DEVICE(HP, HPSA_11, "HP Smart Array 11"),
	CISS_PCI_DEVICE(HP, HPSA_12, "HP Smart Array 12"),
	CISS_PCI_DEVICE(HP, HPSA_13, "HP Smart Array 13"),
	CISS_PCI_DEVICE(HP, HPSA_P700M, "Smart Array P700m"),
	CISS_PCI_DEVICE(HP, HPSA_P212, "Smart Array P212"),
	CISS_PCI_DEVICE(HP, HPSA_P410, "Smart Array P410"),
	CISS_PCI_DEVICE(HP, HPSA_P410I, "Smart Array P410i"),
	CISS_PCI_DEVICE(HP, HPSA_P411, "Smart Array P411"),
	CISS_PCI_DEVICE(HP, HPSA_P812, "Smart Array P822"),
	CISS_PCI_DEVICE(HP, HPSA_P712M, "Smart Array P712m"),
	CISS_PCI_DEVICE(HP, HPSA_14, "Smart Array 14"),
	CISS_PCI_DEVICE(HP, HPSA_P222, "Smart Array P222"),
	CISS_PCI_DEVICE(HP, HPSA_P420, "Smart Array P420"),
	CISS_PCI_DEVICE(HP, HPSA_P421, "Smart Array P421"),
	CISS_PCI_DEVICE(HP, HPSA_P822, "Smart Array P822"),
	CISS_PCI_DEVICE(HP, HPSA_P420I, "Smart Array P420i"),
	CISS_PCI_DEVICE(HP, HPSA_P220I, "Smart Array P220i"),
	CISS_PCI_DEVICE(HP, HPSA_P721I, "Smart Array P721i"),
	CISS_PCI_DEVICE(HP, HPSA_P430I, "Smart Array P430i"),
	CISS_PCI_DEVICE(HP, HPSA_P830I, "Smart Array P830i"),
	CISS_PCI_DEVICE(HP, HPSA_P430, "Smart Array P430"),
	CISS_PCI_DEVICE(HP, HPSA_P431, "Smart Array P431"),
	CISS_PCI_DEVICE(HP, HPSA_P830, "Smart Array P830"),
	CISS_PCI_DEVICE(HP, HPSA_P731M, "Smart Array P731m"),
	CISS_PCI_DEVICE(HP, HPSA_P230I, "Smart Array P230i"),
	CISS_PCI_DEVICE(HP, HPSA_P530, "Smart Array P530"),
	CISS_PCI_DEVICE(HP, HPSA_P531, "Smart Array P531"),
	CISS_PCI_DEVICE(HP, HPSA_P244BR, "Smart Array P244br"),
	CISS_PCI_DEVICE(HP, HPSA_P741M, "Smart Array P741m"),
	CISS_PCI_DEVICE(HP, HPSA_H240AR, "Smart Array H240ar"),
	CISS_PCI_DEVICE(HP, HPSA_P440AR, "Smart Array H440ar"),
	CISS_PCI_DEVICE(HP, HPSA_P840AR, "Smart Array P840ar"),
	CISS_PCI_DEVICE(HP, HPSA_P440, "Smart Array P440"),
	CISS_PCI_DEVICE(HP, HPSA_P441, "Smart Array P441"),
	CISS_PCI_DEVICE(HP, HPSA_P841, "Smart Array P841"),
	CISS_PCI_DEVICE(HP, HPSA_H244BR, "Smart Array H244br"),
	CISS_PCI_DEVICE(HP, HPSA_H240, "Smart Array H240"),
	CISS_PCI_DEVICE(HP, HPSA_H241, "Smart Array H241"),
	CISS_PCI_DEVICE(HP, HPSA_P246BR, "Smart Array P246br"),
	CISS_PCI_DEVICE(HP, HPSA_P840, "Smart Array P840"),
	CISS_PCI_DEVICE(HP, HPSA_P542D, "Smart Array P542d"),
	CISS_PCI_DEVICE(HP, HPSA_P240NR, "Smart Array P240nr"),
	CISS_PCI_DEVICE(HP, HPSA_H240NR, "Smart Array H240nr"),
};

int
ciss_pci_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	pcireg_t reg = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);
	int i;

	for (i = 0; i < __arraycount(ciss_pci_devices); i++)
	{
		if ((PCI_VENDOR(pa->pa_id) == ciss_pci_devices[i].vendor &&
		     PCI_PRODUCT(pa->pa_id) == ciss_pci_devices[i].product) ||
		    (PCI_VENDOR(reg) == ciss_pci_devices[i].vendor &&
		     PCI_PRODUCT(reg) == ciss_pci_devices[i].product))
			return 1;
	}

	return 0;
}

void
ciss_pci_attach(device_t parent, device_t self, void *aux)
{
	struct ciss_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	bus_size_t size, cfgsz;
	pci_intr_handle_t *ih;
	const char *intrstr;
	int cfg_bar, memtype;
	pcireg_t reg = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);
	int i;
	char intrbuf[PCI_INTRSTR_LEN];
	int (*intr_handler)(void *);

	sc->sc_dev = self;

	aprint_naive("\n");
	for (i = 0; i < __arraycount(ciss_pci_devices); i++)
	{
		if ((PCI_VENDOR(pa->pa_id) == ciss_pci_devices[i].vendor &&
		     PCI_PRODUCT(pa->pa_id) == ciss_pci_devices[i].product) ||
		    (PCI_VENDOR(reg) == ciss_pci_devices[i].vendor &&
		     PCI_PRODUCT(reg) == ciss_pci_devices[i].product))
		{
			aprint_normal(": %s\n", ciss_pci_devices[i].name);
			break;
		}
	}

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, CISS_BAR);
	if (memtype != (PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT) &&
	    memtype != (PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT)) {
		aprint_error_dev(self, "wrong BAR type\n");
		return;
	}
	if (pci_mapreg_map(pa, CISS_BAR, memtype, 0,
	    &sc->sc_iot, &sc->sc_ioh, NULL, &size)) {
		aprint_error_dev(self, "can't map controller i/o space\n");
		return;
	}
	sc->sc_dmat = pa->pa_dmat;

	sc->iem = CISS_INTR_OPQ_SA5;
	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);
	if (PCI_VENDOR(reg) == PCI_VENDOR_COMPAQ &&
	    (PCI_PRODUCT(reg) == PCI_PRODUCT_COMPAQ_CSA5i ||
	     PCI_PRODUCT(reg) == PCI_PRODUCT_COMPAQ_CSA532 ||
	     PCI_PRODUCT(reg) == PCI_PRODUCT_COMPAQ_CSA5312))
		sc->iem = CISS_INTR_OPQ_SA5B;

	cfg_bar = bus_space_read_2(sc->sc_iot, sc->sc_ioh, CISS_CFG_BAR);
	sc->cfgoff = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CISS_CFG_OFF);
	if (cfg_bar != CISS_BAR) {
		if (pci_mapreg_map(pa, cfg_bar, PCI_MAPREG_TYPE_MEM, 0,
		    NULL, &sc->cfg_ioh, NULL, &cfgsz)) {
			aprint_error_dev(self,
			    "can't map controller config space\n");
			bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
			return;
		}
	} else {
		sc->cfg_ioh = sc->sc_ioh;
		cfgsz = size;
	}

	if (sc->cfgoff + sizeof(struct ciss_config) > cfgsz) {
		aprint_error_dev(self, "unfit config space\n");
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
		if (cfg_bar != CISS_BAR)
			bus_space_unmap(sc->sc_iot, sc->cfg_ioh, cfgsz);
		return;
	}

	/* Read the configuration */
	bus_space_read_region_4(sc->sc_iot, sc->cfg_ioh, sc->cfgoff,
	    (u_int32_t *)&sc->cfg, sizeof(sc->cfg) / 4);

	/* disable interrupts until ready */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CISS_IMR,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, CISS_IMR) |
		sc->iem | CISS_INTR_OPQ | CISS_INTR_MSI);

	int counts[PCI_INTR_TYPE_SIZE] = {
		[PCI_INTR_TYPE_INTX] = 1,
		[PCI_INTR_TYPE_MSI] = 0,
		[PCI_INTR_TYPE_MSIX] = 0,
	};
	int max_type = PCI_INTR_TYPE_INTX;

	/*
	 * Allow MSI/MSI-X only if PERFORMANT method is supported, SIMPLE
	 * doesn't seem to work with MSI.
	 */
	if (CISS_PERF_SUPPORTED(sc)) {
#if 1
		counts[PCI_INTR_TYPE_MSI] = counts[PCI_INTR_TYPE_MSIX] = 1;
		max_type = PCI_INTR_TYPE_MSIX;
#endif
		sc->iem |= CISS_INTR_OPQ | CISS_INTR_MSI;
	}

	if (pci_intr_alloc(pa, &ih, counts, max_type)) {
		aprint_error_dev(self, "can't map interrupt\n");
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
		if (cfg_bar != CISS_BAR)
			bus_space_unmap(sc->sc_iot, sc->cfg_ioh, cfgsz);
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih[0], intrbuf, sizeof(intrbuf));

	switch (pci_intr_type(pa->pa_pc, ih[0])) {
	case PCI_INTR_TYPE_INTX:
		intr_handler = CISS_PERF_SUPPORTED(sc)
		    ? ciss_intr_perf_intx : ciss_intr_simple_intx;
		break;
	default:
		KASSERT(CISS_PERF_SUPPORTED(sc));
		intr_handler = ciss_intr_perf_msi;
		break;
	}

	sc->sc_ih = pci_intr_establish_xname(pa->pa_pc, ih[0], IPL_BIO,
	    intr_handler, sc, device_xname(self));
	if (!sc->sc_ih) {
		aprint_error_dev(sc->sc_dev, "can't establish interrupt");
		if (intrstr)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		pci_intr_release(pa->pa_pc, ih, 1);
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
		if (cfg_bar != CISS_BAR)
			bus_space_unmap(sc->sc_iot, sc->cfg_ioh, cfgsz);
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	aprint_normal("%s", device_xname(sc->sc_dev));
	if (ciss_attach(sc)) {
		pci_intr_disestablish(pa->pa_pc, sc->sc_ih);
		sc->sc_ih = NULL;
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
		if (cfg_bar != CISS_BAR)
			bus_space_unmap(sc->sc_iot, sc->cfg_ioh, cfgsz);
		return;
	}

	/* enable interrupts now */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CISS_IMR,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, CISS_IMR) & ~sc->iem);
}
