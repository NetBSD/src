/*	$NetBSD: virtio.c,v 1.54 2022/04/24 11:51:09 uwe Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * Copyright (c) 2012 Stefan Fritsch, Alexander Fiveg.
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
__KERNEL_RCSID(0, "$NetBSD: virtio.c,v 1.54 2022/04/24 11:51:09 uwe Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/module.h>

#define VIRTIO_PRIVATE

#include <dev/pci/virtioreg.h> /* XXX: move to non-pci */
#include <dev/pci/virtiovar.h> /* XXX: move to non-pci */

#define MINSEG_INDIRECT		2 /* use indirect if nsegs >= this value */

/* incomplete list */
static const char *virtio_device_name[] = {
	"unknown (0)",			/*  0 */
	"network",			/*  1 */
	"block",			/*  2 */
	"console",			/*  3 */
	"entropy",			/*  4 */
	"memory balloon",		/*  5 */
	"I/O memory",			/*  6 */
	"remote processor messaging",	/*  7 */
	"SCSI",				/*  8 */
	"9P transport",			/*  9 */
};
#define NDEVNAMES	__arraycount(virtio_device_name)

static void	virtio_init_vq(struct virtio_softc *,
		    struct virtqueue *, const bool);

void
virtio_set_status(struct virtio_softc *sc, int status)
{
	sc->sc_ops->set_status(sc, status);
}

/*
 * Reset the device.
 */
/*
 * To reset the device to a known state, do following:
 *	virtio_reset(sc);	     // this will stop the device activity
 *	<dequeue finished requests>; // virtio_dequeue() still can be called
 *	<revoke pending requests in the vqs if any>;
 *	virtio_reinit_start(sc);     // dequeue prohibitted
 *	newfeatures = virtio_negotiate_features(sc, requestedfeatures);
 *	<some other initialization>;
 *	virtio_reinit_end(sc);	     // device activated; enqueue allowed
 * Once attached, feature negotiation can only be allowed after virtio_reset.
 */
void
virtio_reset(struct virtio_softc *sc)
{
	virtio_device_reset(sc);
}

int
virtio_reinit_start(struct virtio_softc *sc)
{
	int i, r;

	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_ACK);
	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_DRIVER);
	for (i = 0; i < sc->sc_nvqs; i++) {
		int n;
		struct virtqueue *vq = &sc->sc_vqs[i];
		n = sc->sc_ops->read_queue_size(sc, vq->vq_index);
		if (n == 0)	/* vq disappeared */
			continue;
		if (n != vq->vq_num) {
			panic("%s: virtqueue size changed, vq index %d\n",
			      device_xname(sc->sc_dev),
			      vq->vq_index);
		}
		virtio_init_vq(sc, vq, true);
		sc->sc_ops->setup_queue(sc, vq->vq_index,
		    vq->vq_dmamap->dm_segs[0].ds_addr);
	}

	r = sc->sc_ops->setup_interrupts(sc, 1);
	if (r != 0)
		goto fail;

	return 0;

fail:
	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_FAILED);

	return 1;
}

void
virtio_reinit_end(struct virtio_softc *sc)
{
	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_DRIVER_OK);
}

/*
 * Feature negotiation.
 */
void
virtio_negotiate_features(struct virtio_softc *sc, uint64_t guest_features)
{
	if (!(device_cfdata(sc->sc_dev)->cf_flags & 1) &&
	    !(device_cfdata(sc->sc_child)->cf_flags & 1)) /* XXX */
		guest_features |= VIRTIO_F_RING_INDIRECT_DESC;
	sc->sc_ops->neg_features(sc, guest_features);
	if (sc->sc_active_features & VIRTIO_F_RING_INDIRECT_DESC)
		sc->sc_indirect = true;
	else
		sc->sc_indirect = false;
}


/*
 * Device configuration registers readers/writers
 */
#if 0
#define DPRINTFR(n, fmt, val, index, num) \
	printf("\n%s (", n); \
	for (int i = 0; i < num; i++) \
		printf("%02x ", bus_space_read_1(sc->sc_devcfg_iot, sc->sc_devcfg_ioh, index+i)); \
	printf(") -> "); printf(fmt, val); printf("\n");
#define DPRINTFR2(n, fmt, val_s, val_n) \
	printf("%s ", n); \
	printf("\n        stream "); printf(fmt, val_s); printf(" norm "); printf(fmt, val_n); printf("\n");
#else
#define DPRINTFR(n, fmt, val, index, num)
#define DPRINTFR2(n, fmt, val_s, val_n)
#endif


uint8_t
virtio_read_device_config_1(struct virtio_softc *sc, int index) {
	bus_space_tag_t	   iot = sc->sc_devcfg_iot;
	bus_space_handle_t ioh = sc->sc_devcfg_ioh;
	uint8_t val;

	val = bus_space_read_1(iot, ioh, index);

	DPRINTFR("read_1", "%02x", val, index, 1);
	return val;
}

uint16_t
virtio_read_device_config_2(struct virtio_softc *sc, int index) {
	bus_space_tag_t	   iot = sc->sc_devcfg_iot;
	bus_space_handle_t ioh = sc->sc_devcfg_ioh;
	uint16_t val;

	val = bus_space_read_2(iot, ioh, index);
	if (BYTE_ORDER != sc->sc_bus_endian)
		val = bswap16(val);

	DPRINTFR("read_2", "%04x", val, index, 2);
	DPRINTFR2("read_2", "%04x",
		bus_space_read_stream_2(sc->sc_devcfg_iot, sc->sc_devcfg_ioh, index),
		bus_space_read_2(sc->sc_devcfg_iot, sc->sc_devcfg_ioh, index));
	return val;
}

uint32_t
virtio_read_device_config_4(struct virtio_softc *sc, int index) {
	bus_space_tag_t	   iot = sc->sc_devcfg_iot;
	bus_space_handle_t ioh = sc->sc_devcfg_ioh;
	uint32_t val;

	val = bus_space_read_4(iot, ioh, index);
	if (BYTE_ORDER != sc->sc_bus_endian)
		val = bswap32(val);

	DPRINTFR("read_4", "%08x", val, index, 4);
	DPRINTFR2("read_4", "%08x",
		bus_space_read_stream_4(sc->sc_devcfg_iot, sc->sc_devcfg_ioh, index),
		bus_space_read_4(sc->sc_devcfg_iot, sc->sc_devcfg_ioh, index));
	return val;
}

/*
 * The Virtio spec explicitly tells that reading and writing 8 bytes are not
 * considered atomic and no triggers may be connected to reading or writing
 * it. We access it using two 32 reads. See virtio spec 4.1.3.1.
 */
uint64_t
virtio_read_device_config_8(struct virtio_softc *sc, int index) {
	bus_space_tag_t	   iot = sc->sc_devcfg_iot;
	bus_space_handle_t ioh = sc->sc_devcfg_ioh;
	union {
		uint64_t u64;
		uint32_t l[2];
	} v;
	uint64_t val;

	v.l[0] = bus_space_read_4(iot, ioh, index);
	v.l[1] = bus_space_read_4(iot, ioh, index + 4);
	if (sc->sc_bus_endian != sc->sc_struct_endian) {
		v.l[0] = bswap32(v.l[0]);
		v.l[1] = bswap32(v.l[1]);
	}
	val = v.u64;

	if (BYTE_ORDER != sc->sc_struct_endian)
		val = bswap64(val);

	DPRINTFR("read_8", "%08lx", val, index, 8);
	DPRINTFR2("read_8 low ", "%08x",
		bus_space_read_stream_4(sc->sc_devcfg_iot, sc->sc_devcfg_ioh, index),
		bus_space_read_4(sc->sc_devcfg_iot, sc->sc_devcfg_ioh, index));
	DPRINTFR2("read_8 high ", "%08x",
		bus_space_read_stream_4(sc->sc_devcfg_iot, sc->sc_devcfg_ioh, index + 4),
		bus_space_read_4(sc->sc_devcfg_iot, sc->sc_devcfg_ioh, index + 4));
	return val;
}

/*
 * In the older virtio spec, device config registers are host endian. On newer
 * they are little endian. Some newer devices however explicitly specify their
 * register to always be little endian. These fuctions cater for these.
 */
uint16_t
virtio_read_device_config_le_2(struct virtio_softc *sc, int index) {
	bus_space_tag_t	   iot = sc->sc_devcfg_iot;
	bus_space_handle_t ioh = sc->sc_devcfg_ioh;
	uint16_t val;

	val = bus_space_read_2(iot, ioh, index);
	if (sc->sc_bus_endian != LITTLE_ENDIAN)
		val = bswap16(val);

	DPRINTFR("read_le_2", "%04x", val, index, 2);
	DPRINTFR2("read_le_2", "%04x",
		bus_space_read_stream_2(sc->sc_devcfg_iot, sc->sc_devcfg_ioh, 0),
		bus_space_read_2(sc->sc_devcfg_iot, sc->sc_devcfg_ioh, 0));
	return val;
}

uint32_t
virtio_read_device_config_le_4(struct virtio_softc *sc, int index) {
	bus_space_tag_t	   iot = sc->sc_devcfg_iot;
	bus_space_handle_t ioh = sc->sc_devcfg_ioh;
	uint32_t val;

	val = bus_space_read_4(iot, ioh, index);
	if (sc->sc_bus_endian != LITTLE_ENDIAN)
		val = bswap32(val);

	DPRINTFR("read_le_4", "%08x", val, index, 4);
	DPRINTFR2("read_le_4", "%08x",
		bus_space_read_stream_4(sc->sc_devcfg_iot, sc->sc_devcfg_ioh, 0),
		bus_space_read_4(sc->sc_devcfg_iot, sc->sc_devcfg_ioh, 0));
	return val;
}

void
virtio_write_device_config_1(struct virtio_softc *sc, int index, uint8_t value)
{
	bus_space_tag_t	   iot = sc->sc_devcfg_iot;
	bus_space_handle_t ioh = sc->sc_devcfg_ioh;

	bus_space_write_1(iot, ioh, index, value);
}

void
virtio_write_device_config_2(struct virtio_softc *sc, int index, uint16_t value)
{
	bus_space_tag_t	   iot = sc->sc_devcfg_iot;
	bus_space_handle_t ioh = sc->sc_devcfg_ioh;

	if (BYTE_ORDER != sc->sc_bus_endian)
		value = bswap16(value);
	bus_space_write_2(iot, ioh, index, value);
}

void
virtio_write_device_config_4(struct virtio_softc *sc, int index, uint32_t value)
{
	bus_space_tag_t	   iot = sc->sc_devcfg_iot;
	bus_space_handle_t ioh = sc->sc_devcfg_ioh;

	if (BYTE_ORDER != sc->sc_bus_endian)
		value = bswap32(value);
	bus_space_write_4(iot, ioh, index, value);
}

/*
 * The Virtio spec explicitly tells that reading and writing 8 bytes are not
 * considered atomic and no triggers may be connected to reading or writing
 * it. We access it using two 32 bit writes. For good measure it is stated to
 * always write lsb first just in case of a hypervisor bug. See See virtio
 * spec 4.1.3.1.
 */
void
virtio_write_device_config_8(struct virtio_softc *sc, int index, uint64_t value)
{
	bus_space_tag_t	   iot = sc->sc_devcfg_iot;
	bus_space_handle_t ioh = sc->sc_devcfg_ioh;
	union {
		uint64_t u64;
		uint32_t l[2];
	} v;

	if (BYTE_ORDER != sc->sc_struct_endian)
		value = bswap64(value);

	v.u64 = value;
	if (sc->sc_bus_endian != sc->sc_struct_endian) {
		v.l[0] = bswap32(v.l[0]);
		v.l[1] = bswap32(v.l[1]);
	}

	if (sc->sc_struct_endian == LITTLE_ENDIAN) {
		bus_space_write_4(iot, ioh, index,     v.l[0]);
		bus_space_write_4(iot, ioh, index + 4, v.l[1]);
	} else {
		bus_space_write_4(iot, ioh, index + 4, v.l[1]);
		bus_space_write_4(iot, ioh, index,     v.l[0]);
	}
}

/*
 * In the older virtio spec, device config registers are host endian. On newer
 * they are little endian. Some newer devices however explicitly specify their
 * register to always be little endian. These fuctions cater for these.
 */
void
virtio_write_device_config_le_2(struct virtio_softc *sc, int index, uint16_t value)
{
	bus_space_tag_t	   iot = sc->sc_devcfg_iot;
	bus_space_handle_t ioh = sc->sc_devcfg_ioh;

	if (sc->sc_bus_endian != LITTLE_ENDIAN)
		value = bswap16(value);
	bus_space_write_2(iot, ioh, index, value);
}

void
virtio_write_device_config_le_4(struct virtio_softc *sc, int index, uint32_t value)
{
	bus_space_tag_t	   iot = sc->sc_devcfg_iot;
	bus_space_handle_t ioh = sc->sc_devcfg_ioh;

	if (sc->sc_bus_endian != LITTLE_ENDIAN)
		value = bswap32(value);
	bus_space_write_4(iot, ioh, index, value);
}


/*
 * data structures endian helpers
 */
uint16_t virtio_rw16(struct virtio_softc *sc, uint16_t val)
{
	KASSERT(sc);
	return BYTE_ORDER != sc->sc_struct_endian ? bswap16(val) : val;
}

uint32_t virtio_rw32(struct virtio_softc *sc, uint32_t val)
{
	KASSERT(sc);
	return BYTE_ORDER != sc->sc_struct_endian ? bswap32(val) : val;
}

uint64_t virtio_rw64(struct virtio_softc *sc, uint64_t val)
{
	KASSERT(sc);
	return BYTE_ORDER != sc->sc_struct_endian ? bswap64(val) : val;
}


/*
 * Interrupt handler.
 */
static void
virtio_soft_intr(void *arg)
{
	struct virtio_softc *sc = arg;

	KASSERT(sc->sc_intrhand != NULL);

	(*sc->sc_intrhand)(sc);
}

/*
 * dmamap sync operations for a virtqueue.
 */
static inline void
vq_sync_descs(struct virtio_softc *sc, struct virtqueue *vq, int ops)
{
	/* availoffset == sizeof(vring_desc)*vq_num */
	bus_dmamap_sync(sc->sc_dmat, vq->vq_dmamap, 0, vq->vq_availoffset,
			ops);
}

static inline void
vq_sync_aring(struct virtio_softc *sc, struct virtqueue *vq, int ops)
{
	uint16_t hdrlen = offsetof(struct vring_avail, ring);
	if (sc->sc_active_features & VIRTIO_F_RING_EVENT_IDX)
		hdrlen += sizeof(uint16_t);

	bus_dmamap_sync(sc->sc_dmat, vq->vq_dmamap,
			vq->vq_availoffset,
			hdrlen + sc->sc_nvqs * sizeof(uint16_t),
			ops);
}

static inline void
vq_sync_uring(struct virtio_softc *sc, struct virtqueue *vq, int ops)
{
	uint16_t hdrlen = offsetof(struct vring_used, ring);
	if (sc->sc_active_features & VIRTIO_F_RING_EVENT_IDX)
		hdrlen += sizeof(uint16_t);

	bus_dmamap_sync(sc->sc_dmat, vq->vq_dmamap,
			vq->vq_usedoffset,
			hdrlen + sc->sc_nvqs * sizeof(struct vring_used_elem),
			ops);
}

static inline void
vq_sync_indirect(struct virtio_softc *sc, struct virtqueue *vq, int slot,
		     int ops)
{
	int offset = vq->vq_indirectoffset
		      + sizeof(struct vring_desc) * vq->vq_maxnsegs * slot;

	bus_dmamap_sync(sc->sc_dmat, vq->vq_dmamap,
			offset, sizeof(struct vring_desc) * vq->vq_maxnsegs,
			ops);
}

/*
 * Can be used as sc_intrhand.
 */
/*
 * Scan vq, bus_dmamap_sync for the vqs (not for the payload),
 * and calls (*vq_done)() if some entries are consumed.
 */
bool
virtio_vq_is_enqueued(struct virtio_softc *sc, struct virtqueue *vq)
{

	if (vq->vq_queued) {
		vq->vq_queued = 0;
		vq_sync_aring(sc, vq, BUS_DMASYNC_POSTWRITE);
	}
	vq_sync_uring(sc, vq, BUS_DMASYNC_POSTREAD);
	membar_consumer();

	return (vq->vq_used_idx != virtio_rw16(sc, vq->vq_used->idx)) ? 1 : 0;
}

int
virtio_vq_intr(struct virtio_softc *sc)
{
	struct virtqueue *vq;
	int i, r = 0;

	for (i = 0; i < sc->sc_nvqs; i++) {
		vq = &sc->sc_vqs[i];
		if (virtio_vq_is_enqueued(sc, vq) == 1) {
			if (vq->vq_done)
				r |= (*vq->vq_done)(vq);
		}
	}

	return r;
}

int
virtio_vq_intrhand(struct virtio_softc *sc)
{
	struct virtqueue *vq;
	int i, r = 0;

	for (i = 0; i < sc->sc_nvqs; i++) {
		vq = &sc->sc_vqs[i];
		r |= (*vq->vq_intrhand)(vq->vq_intrhand_arg);
	}

	return r;
}


/*
 * Increase the event index in order to delay interrupts.
 */
int
virtio_postpone_intr(struct virtio_softc *sc, struct virtqueue *vq,
		uint16_t nslots)
{
	uint16_t	idx, nused;

	idx = vq->vq_used_idx + nslots;

	/* set the new event index: avail_ring->used_event = idx */
	*vq->vq_used_event = virtio_rw16(sc, idx);
	membar_producer();

	vq_sync_aring(vq->vq_owner, vq, BUS_DMASYNC_PREWRITE);
	vq->vq_queued++;

	nused = (uint16_t)
		(virtio_rw16(sc, vq->vq_used->idx) - vq->vq_used_idx);
	KASSERT(nused <= vq->vq_num);

	return nslots < nused;
}

/*
 * Postpone interrupt until 3/4 of the available descriptors have been
 * consumed.
 */
int
virtio_postpone_intr_smart(struct virtio_softc *sc, struct virtqueue *vq)
{
	uint16_t	nslots;

	nslots = (uint16_t)
		(virtio_rw16(sc, vq->vq_avail->idx) - vq->vq_used_idx) * 3 / 4;

	return virtio_postpone_intr(sc, vq, nslots);
}

/*
 * Postpone interrupt until all of the available descriptors have been
 * consumed.
 */
int
virtio_postpone_intr_far(struct virtio_softc *sc, struct virtqueue *vq)
{
	uint16_t	nslots;

	nslots = (uint16_t)
		(virtio_rw16(sc, vq->vq_avail->idx) - vq->vq_used_idx);

	return virtio_postpone_intr(sc, vq, nslots);
}

/*
 * Start/stop vq interrupt.  No guarantee.
 */
void
virtio_stop_vq_intr(struct virtio_softc *sc, struct virtqueue *vq)
{
	if (sc->sc_active_features & VIRTIO_F_RING_EVENT_IDX) {
		/*
		 * No way to disable the interrupt completely with
		 * RingEventIdx. Instead advance used_event by half the
		 * possible value. This won't happen soon and is far enough in
		 * the past to not trigger a spurios interrupt.
		 */
		*vq->vq_used_event = virtio_rw16(sc, vq->vq_used_idx + 0x8000);
	} else {
		vq->vq_avail->flags |= virtio_rw16(sc, VRING_AVAIL_F_NO_INTERRUPT);
	}
	vq_sync_aring(sc, vq, BUS_DMASYNC_PREWRITE);
	vq->vq_queued++;
}

int
virtio_start_vq_intr(struct virtio_softc *sc, struct virtqueue *vq)
{
	if (sc->sc_active_features & VIRTIO_F_RING_EVENT_IDX) {
		/*
		 * If event index feature is negotiated, enabling interrupts
		 * is done through setting the latest consumed index in the
		 * used_event field
		 */
		*vq->vq_used_event = virtio_rw16(sc, vq->vq_used_idx);
	} else {
		vq->vq_avail->flags &= ~virtio_rw16(sc, VRING_AVAIL_F_NO_INTERRUPT);
	}
	vq_sync_aring(sc, vq, BUS_DMASYNC_PREWRITE);
	vq->vq_queued++;

	return vq->vq_used_idx != virtio_rw16(sc, vq->vq_used->idx);
}

/*
 * Initialize vq structure.
 */
static void
virtio_init_vq(struct virtio_softc *sc, struct virtqueue *vq,
    const bool reinit)
{
	int i, j;
	int vq_size = vq->vq_num;

	memset(vq->vq_vaddr, 0, vq->vq_bytesize);

	/* build the indirect descriptor chain */
	if (vq->vq_indirect != NULL) {
		struct vring_desc *vd;

		for (i = 0; i < vq_size; i++) {
			vd = vq->vq_indirect;
			vd += vq->vq_maxnsegs * i;
			for (j = 0; j < vq->vq_maxnsegs-1; j++) {
				vd[j].next = virtio_rw16(sc, j + 1);
			}
		}
	}

	/* free slot management */
	SIMPLEQ_INIT(&vq->vq_freelist);
	for (i = 0; i < vq_size; i++) {
		SIMPLEQ_INSERT_TAIL(&vq->vq_freelist,
				    &vq->vq_entries[i], qe_list);
		vq->vq_entries[i].qe_index = i;
	}
	if (!reinit)
		mutex_init(&vq->vq_freelist_lock, MUTEX_SPIN, sc->sc_ipl);

	/* enqueue/dequeue status */
	vq->vq_avail_idx = 0;
	vq->vq_used_idx = 0;
	vq->vq_queued = 0;
	if (!reinit) {
		mutex_init(&vq->vq_aring_lock, MUTEX_SPIN, sc->sc_ipl);
		mutex_init(&vq->vq_uring_lock, MUTEX_SPIN, sc->sc_ipl);
	}
	vq_sync_aring(sc, vq, BUS_DMASYNC_PREWRITE);
	vq_sync_uring(sc, vq, BUS_DMASYNC_PREREAD);
	vq->vq_queued++;
}

/*
 * Allocate/free a vq.
 */
int
virtio_alloc_vq(struct virtio_softc *sc, struct virtqueue *vq, int index,
    int maxsegsize, int maxnsegs, const char *name)
{
	int vq_size, allocsize1, allocsize2, allocsize3, allocsize = 0;
	int rsegs, r, hdrlen;
#define VIRTQUEUE_ALIGN(n)	(((n)+(VIRTIO_PAGE_SIZE-1))&	\
				 ~(VIRTIO_PAGE_SIZE-1))

	/* Make sure callers allocate vqs in order */
	KASSERT(sc->sc_nvqs == index);

	memset(vq, 0, sizeof(*vq));

	vq_size = sc->sc_ops->read_queue_size(sc, index);
	if (vq_size == 0) {
		aprint_error_dev(sc->sc_dev,
				 "virtqueue not exist, index %d for %s\n",
				 index, name);
		goto err;
	}

	hdrlen = sc->sc_active_features & VIRTIO_F_RING_EVENT_IDX ? 3 : 2;

	/* allocsize1: descriptor table + avail ring + pad */
	allocsize1 = VIRTQUEUE_ALIGN(sizeof(struct vring_desc)*vq_size
			     + sizeof(uint16_t)*(hdrlen + vq_size));
	/* allocsize2: used ring + pad */
	allocsize2 = VIRTQUEUE_ALIGN(sizeof(uint16_t) * hdrlen
			     + sizeof(struct vring_used_elem)*vq_size);
	/* allocsize3: indirect table */
	if (sc->sc_indirect && maxnsegs >= MINSEG_INDIRECT)
		allocsize3 = sizeof(struct vring_desc) * maxnsegs * vq_size;
	else
		allocsize3 = 0;
	allocsize = allocsize1 + allocsize2 + allocsize3;

	/* alloc and map the memory */
	r = bus_dmamem_alloc(sc->sc_dmat, allocsize, VIRTIO_PAGE_SIZE, 0,
			     &vq->vq_segs[0], 1, &rsegs, BUS_DMA_WAITOK);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
				 "virtqueue %d for %s allocation failed, "
				 "error code %d\n", index, name, r);
		goto err;
	}
	r = bus_dmamem_map(sc->sc_dmat, &vq->vq_segs[0], rsegs, allocsize,
			   &vq->vq_vaddr, BUS_DMA_WAITOK);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
				 "virtqueue %d for %s map failed, "
				 "error code %d\n", index, name, r);
		goto err;
	}
	r = bus_dmamap_create(sc->sc_dmat, allocsize, 1, allocsize, 0,
			      BUS_DMA_WAITOK, &vq->vq_dmamap);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
				 "virtqueue %d for %s dmamap creation failed, "
				 "error code %d\n", index, name, r);
		goto err;
	}
	r = bus_dmamap_load(sc->sc_dmat, vq->vq_dmamap,
			    vq->vq_vaddr, allocsize, NULL, BUS_DMA_WAITOK);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
				 "virtqueue %d for %s dmamap load failed, "
				 "error code %d\n", index, name, r);
		goto err;
	}

	/* remember addresses and offsets for later use */
	vq->vq_owner = sc;
	vq->vq_num = vq_size;
	vq->vq_index = index;
	vq->vq_desc = vq->vq_vaddr;
	vq->vq_availoffset = sizeof(struct vring_desc)*vq_size;
	vq->vq_avail = (void*)(((char*)vq->vq_desc) + vq->vq_availoffset);
	vq->vq_used_event = (uint16_t *) ((char *)vq->vq_avail +
		 offsetof(struct vring_avail, ring[vq->vq_num]));
	vq->vq_usedoffset = allocsize1;
	vq->vq_used = (void*)(((char*)vq->vq_desc) + vq->vq_usedoffset);
	vq->vq_avail_event = (uint16_t *)((char *)vq->vq_used +
		 offsetof(struct vring_used, ring[vq->vq_num]));

	if (allocsize3 > 0) {
		vq->vq_indirectoffset = allocsize1 + allocsize2;
		vq->vq_indirect = (void*)(((char*)vq->vq_desc)
					  + vq->vq_indirectoffset);
	}
	vq->vq_bytesize = allocsize;
	vq->vq_maxsegsize = maxsegsize;
	vq->vq_maxnsegs = maxnsegs;

	/* free slot management */
	vq->vq_entries = kmem_zalloc(sizeof(struct vq_entry)*vq_size,
				     KM_SLEEP);
	virtio_init_vq(sc, vq, false);

	/* set the vq address */
	sc->sc_ops->setup_queue(sc, index,
	    vq->vq_dmamap->dm_segs[0].ds_addr);

	aprint_verbose_dev(sc->sc_dev,
			   "allocated %u byte for virtqueue %d for %s, "
			   "size %d\n", allocsize, index, name, vq_size);
	if (allocsize3 > 0)
		aprint_verbose_dev(sc->sc_dev,
				   "using %d byte (%d entries) "
				   "indirect descriptors\n",
				   allocsize3, maxnsegs * vq_size);

	sc->sc_nvqs++;

	return 0;

err:
	sc->sc_ops->setup_queue(sc, index, 0);
	if (vq->vq_dmamap)
		bus_dmamap_destroy(sc->sc_dmat, vq->vq_dmamap);
	if (vq->vq_vaddr)
		bus_dmamem_unmap(sc->sc_dmat, vq->vq_vaddr, allocsize);
	if (vq->vq_segs[0].ds_addr)
		bus_dmamem_free(sc->sc_dmat, &vq->vq_segs[0], 1);
	memset(vq, 0, sizeof(*vq));

	return -1;
}

int
virtio_free_vq(struct virtio_softc *sc, struct virtqueue *vq)
{
	struct vq_entry *qe;
	int i = 0;

	/* device must be already deactivated */
	/* confirm the vq is empty */
	SIMPLEQ_FOREACH(qe, &vq->vq_freelist, qe_list) {
		i++;
	}
	if (i != vq->vq_num) {
		printf("%s: freeing non-empty vq, index %d\n",
		       device_xname(sc->sc_dev), vq->vq_index);
		return EBUSY;
	}

	/* tell device that there's no virtqueue any longer */
	sc->sc_ops->setup_queue(sc, vq->vq_index, 0);

	kmem_free(vq->vq_entries, sizeof(*vq->vq_entries) * vq->vq_num);
	bus_dmamap_unload(sc->sc_dmat, vq->vq_dmamap);
	bus_dmamap_destroy(sc->sc_dmat, vq->vq_dmamap);
	bus_dmamem_unmap(sc->sc_dmat, vq->vq_vaddr, vq->vq_bytesize);
	bus_dmamem_free(sc->sc_dmat, &vq->vq_segs[0], 1);
	mutex_destroy(&vq->vq_freelist_lock);
	mutex_destroy(&vq->vq_uring_lock);
	mutex_destroy(&vq->vq_aring_lock);
	memset(vq, 0, sizeof(*vq));

	sc->sc_nvqs--;

	return 0;
}

/*
 * Free descriptor management.
 */
static struct vq_entry *
vq_alloc_entry(struct virtqueue *vq)
{
	struct vq_entry *qe;

	mutex_enter(&vq->vq_freelist_lock);
	if (SIMPLEQ_EMPTY(&vq->vq_freelist)) {
		mutex_exit(&vq->vq_freelist_lock);
		return NULL;
	}
	qe = SIMPLEQ_FIRST(&vq->vq_freelist);
	SIMPLEQ_REMOVE_HEAD(&vq->vq_freelist, qe_list);
	mutex_exit(&vq->vq_freelist_lock);

	return qe;
}

static void
vq_free_entry(struct virtqueue *vq, struct vq_entry *qe)
{
	mutex_enter(&vq->vq_freelist_lock);
	SIMPLEQ_INSERT_TAIL(&vq->vq_freelist, qe, qe_list);
	mutex_exit(&vq->vq_freelist_lock);

	return;
}

/*
 * Enqueue several dmamaps as a single request.
 */
/*
 * Typical usage:
 *  <queue size> number of followings are stored in arrays
 *  - command blocks (in dmamem) should be pre-allocated and mapped
 *  - dmamaps for command blocks should be pre-allocated and loaded
 *  - dmamaps for payload should be pre-allocated
 *      r = virtio_enqueue_prep(sc, vq, &slot);		// allocate a slot
 *	if (r)		// currently 0 or EAGAIN
 *	  return r;
 *	r = bus_dmamap_load(dmat, dmamap_payload[slot], data, count, ..);
 *	if (r) {
 *	  virtio_enqueue_abort(sc, vq, slot);
 *	  return r;
 *	}
 *	r = virtio_enqueue_reserve(sc, vq, slot,
 *				   dmamap_payload[slot]->dm_nsegs+1);
 *							// ^ +1 for command
 *	if (r) {	// currently 0 or EAGAIN
 *	  bus_dmamap_unload(dmat, dmamap_payload[slot]);
 *	  return r;					// do not call abort()
 *	}
 *	<setup and prepare commands>
 *	bus_dmamap_sync(dmat, dmamap_cmd[slot],... BUS_DMASYNC_PREWRITE);
 *	bus_dmamap_sync(dmat, dmamap_payload[slot],...);
 *	virtio_enqueue(sc, vq, slot, dmamap_cmd[slot], false);
 *	virtio_enqueue(sc, vq, slot, dmamap_payload[slot], iswrite);
 *	virtio_enqueue_commit(sc, vq, slot, true);
 */

/*
 * enqueue_prep: allocate a slot number
 */
int
virtio_enqueue_prep(struct virtio_softc *sc, struct virtqueue *vq, int *slotp)
{
	struct vq_entry *qe1;

	KASSERT(slotp != NULL);

	qe1 = vq_alloc_entry(vq);
	if (qe1 == NULL)
		return EAGAIN;
	/* next slot is not allocated yet */
	qe1->qe_next = -1;
	*slotp = qe1->qe_index;

	return 0;
}

/*
 * enqueue_reserve: allocate remaining slots and build the descriptor chain.
 */
int
virtio_enqueue_reserve(struct virtio_softc *sc, struct virtqueue *vq,
		       int slot, int nsegs)
{
	int indirect;
	struct vq_entry *qe1 = &vq->vq_entries[slot];

	KASSERT(qe1->qe_next == -1);
	KASSERT(1 <= nsegs && nsegs <= vq->vq_num);

	if ((vq->vq_indirect != NULL) &&
	    (nsegs >= MINSEG_INDIRECT) &&
	    (nsegs <= vq->vq_maxnsegs))
		indirect = 1;
	else
		indirect = 0;
	qe1->qe_indirect = indirect;

	if (indirect) {
		struct vring_desc *vd;
		uint64_t addr;
		int i;

		vd = &vq->vq_desc[qe1->qe_index];
		addr = vq->vq_dmamap->dm_segs[0].ds_addr
			+ vq->vq_indirectoffset;
		addr += sizeof(struct vring_desc)
			* vq->vq_maxnsegs * qe1->qe_index;
		vd->addr  = virtio_rw64(sc, addr);
		vd->len   = virtio_rw32(sc, sizeof(struct vring_desc) * nsegs);
		vd->flags = virtio_rw16(sc, VRING_DESC_F_INDIRECT);

		vd = vq->vq_indirect;
		vd += vq->vq_maxnsegs * qe1->qe_index;
		qe1->qe_desc_base = vd;

		for (i = 0; i < nsegs-1; i++) {
			vd[i].flags = virtio_rw16(sc, VRING_DESC_F_NEXT);
		}
		vd[i].flags  = virtio_rw16(sc, 0);
		qe1->qe_next = 0;

		return 0;
	} else {
		struct vring_desc *vd;
		struct vq_entry *qe;
		int i, s;

		vd = &vq->vq_desc[0];
		qe1->qe_desc_base = vd;
		qe1->qe_next = qe1->qe_index;
		s = slot;
		for (i = 0; i < nsegs - 1; i++) {
			qe = vq_alloc_entry(vq);
			if (qe == NULL) {
				vd[s].flags = virtio_rw16(sc, 0);
				virtio_enqueue_abort(sc, vq, slot);
				return EAGAIN;
			}
			vd[s].flags = virtio_rw16(sc, VRING_DESC_F_NEXT);
			vd[s].next  = virtio_rw16(sc, qe->qe_index);
			s = qe->qe_index;
		}
		vd[s].flags = virtio_rw16(sc, 0);

		return 0;
	}
}

/*
 * enqueue: enqueue a single dmamap.
 */
int
virtio_enqueue(struct virtio_softc *sc, struct virtqueue *vq, int slot,
	       bus_dmamap_t dmamap, bool write)
{
	struct vq_entry *qe1 = &vq->vq_entries[slot];
	struct vring_desc *vd = qe1->qe_desc_base;
	int i;
	int s = qe1->qe_next;

	KASSERT(s >= 0);
	KASSERT(dmamap->dm_nsegs > 0);

	for (i = 0; i < dmamap->dm_nsegs; i++) {
		vd[s].addr = virtio_rw64(sc, dmamap->dm_segs[i].ds_addr);
		vd[s].len  = virtio_rw32(sc, dmamap->dm_segs[i].ds_len);
		if (!write)
			vd[s].flags |= virtio_rw16(sc, VRING_DESC_F_WRITE);
		s = virtio_rw16(sc, vd[s].next);
	}
	qe1->qe_next = s;

	return 0;
}

int
virtio_enqueue_p(struct virtio_softc *sc, struct virtqueue *vq, int slot,
		 bus_dmamap_t dmamap, bus_addr_t start, bus_size_t len,
		 bool write)
{
	struct vq_entry *qe1 = &vq->vq_entries[slot];
	struct vring_desc *vd = qe1->qe_desc_base;
	int s = qe1->qe_next;

	KASSERT(s >= 0);
	KASSERT(dmamap->dm_nsegs == 1); /* XXX */
	KASSERT((dmamap->dm_segs[0].ds_len > start) &&
		(dmamap->dm_segs[0].ds_len >= start + len));

	vd[s].addr = virtio_rw64(sc, dmamap->dm_segs[0].ds_addr + start);
	vd[s].len  = virtio_rw32(sc, len);
	if (!write)
		vd[s].flags |= virtio_rw16(sc, VRING_DESC_F_WRITE);
	qe1->qe_next = virtio_rw16(sc, vd[s].next);

	return 0;
}

/*
 * enqueue_commit: add it to the aring.
 */
int
virtio_enqueue_commit(struct virtio_softc *sc, struct virtqueue *vq, int slot,
		      bool notifynow)
{
	struct vq_entry *qe1;

	if (slot < 0) {
		mutex_enter(&vq->vq_aring_lock);
		goto notify;
	}
	vq_sync_descs(sc, vq, BUS_DMASYNC_PREWRITE);
	qe1 = &vq->vq_entries[slot];
	if (qe1->qe_indirect)
		vq_sync_indirect(sc, vq, slot, BUS_DMASYNC_PREWRITE);
	mutex_enter(&vq->vq_aring_lock);
	vq->vq_avail->ring[(vq->vq_avail_idx++) % vq->vq_num] =
		virtio_rw16(sc, slot);

notify:
	if (notifynow) {
		uint16_t o, n, t;
		uint16_t flags;
		o = virtio_rw16(sc, vq->vq_avail->idx);
		n = vq->vq_avail_idx;

		/* publish avail idx */
		membar_producer();
		vq->vq_avail->idx = virtio_rw16(sc, vq->vq_avail_idx);
		vq_sync_aring(sc, vq, BUS_DMASYNC_PREWRITE);
		vq->vq_queued++;

		membar_consumer();
		vq_sync_uring(sc, vq, BUS_DMASYNC_PREREAD);
		if (sc->sc_active_features & VIRTIO_F_RING_EVENT_IDX) {
			t = virtio_rw16(sc, *vq->vq_avail_event) + 1;
			if ((uint16_t) (n - t) < (uint16_t) (n - o))
				sc->sc_ops->kick(sc, vq->vq_index);
		} else {
			flags = virtio_rw16(sc, vq->vq_used->flags);
			if (!(flags & VRING_USED_F_NO_NOTIFY))
				sc->sc_ops->kick(sc, vq->vq_index);
		}
		vq_sync_uring(sc, vq, BUS_DMASYNC_POSTREAD);
		vq_sync_aring(sc, vq, BUS_DMASYNC_POSTWRITE);
	}
	mutex_exit(&vq->vq_aring_lock);

	return 0;
}

/*
 * enqueue_abort: rollback.
 */
int
virtio_enqueue_abort(struct virtio_softc *sc, struct virtqueue *vq, int slot)
{
	struct vq_entry *qe = &vq->vq_entries[slot];
	struct vring_desc *vd;
	int s;

	if (qe->qe_next < 0) {
		vq_free_entry(vq, qe);
		return 0;
	}

	s = slot;
	vd = &vq->vq_desc[0];
	while (virtio_rw16(sc, vd[s].flags) & VRING_DESC_F_NEXT) {
		s = virtio_rw16(sc, vd[s].next);
		vq_free_entry(vq, qe);
		qe = &vq->vq_entries[s];
	}
	vq_free_entry(vq, qe);
	return 0;
}

/*
 * Dequeue a request.
 */
/*
 * dequeue: dequeue a request from uring; dmamap_sync for uring is
 *	    already done in the interrupt handler.
 */
int
virtio_dequeue(struct virtio_softc *sc, struct virtqueue *vq,
	       int *slotp, int *lenp)
{
	uint16_t slot, usedidx;
	struct vq_entry *qe;

	if (vq->vq_used_idx == virtio_rw16(sc, vq->vq_used->idx))
		return ENOENT;
	mutex_enter(&vq->vq_uring_lock);
	usedidx = vq->vq_used_idx++;
	mutex_exit(&vq->vq_uring_lock);
	usedidx %= vq->vq_num;
	slot = virtio_rw32(sc, vq->vq_used->ring[usedidx].id);
	qe = &vq->vq_entries[slot];

	if (qe->qe_indirect)
		vq_sync_indirect(sc, vq, slot, BUS_DMASYNC_POSTWRITE);

	if (slotp)
		*slotp = slot;
	if (lenp)
		*lenp = virtio_rw32(sc, vq->vq_used->ring[usedidx].len);

	return 0;
}

/*
 * dequeue_commit: complete dequeue; the slot is recycled for future use.
 *                 if you forget to call this the slot will be leaked.
 */
int
virtio_dequeue_commit(struct virtio_softc *sc, struct virtqueue *vq, int slot)
{
	struct vq_entry *qe = &vq->vq_entries[slot];
	struct vring_desc *vd = &vq->vq_desc[0];
	int s = slot;

	while (virtio_rw16(sc, vd[s].flags) & VRING_DESC_F_NEXT) {
		s = virtio_rw16(sc, vd[s].next);
		vq_free_entry(vq, qe);
		qe = &vq->vq_entries[s];
	}
	vq_free_entry(vq, qe);

	return 0;
}

/*
 * Attach a child, fill all the members.
 */
void
virtio_child_attach_start(struct virtio_softc *sc, device_t child, int ipl,
		    struct virtqueue *vqs,
		    virtio_callback config_change,
		    virtio_callback intr_hand,
		    int req_flags, int req_features, const char *feat_bits)
{
	char buf[1024];

	sc->sc_child = child;
	sc->sc_ipl = ipl;
	sc->sc_vqs = vqs;
	sc->sc_config_change = config_change;
	sc->sc_intrhand = intr_hand;
	sc->sc_flags = req_flags;

	virtio_negotiate_features(sc, req_features);
	snprintb(buf, sizeof(buf), feat_bits, sc->sc_active_features);
	aprint_normal(": features: %s\n", buf);
	aprint_naive("\n");
}

void
virtio_child_attach_set_vqs(struct virtio_softc *sc,
    struct virtqueue *vqs, int nvq_pairs)
{

	KASSERT(nvq_pairs == 1 ||
	    (sc->sc_flags & VIRTIO_F_INTR_SOFTINT) == 0);
	if (nvq_pairs > 1)
		sc->sc_child_mq = true;

	sc->sc_vqs = vqs;
}

int
virtio_child_attach_finish(struct virtio_softc *sc)
{
	int r;

	sc->sc_finished_called = true;
	r = sc->sc_ops->alloc_interrupts(sc);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev, "failed to allocate interrupts\n");
		goto fail;
	}

	r = sc->sc_ops->setup_interrupts(sc, 0);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev, "failed to setup interrupts\n");
		goto fail;
	}

	KASSERT(sc->sc_soft_ih == NULL);
	if (sc->sc_flags & VIRTIO_F_INTR_SOFTINT) {
		u_int flags = SOFTINT_NET;
		if (sc->sc_flags & VIRTIO_F_INTR_MPSAFE)
			flags |= SOFTINT_MPSAFE;

		sc->sc_soft_ih = softint_establish(flags, virtio_soft_intr, sc);
		if (sc->sc_soft_ih == NULL) {
			sc->sc_ops->free_interrupts(sc);
			aprint_error_dev(sc->sc_dev,
			    "failed to establish soft interrupt\n");
			goto fail;
		}
	}

	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_DRIVER_OK);
	return 0;

fail:
	if (sc->sc_soft_ih) {
		softint_disestablish(sc->sc_soft_ih);
		sc->sc_soft_ih = NULL;
	}

	sc->sc_ops->free_interrupts(sc);

	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_FAILED);
	return 1;
}

void
virtio_child_detach(struct virtio_softc *sc)
{
	sc->sc_child = NULL;
	sc->sc_vqs = NULL;

	virtio_device_reset(sc);

	sc->sc_ops->free_interrupts(sc);

	if (sc->sc_soft_ih) {
		softint_disestablish(sc->sc_soft_ih);
		sc->sc_soft_ih = NULL;
	}
}

void
virtio_child_attach_failed(struct virtio_softc *sc)
{
	virtio_child_detach(sc);

	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_FAILED);

	sc->sc_child = VIRTIO_CHILD_FAILED;
}

bus_dma_tag_t
virtio_dmat(struct virtio_softc *sc)
{
	return sc->sc_dmat;
}

device_t
virtio_child(struct virtio_softc *sc)
{
	return sc->sc_child;
}

int
virtio_intrhand(struct virtio_softc *sc)
{
	return (*sc->sc_intrhand)(sc);
}

uint64_t
virtio_features(struct virtio_softc *sc)
{
	return sc->sc_active_features;
}

int
virtio_attach_failed(struct virtio_softc *sc)
{
	device_t self = sc->sc_dev;

	/* no error if its not connected, but its failed */
	if (sc->sc_childdevid == 0)
		return 1;

	if (sc->sc_child == NULL) {
		aprint_error_dev(self,
			"no matching child driver; not configured\n");
		return 1;
	}

	if (sc->sc_child == VIRTIO_CHILD_FAILED) {
		aprint_error_dev(self, "virtio configuration failed\n");
		return 1;
	}

	/* sanity check */
	if (!sc->sc_finished_called) {
		aprint_error_dev(self, "virtio internal error, child driver "
			"signaled OK but didn't initialize interrupts\n");
		return 1;
	}

	return 0;
}

void
virtio_print_device_type(device_t self, int id, int revision)
{
	aprint_normal_dev(self, "%s device (rev. 0x%02x)\n",
		  (id < NDEVNAMES ? virtio_device_name[id] : "Unknown"),
		  revision);
}


MODULE(MODULE_CLASS_DRIVER, virtio, NULL);

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
virtio_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

#ifdef _MODULE
	switch (cmd) {
	case MODULE_CMD_INIT:
		error = config_init_component(cfdriver_ioconf_virtio,
		    cfattach_ioconf_virtio, cfdata_ioconf_virtio);
		break;
	case MODULE_CMD_FINI:
		error = config_fini_component(cfdriver_ioconf_virtio,
		    cfattach_ioconf_virtio, cfdata_ioconf_virtio);
		break;
	default:
		error = ENOTTY;
		break;
	}
#endif

	return error;
}
