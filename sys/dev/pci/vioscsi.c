/*	$NetBSD: vioscsi.c,v 1.29 2022/01/27 18:38:07 jakllsch Exp $	*/
/*	$OpenBSD: vioscsi.c,v 1.3 2015/03/14 03:38:49 jsg Exp $	*/

/*
 * Copyright (c) 2013 Google Inc.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vioscsi.c,v 1.29 2022/01/27 18:38:07 jakllsch Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/buf.h>
#include <sys/module.h>

#include <dev/pci/vioscsireg.h>
#include <dev/pci/virtiovar.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsiconf.h>

#ifdef VIOSCSI_DEBUG
static int vioscsi_debug = 1;
#define DPRINTF(f) do { if (vioscsi_debug) printf f; } while (/*CONSTCOND*/0)
#else
#define DPRINTF(f) ((void)0)
#endif

struct vioscsi_req {
	struct virtio_scsi_req_hdr	 vr_req;
	struct virtio_scsi_res_hdr	 vr_res;
	struct scsipi_xfer		*vr_xs;
	bus_dmamap_t			 vr_control;
	bus_dmamap_t			 vr_data;
};

struct vioscsi_softc {
	device_t 		 sc_dev;
	struct scsipi_adapter	 sc_adapter;
	struct scsipi_channel 	 sc_channel;

	struct virtqueue	 sc_vqs[3];
#define VIOSCSI_VQ_CONTROL	0
#define VIOSCSI_VQ_EVENT	1
#define VIOSCSI_VQ_REQUEST	2

	struct vioscsi_req	*sc_reqs;
	int			 sc_nreqs;
	bus_dma_segment_t        sc_reqs_segs[1];

	u_int32_t		 sc_seg_max;

	kmutex_t		 sc_mutex;
};

/*
 * Each block request uses at least two segments - one for the header
 * and one for the status.
*/
#define VIRTIO_SCSI_MIN_SEGMENTS 2

static int	 vioscsi_match(device_t, cfdata_t, void *);
static void	 vioscsi_attach(device_t, device_t, void *);
static int	 vioscsi_detach(device_t, int);

static int	 vioscsi_alloc_reqs(struct vioscsi_softc *,
    struct virtio_softc *, int);
static void	 vioscsi_free_reqs(struct vioscsi_softc *,
    struct virtio_softc *);
static void	 vioscsi_scsipi_request(struct scsipi_channel *,
    scsipi_adapter_req_t, void *);
static int	 vioscsi_vq_done(struct virtqueue *);
static void	 vioscsi_req_done(struct vioscsi_softc *, struct virtio_softc *,
    struct vioscsi_req *, struct virtqueue *, int);
static struct vioscsi_req *vioscsi_req_get(struct vioscsi_softc *);
static void	 vioscsi_bad_target(struct scsipi_xfer *);

static const char *const vioscsi_vq_names[] = {
	"control",
	"event",
	"request",
};

CFATTACH_DECL3_NEW(vioscsi, sizeof(struct vioscsi_softc),
    vioscsi_match, vioscsi_attach, vioscsi_detach, NULL, NULL, NULL,
    DVF_DETACH_SHUTDOWN);

static int
vioscsi_match(device_t parent, cfdata_t match, void *aux)
{
	struct virtio_attach_args *va = aux;

	if (va->sc_childdevid == VIRTIO_DEVICE_ID_SCSI)
		return 1;

	return 0;
}

static void
vioscsi_attach(device_t parent, device_t self, void *aux)
{
	struct vioscsi_softc *sc = device_private(self);
	struct virtio_softc *vsc = device_private(parent);
	struct scsipi_adapter *adapt = &sc->sc_adapter;
	struct scsipi_channel *chan = &sc->sc_channel;
	int rv, qsize = 0, i = 0;
	int ipl = IPL_BIO;

	if (virtio_child(vsc) != NULL) {
		aprint_error(": parent %s already has a child\n",
		    device_xname(parent));
		return;
	}

	sc->sc_dev = self;

	virtio_child_attach_start(vsc, self, ipl, sc->sc_vqs,
	    NULL, virtio_vq_intr, VIRTIO_F_INTR_MSIX,
	    0, VIRTIO_COMMON_FLAG_BITS);

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, ipl);

	uint32_t cmd_per_lun = virtio_read_device_config_4(vsc,
	    VIRTIO_SCSI_CONFIG_CMD_PER_LUN);

	uint32_t seg_max = virtio_read_device_config_4(vsc,
	    VIRTIO_SCSI_CONFIG_SEG_MAX);

	uint16_t max_target = virtio_read_device_config_2(vsc,
	    VIRTIO_SCSI_CONFIG_MAX_TARGET);

	uint32_t max_lun = virtio_read_device_config_4(vsc,
	    VIRTIO_SCSI_CONFIG_MAX_LUN);

	sc->sc_seg_max = seg_max;

	for(i=0; i < __arraycount(sc->sc_vqs); i++) {
		rv = virtio_alloc_vq(vsc, &sc->sc_vqs[i], i, MAXPHYS,
		    VIRTIO_SCSI_MIN_SEGMENTS + howmany(MAXPHYS, NBPG),
		    vioscsi_vq_names[i]);
		if (rv) {
			aprint_error_dev(sc->sc_dev,
			    "failed to allocate virtqueue %d\n", i);
			goto err;
		}

		if (i == VIOSCSI_VQ_REQUEST)
			sc->sc_vqs[i].vq_done = vioscsi_vq_done;
	}

	qsize = sc->sc_vqs[VIOSCSI_VQ_REQUEST].vq_num;
	if (vioscsi_alloc_reqs(sc, vsc, qsize))
		goto err;

	aprint_normal_dev(sc->sc_dev,
	    "cmd_per_lun %u qsize %d seg_max %u max_target %hu"
	    " max_lun %u\n",
	    cmd_per_lun, qsize, seg_max, max_target, max_lun);

	if (virtio_child_attach_finish(vsc) != 0)
		goto err;

	/*
	 * Fill in the scsipi_adapter.
	 */
	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_dev = sc->sc_dev;
	adapt->adapt_nchannels = 1;
	adapt->adapt_openings = MIN(qsize, cmd_per_lun);
	adapt->adapt_max_periph = adapt->adapt_openings;
	adapt->adapt_request = vioscsi_scsipi_request;
	adapt->adapt_minphys = minphys;

	/*
	 * Fill in the scsipi_channel.
	 */
	memset(chan, 0, sizeof(*chan));
	chan->chan_adapter = adapt;
	chan->chan_bustype = &scsi_bustype;
	chan->chan_channel = 0;
	chan->chan_ntargets = MIN(1 + max_target, 256);	/* cap reasonably */
	chan->chan_nluns = MIN(1 + max_lun, 16384);	/* cap reasonably */
	chan->chan_id = max_target + 1;
	chan->chan_flags = SCSIPI_CHAN_NOSETTLE;

	config_found(self, &sc->sc_channel, scsiprint, CFARGS_NONE);
	return;

err:
	if (qsize > 0)
		vioscsi_free_reqs(sc, vsc);

	for (i=0; i < __arraycount(sc->sc_vqs); i++) {
		if (sc->sc_vqs[i].vq_num > 0)
			virtio_free_vq(vsc, &sc->sc_vqs[i]);
	}

	virtio_child_attach_failed(vsc);
}

static int
vioscsi_detach(device_t self, int flags)
{
	struct vioscsi_softc *sc = device_private(self);
	struct virtio_softc *vsc = device_private(device_parent(sc->sc_dev));
	int rc, i;

	/*
	 * Dequeue all pending finished requests. Must be done
	 * before we try to detach children so that we process
	 * their pending requests while they still exist.
	 */
	if (sc->sc_vqs[VIOSCSI_VQ_REQUEST].vq_num > 0)
		vioscsi_vq_done(&sc->sc_vqs[VIOSCSI_VQ_REQUEST]);

	if ((rc = config_detach_children(self, flags)) != 0)
		return rc;

	virtio_reset(vsc);

	for (i = 0; i < __arraycount(sc->sc_vqs); i++) {
		if (sc->sc_vqs[i].vq_num > 0)
			virtio_free_vq(vsc, &sc->sc_vqs[i]);
	}

	vioscsi_free_reqs(sc, vsc);

	virtio_child_detach(vsc);

	mutex_destroy(&sc->sc_mutex);

	return 0;
}

#define XS2DMA(xs) \
    ((((xs)->xs_control & XS_CTL_DATA_IN) ? BUS_DMA_READ : BUS_DMA_WRITE) | \
    (((xs)->xs_control & XS_CTL_NOSLEEP) ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK) | \
    BUS_DMA_STREAMING)

#define XS2DMAPRE(xs) (((xs)->xs_control & XS_CTL_DATA_IN) ? \
    BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE)

#define XS2DMAPOST(xs) (((xs)->xs_control & XS_CTL_DATA_IN) ? \
    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE)

static void
vioscsi_scsipi_request(struct scsipi_channel *chan, scsipi_adapter_req_t
    request, void *arg)
{
	struct vioscsi_softc *sc =
	    device_private(chan->chan_adapter->adapt_dev);
	struct virtio_softc *vsc = device_private(device_parent(sc->sc_dev));
	struct scsipi_xfer *xs;
	struct scsipi_periph *periph;
	struct vioscsi_req *vr;
	struct virtio_scsi_req_hdr *req;
	struct virtqueue *vq = &sc->sc_vqs[VIOSCSI_VQ_REQUEST];
	int slot, error;
	bool dopoll;

	DPRINTF(("%s: enter\n", __func__));

	switch (request) {
	case ADAPTER_REQ_RUN_XFER:
		break;
	case ADAPTER_REQ_SET_XFER_MODE:
	{
		struct scsipi_xfer_mode *xm = arg;
		xm->xm_mode = PERIPH_CAP_TQING;
		xm->xm_period = 0;
		xm->xm_offset = 0;
		scsipi_async_event(chan, ASYNC_EVENT_XFER_MODE, xm);
		return;
	}
	default:
		DPRINTF(("%s: unhandled %d\n", __func__, request));
		return;
	}

	xs = arg;
	periph = xs->xs_periph;

	/*
	 * This can happen when we run out of queue slots.
	 */
	vr = vioscsi_req_get(sc);
	if (vr == NULL) {
		xs->error = XS_RESOURCE_SHORTAGE;
		scsipi_done(xs);
		return;
	}

	req = &vr->vr_req;
	slot = vr - sc->sc_reqs;

	/*
	 * "The only supported format for the LUN field is: first byte set to
	 * 1, second byte set to target, third and fourth byte representing a
	 * single level LUN structure, followed by four zero bytes."
	 */
	if (periph->periph_target >= 256 || periph->periph_lun >= 16384
	    || periph->periph_target < 0 || periph->periph_lun < 0) {
		goto stuffup;
	}

	req->lun[0] = 1;
	req->lun[1] = periph->periph_target;
	req->lun[2] = 0x40 | ((periph->periph_lun >> 8) & 0x3F);
	req->lun[3] = periph->periph_lun & 0xFF;
	memset(req->lun + 4, 0, 4);
	DPRINTF(("%s: command %p for %d:%d at slot %d\n", __func__,
	    xs, periph->periph_target, periph->periph_lun, slot));

	/* tag */
	switch (XS_CTL_TAGTYPE(xs)) {
	case XS_CTL_HEAD_TAG:
		req->task_attr = VIRTIO_SCSI_S_HEAD;
		break;

#if 0	/* XXX */
	case XS_CTL_ACA_TAG:
		req->task_attr = VIRTIO_SCSI_S_ACA;
		break;
#endif

	case XS_CTL_ORDERED_TAG:
		req->task_attr = VIRTIO_SCSI_S_ORDERED;
		break;

	case XS_CTL_SIMPLE_TAG:
	default:
		req->task_attr = VIRTIO_SCSI_S_SIMPLE;
		break;
	}
	req->id = virtio_rw64(vsc, slot);

	if ((size_t)xs->cmdlen > sizeof(req->cdb)) {
		DPRINTF(("%s: bad cmdlen %zu > %zu\n", __func__,
		    (size_t)xs->cmdlen, sizeof(req->cdb)));
		goto stuffup;
	}

	memset(req->cdb, 0, sizeof(req->cdb));
	memcpy(req->cdb, xs->cmd, xs->cmdlen);

	error = bus_dmamap_load(virtio_dmat(vsc), vr->vr_data,
	    xs->data, xs->datalen, NULL, XS2DMA(xs));
	if (error) {
		aprint_error_dev(sc->sc_dev, "%s: error %d loading DMA map\n",
		    __func__, error);

		if (error == ENOMEM || error == EAGAIN) {
			/*
			 * Map is allocated with ALLOCNOW, so this should
			 * actually never ever happen.
			 */
			xs->error = XS_RESOURCE_SHORTAGE;
		} else {
stuffup:
			/* not a temporary condition */
			xs->error = XS_DRIVER_STUFFUP;
		}

		virtio_enqueue_abort(vsc, vq, slot);
		scsipi_done(xs);
		return;
	}

	int nsegs = VIRTIO_SCSI_MIN_SEGMENTS;
	if ((xs->xs_control & (XS_CTL_DATA_IN|XS_CTL_DATA_OUT)) != 0)
		nsegs += vr->vr_data->dm_nsegs;

	error = virtio_enqueue_reserve(vsc, vq, slot, nsegs);
	if (error) {
		aprint_error_dev(sc->sc_dev, "error reserving %d (nsegs %d)\n",
		    error, nsegs);
		bus_dmamap_unload(virtio_dmat(vsc), vr->vr_data);
		/* slot already freed by virtio_enqueue_reserve() */
		xs->error = XS_RESOURCE_SHORTAGE;
		scsipi_done(xs);
		return;
	}

	vr->vr_xs = xs;

	bus_dmamap_sync(virtio_dmat(vsc), vr->vr_control,
	    offsetof(struct vioscsi_req, vr_req),
	    sizeof(struct virtio_scsi_req_hdr),
	    BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(virtio_dmat(vsc), vr->vr_control,
	    offsetof(struct vioscsi_req, vr_res),
            sizeof(struct virtio_scsi_res_hdr),
	    BUS_DMASYNC_PREREAD);
	if ((xs->xs_control & (XS_CTL_DATA_IN|XS_CTL_DATA_OUT)) != 0)
		bus_dmamap_sync(virtio_dmat(vsc), vr->vr_data, 0, xs->datalen,
		    XS2DMAPRE(xs));

	virtio_enqueue_p(vsc, vq, slot, vr->vr_control,
	    offsetof(struct vioscsi_req, vr_req),
            sizeof(struct virtio_scsi_req_hdr), 1);
	if (xs->xs_control & XS_CTL_DATA_OUT)
		virtio_enqueue(vsc, vq, slot, vr->vr_data, 1);
	virtio_enqueue_p(vsc, vq, slot, vr->vr_control,
	    offsetof(struct vioscsi_req, vr_res),
            sizeof(struct virtio_scsi_res_hdr), 0);
	if (xs->xs_control & XS_CTL_DATA_IN)
		virtio_enqueue(vsc, vq, slot, vr->vr_data, 0);
	dopoll = (xs->xs_control & XS_CTL_POLL) != 0;
	virtio_enqueue_commit(vsc, vq, slot, 1);

	if (!dopoll)
		return;

	DPRINTF(("%s: polling...\n", __func__));
	// XXX: do this better.
	int timeout = 1000;
	do {
		virtio_intrhand(vsc);
		if (vr->vr_xs != xs)
			break;
		delay(1000);
	} while (--timeout > 0);

	if (vr->vr_xs == xs) {
		// XXX: Abort!
		xs->error = XS_TIMEOUT;
		xs->resid = xs->datalen;
		DPRINTF(("%s: polling timeout\n", __func__));
		scsipi_done(xs);
	}
	DPRINTF(("%s: command %p done (timeout=%d)\n", __func__,
	    xs, timeout));
}

static void
vioscsi_req_done(struct vioscsi_softc *sc, struct virtio_softc *vsc,
    struct vioscsi_req *vr, struct virtqueue *vq, int slot)
{
	struct scsipi_xfer *xs = vr->vr_xs;
	size_t sense_len;

	DPRINTF(("%s: enter\n", __func__));

	bus_dmamap_sync(virtio_dmat(vsc), vr->vr_control,
	    offsetof(struct vioscsi_req, vr_req),
	    sizeof(struct virtio_scsi_req_hdr),
	    BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(virtio_dmat(vsc), vr->vr_control,
	    offsetof(struct vioscsi_req, vr_res),
	    sizeof(struct virtio_scsi_res_hdr),
	    BUS_DMASYNC_POSTREAD);
	if (xs->datalen)
		bus_dmamap_sync(virtio_dmat(vsc), vr->vr_data, 0, xs->datalen,
		    XS2DMAPOST(xs));

	xs->status = vr->vr_res.status;
	xs->resid  = virtio_rw32(vsc, vr->vr_res.residual);

	switch (vr->vr_res.response) {
	case VIRTIO_SCSI_S_OK:
		sense_len = MIN(sizeof(xs->sense),
				virtio_rw32(vsc, vr->vr_res.sense_len));
		memcpy(&xs->sense, vr->vr_res.sense, sense_len);
		xs->error = (sense_len == 0) ? XS_NOERROR : XS_SENSE;
		break;
	case VIRTIO_SCSI_S_BAD_TARGET:
		vioscsi_bad_target(xs);
		break;
	default:
		DPRINTF(("%s: stuffup: %d\n", __func__, vr->vr_res.response));
		xs->error = XS_DRIVER_STUFFUP;
		xs->resid = xs->datalen;
		break;
	}

	DPRINTF(("%s: command %p done %d, %d, %d\n", __func__,
	    xs, xs->error, xs->status, xs->resid));

	bus_dmamap_unload(virtio_dmat(vsc), vr->vr_data);
	vr->vr_xs = NULL;

	virtio_dequeue_commit(vsc, vq, slot);

	mutex_exit(&sc->sc_mutex);
	scsipi_done(xs);
	mutex_enter(&sc->sc_mutex);
}

static void
vioscsi_bad_target(struct scsipi_xfer *xs)
{
	struct scsi_sense_data *sense = &xs->sense.scsi_sense;

	DPRINTF(("%s: bad target %d:%d\n", __func__,
	    xs->xs_periph->periph_target, xs->xs_periph->periph_lun));

	memset(sense, 0, sizeof(*sense));
	sense->response_code = 0x70;
	sense->flags = SKEY_ILLEGAL_REQUEST;
	xs->error = XS_SENSE;
	xs->status = 0;
	xs->resid = 0;
}

static int
vioscsi_vq_done(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioscsi_softc *sc = device_private(virtio_child(vsc));
	int ret = 0;

	DPRINTF(("%s: enter %d\n", __func__, vq->vq_index));

	mutex_enter(&sc->sc_mutex);

	for (;;) {
		int r, slot;

		r = virtio_dequeue(vsc, vq, &slot, NULL);
		if (r != 0)
			break;

		DPRINTF(("%s: slot=%d\n", __func__, slot));

		vioscsi_req_done(sc, vsc, &sc->sc_reqs[slot], vq, slot);

		ret = 1;
	}

	mutex_exit(&sc->sc_mutex);

	DPRINTF(("%s: exit %d: %d\n", __func__, vq->vq_index, ret));

	return ret;
}

static struct vioscsi_req *
vioscsi_req_get(struct vioscsi_softc *sc)
{
	struct virtio_softc *vsc = device_private(device_parent(sc->sc_dev));
	struct virtqueue *vq = &sc->sc_vqs[VIOSCSI_VQ_REQUEST];
	struct vioscsi_req *vr = NULL;
	int r, slot;

	mutex_enter(&sc->sc_mutex);

	if ((r = virtio_enqueue_prep(vsc, vq, &slot)) != 0) {
		DPRINTF(("%s: virtio_enqueue_get error %d\n", __func__, r));
		goto out;
	}
	KASSERT(slot < sc->sc_nreqs);
	vr = &sc->sc_reqs[slot];

	DPRINTF(("%s: %p, %d\n", __func__, vr, slot));

out:
	mutex_exit(&sc->sc_mutex);

	return vr;
}

static int
vioscsi_alloc_reqs(struct vioscsi_softc *sc, struct virtio_softc *vsc,
    int qsize)
{
	size_t allocsize;
	int r, rsegs, slot;
	void *vaddr;
	struct vioscsi_req *vr;

	allocsize = qsize * sizeof(struct vioscsi_req);
	r = bus_dmamem_alloc(virtio_dmat(vsc), allocsize, 0, 0,
	    &sc->sc_reqs_segs[0], 1, &rsegs, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
		    "%s: bus_dmamem_alloc, size %zu, error %d\n", __func__,
		    allocsize, r);
		return r;
	}
	r = bus_dmamem_map(virtio_dmat(vsc), &sc->sc_reqs_segs[0], 1,
	    allocsize, &vaddr, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
		    "%s: bus_dmamem_map failed, error %d\n", __func__, r);
		bus_dmamem_free(virtio_dmat(vsc), &sc->sc_reqs_segs[0], 1);
		return r;
	}
	memset(vaddr, 0, allocsize);

	sc->sc_reqs = vaddr;
	sc->sc_nreqs = qsize;

	/* Prepare maps for the requests */
	for (slot=0; slot < qsize; slot++) {
		vr = &sc->sc_reqs[slot];

		r = bus_dmamap_create(virtio_dmat(vsc),
		    offsetof(struct vioscsi_req, vr_xs), 1,
		    offsetof(struct vioscsi_req, vr_xs), 0,
		    BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW, &vr->vr_control);
		if (r != 0) {
			aprint_error_dev(sc->sc_dev,
		    	    "%s: bus_dmamem_create ctrl failed, error %d\n",
			    __func__, r);
			goto cleanup;
		}

		r = bus_dmamap_create(virtio_dmat(vsc), MAXPHYS, sc->sc_seg_max,
		    MAXPHYS, 0, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW, &vr->vr_data);
		if (r != 0) {
			aprint_error_dev(sc->sc_dev,
		    	    "%s: bus_dmamem_create data failed, error %d\n",
			    __func__, r);
			goto cleanup;
		}

		r = bus_dmamap_load(virtio_dmat(vsc), vr->vr_control,
		    vr, offsetof(struct vioscsi_req, vr_xs), NULL,
		    BUS_DMA_NOWAIT);
		if (r != 0) {
			aprint_error_dev(sc->sc_dev,
		    	    "%s: bus_dmamap_load ctrl error %d\n",
			    __func__, r);
			goto cleanup;
		}
	}

	return 0;

cleanup:
	for (; slot > 0; slot--) {
		vr = &sc->sc_reqs[slot];

		if (vr->vr_control) {
			/* this will also unload the mapping if loaded */
			bus_dmamap_destroy(virtio_dmat(vsc), vr->vr_control);
			vr->vr_control = NULL;
		}

		if (vr->vr_data) {
			bus_dmamap_destroy(virtio_dmat(vsc), vr->vr_data);
			vr->vr_data = NULL;
		}
	}

	bus_dmamem_unmap(virtio_dmat(vsc), vaddr, allocsize);
	bus_dmamem_free(virtio_dmat(vsc), &sc->sc_reqs_segs[0], 1);

	return r;
}

static void
vioscsi_free_reqs(struct vioscsi_softc *sc, struct virtio_softc *vsc)
{
	int slot;
	struct vioscsi_req *vr;

	if (sc->sc_nreqs == 0) {
		/* Not allocated */
		return;
	}

	/* Free request maps */
	for (slot=0; slot < sc->sc_nreqs; slot++) {
		vr = &sc->sc_reqs[slot];

		bus_dmamap_destroy(virtio_dmat(vsc), vr->vr_control);
		bus_dmamap_destroy(virtio_dmat(vsc), vr->vr_data);
	}

	bus_dmamem_unmap(virtio_dmat(vsc), sc->sc_reqs,
			 sc->sc_nreqs * sizeof(struct vioscsi_req));
	bus_dmamem_free(virtio_dmat(vsc), &sc->sc_reqs_segs[0], 1);
}

MODULE(MODULE_CLASS_DRIVER, vioscsi, "virtio");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
vioscsi_modcmd(modcmd_t cmd, void *opaque)
{
        int error = 0;

#ifdef _MODULE
        switch (cmd) {
        case MODULE_CMD_INIT:
                error = config_init_component(cfdriver_ioconf_vioscsi,
                    cfattach_ioconf_vioscsi, cfdata_ioconf_vioscsi);
                break;
        case MODULE_CMD_FINI:
                error = config_fini_component(cfdriver_ioconf_vioscsi,
                    cfattach_ioconf_vioscsi, cfdata_ioconf_vioscsi);
                break;
        default:
                error = ENOTTY;
                break;
        }
#endif

        return error;
}
