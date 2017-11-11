/*      $NetBSD: xbdback_xenbus.c,v 1.65 2017/11/11 21:03:01 riastradh Exp $      */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xbdback_xenbus.c,v 1.65 2017/11/11 21:03:01 riastradh Exp $");

#include <sys/atomic.h>
#include <sys/buf.h>
#include <sys/condvar.h>
#include <sys/conf.h>
#include <sys/disk.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vnode.h>

#include <xen/xen.h>
#include <xen/xen_shm.h>
#include <xen/evtchn.h>
#include <xen/xenbus.h>
#include <xen/xen-public/io/protocols.h>

/* #define XENDEBUG_VBD */
#ifdef XENDEBUG_VBD
#define XENPRINTF(x) printf x
#else
#define XENPRINTF(x)
#endif

#define BLKIF_RING_SIZE __RING_SIZE((blkif_sring_t *)0, PAGE_SIZE)

/*
 * Backend block device driver for Xen
 */

/* Max number of pages per request. The request may not be page aligned */
#define BLKIF_MAX_PAGES_PER_REQUEST (BLKIF_MAX_SEGMENTS_PER_REQUEST + 1)

/* Values are expressed in 512-byte sectors */
#define VBD_BSIZE 512
#define VBD_MAXSECT ((PAGE_SIZE / VBD_BSIZE) - 1)

struct xbdback_request;
struct xbdback_io;
struct xbdback_fragment;
struct xbdback_instance;

/*
 * status of a xbdback instance:
 * WAITING: xbdback instance is connected, waiting for requests
 * RUN: xbdi thread must be woken up, I/Os have to be processed
 * DISCONNECTING: the instance is closing, no more I/Os can be scheduled
 * DISCONNECTED: no I/Os, no ring, the thread should terminate.
 */
typedef enum {WAITING, RUN, DISCONNECTING, DISCONNECTED} xbdback_state_t;

/*
 * Each xbdback instance is managed by a single thread that handles all
 * the I/O processing. As there are a variety of conditions that can block,
 * everything will be done in a sort of continuation-passing style.
 *
 * When the execution has to block to delay processing, for example to
 * allow system to recover because of memory shortage (via shared memory
 * callback), the return value of a continuation can be set to NULL. In that
 * case, the thread will go back to sleeping and wait for the proper
 * condition before it starts processing requests again from where it left.
 * Continuation state is "stored" in the xbdback instance (xbdi_cont and
 * xbdi_cont_aux), and should only be manipulated by the instance thread.
 *
 * As xbdback(4) has to handle different sort of asynchronous events (Xen
 * event channels, biointr() soft interrupts, xenbus commands), the xbdi_lock
 * mutex is used to protect specific elements of the xbdback instance from
 * concurrent access: thread status and ring access (when pushing responses).
 * 
 * Here's how the call graph is supposed to be for a single I/O:
 *
 * xbdback_co_main()
 *        |
 *        |               --> xbdback_co_cache_doflush() or NULL
 *        |               |
 *        |               - xbdback_co_cache_flush2() <- xbdback_co_do_io() <-
 *        |                                            |                     |
 *        |               |-> xbdback_co_cache_flush() -> xbdback_co_map_io()-
 * xbdback_co_main_loop()-|
 *        |               |-> xbdback_co_main_done() ---> xbdback_co_map_io()-
 *        |                                           |                      |
 *        |               -- xbdback_co_main_done2() <-- xbdback_co_do_io() <-
 *        |               |
 *        |               --> xbdback_co_main() or NULL
 *        |
 *     xbdback_co_io() -> xbdback_co_main_incr() -> xbdback_co_main_loop()
 *        |
 *     xbdback_co_io_gotreq()--+--> xbdback_co_map_io() ---
 *        |                    |                          |
 *  -> xbdback_co_io_loop()----|  <- xbdback_co_do_io() <--
 *  |     |     |     |
 *  |     |     |     |----------> xbdback_co_io_gotio()
 *  |     |     |                         |
 *  |     |   xbdback_co_main_incr()      |
 *  |     |     |                         |
 *  |     |   xbdback_co_main_loop()      |
 *  |     |                               |
 *  |  xbdback_co_io_gotio2() <-----------|
 *  |     |           |
 *  |     |           |----------> xbdback_co_io_gotfrag()
 *  |     |                               |
 *  -- xbdback_co_io_gotfrag2() <---------|
 *        |
 *     xbdback_co_main_incr() -> xbdback_co_main_loop()
 */
typedef void *(* xbdback_cont_t)(struct xbdback_instance *, void *);

enum xbdi_proto {
	XBDIP_NATIVE,
	XBDIP_32,
	XBDIP_64
};

/* we keep the xbdback instances in a linked list */
struct xbdback_instance {
	SLIST_ENTRY(xbdback_instance) next;
	struct xenbus_device *xbdi_xbusd; /* our xenstore entry */
	struct xenbus_watch xbdi_watch; /* to watch our store */
	domid_t xbdi_domid;	/* attached to this domain */
	uint32_t xbdi_handle;	/* domain-specific handle */
	char xbdi_name[16];	/* name of this instance */
	/* mutex that protects concurrent access to the xbdback instance */
	kmutex_t xbdi_lock;
	kcondvar_t xbdi_cv;	/* wait channel for thread work */
	xbdback_state_t xbdi_status; /* thread's status */
	/* backing device parameters */
	dev_t xbdi_dev;
	const struct bdevsw *xbdi_bdevsw; /* pointer to the device's bdevsw */
	struct vnode *xbdi_vp;
	uint64_t xbdi_size;
	bool xbdi_ro; /* is device read-only ? */
	/* parameters for the communication */
	unsigned int xbdi_evtchn;
	struct intrhand *xbdi_ih;
	/* private parameters for communication */
	blkif_back_ring_proto_t xbdi_ring;
	enum xbdi_proto xbdi_proto;
	grant_handle_t xbdi_ring_handle; /* to unmap the ring */
	vaddr_t xbdi_ring_va; /* to unmap the ring */
	/* disconnection must be postponed until all I/O is done */
	int xbdi_refcnt;
	/* 
	 * State for I/O processing/coalescing follows; this has to
	 * live here instead of on the stack because of the
	 * continuation-ness (see above).
	 */
	RING_IDX xbdi_req_prod; /* limit on request indices */
	xbdback_cont_t xbdi_cont, xbdi_cont_aux;
	SIMPLEQ_ENTRY(xbdback_instance) xbdi_on_hold; /* waiting on resources */
	/* _request state: track requests fetched from ring */
	struct xbdback_request *xbdi_req; /* if NULL, ignore following */
	blkif_request_t xbdi_xen_req;
	int xbdi_segno;
	/* _io state: I/O associated to this instance */
	struct xbdback_io *xbdi_io; /* if NULL, ignore next field */
	daddr_t xbdi_next_sector;
	uint8_t xbdi_last_fs, xbdi_this_fs; /* first sectors */
	uint8_t xbdi_last_ls, xbdi_this_ls; /* last sectors */
	grant_ref_t xbdi_thisgrt, xbdi_lastgrt; /* grants */
	/* other state */
	int xbdi_same_page; /* are we merging two segments on the same page? */
	uint xbdi_pendingreqs; /* number of I/O in fly */
	struct timeval xbdi_lasterr_time;    /* error time tracking */
#ifdef DEBUG
	struct timeval xbdi_lastfragio_time; /* fragmented I/O tracking */
#endif
};
/* Manipulation of the above reference count. */
#define xbdi_get(xbdip) atomic_inc_uint(&(xbdip)->xbdi_refcnt)
#define xbdi_put(xbdip)                                      \
do {                                                         \
	if (atomic_dec_uint_nv(&(xbdip)->xbdi_refcnt) == 0)  \
               xbdback_finish_disconnect(xbdip);             \
} while (/* CONSTCOND */ 0)

SLIST_HEAD(, xbdback_instance) xbdback_instances;

/*
 * For each request from a guest, a xbdback_request is allocated from
 * a pool.  This will describe the request until completion.  The
 * request may require multiple IO operations to perform, so the
 * per-IO information is not stored here.
 */
struct xbdback_request {
	struct xbdback_instance *rq_xbdi; /* our xbd instance */
	uint64_t rq_id;
	int rq_iocount; /* reference count; or, number of outstanding I/O's */
	int rq_ioerrs;
	uint8_t rq_operation;
};

/*
 * For each I/O operation associated with one of those requests, an
 * xbdback_io is allocated from a pool.  It may correspond to multiple
 * Xen disk requests, or parts of them, if several arrive at once that
 * can be coalesced.
 */
struct xbdback_io {
	/* The instance pointer is duplicated for convenience. */
	struct xbdback_instance *xio_xbdi; /* our xbd instance */
	uint8_t xio_operation;
	union {
		struct {
			struct buf xio_buf; /* our I/O */
			/* xbd requests involved */
			SLIST_HEAD(, xbdback_fragment) xio_rq;
			/* the virtual address to map the request at */
			vaddr_t xio_vaddr;
			/* grants to map */
			grant_ref_t xio_gref[XENSHM_MAX_PAGES_PER_REQUEST];
			/* grants release */
			grant_handle_t xio_gh[XENSHM_MAX_PAGES_PER_REQUEST];
			uint16_t xio_nrma; /* number of guest pages */
			uint16_t xio_mapped; /* == 1: grants are mapped */
		} xio_rw;
		uint64_t xio_flush_id;
	} u;
};
#define xio_buf		u.xio_rw.xio_buf
#define xio_rq		u.xio_rw.xio_rq
#define xio_vaddr	u.xio_rw.xio_vaddr
#define xio_gref	u.xio_rw.xio_gref
#define xio_gh		u.xio_rw.xio_gh
#define xio_nrma	u.xio_rw.xio_nrma
#define xio_mapped	u.xio_rw.xio_mapped

#define xio_flush_id	u.xio_flush_id

/*
 * Rather than having the xbdback_io keep an array of the
 * xbdback_requests involved, since the actual number will probably be
 * small but might be as large as BLKIF_RING_SIZE, use a list.  This
 * would be threaded through xbdback_request, but one of them might be
 * part of multiple I/O's, alas.
 */
struct xbdback_fragment {
	struct xbdback_request *car;
	SLIST_ENTRY(xbdback_fragment) cdr;
};

/*
 * Pools to manage the chain of block requests and I/Os fragments
 * submitted by frontend.
 */
struct xbdback_pool {
	struct pool_cache pc;
	struct timeval last_warning;
} xbdback_request_pool, xbdback_io_pool, xbdback_fragment_pool;

SIMPLEQ_HEAD(xbdback_iqueue, xbdback_instance);
static struct xbdback_iqueue xbdback_shmq;
static int xbdback_shmcb; /* have we already registered a callback? */

/* Interval between reports of I/O errors from frontend */
struct timeval xbdback_err_intvl = { 1, 0 };

#ifdef DEBUG
struct timeval xbdback_fragio_intvl = { 60, 0 };
#endif
       void xbdbackattach(int);
static int  xbdback_xenbus_create(struct xenbus_device *);
static int  xbdback_xenbus_destroy(void *);
static void xbdback_frontend_changed(void *, XenbusState);
static void xbdback_backend_changed(struct xenbus_watch *,
    const char **, unsigned int);
static int  xbdback_evthandler(void *);

static int  xbdback_connect(struct xbdback_instance *);
static void xbdback_disconnect(struct xbdback_instance *);
static void xbdback_finish_disconnect(struct xbdback_instance *);

static struct xbdback_instance *xbdif_lookup(domid_t, uint32_t);

static void *xbdback_co_main(struct xbdback_instance *, void *);
static void *xbdback_co_main_loop(struct xbdback_instance *, void *);
static void *xbdback_co_main_incr(struct xbdback_instance *, void *);
static void *xbdback_co_main_done(struct xbdback_instance *, void *);
static void *xbdback_co_main_done2(struct xbdback_instance *, void *);

static void *xbdback_co_cache_flush(struct xbdback_instance *, void *);
static void *xbdback_co_cache_flush2(struct xbdback_instance *, void *);
static void *xbdback_co_cache_doflush(struct xbdback_instance *, void *);

static void *xbdback_co_io(struct xbdback_instance *, void *);
static void *xbdback_co_io_gotreq(struct xbdback_instance *, void *);
static void *xbdback_co_io_loop(struct xbdback_instance *, void *);
static void *xbdback_co_io_gotio(struct xbdback_instance *, void *);
static void *xbdback_co_io_gotio2(struct xbdback_instance *, void *);
static void *xbdback_co_io_gotfrag(struct xbdback_instance *, void *);
static void *xbdback_co_io_gotfrag2(struct xbdback_instance *, void *);

static void *xbdback_co_map_io(struct xbdback_instance *, void *);
static void *xbdback_co_do_io(struct xbdback_instance *, void *);

static void *xbdback_co_wait_shm_callback(struct xbdback_instance *, void *);

static int  xbdback_shm_callback(void *);
static void xbdback_io_error(struct xbdback_io *, int);
static void xbdback_iodone(struct buf *);
static void xbdback_send_reply(struct xbdback_instance *, uint64_t , int , int);

static void *xbdback_map_shm(struct xbdback_io *);
static void xbdback_unmap_shm(struct xbdback_io *);

static void *xbdback_pool_get(struct xbdback_pool *,
			      struct xbdback_instance *);
static void xbdback_pool_put(struct xbdback_pool *, void *);
static void xbdback_thread(void *);
static void xbdback_wakeup_thread(struct xbdback_instance *);
static void xbdback_trampoline(struct xbdback_instance *, void *);

static struct xenbus_backend_driver xbd_backend_driver = {
	.xbakd_create = xbdback_xenbus_create,
	.xbakd_type = "vbd"
};

void
xbdbackattach(int n)
{
	XENPRINTF(("xbdbackattach\n"));

	/*
	 * initialize the backend driver, register the control message handler
	 * and send driver up message.
	 */
	SLIST_INIT(&xbdback_instances);
	SIMPLEQ_INIT(&xbdback_shmq);
	xbdback_shmcb = 0;

	pool_cache_bootstrap(&xbdback_request_pool.pc,
	    sizeof(struct xbdback_request), 0, 0, 0, "xbbrp", NULL,
	    IPL_SOFTBIO, NULL, NULL, NULL);
	pool_cache_bootstrap(&xbdback_io_pool.pc,
	    sizeof(struct xbdback_io), 0, 0, 0, "xbbip", NULL,
	    IPL_SOFTBIO, NULL, NULL, NULL);
	pool_cache_bootstrap(&xbdback_fragment_pool.pc,
	    sizeof(struct xbdback_fragment), 0, 0, 0, "xbbfp", NULL,
	    IPL_SOFTBIO, NULL, NULL, NULL);

	/* we allocate enough to handle a whole ring at once */
	if (pool_prime(&xbdback_request_pool.pc.pc_pool, BLKIF_RING_SIZE) != 0)
		printf("xbdback: failed to prime request pool\n");
	if (pool_prime(&xbdback_io_pool.pc.pc_pool, BLKIF_RING_SIZE) != 0)
		printf("xbdback: failed to prime io pool\n");
	if (pool_prime(&xbdback_fragment_pool.pc.pc_pool,
            BLKIF_MAX_SEGMENTS_PER_REQUEST * BLKIF_RING_SIZE) != 0)
		printf("xbdback: failed to prime fragment pool\n");

	xenbus_backend_register(&xbd_backend_driver);
}

static int
xbdback_xenbus_create(struct xenbus_device *xbusd)
{
	struct xbdback_instance *xbdi;
	long domid, handle;
	int error, i;
	char *ep;

	if ((error = xenbus_read_ul(NULL, xbusd->xbusd_path,
	    "frontend-id", &domid, 10)) != 0) {
		aprint_error("xbdback: can't read %s/frontend-id: %d\n",
		    xbusd->xbusd_path, error);
		return error;
	}

	/*
	 * get handle: this is the last component of the path; which is
	 * a decimal number. $path/dev contains the device name, which is not
	 * appropriate.
	 */
	for (i = strlen(xbusd->xbusd_path); i > 0; i--) {
		if (xbusd->xbusd_path[i] == '/')
			break;
	}
	if (i == 0) {
		aprint_error("xbdback: can't parse %s\n",
		    xbusd->xbusd_path);
		return EFTYPE;
	}
	handle = strtoul(&xbusd->xbusd_path[i+1], &ep, 10);
	if (*ep != '\0') {
		aprint_error("xbdback: can't parse %s\n",
		    xbusd->xbusd_path);
		return EFTYPE;
	}
			
	if (xbdif_lookup(domid, handle) != NULL) {
		return EEXIST;
	}
	xbdi = kmem_zalloc(sizeof(*xbdi), KM_SLEEP);

	xbdi->xbdi_domid = domid;
	xbdi->xbdi_handle = handle;
	snprintf(xbdi->xbdi_name, sizeof(xbdi->xbdi_name), "xbdb%di%d",
	    xbdi->xbdi_domid, xbdi->xbdi_handle);

	/* initialize status and reference counter */
	xbdi->xbdi_status = DISCONNECTED;
	xbdi_get(xbdi);

	mutex_init(&xbdi->xbdi_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&xbdi->xbdi_cv, xbdi->xbdi_name);
	SLIST_INSERT_HEAD(&xbdback_instances, xbdi, next);

	xbusd->xbusd_u.b.b_cookie = xbdi;	
	xbusd->xbusd_u.b.b_detach = xbdback_xenbus_destroy;
	xbusd->xbusd_otherend_changed = xbdback_frontend_changed;
	xbdi->xbdi_xbusd = xbusd;

	error = xenbus_watch_path2(xbusd, xbusd->xbusd_path, "physical-device",
	    &xbdi->xbdi_watch, xbdback_backend_changed);
	if (error) {
		printf("failed to watch on %s/physical-device: %d\n",
		    xbusd->xbusd_path, error);
		goto fail;
	}
	xbdi->xbdi_watch.xbw_dev = xbusd;
	error = xenbus_switch_state(xbusd, NULL, XenbusStateInitWait);
	if (error) {
		printf("failed to switch state on %s: %d\n",
		    xbusd->xbusd_path, error);
		goto fail2;
	}
	return 0;
fail2:
	unregister_xenbus_watch(&xbdi->xbdi_watch);
fail:
	kmem_free(xbdi, sizeof(*xbdi));
	return error;
}

static int
xbdback_xenbus_destroy(void *arg)
{
	struct xbdback_instance *xbdi = arg;
	struct xenbus_device *xbusd = xbdi->xbdi_xbusd;
	struct gnttab_unmap_grant_ref ungrop;
	int err;

	XENPRINTF(("xbdback_xenbus_destroy state %d\n", xbdi->xbdi_status));

	xbdback_disconnect(xbdi);

	/* unregister watch */
	if (xbdi->xbdi_watch.node) {
		unregister_xenbus_watch(&xbdi->xbdi_watch);
		free(xbdi->xbdi_watch.node, M_DEVBUF);
		xbdi->xbdi_watch.node = NULL;
	}
	/* unmap ring */
	if (xbdi->xbdi_ring_va != 0) {
		ungrop.host_addr = xbdi->xbdi_ring_va;
		ungrop.handle = xbdi->xbdi_ring_handle;
		ungrop.dev_bus_addr = 0;
		err = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref,
		    &ungrop, 1);
		if (err)
		    printf("xbdback %s: unmap_grant_ref failed: %d\n",
			xbusd->xbusd_otherend, err);
		uvm_km_free(kernel_map, xbdi->xbdi_ring_va,
		    PAGE_SIZE, UVM_KMF_VAONLY);
	}
	/* close device */
	if (xbdi->xbdi_size) {
		const char *name;
		struct dkwedge_info wi;
		if (getdiskinfo(xbdi->xbdi_vp, &wi) == 0)
			name = wi.dkw_devname;
		else
			name = "*unknown*";
		printf("xbd backend: detach device %s for domain %d\n",
		    name, xbdi->xbdi_domid);
		vn_close(xbdi->xbdi_vp, FREAD, NOCRED);
	}
	SLIST_REMOVE(&xbdback_instances, xbdi, xbdback_instance, next);
	mutex_destroy(&xbdi->xbdi_lock);
	cv_destroy(&xbdi->xbdi_cv);
	kmem_free(xbdi, sizeof(*xbdi));
	return 0;
}

static int
xbdback_connect(struct xbdback_instance *xbdi)
{
	int len, err;
	struct gnttab_map_grant_ref grop;
	struct gnttab_unmap_grant_ref ungrop;
	evtchn_op_t evop;
	u_long ring_ref, revtchn;
	char *xsproto;
	const char *proto;
	struct xenbus_device *xbusd = xbdi->xbdi_xbusd;

	XENPRINTF(("xbdback %s: connect\n", xbusd->xbusd_path));
	/* read comunication informations */
	err = xenbus_read_ul(NULL, xbusd->xbusd_otherend,
	    "ring-ref", &ring_ref, 10);
	if (err) {
		xenbus_dev_fatal(xbusd, err, "reading %s/ring-ref",
		    xbusd->xbusd_otherend);
		return -1;
	}
	XENPRINTF(("xbdback %s: connect ring-ref %lu\n", xbusd->xbusd_path, ring_ref));
	err = xenbus_read_ul(NULL, xbusd->xbusd_otherend,
	    "event-channel", &revtchn, 10);
	if (err) {
		xenbus_dev_fatal(xbusd, err, "reading %s/event-channel",
		    xbusd->xbusd_otherend);
		return -1;
	}
	XENPRINTF(("xbdback %s: connect revtchn %lu\n", xbusd->xbusd_path, revtchn));
	err = xenbus_read(NULL, xbusd->xbusd_otherend, "protocol",
	    &len, &xsproto);
	if (err) {
		xbdi->xbdi_proto = XBDIP_NATIVE;
		proto = "unspecified";
		XENPRINTF(("xbdback %s: connect no xsproto\n", xbusd->xbusd_path));
	} else {
		XENPRINTF(("xbdback %s: connect xsproto %s\n", xbusd->xbusd_path, xsproto));
		if (strcmp(xsproto, XEN_IO_PROTO_ABI_NATIVE) == 0) {
			xbdi->xbdi_proto = XBDIP_NATIVE;
			proto = XEN_IO_PROTO_ABI_NATIVE;
		} else if (strcmp(xsproto, XEN_IO_PROTO_ABI_X86_32) == 0) {
			xbdi->xbdi_proto = XBDIP_32;
			proto = XEN_IO_PROTO_ABI_X86_32;
		} else if (strcmp(xsproto, XEN_IO_PROTO_ABI_X86_64) == 0) {
			xbdi->xbdi_proto = XBDIP_64;
			proto = XEN_IO_PROTO_ABI_X86_64;
		} else {
			aprint_error("xbd domain %d: unknown proto %s\n",
			    xbdi->xbdi_domid, xsproto);
			free(xsproto, M_DEVBUF);
			return -1;
		}
		free(xsproto, M_DEVBUF);
	}

	/* allocate VA space and map rings */
	xbdi->xbdi_ring_va = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_VAONLY);
	if (xbdi->xbdi_ring_va == 0) {
		xenbus_dev_fatal(xbusd, ENOMEM,
		    "can't get VA for ring", xbusd->xbusd_otherend);
		return -1;
	}
	XENPRINTF(("xbdback %s: connect va 0x%" PRIxVADDR "\n", xbusd->xbusd_path, xbdi->xbdi_ring_va));

	grop.host_addr = xbdi->xbdi_ring_va;
	grop.flags = GNTMAP_host_map;
	grop.ref = ring_ref;
	grop.dom = xbdi->xbdi_domid;
	err = HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref,
	    &grop, 1);
	if (err || grop.status) {
		aprint_error("xbdback %s: can't map grant ref: %d/%d\n",
		    xbusd->xbusd_path, err, grop.status);
		xenbus_dev_fatal(xbusd, EINVAL,
		    "can't map ring", xbusd->xbusd_otherend);
		goto err;
	}
	xbdi->xbdi_ring_handle = grop.handle;
	XENPRINTF(("xbdback %s: connect grhandle %d\n", xbusd->xbusd_path, grop.handle));

	switch(xbdi->xbdi_proto) {
	case XBDIP_NATIVE:
	{
		blkif_sring_t *sring = (void *)xbdi->xbdi_ring_va;
		BACK_RING_INIT(&xbdi->xbdi_ring.ring_n, sring, PAGE_SIZE);
		break;
	}
	case XBDIP_32:
	{
		blkif_x86_32_sring_t *sring = (void *)xbdi->xbdi_ring_va;
		BACK_RING_INIT(&xbdi->xbdi_ring.ring_32, sring, PAGE_SIZE);
		break;
	}
	case XBDIP_64:
	{
		blkif_x86_64_sring_t *sring = (void *)xbdi->xbdi_ring_va;
		BACK_RING_INIT(&xbdi->xbdi_ring.ring_64, sring, PAGE_SIZE);
		break;
	}
	}

	evop.cmd = EVTCHNOP_bind_interdomain;
	evop.u.bind_interdomain.remote_dom = xbdi->xbdi_domid;
	evop.u.bind_interdomain.remote_port = revtchn;
	err = HYPERVISOR_event_channel_op(&evop);
	if (err) {
		aprint_error("blkback %s: "
		    "can't get event channel: %d\n",
		    xbusd->xbusd_otherend, err);
		xenbus_dev_fatal(xbusd, err,
		    "can't bind event channel", xbusd->xbusd_otherend);
		goto err2;
	}
	XENPRINTF(("xbdback %s: connect evchannel %d\n", xbusd->xbusd_path, xbdi->xbdi_evtchn));
	xbdi->xbdi_evtchn = evop.u.bind_interdomain.local_port;

	xbdi->xbdi_ih = intr_establish_xname(0, &xen_pic, xbdi->xbdi_evtchn,
	    IST_LEVEL, IPL_BIO, xbdback_evthandler, xbdi, false,
	    xbdi->xbdi_name);
	KASSERT(xbdi->xbdi_ih != NULL);
	aprint_verbose("xbd backend domain %d handle %#x (%d) "
	    "using event channel %d, protocol %s\n", xbdi->xbdi_domid,
	    xbdi->xbdi_handle, xbdi->xbdi_handle, xbdi->xbdi_evtchn, proto);

	/* enable the xbdback event handler machinery */
	xbdi->xbdi_status = WAITING;
	hypervisor_enable_event(xbdi->xbdi_evtchn);
	hypervisor_notify_via_evtchn(xbdi->xbdi_evtchn);

	if (kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL,
	    xbdback_thread, xbdi, NULL, "%s", xbdi->xbdi_name) == 0)
		return 0;

err2:
	/* unmap ring */
	ungrop.host_addr = xbdi->xbdi_ring_va;
	ungrop.handle = xbdi->xbdi_ring_handle;
	ungrop.dev_bus_addr = 0;
	err = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref,
	    &ungrop, 1);
	if (err)
	    aprint_error("xbdback %s: unmap_grant_ref failed: %d\n",
		xbusd->xbusd_path, err);

err:
	/* free ring VA space */
	uvm_km_free(kernel_map, xbdi->xbdi_ring_va, PAGE_SIZE, UVM_KMF_VAONLY);
	return -1;
}

/*
 * Signal a xbdback thread to disconnect. Done in 'xenwatch' thread context.
 */
static void
xbdback_disconnect(struct xbdback_instance *xbdi)
{
	
	mutex_enter(&xbdi->xbdi_lock);
	if (xbdi->xbdi_status == DISCONNECTED) {
		mutex_exit(&xbdi->xbdi_lock);
		return;
	}
	hypervisor_mask_event(xbdi->xbdi_evtchn);
	intr_disestablish(xbdi->xbdi_ih);

	/* signal thread that we want to disconnect, then wait for it */
	xbdi->xbdi_status = DISCONNECTING;
	cv_signal(&xbdi->xbdi_cv);

	while (xbdi->xbdi_status != DISCONNECTED)
		cv_wait(&xbdi->xbdi_cv, &xbdi->xbdi_lock);

	mutex_exit(&xbdi->xbdi_lock);

	xenbus_switch_state(xbdi->xbdi_xbusd, NULL, XenbusStateClosing);
}

static void
xbdback_frontend_changed(void *arg, XenbusState new_state)
{
	struct xbdback_instance *xbdi = arg;
	struct xenbus_device *xbusd = xbdi->xbdi_xbusd;

	XENPRINTF(("xbdback %s: new state %d\n", xbusd->xbusd_path, new_state));
	switch(new_state) {
	case XenbusStateInitialising:
		break;
	case XenbusStateInitialised:
	case XenbusStateConnected:
		if (xbdi->xbdi_status == WAITING || xbdi->xbdi_status == RUN)
			break;
		xbdback_connect(xbdi);
		break;
	case XenbusStateClosing:
		xbdback_disconnect(xbdi);
		break;
	case XenbusStateClosed:
		/* otherend_changed() should handle it for us */
		panic("xbdback_frontend_changed: closed\n");
	case XenbusStateUnknown:
	case XenbusStateInitWait:
	default:
		aprint_error("xbdback %s: invalid frontend state %d\n",
		    xbusd->xbusd_path, new_state);
	}
	return;
}

static void
xbdback_backend_changed(struct xenbus_watch *watch,
    const char **vec, unsigned int len)
{
	struct xenbus_device *xbusd = watch->xbw_dev;
	struct xbdback_instance *xbdi = xbusd->xbusd_u.b.b_cookie;
	int err;
	long dev;
	char *mode;
	struct xenbus_transaction *xbt;
	const char *devname;
	int major;

	err = xenbus_read_ul(NULL, xbusd->xbusd_path, "physical-device",
	    &dev, 10);
	/*
	 * An error can occur as the watch can fire up just after being
	 * registered. So we have to ignore error  :(
	 */
	if (err)
		return;
	/*
	 * we can also fire up after having opened the device, don't try
	 * to do it twice.
	 */
	if (xbdi->xbdi_vp != NULL) {
		if (xbdi->xbdi_status == WAITING || xbdi->xbdi_status == RUN) {
			if (xbdi->xbdi_dev != dev) {
				printf("xbdback %s: changing physical device "
				    "from %#"PRIx64" to %#lx not supported\n",
				    xbusd->xbusd_path, xbdi->xbdi_dev, dev);
			}
		}
		return;
	}
	xbdi->xbdi_dev = dev;
	err = xenbus_read(NULL, xbusd->xbusd_path, "mode", NULL, &mode);
	if (err) {
		printf("xbdback: failed to read %s/mode: %d\n",
		    xbusd->xbusd_path, err);
		return;
	}
	if (mode[0] == 'w')
		xbdi->xbdi_ro = false;
	else
		xbdi->xbdi_ro = true;
	free(mode, M_DEVBUF);
	major = major(xbdi->xbdi_dev);
	devname = devsw_blk2name(major);
	if (devname == NULL) {
		printf("xbdback %s: unknown device 0x%"PRIx64"\n",
		    xbusd->xbusd_path, xbdi->xbdi_dev);
		return;
	}
	xbdi->xbdi_bdevsw = bdevsw_lookup(xbdi->xbdi_dev);
	if (xbdi->xbdi_bdevsw == NULL) {
		printf("xbdback %s: no bdevsw for device 0x%"PRIx64"\n",
		    xbusd->xbusd_path, xbdi->xbdi_dev);
		return;
	}
	err = bdevvp(xbdi->xbdi_dev, &xbdi->xbdi_vp);
	if (err) {
		printf("xbdback %s: can't open device 0x%"PRIx64": %d\n",
		    xbusd->xbusd_path, xbdi->xbdi_dev, err);
		return;
	}
	err = vn_lock(xbdi->xbdi_vp, LK_EXCLUSIVE | LK_RETRY);
	if (err) {
		printf("xbdback %s: can't vn_lock device 0x%"PRIx64": %d\n",
		    xbusd->xbusd_path, xbdi->xbdi_dev, err);
		vrele(xbdi->xbdi_vp);
		return;
	}
	err  = VOP_OPEN(xbdi->xbdi_vp, FREAD, NOCRED);
	if (err) {
		printf("xbdback %s: can't VOP_OPEN device 0x%"PRIx64": %d\n",
		    xbusd->xbusd_path, xbdi->xbdi_dev, err);
		vput(xbdi->xbdi_vp);
		return;
	}
	VOP_UNLOCK(xbdi->xbdi_vp);

	/* dk device; get wedge data */
	struct dkwedge_info wi;
	if ((err = getdiskinfo(xbdi->xbdi_vp, &wi)) == 0) {
		xbdi->xbdi_size = wi.dkw_size;
		printf("xbd backend: attach device %s (size %" PRIu64 ") "
		    "for domain %d\n", wi.dkw_devname, xbdi->xbdi_size,
		    xbdi->xbdi_domid);
	} else {
		/* If both Ioctls failed set device size to 0 and return */
		printf("xbdback %s: can't DIOCGWEDGEINFO device "
		    "0x%"PRIx64": %d\n", xbusd->xbusd_path,
		    xbdi->xbdi_dev, err);		
		xbdi->xbdi_size = xbdi->xbdi_dev = 0;
		vn_close(xbdi->xbdi_vp, FREAD, NOCRED);
		xbdi->xbdi_vp = NULL;
		return;
	}
again:
	xbt = xenbus_transaction_start();
	if (xbt == NULL) {
		printf("xbdback %s: can't start transaction\n",
		    xbusd->xbusd_path);
		    return;
	}
	err = xenbus_printf(xbt, xbusd->xbusd_path, "sectors", "%" PRIu64 ,
	    xbdi->xbdi_size);
	if (err) {
		printf("xbdback: failed to write %s/sectors: %d\n",
		    xbusd->xbusd_path, err);
		goto abort;
	}
	err = xenbus_printf(xbt, xbusd->xbusd_path, "info", "%u",
	    xbdi->xbdi_ro ? VDISK_READONLY : 0);
	if (err) {
		printf("xbdback: failed to write %s/info: %d\n",
		    xbusd->xbusd_path, err);
		goto abort;
	}
	err = xenbus_printf(xbt, xbusd->xbusd_path, "sector-size", "%lu",
	    (u_long)DEV_BSIZE);
	if (err) {
		printf("xbdback: failed to write %s/sector-size: %d\n",
		    xbusd->xbusd_path, err);
		goto abort;
	}
	err = xenbus_printf(xbt, xbusd->xbusd_path, "feature-flush-cache",
	    "%u", 1);
	if (err) {
		printf("xbdback: failed to write %s/feature-flush-cache: %d\n",
		    xbusd->xbusd_path, err);
		goto abort;
	}
	err = xenbus_transaction_end(xbt, 0);
	if (err == EAGAIN)
		goto again;
	if (err) {
		printf("xbdback %s: can't end transaction: %d\n",
		    xbusd->xbusd_path, err);
	}
	err = xenbus_switch_state(xbusd, NULL, XenbusStateConnected);
	if (err) {
		printf("xbdback %s: can't switch state: %d\n",
		    xbusd->xbusd_path, err);
	}
	return;
abort:
	xenbus_transaction_end(xbt, 1);
}

/*
 * Used by a xbdi thread to signal that it is now disconnected.
 */
static void
xbdback_finish_disconnect(struct xbdback_instance *xbdi)
{
	KASSERT(mutex_owned(&xbdi->xbdi_lock));
	KASSERT(xbdi->xbdi_status == DISCONNECTING);

	xbdi->xbdi_status = DISCONNECTED;

	cv_signal(&xbdi->xbdi_cv);
}

static struct xbdback_instance *
xbdif_lookup(domid_t dom , uint32_t handle)
{
	struct xbdback_instance *xbdi;

	SLIST_FOREACH(xbdi, &xbdback_instances, next) {
		if (xbdi->xbdi_domid == dom && xbdi->xbdi_handle == handle)
			return xbdi;
	}
	return NULL;
}

static int
xbdback_evthandler(void *arg)
{
	struct xbdback_instance *xbdi = arg;

	XENPRINTF(("xbdback_evthandler domain %d: cont %p\n",
	    xbdi->xbdi_domid, xbdi->xbdi_cont));

	xbdback_wakeup_thread(xbdi);

	return 1;
}

/*
 * Main thread routine for one xbdback instance. Woken up by
 * xbdback_evthandler when a domain has I/O work scheduled in a I/O ring.
 */
static void
xbdback_thread(void *arg)
{
	struct xbdback_instance *xbdi = arg;

	for (;;) {
		mutex_enter(&xbdi->xbdi_lock);
		switch (xbdi->xbdi_status) {
		case WAITING:
			cv_wait(&xbdi->xbdi_cv, &xbdi->xbdi_lock);
			mutex_exit(&xbdi->xbdi_lock);
			break;
		case RUN:
			xbdi->xbdi_status = WAITING; /* reset state */
			mutex_exit(&xbdi->xbdi_lock);

			if (xbdi->xbdi_cont == NULL) {
				xbdi->xbdi_cont = xbdback_co_main;
			}

			xbdback_trampoline(xbdi, xbdi);
			break;
		case DISCONNECTING:
			if (xbdi->xbdi_pendingreqs > 0) {
				/* there are pending I/Os. Wait for them. */
				cv_wait(&xbdi->xbdi_cv, &xbdi->xbdi_lock);
				mutex_exit(&xbdi->xbdi_lock);
				break;
			}
			
			/* All I/Os should have been processed by now,
			 * xbdi_refcnt should drop to 0 */
			xbdi_put(xbdi);
			KASSERT(xbdi->xbdi_refcnt == 0);
			mutex_exit(&xbdi->xbdi_lock);
			kthread_exit(0);
			break;
		default:
			panic("%s: invalid state %d",
			    xbdi->xbdi_name, xbdi->xbdi_status);
		}
	}
}

static void *
xbdback_co_main(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;

	xbdi->xbdi_req_prod = xbdi->xbdi_ring.ring_n.sring->req_prod;
	xen_rmb(); /* ensure we see all requests up to req_prod */
	/*
	 * note that we'll eventually get a full ring of request.
	 * in this case, MASK_BLKIF_IDX(req_cons) == MASK_BLKIF_IDX(req_prod)
	 */
	xbdi->xbdi_cont = xbdback_co_main_loop;
	return xbdi;
}

/*
 * Fetch a blkif request from the ring, and pass control to the appropriate
 * continuation.
 * If someone asked for disconnection, do not fetch any more request from
 * the ring.
 */
static void *
xbdback_co_main_loop(struct xbdback_instance *xbdi, void *obj) 
{
	blkif_request_t *req;
	blkif_x86_32_request_t *req32;
	blkif_x86_64_request_t *req64;

	(void)obj;
	req = &xbdi->xbdi_xen_req;
	if (xbdi->xbdi_ring.ring_n.req_cons != xbdi->xbdi_req_prod) {
		switch(xbdi->xbdi_proto) {
		case XBDIP_NATIVE:
			memcpy(req, RING_GET_REQUEST(&xbdi->xbdi_ring.ring_n,
			    xbdi->xbdi_ring.ring_n.req_cons),
			    sizeof(blkif_request_t));
			break;
		case XBDIP_32:
			req32 = RING_GET_REQUEST(&xbdi->xbdi_ring.ring_32,
			    xbdi->xbdi_ring.ring_n.req_cons);
			req->operation = req32->operation;
			req->nr_segments = req32->nr_segments;
			req->handle = req32->handle;
			req->id = req32->id;
			req->sector_number = req32->sector_number;
			break;
			    
		case XBDIP_64:
			req64 = RING_GET_REQUEST(&xbdi->xbdi_ring.ring_64,
			    xbdi->xbdi_ring.ring_n.req_cons);
			req->operation = req64->operation;
			req->nr_segments = req64->nr_segments;
			req->handle = req64->handle;
			req->id = req64->id;
			req->sector_number = req64->sector_number;
			break;
		}
		__insn_barrier();
		XENPRINTF(("xbdback op %d req_cons 0x%x req_prod 0x%x "
		    "resp_prod 0x%x id %" PRIu64 "\n", req->operation,
			xbdi->xbdi_ring.ring_n.req_cons,
			xbdi->xbdi_req_prod,
			xbdi->xbdi_ring.ring_n.rsp_prod_pvt,
			req->id));
		switch(req->operation) {
		case BLKIF_OP_READ:
		case BLKIF_OP_WRITE:
			xbdi->xbdi_cont = xbdback_co_io;
			break;
		case BLKIF_OP_FLUSH_DISKCACHE:
			xbdi_get(xbdi);
			xbdi->xbdi_cont = xbdback_co_cache_flush;
			break;
		default:
			if (ratecheck(&xbdi->xbdi_lasterr_time,
			    &xbdback_err_intvl)) {
				printf("%s: unknown operation %d\n",
				    xbdi->xbdi_name, req->operation);
			}
			xbdback_send_reply(xbdi, req->id, req->operation,
			    BLKIF_RSP_ERROR);
			xbdi->xbdi_cont = xbdback_co_main_incr;
			break;
		}
	} else {
		xbdi->xbdi_cont = xbdback_co_main_done;
	}
	return xbdi;
}

/*
 * Increment consumer index and move on to the next request. In case
 * we want to disconnect, leave continuation now.
 */
static void *
xbdback_co_main_incr(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;
	blkif_back_ring_t *ring = &xbdi->xbdi_ring.ring_n;

	ring->req_cons++;

	/*
	 * Do not bother with locking here when checking for xbdi_status: if
	 * we get a transient state, we will get the right value at
	 * the next increment.
	 */
	if (xbdi->xbdi_status == DISCONNECTING)
		xbdi->xbdi_cont = NULL;
	else
		xbdi->xbdi_cont = xbdback_co_main_loop;

	/*
	 * Each time the thread processes a full ring of requests, give
	 * a chance to other threads to process I/Os too
	 */
	if ((ring->req_cons % BLKIF_RING_SIZE) == 0)
		yield();

	return xbdi;
}

/*
 * Ring processing is over. If there are any I/O still present for this
 * instance, handle them first.
 */
static void *
xbdback_co_main_done(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;
	if (xbdi->xbdi_io != NULL) {
		KASSERT(xbdi->xbdi_io->xio_operation == BLKIF_OP_READ ||
		    xbdi->xbdi_io->xio_operation == BLKIF_OP_WRITE);
		xbdi->xbdi_cont = xbdback_co_map_io;
		xbdi->xbdi_cont_aux = xbdback_co_main_done2;
	} else {
		xbdi->xbdi_cont = xbdback_co_main_done2;
	}
	return xbdi;
}

/*
 * Check for requests in the instance's ring. In case there are, start again
 * from the beginning. If not, stall.
 */
static void *
xbdback_co_main_done2(struct xbdback_instance *xbdi, void *obj)
{
	int work_to_do;

	RING_FINAL_CHECK_FOR_REQUESTS(&xbdi->xbdi_ring.ring_n, work_to_do);
	if (work_to_do)
		xbdi->xbdi_cont = xbdback_co_main;
	else
		xbdi->xbdi_cont = NULL;

	return xbdi;
}

/*
 * Frontend requested a cache flush operation.
 */
static void *
xbdback_co_cache_flush(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;

	XENPRINTF(("xbdback_co_cache_flush %p %p\n", xbdi, obj));
	if (xbdi->xbdi_io != NULL) {
		/* Some I/Os are required for this instance. Process them. */
		KASSERT(xbdi->xbdi_io->xio_operation == BLKIF_OP_READ ||
		    xbdi->xbdi_io->xio_operation == BLKIF_OP_WRITE);
		KASSERT(xbdi->xbdi_pendingreqs > 0);
		xbdi->xbdi_cont = xbdback_co_map_io;
		xbdi->xbdi_cont_aux = xbdback_co_cache_flush2;
	} else {
		xbdi->xbdi_cont = xbdback_co_cache_flush2;
	}
	return xbdi;
}

static void *
xbdback_co_cache_flush2(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;
	XENPRINTF(("xbdback_co_cache_flush2 %p %p\n", xbdi, obj));
	if (xbdi->xbdi_pendingreqs > 0) {
		/*
		 * There are pending requests.
		 * Event or iodone() will restart processing
		 */
		xbdi->xbdi_cont = NULL;
		xbdi_put(xbdi);
		return NULL;
	}
	xbdi->xbdi_cont = xbdback_co_cache_doflush;
	return xbdback_pool_get(&xbdback_io_pool, xbdi);
}

/* Start the flush work */
static void *
xbdback_co_cache_doflush(struct xbdback_instance *xbdi, void *obj)
{
	struct xbdback_io *xbd_io;

	XENPRINTF(("xbdback_co_cache_doflush %p %p\n", xbdi, obj));
	xbd_io = xbdi->xbdi_io = obj;
	xbd_io->xio_xbdi = xbdi;
	xbd_io->xio_operation = xbdi->xbdi_xen_req.operation;
	xbd_io->xio_flush_id = xbdi->xbdi_xen_req.id;
	xbdi->xbdi_cont = xbdback_co_do_io;
	return xbdi;
}

/*
 * A read or write I/O request must be processed. Do some checks first,
 * then get the segment information directly from the ring request.
 */
static void *
xbdback_co_io(struct xbdback_instance *xbdi, void *obj)
{	
	int i, error;
	blkif_request_t *req;
	blkif_x86_32_request_t *req32;
	blkif_x86_64_request_t *req64;

	(void)obj;

	/* some sanity checks */
	req = &xbdi->xbdi_xen_req;
	if (req->nr_segments < 1 ||
	    req->nr_segments > BLKIF_MAX_SEGMENTS_PER_REQUEST) {
		if (ratecheck(&xbdi->xbdi_lasterr_time,
		    &xbdback_err_intvl)) {
			printf("%s: invalid number of segments: %d\n",
			       xbdi->xbdi_name,
			       xbdi->xbdi_xen_req.nr_segments);
		}
		error = EINVAL;
		goto end;
	}

	KASSERT(req->operation == BLKIF_OP_READ ||
	    req->operation == BLKIF_OP_WRITE);
	if (req->operation == BLKIF_OP_WRITE) {
		if (xbdi->xbdi_ro) {
			error = EROFS;
			goto end;
		}
	}

	xbdi->xbdi_segno = 0;

	/* copy request segments */
	switch(xbdi->xbdi_proto) {
	case XBDIP_NATIVE:
		/* already copied in xbdback_co_main_loop */
		break;
	case XBDIP_32:
		req32 = RING_GET_REQUEST(&xbdi->xbdi_ring.ring_32,
		    xbdi->xbdi_ring.ring_n.req_cons);
		for (i = 0; i < req->nr_segments; i++)
			req->seg[i] = req32->seg[i];
		break;
	case XBDIP_64:
		req64 = RING_GET_REQUEST(&xbdi->xbdi_ring.ring_64,
		    xbdi->xbdi_ring.ring_n.req_cons);
		for (i = 0; i < req->nr_segments; i++)
			req->seg[i] = req64->seg[i];
		break;
	}

	xbdi->xbdi_cont = xbdback_co_io_gotreq;
	return xbdback_pool_get(&xbdback_request_pool, xbdi);

 end:
	xbdback_send_reply(xbdi, xbdi->xbdi_xen_req.id,
	    xbdi->xbdi_xen_req.operation, error);
	xbdi->xbdi_cont = xbdback_co_main_incr;
	return xbdi;
}

/*
 * We have fetched segment requests from the ring. In case there are already
 * I/Os prepared for this instance, we can try coalescing the requests
 * with these I/Os.
 */
static void *
xbdback_co_io_gotreq(struct xbdback_instance *xbdi, void *obj)
{
	struct xbdback_request *xrq;

	xrq = xbdi->xbdi_req = obj;
	
	xrq->rq_xbdi = xbdi;
	xrq->rq_iocount = 0;
	xrq->rq_ioerrs = 0;
	xrq->rq_id = xbdi->xbdi_xen_req.id;
	xrq->rq_operation = xbdi->xbdi_xen_req.operation;
	KASSERT(xbdi->xbdi_req->rq_operation == BLKIF_OP_READ ||
	    xbdi->xbdi_req->rq_operation == BLKIF_OP_WRITE);

	/* 
	 * Request-level reasons not to coalesce: different device,
	 * different op, or noncontiguous disk sectors (vs. previous
	 * request handed to us).
	 */
	xbdi->xbdi_cont = xbdback_co_io_loop;
	if (xbdi->xbdi_io != NULL) {
		struct xbdback_request *last_req;
		last_req = SLIST_FIRST(&xbdi->xbdi_io->xio_rq)->car;
		XENPRINTF(("xbdback_io domain %d: hoping for sector %" PRIu64
		    "; got %" PRIu64 "\n", xbdi->xbdi_domid,
		    xbdi->xbdi_next_sector,
		    xbdi->xbdi_xen_req.sector_number));
		if ((xrq->rq_operation != last_req->rq_operation)
		    || (xbdi->xbdi_xen_req.sector_number !=
		    xbdi->xbdi_next_sector)) {
			XENPRINTF(("xbdback_io domain %d: segment break\n",
			    xbdi->xbdi_domid));
			xbdi->xbdi_next_sector =
			    xbdi->xbdi_xen_req.sector_number;
			KASSERT(xbdi->xbdi_io->xio_operation == BLKIF_OP_READ ||
			    xbdi->xbdi_io->xio_operation == BLKIF_OP_WRITE);
			xbdi->xbdi_cont_aux = xbdback_co_io_loop;
			xbdi->xbdi_cont = xbdback_co_map_io;
		}
	} else {
		xbdi->xbdi_next_sector = xbdi->xbdi_xen_req.sector_number;
	}
	return xbdi;
}

/* Handle coalescing of multiple segment requests into one I/O work */
static void *
xbdback_co_io_loop(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;
	KASSERT(xbdi->xbdi_req->rq_operation == BLKIF_OP_READ ||
	    xbdi->xbdi_req->rq_operation == BLKIF_OP_WRITE);
	if (xbdi->xbdi_segno < xbdi->xbdi_xen_req.nr_segments) {
		uint8_t this_fs, this_ls, last_ls;
		grant_ref_t thisgrt;
		/* 
		 * Segment-level reason to coalesce: handling full
		 * pages, or adjacent sector ranges from the same page
		 * (and yes, this latter does happen).  But not if the
		 * array of client pseudo-physical pages is full.
		 */
		this_fs = xbdi->xbdi_xen_req.seg[xbdi->xbdi_segno].first_sect;
		this_ls = xbdi->xbdi_xen_req.seg[xbdi->xbdi_segno].last_sect;
		thisgrt = xbdi->xbdi_xen_req.seg[xbdi->xbdi_segno].gref;
		XENPRINTF(("xbdback_io domain %d: "
			   "first,last_sect[%d]=0%o,0%o\n",
			   xbdi->xbdi_domid, xbdi->xbdi_segno,
			   this_fs, this_ls));
		last_ls = xbdi->xbdi_last_ls = xbdi->xbdi_this_ls;
		xbdi->xbdi_this_fs = this_fs;
		xbdi->xbdi_this_ls = this_ls;
		xbdi->xbdi_thisgrt = thisgrt;
		if (xbdi->xbdi_io != NULL) {
			if (last_ls == VBD_MAXSECT
			    && this_fs == 0
			    && xbdi->xbdi_io->xio_nrma
			    < XENSHM_MAX_PAGES_PER_REQUEST) {
				xbdi->xbdi_same_page = 0;
			} else if (last_ls + 1
				       == this_fs
#ifdef notyet
				   && (last_fas & ~PAGE_MASK)
				       == (this_fas & ~PAGE_MASK)
#else 
				  && 0 /* can't know frame number yet */
#endif
			    ) {
#ifdef DEBUG
				if (ratecheck(&xbdi->xbdi_lastfragio_time,
				    &xbdback_fragio_intvl))
					printf("%s: domain is sending"
					    " excessively fragmented I/O\n",
					    xbdi->xbdi_name);
#endif
				printf("xbdback_io: would maybe glue "
				    "same page sec %d (%d->%d)\n",
				    xbdi->xbdi_segno, this_fs, this_ls);
				XENPRINTF(("xbdback_io domain %d: glue same "
				    "page", xbdi->xbdi_domid));
				panic("notyet!");
				xbdi->xbdi_same_page = 1;
			} else {
				KASSERT(xbdi->xbdi_io->xio_operation ==
				     BLKIF_OP_READ ||
				    xbdi->xbdi_io->xio_operation ==
				     BLKIF_OP_WRITE);
				xbdi->xbdi_cont_aux = xbdback_co_io_loop;
				xbdi->xbdi_cont = xbdback_co_map_io;
				return xbdi;
			}
		} else
			xbdi->xbdi_same_page = 0;

		if (xbdi->xbdi_io == NULL) {
			xbdi->xbdi_cont = xbdback_co_io_gotio;
			return xbdback_pool_get(&xbdback_io_pool, xbdi);
		} else {
			xbdi->xbdi_cont = xbdback_co_io_gotio2;
		}
	} else {
		/* done with the loop over segments; get next request */
		xbdi->xbdi_cont = xbdback_co_main_incr;
	}
	return xbdi;
}

/* Prepare an I/O buffer for a xbdback instance */
static void *
xbdback_co_io_gotio(struct xbdback_instance *xbdi, void *obj)
{
	struct xbdback_io *xbd_io;
	vaddr_t start_offset; /* start offset in vm area */
	int buf_flags;

	xbdi_get(xbdi);
	atomic_inc_uint(&xbdi->xbdi_pendingreqs);
	
	xbd_io = xbdi->xbdi_io = obj;
	buf_init(&xbd_io->xio_buf);
	xbd_io->xio_xbdi = xbdi;
	SLIST_INIT(&xbd_io->xio_rq);
	xbd_io->xio_nrma = 0;
	xbd_io->xio_mapped = 0;
	xbd_io->xio_operation = xbdi->xbdi_xen_req.operation;

	start_offset = xbdi->xbdi_this_fs * VBD_BSIZE;
	
	if (xbdi->xbdi_xen_req.operation == BLKIF_OP_WRITE) {
		buf_flags = B_WRITE;
	} else {
		buf_flags = B_READ;
	}

	xbd_io->xio_buf.b_flags = buf_flags;
	xbd_io->xio_buf.b_cflags = 0;
	xbd_io->xio_buf.b_oflags = 0;
	xbd_io->xio_buf.b_iodone = xbdback_iodone;
	xbd_io->xio_buf.b_proc = NULL;
	xbd_io->xio_buf.b_vp = xbdi->xbdi_vp;
	xbd_io->xio_buf.b_objlock = xbdi->xbdi_vp->v_interlock;
	xbd_io->xio_buf.b_dev = xbdi->xbdi_dev;
	xbd_io->xio_buf.b_blkno = xbdi->xbdi_next_sector;
	xbd_io->xio_buf.b_bcount = 0;
	xbd_io->xio_buf.b_data = (void *)start_offset;
	xbd_io->xio_buf.b_private = xbd_io;

	xbdi->xbdi_cont = xbdback_co_io_gotio2;
	return xbdi;
}

/* Manage fragments */
static void *
xbdback_co_io_gotio2(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;
	if (xbdi->xbdi_segno == 0 || SLIST_EMPTY(&xbdi->xbdi_io->xio_rq)) {
		/* if this is the first segment of a new request */
		/* or if it's the first segment of the io */
		xbdi->xbdi_cont = xbdback_co_io_gotfrag;
		return xbdback_pool_get(&xbdback_fragment_pool, xbdi);
	}
	xbdi->xbdi_cont = xbdback_co_io_gotfrag2;
	return xbdi;
}

/* Prepare the instance for its first fragment */
static void *
xbdback_co_io_gotfrag(struct xbdback_instance *xbdi, void *obj)
{
	struct xbdback_fragment *xbd_fr;

	xbd_fr = obj;
	xbd_fr->car = xbdi->xbdi_req;
	SLIST_INSERT_HEAD(&xbdi->xbdi_io->xio_rq, xbd_fr, cdr);
	++xbdi->xbdi_req->rq_iocount;

	xbdi->xbdi_cont = xbdback_co_io_gotfrag2;
	return xbdi;
}

/* Last routine to manage segments fragments for one I/O */
static void *
xbdback_co_io_gotfrag2(struct xbdback_instance *xbdi, void *obj)
{
	struct xbdback_io *xbd_io;
	int seg_size;
	uint8_t this_fs, this_ls;

	this_fs = xbdi->xbdi_this_fs;
	this_ls = xbdi->xbdi_this_ls;
	xbd_io = xbdi->xbdi_io;
	seg_size = this_ls - this_fs + 1;

	if (seg_size < 0) {
		if (ratecheck(&xbdi->xbdi_lasterr_time, &xbdback_err_intvl)) {
			printf("xbdback_io domain %d: negative-size request "
			    "(%d %d)\n",
			    xbdi->xbdi_domid, this_ls, this_fs);
		}
		xbdback_io_error(xbdi->xbdi_io, EINVAL);
		xbdi->xbdi_io = NULL;
		xbdi->xbdi_cont = xbdback_co_main_incr;
		return xbdi;
	}
	
	if (!xbdi->xbdi_same_page) {
		XENPRINTF(("xbdback_io domain %d: appending grant %u\n",
			   xbdi->xbdi_domid, (u_int)xbdi->xbdi_thisgrt));
		xbd_io->xio_gref[xbd_io->xio_nrma++] = xbdi->xbdi_thisgrt;
	}

	xbd_io->xio_buf.b_bcount += (daddr_t)(seg_size * VBD_BSIZE);
	XENPRINTF(("xbdback_io domain %d: start sect %d size %d\n",
	    xbdi->xbdi_domid, (int)xbdi->xbdi_next_sector, seg_size));
	
	/* Finally, the end of the segment loop! */
	xbdi->xbdi_next_sector += seg_size;
	++xbdi->xbdi_segno;
	xbdi->xbdi_cont = xbdback_co_io_loop;
	return xbdi;
}

/*
 * Map the different I/O requests in backend's VA space.
 */
static void *
xbdback_co_map_io(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;
	XENPRINTF(("xbdback_io domain %d: flush sect %ld size %d ptr 0x%lx\n",
	    xbdi->xbdi_domid, (long)xbdi->xbdi_io->xio_buf.b_blkno,
	    (int)xbdi->xbdi_io->xio_buf.b_bcount, (long)xbdi->xbdi_io));
	xbdi->xbdi_cont = xbdback_co_do_io;
	return xbdback_map_shm(xbdi->xbdi_io);
}

static void
xbdback_io_error(struct xbdback_io *xbd_io, int error)
{
	xbd_io->xio_buf.b_error = error;
	xbdback_iodone(&xbd_io->xio_buf);
}

/*
 * Main xbdback I/O routine. It can either perform a flush operation or
 * schedule a read/write operation.
 */
static void *
xbdback_co_do_io(struct xbdback_instance *xbdi, void *obj)
{
	struct xbdback_io *xbd_io = xbdi->xbdi_io;

	switch (xbd_io->xio_operation) {
	case BLKIF_OP_FLUSH_DISKCACHE:
	{
		int error;
		int force = 1;

		error = VOP_IOCTL(xbdi->xbdi_vp, DIOCCACHESYNC, &force, FWRITE,
		    kauth_cred_get());
		if (error) {
			aprint_error("xbdback %s: DIOCCACHESYNC returned %d\n",
			    xbdi->xbdi_xbusd->xbusd_path, error);
			 if (error == EOPNOTSUPP || error == ENOTTY)
				error = BLKIF_RSP_EOPNOTSUPP;
			 else
				error = BLKIF_RSP_ERROR;
		} else
			error = BLKIF_RSP_OKAY;
		xbdback_send_reply(xbdi, xbd_io->xio_flush_id,
		    xbd_io->xio_operation, error);
		xbdback_pool_put(&xbdback_io_pool, xbd_io);
		xbdi_put(xbdi);
		xbdi->xbdi_io = NULL;
		xbdi->xbdi_cont = xbdback_co_main_incr;
		return xbdi;
	}
	case BLKIF_OP_READ:
	case BLKIF_OP_WRITE:
		xbd_io->xio_buf.b_data = (void *)
		    ((vaddr_t)xbd_io->xio_buf.b_data + xbd_io->xio_vaddr);
#ifdef DIAGNOSTIC
		{
		vaddr_t bdata = (vaddr_t)xbd_io->xio_buf.b_data;
		int nsegs =
		    ((((bdata + xbd_io->xio_buf.b_bcount - 1) & ~PAGE_MASK) -
		    (bdata & ~PAGE_MASK)) >> PAGE_SHIFT) + 1;
		if ((bdata & ~PAGE_MASK) != (xbd_io->xio_vaddr & ~PAGE_MASK)) {
			printf("xbdback_co_do_io: vaddr %#" PRIxVADDR
			    " bdata %#" PRIxVADDR "\n",
			    xbd_io->xio_vaddr, bdata);
			panic("xbdback_co_do_io: bdata page change");
		}
		if (nsegs > xbd_io->xio_nrma) {
			printf("xbdback_co_do_io: vaddr %#" PRIxVADDR
			    " bcount %#x doesn't fit in %d pages\n",
			    bdata, xbd_io->xio_buf.b_bcount, xbd_io->xio_nrma);
			panic("xbdback_co_do_io: not enough pages");
		}
		}
#endif
		if ((xbd_io->xio_buf.b_flags & B_READ) == 0) {
			mutex_enter(xbd_io->xio_buf.b_vp->v_interlock);
			xbd_io->xio_buf.b_vp->v_numoutput++;
			mutex_exit(xbd_io->xio_buf.b_vp->v_interlock);
		}
		bdev_strategy(&xbd_io->xio_buf);
		/* will call xbdback_iodone() asynchronously when done */
		xbdi->xbdi_io = NULL;
		xbdi->xbdi_cont = xbdi->xbdi_cont_aux;
		return xbdi;
	default:
		/* Should never happen */
		panic("xbdback_co_do_io: unsupported operation %d",
		    xbd_io->xio_operation);
	}
}

/*
 * Called from softint(9) context when an I/O is done: for each request, send
 * back the associated reply to the domain.
 *
 * This gets reused by xbdback_io_error to report errors from other sources.
 */
static void
xbdback_iodone(struct buf *bp)
{
	struct xbdback_io *xbd_io;
	struct xbdback_instance *xbdi;
	int errp;

	xbd_io = bp->b_private;
	xbdi = xbd_io->xio_xbdi;

	XENPRINTF(("xbdback_io domain %d: iodone ptr 0x%lx\n",
		   xbdi->xbdi_domid, (long)xbd_io));

	if (xbd_io->xio_mapped == 1)
		xbdback_unmap_shm(xbd_io);

	if (bp->b_error != 0) {
		printf("xbd IO domain %d: error %d\n",
		       xbdi->xbdi_domid, bp->b_error);
		errp = 1;
	} else
		errp = 0;
	
	/* for each constituent xbd request */
	while(!SLIST_EMPTY(&xbd_io->xio_rq)) {
		struct xbdback_fragment *xbd_fr;
		struct xbdback_request *xbd_req;
		struct xbdback_instance *rxbdi __diagused;
		int error;
		
		xbd_fr = SLIST_FIRST(&xbd_io->xio_rq);
		xbd_req = xbd_fr->car;
		SLIST_REMOVE_HEAD(&xbd_io->xio_rq, cdr);
		xbdback_pool_put(&xbdback_fragment_pool, xbd_fr);
		
		if (errp)
			++xbd_req->rq_ioerrs;
		
		/* finalize it only if this was its last I/O */
		if (--xbd_req->rq_iocount > 0)
			continue;

		rxbdi = xbd_req->rq_xbdi;
		KASSERT(xbdi == rxbdi);
		
		error = xbd_req->rq_ioerrs > 0
		    ? BLKIF_RSP_ERROR
		    : BLKIF_RSP_OKAY;

		XENPRINTF(("xbdback_io domain %d: end request %"PRIu64
		    "error=%d\n",
		    xbdi->xbdi_domid, xbd_req->rq_id, error));
		xbdback_send_reply(xbdi, xbd_req->rq_id,
		    xbd_req->rq_operation, error);
		xbdback_pool_put(&xbdback_request_pool, xbd_req);
	}
	xbdi_put(xbdi);
	atomic_dec_uint(&xbdi->xbdi_pendingreqs);
	buf_destroy(&xbd_io->xio_buf);
	xbdback_pool_put(&xbdback_io_pool, xbd_io);

	xbdback_wakeup_thread(xbdi);
}

/*
 * Wake up the per xbdback instance thread.
 */
static void
xbdback_wakeup_thread(struct xbdback_instance *xbdi)
{

	mutex_enter(&xbdi->xbdi_lock);
	/* only set RUN state when we are WAITING for work */
	if (xbdi->xbdi_status == WAITING)
	       xbdi->xbdi_status = RUN;
	cv_broadcast(&xbdi->xbdi_cv);
	mutex_exit(&xbdi->xbdi_lock);
}

/*
 * called once a request has completed. Place the reply in the ring and
 * notify the guest OS.
 */
static void
xbdback_send_reply(struct xbdback_instance *xbdi, uint64_t id,
    int op, int status)
{
	blkif_response_t *resp_n;
	blkif_x86_32_response_t *resp32;
	blkif_x86_64_response_t *resp64;
	int notify;

	/*
	 * The ring can be accessed by the xbdback thread, xbdback_iodone()
	 * handler, or any handler that triggered the shm callback. So
	 * protect ring access via the xbdi_lock mutex.
	 */
	mutex_enter(&xbdi->xbdi_lock);
	switch (xbdi->xbdi_proto) {
	case XBDIP_NATIVE:
		resp_n = RING_GET_RESPONSE(&xbdi->xbdi_ring.ring_n,
		    xbdi->xbdi_ring.ring_n.rsp_prod_pvt);
		resp_n->id        = id;
		resp_n->operation = op;
		resp_n->status    = status;
		break;
	case XBDIP_32:
		resp32 = RING_GET_RESPONSE(&xbdi->xbdi_ring.ring_32,
		    xbdi->xbdi_ring.ring_n.rsp_prod_pvt);
		resp32->id        = id;
		resp32->operation = op;
		resp32->status    = status;
		break;
	case XBDIP_64:
		resp64 = RING_GET_RESPONSE(&xbdi->xbdi_ring.ring_64,
		    xbdi->xbdi_ring.ring_n.rsp_prod_pvt);
		resp64->id        = id;
		resp64->operation = op;
		resp64->status    = status;
		break;
	}
	xbdi->xbdi_ring.ring_n.rsp_prod_pvt++;
	RING_PUSH_RESPONSES_AND_CHECK_NOTIFY(&xbdi->xbdi_ring.ring_n, notify);
	mutex_exit(&xbdi->xbdi_lock);

	if (notify) {
		XENPRINTF(("xbdback_send_reply notify %d\n", xbdi->xbdi_domid));
		hypervisor_notify_via_evtchn(xbdi->xbdi_evtchn);
	}
}

/*
 * Map multiple entries of an I/O request into backend's VA space.
 * The xbd_io->xio_gref array has to be filled out by the caller.
 */
static void *
xbdback_map_shm(struct xbdback_io *xbd_io)
{
	struct xbdback_instance *xbdi;
	struct xbdback_request *xbd_rq;
	int error, s;

#ifdef XENDEBUG_VBD
	int i;
	printf("xbdback_map_shm map grant ");
	for (i = 0; i < xbd_io->xio_nrma; i++) {
		printf("%u ", (u_int)xbd_io->xio_gref[i]);
	}
#endif

	KASSERT(xbd_io->xio_mapped == 0);

	xbdi = xbd_io->xio_xbdi;
	xbd_rq = SLIST_FIRST(&xbd_io->xio_rq)->car;

	error = xen_shm_map(xbd_io->xio_nrma, xbdi->xbdi_domid,
	    xbd_io->xio_gref, &xbd_io->xio_vaddr, xbd_io->xio_gh, 
	    (xbd_rq->rq_operation == BLKIF_OP_WRITE) ? XSHM_RO : 0);

	switch(error) {
	case 0:
#ifdef XENDEBUG_VBD
		printf("handle ");
		for (i = 0; i < xbd_io->xio_nrma; i++) {
			printf("%u ", (u_int)xbd_io->xio_gh[i]);
		}
		printf("\n");
#endif
		xbd_io->xio_mapped = 1;
		return xbdi;
	case ENOMEM:
		s = splvm();
		if (!xbdback_shmcb) {
			if (xen_shm_callback(xbdback_shm_callback, xbdi)
			    != 0) {
				splx(s);
				panic("xbdback_map_shm: "
				      "xen_shm_callback failed");
			}
			xbdback_shmcb = 1;
		}
		SIMPLEQ_INSERT_TAIL(&xbdback_shmq, xbdi, xbdi_on_hold);
		splx(s);
		/* Put the thread to sleep until the callback is called */
		xbdi->xbdi_cont = xbdback_co_wait_shm_callback;
		return NULL;
	default:
		if (ratecheck(&xbdi->xbdi_lasterr_time, &xbdback_err_intvl)) {
			printf("xbdback_map_shm: xen_shm error %d ", error);
		}
		xbdback_io_error(xbdi->xbdi_io, error);
		xbdi->xbdi_io = NULL;
		xbdi->xbdi_cont = xbdi->xbdi_cont_aux;
		return xbdi;
	}
}

static int
xbdback_shm_callback(void *arg)
{
        int error, s;

	/*
	 * The shm callback may be executed at any level, including
	 * IPL_BIO and IPL_NET levels. Raise to the lowest priority level
	 * that can mask both.
	 */
	s = splvm();
	while(!SIMPLEQ_EMPTY(&xbdback_shmq)) {
		struct xbdback_instance *xbdi;
		struct xbdback_io *xbd_io;
		struct xbdback_request *xbd_rq;
		
		xbdi = SIMPLEQ_FIRST(&xbdback_shmq);
		xbd_io = xbdi->xbdi_io;
		xbd_rq = SLIST_FIRST(&xbd_io->xio_rq)->car;
		KASSERT(xbd_io->xio_mapped == 0);
		
		error = xen_shm_map(xbd_io->xio_nrma,
		    xbdi->xbdi_domid, xbd_io->xio_gref,
		    &xbd_io->xio_vaddr, xbd_io->xio_gh, 
		    XSHM_CALLBACK |
		    ((xbd_rq->rq_operation == BLKIF_OP_WRITE) ? XSHM_RO: 0));
		switch(error) {
		case ENOMEM:
			splx(s);
			return -1; /* will try again later */
		case 0:
			SIMPLEQ_REMOVE_HEAD(&xbdback_shmq, xbdi_on_hold);
			xbd_io->xio_mapped = 1;
			xbdback_wakeup_thread(xbdi);
			break;
		default:
			SIMPLEQ_REMOVE_HEAD(&xbdback_shmq, xbdi_on_hold);
			printf("xbdback_shm_callback: xen_shm error %d\n",
			       error);
			xbdback_io_error(xbd_io, error);
			xbdi->xbdi_io = NULL;
			xbdback_wakeup_thread(xbdi);
			break;
		}
	}
	xbdback_shmcb = 0;
	splx(s);
	return 0;
}

/*
 * Allows waiting for the shm callback to complete.
 */
static void *
xbdback_co_wait_shm_callback(struct xbdback_instance *xbdi, void *obj)
{

	if (xbdi->xbdi_io == NULL || xbdi->xbdi_io->xio_mapped == 1) {
		/*
		 * Only proceed to next step when the callback reported
		 * success or failure.
		 */
		xbdi->xbdi_cont = xbdi->xbdi_cont_aux;
		return xbdi;
	} else {
		/* go back to sleep */
		return NULL;
	}
}

/* unmap a request from our virtual address space (request is done) */
static void
xbdback_unmap_shm(struct xbdback_io *xbd_io)
{
#ifdef XENDEBUG_VBD
	int i;
	printf("xbdback_unmap_shm handle ");
	for (i = 0; i < xbd_io->xio_nrma; i++) {
		printf("%u ", (u_int)xbd_io->xio_gh[i]);
	}
	printf("\n");
#endif

	KASSERT(xbd_io->xio_mapped == 1);
	xbd_io->xio_mapped = 0;
	xen_shm_unmap(xbd_io->xio_vaddr, xbd_io->xio_nrma,
	    xbd_io->xio_gh);
	xbd_io->xio_vaddr = -1;
}

/* Obtain memory from a pool */
static void *
xbdback_pool_get(struct xbdback_pool *pp,
			      struct xbdback_instance *xbdi)
{
	return pool_cache_get(&pp->pc, PR_WAITOK);
}

/* Restore memory to a pool */
static void
xbdback_pool_put(struct xbdback_pool *pp, void *item)
{
	pool_cache_put(&pp->pc, item);
}

/*
 * Trampoline routine. Calls continuations in a loop and only exits when
 * either the returned object or the next callback is NULL.
 */
static void
xbdback_trampoline(struct xbdback_instance *xbdi, void *obj)
{
	xbdback_cont_t cont;

	while(obj != NULL && xbdi->xbdi_cont != NULL) {
		cont = xbdi->xbdi_cont;
#ifdef DIAGNOSTIC
		xbdi->xbdi_cont = (xbdback_cont_t)0xDEADBEEF;
#endif
		obj = (*cont)(xbdi, obj);
#ifdef DIAGNOSTIC
		if (xbdi->xbdi_cont == (xbdback_cont_t)0xDEADBEEF) {
			printf("xbdback_trampoline: 0x%lx didn't set "
			       "xbdi->xbdi_cont!\n", (long)cont);
			panic("xbdback_trampoline: bad continuation");
		}
#endif
	}
}
