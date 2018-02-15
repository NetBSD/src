/* $NetBSD: virtio_pci.c,v 1.2 2018/02/15 19:05:10 uwe Exp $ */

/*
 * Copyright (c) 2010 Minoura Makoto.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/device.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#define VIRTIO_PRIVATE

#include <dev/pci/virtioreg.h> /* XXX: move to non-pci */
#include <dev/pci/virtiovar.h> /* XXX: move to non-pci */

static int	virtio_match(device_t, cfdata_t, void *);
static void	virtio_attach(device_t, device_t, void *);
static int	virtio_rescan(device_t, const char *, const int *);
static int	virtio_detach(device_t, int);

static const char *virtio_device_name[] = {
	"Unknown (0)",			/* 0 */
	"Network",			/* 1 */
	"Block",			/* 2 */
	"Console",			/* 3 */
	"Entropy",			/* 4 */
	"Memory Balloon",		/* 5 */
	"I/O Memory",			/* 6 */
	"Remote Processor Messaging",	/* 7 */
	"SCSI",				/* 8 */
	"9P Transport",			/* 9 */
	"mac80211 wlan",		/* 10 */
};
#define NDEVNAMES	__arraycount(virtio_device_name)

CFATTACH_DECL3_NEW(virtio_pci, sizeof(struct virtio_softc),
    virtio_match, virtio_attach, virtio_detach, NULL, virtio_rescan, NULL,
    DVF_DETACH_SHUTDOWN);

static int
virtio_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa;

	pa = (struct pci_attach_args *)aux;
	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_QUMRANET:
		if ((PCI_PRODUCT_QUMRANET_VIRTIO_1000 <=
		     PCI_PRODUCT(pa->pa_id)) &&
		    (PCI_PRODUCT(pa->pa_id) <=
		     PCI_PRODUCT_QUMRANET_VIRTIO_103F))
			return 1;
		break;
	}

	return 0;
}

static void
virtio_attach(device_t parent, device_t self, void *aux)
{
	struct virtio_softc *sc = device_private(self);
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	int revision;
	pcireg_t id;
	pcireg_t csr;

	revision = PCI_REVISION(pa->pa_class);
	if (revision != 0) {
		aprint_normal(": unknown revision 0x%02x; giving up\n",
			      revision);
		return;
	}
	aprint_normal("\n");
	aprint_naive("\n");

	/* subsystem ID shows what I am */
	id = pci_conf_read(pc, tag, PCI_SUBSYS_ID_REG);
	aprint_normal_dev(self, "Virtio %s Device (rev. 0x%02x)\n",
			  (PCI_SUBSYS_ID(id) < NDEVNAMES?
			   virtio_device_name[PCI_SUBSYS_ID(id)] : "Unknown"),
			  revision);

	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	csr |= PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_IO_ENABLE;
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);

	sc->sc_dev = self;
	sc->sc_pc = pc;
	sc->sc_tag = tag;
	sc->sc_iot = pa->pa_iot;
	if (pci_dma64_available(pa))
		sc->sc_dmat = pa->pa_dmat64;
	else
		sc->sc_dmat = pa->pa_dmat;
	sc->sc_config_offset = VIRTIO_CONFIG_DEVICE_CONFIG_NOMSI;

	if (pci_mapreg_map(pa, PCI_MAPREG_START, PCI_MAPREG_TYPE_IO, 0,
			   &sc->sc_iot, &sc->sc_ioh, NULL, &sc->sc_iosize)) {
		aprint_error_dev(self, "can't map i/o space\n");
		return;
	}

	virtio_device_reset(sc);
	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_ACK);
	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_DRIVER);

	sc->sc_childdevid = PCI_SUBSYS_ID(id);
	sc->sc_child = NULL;
	sc->sc_pa = *pa;
	virtio_rescan(self, "virtio", 0);
	return;
}

/* ARGSUSED */
static int
virtio_rescan(device_t self, const char *attr, const int *scan_flags)
{
	struct virtio_softc *sc;
	struct virtio_attach_args va;

	sc = device_private(self);
	if (sc->sc_child)	/* Child already attached? */
		return 0;

	memset(&va, 0, sizeof(va));
	va.sc_childdevid = sc->sc_childdevid;

	config_found_ia(self, attr, &va, NULL);

	if (sc->sc_child == NULL) {
		aprint_error_dev(self,
				 "no matching child driver; not configured\n");
		return 0;
	}
	
	if (sc->sc_child == VIRTIO_CHILD_FAILED) {
		aprint_error_dev(self,
				 "virtio configuration failed\n");
		return 0;
	}

	/*
	 * Make sure child drivers initialize interrupts via call
	 * to virtio_child_attach_finish().
	 */
	KASSERT(sc->sc_ihs_num != 0);

	return 0;
}


static int
virtio_detach(device_t self, int flags)
{
	struct virtio_softc *sc = device_private(self);
	int r;

	if (sc->sc_child != NULL) {
		r = config_detach(sc->sc_child, flags);
		if (r)
			return r;
	}

	/* Check that child detached properly */
	KASSERT(sc->sc_child == NULL);
	KASSERT(sc->sc_vqs == NULL);
	KASSERT(sc->sc_ihs_num == 0);

	if (sc->sc_iosize)
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_iosize);
	sc->sc_iosize = 0;

	return 0;
}
