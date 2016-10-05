/*	$NetBSD: ld_virtio.c,v 1.6.4.4 2016/10/05 20:55:43 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ld_virtio.c,v 1.6.4.4 2016/10/05 20:55:43 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/mutex.h>
#include <sys/module.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/ldvar.h>
#include <dev/pci/virtioreg.h>
#include <dev/pci/virtiovar.h>

#include "ioconf.h"

/*
 * ld_virtioreg:
 */
/* Configuration registers */
#define VIRTIO_BLK_CONFIG_CAPACITY	0 /* 64bit */
#define VIRTIO_BLK_CONFIG_SIZE_MAX	8 /* 32bit */
#define VIRTIO_BLK_CONFIG_SEG_MAX	12 /* 32bit */
#define VIRTIO_BLK_CONFIG_GEOMETRY_C	16 /* 16bit */
#define VIRTIO_BLK_CONFIG_GEOMETRY_H	18 /* 8bit */
#define VIRTIO_BLK_CONFIG_GEOMETRY_S	19 /* 8bit */
#define VIRTIO_BLK_CONFIG_BLK_SIZE	20 /* 32bit */

/* Feature bits */
#define VIRTIO_BLK_F_BARRIER	(1<<0)
#define VIRTIO_BLK_F_SIZE_MAX	(1<<1)
#define VIRTIO_BLK_F_SEG_MAX	(1<<2)
#define VIRTIO_BLK_F_GEOMETRY	(1<<4)
#define VIRTIO_BLK_F_RO		(1<<5)
#define VIRTIO_BLK_F_BLK_SIZE	(1<<6)
#define VIRTIO_BLK_F_SCSI	(1<<7)
#define VIRTIO_BLK_F_FLUSH	(1<<9)

/*
 * Each block request uses at least two segments - one for the header
 * and one for the status.
*/
#define	VIRTIO_BLK_MIN_SEGMENTS	2

#define VIRTIO_BLK_FLAG_BITS \
	VIRTIO_COMMON_FLAG_BITS \
	"\x0a""FLUSH" \
	"\x08""SCSI" \
	"\x07""BLK_SIZE" \
	"\x06""RO" \
	"\x05""GEOMETRY" \
	"\x03""SEG_MAX" \
	"\x02""SIZE_MAX" \
	"\x01""BARRIER"

/* Command */
#define VIRTIO_BLK_T_IN		0
#define VIRTIO_BLK_T_OUT	1
#define VIRTIO_BLK_T_BARRIER	0x80000000

/* Status */
#define VIRTIO_BLK_S_OK		0
#define VIRTIO_BLK_S_IOERR	1

/* Request header structure */
struct virtio_blk_req_hdr {
	uint32_t	type;	/* VIRTIO_BLK_T_* */
	uint32_t	ioprio;
	uint64_t	sector;
} __packed;
/* 512*virtio_blk_req_hdr.sector byte payload and 1 byte status follows */


/*
 * ld_virtiovar:
 */
struct virtio_blk_req {
	struct virtio_blk_req_hdr	vr_hdr;
	uint8_t				vr_status;
	struct buf			*vr_bp;
	bus_dmamap_t			vr_cmdsts;
	bus_dmamap_t			vr_payload;
};

struct ld_virtio_softc {
	struct ld_softc		sc_ld;
	device_t		sc_dev;

	struct virtio_softc	*sc_virtio;
	struct virtqueue	sc_vq;

	struct virtio_blk_req	*sc_reqs;
	bus_dma_segment_t	sc_reqs_seg;

	int			sc_readonly;
};

static int	ld_virtio_match(device_t, cfdata_t, void *);
static void	ld_virtio_attach(device_t, device_t, void *);
static int	ld_virtio_detach(device_t, int);

CFATTACH_DECL_NEW(ld_virtio, sizeof(struct ld_virtio_softc),
    ld_virtio_match, ld_virtio_attach, ld_virtio_detach, NULL);

static int
ld_virtio_match(device_t parent, cfdata_t match, void *aux)
{
	struct virtio_softc *va = aux;

	if (va->sc_childdevid == PCI_PRODUCT_VIRTIO_BLOCK)
		return 1;

	return 0;
}

static int ld_virtio_vq_done(struct virtqueue *);
static int ld_virtio_dump(struct ld_softc *, void *, int, int);
static int ld_virtio_start(struct ld_softc *, struct buf *);

static int
ld_virtio_alloc_reqs(struct ld_virtio_softc *sc, int qsize)
{
	int allocsize, r, rsegs, i;
	struct ld_softc *ld = &sc->sc_ld;
	void *vaddr;

	allocsize = sizeof(struct virtio_blk_req) * qsize;
	r = bus_dmamem_alloc(sc->sc_virtio->sc_dmat, allocsize, 0, 0,
			     &sc->sc_reqs_seg, 1, &rsegs, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
				 "DMA memory allocation failed, size %d, "
				 "error code %d\n", allocsize, r);
		goto err_none;
	}
	r = bus_dmamem_map(sc->sc_virtio->sc_dmat,
			   &sc->sc_reqs_seg, 1, allocsize,
			   &vaddr, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
				 "DMA memory map failed, "
				 "error code %d\n", r);
		goto err_dmamem_alloc;
	}
	sc->sc_reqs = vaddr;
	memset(vaddr, 0, allocsize);
	for (i = 0; i < qsize; i++) {
		struct virtio_blk_req *vr = &sc->sc_reqs[i];
		r = bus_dmamap_create(sc->sc_virtio->sc_dmat,
				      offsetof(struct virtio_blk_req, vr_bp),
				      1,
				      offsetof(struct virtio_blk_req, vr_bp),
				      0,
				      BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW,
				      &vr->vr_cmdsts);
		if (r != 0) {
			aprint_error_dev(sc->sc_dev,
					 "command dmamap creation failed, "
					 "error code %d\n", r);
			goto err_reqs;
		}
		r = bus_dmamap_load(sc->sc_virtio->sc_dmat, vr->vr_cmdsts,
				    &vr->vr_hdr,
				    offsetof(struct virtio_blk_req, vr_bp),
				    NULL, BUS_DMA_NOWAIT);
		if (r != 0) {
			aprint_error_dev(sc->sc_dev,
					 "command dmamap load failed, "
					 "error code %d\n", r);
			goto err_reqs;
		}
		r = bus_dmamap_create(sc->sc_virtio->sc_dmat,
				      ld->sc_maxxfer,
				      (ld->sc_maxxfer / NBPG) +
				      VIRTIO_BLK_MIN_SEGMENTS,
				      ld->sc_maxxfer,
				      0,
				      BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW,
				      &vr->vr_payload);
		if (r != 0) {
			aprint_error_dev(sc->sc_dev,
					 "payload dmamap creation failed, "
					 "error code %d\n", r);
			goto err_reqs;
		}
	}
	return 0;

err_reqs:
	for (i = 0; i < qsize; i++) {
		struct virtio_blk_req *vr = &sc->sc_reqs[i];
		if (vr->vr_cmdsts) {
			bus_dmamap_destroy(sc->sc_virtio->sc_dmat,
					   vr->vr_cmdsts);
			vr->vr_cmdsts = 0;
		}
		if (vr->vr_payload) {
			bus_dmamap_destroy(sc->sc_virtio->sc_dmat,
					   vr->vr_payload);
			vr->vr_payload = 0;
		}
	}
	bus_dmamem_unmap(sc->sc_virtio->sc_dmat, sc->sc_reqs, allocsize);
err_dmamem_alloc:
	bus_dmamem_free(sc->sc_virtio->sc_dmat, &sc->sc_reqs_seg, 1);
err_none:
	return -1;
}

static void
ld_virtio_attach(device_t parent, device_t self, void *aux)
{
	struct ld_virtio_softc *sc = device_private(self);
	struct ld_softc *ld = &sc->sc_ld;
	struct virtio_softc *vsc = device_private(parent);
	uint32_t features;
	char buf[256];
	int qsize, maxxfersize, maxnsegs;

	if (vsc->sc_child != NULL) {
		aprint_normal(": child already attached for %s; "
			      "something wrong...\n", device_xname(parent));
		return;
	}

	sc->sc_dev = self;
	sc->sc_virtio = vsc;

	vsc->sc_child = self;
	vsc->sc_ipl = IPL_BIO;
	vsc->sc_vqs = &sc->sc_vq;
	vsc->sc_nvqs = 1;
	vsc->sc_config_change = NULL;
	vsc->sc_intrhand = virtio_vq_intr;
	vsc->sc_flags = 0;

	features = virtio_negotiate_features(vsc,
					     (VIRTIO_BLK_F_SIZE_MAX |
					      VIRTIO_BLK_F_SEG_MAX |
					      VIRTIO_BLK_F_GEOMETRY |
					      VIRTIO_BLK_F_RO |
					      VIRTIO_BLK_F_BLK_SIZE));
	if (features & VIRTIO_BLK_F_RO)
		sc->sc_readonly = 1;
	else
		sc->sc_readonly = 0;

	snprintb(buf, sizeof(buf), VIRTIO_BLK_FLAG_BITS, features);
	aprint_normal(": Features: %s\n", buf);
	aprint_naive("\n");
	if (features & VIRTIO_BLK_F_BLK_SIZE) {
		ld->sc_secsize = virtio_read_device_config_4(vsc,
					VIRTIO_BLK_CONFIG_BLK_SIZE);
	} else
		ld->sc_secsize = 512;

	/* At least genfs_io assumes maxxfer == MAXPHYS. */
	if (features & VIRTIO_BLK_F_SIZE_MAX) {
		maxxfersize = virtio_read_device_config_4(vsc,
		    VIRTIO_BLK_CONFIG_SIZE_MAX);
		if (maxxfersize < MAXPHYS) {
			aprint_error_dev(sc->sc_dev,
			    "Too small SIZE_MAX %dK minimum is %dK\n",
			    maxxfersize / 1024, MAXPHYS / 1024);
			// goto err;
			maxxfersize = MAXPHYS;
		} else if (maxxfersize > MAXPHYS) {
			aprint_normal_dev(sc->sc_dev,
			    "Clip SEG_MAX from %dK to %dK\n",
			    maxxfersize / 1024,
			    MAXPHYS / 1024);
			maxxfersize = MAXPHYS;
		}
	} else
		maxxfersize = MAXPHYS;

	if (features & VIRTIO_BLK_F_SEG_MAX) {
		maxnsegs = virtio_read_device_config_4(vsc,
		    VIRTIO_BLK_CONFIG_SEG_MAX);
		if (maxnsegs < VIRTIO_BLK_MIN_SEGMENTS) {
			aprint_error_dev(sc->sc_dev,
			    "Too small SEG_MAX %d minimum is %d\n",
			    maxnsegs, VIRTIO_BLK_MIN_SEGMENTS);
			maxnsegs = maxxfersize / NBPG;
			// goto err;
		}
	} else
		maxnsegs = maxxfersize / NBPG;

	/* 2 for the minimum size */
	maxnsegs += VIRTIO_BLK_MIN_SEGMENTS;

	if (virtio_alloc_vq(vsc, &sc->sc_vq, 0, maxxfersize, maxnsegs,
	    "I/O request") != 0) {
		goto err;
	}
	qsize = sc->sc_vq.vq_num;
	sc->sc_vq.vq_done = ld_virtio_vq_done;

	ld->sc_dv = self;
	ld->sc_secperunit = virtio_read_device_config_8(vsc,
				VIRTIO_BLK_CONFIG_CAPACITY);
	ld->sc_maxxfer = maxxfersize;
	if (features & VIRTIO_BLK_F_GEOMETRY) {
		ld->sc_ncylinders = virtio_read_device_config_2(vsc,
					VIRTIO_BLK_CONFIG_GEOMETRY_C);
		ld->sc_nheads     = virtio_read_device_config_1(vsc,
					VIRTIO_BLK_CONFIG_GEOMETRY_H);
		ld->sc_nsectors   = virtio_read_device_config_1(vsc,
					VIRTIO_BLK_CONFIG_GEOMETRY_S);
	}
	ld->sc_maxqueuecnt = qsize;

	if (ld_virtio_alloc_reqs(sc, qsize) < 0)
		goto err;

	ld->sc_dump = ld_virtio_dump;
	ld->sc_flush = NULL;
	ld->sc_start = ld_virtio_start;

	ld->sc_flags = LDF_ENABLED;
	ldattach(ld, BUFQ_DISK_DEFAULT_STRAT);

	return;

err:
	vsc->sc_child = (void*)1;
	return;
}

static int
ld_virtio_start(struct ld_softc *ld, struct buf *bp)
{
	/* splbio */
	struct ld_virtio_softc *sc = device_private(ld->sc_dv);
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq;
	struct virtio_blk_req *vr;
	int r;
	int isread = (bp->b_flags & B_READ);
	int slot;

	if (sc->sc_readonly && !isread)
		return EIO;

	r = virtio_enqueue_prep(vsc, vq, &slot);
	if (r != 0)
		return r;

	vr = &sc->sc_reqs[slot];
	KASSERT(vr->vr_bp == NULL);

	r = bus_dmamap_load(vsc->sc_dmat, vr->vr_payload,
			    bp->b_data, bp->b_bcount, NULL,
			    ((isread?BUS_DMA_READ:BUS_DMA_WRITE)
			     |BUS_DMA_NOWAIT));
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
		    "payload dmamap failed, error code %d\n", r);
		virtio_enqueue_abort(vsc, vq, slot);
		return r;
	}

	r = virtio_enqueue_reserve(vsc, vq, slot, vr->vr_payload->dm_nsegs +
	    VIRTIO_BLK_MIN_SEGMENTS);
	if (r != 0) {
		virtio_enqueue_abort(vsc, vq, slot);
		bus_dmamap_unload(vsc->sc_dmat, vr->vr_payload);
		return r;
	}

	vr->vr_bp = bp;
	vr->vr_hdr.type = isread?VIRTIO_BLK_T_IN:VIRTIO_BLK_T_OUT;
	vr->vr_hdr.ioprio = 0;
	vr->vr_hdr.sector = bp->b_rawblkno * sc->sc_ld.sc_secsize / 512;

	bus_dmamap_sync(vsc->sc_dmat, vr->vr_cmdsts,
			0, sizeof(struct virtio_blk_req_hdr),
			BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(vsc->sc_dmat, vr->vr_payload,
			0, bp->b_bcount,
			isread?BUS_DMASYNC_PREREAD:BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(vsc->sc_dmat, vr->vr_cmdsts,
			offsetof(struct virtio_blk_req, vr_status),
			sizeof(uint8_t),
			BUS_DMASYNC_PREREAD);

	virtio_enqueue_p(vsc, vq, slot, vr->vr_cmdsts,
			 0, sizeof(struct virtio_blk_req_hdr),
			 true);
	virtio_enqueue(vsc, vq, slot, vr->vr_payload, !isread);
	virtio_enqueue_p(vsc, vq, slot, vr->vr_cmdsts,
			 offsetof(struct virtio_blk_req, vr_status),
			 sizeof(uint8_t),
			 false);
	virtio_enqueue_commit(vsc, vq, slot, true);

	return 0;
}

static void
ld_virtio_vq_done1(struct ld_virtio_softc *sc, struct virtio_softc *vsc,
		   struct virtqueue *vq, int slot)
{
	struct virtio_blk_req *vr = &sc->sc_reqs[slot];
	struct buf *bp = vr->vr_bp;

	vr->vr_bp = NULL;

	bus_dmamap_sync(vsc->sc_dmat, vr->vr_cmdsts,
			0, sizeof(struct virtio_blk_req_hdr),
			BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(vsc->sc_dmat, vr->vr_payload,
			0, bp->b_bcount,
			(bp->b_flags & B_READ)?BUS_DMASYNC_POSTREAD
					      :BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(vsc->sc_dmat, vr->vr_cmdsts,
			sizeof(struct virtio_blk_req_hdr), sizeof(uint8_t),
			BUS_DMASYNC_POSTREAD);

	if (vr->vr_status != VIRTIO_BLK_S_OK) {
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
	} else {
		bp->b_error = 0;
		bp->b_resid = 0;
	}

	virtio_dequeue_commit(vsc, vq, slot);

	lddone(&sc->sc_ld, bp);
}

static int
ld_virtio_vq_done(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct ld_virtio_softc *sc = device_private(vsc->sc_child);
	int r = 0;
	int slot;

again:
	if (virtio_dequeue(vsc, vq, &slot, NULL))
		return r;
	r = 1;

	ld_virtio_vq_done1(sc, vsc, vq, slot);
	goto again;
}

static int
ld_virtio_dump(struct ld_softc *ld, void *data, int blkno, int blkcnt)
{
	struct ld_virtio_softc *sc = device_private(ld->sc_dv);
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq;
	struct virtio_blk_req *vr;
	int slot, r;

	if (sc->sc_readonly)
		return EIO;

	r = virtio_enqueue_prep(vsc, vq, &slot);
	if (r != 0) {
		if (r == EAGAIN) { /* no free slot; dequeue first */
			delay(100);
			ld_virtio_vq_done(vq);
			r = virtio_enqueue_prep(vsc, vq, &slot);
			if (r != 0)
				return r;
		}
		return r;
	}
	vr = &sc->sc_reqs[slot];
	r = bus_dmamap_load(vsc->sc_dmat, vr->vr_payload,
			    data, blkcnt*ld->sc_secsize, NULL,
			    BUS_DMA_WRITE|BUS_DMA_NOWAIT);
	if (r != 0)
		return r;

	r = virtio_enqueue_reserve(vsc, vq, slot, vr->vr_payload->dm_nsegs +
	    VIRTIO_BLK_MIN_SEGMENTS);
	if (r != 0) {
		bus_dmamap_unload(vsc->sc_dmat, vr->vr_payload);
		return r;
	}

	vr->vr_bp = (void*)0xdeadbeef;
	vr->vr_hdr.type = VIRTIO_BLK_T_OUT;
	vr->vr_hdr.ioprio = 0;
	vr->vr_hdr.sector = (daddr_t) blkno * ld->sc_secsize / 512;

	bus_dmamap_sync(vsc->sc_dmat, vr->vr_cmdsts,
			0, sizeof(struct virtio_blk_req_hdr),
			BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(vsc->sc_dmat, vr->vr_payload,
			0, blkcnt*ld->sc_secsize,
			BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(vsc->sc_dmat, vr->vr_cmdsts,
			offsetof(struct virtio_blk_req, vr_status),
			sizeof(uint8_t),
			BUS_DMASYNC_PREREAD);

	virtio_enqueue_p(vsc, vq, slot, vr->vr_cmdsts,
			 0, sizeof(struct virtio_blk_req_hdr),
			 true);
	virtio_enqueue(vsc, vq, slot, vr->vr_payload, true);
	virtio_enqueue_p(vsc, vq, slot, vr->vr_cmdsts,
			 offsetof(struct virtio_blk_req, vr_status),
			 sizeof(uint8_t),
			 false);
	virtio_enqueue_commit(vsc, vq, slot, true);

	for ( ; ; ) {
		int dslot;

		r = virtio_dequeue(vsc, vq, &dslot, NULL);
		if (r != 0)
			continue;
		if (dslot != slot) {
			ld_virtio_vq_done1(sc, vsc, vq, dslot);
			continue;
		} else
			break;
	}

	bus_dmamap_sync(vsc->sc_dmat, vr->vr_cmdsts,
			0, sizeof(struct virtio_blk_req_hdr),
			BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(vsc->sc_dmat, vr->vr_payload,
			0, blkcnt*ld->sc_secsize,
			BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(vsc->sc_dmat, vr->vr_cmdsts,
			offsetof(struct virtio_blk_req, vr_status),
			sizeof(uint8_t),
			BUS_DMASYNC_POSTREAD);
	if (vr->vr_status == VIRTIO_BLK_S_OK)
		r = 0;
	else
		r = EIO;
	virtio_dequeue_commit(vsc, vq, slot);

	return r;
}

static int
ld_virtio_detach(device_t self, int flags)
{
	struct ld_virtio_softc *sc = device_private(self);
	struct ld_softc *ld = &sc->sc_ld;
	bus_dma_tag_t dmat = sc->sc_virtio->sc_dmat;
	int r, i, qsize;

	qsize = sc->sc_vq.vq_num;
	r = ldbegindetach(ld, flags);
	if (r != 0)
		return r;
	virtio_reset(sc->sc_virtio);
	virtio_free_vq(sc->sc_virtio, &sc->sc_vq);

	for (i = 0; i < qsize; i++) {
		bus_dmamap_destroy(dmat,
				   sc->sc_reqs[i].vr_cmdsts);
		bus_dmamap_destroy(dmat,
				   sc->sc_reqs[i].vr_payload);
	}
	bus_dmamem_unmap(dmat, sc->sc_reqs,
			 sizeof(struct virtio_blk_req) * qsize);
	bus_dmamem_free(dmat, &sc->sc_reqs_seg, 1);

	ldenddetach(ld);

	return 0;
}

MODULE(MODULE_CLASS_DRIVER, ld_virtio, "ld,virtio");

#ifdef _MODULE
/*
 * XXX Don't allow ioconf.c to redefine the "struct cfdriver ld_cd"
 * XXX it will be defined in the common-code module
 */
#undef  CFDRIVER_DECL
#define CFDRIVER_DECL(name, class, attr)
#include "ioconf.c"
#endif

static int
ld_virtio_modcmd(modcmd_t cmd, void *opaque)
{
#ifdef _MODULE
	/*
	 * We ignore the cfdriver_vec[] that ioconf provides, since
	 * the cfdrivers are attached already.
	 */
	static struct cfdriver * const no_cfdriver_vec[] = { NULL };
#endif
	int error = 0;

#ifdef _MODULE
	switch (cmd) {
	case MODULE_CMD_INIT:
		error = config_init_component(no_cfdriver_vec,
		    cfattach_ioconf_ld_virtio, cfdata_ioconf_ld_virtio);
		break;
	case MODULE_CMD_FINI:
		error = config_fini_component(no_cfdriver_vec,
		    cfattach_ioconf_ld_virtio, cfdata_ioconf_ld_virtio);
		break;
	default:
		error = ENOTTY;
		break;
	}
#endif

	return error;
}
