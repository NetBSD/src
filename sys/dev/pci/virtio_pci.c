/* $NetBSD: virtio_pci.c,v 1.2.2.1 2018/06/25 07:26:01 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: virtio_pci.c,v 1.2.2.1 2018/06/25 07:26:01 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/module.h>

#include <sys/device.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#define VIRTIO_PRIVATE

#include <dev/pci/virtioreg.h> /* XXX: move to non-pci */
#include <dev/pci/virtiovar.h> /* XXX: move to non-pci */

static int	virtio_pci_match(device_t, cfdata_t, void *);
static void	virtio_pci_attach(device_t, device_t, void *);
static int	virtio_pci_rescan(device_t, const char *, const int *);
static int	virtio_pci_detach(device_t, int);

struct virtio_pci_softc {
	struct virtio_softc	sc_sc;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_size_t		sc_iosize;
	struct pci_attach_args	sc_pa;
	pci_intr_handle_t	*sc_ihp;
	void			**sc_ihs;
	int			sc_ihs_num;
	int			sc_config_offset;
};

static void	virtio_pci_kick(struct virtio_softc *, uint16_t);
static uint8_t	virtio_pci_read_device_config_1(struct virtio_softc *, int);
static uint16_t	virtio_pci_read_device_config_2(struct virtio_softc *, int);
static uint32_t	virtio_pci_read_device_config_4(struct virtio_softc *, int);
static uint64_t	virtio_pci_read_device_config_8(struct virtio_softc *, int);
static void 	virtio_pci_write_device_config_1(struct virtio_softc *, int, uint8_t);
static void	virtio_pci_write_device_config_2(struct virtio_softc *, int, uint16_t);
static void	virtio_pci_write_device_config_4(struct virtio_softc *, int, uint32_t);
static void	virtio_pci_write_device_config_8(struct virtio_softc *, int, uint64_t);
static uint16_t	virtio_pci_read_queue_size(struct virtio_softc *, uint16_t);
static void	virtio_pci_setup_queue(struct virtio_softc *, uint16_t, uint32_t);
static void	virtio_pci_set_status(struct virtio_softc *, int);
static uint32_t	virtio_pci_negotiate_features(struct virtio_softc *, uint32_t);
static int	virtio_pci_setup_interrupts(struct virtio_softc *);
static void	virtio_pci_free_interrupts(struct virtio_softc *);

static int	virtio_pci_intr(void *arg);
static int	virtio_pci_msix_queue_intr(void *);
static int	virtio_pci_msix_config_intr(void *);
static int	virtio_pci_setup_msix_vectors(struct virtio_softc *);
static int	virtio_pci_setup_msix_interrupts(struct virtio_softc *,
		    struct pci_attach_args *);
static int	virtio_pci_setup_intx_interrupt(struct virtio_softc *,
		    struct pci_attach_args *);

#define VIRTIO_MSIX_CONFIG_VECTOR_INDEX	0
#define VIRTIO_MSIX_QUEUE_VECTOR_INDEX	1

/* we use the legacy virtio spec, so the PCI registers are host native
 * byte order, not PCI (i.e. LE) byte order */
#if BYTE_ORDER == BIG_ENDIAN
#define REG_HI_OFF      0
#define REG_LO_OFF      4
#ifndef __BUS_SPACE_HAS_STREAM_METHODS
#define bus_space_read_stream_1 bus_space_read_1
#define bus_space_write_stream_1 bus_space_write_1
static inline uint16_t
bus_space_read_stream_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o)
{
	return le16toh(bus_space_read_2(t, h, o));
}
static inline void
bus_space_write_stream_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint16_t v)
{
	bus_space_write_2(t, h, o, htole16(v));
}
static inline uint32_t
bus_space_read_stream_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o)
{
	return le32toh(bus_space_read_4(t, h, o));
}
static inline void
bus_space_write_stream_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint32_t v)
{
	bus_space_write_4(t, h, o, htole32(v));
}
#endif
#else
#define REG_HI_OFF	4
#define REG_LO_OFF	0
#ifndef __BUS_SPACE_HAS_STREAM_METHODS
#define bus_space_read_stream_1 bus_space_read_1
#define bus_space_read_stream_2 bus_space_read_2
#define bus_space_read_stream_4 bus_space_read_4
#define bus_space_write_stream_1 bus_space_write_1
#define bus_space_write_stream_2 bus_space_write_2
#define bus_space_write_stream_4 bus_space_write_4
#endif
#endif


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

CFATTACH_DECL3_NEW(virtio_pci, sizeof(struct virtio_pci_softc),
    virtio_pci_match, virtio_pci_attach, virtio_pci_detach, NULL,
    virtio_pci_rescan, NULL, DVF_DETACH_SHUTDOWN);

static const struct virtio_ops virtio_pci_ops = {
	.kick = virtio_pci_kick,
	.read_dev_cfg_1 = virtio_pci_read_device_config_1,
	.read_dev_cfg_2 = virtio_pci_read_device_config_2,
	.read_dev_cfg_4 = virtio_pci_read_device_config_4,
	.read_dev_cfg_8 = virtio_pci_read_device_config_8,
	.write_dev_cfg_1 = virtio_pci_write_device_config_1,
	.write_dev_cfg_2 = virtio_pci_write_device_config_2,
	.write_dev_cfg_4 = virtio_pci_write_device_config_4,
	.write_dev_cfg_8 = virtio_pci_write_device_config_8,
	.read_queue_size = virtio_pci_read_queue_size,
	.setup_queue = virtio_pci_setup_queue,
	.set_status = virtio_pci_set_status,
	.neg_features = virtio_pci_negotiate_features,
	.setup_interrupts = virtio_pci_setup_interrupts,
	.free_interrupts = virtio_pci_free_interrupts,
};

static int
virtio_pci_match(device_t parent, cfdata_t match, void *aux)
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
virtio_pci_attach(device_t parent, device_t self, void *aux)
{
	struct virtio_pci_softc * const psc = device_private(self);
	struct virtio_softc * const sc = &psc->sc_sc;
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
	sc->sc_ops = &virtio_pci_ops;
	psc->sc_pa = *pa;
	psc->sc_iot = pa->pa_iot;
	if (pci_dma64_available(pa))
		sc->sc_dmat = pa->pa_dmat64;
	else
		sc->sc_dmat = pa->pa_dmat;
	psc->sc_config_offset = VIRTIO_CONFIG_DEVICE_CONFIG_NOMSI;

	if (pci_mapreg_map(pa, PCI_MAPREG_START, PCI_MAPREG_TYPE_IO, 0,
			   &psc->sc_iot, &psc->sc_ioh, NULL, &psc->sc_iosize)) {
		aprint_error_dev(self, "can't map i/o space\n");
		return;
	}

	virtio_device_reset(sc);
	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_ACK);
	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_DRIVER);

	sc->sc_childdevid = PCI_SUBSYS_ID(id);
	sc->sc_child = NULL;
	virtio_pci_rescan(self, "virtio", 0);
	return;
}

/* ARGSUSED */
static int
virtio_pci_rescan(device_t self, const char *attr, const int *scan_flags)
{
	struct virtio_pci_softc * const psc = device_private(self);
	struct virtio_softc * const sc = &psc->sc_sc;
	struct virtio_attach_args va;

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
	KASSERT(psc->sc_ihs_num != 0);

	return 0;
}


static int
virtio_pci_detach(device_t self, int flags)
{
	struct virtio_pci_softc * const psc = device_private(self);
	struct virtio_softc * const sc = &psc->sc_sc;
	int r;

	if (sc->sc_child != NULL) {
		r = config_detach(sc->sc_child, flags);
		if (r)
			return r;
	}

	/* Check that child detached properly */
	KASSERT(sc->sc_child == NULL);
	KASSERT(sc->sc_vqs == NULL);
	KASSERT(psc->sc_ihs_num == 0);

	if (psc->sc_iosize)
		bus_space_unmap(psc->sc_iot, psc->sc_ioh, psc->sc_iosize);
	psc->sc_iosize = 0;

	return 0;
}

static void
virtio_pci_kick(struct virtio_softc *sc, uint16_t idx)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;

	bus_space_write_stream_2(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_QUEUE_NOTIFY, idx);
}

static uint8_t
virtio_pci_read_device_config_1(struct virtio_softc *sc, int index)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	return bus_space_read_stream_1(psc->sc_iot, psc->sc_ioh,
	    psc->sc_config_offset + index);
}

static uint16_t
virtio_pci_read_device_config_2(struct virtio_softc *sc, int index)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	return bus_space_read_stream_2(psc->sc_iot, psc->sc_ioh,
	    psc->sc_config_offset + index);
}

static uint32_t
virtio_pci_read_device_config_4(struct virtio_softc *sc, int index)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	return bus_space_read_stream_4(psc->sc_iot, psc->sc_ioh,
	    psc->sc_config_offset + index);
}

static uint64_t
virtio_pci_read_device_config_8(struct virtio_softc *sc, int index)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	uint64_t r;

	r = bus_space_read_stream_4(psc->sc_iot, psc->sc_ioh,
	    psc->sc_config_offset + index + REG_HI_OFF);
	r <<= 32;
	r |= bus_space_read_stream_4(psc->sc_iot, psc->sc_ioh,
	    psc->sc_config_offset + index + REG_LO_OFF);

	return r;
}

static void
virtio_pci_write_device_config_1(struct virtio_softc *sc, int index,
    uint8_t value)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;

	bus_space_write_stream_1(psc->sc_iot, psc->sc_ioh,
	    psc->sc_config_offset + index, value);
}

static void
virtio_pci_write_device_config_2(struct virtio_softc *sc, int index,
    uint16_t value)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;

	bus_space_write_stream_2(psc->sc_iot, psc->sc_ioh,
	    psc->sc_config_offset + index, value);
}

static void
virtio_pci_write_device_config_4(struct virtio_softc *sc, int index,
    uint32_t value)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;

	bus_space_write_stream_4(psc->sc_iot, psc->sc_ioh,
	    psc->sc_config_offset + index, value);
}

static void
virtio_pci_write_device_config_8(struct virtio_softc *sc, int index,
    uint64_t value)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;

	bus_space_write_stream_4(psc->sc_iot, psc->sc_ioh,
	    psc->sc_config_offset + index + REG_LO_OFF,
	    value & 0xffffffff);
	bus_space_write_stream_4(psc->sc_iot, psc->sc_ioh,
	    psc->sc_config_offset + index + REG_HI_OFF,
	    value >> 32);
}

static uint16_t
virtio_pci_read_queue_size(struct virtio_softc *sc, uint16_t idx)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;

	bus_space_write_stream_2(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_QUEUE_SELECT, idx);
	return bus_space_read_stream_2(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_QUEUE_SIZE);
}

static void
virtio_pci_setup_queue(struct virtio_softc *sc, uint16_t idx, uint32_t addr)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;

	bus_space_write_stream_2(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_QUEUE_SELECT, idx);
	bus_space_write_stream_4(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_QUEUE_ADDRESS, addr);

	if (psc->sc_ihs_num > 1) {
		int vec = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;
		if (false) /* (for per-vq vectors) */
			vec += idx;
		bus_space_write_stream_2(psc->sc_iot, psc->sc_ioh,
		    VIRTIO_CONFIG_MSI_QUEUE_VECTOR, vec);
	}
}

static void
virtio_pci_set_status(struct virtio_softc *sc, int status)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	int old = 0;

	if (status != 0) {
	    old = bus_space_read_stream_1(psc->sc_iot, psc->sc_ioh,
		VIRTIO_CONFIG_DEVICE_STATUS);
	}
	bus_space_write_stream_1(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_DEVICE_STATUS, status|old);
}

static uint32_t
virtio_pci_negotiate_features(struct virtio_softc *sc, uint32_t guest_features)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	uint32_t r;

	r = bus_space_read_stream_4(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_DEVICE_FEATURES);
	r &= guest_features;
	bus_space_write_stream_4(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_GUEST_FEATURES, r);

	return r;
}


static int
virtio_pci_setup_msix_vectors(struct virtio_softc *sc)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	int offset, vector, ret, qid;

	offset = VIRTIO_CONFIG_MSI_CONFIG_VECTOR;
	vector = VIRTIO_MSIX_CONFIG_VECTOR_INDEX;

	bus_space_write_stream_2(psc->sc_iot, psc->sc_ioh, offset, vector);
	ret = bus_space_read_stream_2(psc->sc_iot, psc->sc_ioh, offset);
	aprint_debug_dev(sc->sc_dev, "expected=%d, actual=%d\n",
	    vector, ret);
	if (ret != vector)
		return -1;

	for (qid = 0; qid < sc->sc_nvqs; qid++) {
		offset = VIRTIO_CONFIG_QUEUE_SELECT;
		bus_space_write_stream_2(psc->sc_iot, psc->sc_ioh, offset, qid);

		offset = VIRTIO_CONFIG_MSI_QUEUE_VECTOR;
		vector = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;

		bus_space_write_stream_2(psc->sc_iot, psc->sc_ioh, offset, vector);
		ret = bus_space_read_stream_2(psc->sc_iot, psc->sc_ioh, offset);
		aprint_debug_dev(sc->sc_dev, "expected=%d, actual=%d\n",
		    vector, ret);
		if (ret != vector)
			return -1;
	}

	return 0;
}

static int
virtio_pci_setup_msix_interrupts(struct virtio_softc *sc,
    struct pci_attach_args *pa)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	device_t self = sc->sc_dev;
	pci_chipset_tag_t pc = pa->pa_pc;
	char intrbuf[PCI_INTRSTR_LEN];
	char const *intrstr;
	int idx;

	idx = VIRTIO_MSIX_CONFIG_VECTOR_INDEX;
	if (sc->sc_flags & VIRTIO_F_PCI_INTR_MPSAFE)
		pci_intr_setattr(pc, &psc->sc_ihp[idx], PCI_INTR_MPSAFE, true);

	psc->sc_ihs[idx] = pci_intr_establish_xname(pc, psc->sc_ihp[idx],
	    sc->sc_ipl, virtio_pci_msix_config_intr, sc, device_xname(sc->sc_dev));
	if (psc->sc_ihs[idx] == NULL) {
		aprint_error_dev(self, "couldn't establish MSI-X for config\n");
		goto error;
	}

	idx = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;
	if (sc->sc_flags & VIRTIO_F_PCI_INTR_MPSAFE)
		pci_intr_setattr(pc, &psc->sc_ihp[idx], PCI_INTR_MPSAFE, true);

	psc->sc_ihs[idx] = pci_intr_establish_xname(pc, psc->sc_ihp[idx],
	    sc->sc_ipl, virtio_pci_msix_queue_intr, sc, device_xname(sc->sc_dev));
	if (psc->sc_ihs[idx] == NULL) {
		aprint_error_dev(self, "couldn't establish MSI-X for queues\n");
		goto error;
	}

	if (virtio_pci_setup_msix_vectors(sc) != 0) {
		aprint_error_dev(self, "couldn't setup MSI-X vectors\n");
		goto error;
	}

	idx = VIRTIO_MSIX_CONFIG_VECTOR_INDEX;
	intrstr = pci_intr_string(pc, psc->sc_ihp[idx], intrbuf, sizeof(intrbuf));
	aprint_normal_dev(self, "config interrupting at %s\n", intrstr);
	idx = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;
	intrstr = pci_intr_string(pc, psc->sc_ihp[idx], intrbuf, sizeof(intrbuf));
	aprint_normal_dev(self, "queues interrupting at %s\n", intrstr);

	return 0;

error:
	idx = VIRTIO_MSIX_CONFIG_VECTOR_INDEX;
	if (psc->sc_ihs[idx] != NULL)
		pci_intr_disestablish(psc->sc_pa.pa_pc, psc->sc_ihs[idx]);
	idx = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;
	if (psc->sc_ihs[idx] != NULL)
		pci_intr_disestablish(psc->sc_pa.pa_pc, psc->sc_ihs[idx]);

	return -1;
}

static int
virtio_pci_setup_intx_interrupt(struct virtio_softc *sc,
    struct pci_attach_args *pa)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	device_t self = sc->sc_dev;
	pci_chipset_tag_t pc = pa->pa_pc;
	char intrbuf[PCI_INTRSTR_LEN];
	char const *intrstr;

	if (sc->sc_flags & VIRTIO_F_PCI_INTR_MPSAFE)
		pci_intr_setattr(pc, &psc->sc_ihp[0], PCI_INTR_MPSAFE, true);

	psc->sc_ihs[0] = pci_intr_establish_xname(pc, psc->sc_ihp[0],
	    sc->sc_ipl, virtio_pci_intr, sc, device_xname(sc->sc_dev));
	if (psc->sc_ihs[0] == NULL) {
		aprint_error_dev(self, "couldn't establish INTx\n");
		return -1;
	}

	intrstr = pci_intr_string(pc, psc->sc_ihp[0], intrbuf, sizeof(intrbuf));
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	return 0;
}

static int
virtio_pci_setup_interrupts(struct virtio_softc *sc)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	device_t self = sc->sc_dev;
	pci_chipset_tag_t pc = psc->sc_pa.pa_pc;
	int error;
	int nmsix;
	int counts[PCI_INTR_TYPE_SIZE];
	pci_intr_type_t max_type;

	nmsix = pci_msix_count(psc->sc_pa.pa_pc, psc->sc_pa.pa_tag);
	aprint_debug_dev(self, "pci_msix_count=%d\n", nmsix);

	/* We need at least two: one for config and the other for queues */
	if ((sc->sc_flags & VIRTIO_F_PCI_INTR_MSIX) == 0 || nmsix < 2) {
		/* Try INTx only */
		max_type = PCI_INTR_TYPE_INTX;
		counts[PCI_INTR_TYPE_INTX] = 1;
	} else {
		/* Try MSI-X first and INTx second */
		max_type = PCI_INTR_TYPE_MSIX;
		counts[PCI_INTR_TYPE_MSIX] = 2;
		counts[PCI_INTR_TYPE_MSI] = 0;
		counts[PCI_INTR_TYPE_INTX] = 1;
	}

retry:
	error = pci_intr_alloc(&psc->sc_pa, &psc->sc_ihp, counts, max_type);
	if (error != 0) {
		aprint_error_dev(self, "couldn't map interrupt\n");
		return -1;
	}

	if (pci_intr_type(pc, psc->sc_ihp[0]) == PCI_INTR_TYPE_MSIX) {
		psc->sc_ihs = kmem_alloc(sizeof(*psc->sc_ihs) * 2,
		    KM_SLEEP);

		error = virtio_pci_setup_msix_interrupts(sc, &psc->sc_pa);
		if (error != 0) {
			kmem_free(psc->sc_ihs, sizeof(*psc->sc_ihs) * 2);
			pci_intr_release(pc, psc->sc_ihp, 2);

			/* Retry INTx */
			max_type = PCI_INTR_TYPE_INTX;
			counts[PCI_INTR_TYPE_INTX] = 1;
			goto retry;
		}

		psc->sc_ihs_num = 2;
		psc->sc_config_offset = VIRTIO_CONFIG_DEVICE_CONFIG_MSI;
	} else if (pci_intr_type(pc, psc->sc_ihp[0]) == PCI_INTR_TYPE_INTX) {
		psc->sc_ihs = kmem_alloc(sizeof(*psc->sc_ihs) * 1,
		    KM_SLEEP);

		error = virtio_pci_setup_intx_interrupt(sc, &psc->sc_pa);
		if (error != 0) {
			kmem_free(psc->sc_ihs, sizeof(*psc->sc_ihs) * 1);
			pci_intr_release(pc, psc->sc_ihp, 1);
			return -1;
		}

		psc->sc_ihs_num = 1;
		psc->sc_config_offset = VIRTIO_CONFIG_DEVICE_CONFIG_NOMSI;
	}

	return 0;
}

static void
virtio_pci_free_interrupts(struct virtio_softc *sc)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;

	for (int i = 0; i < psc->sc_ihs_num; i++) {
		if (psc->sc_ihs[i] == NULL)
			continue;
		pci_intr_disestablish(psc->sc_pa.pa_pc, psc->sc_ihs[i]);
		psc->sc_ihs[i] = NULL;
	}

	if (psc->sc_ihs_num > 0)
		pci_intr_release(psc->sc_pa.pa_pc, psc->sc_ihp, psc->sc_ihs_num);

	if (psc->sc_ihs != NULL) {
		kmem_free(psc->sc_ihs, sizeof(*psc->sc_ihs) * psc->sc_ihs_num);
		psc->sc_ihs = NULL;
	}
	psc->sc_ihs_num = 0;
}

/*
 * Interrupt handler.
 */
static int
virtio_pci_intr(void *arg)
{
	struct virtio_softc *sc = arg;
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	int isr, r = 0;

	/* check and ack the interrupt */
	isr = bus_space_read_stream_1(psc->sc_iot, psc->sc_ioh,
			       VIRTIO_CONFIG_ISR_STATUS);
	if (isr == 0)
		return 0;
	if ((isr & VIRTIO_CONFIG_ISR_CONFIG_CHANGE) &&
	    (sc->sc_config_change != NULL))
		r = (sc->sc_config_change)(sc);
	if (sc->sc_intrhand != NULL) {
		if (sc->sc_soft_ih != NULL)
			softint_schedule(sc->sc_soft_ih);
		else
			r |= (sc->sc_intrhand)(sc);
	}

	return r;
}

static int
virtio_pci_msix_queue_intr(void *arg)
{
	struct virtio_softc *sc = arg;
	int r = 0;

	if (sc->sc_intrhand != NULL) {
		if (sc->sc_soft_ih != NULL)
			softint_schedule(sc->sc_soft_ih);
		else
			r |= (sc->sc_intrhand)(sc);
	}

	return r;
}

static int
virtio_pci_msix_config_intr(void *arg)
{
	struct virtio_softc *sc = arg;
	int r = 0;

	if (sc->sc_config_change != NULL)
		r = (sc->sc_config_change)(sc);
	return r;
}

MODULE(MODULE_CLASS_DRIVER, virtio_pci, "pci,virtio");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
virtio_pci_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

#ifdef _MODULE
	switch (cmd) {
	case MODULE_CMD_INIT:
		error = config_init_component(cfdriver_ioconf_virtio_pci,
		    cfattach_ioconf_virtio_pci, cfdata_ioconf_virtio_pci);
		break;
	case MODULE_CMD_FINI:
		error = config_fini_component(cfdriver_ioconf_virtio_pci,
		    cfattach_ioconf_virtio_pci, cfdata_ioconf_virtio_pci);
		break;
	default:
		error = ENOTTY;
		break;
	}
#endif

	return error;
}

