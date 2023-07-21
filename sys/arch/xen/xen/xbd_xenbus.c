/*      $NetBSD: xbd_xenbus.c,v 1.133 2023/07/21 11:28:50 bouyer Exp $      */

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 *
 */

/*
 * The file contains the xbd frontend code required for block-level
 * communications (similar to hard disks) between two Xen domains.
 *
 * We are not supposed to receive solicitations spontaneously from backend. The
 * protocol is therefore fairly simple and uses only one ring to communicate
 * with backend: frontend posts requests to the ring then wait for their
 * replies asynchronously.
 *
 * xbd follows NetBSD's disk(9) convention. At any time, a LWP can schedule
 * an operation request for the device (be it open(), read(), write(), ...).
 * Calls are typically processed that way:
 * - initiate request: xbdread/write/open/ioctl/..
 * - depending on operation, it is handled directly by disk(9) subsystem or
 *   goes through physio(9) first.
 * - the request is ultimately processed by xbd_diskstart() that prepares the
 *   xbd requests, post them in the ring I/O queue, then signal the backend.
 *
 * When a response is available in the queue, the backend signals the frontend
 * via its event channel. This triggers xbd_handler(), which will link back
 * the response to its request through the request ID, and mark the I/O as
 * completed.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xbd_xenbus.c,v 1.133 2023/07/21 11:28:50 bouyer Exp $");

#include "opt_xen.h"


#include <sys/param.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/mutex.h>

#include <dev/dkvar.h>

#include <uvm/uvm.h>

#include <xen/intr.h>
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/granttables.h>
#include <xen/include/public/io/blkif.h>
#include <xen/include/public/io/protocols.h>

#include <xen/xenbus.h>
#include "locators.h"

#undef XBD_DEBUG
#ifdef XBD_DEBUG
#define DPRINTF(x) printf x;
#else
#define DPRINTF(x)
#endif

#define GRANT_INVALID_REF -1

#define XBD_RING_SIZE __CONST_RING_SIZE(blkif, PAGE_SIZE)
#define XBD_MAX_XFER (PAGE_SIZE * BLKIF_MAX_SEGMENTS_PER_REQUEST)
#define XBD_MAX_CHUNK	32*1024		/* max I/O size we process in 1 req */
#define XBD_XFER_LIMIT	(2*XBD_MAX_XFER)

#define XEN_BSHIFT      9               /* log2(XEN_BSIZE) */
#define XEN_BSIZE       (1 << XEN_BSHIFT)

CTASSERT((MAXPHYS <= 2*XBD_MAX_CHUNK));
CTASSERT(XEN_BSIZE == DEV_BSIZE);

struct xbd_indirect {
	SLIST_ENTRY(xbd_indirect) in_next;
	struct blkif_request_segment *in_addr;
	grant_ref_t in_gntref;
};

struct xbd_req {
	SLIST_ENTRY(xbd_req) req_next;
	uint16_t req_id; /* ID passed to backend */
	bus_dmamap_t req_dmamap;
	struct xbd_req *req_parent, *req_child;
	bool req_parent_done;
	union {
	    struct {
		grant_ref_t req_gntref[XBD_XFER_LIMIT >> PAGE_SHIFT];
		struct buf *req_bp; /* buffer associated with this request */
		void *req_data; /* pointer to the data buffer */
		struct xbd_indirect *req_indirect;	/* indirect page */
	    } req_rw;
	    struct {
		int s_error;
		int s_done;
	    } req_sync;
	} u;
};
#define req_gntref	u.req_rw.req_gntref
#define req_bp		u.req_rw.req_bp
#define req_data	u.req_rw.req_data
#define req_indirect	u.req_rw.req_indirect
#define req_sync	u.req_sync

struct xbd_xenbus_softc {
	struct dk_softc sc_dksc;	/* Must be first in this struct */
	struct xenbus_device *sc_xbusd;
	unsigned int sc_evtchn;

	struct intrhand *sc_ih; /* Interrupt handler for this instance. */
	kmutex_t sc_lock;
	kcondvar_t sc_cache_flush_cv;
	kcondvar_t sc_req_cv;
	kcondvar_t sc_detach_cv;
	kcondvar_t sc_suspend_cv;

	blkif_front_ring_t sc_ring;
	grant_ref_t sc_ring_gntref;

	struct xbd_req sc_reqs[XBD_RING_SIZE];
	SLIST_HEAD(,xbd_req) sc_xbdreq_head; /* list of free requests */

	struct xbd_indirect sc_indirect[XBD_RING_SIZE];
	SLIST_HEAD(,xbd_indirect) sc_indirect_head;

	vmem_addr_t sc_unalign_buffer;
	void *sc_unalign_used;

	int sc_backend_status; /* our status with backend */
#define BLKIF_STATE_DISCONNECTED 0
#define BLKIF_STATE_CONNECTED    1
#define BLKIF_STATE_SUSPENDED    2

	int sc_shutdown;
#define BLKIF_SHUTDOWN_RUN    0 /* no shutdown */
#define BLKIF_SHUTDOWN_REMOTE 1 /* backend-initiated shutdown in progress */
#define BLKIF_SHUTDOWN_LOCAL  2 /* locally-initiated shutdown in progress */

	uint64_t sc_sectors; /* number of XEN_BSIZE sectors for this device */
	u_long sc_secsize; /* sector size */
	uint64_t sc_xbdsize; /* size of disk in DEV_BSIZE */
	u_long sc_info; /* VDISK_* */
	u_long sc_handle; /* from backend */
	int sc_features;
#define BLKIF_FEATURE_CACHE_FLUSH	0x1
#define BLKIF_FEATURE_BARRIER		0x2
#define BLKIF_FEATURE_PERSISTENT	0x4
#define BLKIF_FEATURE_INDIRECT		0x8
#define BLKIF_FEATURE_BITS		\
	"\20\1CACHE-FLUSH\2BARRIER\3PERSISTENT\4INDIRECT"
	struct evcnt sc_cnt_map_unalign;
	struct evcnt sc_cnt_unalign_busy;
	struct evcnt sc_cnt_queue_full;
	struct evcnt sc_cnt_indirect;
};

static int  xbd_xenbus_match(device_t, cfdata_t, void *);
static void xbd_xenbus_attach(device_t, device_t, void *);
static int  xbd_xenbus_detach(device_t, int);

static bool xbd_xenbus_suspend(device_t, const pmf_qual_t *);
static bool xbd_xenbus_resume(device_t, const pmf_qual_t *);

static int  xbd_handler(void *);
static int  xbd_diskstart(device_t, struct buf *);
static void xbd_iosize(device_t, int *);
static void xbd_backend_changed(void *, XenbusState);
static void xbd_connect(struct xbd_xenbus_softc *);
static void xbd_features(struct xbd_xenbus_softc *);

static void xbd_diskstart_submit(struct xbd_xenbus_softc *, int,
	struct buf *bp, int, bus_dmamap_t, grant_ref_t *);
static void xbd_diskstart_submit_indirect(struct xbd_xenbus_softc *,
	struct xbd_req *, struct buf *bp);
static int  xbd_map_align(struct xbd_xenbus_softc *, struct xbd_req *);
static void xbd_unmap_align(struct xbd_xenbus_softc *, struct xbd_req *,
	struct buf *);

static void xbdminphys(struct buf *);

CFATTACH_DECL3_NEW(xbd, sizeof(struct xbd_xenbus_softc),
    xbd_xenbus_match, xbd_xenbus_attach, xbd_xenbus_detach, NULL, NULL, NULL,
    DVF_DETACH_SHUTDOWN);

static dev_type_open(xbdopen);
static dev_type_close(xbdclose);
static dev_type_read(xbdread);
static dev_type_write(xbdwrite);
static dev_type_ioctl(xbdioctl);
static dev_type_strategy(xbdstrategy);
static dev_type_dump(xbddump);
static dev_type_size(xbdsize);

const struct bdevsw xbd_bdevsw = {
	.d_open = xbdopen,
	.d_close = xbdclose,
	.d_strategy = xbdstrategy,
	.d_ioctl = xbdioctl,
	.d_dump = xbddump,
	.d_psize = xbdsize,
	.d_discard = nodiscard,
	.d_flag = D_DISK | D_MPSAFE
};

const struct cdevsw xbd_cdevsw = {
	.d_open = xbdopen,
	.d_close = xbdclose,
	.d_read = xbdread,
	.d_write = xbdwrite,
	.d_ioctl = xbdioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_DISK | D_MPSAFE
};

extern struct cfdriver xbd_cd;

static const struct dkdriver xbddkdriver = {
        .d_strategy = xbdstrategy,
	.d_minphys = xbdminphys,
	.d_open = xbdopen,
	.d_close = xbdclose,
	.d_diskstart = xbd_diskstart,
	.d_iosize = xbd_iosize,
};

static int
xbd_xenbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct xenbusdev_attach_args *xa = aux;

	if (strcmp(xa->xa_type, "vbd") != 0)
		return 0;

	if (match->cf_loc[XENBUSCF_ID] != XENBUSCF_ID_DEFAULT &&
	    match->cf_loc[XENBUSCF_ID] != xa->xa_id)
		return 0;

	return 1;
}

static void
xbd_xenbus_attach(device_t parent, device_t self, void *aux)
{
	struct xbd_xenbus_softc *sc = device_private(self);
	struct xenbusdev_attach_args *xa = aux;
	blkif_sring_t *ring;
	RING_IDX i;

	config_pending_incr(self);
	aprint_normal(": Xen Virtual Block Device Interface\n");

	dk_init(&sc->sc_dksc, self, DKTYPE_ESDI);
	disk_init(&sc->sc_dksc.sc_dkdev, device_xname(self), &xbddkdriver);

	sc->sc_xbusd = xa->xa_xbusd;
	sc->sc_xbusd->xbusd_otherend_changed = xbd_backend_changed;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_cache_flush_cv, "xbdsync");
	cv_init(&sc->sc_req_cv, "xbdreq");
	cv_init(&sc->sc_detach_cv, "xbddetach");
	cv_init(&sc->sc_suspend_cv, "xbdsuspend");

	xbd_features(sc);

	/* initialize free requests list */
	SLIST_INIT(&sc->sc_xbdreq_head);
	for (i = 0; i < XBD_RING_SIZE; i++) {
		sc->sc_reqs[i].req_id = i;
		SLIST_INSERT_HEAD(&sc->sc_xbdreq_head, &sc->sc_reqs[i],
		    req_next);
	}

	if (sc->sc_features & BLKIF_FEATURE_INDIRECT) {
		/* initialize indirect page list */
		for (i = 0; i < XBD_RING_SIZE; i++) {
			vmem_addr_t va;
			if (uvm_km_kmem_alloc(kmem_va_arena,
			    PAGE_SIZE, VM_SLEEP | VM_INSTANTFIT, &va) != 0) {
				aprint_error_dev(self,
				    "can't alloc indirect pages\n");
				return;
			}
			sc->sc_indirect[i].in_addr = (void *)va;
			SLIST_INSERT_HEAD(&sc->sc_indirect_head,
			    &sc->sc_indirect[i], in_next);
		}
	}

	sc->sc_backend_status = BLKIF_STATE_DISCONNECTED;
	sc->sc_shutdown = BLKIF_SHUTDOWN_REMOTE;

	ring = (void *)uvm_km_alloc(kernel_map, PAGE_SIZE, 0, UVM_KMF_WIRED);
	if (ring == NULL)
		panic("%s: can't alloc ring", device_xname(self));
	sc->sc_ring.sring = ring;

	evcnt_attach_dynamic(&sc->sc_cnt_map_unalign, EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "map unaligned");
	evcnt_attach_dynamic(&sc->sc_cnt_unalign_busy, EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "map unaligned");
	evcnt_attach_dynamic(&sc->sc_cnt_queue_full, EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "queue full");
	evcnt_attach_dynamic(&sc->sc_cnt_indirect, EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "indirect segment");

	for (i = 0; i < XBD_RING_SIZE; i++) {
		if (bus_dmamap_create(sc->sc_xbusd->xbusd_dmat,
		    MAXPHYS, XBD_XFER_LIMIT >> PAGE_SHIFT,
		    PAGE_SIZE, PAGE_SIZE, BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW,
		    &sc->sc_reqs[i].req_dmamap) != 0) {
			aprint_error_dev(self, "can't alloc dma maps\n");
			return;
		}
	}

	if (uvm_km_kmem_alloc(kmem_va_arena,
	    MAXPHYS, VM_SLEEP | VM_INSTANTFIT, &sc->sc_unalign_buffer) != 0) {
		aprint_error_dev(self, "can't alloc align buffer\n");
		return;
	}

	/* resume shared structures and tell backend that we are ready */
	if (xbd_xenbus_resume(self, PMF_Q_NONE) == false) {
		uvm_km_free(kernel_map, (vaddr_t)ring, PAGE_SIZE,
		    UVM_KMF_WIRED);
		return;
	}

	if (!pmf_device_register(self, xbd_xenbus_suspend, xbd_xenbus_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
xbd_xenbus_detach(device_t dev, int flags)
{
	struct xbd_xenbus_softc *sc = device_private(dev);
	int bmaj, cmaj, i, mn, rc;

	DPRINTF(("%s: xbd_detach\n", device_xname(dev)));

	rc = disk_begindetach(&sc->sc_dksc.sc_dkdev, NULL, dev, flags);
	if (rc != 0)
		return rc;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_shutdown == BLKIF_SHUTDOWN_RUN) {
		sc->sc_shutdown = BLKIF_SHUTDOWN_LOCAL;

		/* wait for requests to complete */
		while (sc->sc_backend_status == BLKIF_STATE_CONNECTED &&
		    disk_isbusy(&sc->sc_dksc.sc_dkdev)) {
			cv_timedwait(&sc->sc_detach_cv, &sc->sc_lock, hz/2);
		}
		mutex_exit(&sc->sc_lock);

		/* Trigger state transition with backend */
		xenbus_switch_state(sc->sc_xbusd, NULL, XenbusStateClosing);

		mutex_enter(&sc->sc_lock);
	}
	if ((flags & DETACH_FORCE) == 0) {
		/* xbd_xenbus_detach already in progress */
		cv_broadcast(&sc->sc_detach_cv);
		mutex_exit(&sc->sc_lock);
		return EALREADY;
	}
	mutex_exit(&sc->sc_lock);
	while (xenbus_read_driver_state(sc->sc_xbusd->xbusd_otherend)
	    != XenbusStateClosed) {
		mutex_enter(&sc->sc_lock);
		cv_timedwait(&sc->sc_detach_cv, &sc->sc_lock, hz/2);
		mutex_exit(&sc->sc_lock);
	}

	/* locate the major number */
	bmaj = bdevsw_lookup_major(&xbd_bdevsw);
	cmaj = cdevsw_lookup_major(&xbd_cdevsw);

	/* Nuke the vnodes for any open instances. */
	for (i = 0; i < MAXPARTITIONS; i++) {
		mn = DISKMINOR(device_unit(dev), i);
		vdevgone(bmaj, mn, mn, VBLK);
		vdevgone(cmaj, mn, mn, VCHR);
	}

	if (sc->sc_backend_status == BLKIF_STATE_CONNECTED) {
		/* Delete all of our wedges. */
		dkwedge_delall(&sc->sc_dksc.sc_dkdev);

		/* Kill off any queued buffers. */
		dk_drain(&sc->sc_dksc);
		bufq_free(sc->sc_dksc.sc_bufq);

		/* detach disk */
		disk_detach(&sc->sc_dksc.sc_dkdev);
		disk_destroy(&sc->sc_dksc.sc_dkdev);
		dk_detach(&sc->sc_dksc);
	}

	hypervisor_mask_event(sc->sc_evtchn);
	if (sc->sc_ih != NULL) {
		xen_intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}

	mutex_enter(&sc->sc_lock);
	while (xengnt_status(sc->sc_ring_gntref))
		cv_timedwait(&sc->sc_detach_cv, &sc->sc_lock, hz/2);
	mutex_exit(&sc->sc_lock);

	xengnt_revoke_access(sc->sc_ring_gntref);
	uvm_km_free(kernel_map, (vaddr_t)sc->sc_ring.sring,
	    PAGE_SIZE, UVM_KMF_WIRED);

	for (i = 0; i < XBD_RING_SIZE; i++) {
		if (sc->sc_reqs[i].req_dmamap != NULL) {
			bus_dmamap_destroy(sc->sc_xbusd->xbusd_dmat,
			    sc->sc_reqs[i].req_dmamap);
			sc->sc_reqs[i].req_dmamap = NULL;
		}
	}

	if (sc->sc_unalign_buffer != 0) {
		uvm_km_kmem_free(kmem_va_arena, sc->sc_unalign_buffer, MAXPHYS);
		sc->sc_unalign_buffer = 0;
	}

	mutex_destroy(&sc->sc_lock);

	evcnt_detach(&sc->sc_cnt_map_unalign);
	evcnt_detach(&sc->sc_cnt_unalign_busy);
	evcnt_detach(&sc->sc_cnt_queue_full);
	evcnt_detach(&sc->sc_cnt_indirect);

	pmf_device_deregister(dev);

	return 0;
}

static bool
xbd_xenbus_suspend(device_t dev, const pmf_qual_t *qual) {

	struct xbd_xenbus_softc *sc;

	sc = device_private(dev);

	mutex_enter(&sc->sc_lock);
	/* wait for requests to complete, then suspend device */
	while (sc->sc_backend_status == BLKIF_STATE_CONNECTED &&
	    disk_isbusy(&sc->sc_dksc.sc_dkdev)) {
		cv_timedwait(&sc->sc_suspend_cv, &sc->sc_lock, hz/2);
	}

	hypervisor_mask_event(sc->sc_evtchn);
	sc->sc_backend_status = BLKIF_STATE_SUSPENDED;

#ifdef DIAGNOSTIC
	/* Check that all requests are finished and device ready for resume */
	int reqcnt = 0;
	struct xbd_req *req;
	SLIST_FOREACH(req, &sc->sc_xbdreq_head, req_next)
		reqcnt++;
	KASSERT(reqcnt == __arraycount(sc->sc_reqs));

	int incnt = 0;
	struct xbd_indirect *in;
	SLIST_FOREACH(in, &sc->sc_indirect_head, in_next)
		incnt++;
	KASSERT(incnt == __arraycount(sc->sc_indirect));
#endif

	mutex_exit(&sc->sc_lock);

	xenbus_device_suspend(sc->sc_xbusd);
	aprint_verbose_dev(dev, "removed event channel %d\n", sc->sc_evtchn);

	return true;
}

static bool
xbd_xenbus_resume(device_t dev, const pmf_qual_t *qual)
{
	struct xbd_xenbus_softc *sc;
	struct xenbus_transaction *xbt;
	int error;
	blkif_sring_t *ring;
	paddr_t ma;
	const char *errmsg;

	sc = device_private(dev);

	/* All grants were removed during suspend */
	sc->sc_ring_gntref = GRANT_INVALID_REF;

	/* Initialize ring */
	ring = sc->sc_ring.sring;
	memset(ring, 0, PAGE_SIZE);
	SHARED_RING_INIT(ring);
	FRONT_RING_INIT(&sc->sc_ring, ring, PAGE_SIZE);

	/*
	 * get MA address of the ring, and use it to set up the grant entry
	 * for the block device
	 */
	(void)pmap_extract_ma(pmap_kernel(), (vaddr_t)ring, &ma);
	error = xenbus_grant_ring(sc->sc_xbusd, ma, &sc->sc_ring_gntref);
	if (error)
		goto abort_resume;

	if (sc->sc_features & BLKIF_FEATURE_INDIRECT) {
		for (int i = 0; i < XBD_RING_SIZE; i++) {
			vaddr_t va = (vaddr_t)sc->sc_indirect[i].in_addr;
			KASSERT(va != 0);
			KASSERT((va & PAGE_MASK) == 0);
			(void)pmap_extract_ma(pmap_kernel(), va, &ma);
			if (xengnt_grant_access(
			    sc->sc_xbusd->xbusd_otherend_id,
			    ma, true, &sc->sc_indirect[i].in_gntref)) {
				aprint_error_dev(dev,
				    "indirect page grant failed\n");
				goto abort_resume;
			}
		}
	}

	error = xenbus_alloc_evtchn(sc->sc_xbusd, &sc->sc_evtchn);
	if (error)
		goto abort_resume;

	if (sc->sc_ih != NULL) {
		xen_intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}
	aprint_verbose_dev(dev, "using event channel %d\n",
	    sc->sc_evtchn);
	sc->sc_ih = xen_intr_establish_xname(-1, &xen_pic, sc->sc_evtchn,
	    IST_LEVEL, IPL_BIO, &xbd_handler, sc, true, device_xname(dev));
	KASSERT(sc->sc_ih != NULL);

again:
	xbt = xenbus_transaction_start();
	if (xbt == NULL)
		return false;
	error = xenbus_printf(xbt, sc->sc_xbusd->xbusd_path,
	    "ring-ref","%u", sc->sc_ring_gntref);
	if (error) {
		errmsg = "writing ring-ref";
		goto abort_transaction;
	}
	error = xenbus_printf(xbt, sc->sc_xbusd->xbusd_path,
	    "event-channel", "%u", sc->sc_evtchn);
	if (error) {
		errmsg = "writing event channel";
		goto abort_transaction;
	}
	error = xenbus_printf(xbt, sc->sc_xbusd->xbusd_path,
	    "protocol", "%s", XEN_IO_PROTO_ABI_NATIVE);
	if (error) {
		errmsg = "writing protocol";
		goto abort_transaction;
	}
	error = xenbus_transaction_end(xbt, 0);
	if (error == EAGAIN)
		goto again;
	if (error != 0) {
		xenbus_dev_fatal(sc->sc_xbusd, error,
		    "completing transaction");
		return false;
	}

	xenbus_switch_state(sc->sc_xbusd, NULL, XenbusStateInitialised);

	if (sc->sc_backend_status == BLKIF_STATE_SUSPENDED) {
		/*
		 * device was suspended, softc structures are
		 * already initialized - we use a shortcut
		 */
		sc->sc_backend_status = BLKIF_STATE_CONNECTED;
		xenbus_device_resume(sc->sc_xbusd);
		hypervisor_unmask_event(sc->sc_evtchn);
		xenbus_switch_state(sc->sc_xbusd, NULL, XenbusStateConnected);
	}

	return true;

abort_resume:
	xenbus_dev_fatal(sc->sc_xbusd, error, "resuming device");
	return false;

abort_transaction:
	xenbus_transaction_end(xbt, 1);
	xenbus_dev_fatal(sc->sc_xbusd, error, "%s", errmsg);
	return false;
}

static void
xbd_backend_changed(void *arg, XenbusState new_state)
{
	struct xbd_xenbus_softc *sc = device_private((device_t)arg);
	struct disk_geom *dg;

	char buf[64];
	DPRINTF(("%s: new backend state %d\n",
	    device_xname(sc->sc_dksc.sc_dev), new_state));

	switch (new_state) {
	case XenbusStateUnknown:
	case XenbusStateInitialising:
	case XenbusStateInitWait:
	case XenbusStateInitialised:
		break;
	case XenbusStateClosing:
		mutex_enter(&sc->sc_lock);
		if (sc->sc_shutdown == BLKIF_SHUTDOWN_RUN)
			sc->sc_shutdown = BLKIF_SHUTDOWN_REMOTE;
		/* wait for requests to complete */
		while (sc->sc_backend_status == BLKIF_STATE_CONNECTED &&
		    disk_isbusy(&sc->sc_dksc.sc_dkdev)) {
			cv_timedwait(&sc->sc_detach_cv, &sc->sc_lock, hz/2);
		}
		mutex_exit(&sc->sc_lock);
		xenbus_switch_state(sc->sc_xbusd, NULL, XenbusStateClosed);
		break;
	case XenbusStateConnected:
		/*
		 * note that xbd_backend_changed() can only be called by
		 * the xenbus thread.
		 */

		if (sc->sc_backend_status == BLKIF_STATE_CONNECTED ||
		    sc->sc_backend_status == BLKIF_STATE_SUSPENDED)
			/* already connected */
			return;

		xbd_connect(sc);
		sc->sc_shutdown = BLKIF_SHUTDOWN_RUN;
		sc->sc_xbdsize =
		    sc->sc_sectors * (uint64_t)XEN_BSIZE / DEV_BSIZE;
		dg = &sc->sc_dksc.sc_dkdev.dk_geom;
		memset(dg, 0, sizeof(*dg));

		dg->dg_secperunit = sc->sc_xbdsize;
		dg->dg_secsize = DEV_BSIZE;
		dg->dg_ntracks = 1;
		// XXX: Ok to hard-code DEV_BSIZE?
		dg->dg_nsectors = 1024 * (1024 / dg->dg_secsize);
		dg->dg_ncylinders = dg->dg_secperunit / dg->dg_nsectors;

		bufq_alloc(&sc->sc_dksc.sc_bufq, "fcfs", 0);
		dk_attach(&sc->sc_dksc);
		disk_attach(&sc->sc_dksc.sc_dkdev);

		sc->sc_backend_status = BLKIF_STATE_CONNECTED;
		hypervisor_unmask_event(sc->sc_evtchn);

		format_bytes(buf, uimin(9, sizeof(buf)),
		    sc->sc_sectors * XEN_BSIZE);
		aprint_normal_dev(sc->sc_dksc.sc_dev,
				"%s, %d bytes/sect x %" PRIu64 " sectors\n",
				buf, (int)dg->dg_secsize, sc->sc_xbdsize);
		snprintb(buf, sizeof(buf), BLKIF_FEATURE_BITS,
		    sc->sc_features);
		aprint_normal_dev(sc->sc_dksc.sc_dev,
		    "backend features %s\n", buf);

		/* Discover wedges on this disk. */
		dkwedge_discover(&sc->sc_dksc.sc_dkdev);

		disk_set_info(sc->sc_dksc.sc_dev, &sc->sc_dksc.sc_dkdev, NULL);

		/* the disk should be working now */
		config_pending_decr(sc->sc_dksc.sc_dev);
		break;
	default:
		panic("bad backend state %d", new_state);
	}
}

static void
xbd_connect(struct xbd_xenbus_softc *sc)
{
	int err;
	unsigned long long sectors;
	u_long val;

	/*
	 * Must read feature-persistent later, e.g. Linux Dom0 writes
	 * this together with the device info.
	 */
	err = xenbus_read_ul(NULL, sc->sc_xbusd->xbusd_otherend,
	    "feature-persistent", &val, 10);
	if (err)
		val = 0;
	if (val > 0)
		sc->sc_features |= BLKIF_FEATURE_PERSISTENT;

	err = xenbus_read_ul(NULL,
	    sc->sc_xbusd->xbusd_path, "virtual-device", &sc->sc_handle, 10);
	if (err)
		panic("%s: can't read number from %s/virtual-device\n",
		    device_xname(sc->sc_dksc.sc_dev),
		    sc->sc_xbusd->xbusd_otherend);
	err = xenbus_read_ull(NULL,
	    sc->sc_xbusd->xbusd_otherend, "sectors", &sectors, 10);
	if (err)
		panic("%s: can't read number from %s/sectors\n",
		    device_xname(sc->sc_dksc.sc_dev),
		    sc->sc_xbusd->xbusd_otherend);
	sc->sc_sectors = sectors;

	err = xenbus_read_ul(NULL,
	    sc->sc_xbusd->xbusd_otherend, "info", &sc->sc_info, 10);
	if (err)
		panic("%s: can't read number from %s/info\n",
		    device_xname(sc->sc_dksc.sc_dev),
		    sc->sc_xbusd->xbusd_otherend);
	err = xenbus_read_ul(NULL,
	    sc->sc_xbusd->xbusd_otherend, "sector-size", &sc->sc_secsize, 10);
	if (err)
		panic("%s: can't read number from %s/sector-size\n",
		    device_xname(sc->sc_dksc.sc_dev),
		    sc->sc_xbusd->xbusd_otherend);

	xenbus_switch_state(sc->sc_xbusd, NULL, XenbusStateConnected);
}

static void
xbd_features(struct xbd_xenbus_softc *sc)
{
	int err;
	u_long val;

	err = xenbus_read_ul(NULL, sc->sc_xbusd->xbusd_otherend,
	    "feature-flush-cache", &val, 10);
	if (err)
		val = 0;
	if (val > 0)
		sc->sc_features |= BLKIF_FEATURE_CACHE_FLUSH;

	err = xenbus_read_ul(NULL, sc->sc_xbusd->xbusd_otherend,
	    "feature-barrier", &val, 10);
	if (err)
		val = 0;
	if (val > 0)
		sc->sc_features |= BLKIF_FEATURE_BARRIER;

	err = xenbus_read_ul(NULL, sc->sc_xbusd->xbusd_otherend,
	    "feature-max-indirect-segments", &val, 10);
	if (err)
		val = 0;
	if (val >= (MAXPHYS >> PAGE_SHIFT) + 1) {
		/* We can use indirect segments, the limit is big enough */
		sc->sc_features |= BLKIF_FEATURE_INDIRECT;
	}
}

static int
xbd_handler(void *arg)
{
	struct xbd_xenbus_softc *sc = arg;
	struct buf *bp;
	RING_IDX resp_prod, i;
	int more_to_do;
	int seg;
	grant_ref_t gntref;

	DPRINTF(("xbd_handler(%s)\n", device_xname(sc->sc_dksc.sc_dev)));

	if (__predict_false(sc->sc_backend_status != BLKIF_STATE_CONNECTED))
		return 0;

	mutex_enter(&sc->sc_lock);
again:
	resp_prod = sc->sc_ring.sring->rsp_prod;
	xen_rmb(); /* ensure we see replies up to resp_prod */
	for (i = sc->sc_ring.rsp_cons; i != resp_prod; i++) {
		blkif_response_t *rep = RING_GET_RESPONSE(&sc->sc_ring, i);
		struct xbd_req *xbdreq = &sc->sc_reqs[rep->id];

		if (rep->operation == BLKIF_OP_FLUSH_DISKCACHE) {
			KASSERT(xbdreq->req_bp == NULL);
			xbdreq->req_sync.s_error = rep->status;
			xbdreq->req_sync.s_done = 1;
			cv_broadcast(&sc->sc_cache_flush_cv);
			/* caller will free the req */
			continue;
		}

		if (rep->operation != BLKIF_OP_READ &&
		    rep->operation != BLKIF_OP_WRITE) {
			aprint_error_dev(sc->sc_dksc.sc_dev,
			    "bad operation %d from backend\n", rep->operation);
			continue;
		}

		bp = xbdreq->req_bp;
		xbdreq->req_bp = NULL;
		KASSERT(bp != NULL && bp->b_data != NULL);
		DPRINTF(("%s(%p): b_bcount = %ld\n", __func__,
		    bp, (long)bp->b_bcount));

		if (bp->b_error != 0 || rep->status != BLKIF_RSP_OKAY) {
			bp->b_error = EIO;
			bp->b_resid = bp->b_bcount;
		}

		if (xbdreq->req_parent) {
			struct xbd_req *req_parent = xbdreq->req_parent;

			/* Unhook and recycle child */
			xbdreq->req_parent = NULL;
			req_parent->req_child = NULL;
			SLIST_INSERT_HEAD(&sc->sc_xbdreq_head, xbdreq,
				    req_next);

			if (!req_parent->req_parent_done) {
				/* Finished before parent, nothig else to do */
				continue;
			}

			/* Must do the cleanup now */
			xbdreq = req_parent;
		}
		if (xbdreq->req_child) {
			/* Finished before child, child will cleanup */
			xbdreq->req_parent_done = true;
			continue;
		}

		if (bp->b_error == 0)
			bp->b_resid = 0;

		KASSERT(xbdreq->req_dmamap->dm_nsegs > 0);
		for (seg = 0; seg < xbdreq->req_dmamap->dm_nsegs; seg++) {
			/*
			 * We are not allowing persistent mappings, so
			 * expect the backend to release the grant
			 * immediately.
			 */
			if (xbdreq->req_indirect) {
				gntref =
				    xbdreq->req_indirect->in_addr[seg].gref;
			} else
				gntref = xbdreq->req_gntref[seg];
			KASSERT(xengnt_status(gntref) == 0);
			xengnt_revoke_access(gntref);
		}

		bus_dmamap_unload(sc->sc_xbusd->xbusd_dmat, xbdreq->req_dmamap);

		if (__predict_false(bp->b_data != xbdreq->req_data))
			xbd_unmap_align(sc, xbdreq, bp);
		xbdreq->req_data = NULL;

		dk_done(&sc->sc_dksc, bp);

		if (xbdreq->req_indirect) {
			/* No persistent mappings, so check that
			 * backend unmapped the indirect segment grant too.
			 */
			KASSERT(xengnt_status(xbdreq->req_indirect->in_gntref)
			    == 0);
			SLIST_INSERT_HEAD(&sc->sc_indirect_head,
			    xbdreq->req_indirect, in_next);
			xbdreq->req_indirect = NULL;
		}
		SLIST_INSERT_HEAD(&sc->sc_xbdreq_head, xbdreq, req_next);
	}
	sc->sc_ring.rsp_cons = i;

	xen_wmb();
	RING_FINAL_CHECK_FOR_RESPONSES(&sc->sc_ring, more_to_do);
	if (more_to_do)
		goto again;

	cv_signal(&sc->sc_req_cv);
	mutex_exit(&sc->sc_lock);

	dk_start(&sc->sc_dksc, NULL);

	return 1;
}

static void
xbdminphys(struct buf *bp)
{
	if (bp->b_bcount > XBD_XFER_LIMIT) {
		bp->b_bcount = XBD_XFER_LIMIT;
	}
	minphys(bp);
}

static void
xbd_iosize(device_t dev, int *maxxfer)
{
	/*
	 * Always restrict dumps to XBD_MAX_XFER to avoid indirect segments,
	 * so that it uses as little memory as possible.
	 */
	if (*maxxfer > XBD_MAX_XFER)
		*maxxfer = XBD_MAX_XFER;
}

static int
xbdopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct	xbd_xenbus_softc *sc;

	sc = device_lookup_private(&xbd_cd, DISKUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if ((flags & FWRITE) && (sc->sc_info & VDISK_READONLY))
		return EROFS;

	DPRINTF(("xbdopen(%" PRIx64 ", %d)\n", dev, flags));
	return dk_open(&sc->sc_dksc, dev, flags, fmt, l);
}

static int
xbdclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct xbd_xenbus_softc *sc;

	sc = device_lookup_private(&xbd_cd, DISKUNIT(dev));

	DPRINTF(("xbdclose(%" PRIx64 ", %d)\n", dev, flags));
	return dk_close(&sc->sc_dksc, dev, flags, fmt, l);
}

static void
xbdstrategy(struct buf *bp)
{
	struct xbd_xenbus_softc *sc;

	sc = device_lookup_private(&xbd_cd, DISKUNIT(bp->b_dev));

	DPRINTF(("xbdstrategy(%p): b_bcount = %ld\n", bp,
	    (long)bp->b_bcount));

	if (sc == NULL || sc->sc_shutdown != BLKIF_SHUTDOWN_RUN) {
		bp->b_error = EIO;
		biodone(bp);
		return;
	}
	if (__predict_false((sc->sc_info & VDISK_READONLY) &&
	    (bp->b_flags & B_READ) == 0)) {
		bp->b_error = EROFS;
		biodone(bp);
		return;
	}

	dk_strategy(&sc->sc_dksc, bp);
	return;
}

static int
xbdsize(dev_t dev)
{
	struct	xbd_xenbus_softc *sc;

	DPRINTF(("xbdsize(%" PRIx64 ")\n", dev));

	sc = device_lookup_private(&xbd_cd, DISKUNIT(dev));
	if (sc == NULL || sc->sc_shutdown != BLKIF_SHUTDOWN_RUN)
		return -1;
	return dk_size(&sc->sc_dksc, dev);
}

static int
xbdread(dev_t dev, struct uio *uio, int flags)
{
	struct xbd_xenbus_softc *sc =
	    device_lookup_private(&xbd_cd, DISKUNIT(dev));
	struct  dk_softc *dksc = &sc->sc_dksc;

	if (!DK_ATTACHED(dksc))
		return ENXIO;
	return physio(xbdstrategy, NULL, dev, B_READ, xbdminphys, uio);
}

static int
xbdwrite(dev_t dev, struct uio *uio, int flags)
{
	struct xbd_xenbus_softc *sc =
	    device_lookup_private(&xbd_cd, DISKUNIT(dev));
	struct  dk_softc *dksc = &sc->sc_dksc;

	if (!DK_ATTACHED(dksc))
		return ENXIO;
	if (__predict_false(sc->sc_info & VDISK_READONLY))
		return EROFS;
	return physio(xbdstrategy, NULL, dev, B_WRITE, xbdminphys, uio);
}

static int
xbdioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct xbd_xenbus_softc *sc =
	    device_lookup_private(&xbd_cd, DISKUNIT(dev));
	struct	dk_softc *dksc;
	int	error;
	struct xbd_req *xbdreq;
	blkif_request_t *req;
	int notify;

	DPRINTF(("xbdioctl(%" PRIx64 ", %08lx, %p, %d, %p)\n",
	    dev, cmd, data, flag, l));
	dksc = &sc->sc_dksc;

	switch (cmd) {
	case DIOCGCACHE:
	    {
		/* Assume there is write cache if cache-flush is supported */
		int *bitsp = (int *)data;
		*bitsp = 0;
		if (sc->sc_features & BLKIF_FEATURE_CACHE_FLUSH)
			*bitsp |= DKCACHE_WRITE;
		error = 0;
		break;
	    }
	case DIOCCACHESYNC:
		if ((sc->sc_features & BLKIF_FEATURE_CACHE_FLUSH) == 0)
			return EOPNOTSUPP;

		mutex_enter(&sc->sc_lock);
		while ((xbdreq = SLIST_FIRST(&sc->sc_xbdreq_head)) == NULL)
			cv_wait(&sc->sc_req_cv, &sc->sc_lock);
		KASSERT(!RING_FULL(&sc->sc_ring));

		SLIST_REMOVE_HEAD(&sc->sc_xbdreq_head, req_next);
		req = RING_GET_REQUEST(&sc->sc_ring,
		    sc->sc_ring.req_prod_pvt);
		req->id = xbdreq->req_id;
		req->operation = BLKIF_OP_FLUSH_DISKCACHE;
		req->handle = sc->sc_handle;
		xbdreq->req_sync.s_done = 0;
		sc->sc_ring.req_prod_pvt++;
		RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&sc->sc_ring, notify);
		if (notify)
			hypervisor_notify_via_evtchn(sc->sc_evtchn);
		/* request sent, now wait for completion */
		while (xbdreq->req_sync.s_done == 0)
			cv_wait(&sc->sc_cache_flush_cv, &sc->sc_lock);

		if (xbdreq->req_sync.s_error == BLKIF_RSP_EOPNOTSUPP)
			error = EOPNOTSUPP;
		else if (xbdreq->req_sync.s_error == BLKIF_RSP_OKAY)
			error = 0;
		else
			error = EIO;
		SLIST_INSERT_HEAD(&sc->sc_xbdreq_head, xbdreq, req_next);
		cv_signal(&sc->sc_req_cv);
		mutex_exit(&sc->sc_lock);

		/* Restart I/O if it was waiting for req */
		dk_start(&sc->sc_dksc, NULL);
		break;

	default:
		error = dk_ioctl(dksc, dev, cmd, data, flag, l);
		break;
	}

	return error;
}

static int
xbddump(dev_t dev, daddr_t blkno, void *va, size_t size)
{
	struct xbd_xenbus_softc *sc;

	sc  = device_lookup_private(&xbd_cd, DISKUNIT(dev));
	if (sc == NULL)
		return (ENXIO);

	DPRINTF(("xbddump(%" PRIx64 ", %" PRId64 ", %p, %lu)\n", dev, blkno, va,
	    (unsigned long)size));
	return dk_dump(&sc->sc_dksc, dev, blkno, va, size, 0);
}

static int
xbd_diskstart(device_t self, struct buf *bp)
{
	struct xbd_xenbus_softc *sc = device_private(self);
	struct xbd_req *xbdreq;
	int error = 0;
	int notify;

	KASSERT(bp->b_bcount <= MAXPHYS);

	DPRINTF(("xbd_diskstart(%p): b_bcount = %ld\n",
	    bp, (long)bp->b_bcount));

	mutex_enter(&sc->sc_lock);

	if (sc->sc_shutdown != BLKIF_SHUTDOWN_RUN) {
		error = EIO;
		goto out;
	}

	if (bp->b_rawblkno < 0 || bp->b_rawblkno > sc->sc_xbdsize) {
		/* invalid block number */
		error = EINVAL;
		goto out;
	}

	if (__predict_false(
	    sc->sc_backend_status == BLKIF_STATE_SUSPENDED)) {
		/* device is suspended, do not consume buffer */
		DPRINTF(("%s: (xbd_diskstart) device suspended\n",
		    sc->sc_dksc.sc_xname));
		error = EAGAIN;
		goto out;
	}

	xbdreq = SLIST_FIRST(&sc->sc_xbdreq_head);
	if (__predict_false(xbdreq == NULL)) {
		sc->sc_cnt_queue_full.ev_count++;
		DPRINTF(("xbd_diskstart: no req\n"));
		error = EAGAIN;
		goto out;
	}
	KASSERT(!RING_FULL(&sc->sc_ring));

	if ((sc->sc_features & BLKIF_FEATURE_INDIRECT) == 0
	    && bp->b_bcount > XBD_MAX_CHUNK) {
		if (!SLIST_NEXT(xbdreq, req_next)) {
			DPRINTF(("%s: need extra req\n", __func__));
			error = EAGAIN;
			goto out;
		}
	}

	bp->b_resid = bp->b_bcount;
	xbdreq->req_bp = bp;
	xbdreq->req_data = bp->b_data;
	if (__predict_false((vaddr_t)bp->b_data & (XEN_BSIZE - 1))) {
		if (__predict_false(xbd_map_align(sc, xbdreq) != 0)) {
			DPRINTF(("xbd_diskstart: no align\n"));
			error = EAGAIN;
			goto out;
		}
	}

	if (__predict_false(bus_dmamap_load(sc->sc_xbusd->xbusd_dmat,
	    xbdreq->req_dmamap, xbdreq->req_data, bp->b_bcount, NULL,
	    BUS_DMA_NOWAIT) != 0)) {
		printf("%s: %s: bus_dmamap_load failed\n",
		    device_xname(sc->sc_dksc.sc_dev), __func__);
		if (__predict_false(bp->b_data != xbdreq->req_data))
			xbd_unmap_align(sc, xbdreq, NULL);
		error = EINVAL;
		goto out;
	}
	KASSERTMSG(xbdreq->req_dmamap->dm_nsegs > 0,
	    "dm_nsegs == 0 with bcount %d", bp->b_bcount);

	for (int seg = 0; seg < xbdreq->req_dmamap->dm_nsegs; seg++) {
		KASSERT(seg < __arraycount(xbdreq->req_gntref));

		paddr_t ma = xbdreq->req_dmamap->dm_segs[seg].ds_addr;
		if (__predict_false(xengnt_grant_access(
		    sc->sc_xbusd->xbusd_otherend_id,
		    (ma & ~PAGE_MASK), (bp->b_flags & B_READ) == 0,
		    &xbdreq->req_gntref[seg]))) {
			printf("%s: %s: xengnt_grant_access failed\n",
			    device_xname(sc->sc_dksc.sc_dev), __func__);
			if (seg > 0) {
				for (; --seg >= 0; ) {
					xengnt_revoke_access(
					    xbdreq->req_gntref[seg]);
				}
			}
			bus_dmamap_unload(sc->sc_xbusd->xbusd_dmat,
			    xbdreq->req_dmamap);
			if (__predict_false(bp->b_data != xbdreq->req_data))
				xbd_unmap_align(sc, xbdreq, NULL);
			error = EAGAIN;
			goto out;
		}
	}

	KASSERT(xbdreq->req_parent == NULL);
	KASSERT(xbdreq->req_child == NULL);

	/* We are now committed to the transfer */
	SLIST_REMOVE_HEAD(&sc->sc_xbdreq_head, req_next);

	if ((sc->sc_features & BLKIF_FEATURE_INDIRECT) != 0 &&
	    bp->b_bcount > XBD_MAX_CHUNK) {
		xbd_diskstart_submit_indirect(sc, xbdreq, bp);
		goto push;
	}

	xbd_diskstart_submit(sc, xbdreq->req_id,
	    bp, 0, xbdreq->req_dmamap, xbdreq->req_gntref);

	if (bp->b_bcount > XBD_MAX_CHUNK) {
		KASSERT(!RING_FULL(&sc->sc_ring));
		struct xbd_req *xbdreq2 = SLIST_FIRST(&sc->sc_xbdreq_head);
		KASSERT(xbdreq2 != NULL); /* Checked earlier */
		SLIST_REMOVE_HEAD(&sc->sc_xbdreq_head, req_next);
		xbdreq->req_child = xbdreq2;
		xbdreq->req_parent_done = false;
		xbdreq2->req_parent = xbdreq;
		xbdreq2->req_bp = bp;
		xbdreq2->req_data = xbdreq->req_data;
		xbd_diskstart_submit(sc, xbdreq2->req_id,
		    bp, XBD_MAX_CHUNK, xbdreq->req_dmamap,
		    xbdreq->req_gntref);
	}

push:
	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&sc->sc_ring, notify);
	if (notify)
		hypervisor_notify_via_evtchn(sc->sc_evtchn);
out:
	mutex_exit(&sc->sc_lock);
	return error;
}

static void
xbd_diskstart_submit(struct xbd_xenbus_softc *sc,
    int req_id, struct buf *bp, int start, bus_dmamap_t dmamap,
    grant_ref_t *gntref)
{
	blkif_request_t *req;
	paddr_t ma;
	int nsects, nbytes, dmaseg, first_sect, size, segidx = 0;
	struct blkif_request_segment *reqseg;

	KASSERT(mutex_owned(&sc->sc_lock));

	req = RING_GET_REQUEST(&sc->sc_ring, sc->sc_ring.req_prod_pvt);
	req->id = req_id;
	req->operation =
	    bp->b_flags & B_READ ? BLKIF_OP_READ : BLKIF_OP_WRITE;
	req->sector_number = bp->b_rawblkno + (start >> XEN_BSHIFT);
	req->handle = sc->sc_handle;

	size = uimin(bp->b_bcount - start, XBD_MAX_CHUNK);
	for (dmaseg = 0; dmaseg < dmamap->dm_nsegs && size > 0; dmaseg++) {
		bus_dma_segment_t *ds = &dmamap->dm_segs[dmaseg];

		ma = ds->ds_addr;
		nbytes = ds->ds_len;

		if (start > 0) {
			if (start >= nbytes) {
				start -= nbytes;
				continue;
			}
			ma += start;
			nbytes -= start;
			start = 0;
		}
		size -= nbytes;

		KASSERT(((ma & PAGE_MASK) & (XEN_BSIZE - 1)) == 0);
		KASSERT((nbytes & (XEN_BSIZE - 1)) == 0);
		KASSERT((size & (XEN_BSIZE - 1)) == 0);
		first_sect = (ma & PAGE_MASK) >> XEN_BSHIFT;
		nsects = nbytes >> XEN_BSHIFT;

		reqseg = &req->seg[segidx++];
		reqseg->first_sect = first_sect;
		reqseg->last_sect = first_sect + nsects - 1;
		KASSERT(reqseg->first_sect <= reqseg->last_sect);
		KASSERT(reqseg->last_sect < (PAGE_SIZE / XEN_BSIZE));

		reqseg->gref = gntref[dmaseg];
	}
	KASSERT(segidx > 0);
	req->nr_segments = segidx;
	sc->sc_ring.req_prod_pvt++;
}

static void
xbd_diskstart_submit_indirect(struct xbd_xenbus_softc *sc,
    struct xbd_req *xbdreq, struct buf *bp)
{
	blkif_request_indirect_t *req;
	paddr_t ma;
	int nsects, nbytes, dmaseg, first_sect;
	struct blkif_request_segment *reqseg;

	KASSERT(mutex_owned(&sc->sc_lock));

	req = (blkif_request_indirect_t *)RING_GET_REQUEST(&sc->sc_ring,
	    sc->sc_ring.req_prod_pvt);
	req->id = xbdreq->req_id;
	req->operation = BLKIF_OP_INDIRECT;
	req->indirect_op =
	    bp->b_flags & B_READ ? BLKIF_OP_READ : BLKIF_OP_WRITE;
	req->sector_number = bp->b_rawblkno;
	req->handle = sc->sc_handle;

	xbdreq->req_indirect = SLIST_FIRST(&sc->sc_indirect_head);
	KASSERT(xbdreq->req_indirect != NULL);	/* always as many as reqs */
	SLIST_REMOVE_HEAD(&sc->sc_indirect_head, in_next);
	req->indirect_grefs[0] = xbdreq->req_indirect->in_gntref;

	reqseg = xbdreq->req_indirect->in_addr;
	for (dmaseg = 0; dmaseg < xbdreq->req_dmamap->dm_nsegs; dmaseg++) {
		bus_dma_segment_t *ds = &xbdreq->req_dmamap->dm_segs[dmaseg];

		ma = ds->ds_addr;
		nbytes = ds->ds_len;

		first_sect = (ma & PAGE_MASK) >> XEN_BSHIFT;
		nsects = nbytes >> XEN_BSHIFT;

		reqseg->first_sect = first_sect;
		reqseg->last_sect = first_sect + nsects - 1;
		reqseg->gref = xbdreq->req_gntref[dmaseg];

		KASSERT(reqseg->first_sect <= reqseg->last_sect);
		KASSERT(reqseg->last_sect < (PAGE_SIZE / XEN_BSIZE));

		reqseg++;
	}
	req->nr_segments = dmaseg;
	sc->sc_ring.req_prod_pvt++;

	sc->sc_cnt_indirect.ev_count++;
}

static int
xbd_map_align(struct xbd_xenbus_softc *sc, struct xbd_req *req)
{
	/*
	 * Only can get here if this is physio() request, block I/O
	 * uses DEV_BSIZE-aligned buffers.
	 */
	KASSERT((req->req_bp->b_flags & B_PHYS) != 0);

	sc->sc_cnt_map_unalign.ev_count++;

	if (sc->sc_unalign_used) {
		sc->sc_cnt_unalign_busy.ev_count++;
		return EAGAIN;
	}
	sc->sc_unalign_used = req->req_bp;

	KASSERT(req->req_bp->b_bcount <= MAXPHYS);
	req->req_data = (void *)sc->sc_unalign_buffer;
	if ((req->req_bp->b_flags & B_READ) == 0)
		memcpy(req->req_data, req->req_bp->b_data,
		    req->req_bp->b_bcount);
	return 0;
}

static void
xbd_unmap_align(struct xbd_xenbus_softc *sc, struct xbd_req *req,
    struct buf *bp)
{
	KASSERT(!bp || sc->sc_unalign_used == bp);
	if (bp && bp->b_flags & B_READ)
		memcpy(bp->b_data, req->req_data, bp->b_bcount);
	sc->sc_unalign_used = NULL;
}
