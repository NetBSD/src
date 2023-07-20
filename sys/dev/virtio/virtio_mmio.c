/*	$NetBSD: virtio_mmio.c,v 1.11 2023/07/07 07:19:36 rin Exp $	*/
/*	$OpenBSD: virtio_mmio.c,v 1.2 2017/02/24 17:12:31 patrick Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: virtio_mmio.c,v 1.11 2023/07/07 07:19:36 rin Exp $");

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
#define VIRTIO_MMIO_HOST_FEATURES	0x010
#define VIRTIO_MMIO_HOST_FEATURES_SEL	0x014
#define VIRTIO_MMIO_GUEST_FEATURES	0x020
#define VIRTIO_MMIO_GUEST_FEATURES_SEL	0x024
#define VIRTIO_MMIO_GUEST_PAGE_SIZE	0x028
#define VIRTIO_MMIO_QUEUE_SEL		0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX	0x034
#define VIRTIO_MMIO_QUEUE_NUM		0x038
#define VIRTIO_MMIO_QUEUE_ALIGN		0x03c
#define VIRTIO_MMIO_QUEUE_PFN		0x040
#define VIRTIO_MMIO_QUEUE_NOTIFY	0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS	0x060
#define VIRTIO_MMIO_INTERRUPT_ACK	0x064
#define VIRTIO_MMIO_STATUS		0x070
#define VIRTIO_MMIO_CONFIG		0x100

#define VIRTIO_MMIO_INT_VRING		(1 << 0)
#define VIRTIO_MMIO_INT_CONFIG		(1 << 1)

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
static void	virtio_mmio_setup_queue(struct virtio_softc *, uint16_t, uint64_t);
static void	virtio_mmio_set_status(struct virtio_softc *, int);
static void	virtio_mmio_negotiate_features(struct virtio_softc *, uint64_t);
static int	virtio_mmio_alloc_interrupts(struct virtio_softc *);
static void	virtio_mmio_free_interrupts(struct virtio_softc *);
static int	virtio_mmio_setup_interrupts(struct virtio_softc *, int);

static const struct virtio_ops virtio_mmio_ops = {
	.kick = virtio_mmio_kick,
	.read_queue_size = virtio_mmio_read_queue_size,
	.setup_queue = virtio_mmio_setup_queue,
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
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_QUEUE_SEL, idx);
	return bus_space_read_4(sc->sc_iot, sc->sc_ioh,
	    VIRTIO_MMIO_QUEUE_NUM_MAX);
}

static void
virtio_mmio_setup_queue(struct virtio_softc *vsc, uint16_t idx, uint64_t addr)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_QUEUE_SEL, idx);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_QUEUE_NUM,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_QUEUE_NUM_MAX));
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_QUEUE_ALIGN,
	    VIRTIO_PAGE_SIZE);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_QUEUE_PFN,
	    addr / VIRTIO_PAGE_SIZE);
}

static void
virtio_mmio_set_status(struct virtio_softc *vsc, int status)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	int old = 0;

	if (status != 0)
		old = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
				       VIRTIO_MMIO_STATUS);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_STATUS,
			  status|old);
}

bool
virtio_mmio_common_probe_present(struct virtio_mmio_softc *sc)
{
	uint32_t magic;

	magic = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
	    VIRTIO_MMIO_MAGIC_VALUE);
	return (magic == VIRTIO_MMIO_MAGIC);
}

void
virtio_mmio_common_attach(struct virtio_mmio_softc *sc)
{
	struct virtio_softc *vsc = &sc->sc_sc;
	device_t self = vsc->sc_dev;
	uint32_t id, magic, ver;

	magic = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
	    VIRTIO_MMIO_MAGIC_VALUE);
	if (magic != VIRTIO_MMIO_MAGIC) {
		aprint_error_dev(vsc->sc_dev,
		    "wrong magic value 0x%08x; giving up\n", magic);
		return;
	}

	ver = bus_space_read_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_VERSION);
	if (ver != 1) {
		aprint_error_dev(vsc->sc_dev,
		    "unknown version 0x%02x; giving up\n", ver);
		return;
	}

	id = bus_space_read_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_DEVICE_ID);

	/* we could use PAGE_SIZE, but virtio(4) assumes 4KiB for now */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_GUEST_PAGE_SIZE,
	    VIRTIO_PAGE_SIZE);

	/* no device connected. */
	if (id == 0)
		return;

	virtio_print_device_type(self, id, ver);
	vsc->sc_ops = &virtio_mmio_ops;
	vsc->sc_bus_endian    = READ_ENDIAN;
	vsc->sc_struct_endian = STRUCT_ENDIAN;

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
 */
static void
virtio_mmio_negotiate_features(struct virtio_softc *vsc, uint64_t
    guest_features)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	uint32_t r;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
	    VIRTIO_MMIO_HOST_FEATURES_SEL, 0);
	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
				VIRTIO_MMIO_HOST_FEATURES);
	r &= guest_features;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
	    VIRTIO_MMIO_GUEST_FEATURES_SEL, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  VIRTIO_MMIO_GUEST_FEATURES, r);

	vsc->sc_active_features = r;
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
	isr = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
			       VIRTIO_MMIO_INTERRUPT_STATUS);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  VIRTIO_MMIO_INTERRUPT_ACK, isr);
	if ((isr & VIRTIO_MMIO_INT_CONFIG) &&
	    (vsc->sc_config_change != NULL))
		r = (vsc->sc_config_change)(vsc);
	if ((isr & VIRTIO_MMIO_INT_VRING) &&
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
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_QUEUE_NOTIFY,
	    idx);
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
