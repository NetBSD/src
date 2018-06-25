/*	$NetBSD: virtio.c,v 1.30.2.1 2018/06/25 07:26:01 pgoyette Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: virtio.c,v 1.30.2.1 2018/06/25 07:26:01 pgoyette Exp $");

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
 *	virtio_reinit_begin(sc);     // dequeue prohibitted
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

void
virtio_reinit_start(struct virtio_softc *sc)
{
	int i;

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
		    vq->vq_dmamap->dm_segs[0].ds_addr / VIRTIO_PAGE_SIZE);
	}
}

void
virtio_reinit_end(struct virtio_softc *sc)
{
	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_DRIVER_OK);
}

/*
 * Feature negotiation.
 */
uint32_t
virtio_negotiate_features(struct virtio_softc *sc, uint32_t guest_features)
{
	uint32_t r;

	if (!(device_cfdata(sc->sc_dev)->cf_flags & 1) &&
	    !(device_cfdata(sc->sc_child)->cf_flags & 1)) /* XXX */
		guest_features |= VIRTIO_F_RING_INDIRECT_DESC;
	r = sc->sc_ops->neg_features(sc, guest_features);
	sc->sc_features = r;
	if (r & VIRTIO_F_RING_INDIRECT_DESC)
		sc->sc_indirect = true;
	else
		sc->sc_indirect = false;

	return r;
}

/*
 * Device configuration registers.
 */
uint8_t
virtio_read_device_config_1(struct virtio_softc *sc, int index)
{
	return sc->sc_ops->read_dev_cfg_1(sc, index);
}

uint16_t
virtio_read_device_config_2(struct virtio_softc *sc, int index)
{
	return sc->sc_ops->read_dev_cfg_2(sc, index);
}

uint32_t
virtio_read_device_config_4(struct virtio_softc *sc, int index)
{
	return sc->sc_ops->read_dev_cfg_4(sc, index);
}

uint64_t
virtio_read_device_config_8(struct virtio_softc *sc, int index)
{
	return sc->sc_ops->read_dev_cfg_8(sc, index);
}

void
virtio_write_device_config_1(struct virtio_softc *sc,
			     int index, uint8_t value)
{
	return sc->sc_ops->write_dev_cfg_1(sc, index, value);
}

void
virtio_write_device_config_2(struct virtio_softc *sc,
			     int index, uint16_t value)
{
	return sc->sc_ops->write_dev_cfg_2(sc, index, value);
}

void
virtio_write_device_config_4(struct virtio_softc *sc,
			     int index, uint32_t value)
{
	return sc->sc_ops->write_dev_cfg_4(sc, index, value);
}

void
virtio_write_device_config_8(struct virtio_softc *sc,
			     int index, uint64_t value)
{
	return sc->sc_ops->write_dev_cfg_8(sc, index, value);
}

/*
 * Interrupt handler.
 */
static void
virtio_soft_intr(void *arg)
{
	struct virtio_softc *sc = arg;

	KASSERT(sc->sc_intrhand != NULL);

	(sc->sc_intrhand)(sc);
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
	bus_dmamap_sync(sc->sc_dmat, vq->vq_dmamap,
			vq->vq_availoffset,
			offsetof(struct vring_avail, ring)
			 + vq->vq_num * sizeof(uint16_t),
			ops);
}

static inline void
vq_sync_uring(struct virtio_softc *sc, struct virtqueue *vq, int ops)
{
	bus_dmamap_sync(sc->sc_dmat, vq->vq_dmamap,
			vq->vq_usedoffset,
			offsetof(struct vring_used, ring)
			 + vq->vq_num * sizeof(struct vring_used_elem),
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
int
virtio_vq_intr(struct virtio_softc *sc)
{
	struct virtqueue *vq;
	int i, r = 0;

	for (i = 0; i < sc->sc_nvqs; i++) {
		vq = &sc->sc_vqs[i];
		if (vq->vq_queued) {
			vq->vq_queued = 0;
			vq_sync_aring(sc, vq, BUS_DMASYNC_POSTWRITE);
		}
		vq_sync_uring(sc, vq, BUS_DMASYNC_POSTREAD);
		membar_consumer();
		if (vq->vq_used_idx != vq->vq_used->idx) {
			if (vq->vq_done)
				r |= (vq->vq_done)(vq);
		}
	}

	return r;
}

/*
 * Start/stop vq interrupt.  No guarantee.
 */
void
virtio_stop_vq_intr(struct virtio_softc *sc, struct virtqueue *vq)
{
	vq->vq_avail->flags |= VRING_AVAIL_F_NO_INTERRUPT;
	vq_sync_aring(sc, vq, BUS_DMASYNC_PREWRITE);
	vq->vq_queued++;
}

void
virtio_start_vq_intr(struct virtio_softc *sc, struct virtqueue *vq)
{
	vq->vq_avail->flags &= ~VRING_AVAIL_F_NO_INTERRUPT;
	vq_sync_aring(sc, vq, BUS_DMASYNC_PREWRITE);
	vq->vq_queued++;
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
				vd[j].next = j + 1;
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
	int rsegs, r;
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
	/* allocsize1: descriptor table + avail ring + pad */
	allocsize1 = VIRTQUEUE_ALIGN(sizeof(struct vring_desc)*vq_size
				     + sizeof(uint16_t)*(2+vq_size));
	/* allocsize2: used ring + pad */
	allocsize2 = VIRTQUEUE_ALIGN(sizeof(uint16_t)*2
				     + sizeof(struct vring_used_elem)*vq_size);
	/* allocsize3: indirect table */
	if (sc->sc_indirect && maxnsegs >= MINSEG_INDIRECT)
		allocsize3 = sizeof(struct vring_desc) * maxnsegs * vq_size;
	else
		allocsize3 = 0;
	allocsize = allocsize1 + allocsize2 + allocsize3;

	/* alloc and map the memory */
	r = bus_dmamem_alloc(sc->sc_dmat, allocsize, VIRTIO_PAGE_SIZE, 0,
			     &vq->vq_segs[0], 1, &rsegs, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
				 "virtqueue %d for %s allocation failed, "
				 "error code %d\n", index, name, r);
		goto err;
	}
	r = bus_dmamem_map(sc->sc_dmat, &vq->vq_segs[0], 1, allocsize,
			   &vq->vq_vaddr, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
				 "virtqueue %d for %s map failed, "
				 "error code %d\n", index, name, r);
		goto err;
	}
	r = bus_dmamap_create(sc->sc_dmat, allocsize, 1, allocsize, 0,
			      BUS_DMA_NOWAIT, &vq->vq_dmamap);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
				 "virtqueue %d for %s dmamap creation failed, "
				 "error code %d\n", index, name, r);
		goto err;
	}
	r = bus_dmamap_load(sc->sc_dmat, vq->vq_dmamap,
			    vq->vq_vaddr, allocsize, NULL, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
				 "virtqueue %d for %s dmamap load failed, "
				 "error code %d\n", index, name, r);
		goto err;
	}

	/* set the vq address */
	sc->sc_ops->setup_queue(sc, index,
	    vq->vq_dmamap->dm_segs[0].ds_addr / VIRTIO_PAGE_SIZE);

	/* remember addresses and offsets for later use */
	vq->vq_owner = sc;
	vq->vq_num = vq_size;
	vq->vq_index = index;
	vq->vq_desc = vq->vq_vaddr;
	vq->vq_availoffset = sizeof(struct vring_desc)*vq_size;
	vq->vq_avail = (void*)(((char*)vq->vq_desc) + vq->vq_availoffset);
	vq->vq_usedoffset = allocsize1;
	vq->vq_used = (void*)(((char*)vq->vq_desc) + vq->vq_usedoffset);
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
				     KM_NOSLEEP);
	if (vq->vq_entries == NULL) {
		r = ENOMEM;
		goto err;
	}

	virtio_init_vq(sc, vq, false);

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
		int i;

		vd = &vq->vq_desc[qe1->qe_index];
		vd->addr = vq->vq_dmamap->dm_segs[0].ds_addr
			+ vq->vq_indirectoffset;
		vd->addr += sizeof(struct vring_desc)
			* vq->vq_maxnsegs * qe1->qe_index;
		vd->len = sizeof(struct vring_desc) * nsegs;
		vd->flags = VRING_DESC_F_INDIRECT;

		vd = vq->vq_indirect;
		vd += vq->vq_maxnsegs * qe1->qe_index;
		qe1->qe_desc_base = vd;

		for (i = 0; i < nsegs-1; i++) {
			vd[i].flags = VRING_DESC_F_NEXT;
		}
		vd[i].flags = 0;
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
				vd[s].flags = 0;
				virtio_enqueue_abort(sc, vq, slot);
				return EAGAIN;
			}
			vd[s].flags = VRING_DESC_F_NEXT;
			vd[s].next = qe->qe_index;
			s = qe->qe_index;
		}
		vd[s].flags = 0;

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
		vd[s].addr = dmamap->dm_segs[i].ds_addr;
		vd[s].len = dmamap->dm_segs[i].ds_len;
		if (!write)
			vd[s].flags |= VRING_DESC_F_WRITE;
		s = vd[s].next;
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

	vd[s].addr = dmamap->dm_segs[0].ds_addr + start;
	vd[s].len = len;
	if (!write)
		vd[s].flags |= VRING_DESC_F_WRITE;
	qe1->qe_next = vd[s].next;

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
	vq->vq_avail->ring[(vq->vq_avail_idx++) % vq->vq_num] = slot;

notify:
	if (notifynow) {
		vq_sync_aring(sc, vq, BUS_DMASYNC_PREWRITE);
		vq_sync_uring(sc, vq, BUS_DMASYNC_PREREAD);
		membar_producer();
		vq->vq_avail->idx = vq->vq_avail_idx;
		vq_sync_aring(sc, vq, BUS_DMASYNC_PREWRITE);
		membar_producer();
		vq->vq_queued++;
		vq_sync_uring(sc, vq, BUS_DMASYNC_POSTREAD);
		membar_consumer();
		if (!(vq->vq_used->flags & VRING_USED_F_NO_NOTIFY))
			sc->sc_ops->kick(sc, vq->vq_index);
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
	while (vd[s].flags & VRING_DESC_F_NEXT) {
		s = vd[s].next;
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

	if (vq->vq_used_idx == vq->vq_used->idx)
		return ENOENT;
	mutex_enter(&vq->vq_uring_lock);
	usedidx = vq->vq_used_idx++;
	mutex_exit(&vq->vq_uring_lock);
	usedidx %= vq->vq_num;
	slot = vq->vq_used->ring[usedidx].id;
	qe = &vq->vq_entries[slot];

	if (qe->qe_indirect)
		vq_sync_indirect(sc, vq, slot, BUS_DMASYNC_POSTWRITE);

	if (slotp)
		*slotp = slot;
	if (lenp)
		*lenp = vq->vq_used->ring[usedidx].len;

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

	while (vd[s].flags & VRING_DESC_F_NEXT) {
		s = vd[s].next;
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
	char buf[256];
	int features;

	sc->sc_child = child;
	sc->sc_ipl = ipl;
	sc->sc_vqs = vqs;
	sc->sc_config_change = config_change;
	sc->sc_intrhand = intr_hand;
	sc->sc_flags = req_flags;

	features = virtio_negotiate_features(sc, req_features);
	snprintb(buf, sizeof(buf), feat_bits, features);
	aprint_normal(": Features: %s\n", buf);
	aprint_naive("\n");
}

int
virtio_child_attach_finish(struct virtio_softc *sc)
{
	int r;

	r = sc->sc_ops->setup_interrupts(sc);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev, "failed to setup interrupts\n");
		goto fail;
	}

	KASSERT(sc->sc_soft_ih == NULL);
	if (sc->sc_flags & VIRTIO_F_PCI_INTR_SOFTINT) {
		u_int flags = SOFTINT_NET; 
		if (sc->sc_flags & VIRTIO_F_PCI_INTR_MPSAFE)
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
	return (sc->sc_intrhand)(sc);
}

uint32_t
virtio_features(struct virtio_softc *sc)
{
	return sc->sc_features;
}

int
virtiobusprint(void *aux, const char *pnp)
{
	struct virtio_attach_args * const va = aux;

	if (pnp)
		aprint_normal("Device ID %d at %s", va->sc_childdevid, pnp);

	return UNCONF;
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
