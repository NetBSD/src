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
__KERNEL_RCSID(0, "$NetBSD: vioscsi.c,v 1.3 2015/10/30 21:18:16 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/buf.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

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
	struct vioscsi_req	*sc_reqs;
	bus_dma_segment_t        sc_reqs_segs[1];

	u_int32_t		 sc_seg_max;
};

/*      
 * Each block request uses at least two segments - one for the header
 * and one for the status.
*/
#define VIRTIO_SCSI_MIN_SEGMENTS 2

static int	 vioscsi_match(device_t, cfdata_t, void *);
static void	 vioscsi_attach(device_t, device_t, void *);

static int	 vioscsi_alloc_reqs(struct vioscsi_softc *,
    struct virtio_softc *, int, uint32_t);
static void	 vioscsi_scsipi_request(struct scsipi_channel *,
    scsipi_adapter_req_t, void *);
static int	 vioscsi_vq_done(struct virtqueue *);
static void	 vioscsi_req_done(struct vioscsi_softc *, struct virtio_softc *,
    struct vioscsi_req *);
static struct vioscsi_req *vioscsi_req_get(struct vioscsi_softc *);
static void	 vioscsi_req_put(struct vioscsi_softc *, struct vioscsi_req *);

static const char *const vioscsi_vq_names[] = {
	"control",
	"event",
	"request",
};

CFATTACH_DECL_NEW(vioscsi, sizeof(struct vioscsi_softc),
    vioscsi_match, vioscsi_attach, NULL, NULL);

static int
vioscsi_match(device_t parent, cfdata_t match, void *aux)
{
	struct virtio_softc *va = aux;

	if (va->sc_childdevid == PCI_PRODUCT_VIRTIO_SCSI)
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
	uint32_t features;
	char buf[256];
	int rv;

	if (vsc->sc_child != NULL) {
		aprint_error(": parent %s already has a child\n",
		    device_xname(parent));
		return;
	}

	sc->sc_dev = self;

	vsc->sc_child = self;
	vsc->sc_ipl = IPL_BIO;
	vsc->sc_vqs = sc->sc_vqs;
	vsc->sc_nvqs = __arraycount(sc->sc_vqs);
	vsc->sc_config_change = NULL;
	vsc->sc_intrhand = virtio_vq_intr;
	vsc->sc_flags = 0;

	features = virtio_negotiate_features(vsc, 0);
	snprintb(buf, sizeof(buf), VIRTIO_COMMON_FLAG_BITS, features);
	aprint_normal(": Features: %s\n", buf);
	aprint_naive("\n");

	uint32_t cmd_per_lun = virtio_read_device_config_4(vsc,
	    VIRTIO_SCSI_CONFIG_CMD_PER_LUN);

	uint32_t seg_max = virtio_read_device_config_4(vsc,
	    VIRTIO_SCSI_CONFIG_SEG_MAX);

	uint16_t max_target = virtio_read_device_config_2(vsc,
	    VIRTIO_SCSI_CONFIG_MAX_TARGET);

	uint16_t max_channel = virtio_read_device_config_2(vsc,
	    VIRTIO_SCSI_CONFIG_MAX_CHANNEL);

	uint32_t max_lun = virtio_read_device_config_4(vsc,
	    VIRTIO_SCSI_CONFIG_MAX_LUN);

	sc->sc_seg_max = seg_max;

	for (size_t i = 0; i < __arraycount(sc->sc_vqs); i++) {
		rv = virtio_alloc_vq(vsc, &sc->sc_vqs[i], i, MAXPHYS,
		    1 + howmany(MAXPHYS, NBPG), vioscsi_vq_names[i]);
		if (rv) {
			aprint_error_dev(sc->sc_dev,
			    "failed to allocate virtqueue %zu\n", i);
			return;
		}
		sc->sc_vqs[i].vq_done = vioscsi_vq_done;
	}

	int qsize = sc->sc_vqs[2].vq_num;
	aprint_normal_dev(sc->sc_dev, "qsize %d\n", qsize);
	if (vioscsi_alloc_reqs(sc, vsc, qsize, seg_max))
		return;

	/*
	 * Fill in the scsipi_adapter.
	 */
	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_dev = sc->sc_dev;
	adapt->adapt_nchannels = max_channel;
	adapt->adapt_openings = cmd_per_lun;
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
	chan->chan_ntargets = max_target;
	chan->chan_nluns = max_lun;
	chan->chan_id = 0;

	config_found(sc->sc_dev, &sc->sc_channel, scsiprint);
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
	struct virtqueue *vq = &sc->sc_vqs[2];
	int slot, error;

	DPRINTF(("%s: enter\n", __func__));

	if (request != ADAPTER_REQ_RUN_XFER) {
		DPRINTF(("%s: unhandled %d\n", __func__, request));
		return;
	}
	
	xs = arg;
	periph = xs->xs_periph;

	vr = vioscsi_req_get(sc);
#ifdef DIAGNOSTIC
	/*
	 * This should never happen as we track the resources
	 * in the mid-layer.
	 */
	if (vr == NULL) {
		scsipi_printaddr(xs->xs_periph);
		panic("%s: unable to allocate request\n", __func__);
	}
#endif
	req = &vr->vr_req;
	slot = vr - sc->sc_reqs;

	vr->vr_xs = xs;

	/*
	 * "The only supported format for the LUN field is: first byte set to
	 * 1, second byte set to target, third and fourth byte representing a
	 * single level LUN structure, followed by four zero bytes."
	 */
	if (periph->periph_target >= 256 || periph->periph_lun >= 16384) {
		DPRINTF(("%s: bad target %u or lun %u\n", __func__,
		    periph->periph_target, periph->periph_lun));
		goto stuffup;
	}
	req->lun[0] = 1;
	req->lun[1] = periph->periph_target - 1;
	req->lun[2] = 0x40 | (periph->periph_lun >> 8);
	req->lun[3] = periph->periph_lun;
	memset(req->lun + 4, 0, 4);
	DPRINTF(("%s: command for %u:%u at slot %d\n", __func__,
	    periph->periph_target - 1, periph->periph_lun, slot));

	if ((size_t)xs->cmdlen > sizeof(req->cdb)) {
		DPRINTF(("%s: bad cmdlen %zu > %zu\n", __func__,
		    (size_t)xs->cmdlen, sizeof(req->cdb)));
		goto stuffup;
	}

	memset(req->cdb, 0, sizeof(req->cdb));
	memcpy(req->cdb, xs->cmd, xs->cmdlen);

	error = bus_dmamap_load(vsc->sc_dmat, vr->vr_data,
	    xs->data, xs->datalen, NULL, XS2DMA(xs));
	switch (error) {
	case 0:
		break;
	case ENOMEM:
	case EAGAIN:
		xs->error = XS_RESOURCE_SHORTAGE;
		goto nomore;
	default:
		aprint_error_dev(sc->sc_dev, "error %d loading DMA map\n",
		    error);
	stuffup:
		xs->error = XS_DRIVER_STUFFUP;
nomore:
		// XXX: free req?
		scsipi_done(xs);
		return;
	}

	int nsegs = VIRTIO_SCSI_MIN_SEGMENTS;
	if ((xs->xs_control & (XS_CTL_DATA_IN|XS_CTL_DATA_OUT)) != 0)
		nsegs += vr->vr_data->dm_nsegs;

	error = virtio_enqueue_reserve(vsc, vq, slot, nsegs);
	if (error) {
		DPRINTF(("%s: error reserving %d\n", __func__, error));
		goto stuffup;
	}

	bus_dmamap_sync(vsc->sc_dmat, vr->vr_control,
	    offsetof(struct vioscsi_req, vr_req),
	    sizeof(struct virtio_scsi_req_hdr),
	    BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(vsc->sc_dmat, vr->vr_control,
	    offsetof(struct vioscsi_req, vr_res),
            sizeof(struct virtio_scsi_res_hdr),
	    BUS_DMASYNC_PREREAD);
	if ((xs->xs_control & (XS_CTL_DATA_IN|XS_CTL_DATA_OUT)) != 0)
		bus_dmamap_sync(vsc->sc_dmat, vr->vr_data, 0, xs->datalen,
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
	virtio_enqueue_commit(vsc, vq, slot, 1);

	if ((xs->xs_control & XS_CTL_POLL) == 0)
		return;

	DPRINTF(("%s: polling...\n", __func__));
	// XXX: do this better.
	int timeout = 1000;
	do {
		(*vsc->sc_intrhand)(vsc);
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
	DPRINTF(("%s: done (timeout=%d)\n", __func__, timeout));
}

static void
vioscsi_req_done(struct vioscsi_softc *sc, struct virtio_softc *vsc,
    struct vioscsi_req *vr)
{
	struct scsipi_xfer *xs = vr->vr_xs;

	DPRINTF(("%s: enter\n", __func__));

	bus_dmamap_sync(vsc->sc_dmat, vr->vr_control,
	    offsetof(struct vioscsi_req, vr_req),
	    sizeof(struct virtio_scsi_req_hdr),
	    BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(vsc->sc_dmat, vr->vr_control,
	    offsetof(struct vioscsi_req, vr_res),
	    sizeof(struct virtio_scsi_res_hdr),
	    BUS_DMASYNC_POSTREAD);
	bus_dmamap_sync(vsc->sc_dmat, vr->vr_data, 0, xs->datalen,
	    XS2DMAPOST(xs));

	if (vr->vr_res.response != VIRTIO_SCSI_S_OK) {
		xs->error = XS_DRIVER_STUFFUP;
		xs->resid = xs->datalen;
		DPRINTF(("%s: stuffup: %d\n", __func__, vr->vr_res.response));
		goto done;
	}

	size_t sense_len = MIN(sizeof(xs->sense), vr->vr_res.sense_len);
	memcpy(&xs->sense, vr->vr_res.sense, sense_len);
	xs->error = (sense_len == 0) ? XS_NOERROR : XS_SENSE;

	xs->status = vr->vr_res.status;
	xs->resid = vr->vr_res.residual;

	DPRINTF(("%s: done %d, %d, %d\n", __func__,
	    xs->error, xs->status, xs->resid));

done:
	vr->vr_xs = NULL;
	vioscsi_req_put(sc, vr);
	scsipi_done(xs);
}

static int
vioscsi_vq_done(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct vioscsi_softc *sc = device_private(vsc->sc_child);
	int ret = 0;

	DPRINTF(("%s: enter\n", __func__));

	for (;;) {
		int r, slot;
		r = virtio_dequeue(vsc, vq, &slot, NULL);
		if (r != 0)
			break;

		DPRINTF(("%s: slot=%d\n", __func__, slot));
		vioscsi_req_done(sc, vsc, &sc->sc_reqs[slot]);
		ret = 1;
	}

	DPRINTF(("%s: exit %d\n", __func__, ret));

	return ret;
}

static struct vioscsi_req *
vioscsi_req_get(struct vioscsi_softc *sc)
{
	struct virtio_softc *vsc = device_private(device_parent(sc->sc_dev));
	struct virtqueue *vq = &sc->sc_vqs[2];
	struct vioscsi_req *vr;
	int r, slot;

	if ((r = virtio_enqueue_prep(vsc, vq, &slot)) != 0) {
		DPRINTF(("%s: virtio_enqueue_get error %d\n", __func__, r));
		goto err1;
	}
	vr = &sc->sc_reqs[slot];

	vr->vr_req.id = slot;
	vr->vr_req.task_attr = VIRTIO_SCSI_S_SIMPLE;

	r = bus_dmamap_create(vsc->sc_dmat,
	    offsetof(struct vioscsi_req, vr_xs), 1,
	    offsetof(struct vioscsi_req, vr_xs), 0,
	    BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW, &vr->vr_control);
	if (r != 0) {
		DPRINTF(("%s: bus_dmamap_create xs error %d\n", __func__, r));
		goto err2;
	}
	r = bus_dmamap_create(vsc->sc_dmat, MAXPHYS, sc->sc_seg_max,
	    MAXPHYS, 0, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW, &vr->vr_data);
	if (r != 0) {
		DPRINTF(("%s: bus_dmamap_create data error %d\n", __func__, r));
		goto err3;
	}
	r = bus_dmamap_load(vsc->sc_dmat, vr->vr_control,
	    vr, offsetof(struct vioscsi_req, vr_xs), NULL,
	    BUS_DMA_NOWAIT);
	if (r != 0) {
		DPRINTF(("%s: bus_dmamap_create ctrl error %d\n", __func__, r));
		goto err4;
	}

	DPRINTF(("%s: %p, %d\n", __func__, vr, slot));

	return vr;

err4:
	bus_dmamap_destroy(vsc->sc_dmat, vr->vr_data);
err3:
	bus_dmamap_destroy(vsc->sc_dmat, vr->vr_control);
err2:
	virtio_enqueue_abort(vsc, vq, slot);
err1:
	return NULL;
}

static void
vioscsi_req_put(struct vioscsi_softc *sc, struct vioscsi_req *vr)
{
	struct virtio_softc *vsc = device_private(device_parent(sc->sc_dev));
	struct virtqueue *vq = &sc->sc_vqs[2];
	int slot = vr - sc->sc_reqs;

	DPRINTF(("%s: %p, %d\n", __func__, vr, slot));

	bus_dmamap_destroy(vsc->sc_dmat, vr->vr_control);
	bus_dmamap_destroy(vsc->sc_dmat, vr->vr_data);

	virtio_dequeue_commit(vsc, vq, slot);
}

int
vioscsi_alloc_reqs(struct vioscsi_softc *sc, struct virtio_softc *vsc,
    int qsize, uint32_t seg_max)
{
	size_t allocsize;
	int r, rsegs;
	void *vaddr;

	allocsize = qsize * sizeof(struct vioscsi_req);
	r = bus_dmamem_alloc(vsc->sc_dmat, allocsize, 0, 0,
	    &sc->sc_reqs_segs[0], 1, &rsegs, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
		    "%s: bus_dmamem_alloc, size %zu, error %d\n", __func__,
		    allocsize, r);
		return 1;
	}
	r = bus_dmamem_map(vsc->sc_dmat, &sc->sc_reqs_segs[0], 1,
	    allocsize, &vaddr, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(sc->sc_dev,
		    "%s: bus_dmamem_map failed, error %d\n", __func__, r);
		bus_dmamem_free(vsc->sc_dmat, &sc->sc_reqs_segs[0], 1);
		return 1;
	}
	sc->sc_reqs = vaddr;
	memset(vaddr, 0, allocsize);
	return 0;
}
