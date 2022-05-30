/* $NetBSD: virtio_pci.c,v 1.38 2022/05/30 20:28:18 riastradh Exp $ */

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * Copyright (c) 2012 Stefan Fritsch.
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
__KERNEL_RCSID(0, "$NetBSD: virtio_pci.c,v 1.38 2022/05/30 20:28:18 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/endian.h>
#include <sys/interrupt.h>
#include <sys/syslog.h>

#include <sys/device.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/pci/virtioreg.h> /* XXX: move to non-pci */
#include <dev/pci/virtio_pcireg.h>

#define VIRTIO_PRIVATE
#include <dev/pci/virtiovar.h> /* XXX: move to non-pci */


#define VIRTIO_PCI_LOG(_sc, _use_log, _fmt, _args...)	\
do {							\
	if ((_use_log)) {				\
		log(LOG_DEBUG, "%s: " _fmt,		\
		    device_xname((_sc)->sc_dev),	\
		    ##_args);				\
	} else {					\
		aprint_error_dev((_sc)->sc_dev,		\
		    _fmt, ##_args);			\
	}						\
} while(0)

static int	virtio_pci_match(device_t, cfdata_t, void *);
static void	virtio_pci_attach(device_t, device_t, void *);
static int	virtio_pci_rescan(device_t, const char *, const int *);
static int	virtio_pci_detach(device_t, int);


#define NMAPREG		((PCI_MAPREG_END - PCI_MAPREG_START) / \
				sizeof(pcireg_t))
struct virtio_pci_softc {
	struct virtio_softc	sc_sc;

	/* IO space */
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_size_t		sc_iosize;
	bus_size_t		sc_mapped_iosize;

	/* BARs */
	bus_space_tag_t		sc_bars_iot[NMAPREG];
	bus_space_handle_t	sc_bars_ioh[NMAPREG];
	bus_size_t		sc_bars_iosize[NMAPREG];

	/* notify space */
	bus_space_tag_t		sc_notify_iot;
	bus_space_handle_t	sc_notify_ioh;
	bus_size_t		sc_notify_iosize;
	uint32_t		sc_notify_off_multiplier;

	/* isr space */
	bus_space_tag_t		sc_isr_iot;
	bus_space_handle_t	sc_isr_ioh;
	bus_size_t		sc_isr_iosize;

	/* generic */
	struct pci_attach_args	sc_pa;
	pci_intr_handle_t	*sc_ihp;
	void			**sc_ihs;
	int			sc_ihs_num;
	int			sc_devcfg_offset;	/* for 0.9 */
};

static int	virtio_pci_attach_09(device_t, void *);
static void	virtio_pci_kick_09(struct virtio_softc *, uint16_t);
static uint16_t	virtio_pci_read_queue_size_09(struct virtio_softc *, uint16_t);
static void	virtio_pci_setup_queue_09(struct virtio_softc *, uint16_t, uint64_t);
static void	virtio_pci_set_status_09(struct virtio_softc *, int);
static void	virtio_pci_negotiate_features_09(struct virtio_softc *, uint64_t);

static int	virtio_pci_attach_10(device_t, void *);
static void	virtio_pci_kick_10(struct virtio_softc *, uint16_t);
static uint16_t	virtio_pci_read_queue_size_10(struct virtio_softc *, uint16_t);
static void	virtio_pci_setup_queue_10(struct virtio_softc *, uint16_t, uint64_t);
static void	virtio_pci_set_status_10(struct virtio_softc *, int);
static void	virtio_pci_negotiate_features_10(struct virtio_softc *, uint64_t);
static int	virtio_pci_find_cap(struct virtio_pci_softc *psc, int cfg_type, void *buf, int buflen);

static int	virtio_pci_alloc_interrupts(struct virtio_softc *);
static void	virtio_pci_free_interrupts(struct virtio_softc *);
static int	virtio_pci_adjust_config_region(struct virtio_pci_softc *psc);
static int	virtio_pci_intr(void *arg);
static int	virtio_pci_msix_queue_intr(void *);
static int	virtio_pci_msix_config_intr(void *);
static int	virtio_pci_setup_interrupts_09(struct virtio_softc *, int);
static int	virtio_pci_setup_interrupts_10(struct virtio_softc *, int);
static int	virtio_pci_establish_msix_interrupts(struct virtio_softc *,
		    struct pci_attach_args *);
static int	virtio_pci_establish_intx_interrupt(struct virtio_softc *,
		    struct pci_attach_args *);
static bool	virtio_pci_msix_enabled(struct virtio_pci_softc *);

#define VIRTIO_MSIX_CONFIG_VECTOR_INDEX	0
#define VIRTIO_MSIX_QUEUE_VECTOR_INDEX	1

/*
 * When using PCI attached virtio on aarch64-eb under Qemu, the IO space
 * suddenly read BIG_ENDIAN where it should stay LITTLE_ENDIAN. The data read
 * 1 byte at a time seem OK but reading bigger lengths result in swapped
 * endian. This is most notable on reading 8 byters since we can't use
 * bus_space_{read,write}_8().
 */

#if defined(__aarch64__) && BYTE_ORDER == BIG_ENDIAN
#	define READ_ENDIAN_09	BIG_ENDIAN	/* should be LITTLE_ENDIAN */
#	define READ_ENDIAN_10	BIG_ENDIAN
#	define STRUCT_ENDIAN_09	BIG_ENDIAN
#	define STRUCT_ENDIAN_10	LITTLE_ENDIAN
#elif BYTE_ORDER == BIG_ENDIAN
#	define READ_ENDIAN_09	LITTLE_ENDIAN
#	define READ_ENDIAN_10	BIG_ENDIAN
#	define STRUCT_ENDIAN_09	BIG_ENDIAN
#	define STRUCT_ENDIAN_10	LITTLE_ENDIAN
#else /* little endian */
#	define READ_ENDIAN_09	LITTLE_ENDIAN
#	define READ_ENDIAN_10	LITTLE_ENDIAN
#	define STRUCT_ENDIAN_09	LITTLE_ENDIAN
#	define STRUCT_ENDIAN_10	LITTLE_ENDIAN
#endif


CFATTACH_DECL3_NEW(virtio_pci, sizeof(struct virtio_pci_softc),
    virtio_pci_match, virtio_pci_attach, virtio_pci_detach, NULL,
    virtio_pci_rescan, NULL, DVF_DETACH_SHUTDOWN);

static const struct virtio_ops virtio_pci_ops_09 = {
	.kick = virtio_pci_kick_09,
	.read_queue_size = virtio_pci_read_queue_size_09,
	.setup_queue = virtio_pci_setup_queue_09,
	.set_status = virtio_pci_set_status_09,
	.neg_features = virtio_pci_negotiate_features_09,
	.alloc_interrupts = virtio_pci_alloc_interrupts,
	.free_interrupts = virtio_pci_free_interrupts,
	.setup_interrupts = virtio_pci_setup_interrupts_09,
};

static const struct virtio_ops virtio_pci_ops_10 = {
	.kick = virtio_pci_kick_10,
	.read_queue_size = virtio_pci_read_queue_size_10,
	.setup_queue = virtio_pci_setup_queue_10,
	.set_status = virtio_pci_set_status_10,
	.neg_features = virtio_pci_negotiate_features_10,
	.alloc_interrupts = virtio_pci_alloc_interrupts,
	.free_interrupts = virtio_pci_free_interrupts,
	.setup_interrupts = virtio_pci_setup_interrupts_10,
};

static int
virtio_pci_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa;

	pa = (struct pci_attach_args *)aux;
	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_QUMRANET:
		/* Transitional devices MUST have a PCI Revision ID of 0. */
		if (((PCI_PRODUCT_QUMRANET_VIRTIO_1000 <=
		      PCI_PRODUCT(pa->pa_id)) &&
		     (PCI_PRODUCT(pa->pa_id) <=
		      PCI_PRODUCT_QUMRANET_VIRTIO_103F)) &&
	              PCI_REVISION(pa->pa_class) == 0)
			return 1;
		/*
		 * Non-transitional devices SHOULD have a PCI Revision
		 * ID of 1 or higher.  Drivers MUST match any PCI
		 * Revision ID value.
		 */
		if (((PCI_PRODUCT_QUMRANET_VIRTIO_1040 <=
		      PCI_PRODUCT(pa->pa_id)) &&
		     (PCI_PRODUCT(pa->pa_id) <=
		      PCI_PRODUCT_QUMRANET_VIRTIO_107F)) &&
		      /* XXX: TODO */
		      PCI_REVISION(pa->pa_class) == 1)
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
	int ret;
	pcireg_t id;
	pcireg_t csr;

	revision = PCI_REVISION(pa->pa_class);
	switch (revision) {
	case 0:
		/* subsystem ID shows what I am */
		id = PCI_SUBSYS_ID(pci_conf_read(pc, tag, PCI_SUBSYS_ID_REG));
		break;
	case 1:
		/* pci product number shows what I am */
		id = PCI_PRODUCT(pa->pa_id) - PCI_PRODUCT_QUMRANET_VIRTIO_1040;
		break;
	default:
		aprint_normal(": unknown revision 0x%02x; giving up\n",
			      revision);
		return;
	}

	aprint_normal("\n");
	aprint_naive("\n");
	virtio_print_device_type(self, id, revision);

	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	csr |= PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_IO_ENABLE;
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);

	sc->sc_dev = self;
	psc->sc_pa = *pa;
	psc->sc_iot = pa->pa_iot;

	sc->sc_dmat = pa->pa_dmat;
	if (pci_dma64_available(pa))
		sc->sc_dmat = pa->pa_dmat64;

	/* attach is dependent on revision */
	ret = 0;
	if (revision == 1) {
		/* try to attach 1.0 */
		ret = virtio_pci_attach_10(self, aux);
	}
	if (ret == 0 && revision == 0) {
		/* revision 0 means 0.9 only or both 0.9 and 1.0 */
		ret = virtio_pci_attach_09(self, aux);
	}
	if (ret) {
		aprint_error_dev(self, "cannot attach (%d)\n", ret);
		return;
	}
	KASSERT(sc->sc_ops);

	/* preset config region */
	psc->sc_devcfg_offset = VIRTIO_CONFIG_DEVICE_CONFIG_NOMSI;
	if (virtio_pci_adjust_config_region(psc))
		return;

	/* generic */
	virtio_device_reset(sc);
	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_ACK);
	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_DRIVER);

	sc->sc_childdevid = id;
	sc->sc_child = NULL;
	virtio_pci_rescan(self, NULL, NULL);
	return;
}

/* ARGSUSED */
static int
virtio_pci_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct virtio_pci_softc * const psc = device_private(self);
	struct virtio_softc * const sc = &psc->sc_sc;
	struct virtio_attach_args va;

	if (sc->sc_child)	/* Child already attached? */
		return 0;

	memset(&va, 0, sizeof(va));
	va.sc_childdevid = sc->sc_childdevid;

	config_found(self, &va, NULL, CFARGS_NONE);

	if (virtio_attach_failed(sc))
		return 0;

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
		bus_space_unmap(psc->sc_iot, psc->sc_ioh,
			psc->sc_mapped_iosize);
	psc->sc_iosize = 0;

	return 0;
}


static int
virtio_pci_attach_09(device_t self, void *aux)
	//struct virtio_pci_softc *psc, struct pci_attach_args *pa)
{
	struct virtio_pci_softc * const psc = device_private(self);
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	struct virtio_softc * const sc = &psc->sc_sc;
//	pci_chipset_tag_t pc = pa->pa_pc;
//	pcitag_t tag = pa->pa_tag;

	/* complete IO region */
	if (pci_mapreg_map(pa, PCI_MAPREG_START, PCI_MAPREG_TYPE_IO, 0,
			   &psc->sc_iot, &psc->sc_ioh, NULL, &psc->sc_iosize)) {
		aprint_error_dev(self, "can't map i/o space\n");
		return EIO;
	}
	psc->sc_mapped_iosize = psc->sc_iosize;

	/* queue space */
	if (bus_space_subregion(psc->sc_iot, psc->sc_ioh,
			VIRTIO_CONFIG_QUEUE_NOTIFY, 2, &psc->sc_notify_ioh)) {
		aprint_error_dev(self, "can't map notify i/o space\n");
		return EIO;
	}
	psc->sc_notify_iosize = 2;
	psc->sc_notify_iot = psc->sc_iot;

	/* ISR space */
	if (bus_space_subregion(psc->sc_iot, psc->sc_ioh,
			VIRTIO_CONFIG_ISR_STATUS, 1, &psc->sc_isr_ioh)) {
		aprint_error_dev(self, "can't map isr i/o space\n");
		return EIO;
	}
	psc->sc_isr_iosize = 1;
	psc->sc_isr_iot = psc->sc_iot;

	/* set our version 0.9 ops */
	sc->sc_ops = &virtio_pci_ops_09;
	sc->sc_bus_endian    = READ_ENDIAN_09;
	sc->sc_struct_endian = STRUCT_ENDIAN_09;
	return 0;
}


static int
virtio_pci_attach_10(device_t self, void *aux)
{
	struct virtio_pci_softc * const psc = device_private(self);
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	struct virtio_softc * const sc = &psc->sc_sc;
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;

	struct virtio_pci_cap common, isr, device;
	struct virtio_pci_notify_cap notify;
	int have_device_cfg = 0;
	bus_size_t bars[NMAPREG] = { 0 };
	int bars_idx[NMAPREG] = { 0 };
	struct virtio_pci_cap *caps[] = { &common, &isr, &device, &notify.cap };
	int i, j, ret = 0;

	if (virtio_pci_find_cap(psc, VIRTIO_PCI_CAP_COMMON_CFG,
			&common, sizeof(common)))
		return ENODEV;
	if (virtio_pci_find_cap(psc, VIRTIO_PCI_CAP_NOTIFY_CFG,
			&notify, sizeof(notify)))
		return ENODEV;
	if (virtio_pci_find_cap(psc, VIRTIO_PCI_CAP_ISR_CFG,
			&isr, sizeof(isr)))
		return ENODEV;
	if (virtio_pci_find_cap(psc, VIRTIO_PCI_CAP_DEVICE_CFG,
			&device, sizeof(device)))
		memset(&device, 0, sizeof(device));
	else
		have_device_cfg = 1;

	/* Figure out which bars we need to map */
	for (i = 0; i < __arraycount(caps); i++) {
		int bar = caps[i]->bar;
		bus_size_t len = caps[i]->offset + caps[i]->length;
		if (caps[i]->length == 0)
			continue;
		if (bars[bar] < len)
			bars[bar] = len;
	}

	for (i = j = 0; i < __arraycount(bars); i++) {
		int reg;
		pcireg_t type;
		if (bars[i] == 0)
			continue;
		reg = PCI_BAR(i);
		type = pci_mapreg_type(pc, tag, reg);
		if (pci_mapreg_map(pa, reg, type, 0,
				&psc->sc_bars_iot[j], &psc->sc_bars_ioh[j],
				NULL, &psc->sc_bars_iosize[j])) {
			aprint_error_dev(self, "can't map bar %u \n", i);
			ret = EIO;
			goto err;
		}
		aprint_debug_dev(self,
		    "bar[%d]: iot %p, size 0x%" PRIxBUSSIZE "\n",
		    j, psc->sc_bars_iot[j], psc->sc_bars_iosize[j]);
		bars_idx[i] = j;
		j++;
	}

	i = bars_idx[notify.cap.bar];
	if (bus_space_subregion(psc->sc_bars_iot[i], psc->sc_bars_ioh[i],
			notify.cap.offset, notify.cap.length,
			&psc->sc_notify_ioh)) {
		aprint_error_dev(self, "can't map notify i/o space\n");
		ret = EIO;
		goto err;
	}
	psc->sc_notify_iosize = notify.cap.length;
	psc->sc_notify_iot = psc->sc_bars_iot[i];
	psc->sc_notify_off_multiplier = le32toh(notify.notify_off_multiplier);

	if (have_device_cfg) {
		i = bars_idx[device.bar];
		if (bus_space_subregion(psc->sc_bars_iot[i], psc->sc_bars_ioh[i],
				device.offset, device.length,
				&sc->sc_devcfg_ioh)) {
			aprint_error_dev(self, "can't map devcfg i/o space\n");
			ret = EIO;
			goto err;
		}
		aprint_debug_dev(self,
			"device.offset = 0x%x, device.length = 0x%x\n",
			device.offset, device.length);
		sc->sc_devcfg_iosize = device.length;
		sc->sc_devcfg_iot = psc->sc_bars_iot[i];
	}

	i = bars_idx[isr.bar];
	if (bus_space_subregion(psc->sc_bars_iot[i], psc->sc_bars_ioh[i],
			isr.offset, isr.length, &psc->sc_isr_ioh)) {
		aprint_error_dev(self, "can't map isr i/o space\n");
		ret = EIO;
		goto err;
	}
	psc->sc_isr_iosize = isr.length;
	psc->sc_isr_iot = psc->sc_bars_iot[i];

	i = bars_idx[common.bar];
	if (bus_space_subregion(psc->sc_bars_iot[i], psc->sc_bars_ioh[i],
			common.offset, common.length, &psc->sc_ioh)) {
		aprint_error_dev(self, "can't map common i/o space\n");
		ret = EIO;
		goto err;
	}
	psc->sc_iosize = common.length;
	psc->sc_iot = psc->sc_bars_iot[i];
	psc->sc_mapped_iosize = psc->sc_bars_iosize[i];

	psc->sc_sc.sc_version_1 = 1;

	/* set our version 1.0 ops */
	sc->sc_ops = &virtio_pci_ops_10;
	sc->sc_bus_endian    = READ_ENDIAN_10;
	sc->sc_struct_endian = STRUCT_ENDIAN_10;
	return 0;

err:
	/* undo our pci_mapreg_map()s */ 
	for (i = 0; i < __arraycount(bars); i++) {
		if (psc->sc_bars_iosize[i] == 0)
			continue;
		bus_space_unmap(psc->sc_bars_iot[i], psc->sc_bars_ioh[i],
				psc->sc_bars_iosize[i]);
	}
	return ret;
}

/* v1.0 attach helper */
static int
virtio_pci_find_cap(struct virtio_pci_softc *psc, int cfg_type, void *buf, int buflen)
{
	device_t self = psc->sc_sc.sc_dev;
	pci_chipset_tag_t pc = psc->sc_pa.pa_pc;
	pcitag_t tag = psc->sc_pa.pa_tag;
	unsigned int offset, i, len;
	union {
		pcireg_t reg[8];
		struct virtio_pci_cap vcap;
	} *v = buf;

	if (buflen < sizeof(struct virtio_pci_cap))
		return ERANGE;

	if (!pci_get_capability(pc, tag, PCI_CAP_VENDSPEC, &offset, &v->reg[0]))
		return ENOENT;

	do {
		for (i = 0; i < 4; i++)
			v->reg[i] =
				le32toh(pci_conf_read(pc, tag, offset + i * 4));
		if (v->vcap.cfg_type == cfg_type)
			break;
		offset = v->vcap.cap_next;
	} while (offset != 0);

	if (offset == 0)
		return ENOENT;

	if (v->vcap.cap_len > sizeof(struct virtio_pci_cap)) {
		len = roundup(v->vcap.cap_len, sizeof(pcireg_t));
		if (len > buflen) {
			aprint_error_dev(self, "%s cap too large\n", __func__);
			return ERANGE;
		}
		for (i = 4; i < len / sizeof(pcireg_t);  i++)
			v->reg[i] =
				le32toh(pci_conf_read(pc, tag, offset + i * 4));
	}

	/* endian fixup */
	v->vcap.offset = le32toh(v->vcap.offset);
	v->vcap.length = le32toh(v->vcap.length);
	return 0;
}


/* -------------------------------------
 * Version 0.9 support
 * -------------------------------------*/

static void
virtio_pci_kick_09(struct virtio_softc *sc, uint16_t idx)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;

	bus_space_write_2(psc->sc_notify_iot, psc->sc_notify_ioh, 0, idx);
}

/* only applicable for v 0.9 but also called for 1.0 */
static int
virtio_pci_adjust_config_region(struct virtio_pci_softc *psc)
{
	struct virtio_softc * const sc = &psc->sc_sc;
	device_t self = sc->sc_dev;

	if (psc->sc_sc.sc_version_1)
		return 0;

	sc->sc_devcfg_iosize = psc->sc_iosize - psc->sc_devcfg_offset;
	sc->sc_devcfg_iot = psc->sc_iot;
	if (bus_space_subregion(psc->sc_iot, psc->sc_ioh,
			psc->sc_devcfg_offset, sc->sc_devcfg_iosize,
			&sc->sc_devcfg_ioh)) {
		aprint_error_dev(self, "can't map config i/o space\n");
		return EIO;
	}

	return 0;
}

static uint16_t
virtio_pci_read_queue_size_09(struct virtio_softc *sc, uint16_t idx)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;

	bus_space_write_2(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_QUEUE_SELECT, idx);
	return bus_space_read_2(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_QUEUE_SIZE);
}

static void
virtio_pci_setup_queue_09(struct virtio_softc *sc, uint16_t idx, uint64_t addr)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;

	bus_space_write_2(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_QUEUE_SELECT, idx);
	bus_space_write_4(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_QUEUE_ADDRESS, addr / VIRTIO_PAGE_SIZE);

	if (psc->sc_ihs_num > 1) {
		int vec = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;
		if (sc->sc_child_mq)
			vec += idx;
		bus_space_write_2(psc->sc_iot, psc->sc_ioh,
		    VIRTIO_CONFIG_MSI_QUEUE_VECTOR, vec);
	}
}

static void
virtio_pci_set_status_09(struct virtio_softc *sc, int status)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	int old = 0;

	if (status != 0) {
	    old = bus_space_read_1(psc->sc_iot, psc->sc_ioh,
		VIRTIO_CONFIG_DEVICE_STATUS);
	}
	bus_space_write_1(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_DEVICE_STATUS, status|old);
}

static void
virtio_pci_negotiate_features_09(struct virtio_softc *sc, uint64_t guest_features)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	uint32_t r;

	r = bus_space_read_4(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_DEVICE_FEATURES);

	r &= guest_features;

	bus_space_write_4(psc->sc_iot, psc->sc_ioh,
	    VIRTIO_CONFIG_GUEST_FEATURES, r);

	sc->sc_active_features = r;
}

/* -------------------------------------
 * Version 1.0 support
 * -------------------------------------*/

static void
virtio_pci_kick_10(struct virtio_softc *sc, uint16_t idx)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	unsigned offset = sc->sc_vqs[idx].vq_notify_off *
		psc->sc_notify_off_multiplier;

	bus_space_write_2(psc->sc_notify_iot, psc->sc_notify_ioh, offset, idx);
}


static uint16_t
virtio_pci_read_queue_size_10(struct virtio_softc *sc, uint16_t idx)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	bus_space_tag_t	   iot = psc->sc_iot;
	bus_space_handle_t ioh = psc->sc_ioh;

	bus_space_write_2(iot, ioh, VIRTIO_CONFIG1_QUEUE_SELECT, idx);
	return bus_space_read_2(iot, ioh, VIRTIO_CONFIG1_QUEUE_SIZE);
}

/*
 * By definition little endian only in v1.0.  NB: "MAY" in the text
 * below refers to "independently" (i.e. the order of accesses) not
 * "32-bit" (which is restricted by the earlier "MUST").
 *
 * 4.1.3.1 Driver Requirements: PCI Device Layout
 *
 * For device configuration access, the driver MUST use ... 32-bit
 * wide and aligned accesses for ... 64-bit wide fields.  For 64-bit
 * fields, the driver MAY access each of the high and low 32-bit parts
 * of the field independently.
 */
static __inline void
virtio_pci_bus_space_write_8(bus_space_tag_t iot, bus_space_handle_t ioh,
     bus_size_t offset, uint64_t value)
{
#if _QUAD_HIGHWORD
	bus_space_write_4(iot, ioh, offset, BUS_ADDR_LO32(value));
	bus_space_write_4(iot, ioh, offset + 4, BUS_ADDR_HI32(value));
#else
	bus_space_write_4(iot, ioh, offset, BUS_ADDR_HI32(value));
	bus_space_write_4(iot, ioh, offset + 4, BUS_ADDR_LO32(value));
#endif
}

static void
virtio_pci_setup_queue_10(struct virtio_softc *sc, uint16_t idx, uint64_t addr)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	struct virtqueue *vq = &sc->sc_vqs[idx];
	bus_space_tag_t	   iot = psc->sc_iot;
	bus_space_handle_t ioh = psc->sc_ioh;
	KASSERT(vq->vq_index == idx);

	bus_space_write_2(iot, ioh, VIRTIO_CONFIG1_QUEUE_SELECT, vq->vq_index);
	if (addr == 0) {
		bus_space_write_2(iot, ioh, VIRTIO_CONFIG1_QUEUE_ENABLE, 0);
		virtio_pci_bus_space_write_8(iot, ioh,
		    VIRTIO_CONFIG1_QUEUE_DESC,   0);
		virtio_pci_bus_space_write_8(iot, ioh,
		    VIRTIO_CONFIG1_QUEUE_AVAIL,  0);
		virtio_pci_bus_space_write_8(iot, ioh,
		    VIRTIO_CONFIG1_QUEUE_USED,   0);
	} else {
		virtio_pci_bus_space_write_8(iot, ioh,
			VIRTIO_CONFIG1_QUEUE_DESC, addr);
		virtio_pci_bus_space_write_8(iot, ioh,
			VIRTIO_CONFIG1_QUEUE_AVAIL, addr + vq->vq_availoffset);
		virtio_pci_bus_space_write_8(iot, ioh,
			VIRTIO_CONFIG1_QUEUE_USED, addr + vq->vq_usedoffset);
		bus_space_write_2(iot, ioh,
			VIRTIO_CONFIG1_QUEUE_ENABLE, 1);
		vq->vq_notify_off = bus_space_read_2(iot, ioh,
			VIRTIO_CONFIG1_QUEUE_NOTIFY_OFF);
	}

	if (psc->sc_ihs_num > 1) {
		int vec = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;
		if (sc->sc_child_mq)
			vec += idx;
		bus_space_write_2(iot, ioh,
			VIRTIO_CONFIG1_QUEUE_MSIX_VECTOR, vec);
	}
}

static void
virtio_pci_set_status_10(struct virtio_softc *sc, int status)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	bus_space_tag_t	   iot = psc->sc_iot;
	bus_space_handle_t ioh = psc->sc_ioh;
	int old = 0;

	if (status)
		old = bus_space_read_1(iot, ioh, VIRTIO_CONFIG1_DEVICE_STATUS);
	bus_space_write_1(iot, ioh, VIRTIO_CONFIG1_DEVICE_STATUS, status | old);
}

void
virtio_pci_negotiate_features_10(struct virtio_softc *sc, uint64_t guest_features)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	device_t self          =  sc->sc_dev;
	bus_space_tag_t	   iot = psc->sc_iot;
	bus_space_handle_t ioh = psc->sc_ioh;
	uint64_t host, negotiated, device_status;

	guest_features |= VIRTIO_F_VERSION_1;
	/* notify on empty is 0.9 only */
	guest_features &= ~VIRTIO_F_NOTIFY_ON_EMPTY;
	sc->sc_active_features = 0;

	bus_space_write_4(iot, ioh, VIRTIO_CONFIG1_DEVICE_FEATURE_SELECT, 0);
	host = bus_space_read_4(iot, ioh, VIRTIO_CONFIG1_DEVICE_FEATURE);
	bus_space_write_4(iot, ioh, VIRTIO_CONFIG1_DEVICE_FEATURE_SELECT, 1);
	host |= (uint64_t)
		bus_space_read_4(iot, ioh, VIRTIO_CONFIG1_DEVICE_FEATURE) << 32;

	negotiated = host & guest_features;

	bus_space_write_4(iot, ioh, VIRTIO_CONFIG1_DRIVER_FEATURE_SELECT, 0);
	bus_space_write_4(iot, ioh, VIRTIO_CONFIG1_DRIVER_FEATURE,
			negotiated & 0xffffffff);
	bus_space_write_4(iot, ioh, VIRTIO_CONFIG1_DRIVER_FEATURE_SELECT, 1);
	bus_space_write_4(iot, ioh, VIRTIO_CONFIG1_DRIVER_FEATURE,
			negotiated >> 32);
	virtio_pci_set_status_10(sc, VIRTIO_CONFIG_DEVICE_STATUS_FEATURES_OK);

	device_status = bus_space_read_1(iot, ioh, VIRTIO_CONFIG1_DEVICE_STATUS);
	if ((device_status & VIRTIO_CONFIG_DEVICE_STATUS_FEATURES_OK) == 0) {
		aprint_error_dev(self, "feature negotiation failed\n");
		bus_space_write_1(iot, ioh, VIRTIO_CONFIG1_DEVICE_STATUS,
				VIRTIO_CONFIG_DEVICE_STATUS_FAILED);
		return;
	}

	if ((negotiated & VIRTIO_F_VERSION_1) == 0) {
		aprint_error_dev(self, "host rejected version 1\n");
		bus_space_write_1(iot, ioh, VIRTIO_CONFIG1_DEVICE_STATUS,
				VIRTIO_CONFIG_DEVICE_STATUS_FAILED);
		return;
	}

	sc->sc_active_features = negotiated;
	return;
}


/* -------------------------------------
 * Generic PCI interrupt code
 * -------------------------------------*/

static int
virtio_pci_setup_interrupts_10(struct virtio_softc *sc, int reinit)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	bus_space_tag_t	   iot = psc->sc_iot;
	bus_space_handle_t ioh = psc->sc_ioh;
	int vector, ret, qid;

	if (!virtio_pci_msix_enabled(psc))
		return 0;

	vector = VIRTIO_MSIX_CONFIG_VECTOR_INDEX;
	bus_space_write_2(iot, ioh,
		VIRTIO_CONFIG1_CONFIG_MSIX_VECTOR, vector);
	ret = bus_space_read_2(iot, ioh, VIRTIO_CONFIG1_CONFIG_MSIX_VECTOR);
	if (ret != vector) {
		VIRTIO_PCI_LOG(sc, reinit,
		    "can't set config msix vector\n");
		return -1;
	}

	for (qid = 0; qid < sc->sc_nvqs; qid++) {
		vector = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;

		if (sc->sc_child_mq)
			vector += qid;
		bus_space_write_2(iot, ioh, VIRTIO_CONFIG1_QUEUE_SELECT, qid);
		bus_space_write_2(iot, ioh, VIRTIO_CONFIG1_QUEUE_MSIX_VECTOR,
			vector);
		ret = bus_space_read_2(iot, ioh,
			VIRTIO_CONFIG1_QUEUE_MSIX_VECTOR);
		if (ret != vector) {
			VIRTIO_PCI_LOG(sc, reinit, "can't set queue %d "
			    "msix vector\n", qid);
			return -1;
		}
	}

	return 0;
}

static int
virtio_pci_setup_interrupts_09(struct virtio_softc *sc, int reinit)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	int offset, vector, ret, qid;

	if (!virtio_pci_msix_enabled(psc))
		return 0;

	offset = VIRTIO_CONFIG_MSI_CONFIG_VECTOR;
	vector = VIRTIO_MSIX_CONFIG_VECTOR_INDEX;

	bus_space_write_2(psc->sc_iot, psc->sc_ioh, offset, vector);
	ret = bus_space_read_2(psc->sc_iot, psc->sc_ioh, offset);
	if (ret != vector) {
		aprint_debug_dev(sc->sc_dev, "%s: expected=%d, actual=%d\n",
		    __func__, vector, ret);
		VIRTIO_PCI_LOG(sc, reinit,
		    "can't set config msix vector\n");
		return -1;
	}

	for (qid = 0; qid < sc->sc_nvqs; qid++) {
		offset = VIRTIO_CONFIG_QUEUE_SELECT;
		bus_space_write_2(psc->sc_iot, psc->sc_ioh, offset, qid);

		offset = VIRTIO_CONFIG_MSI_QUEUE_VECTOR;
		vector = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;

		if (sc->sc_child_mq)
			vector += qid;

		bus_space_write_2(psc->sc_iot, psc->sc_ioh, offset, vector);
		ret = bus_space_read_2(psc->sc_iot, psc->sc_ioh, offset);
		if (ret != vector) {
			aprint_debug_dev(sc->sc_dev, "%s[qid=%d]:"
			    " expected=%d, actual=%d\n",
			    __func__, qid, vector, ret);
			VIRTIO_PCI_LOG(sc, reinit, "can't set queue %d "
			    "msix vector\n", qid);
			return -1;
		}
	}

	return 0;
}

static int
virtio_pci_establish_msix_interrupts(struct virtio_softc *sc,
    struct pci_attach_args *pa)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	device_t self = sc->sc_dev;
	pci_chipset_tag_t pc = pa->pa_pc;
	struct virtqueue *vq;
	char intrbuf[PCI_INTRSTR_LEN];
	char intr_xname[INTRDEVNAMEBUF];
	char const *intrstr;
	int idx, qid, n;

	idx = VIRTIO_MSIX_CONFIG_VECTOR_INDEX;
	if (sc->sc_flags & VIRTIO_F_INTR_MPSAFE)
		pci_intr_setattr(pc, &psc->sc_ihp[idx], PCI_INTR_MPSAFE, true);

	snprintf(intr_xname, sizeof(intr_xname), "%s config",
	    device_xname(sc->sc_dev));

	psc->sc_ihs[idx] = pci_intr_establish_xname(pc, psc->sc_ihp[idx],
	    sc->sc_ipl, virtio_pci_msix_config_intr, sc, intr_xname);
	if (psc->sc_ihs[idx] == NULL) {
		aprint_error_dev(self, "couldn't establish MSI-X for config\n");
		goto error;
	}

	idx = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;
	if (sc->sc_child_mq) {
		for (qid = 0; qid < sc->sc_nvqs; qid++) {
			n = idx + qid;
			vq = &sc->sc_vqs[qid];

			snprintf(intr_xname, sizeof(intr_xname), "%s vq#%d",
			    device_xname(sc->sc_dev), qid);

			if (sc->sc_flags & VIRTIO_F_INTR_MPSAFE) {
				pci_intr_setattr(pc, &psc->sc_ihp[n],
				    PCI_INTR_MPSAFE, true);
			}

			psc->sc_ihs[n] = pci_intr_establish_xname(pc, psc->sc_ihp[n],
			    sc->sc_ipl, vq->vq_intrhand, vq->vq_intrhand_arg, intr_xname);
			if (psc->sc_ihs[n] == NULL) {
				aprint_error_dev(self, "couldn't establish MSI-X for a vq\n");
				goto error;
			}
		}
	} else {
		if (sc->sc_flags & VIRTIO_F_INTR_MPSAFE)
			pci_intr_setattr(pc, &psc->sc_ihp[idx], PCI_INTR_MPSAFE, true);

		snprintf(intr_xname, sizeof(intr_xname), "%s queues",
		    device_xname(sc->sc_dev));
		psc->sc_ihs[idx] = pci_intr_establish_xname(pc, psc->sc_ihp[idx],
		    sc->sc_ipl, virtio_pci_msix_queue_intr, sc, intr_xname);
		if (psc->sc_ihs[idx] == NULL) {
			aprint_error_dev(self, "couldn't establish MSI-X for queues\n");
			goto error;
		}
	}

	idx = VIRTIO_MSIX_CONFIG_VECTOR_INDEX;
	intrstr = pci_intr_string(pc, psc->sc_ihp[idx], intrbuf, sizeof(intrbuf));
	aprint_normal_dev(self, "config interrupting at %s\n", intrstr);
	idx = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;
	if (sc->sc_child_mq) {
		kcpuset_t *affinity;
		int affinity_to, r;

		kcpuset_create(&affinity, false);

		for (qid = 0; qid < sc->sc_nvqs; qid++) {
			n = idx + qid;
			affinity_to = (qid / 2) % ncpu;

			intrstr = pci_intr_string(pc, psc->sc_ihp[n],
			    intrbuf, sizeof(intrbuf));

			kcpuset_zero(affinity);
			kcpuset_set(affinity, affinity_to);
			r = interrupt_distribute(psc->sc_ihs[n], affinity, NULL);
			if (r == 0) {
				aprint_normal_dev(self,
				    "for vq #%d interrupting at %s affinity to %u\n",
				    qid, intrstr, affinity_to);
			} else {
				aprint_normal_dev(self,
				    "for vq #%d interrupting at %s\n",
				    qid, intrstr);
			}
		}

		kcpuset_destroy(affinity);
	} else {
		intrstr = pci_intr_string(pc, psc->sc_ihp[idx], intrbuf, sizeof(intrbuf));
		aprint_normal_dev(self, "queues interrupting at %s\n", intrstr);
	}

	return 0;

error:
	idx = VIRTIO_MSIX_CONFIG_VECTOR_INDEX;
	if (psc->sc_ihs[idx] != NULL)
		pci_intr_disestablish(psc->sc_pa.pa_pc, psc->sc_ihs[idx]);
	idx = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;
	if (sc->sc_child_mq) {
		for (qid = 0; qid < sc->sc_nvqs; qid++) {
			n = idx + qid;
			if (psc->sc_ihs[n] == NULL)
				continue;
			pci_intr_disestablish(psc->sc_pa.pa_pc, psc->sc_ihs[n]);
		}

	} else {
		if (psc->sc_ihs[idx] != NULL)
			pci_intr_disestablish(psc->sc_pa.pa_pc, psc->sc_ihs[idx]);
	}

	return -1;
}

static int
virtio_pci_establish_intx_interrupt(struct virtio_softc *sc,
    struct pci_attach_args *pa)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	device_t self = sc->sc_dev;
	pci_chipset_tag_t pc = pa->pa_pc;
	char intrbuf[PCI_INTRSTR_LEN];
	char const *intrstr;

	if (sc->sc_flags & VIRTIO_F_INTR_MPSAFE)
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
virtio_pci_alloc_interrupts(struct virtio_softc *sc)
{
	struct virtio_pci_softc * const psc = (struct virtio_pci_softc *)sc;
	device_t self = sc->sc_dev;
	pci_chipset_tag_t pc = psc->sc_pa.pa_pc;
	pcitag_t tag = psc->sc_pa.pa_tag;
	int error;
	int nmsix;
	int off;
	int counts[PCI_INTR_TYPE_SIZE];
	pci_intr_type_t max_type;
	pcireg_t ctl;

	nmsix = pci_msix_count(psc->sc_pa.pa_pc, psc->sc_pa.pa_tag);
	aprint_debug_dev(self, "pci_msix_count=%d\n", nmsix);

	/* We need at least two: one for config and the other for queues */
	if ((sc->sc_flags & VIRTIO_F_INTR_MSIX) == 0 || nmsix < 2) {
		/* Try INTx only */
		max_type = PCI_INTR_TYPE_INTX;
		counts[PCI_INTR_TYPE_INTX] = 1;
	} else {
		/* Try MSI-X first and INTx second */
		if (sc->sc_nvqs + VIRTIO_MSIX_QUEUE_VECTOR_INDEX <= nmsix) {
			nmsix = sc->sc_nvqs + VIRTIO_MSIX_QUEUE_VECTOR_INDEX;
		} else {
			sc->sc_child_mq = false;
		}

		if (sc->sc_child_mq == false) {
			nmsix = 2;
		}

		max_type = PCI_INTR_TYPE_MSIX;
		counts[PCI_INTR_TYPE_MSIX] = nmsix;
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
		psc->sc_ihs = kmem_zalloc(sizeof(*psc->sc_ihs) * nmsix,
		    KM_SLEEP);

		error = virtio_pci_establish_msix_interrupts(sc, &psc->sc_pa);
		if (error != 0) {
			kmem_free(psc->sc_ihs, sizeof(*psc->sc_ihs) * nmsix);
			pci_intr_release(pc, psc->sc_ihp, nmsix);

			/* Retry INTx */
			max_type = PCI_INTR_TYPE_INTX;
			counts[PCI_INTR_TYPE_INTX] = 1;
			goto retry;
		}

		psc->sc_ihs_num = nmsix;
		psc->sc_devcfg_offset = VIRTIO_CONFIG_DEVICE_CONFIG_MSI;
		virtio_pci_adjust_config_region(psc);
	} else if (pci_intr_type(pc, psc->sc_ihp[0]) == PCI_INTR_TYPE_INTX) {
		psc->sc_ihs = kmem_zalloc(sizeof(*psc->sc_ihs) * 1,
		    KM_SLEEP);

		error = virtio_pci_establish_intx_interrupt(sc, &psc->sc_pa);
		if (error != 0) {
			kmem_free(psc->sc_ihs, sizeof(*psc->sc_ihs) * 1);
			pci_intr_release(pc, psc->sc_ihp, 1);
			return -1;
		}

		psc->sc_ihs_num = 1;
		psc->sc_devcfg_offset = VIRTIO_CONFIG_DEVICE_CONFIG_NOMSI;
		virtio_pci_adjust_config_region(psc);

		error = pci_get_capability(pc, tag, PCI_CAP_MSIX, &off, NULL);
		if (error != 0) {
			ctl = pci_conf_read(pc, tag, off + PCI_MSIX_CTL);
			ctl &= ~PCI_MSIX_CTL_ENABLE;
			pci_conf_write(pc, tag, off + PCI_MSIX_CTL, ctl);
		}
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

static bool
virtio_pci_msix_enabled(struct virtio_pci_softc *psc)
{
	pci_chipset_tag_t pc = psc->sc_pa.pa_pc;

	if (pci_intr_type(pc, psc->sc_ihp[0]) == PCI_INTR_TYPE_MSIX)
		return true;

	return false;
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
	isr = bus_space_read_1(psc->sc_isr_iot, psc->sc_isr_ioh, 0);
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
