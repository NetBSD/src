/*      $NetBSD: xbd_xenbus.c,v 1.62.2.3 2014/08/10 06:54:11 tls Exp $      */

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
 * - the request is ultimately processed by xbdstart() that prepares the
 *   xbd requests, post them in the ring I/O queue, then signal the backend.
 *
 * When a response is available in the queue, the backend signals the frontend
 * via its event channel. This triggers xbd_handler(), which will link back
 * the response to its request through the request ID, and mark the I/O as
 * completed.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xbd_xenbus.c,v 1.62.2.3 2014/08/10 06:54:11 tls Exp $");

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

#include <dev/dkvar.h>

#include <uvm/uvm.h>

#include <sys/rnd.h>

#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/granttables.h>
#include <xen/xen-public/io/blkif.h>
#include <xen/xen-public/io/protocols.h>

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

#define XEN_BSHIFT      9               /* log2(XEN_BSIZE) */
#define XEN_BSIZE       (1 << XEN_BSHIFT) 

struct xbd_req {
	SLIST_ENTRY(xbd_req) req_next;
	uint16_t req_id; /* ID passed to backend */
	union {
	    struct {
		grant_ref_t req_gntref[BLKIF_MAX_SEGMENTS_PER_REQUEST];
		int req_nr_segments; /* number of segments in this request */
		struct buf *req_bp; /* buffer associated with this request */
		void *req_data; /* pointer to the data buffer */
	    } req_rw;
	    struct {
		int s_error;
		volatile int s_done;
	    } req_sync;
	} u;
};
#define req_gntref	u.req_rw.req_gntref
#define req_nr_segments	u.req_rw.req_nr_segments
#define req_bp		u.req_rw.req_bp
#define req_data	u.req_rw.req_data
#define req_sync	u.req_sync

struct xbd_xenbus_softc {
	struct dk_softc sc_dksc;	/* Must be first in this struct */
	struct xenbus_device *sc_xbusd;

	blkif_front_ring_t sc_ring;

	unsigned int sc_evtchn;

	grant_ref_t sc_ring_gntref;

	struct xbd_req sc_reqs[XBD_RING_SIZE];
	SLIST_HEAD(,xbd_req) sc_xbdreq_head; /* list of free requests */
	bool sc_xbdreq_wait; /* special waiting on xbd_req */

	int sc_backend_status; /* our status with backend */
#define BLKIF_STATE_DISCONNECTED 0
#define BLKIF_STATE_CONNECTED    1
#define BLKIF_STATE_SUSPENDED    2

	int sc_shutdown;
#define BLKIF_SHUTDOWN_RUN    0 /* no shutdown */
#define BLKIF_SHUTDOWN_REMOTE 1 /* backend-initiated shutdown in progress */
#define BLKIF_SHUTDOWN_LOCAL  2 /* locally-initiated shutdown in progress */

	uint64_t sc_sectors; /* number of sectors for this device */
	u_long sc_secsize; /* sector size */
	uint64_t sc_xbdsize; /* size of disk in DEV_BSIZE */
	u_long sc_info; /* VDISK_* */
	u_long sc_handle; /* from backend */
	int sc_cache_flush; /* backend supports BLKIF_OP_FLUSH_DISKCACHE */
	krndsource_t     sc_rnd_source;
};

#if 0
/* too big to be on stack */
static multicall_entry_t rq_mcl[XBD_RING_SIZE+1];
static paddr_t rq_pages[XBD_RING_SIZE];
#endif

static int  xbd_xenbus_match(device_t, cfdata_t, void *);
static void xbd_xenbus_attach(device_t, device_t, void *);
static int  xbd_xenbus_detach(device_t, int);

static bool xbd_xenbus_suspend(device_t, const pmf_qual_t *);
static bool xbd_xenbus_resume(device_t, const pmf_qual_t *);

static int  xbd_handler(void *);
static void xbdstart(struct dk_softc *);
static void xbd_backend_changed(void *, XenbusState);
static void xbd_connect(struct xbd_xenbus_softc *);

static int  xbd_map_align(struct xbd_req *);
static void xbd_unmap_align(struct xbd_req *);

static void xbdminphys(struct buf *);

CFATTACH_DECL3_NEW(xbd, sizeof(struct xbd_xenbus_softc),
    xbd_xenbus_match, xbd_xenbus_attach, xbd_xenbus_detach, NULL, NULL, NULL,
    DVF_DETACH_SHUTDOWN);

dev_type_open(xbdopen);
dev_type_close(xbdclose);
dev_type_read(xbdread);
dev_type_write(xbdwrite);
dev_type_ioctl(xbdioctl);
dev_type_strategy(xbdstrategy);
dev_type_dump(xbddump);
dev_type_size(xbdsize);

const struct bdevsw xbd_bdevsw = {
	.d_open = xbdopen,
	.d_close = xbdclose,
	.d_strategy = xbdstrategy,
	.d_ioctl = xbdioctl,
	.d_dump = xbddump,
	.d_psize = xbdsize,
	.d_discard = nodiscard,
	.d_flag = D_DISK
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
	.d_flag = D_DISK
};

extern struct cfdriver xbd_cd;

/* Pseudo-disk Interface */
static struct dk_intf dkintf_esdi = {
        DTYPE_ESDI,
	"Xen Virtual ESDI",
	xbdopen,
	xbdclose,
	xbdstrategy,
	xbdstart,
}, *di = &dkintf_esdi;

static struct dkdriver xbddkdriver = {
        .d_strategy = xbdstrategy,
	.d_minphys = xbdminphys,
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
#ifdef XBD_DEBUG
	char **dir, *val;
	int dir_n = 0;
	char id_str[20];
	int err;
#endif

	config_pending_incr(self);
	aprint_normal(": Xen Virtual Block Device Interface\n");

	dk_sc_init(&sc->sc_dksc, device_xname(self));
	sc->sc_dksc.sc_dev = self;

#ifdef XBD_DEBUG
	printf("path: %s\n", xa->xa_xbusd->xbusd_path);
	snprintf(id_str, sizeof(id_str), "%d", xa->xa_id);
	err = xenbus_directory(NULL, "device/vbd", id_str, &dir_n, &dir);
	if (err) {
		aprint_error_dev(self, "xenbus_directory err %d\n", err);
	} else {
		printf("%s/\n", xa->xa_xbusd->xbusd_path);
		for (i = 0; i < dir_n; i++) {
			printf("\t/%s", dir[i]);
			err = xenbus_read(NULL, xa->xa_xbusd->xbusd_path,
					  dir[i], NULL, &val);
			if (err) {
				aprint_error_dev(self, "xenbus_read err %d\n",
						 err);
			} else {
				printf(" = %s\n", val);
				free(val, M_DEVBUF);
			}
		}
	}
#endif /* XBD_DEBUG */
	sc->sc_xbusd = xa->xa_xbusd;
	sc->sc_xbusd->xbusd_otherend_changed = xbd_backend_changed;

	disk_init(&sc->sc_dksc.sc_dkdev, device_xname(self), &xbddkdriver);
	/* initialize free requests list */
	SLIST_INIT(&sc->sc_xbdreq_head);
	for (i = 0; i < XBD_RING_SIZE; i++) {
		sc->sc_reqs[i].req_id = i;
		SLIST_INSERT_HEAD(&sc->sc_xbdreq_head, &sc->sc_reqs[i],
		    req_next);
	}

	sc->sc_backend_status = BLKIF_STATE_DISCONNECTED;
	sc->sc_shutdown = BLKIF_SHUTDOWN_REMOTE;

	ring = (void *)uvm_km_alloc(kernel_map, PAGE_SIZE, 0, UVM_KMF_WIRED);
	if (ring == NULL)
		panic("%s: can't alloc ring", device_xname(self));
	sc->sc_ring.sring = ring;

	/* resume shared structures and tell backend that we are ready */
	if (xbd_xenbus_resume(self, PMF_Q_NONE) == false) {
		uvm_km_free(kernel_map, (vaddr_t)ring, PAGE_SIZE,
		    UVM_KMF_WIRED);
		return;
	}

	rnd_attach_source(&sc->sc_rnd_source, device_xname(self),
	    RND_TYPE_DISK, RND_FLAG_DEFAULT);

	if (!pmf_device_register(self, xbd_xenbus_suspend, xbd_xenbus_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

}

static int
xbd_xenbus_detach(device_t dev, int flags)
{
	struct xbd_xenbus_softc *sc = device_private(dev);
	int bmaj, cmaj, i, mn, rc, s;

	rc = disk_begindetach(&sc->sc_dksc.sc_dkdev, NULL, dev, flags);
	if (rc != 0)
		return rc;

	s = splbio();
	DPRINTF(("%s: xbd_detach\n", device_xname(dev)));
	if (sc->sc_shutdown == BLKIF_SHUTDOWN_RUN) {
		sc->sc_shutdown = BLKIF_SHUTDOWN_LOCAL;
		/* wait for requests to complete */
		while (sc->sc_backend_status == BLKIF_STATE_CONNECTED &&
		    sc->sc_dksc.sc_dkdev.dk_stats->io_busy > 0)
			tsleep(xbd_xenbus_detach, PRIBIO, "xbddetach", hz/2);

		xenbus_switch_state(sc->sc_xbusd, NULL, XenbusStateClosing);
	}
	if ((flags & DETACH_FORCE) == 0) {
		/* xbd_xenbus_detach already in progress */
		wakeup(xbd_xenbus_detach);
		splx(s);
		return EALREADY;
	}
	while (xenbus_read_driver_state(sc->sc_xbusd->xbusd_otherend)
	    != XenbusStateClosed)
		tsleep(xbd_xenbus_detach, PRIBIO, "xbddetach2", hz/2);
	splx(s);

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

		s = splbio();
		/* Kill off any queued buffers. */
		bufq_drain(sc->sc_dksc.sc_bufq);
		bufq_free(sc->sc_dksc.sc_bufq);
		splx(s);

		/* detach disk */
		disk_detach(&sc->sc_dksc.sc_dkdev);
		disk_destroy(&sc->sc_dksc.sc_dkdev);
		/* Unhook the entropy source. */
		rnd_detach_source(&sc->sc_rnd_source);
	}

	hypervisor_mask_event(sc->sc_evtchn);
	event_remove_handler(sc->sc_evtchn, &xbd_handler, sc);
	while (xengnt_status(sc->sc_ring_gntref)) {
		tsleep(xbd_xenbus_detach, PRIBIO, "xbd_ref", hz/2);
	}
	xengnt_revoke_access(sc->sc_ring_gntref);
	uvm_km_free(kernel_map, (vaddr_t)sc->sc_ring.sring,
	    PAGE_SIZE, UVM_KMF_WIRED);

	pmf_device_deregister(dev);

	return 0;
}

static bool
xbd_xenbus_suspend(device_t dev, const pmf_qual_t *qual) {

	int s;
	struct xbd_xenbus_softc *sc;

	sc = device_private(dev);

	s = splbio();
	/* wait for requests to complete, then suspend device */
	while (sc->sc_backend_status == BLKIF_STATE_CONNECTED &&
	    sc->sc_dksc.sc_dkdev.dk_stats->io_busy > 0)
		tsleep(xbd_xenbus_suspend, PRIBIO, "xbdsuspend", hz/2);

	hypervisor_mask_event(sc->sc_evtchn);
	sc->sc_backend_status = BLKIF_STATE_SUSPENDED;
	event_remove_handler(sc->sc_evtchn, xbd_handler, sc);

	splx(s);

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

	if (sc->sc_backend_status == BLKIF_STATE_SUSPENDED) {
		/*
		 * Device was suspended, so ensure that access associated to
		 * the block I/O ring is revoked.
		 */
		xengnt_revoke_access(sc->sc_ring_gntref);
	}
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

	error = xenbus_alloc_evtchn(sc->sc_xbusd, &sc->sc_evtchn);
	if (error)
		goto abort_resume;

	aprint_verbose_dev(dev, "using event channel %d\n",
	    sc->sc_evtchn);
	event_set_handler(sc->sc_evtchn, &xbd_handler, sc,
	    IPL_BIO, device_xname(dev));

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
		hypervisor_enable_event(sc->sc_evtchn);
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

static void xbd_backend_changed(void *arg, XenbusState new_state)
{
	struct xbd_xenbus_softc *sc = device_private((device_t)arg);
	struct disk_geom *dg;

	char buf[9];
	int s;
	DPRINTF(("%s: new backend state %d\n",
	    device_xname(sc->sc_dksc.sc_dev), new_state));

	switch (new_state) {
	case XenbusStateUnknown:
	case XenbusStateInitialising:
	case XenbusStateInitWait:
	case XenbusStateInitialised:
		break;
	case XenbusStateClosing:
		s = splbio();
		if (sc->sc_shutdown == BLKIF_SHUTDOWN_RUN)
			sc->sc_shutdown = BLKIF_SHUTDOWN_REMOTE;
		/* wait for requests to complete */
		while (sc->sc_backend_status == BLKIF_STATE_CONNECTED &&
		    sc->sc_dksc.sc_dkdev.dk_stats->io_busy > 0)
			tsleep(xbd_xenbus_detach, PRIBIO, "xbddetach", hz/2);
		splx(s);
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
		hypervisor_enable_event(sc->sc_evtchn);

		sc->sc_xbdsize =
		    sc->sc_sectors * (uint64_t)sc->sc_secsize / DEV_BSIZE;
		dg = &sc->sc_dksc.sc_dkdev.dk_geom;
		memset(dg, 0, sizeof(*dg));	

		dg->dg_secperunit = sc->sc_xbdsize;
		dg->dg_secsize = DEV_BSIZE;
		dg->dg_ntracks = 1;
		// XXX: Ok to hard-code DEV_BSIZE?
		dg->dg_nsectors = 1024 * (1024 / dg->dg_secsize);
		dg->dg_ncylinders = dg->dg_secperunit / dg->dg_nsectors;

		bufq_alloc(&sc->sc_dksc.sc_bufq, "fcfs", 0);
		sc->sc_dksc.sc_flags |= DKF_INITED;
		disk_attach(&sc->sc_dksc.sc_dkdev);

		sc->sc_backend_status = BLKIF_STATE_CONNECTED;

		/* try to read the disklabel */
		dk_getdisklabel(di, &sc->sc_dksc, 0 /* XXX ? */);
		format_bytes(buf, sizeof(buf), sc->sc_sectors * sc->sc_secsize);
		aprint_verbose_dev(sc->sc_dksc.sc_dev,
				"%s, %d bytes/sect x %" PRIu64 " sectors\n",
				buf, (int)dg->dg_secsize, sc->sc_xbdsize);
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
	u_long cache_flush;

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

	err = xenbus_read_ul(NULL, sc->sc_xbusd->xbusd_otherend,
	    "feature-flush-cache", &cache_flush, 10);
	if (err)
		cache_flush = 0;
	if (cache_flush > 0)
		sc->sc_cache_flush = 1;
	else
		sc->sc_cache_flush = 0;

	xenbus_switch_state(sc->sc_xbusd, NULL, XenbusStateConnected);
}

static int
xbd_handler(void *arg)
{
	struct xbd_xenbus_softc *sc = arg;
	struct buf *bp;
	RING_IDX resp_prod, i;
	int more_to_do;
	int seg;

	DPRINTF(("xbd_handler(%s)\n", device_xname(sc->sc_dksc.sc_dev)));

	if (__predict_false(sc->sc_backend_status != BLKIF_STATE_CONNECTED))
		return 0;
again:
	resp_prod = sc->sc_ring.sring->rsp_prod;
	xen_rmb(); /* ensure we see replies up to resp_prod */
	for (i = sc->sc_ring.rsp_cons; i != resp_prod; i++) {
		blkif_response_t *rep = RING_GET_RESPONSE(&sc->sc_ring, i);
		struct xbd_req *xbdreq = &sc->sc_reqs[rep->id];
		DPRINTF(("xbd_handler(%p): b_bcount = %ld\n",
		    xbdreq->req_bp, (long)bp->b_bcount));
		bp = xbdreq->req_bp;
		if (rep->operation == BLKIF_OP_FLUSH_DISKCACHE) {
			xbdreq->req_sync.s_error = rep->status;
			xbdreq->req_sync.s_done = 1;
			wakeup(xbdreq);
			/* caller will free the req */
			continue;
		}
		for (seg = xbdreq->req_nr_segments - 1; seg >= 0; seg--) {
			if (__predict_false(
			    xengnt_status(xbdreq->req_gntref[seg]))) {
				aprint_verbose_dev(sc->sc_dksc.sc_dev,
					"grant still used by backend\n");
				sc->sc_ring.rsp_cons = i;
				xbdreq->req_nr_segments = seg + 1;
				goto done;
			}
			xengnt_revoke_access(xbdreq->req_gntref[seg]);
			xbdreq->req_nr_segments--;
		}
		if (rep->operation != BLKIF_OP_READ &&
		    rep->operation != BLKIF_OP_WRITE) {
			aprint_error_dev(sc->sc_dksc.sc_dev,
			    "bad operation %d from backend\n", rep->operation);
			bp->b_error = EIO;
			bp->b_resid = bp->b_bcount;
			goto next;
		}
		if (rep->status != BLKIF_RSP_OKAY) {
			bp->b_error = EIO;
			bp->b_resid = bp->b_bcount;
			goto next;
		}
		/* b_resid was set in xbdstart */
next:
		if (bp->b_data != xbdreq->req_data)
			xbd_unmap_align(xbdreq);
		disk_unbusy(&sc->sc_dksc.sc_dkdev,
		    (bp->b_bcount - bp->b_resid),
		    (bp->b_flags & B_READ));
		rnd_add_uint32(&sc->sc_rnd_source,
		    bp->b_blkno);
		biodone(bp);
		SLIST_INSERT_HEAD(&sc->sc_xbdreq_head, xbdreq, req_next);
	}
done:
	xen_rmb();
	sc->sc_ring.rsp_cons = i;

	RING_FINAL_CHECK_FOR_RESPONSES(&sc->sc_ring, more_to_do);
	if (more_to_do)
		goto again;

	if (sc->sc_xbdreq_wait)
		wakeup(&sc->sc_xbdreq_wait);
	else
		xbdstart(&sc->sc_dksc);
	return 1;
}

static void
xbdminphys(struct buf *bp)
{
	if (bp->b_bcount > XBD_MAX_XFER) {
		bp->b_bcount = XBD_MAX_XFER;
	}
	minphys(bp);
}

int
xbdopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct	xbd_xenbus_softc *sc;

	sc = device_lookup_private(&xbd_cd, DISKUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if ((flags & FWRITE) && (sc->sc_info & VDISK_READONLY))
		return EROFS;

	DPRINTF(("xbdopen(0x%04x, %d)\n", dev, flags));
	return dk_open(di, &sc->sc_dksc, dev, flags, fmt, l);
}

int
xbdclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct xbd_xenbus_softc *sc;

	sc = device_lookup_private(&xbd_cd, DISKUNIT(dev));

	DPRINTF(("xbdclose(%d, %d)\n", dev, flags));
	return dk_close(di, &sc->sc_dksc, dev, flags, fmt, l);
}

void
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

	dk_strategy(di, &sc->sc_dksc, bp);
	return;
}

int
xbdsize(dev_t dev)
{
	struct	xbd_xenbus_softc *sc;

	DPRINTF(("xbdsize(%d)\n", dev));

	sc = device_lookup_private(&xbd_cd, DISKUNIT(dev));
	if (sc == NULL || sc->sc_shutdown != BLKIF_SHUTDOWN_RUN)
		return -1;
	return dk_size(di, &sc->sc_dksc, dev);
}

int
xbdread(dev_t dev, struct uio *uio, int flags)
{
	struct xbd_xenbus_softc *sc = 
	    device_lookup_private(&xbd_cd, DISKUNIT(dev));
	struct  dk_softc *dksc = &sc->sc_dksc;

	if ((dksc->sc_flags & DKF_INITED) == 0)
		return ENXIO;
	return physio(xbdstrategy, NULL, dev, B_READ, xbdminphys, uio);
}

int
xbdwrite(dev_t dev, struct uio *uio, int flags)
{
	struct xbd_xenbus_softc *sc =
	    device_lookup_private(&xbd_cd, DISKUNIT(dev));
	struct  dk_softc *dksc = &sc->sc_dksc;

	if ((dksc->sc_flags & DKF_INITED) == 0)
		return ENXIO;
	if (__predict_false(sc->sc_info & VDISK_READONLY))
		return EROFS;
	return physio(xbdstrategy, NULL, dev, B_WRITE, xbdminphys, uio);
}

int
xbdioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct xbd_xenbus_softc *sc =
	    device_lookup_private(&xbd_cd, DISKUNIT(dev));
	struct	dk_softc *dksc;
	int	error;
	int s;
	struct xbd_req *xbdreq;
	blkif_request_t *req;
	int notify;

	DPRINTF(("xbdioctl(%d, %08lx, %p, %d, %p)\n",
	    dev, cmd, data, flag, l));
	dksc = &sc->sc_dksc;

	error = disk_ioctl(&sc->sc_dksc.sc_dkdev, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	switch (cmd) {
	case DIOCSSTRATEGY:
		error = EOPNOTSUPP;
		break;
	case DIOCCACHESYNC:
		if (sc->sc_cache_flush <= 0) {
			if (sc->sc_cache_flush == 0) {
				aprint_error_dev(sc->sc_dksc.sc_dev,
				    "WARNING: cache flush not supported "
				    "by backend\n");
				sc->sc_cache_flush = -1;
			}
			return EOPNOTSUPP;
		}

		s = splbio();

		while (RING_FULL(&sc->sc_ring)) {
			sc->sc_xbdreq_wait = 1;
			tsleep(&sc->sc_xbdreq_wait, PRIBIO, "xbdreq", 0);
		}
		sc->sc_xbdreq_wait = 0;

		xbdreq = SLIST_FIRST(&sc->sc_xbdreq_head);
		if (__predict_false(xbdreq == NULL)) {
			DPRINTF(("xbdioctl: no req\n"));
			error = ENOMEM;
		} else {
			SLIST_REMOVE_HEAD(&sc->sc_xbdreq_head, req_next);
			req = RING_GET_REQUEST(&sc->sc_ring,
			    sc->sc_ring.req_prod_pvt);
			req->id = xbdreq->req_id;
			req->operation = BLKIF_OP_FLUSH_DISKCACHE;
			req->handle = sc->sc_handle;
			xbdreq->req_sync.s_done = 0;
			sc->sc_ring.req_prod_pvt++;
			RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&sc->sc_ring,
			    notify);
			if (notify)
				hypervisor_notify_via_evtchn(sc->sc_evtchn);
			/* request sent, no wait for completion */
			while (xbdreq->req_sync.s_done == 0) {
				tsleep(xbdreq, PRIBIO, "xbdsync", 0);
			}
			if (xbdreq->req_sync.s_error == BLKIF_RSP_EOPNOTSUPP)
				error = EOPNOTSUPP;
			else if (xbdreq->req_sync.s_error == BLKIF_RSP_OKAY)
				error = 0;
			else
				error = EIO;
			SLIST_INSERT_HEAD(&sc->sc_xbdreq_head, xbdreq,
			    req_next);
		}
		splx(s);
		break;

	default:
		error = dk_ioctl(di, dksc, dev, cmd, data, flag, l);
		break;
	}

	return error;
}

int
xbddump(dev_t dev, daddr_t blkno, void *va, size_t size)
{
	struct xbd_xenbus_softc *sc;

	sc  = device_lookup_private(&xbd_cd, DISKUNIT(dev));
	if (sc == NULL)
		return (ENXIO);

	DPRINTF(("xbddump(%d, %" PRId64 ", %p, %lu)\n", dev, blkno, va,
	    (unsigned long)size));
	return dk_dump(di, &sc->sc_dksc, dev, blkno, va, size);
}

static void
xbdstart(struct dk_softc *dksc)
{
	struct xbd_xenbus_softc *sc = (struct xbd_xenbus_softc *)dksc;
	struct buf *bp;
#ifdef DIAGNOSTIC
	struct  buf *qbp; 
#endif
	struct xbd_req *xbdreq;
	blkif_request_t *req;
	size_t bcount, off;
	paddr_t ma;
	vaddr_t va;
	int nsects, nbytes, seg;
	int notify;

	while ((bp = bufq_peek(dksc->sc_bufq)) != NULL) {

		DPRINTF(("xbdstart(%p): b_bcount = %ld\n",
		    bp, (long)bp->b_bcount));

		if (sc->sc_shutdown != BLKIF_SHUTDOWN_RUN) {
			bp->b_error = EIO;
			goto err;
		}

		if (bp->b_rawblkno < 0 || bp->b_rawblkno > sc->sc_xbdsize) {
			/* invalid block number */
			bp->b_error = EINVAL;
			goto err;
		}

		if (bp->b_rawblkno == sc->sc_xbdsize) {
			/* at end of disk; return short read */
			bp->b_resid = bp->b_bcount;
#ifdef DIAGNOSTIC 
			qbp = bufq_get(dksc->sc_bufq);
			KASSERT(bp == qbp);
#else
			(void)bufq_get(dksc->sc_bufq);
#endif
			biodone(bp);
			continue;
		}

		if (__predict_false(
		    sc->sc_backend_status == BLKIF_STATE_SUSPENDED)) {
			/* device is suspended, do not consume buffer */
			DPRINTF(("%s: (xbdstart) device suspended\n",
			    device_xname(sc->sc_dksc.sc_dev)));
			goto out;
		}

		if (RING_FULL(&sc->sc_ring) || sc->sc_xbdreq_wait) {
			DPRINTF(("xbdstart: ring_full\n"));
			goto out;
		}

		xbdreq = SLIST_FIRST(&sc->sc_xbdreq_head);
		if (__predict_false(xbdreq == NULL)) {
			DPRINTF(("xbdstart: no req\n"));
			goto out;
		}

		xbdreq->req_bp = bp;
		xbdreq->req_data = bp->b_data;
		if ((vaddr_t)bp->b_data & (XEN_BSIZE - 1)) {
			if (__predict_false(xbd_map_align(xbdreq) != 0)) {
				DPRINTF(("xbdstart: no align\n"));
				goto out;
			}
		}
		/* now we're sure we'll send this buf */
#ifdef DIAGNOSTIC 
		qbp = bufq_get(dksc->sc_bufq);
		KASSERT(bp == qbp);
#else
		(void)bufq_get(dksc->sc_bufq);
#endif
		disk_busy(&dksc->sc_dkdev);

		SLIST_REMOVE_HEAD(&sc->sc_xbdreq_head, req_next);
		req = RING_GET_REQUEST(&sc->sc_ring, sc->sc_ring.req_prod_pvt);
		req->id = xbdreq->req_id;
		req->operation =
		    bp->b_flags & B_READ ? BLKIF_OP_READ : BLKIF_OP_WRITE;
		req->sector_number = bp->b_rawblkno;
		req->handle = sc->sc_handle;

		va = (vaddr_t)xbdreq->req_data & ~PAGE_MASK;
		off = (vaddr_t)xbdreq->req_data & PAGE_MASK;
		if (bp->b_rawblkno + bp->b_bcount / DEV_BSIZE >=
		    sc->sc_xbdsize) {
			bcount = (sc->sc_xbdsize - bp->b_rawblkno) * DEV_BSIZE;
			bp->b_resid = bp->b_bcount - bcount;
		} else {
			bcount = bp->b_bcount;
			bp->b_resid = 0;
		}
		if (bcount > XBD_MAX_XFER) {
			bp->b_resid += bcount - XBD_MAX_XFER;
			bcount = XBD_MAX_XFER;
		}
		for (seg = 0; bcount > 0;) {
			pmap_extract_ma(pmap_kernel(), va, &ma);
			KASSERT((ma & (XEN_BSIZE - 1)) == 0);
			if (bcount > PAGE_SIZE - off)
				nbytes = PAGE_SIZE - off;
			else
				nbytes = bcount;
			nsects = nbytes >> XEN_BSHIFT;
			req->seg[seg].first_sect = off >> XEN_BSHIFT;
			req->seg[seg].last_sect =
			    (off >> XEN_BSHIFT) + nsects - 1;
			KASSERT(req->seg[seg].first_sect <=
			    req->seg[seg].last_sect);
			KASSERT(req->seg[seg].last_sect < 8);
			if (__predict_false(xengnt_grant_access(
			    sc->sc_xbusd->xbusd_otherend_id, ma,
			    (bp->b_flags & B_READ) == 0,
			    &xbdreq->req_gntref[seg])))
				panic("xbdstart: xengnt_grant_access"); /* XXX XXX !!! */
			req->seg[seg].gref = xbdreq->req_gntref[seg];
			seg++;
			KASSERT(seg <= BLKIF_MAX_SEGMENTS_PER_REQUEST);
			va += PAGE_SIZE;
			off = 0;
			bcount -= nbytes;
		}
		xbdreq->req_nr_segments = req->nr_segments = seg;
		sc->sc_ring.req_prod_pvt++;
	}

out:
	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&sc->sc_ring, notify);
	if (notify)
		hypervisor_notify_via_evtchn(sc->sc_evtchn);
	return;

err:
#ifdef DIAGNOSTIC 
	qbp = bufq_get(dksc->sc_bufq);
	KASSERT(bp == qbp);
#else
	(void)bufq_get(dksc->sc_bufq);
#endif
	bp->b_resid = bp->b_bcount;
	biodone(bp);
	return;
}

static int
xbd_map_align(struct xbd_req *req)
{
	int s = splvm();
	int rc;

	rc = uvm_km_kmem_alloc(kmem_va_arena,
	    req->req_bp->b_bcount, (VM_NOSLEEP | VM_INSTANTFIT),
	    (vmem_addr_t *)&req->req_data);
	splx(s);
	if (__predict_false(rc != 0))
		return ENOMEM;
	if ((req->req_bp->b_flags & B_READ) == 0)
		memcpy(req->req_data, req->req_bp->b_data,
		    req->req_bp->b_bcount);
	return 0;
}

static void
xbd_unmap_align(struct xbd_req *req)
{
	int s;
	if (req->req_bp->b_flags & B_READ)
		memcpy(req->req_bp->b_data, req->req_data,
		    req->req_bp->b_bcount);
	s = splvm();
	uvm_km_kmem_free(kmem_va_arena, (vaddr_t)req->req_data, req->req_bp->b_bcount);
	splx(s);
}
