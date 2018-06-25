/*	$NetBSD: virtio_mmio.c,v 1.2.2.2 2018/06/25 07:26:03 pgoyette Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: virtio_mmio.c,v 1.2.2.2 2018/06/25 07:26:03 pgoyette Exp $");

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
 * XXX: Before being used on big endian arches, the access to config registers
 * XXX: needs to be reviewed/fixed. The non-device specific registers are
 * XXX: PCI-endian while the device specific registers are native endian.
 */

static void	virtio_mmio_kick(struct virtio_softc *, uint16_t);
static uint8_t	virtio_mmio_read_device_config_1(struct virtio_softc *, int);
static uint16_t	virtio_mmio_read_device_config_2(struct virtio_softc *, int);
static uint32_t	virtio_mmio_read_device_config_4(struct virtio_softc *, int);
static uint64_t	virtio_mmio_read_device_config_8(struct virtio_softc *, int);
static void	virtio_mmio_write_device_config_1(struct virtio_softc *, int, uint8_t);
static void	virtio_mmio_write_device_config_2(struct virtio_softc *, int, uint16_t);
static void	virtio_mmio_write_device_config_4(struct virtio_softc *, int, uint32_t);
static void	virtio_mmio_write_device_config_8(struct virtio_softc *, int, uint64_t);
static uint16_t	virtio_mmio_read_queue_size(struct virtio_softc *, uint16_t);
static void	virtio_mmio_setup_queue(struct virtio_softc *, uint16_t, uint32_t);
static void	virtio_mmio_set_status(struct virtio_softc *, int);
static uint32_t	virtio_mmio_negotiate_features(struct virtio_softc *, uint32_t);
static int	virtio_mmio_setup_interrupts(struct virtio_softc *);
static void	virtio_mmio_free_interrupts(struct virtio_softc *);

static const struct virtio_ops virtio_mmio_ops = {
	.kick = virtio_mmio_kick,
	.read_dev_cfg_1 = virtio_mmio_read_device_config_1,
	.read_dev_cfg_2 = virtio_mmio_read_device_config_2,
	.read_dev_cfg_4 = virtio_mmio_read_device_config_4,
	.read_dev_cfg_8 = virtio_mmio_read_device_config_8,
	.write_dev_cfg_1 = virtio_mmio_write_device_config_1,
	.write_dev_cfg_2 = virtio_mmio_write_device_config_2,
	.write_dev_cfg_4 = virtio_mmio_write_device_config_4,
	.write_dev_cfg_8 = virtio_mmio_write_device_config_8,
	.read_queue_size = virtio_mmio_read_queue_size,
	.setup_queue = virtio_mmio_setup_queue,
	.set_status = virtio_mmio_set_status,
	.neg_features = virtio_mmio_negotiate_features,
	.setup_interrupts = virtio_mmio_setup_interrupts,
	.free_interrupts = virtio_mmio_free_interrupts,
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
virtio_mmio_setup_queue(struct virtio_softc *vsc, uint16_t idx, uint32_t addr)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_QUEUE_SEL, idx);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_QUEUE_NUM,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_QUEUE_NUM_MAX));
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_QUEUE_ALIGN,
	    VIRTIO_PAGE_SIZE);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, VIRTIO_MMIO_QUEUE_PFN, addr);
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

void
virtio_mmio_common_attach(struct virtio_mmio_softc *sc)
{
	struct virtio_softc *vsc = &sc->sc_sc;
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

	/* No device connected. */
	if (id == 0)
		return;

	vsc->sc_ops = &virtio_mmio_ops;

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

	if (vsc->sc_child != NULL && vsc->sc_child != VIRTIO_CHILD_FAILED) {
		r = config_detach(vsc->sc_child, flags);
		if (r)
			return r;
	}
	KASSERT(vsc->sc_child == NULL || vsc->sc_child == VIRTIO_CHILD_FAILED);
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
static uint32_t
virtio_mmio_negotiate_features(struct virtio_softc *vsc, uint32_t
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
	return r;
}

/*
 * Device configuration registers.
 */
static uint8_t
virtio_mmio_read_device_config_1(struct virtio_softc *vsc, int index)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh,
				VIRTIO_MMIO_CONFIG + index);
}

static uint16_t
virtio_mmio_read_device_config_2(struct virtio_softc *vsc, int index)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh,
				VIRTIO_MMIO_CONFIG + index);
}

static uint32_t
virtio_mmio_read_device_config_4(struct virtio_softc *vsc, int index)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	return bus_space_read_4(sc->sc_iot, sc->sc_ioh,
				VIRTIO_MMIO_CONFIG + index);
}

static uint64_t
virtio_mmio_read_device_config_8(struct virtio_softc *vsc, int index)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	uint64_t r;

	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
			     VIRTIO_MMIO_CONFIG + index + sizeof(uint32_t));
	r <<= 32;
	r += bus_space_read_4(sc->sc_iot, sc->sc_ioh,
			      VIRTIO_MMIO_CONFIG + index);
	return r;
}

static void
virtio_mmio_write_device_config_1(struct virtio_softc *vsc,
			     int index, uint8_t value)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			  VIRTIO_MMIO_CONFIG + index, value);
}

static void
virtio_mmio_write_device_config_2(struct virtio_softc *vsc,
			     int index, uint16_t value)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
			  VIRTIO_MMIO_CONFIG + index, value);
}

static void
virtio_mmio_write_device_config_4(struct virtio_softc *vsc,
			     int index, uint32_t value)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  VIRTIO_MMIO_CONFIG + index, value);
}

static void
virtio_mmio_write_device_config_8(struct virtio_softc *vsc,
			     int index, uint64_t value)
{
	struct virtio_mmio_softc *sc = (struct virtio_mmio_softc *)vsc;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  VIRTIO_MMIO_CONFIG + index,
			  value & 0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  VIRTIO_MMIO_CONFIG + index + sizeof(uint32_t),
			  value >> 32);
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
virtio_mmio_setup_interrupts(struct virtio_softc *vsc)
{
	struct virtio_mmio_softc * const sc = (struct virtio_mmio_softc *)vsc;

	return sc->sc_setup_interrupts(sc);
}

static void
virtio_mmio_free_interrupts(struct virtio_softc *vsc)
{
	struct virtio_mmio_softc * const sc = (struct virtio_mmio_softc *)vsc;

	sc->sc_free_interrupts(sc);
}
