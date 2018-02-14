/*	$NetBSD: virtio.c,v 1.30 2018/02/14 14:04:48 uwe Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: virtio.c,v 1.30 2018/02/14 14:04:48 uwe Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/module.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#define VIRTIO_PRIVATE

#include <dev/pci/virtioreg.h> /* XXX: move to non-pci */
#include <dev/pci/virtiovar.h> /* XXX: move to non-pci */

#define MINSEG_INDIRECT		2 /* use indirect if nsegs >= this value */

static int	virtio_intr(void *arg);
static int	virtio_msix_queue_intr(void *);
static int	virtio_msix_config_intr(void *);
static int	virtio_setup_msix_vectors(struct virtio_softc *);
static int	virtio_setup_msix_interrupts(struct virtio_softc *,
		    struct pci_attach_args *);
static int	virtio_setup_intx_interrupt(struct virtio_softc *,
		    struct pci_attach_args *);
static int	virtio_setup_interrupts(struct virtio_softc *);
static void	virtio_free_interrupts(struct virtio_softc *);
static void	virtio_soft_intr(void *arg);
static void	virtio_init_vq(struct virtio_softc *,
		    struct virtqueue *, const bool);


/* we use the legacy virtio spec, so the pci registers are host native
 * byte order, not pci (i.e. LE) byte order */
static inline uint16_t
nbo_bus_space_read_2(bus_space_tag_t space, bus_space_handle_t handle,
         bus_size_t offset)
{
	return le16toh(bus_space_read_2(space, handle, offset));
}

static inline uint32_t
nbo_bus_space_read_4(bus_space_tag_t space, bus_space_handle_t handle,
	bus_size_t offset)
{
	return le32toh(bus_space_read_4(space, handle, offset));
}

static void
nbo_bus_space_write_2(bus_space_tag_t space, bus_space_handle_t handle,
	bus_size_t offset, uint16_t value)
{
	bus_space_write_2(space, handle, offset, htole16(value));
}

static void
nbo_bus_space_write_4(bus_space_tag_t space, bus_space_handle_t handle,
	bus_size_t offset, uint32_t value)
{
	bus_space_write_4(space, handle, offset, htole32(value));
}

/* some functions access registers at 4 byte offset for little/high halves */
#if BYTE_ORDER == BIG_ENDIAN
#define REG_HI_OFF	0
#define REG_LO_OFF	4
#else
#define REG_HI_OFF	4
#define REG_LO_OFF	0
#endif

void
virtio_set_status(struct virtio_softc *sc, int status)
{
	int old = 0;

	if (status != 0)
		old = bus_space_read_1(sc->sc_iot, sc->sc_ioh,
				       VIRTIO_CONFIG_DEVICE_STATUS);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, VIRTIO_CONFIG_DEVICE_STATUS,
			  status|old);
}

#define VIRTIO_MSIX_CONFIG_VECTOR_INDEX	0
#define VIRTIO_MSIX_QUEUE_VECTOR_INDEX	1

static int
virtio_setup_msix_vectors(struct virtio_softc *sc)
{
	int offset, vector, ret, qid;

	offset = VIRTIO_CONFIG_MSI_CONFIG_VECTOR;
	vector = VIRTIO_MSIX_CONFIG_VECTOR_INDEX;

	nbo_bus_space_write_2(sc->sc_iot, sc->sc_ioh, offset, vector);
	ret = nbo_bus_space_read_2(sc->sc_iot, sc->sc_ioh, offset);
	aprint_debug_dev(sc->sc_dev, "expected=%d, actual=%d\n",
	    vector, ret);
	if (ret != vector)
		return -1;

	for (qid = 0; qid < sc->sc_nvqs; qid++) {
		offset = VIRTIO_CONFIG_QUEUE_SELECT;
		nbo_bus_space_write_2(sc->sc_iot, sc->sc_ioh, offset, qid);

		offset = VIRTIO_CONFIG_MSI_QUEUE_VECTOR;
		vector = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;

		nbo_bus_space_write_2(sc->sc_iot, sc->sc_ioh, offset, vector);
		ret = nbo_bus_space_read_2(sc->sc_iot, sc->sc_ioh, offset);
		aprint_debug_dev(sc->sc_dev, "expected=%d, actual=%d\n",
		    vector, ret);
		if (ret != vector)
			return -1;
	}

	return 0;
}

static int
virtio_setup_msix_interrupts(struct virtio_softc *sc,
    struct pci_attach_args *pa)
{
	device_t self = sc->sc_dev;
	pci_chipset_tag_t pc = pa->pa_pc;
	char intrbuf[PCI_INTRSTR_LEN];
	char const *intrstr;
	int idx;

	idx = VIRTIO_MSIX_CONFIG_VECTOR_INDEX;
	if (sc->sc_flags & VIRTIO_F_PCI_INTR_MPSAFE)
		pci_intr_setattr(pc, &sc->sc_ihp[idx], PCI_INTR_MPSAFE, true);

	sc->sc_ihs[idx] = pci_intr_establish_xname(pc, sc->sc_ihp[idx],
	    sc->sc_ipl, virtio_msix_config_intr, sc, device_xname(sc->sc_dev));
	if (sc->sc_ihs[idx] == NULL) {
		aprint_error_dev(self, "couldn't establish MSI-X for config\n");
		goto error;
	}

	idx = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;
	if (sc->sc_flags & VIRTIO_F_PCI_INTR_MPSAFE)
		pci_intr_setattr(pc, &sc->sc_ihp[idx], PCI_INTR_MPSAFE, true);

	sc->sc_ihs[idx] = pci_intr_establish_xname(pc, sc->sc_ihp[idx],
	    sc->sc_ipl, virtio_msix_queue_intr, sc, device_xname(sc->sc_dev));
	if (sc->sc_ihs[idx] == NULL) {
		aprint_error_dev(self, "couldn't establish MSI-X for queues\n");
		goto error;
	}

	if (virtio_setup_msix_vectors(sc) != 0) {
		aprint_error_dev(self, "couldn't setup MSI-X vectors\n");
		goto error;
	}

	idx = VIRTIO_MSIX_CONFIG_VECTOR_INDEX;
	intrstr = pci_intr_string(pc, sc->sc_ihp[idx], intrbuf, sizeof(intrbuf));
	aprint_normal_dev(self, "config interrupting at %s\n", intrstr);
	idx = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;
	intrstr = pci_intr_string(pc, sc->sc_ihp[idx], intrbuf, sizeof(intrbuf));
	aprint_normal_dev(self, "queues interrupting at %s\n", intrstr);

	return 0;

error:
	idx = VIRTIO_MSIX_CONFIG_VECTOR_INDEX;
	if (sc->sc_ihs[idx] != NULL)
		pci_intr_disestablish(sc->sc_pc, sc->sc_ihs[idx]);
	idx = VIRTIO_MSIX_QUEUE_VECTOR_INDEX;
	if (sc->sc_ihs[idx] != NULL)
		pci_intr_disestablish(sc->sc_pc, sc->sc_ihs[idx]);

	return -1;
}

static int
virtio_setup_intx_interrupt(struct virtio_softc *sc,
    struct pci_attach_args *pa)
{
	device_t self = sc->sc_dev;
	pci_chipset_tag_t pc = pa->pa_pc;
	char intrbuf[PCI_INTRSTR_LEN];
	char const *intrstr;

	if (sc->sc_flags & VIRTIO_F_PCI_INTR_MPSAFE)
		pci_intr_setattr(pc, &sc->sc_ihp[0], PCI_INTR_MPSAFE, true);

	sc->sc_ihs[0] = pci_intr_establish_xname(pc, sc->sc_ihp[0],
	    sc->sc_ipl, virtio_intr, sc, device_xname(sc->sc_dev));
	if (sc->sc_ihs[0] == NULL) {
		aprint_error_dev(self, "couldn't establish INTx\n");
		return -1;
	}

	intrstr = pci_intr_string(pc, sc->sc_ihp[0], intrbuf, sizeof(intrbuf));
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	return 0;
}

static int
virtio_setup_interrupts(struct virtio_softc *sc)
{
	device_t self = sc->sc_dev;
	pci_chipset_tag_t pc = sc->sc_pa.pa_pc;
	int error;
	int nmsix;
	int counts[PCI_INTR_TYPE_SIZE];
	pci_intr_type_t max_type;

	nmsix = pci_msix_count(sc->sc_pa.pa_pc, sc->sc_pa.pa_tag);
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
	error = pci_intr_alloc(&sc->sc_pa, &sc->sc_ihp, counts, max_type);
	if (error != 0) {
		aprint_error_dev(self, "couldn't map interrupt\n");
		return -1;
	}

	if (pci_intr_type(pc, sc->sc_ihp[0]) == PCI_INTR_TYPE_MSIX) {
		sc->sc_ihs = kmem_alloc(sizeof(*sc->sc_ihs) * 2,
		    KM_SLEEP);

		error = virtio_setup_msix_interrupts(sc, &sc->sc_pa);
		if (error != 0) {
			kmem_free(sc->sc_ihs, sizeof(*sc->sc_ihs) * 2);
			pci_intr_release(pc, sc->sc_ihp, 2);

			/* Retry INTx */
			max_type = PCI_INTR_TYPE_INTX;
			counts[PCI_INTR_TYPE_INTX] = 1;
			goto retry;
		}

		sc->sc_ihs_num = 2;
		sc->sc_config_offset = VIRTIO_CONFIG_DEVICE_CONFIG_MSI;
	} else if (pci_intr_type(pc, sc->sc_ihp[0]) == PCI_INTR_TYPE_INTX) {
		sc->sc_ihs = kmem_alloc(sizeof(*sc->sc_ihs) * 1,
		    KM_SLEEP);

		error = virtio_setup_intx_interrupt(sc, &sc->sc_pa);
		if (error != 0) {
			kmem_free(sc->sc_ihs, sizeof(*sc->sc_ihs) * 1);
			pci_intr_release(pc, sc->sc_ihp, 1);
			return -1;
		}

		sc->sc_ihs_num = 1;
		sc->sc_config_offset = VIRTIO_CONFIG_DEVICE_CONFIG_NOMSI;
	}

	KASSERT(sc->sc_soft_ih == NULL);
	if (sc->sc_flags & VIRTIO_F_PCI_INTR_SOFTINT) {
		u_int flags = SOFTINT_NET;
		if (sc->sc_flags & VIRTIO_F_PCI_INTR_MPSAFE)
			flags |= SOFTINT_MPSAFE;

		sc->sc_soft_ih = softint_establish(flags, virtio_soft_intr, sc);
		if (sc->sc_soft_ih == NULL) {
			virtio_free_interrupts(sc);
			aprint_error_dev(sc->sc_dev,
			    "failed to establish soft interrupt\n");
			return -1;
		}
	}

	return 0;
}

static void
virtio_free_interrupts(struct virtio_softc *sc)
{
	for (int i = 0; i < sc->sc_ihs_num; i++) {
		if (sc->sc_ihs[i] == NULL)
			continue;
		pci_intr_disestablish(sc->sc_pc, sc->sc_ihs[i]);
		sc->sc_ihs[i] = NULL;
	}

	if (sc->sc_ihs_num > 0)
		pci_intr_release(sc->sc_pc, sc->sc_ihp, sc->sc_ihs_num);

	if (sc->sc_soft_ih) {
		softint_disestablish(sc->sc_soft_ih);
		sc->sc_soft_ih = NULL;
	}

	if (sc->sc_ihs != NULL) {
		kmem_free(sc->sc_ihs, sizeof(*sc->sc_ihs) * sc->sc_ihs_num);
		sc->sc_ihs = NULL;
	}
	sc->sc_ihs_num = 0;
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
		nbo_bus_space_write_2(sc->sc_iot, sc->sc_ioh,
				  VIRTIO_CONFIG_QUEUE_SELECT,
				  vq->vq_index);
		n = nbo_bus_space_read_2(sc->sc_iot, sc->sc_ioh,
				     VIRTIO_CONFIG_QUEUE_SIZE);
		if (n == 0)	/* vq disappeared */
			continue;
		if (n != vq->vq_num) {
			panic("%s: virtqueue size changed, vq index %d\n",
			      device_xname(sc->sc_dev),
			      vq->vq_index);
		}
		virtio_init_vq(sc, vq, true);
		nbo_bus_space_write_4(sc->sc_iot, sc->sc_ioh,
				  VIRTIO_CONFIG_QUEUE_ADDRESS,
				  (vq->vq_dmamap->dm_segs[0].ds_addr
				   / VIRTIO_PAGE_SIZE));
	}

	/* MSI-X should have more than one handles where INTx has just one */
	if (sc->sc_ihs_num > 1) {
		if (virtio_setup_msix_vectors(sc) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "couldn't setup MSI-X vectors\n");
			return;
		}
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
	r = nbo_bus_space_read_4(sc->sc_iot, sc->sc_ioh,
			     VIRTIO_CONFIG_DEVICE_FEATURES);
	r &= guest_features;
	nbo_bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  VIRTIO_CONFIG_GUEST_FEATURES, r);
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
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh,
				sc->sc_config_offset + index);
}

uint16_t
virtio_read_device_config_2(struct virtio_softc *sc, int index)
{
	return nbo_bus_space_read_2(sc->sc_iot, sc->sc_ioh,
				sc->sc_config_offset + index);
}

uint32_t
virtio_read_device_config_4(struct virtio_softc *sc, int index)
{
	return nbo_bus_space_read_4(sc->sc_iot, sc->sc_ioh,
				sc->sc_config_offset + index);
}

uint64_t
virtio_read_device_config_8(struct virtio_softc *sc, int index)
{
	uint64_t r;

	r = nbo_bus_space_read_4(sc->sc_iot, sc->sc_ioh,
			     sc->sc_config_offset + index + REG_HI_OFF);
	r <<= 32;
	r |= nbo_bus_space_read_4(sc->sc_iot, sc->sc_ioh,
			      sc->sc_config_offset + index + REG_LO_OFF);
	
	return r;
}

void
virtio_write_device_config_1(struct virtio_softc *sc,
			     int index, uint8_t value)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			  sc->sc_config_offset + index, value);
}

void
virtio_write_device_config_2(struct virtio_softc *sc,
			     int index, uint16_t value)
{
	nbo_bus_space_write_2(sc->sc_iot, sc->sc_ioh,
			  sc->sc_config_offset + index, value);
}

void
virtio_write_device_config_4(struct virtio_softc *sc,
			     int index, uint32_t value)
{
	nbo_bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  sc->sc_config_offset + index, value);
}

void
virtio_write_device_config_8(struct virtio_softc *sc,
			     int index, uint64_t value)
{
	nbo_bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  sc->sc_config_offset + index + REG_LO_OFF,
			  value & 0xffffffff);
	nbo_bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  sc->sc_config_offset + index + REG_HI_OFF,
			  value >> 32);
}

/*
 * Interrupt handler.
 */
static int
virtio_intr(void *arg)
{
	struct virtio_softc *sc = arg;
	int isr, r = 0;

	/* check and ack the interrupt */
	isr = bus_space_read_1(sc->sc_iot, sc->sc_ioh,
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
virtio_msix_queue_intr(void *arg)
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
virtio_msix_config_intr(void *arg)
{
	struct virtio_softc *sc = arg;
	int r = 0;

	if (sc->sc_config_change != NULL)
		r = (sc->sc_config_change)(sc);
	return r;
}

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

	nbo_bus_space_write_2(sc->sc_iot, sc->sc_ioh,
			  VIRTIO_CONFIG_QUEUE_SELECT, index);
	vq_size = nbo_bus_space_read_2(sc->sc_iot, sc->sc_ioh,
				   VIRTIO_CONFIG_QUEUE_SIZE);
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
	nbo_bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  VIRTIO_CONFIG_QUEUE_ADDRESS,
			  (vq->vq_dmamap->dm_segs[0].ds_addr
			   / VIRTIO_PAGE_SIZE));

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
	nbo_bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  VIRTIO_CONFIG_QUEUE_ADDRESS, 0);
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
	nbo_bus_space_write_2(sc->sc_iot, sc->sc_ioh,
			  VIRTIO_CONFIG_QUEUE_SELECT, vq->vq_index);
	nbo_bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  VIRTIO_CONFIG_QUEUE_ADDRESS, 0);

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
			nbo_bus_space_write_2(sc->sc_iot, sc->sc_ioh,
					  VIRTIO_CONFIG_QUEUE_NOTIFY,
					  vq->vq_index);
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

	r = virtio_setup_interrupts(sc);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev, "failed to setup interrupts\n");
		virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_FAILED);
		return 1;
	}

	virtio_set_status(sc, VIRTIO_CONFIG_DEVICE_STATUS_DRIVER_OK);

	return 0;
}

void
virtio_child_detach(struct virtio_softc *sc)
{
	sc->sc_child = NULL;
	sc->sc_vqs = NULL;

	virtio_device_reset(sc);

	virtio_free_interrupts(sc);
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

MODULE(MODULE_CLASS_DRIVER, virtio, "pci");
 
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
