/*	$NetBSD: virtio_mmio.c,v 1.14 2024/03/09 11:55:59 isaki Exp $	*/
/*	$OpenBSD: virtio_mmio.c,v 1.2 2017/02/24 17:12:31 patrick Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Copyright (c) 2014 Patrick Wildt <patrick@blueri.se>
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
__KERNEL_RCSID(0, "$NetBSD: virtio_mmio.c,v 1.14 2024/03/09 11:55:59 isaki Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/mutex.h>

#define VIRTIO_PRIVATE
#include <dev/virtio/virtio_mmiovar.h>

#define VIRTIO_MMIO_MAGIC		('v' | 'i' << 8 | 'r' << 16 | 't' << 24)

#define VIRTIO_MMIO_MAGIC_VALUE		0x000
#define VIRTIO_MMIO_VERSION		0x004
#define VIRTIO_MMIO_DEVICE_ID		0x008
#define VIRTIO_MMIO_VENDOR_ID		0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES	0x010	/* "HostFeatures" in v1 */
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL	0x014	/* "HostFeaturesSel" in v1 */
#define VIRTIO_MMIO_DRIVER_FEATURES	0x020	/* "GuestFeatures" in v1 */
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL	0x024	/* "GuestFeaturesSel" in v1 */
#define VIRTIO_MMIO_V1_GUEST_PAGE_SIZE	0x028
#define VIRTIO_MMIO_QUEUE_SEL		0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX	0x034
#define VIRTIO_MMIO_QUEUE_NUM		0x038
#define VIRTIO_MMIO_V1_QUEUE_ALIGN	0x03c
#define VIRTIO_MMIO_V1_QUEUE_PFN	0x040
#define	VIRTIO_MMIO_QUEUE_READY		0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY	0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS	0x060
#define VIRTIO_MMIO_INTERRUPT_ACK	0x064
#define VIRTIO_MMIO_STATUS		0x070
#define	VIRTIO_MMIO_V2_QUEUE_DESC_LOW	0x080
#define	VIRTIO_MMIO_V2_QUEUE_DESC_HIGH	0x084
#define	VIRTIO_MMIO_V2_QUEUE_AVAIL_LOW	0x090
#define	VIRTIO_MMIO_V2_QUEUE_AVAIL_HIGH	0x094
#define	VIRTIO_MMIO_V2_QUEUE_USED_LOW	0x0a0
#define	VIRTIO_MMIO_V2_QUEUE_USED_HIGH	0x0a4
#define	VIRTIO_MMIO_V2_CONFIG_GEN	0x0fc
#define VIRTIO_MMIO_CONFIG		0x100

/*
 * MMIO configuration space for virtio-mmio v1 is in guest byte order.
 *
 * XXX For big-endian aarch64 and arm, see note in virtio_pci.c.
 */

#if (defined(__aarch64__) || defined(__arm__)) && BYTE_ORDER == BIG_ENDIAN
#	define READ_ENDIAN	LITTLE_ENDIAN
#	define STRUCT_ENDIAN	BIG_ENDIAN
#elif BYTE_ORDER == BIG_ENDIAN
#	define READ_ENDIAN	BIG_ENDIAN
#	define STRUCT_ENDIAN	BIG_ENDIAN
#else
#	define READ_ENDIAN	LITTLE_ENDIAN
#	define STRUCT_ENDIAN	LITTLE_ENDIAN
#endif


static void	virtio_mmio_kick(struct virtio_softc *, uint16_t);
static uint16_t	virtio_mmio_read_queue_size(struct virtio_softc *, uint16_t);
static void	virtio_mmio_v1_setup_queue(struct virtio_softc *, uint16_t, uint64_t);
static void	virtio_mmio_v2_setup_queue(struct virtio_softc *, uint16_t, uint64_t);
static int	virtio_mmio_get_status(struct virtio_softc *);
static void	virtio_mmio_set_status(struct virtio_softc *, int);
static void	virtio_mmio_negotiate_features(struct virtio_softc *, uint64_t);
static int	virtio_mmio_alloc_interrupts(struct virtio_softc *);
static void	virtio_mmio_free_interrupts(struct virtio_softc *);
static int	virtio_mmio_setup_interrupts(struct virtio_softc *, int);

static uint32_t
virtio_mmio_reg_read(struct virtio_mmio_softc *sc, bus_addr_t reg)
{
	uint32_t val;

	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, reg);
	if (sc->sc_le_regs) {
		val = le32toh(val);
	}
	return val;
}

static void
virtio_mmio_reg_write(struct virtio_mmio_softc *sc, bus_addr_t reg,
    uint32_t val)
{
	if (sc->sc_le_regs) {
		val = htole32(val);
	}
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, reg, val);
}

static void
virtio_mmio_v2_set_addr(struct virtio_mmio_softc *sc, bus_addr_t reg,
    uint64_t addr)
{
	virtio_mmio_reg_write(sc, reg,     BUS_ADDR_LO32(addr));
	virtio_mmio_reg_write(sc, reg + 4, BUS_ADDR_HI32(addr));
}

static const struct virtio_ops virtio_mmio_v1_ops = {
	.kick = virtio_mmio_kick,
	.read_queue_size = virtio_mmio_read_queue_size,
	.setup_queue = virtio_mmio_v1_setup_queue,
	.set_status = virtio_mmio_set_status,
	.neg_features = virtio_mmio_negotiate_features,
	.alloc_interrupts = virtio_mmio_alloc_interrupts,
	.free_interrupts = virtio_mmio_free_interrupts,
	.setup_interrupts = virtio_mmio_setup_interrupts,
};

static const struct virtio_ops virtio_mmio_v2_ops = {
	.kick = virtio_mmio_kick,
	.read_queue_size = virtio_mmio_read_queue_size,
	.setup_queue = virtio_mmio_v2_setup_queue,
	.set_status = virtio_mmio_set_status,
	.neg_features = virtio_mmio_negotiate_features,
	.alloc_interrupts = virtio_mmio_alloc_interrupts,
	.free_interrupts = virtio_mmio_free_interrupts,
	.setup_interrupts = virtio_mmio_setup_interrupts,
};

static uint16_t
virtio_mmio_read_queue_size(struct virtio_softc *vsc, uint16_t idx)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	virtio_mmio_reg_write(sc, VIRTIO_MMIO_QUEUE_SEL, idx);
	return virtio_mmio_reg_read(sc, VIRTIO_MMIO_QUEUE_NUM_MAX);
}

static void
virtio_mmio_v1_setup_queue(struct virtio_softc *vsc, uint16_t idx,
    uint64_t addr)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;

	virtio_mmio_reg_write(sc, VIRTIO_MMIO_QUEUE_SEL, idx);
	virtio_mmio_reg_write(sc, VIRTIO_MMIO_QUEUE_NUM,
	    virtio_mmio_reg_read(sc, VIRTIO_MMIO_QUEUE_NUM_MAX));
	virtio_mmio_reg_write(sc, VIRTIO_MMIO_V1_QUEUE_ALIGN,
	    VIRTIO_PAGE_SIZE);
	virtio_mmio_reg_write(sc, VIRTIO_MMIO_V1_QUEUE_PFN,
	    addr / VIRTIO_PAGE_SIZE);
}

static void
virtio_mmio_v2_setup_queue(struct virtio_softc *vsc, uint16_t idx,
    uint64_t addr)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	struct virtqueue *vq;

	virtio_mmio_reg_write(sc, VIRTIO_MMIO_QUEUE_SEL, idx);
	if (addr == 0) {
		virtio_mmio_reg_write(sc, VIRTIO_MMIO_QUEUE_READY, 0);
		virtio_mmio_v2_set_addr(sc, VIRTIO_MMIO_V2_QUEUE_DESC_LOW, 0);
		virtio_mmio_v2_set_addr(sc, VIRTIO_MMIO_V2_QUEUE_AVAIL_LOW, 0);
		virtio_mmio_v2_set_addr(sc, VIRTIO_MMIO_V2_QUEUE_USED_LOW, 0);
	} else {
		vq = &vsc->sc_vqs[idx];
		KASSERT(vq->vq_index == idx);

		virtio_mmio_reg_write(sc, VIRTIO_MMIO_QUEUE_NUM,
		    virtio_mmio_reg_read(sc, VIRTIO_MMIO_QUEUE_NUM_MAX));
		virtio_mmio_v2_set_addr(sc, VIRTIO_MMIO_V2_QUEUE_DESC_LOW,
		    addr);
		virtio_mmio_v2_set_addr(sc, VIRTIO_MMIO_V2_QUEUE_AVAIL_LOW,
		    addr + vq->vq_availoffset);
		virtio_mmio_v2_set_addr(sc, VIRTIO_MMIO_V2_QUEUE_USED_LOW,
		    addr + vq->vq_usedoffset);
		virtio_mmio_reg_write(sc, VIRTIO_MMIO_QUEUE_READY, 1);
	}
}

static int
virtio_mmio_get_status(struct virtio_softc *vsc)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;

	return virtio_mmio_reg_read(sc, VIRTIO_MMIO_STATUS);
}

static void
virtio_mmio_set_status(struct virtio_softc *vsc, int status)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	int old = 0;

	if (status != 0)
		old = virtio_mmio_reg_read(sc, VIRTIO_MMIO_STATUS);
	virtio_mmio_reg_write(sc, VIRTIO_MMIO_STATUS, status|old);
}

bool
virtio_mmio_common_probe_present(struct virtio_mmio_softc *sc)
{
	uint32_t magic;

	/* XXX */
	magic = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
	    VIRTIO_MMIO_MAGIC_VALUE);
	return (magic == VIRTIO_MMIO_MAGIC);
}


void
virtio_mmio_common_attach(struct virtio_mmio_softc *sc)
{
	struct virtio_softc *vsc = &sc->sc_sc;
	device_t self = vsc->sc_dev;
	uint32_t id, magic;
	int virtio_vers;

	magic = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
	    VIRTIO_MMIO_MAGIC_VALUE);
	if (magic != VIRTIO_MMIO_MAGIC) {
		if (magic == le32toh(VIRTIO_MMIO_MAGIC)) {
			sc->sc_le_regs = true;
		} else {
			aprint_error_dev(vsc->sc_dev,
			    "wrong magic value 0x%08x; giving up\n", magic);
			return;
		}
	}
	vsc->sc_bus_endian    = READ_ENDIAN;
	vsc->sc_struct_endian = STRUCT_ENDIAN;

	sc->sc_mmio_vers = virtio_mmio_reg_read(sc, VIRTIO_MMIO_VERSION);
	switch (sc->sc_mmio_vers) {
	case 1:
		/* we could use PAGE_SIZE, but virtio(4) assumes 4KiB for now */
		virtio_mmio_reg_write(sc,
		    VIRTIO_MMIO_V1_GUEST_PAGE_SIZE, VIRTIO_PAGE_SIZE);
		vsc->sc_ops = &virtio_mmio_v1_ops;
		/*
		 * MMIO v1 ("legacy") is documented in the VirtIO 0.9.x
		 * draft(s) and uses the same page-oriented queue setup,
		 * so that's what we'll report as the VirtIO version.
		 */
		virtio_vers = 0;
		break;

	case 2:
		vsc->sc_ops = &virtio_mmio_v2_ops;
		/*
		 * MMIO v2 is documented in the VirtIO 1.0 spec.
		 */
		virtio_vers = 1;
		break;

	default:
		aprint_error_dev(vsc->sc_dev,
		    "unknown version 0x%08x; giving up\n", sc->sc_mmio_vers);
		return;
	}
	aprint_normal_dev(self, "VirtIO-MMIO-v%u\n", sc->sc_mmio_vers);

	id = virtio_mmio_reg_read(sc, VIRTIO_MMIO_DEVICE_ID);
	if (id == 0) {
		/* no device connected. */
		return;
	}

	virtio_print_device_type(self, id, virtio_vers);

	/* set up our device config tag */
	vsc->sc_devcfg_iosize = sc->sc_iosize - VIRTIO_MMIO_CONFIG;
	vsc->sc_devcfg_iot = sc->sc_iot;
	if (bus_space_subregion(sc->sc_iot, sc->sc_ioh,
			VIRTIO_MMIO_CONFIG, vsc->sc_devcfg_iosize,
			&vsc->sc_devcfg_ioh)) {
		aprint_error_dev(self, "can't map config i/o space\n");
		return;
	}

	virtio_device_reset(vsc);
	virtio_mmio_set_status(vsc, VIRTIO_CONFIG_DEVICE_STATUS_ACK);
	virtio_mmio_set_status(vsc, VIRTIO_CONFIG_DEVICE_STATUS_DRIVER);

	/* XXX: use softc as aux... */
	vsc->sc_childdevid = id;
	vsc->sc_child = NULL;
}

int
virtio_mmio_common_detach(struct virtio_mmio_softc *sc, int flags)
{
	struct virtio_softc *vsc = &sc->sc_sc;
	int r;

	r = config_detach_children(vsc->sc_dev, flags);
	if (r != 0)
		return r;

	KASSERT(vsc->sc_child == NULL);
	KASSERT(vsc->sc_vqs == NULL);
	KASSERT(sc->sc_ih == NULL);

	if (sc->sc_iosize) {
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_iosize);
		sc->sc_iosize = 0;
	}

	return 0;
}

/*
 * Feature negotiation.
 *
 * We fold pre-VirtIO-1.0 feature negotiation into this single routine
 * because the "legacy" (MMIO-v1) also had the feature sel registers.
 */
static void
virtio_mmio_negotiate_features(struct virtio_softc *vsc, uint64_t
    driver_features)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	device_t self = vsc->sc_dev;
	uint64_t saved_driver_features = driver_features;
	uint64_t device_features, negotiated;
	uint32_t device_status;

	driver_features |= VIRTIO_F_VERSION_1;
	vsc->sc_active_features = 0;

	virtio_mmio_reg_write(sc, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 0);
	device_features = virtio_mmio_reg_read(sc, VIRTIO_MMIO_DEVICE_FEATURES);
	virtio_mmio_reg_write(sc, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 1);
	device_features |= (uint64_t)
	    virtio_mmio_reg_read(sc, VIRTIO_MMIO_DEVICE_FEATURES) << 32;

	/* notify on empty is 0.9 only */
	if (device_features & VIRTIO_F_VERSION_1) {
		driver_features &= ~VIRTIO_F_NOTIFY_ON_EMPTY;
	} else {
		/*
		 * Require version 1 for MMIO-v2 transport.
		 */
		if (sc->sc_mmio_vers >= 2) {
			aprint_error_dev(self, "MMIO-v%u requires version 1\n",
			    sc->sc_mmio_vers);
			virtio_mmio_set_status(vsc,
			    VIRTIO_CONFIG_DEVICE_STATUS_FAILED);
			return;
		}
		/*
		 * If the driver requires version 1, but the device doesn't
		 * support it, fail now.
		 */
		if (saved_driver_features & VIRTIO_F_VERSION_1) {
			aprint_error_dev(self, "device rejected version 1\n");
			virtio_mmio_set_status(vsc,
			    VIRTIO_CONFIG_DEVICE_STATUS_FAILED);
			return;
		}
	}

	negotiated = device_features & driver_features;

	virtio_mmio_reg_write(sc, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0);
	virtio_mmio_reg_write(sc, VIRTIO_MMIO_DRIVER_FEATURES,
	    (uint32_t)negotiated);
	virtio_mmio_reg_write(sc, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 1);
	virtio_mmio_reg_write(sc, VIRTIO_MMIO_DRIVER_FEATURES,
	    (uint32_t)(negotiated >> 32));

	/*
	 * FEATURES_OK status is not present pre-1.0.
	 */
	if (device_features & VIRTIO_F_VERSION_1) {
		virtio_mmio_set_status(vsc,
		    VIRTIO_CONFIG_DEVICE_STATUS_FEATURES_OK);
		device_status = virtio_mmio_get_status(vsc);
		if ((device_status &
		     VIRTIO_CONFIG_DEVICE_STATUS_FEATURES_OK) == 0) {
			aprint_error_dev(self, "feature negotiation failed\n");
			virtio_mmio_set_status(vsc,
			    VIRTIO_CONFIG_DEVICE_STATUS_FAILED);
			return;
		}
	}

	if (negotiated & VIRTIO_F_VERSION_1) {
		/*
		 * All VirtIO 1.0 access is little-endian.
		 */
		vsc->sc_bus_endian    = LITTLE_ENDIAN;
		vsc->sc_struct_endian = LITTLE_ENDIAN;
	}

	vsc->sc_active_features = negotiated;
}

/*
 * Interrupt handler.
 */
int
virtio_mmio_intr(void *arg)
{
	struct virtio_mmio_softc *sc = arg;
	struct virtio_softc *vsc = &sc->sc_sc;
	int isr, r = 0;

	/* check and ack the interrupt */
	isr = virtio_mmio_reg_read(sc, VIRTIO_MMIO_INTERRUPT_STATUS);
	virtio_mmio_reg_write(sc, VIRTIO_MMIO_INTERRUPT_ACK, isr);
	if ((isr & VIRTIO_CONFIG_ISR_CONFIG_CHANGE) &&
	    (vsc->sc_config_change != NULL))
		r = (vsc->sc_config_change)(vsc);
	if ((isr & VIRTIO_CONFIG_ISR_QUEUE_INTERRUPT) &&
	    (vsc->sc_intrhand != NULL)) {
		if (vsc->sc_soft_ih != NULL)
			softint_schedule(vsc->sc_soft_ih);
		else
			r |= (vsc->sc_intrhand)(vsc);
	}

	return r;
}

static void
virtio_mmio_kick(struct virtio_softc *vsc, uint16_t idx)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	virtio_mmio_reg_write(sc, VIRTIO_MMIO_QUEUE_NOTIFY, idx);
}

static int
virtio_mmio_alloc_interrupts(struct virtio_softc *vsc)
{
	struct virtio_mmio_softc * const sc = (struct virtio_mmio_softc *)vsc;

	return sc->sc_alloc_interrupts(sc);
}

static void
virtio_mmio_free_interrupts(struct virtio_softc *vsc)
{
	struct virtio_mmio_softc * const sc = (struct virtio_mmio_softc *)vsc;

	sc->sc_free_interrupts(sc);
}

static int
virtio_mmio_setup_interrupts(struct virtio_softc *vsc __unused,
    int reinit __unused)
{

	return 0;
}
