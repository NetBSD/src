/*	$NetBSD: mq200_pci.c,v 1.2.8.2 2002/06/23 17:36:51 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002 TAKEMURA Shin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mq200_pci.c,v 1.2.8.2 2002/06/23 17:36:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciio.h>

#include <hpcmips/dev/mq200var.h>

struct mq200_pci_softc {
	struct mq200_softc	sc_mq200;
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;
};

int	mq200_pci_match(struct device *, struct cfdata *, void *);
void	mq200_pci_attach(struct device *, struct device *, void *);

struct cfattach mqvideo_pci_ca = {
	sizeof(struct mq200_pci_softc), 
	mq200_pci_match, 
	mq200_pci_attach,
};

int
mq200_pci_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	/* check vendor id and product id */
	if (pa->pa_id !=
	    PCI_ID_CODE(PCI_VENDOR_MEDIAQ, PCI_PRODUCT_MEDIAQ_MQ200))
		return (0);

	return (1);
}

void
mq200_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct mq200_pci_softc *psc = (void *) self;
	struct mq200_softc *sc = &psc->sc_mq200;
	struct pci_attach_args *pa = aux;
	int res;

	psc->sc_pc = pa->pa_pc;
	psc->sc_pcitag = pa->pa_tag;

	/* check whether it is disabled by firmware */
	if (!(pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) &
	    PCI_COMMAND_MEM_ENABLE)) {
		printf("%s: disabled\n", sc->sc_dev.dv_xname);
		return;
	}

	/* Base Address Register 0: base address of control registers */
	res = pci_mapreg_map(pa, PCI_MAPREG_START, PCI_MAPREG_TYPE_MEM,
	    0, &sc->sc_iot, &sc->sc_ioh, NULL, NULL);
	if (res != 0) {
		printf("%s: can't map registers\n", sc->sc_dev.dv_xname);
		return;
	}

	/* Base Address Register 1: base address of frame buffer */
	res = pci_mapreg_info(psc->sc_pc, psc->sc_pcitag, PCI_MAPREG_START+4,
	    PCI_MAPREG_TYPE_MEM, &sc->sc_baseaddr, NULL, NULL);
	if (res != 0) {
		printf("%s: can't map frame buffer\n", sc->sc_dev.dv_xname);
		return;
	}

	mq200_attach(sc);
}
