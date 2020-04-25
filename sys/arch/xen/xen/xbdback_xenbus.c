/*      $NetBSD: xbdback_xenbus.c,v 1.77.2.4 2020/04/25 13:16:48 bouyer Exp $      */

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
__KERNEL_RCSID(0, "$NetBSD: xbdback_xenbus.c,v 1.77.2.4 2020/04/25 13:16:48 bouyer Exp $");

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
#include <xen/xenring.h>
#include <xen/include/public/io/protocols.h>

/* #define XENDEBUG_VBD */
#ifdef XENDEBUG_VBD
#define XENPRINTF(x) printf x
#else
#define XENPRINTF(x)
#endif

#define BLKIF_RING_SIZE __CONST_RING_SIZE(blkif, PAGE_SIZE)

/*
 * Backend block device driver for Xen
 */

/* Values are expressed in 512-byte sectors */
#define VBD_BSIZE 512
#define VBD_MAXSECT ((PAGE_SIZE / VBD_BSIZE) - 1)

/* Need to alloc one extra page to account for possible mapping offset */
#define VBD_VA_SIZE	(MAXPHYS + PAGE_SIZE)
#define VBD_MAX_INDIRECT_SEGMENTS	VBD_VA_SIZE >> PAGE_SHIFT

CTASSERT(XENSHM_MAX_PAGES_PER_REQUEST >= VBD_MAX_INDIRECT_SEGMENTS);

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
 * Continuation state is "stored" in the xbdback instance (xbdi_cont),
 * and should only be manipulated by the instance thread.
 *
 * As xbdback(4) has to handle different sort of asynchronous events (Xen
 * event channels, biointr() soft interrupts, xenbus commands), the xbdi_lock
 * mutex is used to protect specific elements of the xbdback instance from
 * concurrent access: thread status and ring access (when pushing responses).
 * 
 * Here's how the call graph is supposed to be for a single I/O:
 *
 * xbdback_co_main()
 *        |               --> xbdback_co_cache_flush()
 *        |               |    |
 *        |               |    -> xbdback_co_cache_doflush() or NULL
 *        |               |        |
 *        |               |        -> xbdback_co_do_io()
 * xbdback_co_main_loop()-|
 *        |               |-> xbdback_co_main_done2() or NULL
 *        |               |
 *        |               --> xbdback_co_main_incr() -> xbdback_co_main_loop()
 *        |
 *     xbdback_co_io() -> xbdback_co_main_incr() -> xbdback_co_main_loop()
 *        |
 *     xbdback_co_io_gotio() -> xbdback_map_shm()
 *        |                     |
 *        |                     xbdback_co_main_incr() -> xbdback_co_main_loop()
 *        |
 *     xbdback_co_do_io() 
 *        |
 *     xbdback_co_main_incr() -> xbdback_co_main_loop()
 */
typedef void *(* xbdback_cont_t)(struct xbdback_instance *, void *);

enum xbdi_proto {
	XBDIP_NATIVE,
	XBDIP_32,
	XBDIP_64
};

struct xbdback_va {
	SLIST_ENTRY(xbdback_va) xv_next;
	vaddr_t xv_vaddr;
};

/*
 * For each I/O operation associated with one of those requests, an
 * xbdback_io is allocated from a pool.  It may correspond to multiple
 * Xen disk requests, or parts of them, if several arrive at once that
 * can be coalesced.
 */
struct xbdback_io {
	SLIST_ENTRY(xbdback_io) xio_next;
	/* The instance pointer is duplicated for convenience. */
	struct xbdback_instance *xio_xbdi; /* our xbd instance */
	uint8_t xio_operation;
	uint64_t xio_id;
	union {
		struct {
			struct buf xio_buf; /* our I/O */
			/* the virtual address to map the request at */
			vaddr_t xio_vaddr;
			struct xbdback_va *xio_xv;
			vaddr_t xio_start_offset;	/* I/O start offset */
			/* grants to map */
			grant_ref_t xio_gref[VBD_MAX_INDIRECT_SEGMENTS];
			/* grants release */
			grant_handle_t xio_gh[VBD_MAX_INDIRECT_SEGMENTS];
			uint16_t xio_nrma; /* number of guest pages */
		} xio_rw;
	} u;
};
#define xio_buf		u.xio_rw.xio_buf
#define xio_vaddr	u.xio_rw.xio_vaddr
#define xio_start_offset	u.xio_rw.xio_start_offset
#define xio_xv		u.xio_rw.xio_xv
#define xio_gref	u.xio_rw.xio_gref
#define xio_gh		u.xio_rw.xio_gh
#define xio_nrma	u.xio_rw.xio_nrma

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
	/* context and KVA for mapping transfers */
	struct xbdback_io xbdi_io[BLKIF_RING_SIZE];
	SLIST_HEAD(, xbdback_io) xbdi_io_free;
	struct xbdback_va xbdi_va[BLKIF_RING_SIZE];
	SLIST_HEAD(, xbdback_va) xbdi_va_free;
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
	xbdback_cont_t xbdi_cont;
	/* _request state: track requests fetched from ring */
	blkif_request_t xbdi_xen_req;
	struct blkif_request_segment xbdi_seg[VBD_MAX_INDIRECT_SEGMENTS];
	bus_dmamap_t xbdi_seg_dmamap;
	grant_ref_t xbdi_in_gntref;
	/* other state */
	uint xbdi_pendingreqs; /* number of I/O in fly */
	struct timeval xbdi_lasterr_time;    /* error time tracking */
};
/* Manipulation of the above reference count. */
#define xbdi_get(xbdip) (xbdip)->xbdi_refcnt++
#define xbdi_put(xbdip)						\
do {								\
	if (--((xbdip)->xbdi_refcnt) == 0)  			\
               xbdback_finish_disconnect(xbdip);		\
} while (/* CONSTCOND */ 0)

static SLIST_HEAD(, xbdback_instance) xbdback_instances;
static kmutex_t xbdback_lock;

/* Interval between reports of I/O errors from frontend */
static const struct timeval xbdback_err_intvl = { 1, 0 };

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

static bool xbdif_lookup(domid_t, uint32_t);

static void *xbdback_co_main(struct xbdback_instance *, void *);
static void *xbdback_co_main_loop(struct xbdback_instance *, void *);
static void *xbdback_co_main_incr(struct xbdback_instance *, void *);
static void *xbdback_co_main_done2(struct xbdback_instance *, void *);

static void *xbdback_co_cache_flush(struct xbdback_instance *, void *);
static void *xbdback_co_cache_doflush(struct xbdback_instance *, void *);

static void *xbdback_co_io(struct xbdback_instance *, void *);
static void *xbdback_co_io_gotio(struct xbdback_instance *, void *);

static void *xbdback_co_do_io(struct xbdback_instance *, void *);

static void xbdback_io_error(struct xbdback_io *, int);
static void xbdback_iodone(struct buf *);
static void xbdback_iodone_locked(struct xbdback_instance *,
		struct xbdback_io *, struct buf *);
static void xbdback_send_reply(struct xbdback_instance *, uint64_t , int , int);

static void *xbdback_map_shm(struct xbdback_io *);
static void xbdback_unmap_shm(struct xbdback_io *);

static struct xbdback_io *xbdback_io_get(struct xbdback_instance *);
static void xbdback_io_put(struct xbdback_instance *, struct xbdback_io *);
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
	mutex_init(&xbdback_lock, MUTEX_DEFAULT, IPL_NONE);

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

	xbdi = kmem_zalloc(sizeof(*xbdi), KM_SLEEP);

	xbdi->xbdi_domid = domid;
	xbdi->xbdi_handle = handle;
	snprintf(xbdi->xbdi_name, sizeof(xbdi->xbdi_name), "xbdb%di%d",
	    xbdi->xbdi_domid, xbdi->xbdi_handle);

	mutex_enter(&xbdback_lock);
	if (xbdif_lookup(domid, handle)) {
		mutex_exit(&xbdback_lock);
		kmem_free(xbdi, sizeof(*xbdi));
		return EEXIST;
	}
	SLIST_INSERT_HEAD(&xbdback_instances, xbdi, next);
	mutex_exit(&xbdback_lock);

	/* initialize status and reference counter */
	xbdi->xbdi_status = DISCONNECTED;
	xbdi_get(xbdi);

	mutex_init(&xbdi->xbdi_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&xbdi->xbdi_cv, xbdi->xbdi_name);

	xbusd->xbusd_u.b.b_cookie = xbdi;	
	xbusd->xbusd_u.b.b_detach = xbdback_xenbus_destroy;
	xbusd->xbusd_otherend_changed = xbdback_frontend_changed;
	xbdi->xbdi_xbusd = xbusd;

	if (bus_dmamap_create(xbdi->xbdi_xbusd->xbusd_dmat, PAGE_SIZE,
	    1, PAGE_SIZE, PAGE_SIZE, BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW,
	    &xbdi->xbdi_seg_dmamap) != 0) {
		printf("%s: can't create dma map for indirect segments\n",
		    xbdi->xbdi_name);
		goto fail;
	}
	if (bus_dmamap_load(xbdi->xbdi_xbusd->xbusd_dmat,
	    xbdi->xbdi_seg_dmamap, xbdi->xbdi_seg,
	    sizeof(xbdi->xbdi_seg), NULL, BUS_DMA_WAITOK) != 0) {
		printf("%s: can't load dma map for indirect segments\n",
		    xbdi->xbdi_name);
		goto fail;
	}
	KASSERT(xbdi->xbdi_seg_dmamap->dm_nsegs == 1);

	SLIST_INIT(&xbdi->xbdi_va_free);
	for (i = 0; i < BLKIF_RING_SIZE; i++) {
		xbdi->xbdi_va[i].xv_vaddr = uvm_km_alloc(kernel_map,
		    VBD_VA_SIZE, 0, UVM_KMF_VAONLY|UVM_KMF_WAITVA);
		SLIST_INSERT_HEAD(&xbdi->xbdi_va_free, &xbdi->xbdi_va[i],
		    xv_next);
	}

	SLIST_INIT(&xbdi->xbdi_io_free);
	for (i = 0; i < BLKIF_RING_SIZE; i++) {
		SLIST_INSERT_HEAD(&xbdi->xbdi_io_free, &xbdi->xbdi_io[i],
		    xio_next);
	}

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
	if (xbdi->xbdi_watch.node)
		xenbus_unwatch_path(&xbdi->xbdi_watch);

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
	mutex_enter(&xbdback_lock);
	SLIST_REMOVE(&xbdback_instances, xbdi, xbdback_instance, next);
	mutex_exit(&xbdback_lock);

	for (int i = 0; i < BLKIF_RING_SIZE; i++) {
		if (xbdi->xbdi_va[i].xv_vaddr != 0) {
			uvm_km_free(kernel_map, xbdi->xbdi_va[i].xv_vaddr,
			    VBD_VA_SIZE, UVM_KMF_VAONLY);
			xbdi->xbdi_va[i].xv_vaddr = 0;
		}
	}

	bus_dmamap_unload(xbdi->xbdi_xbusd->xbusd_dmat, xbdi->xbdi_seg_dmamap);
	bus_dmamap_destroy(xbdi->xbdi_xbusd->xbusd_dmat, xbdi->xbdi_seg_dmamap);

	mutex_destroy(&xbdi->xbdi_lock);
	cv_destroy(&xbdi->xbdi_cv);
	kmem_free(xbdi, sizeof(*xbdi));
	return 0;
}

static int
xbdback_connect(struct xbdback_instance *xbdi)
{
	int err;
	struct gnttab_map_grant_ref grop;
	struct gnttab_unmap_grant_ref ungrop;
	evtchn_op_t evop;
	u_long ring_ref, revtchn;
	char xsproto[32];
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
	    xsproto, sizeof(xsproto));
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
			return -1;
		}
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

	xbdi->xbdi_ih = xen_intr_establish_xname(-1, &xen_pic, xbdi->xbdi_evtchn,
	    IST_LEVEL, IPL_BIO, xbdback_evthandler, xbdi, false,
	    xbdi->xbdi_name);
	KASSERT(xbdi->xbdi_ih != NULL);
	aprint_verbose("xbd backend domain %d handle %#x (%d) "
	    "using event channel %d, protocol %s\n", xbdi->xbdi_domid,
	    xbdi->xbdi_handle, xbdi->xbdi_handle, xbdi->xbdi_evtchn, proto);

	/* enable the xbdback event handler machinery */
	xbdi->xbdi_status = WAITING;
	hypervisor_unmask_event(xbdi->xbdi_evtchn);
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
	xen_intr_disestablish(xbdi->xbdi_ih);

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
	char mode[32];
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
	err = xenbus_read(NULL, xbusd->xbusd_path, "mode", mode, sizeof(mode));
	if (err) {
		printf("xbdback: failed to read %s/mode: %d\n",
		    xbusd->xbusd_path, err);
		return;
	}
	if (mode[0] == 'w')
		xbdi->xbdi_ro = false;
	else
		xbdi->xbdi_ro = true;
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
	err = xenbus_printf(xbt, xbusd->xbusd_path,
	    "feature-max-indirect-segments", "%u", VBD_MAX_INDIRECT_SEGMENTS);
	if (err) {
		printf("xbdback: failed to write %s/feature-indirect: %d\n",
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

	cv_broadcast(&xbdi->xbdi_cv);
}

static bool
xbdif_lookup(domid_t dom , uint32_t handle)
{
	struct xbdback_instance *xbdi;
	bool found = false;

	KASSERT(mutex_owned(&xbdback_lock));

	SLIST_FOREACH(xbdi, &xbdback_instances, next) {
		if (xbdi->xbdi_domid == dom && xbdi->xbdi_handle == handle) {
			found = true;
			break;
		}
	}

	return found;
}

static int
xbdback_evthandler(void *arg)
{
	struct xbdback_instance *xbdi = arg;

	XENPRINTF(("xbdback_evthandler domain %d: cont %p\n",
	    xbdi->xbdi_domid, xbdi->xbdi_cont));

	mutex_enter(&xbdi->xbdi_lock);
	xbdback_wakeup_thread(xbdi);
	mutex_exit(&xbdi->xbdi_lock);

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

	mutex_enter(&xbdi->xbdi_lock);
	for (;;) {
		switch (xbdi->xbdi_status) {
		case WAITING:
			cv_wait(&xbdi->xbdi_cv, &xbdi->xbdi_lock);
			break;
		case RUN:
			xbdi->xbdi_status = WAITING; /* reset state */

			if (xbdi->xbdi_cont == NULL) {
				xbdi->xbdi_cont = xbdback_co_main;
			}

			xbdback_trampoline(xbdi, xbdi);
			break;
		case DISCONNECTING:
			if (xbdi->xbdi_pendingreqs > 0) {
				/* there are pending I/Os. Wait for them. */
				cv_wait(&xbdi->xbdi_cv, &xbdi->xbdi_lock);
				continue;
			}
			
			/* All I/Os should have been processed by now,
			 * xbdi_refcnt should drop to 0 */
			xbdi_put(xbdi);
			KASSERT(xbdi->xbdi_refcnt == 0);
			goto out;
			/* NOTREACHED */
		default:
			panic("%s: invalid state %d",
			    xbdi->xbdi_name, xbdi->xbdi_status);
		}
	}
out:
	mutex_exit(&xbdi->xbdi_lock);

	kthread_exit(0);
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
xbdback_co_main_loop(struct xbdback_instance *xbdi, void *obj __unused) 
{
	blkif_request_t *req, *reqn;
	blkif_x86_32_request_t *req32;
	blkif_x86_64_request_t *req64;
	blkif_request_indirect_t *rin;

	if (xbdi->xbdi_ring.ring_n.req_cons != xbdi->xbdi_req_prod) {
		req = &xbdi->xbdi_xen_req;
		memset(req, 0, sizeof(*req));

		switch(xbdi->xbdi_proto) {
		case XBDIP_NATIVE:
			reqn = RING_GET_REQUEST(&xbdi->xbdi_ring.ring_n,
			    xbdi->xbdi_ring.ring_n.req_cons);
			req->operation = reqn->operation;
			req->id = reqn->id;
			break;
		case XBDIP_32:
			req32 = RING_GET_REQUEST(&xbdi->xbdi_ring.ring_32,
			    xbdi->xbdi_ring.ring_n.req_cons);
			req->operation = req32->operation;
			req->id = req32->id;
			break;
		case XBDIP_64:
			req64 = RING_GET_REQUEST(&xbdi->xbdi_ring.ring_64,
			    xbdi->xbdi_ring.ring_n.req_cons);
			req->operation = req64->operation;
			req->id = req64->id;
			break;
		}
		__insn_barrier();
		XENPRINTF(("xbdback op %d req_cons 0x%x req_prod 0x%x "
		    "resp_prod 0x%x id %" PRIu64 "\n", req->operation,
			xbdi->xbdi_ring.ring_n.req_cons,
			xbdi->xbdi_req_prod,
			xbdi->xbdi_ring.ring_n.rsp_prod_pvt,
			req->id));
		switch (req->operation) {
		case BLKIF_OP_INDIRECT:
			/* just check indirect_op, rest is handled later */
			rin = (blkif_request_indirect_t *)
			    RING_GET_REQUEST(&xbdi->xbdi_ring.ring_n,
				xbdi->xbdi_ring.ring_n.req_cons);
			if (rin->indirect_op != BLKIF_OP_READ &&
			    rin->indirect_op != BLKIF_OP_WRITE) {
				if (ratecheck(&xbdi->xbdi_lasterr_time,
				    &xbdback_err_intvl)) {
					printf("%s: unknown ind operation %d\n",
					    xbdi->xbdi_name,
					    rin->indirect_op);
				}
				goto fail;
			}
			/* FALLTHROUGH */
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
fail:
			xbdback_send_reply(xbdi, req->id, req->operation,
			    BLKIF_RSP_ERROR);
			xbdi->xbdi_cont = xbdback_co_main_incr;
			break;
		}
	} else {
		xbdi->xbdi_cont = xbdback_co_main_done2;
	}
	return xbdi;
}

/*
 * Increment consumer index and move on to the next request. In case
 * we want to disconnect, leave continuation now.
 */
static void *
xbdback_co_main_incr(struct xbdback_instance *xbdi, void *obj __unused)
{
	KASSERT(mutex_owned(&xbdi->xbdi_lock));

	blkif_back_ring_t *ring = &xbdi->xbdi_ring.ring_n;

	ring->req_cons++;

	if (xbdi->xbdi_status == DISCONNECTING)
		xbdi->xbdi_cont = NULL;
	else
		xbdi->xbdi_cont = xbdback_co_main_loop;

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
xbdback_co_cache_flush(struct xbdback_instance *xbdi, void *obj __unused)
{
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
	return xbdback_io_get(xbdi);
}

/* Start the flush work */
static void *
xbdback_co_cache_doflush(struct xbdback_instance *xbdi, void *obj)
{
	struct xbdback_io *xbd_io;

	XENPRINTF(("xbdback_co_cache_doflush %p %p\n", xbdi, obj));
	xbd_io = obj;
	xbd_io->xio_xbdi = xbdi;
	xbd_io->xio_operation = xbdi->xbdi_xen_req.operation;
	xbd_io->xio_id = xbdi->xbdi_xen_req.id;
	xbdi->xbdi_cont = xbdback_co_do_io;
	return xbd_io;
}

/*
 * A read or write I/O request must be processed. Do some checks first,
 * then get the segment information directly from the ring request.
 */
static void *
xbdback_co_io(struct xbdback_instance *xbdi, void *obj __unused)
{	
	int i, error;
	blkif_request_t *req, *reqn;
	blkif_x86_32_request_t *req32;
	blkif_x86_64_request_t *req64;
	blkif_request_indirect_t *rinn;
	blkif_x86_32_request_indirect_t *rin32;
	blkif_x86_64_request_indirect_t *rin64;

	req = &xbdi->xbdi_xen_req;

	/* some sanity checks */
	KASSERT(req->operation == BLKIF_OP_READ ||
	    req->operation == BLKIF_OP_WRITE ||
	    req->operation == BLKIF_OP_INDIRECT);
	if (req->operation == BLKIF_OP_WRITE) {
		if (xbdi->xbdi_ro) {
			error = EROFS;
			goto end;
		}
	}

	/* copy request segments */
	switch (xbdi->xbdi_proto) {
	case XBDIP_NATIVE:
		reqn = RING_GET_REQUEST(&xbdi->xbdi_ring.ring_n,
		    xbdi->xbdi_ring.ring_n.req_cons);
		req->handle = reqn->handle;
		req->sector_number = reqn->sector_number;
		if (reqn->operation == BLKIF_OP_INDIRECT) {
			rinn = (blkif_request_indirect_t *)reqn;
			req->operation = rinn->indirect_op;
			req->nr_segments = (uint8_t)rinn->nr_segments;
			if (req->nr_segments > VBD_MAX_INDIRECT_SEGMENTS)
				goto bad_nr_segments;
			xbdi->xbdi_in_gntref = rinn->indirect_grefs[0];
			/* first_sect and segment grefs fetched later */
		} else {
			req->nr_segments = reqn->nr_segments;
			if (req->nr_segments > BLKIF_MAX_SEGMENTS_PER_REQUEST)
				goto bad_nr_segments;
			for (i = 0; i < req->nr_segments; i++)
				xbdi->xbdi_seg[i] = reqn->seg[i];
			xbdi->xbdi_in_gntref = 0;
		}
		break;
	case XBDIP_32:
		req32 = RING_GET_REQUEST(&xbdi->xbdi_ring.ring_32,
		    xbdi->xbdi_ring.ring_n.req_cons);
		req->handle = req32->handle;
		req->sector_number = req32->sector_number;
		if (req32->operation == BLKIF_OP_INDIRECT) {
			rin32 = (blkif_x86_32_request_indirect_t *)req32;
			req->operation = rin32->indirect_op;
			req->nr_segments = (uint8_t)rin32->nr_segments;
			if (req->nr_segments > VBD_MAX_INDIRECT_SEGMENTS)
				goto bad_nr_segments;
			xbdi->xbdi_in_gntref = rin32->indirect_grefs[0];
			/* first_sect and segment grefs fetched later */
		} else {
			req->nr_segments = req32->nr_segments;
			if (req->nr_segments > BLKIF_MAX_SEGMENTS_PER_REQUEST)
				goto bad_nr_segments;
			for (i = 0; i < req->nr_segments; i++)
				xbdi->xbdi_seg[i] = req32->seg[i];
			xbdi->xbdi_in_gntref = 0;
		}
		break;
	case XBDIP_64:
		req64 = RING_GET_REQUEST(&xbdi->xbdi_ring.ring_64,
		    xbdi->xbdi_ring.ring_n.req_cons);
		req->handle = req64->handle;
		req->sector_number = req64->sector_number;
		if (req64->operation == BLKIF_OP_INDIRECT) {
			rin64 = (blkif_x86_64_request_indirect_t *)req64;
			req->nr_segments = (uint8_t)rin64->nr_segments;
			if (req->nr_segments > VBD_MAX_INDIRECT_SEGMENTS)
				goto bad_nr_segments;
			xbdi->xbdi_in_gntref = rin64->indirect_grefs[0];
			/* first_sect and segment grefs fetched later */
		} else {
			req->nr_segments = req64->nr_segments;
			if (req->nr_segments > BLKIF_MAX_SEGMENTS_PER_REQUEST)
				goto bad_nr_segments;
			for (i = 0; i < req->nr_segments; i++)
				xbdi->xbdi_seg[i] = req64->seg[i];
			xbdi->xbdi_in_gntref = 0;
		}
		break;
	}

	/* Max value checked already earlier */
	if (req->nr_segments < 1)
		goto bad_nr_segments;

	xbdi->xbdi_cont = xbdback_co_io_gotio;
	return xbdback_io_get(xbdi);

 bad_nr_segments:
	if (ratecheck(&xbdi->xbdi_lasterr_time, &xbdback_err_intvl)) {
		printf("%s: invalid number of segments: %d\n",
		       xbdi->xbdi_name, req->nr_segments);
	}
	error = EINVAL;
	/* FALLTHROUGH */

 end:
	xbdback_send_reply(xbdi, xbdi->xbdi_xen_req.id,
	    xbdi->xbdi_xen_req.operation,
	    (error == EROFS) ? BLKIF_RSP_EOPNOTSUPP : BLKIF_RSP_ERROR);
	xbdi->xbdi_cont = xbdback_co_main_incr;
	return xbdi;
}

/* Prepare an I/O buffer for a xbdback instance */
static void *
xbdback_co_io_gotio(struct xbdback_instance *xbdi, void *obj)
{
	struct xbdback_io *xbd_io;
	int buf_flags;
	size_t bcount;
	blkif_request_t *req;

	KASSERT(mutex_owned(&xbdi->xbdi_lock));

	xbdi_get(xbdi);
	xbdi->xbdi_pendingreqs++;
	
	req = &xbdi->xbdi_xen_req;
	xbd_io = obj;
	memset(xbd_io, 0, sizeof(*xbd_io));
	buf_init(&xbd_io->xio_buf);
	xbd_io->xio_xbdi = xbdi;
	xbd_io->xio_operation = req->operation;
	xbd_io->xio_id = req->id;

	/* If segments are on an indirect page, copy them now */
	if (xbdi->xbdi_in_gntref) {
		gnttab_copy_t gop;
		paddr_t ma;

		gop.flags = GNTCOPY_source_gref;
		gop.len = req->nr_segments
		    * sizeof(struct blkif_request_segment);

		gop.source.u.ref = xbdi->xbdi_in_gntref;
		gop.source.offset = 0;
		gop.source.domid = xbdi->xbdi_domid;

		ma = xbdi->xbdi_seg_dmamap->dm_segs[0].ds_addr;
		gop.dest.offset = ma & PAGE_MASK;
		gop.dest.domid = DOMID_SELF;
		gop.dest.u.gmfn = ma >> PAGE_SHIFT;

		if (HYPERVISOR_grant_table_op(GNTTABOP_copy, &gop, 1) != 0) {
			printf("%s: GNTTABOP_copy failed\n", xbdi->xbdi_name);
			xbdback_send_reply(xbdi, xbdi->xbdi_xen_req.id,
			    xbdi->xbdi_xen_req.operation,
			    BLKIF_RSP_ERROR);
			xbdi->xbdi_cont = xbdback_co_main_incr;
			return NULL;
		}
	}

	/* Process segments */
	bcount = 0;
	for (int i = 0; i < req->nr_segments; i++) {
		struct blkif_request_segment *seg = &xbdi->xbdi_seg[i];
		xbd_io->xio_gref[i] = seg->gref;
		bcount += (seg->last_sect - seg->first_sect + 1)
			* VBD_BSIZE;
	}
	xbd_io->xio_nrma = req->nr_segments;
	xbd_io->xio_start_offset = xbdi->xbdi_seg[0].first_sect * VBD_BSIZE;

	KASSERT(bcount <= MAXPHYS);
	KASSERT(xbd_io->xio_start_offset < PAGE_SIZE);
	KASSERT(bcount + xbd_io->xio_start_offset < VBD_VA_SIZE);

	/* Fill-in the buf */
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
	xbd_io->xio_buf.b_blkno = req->sector_number;
	xbd_io->xio_buf.b_bcount = bcount;
	xbd_io->xio_buf.b_data = NULL;
	xbd_io->xio_buf.b_private = xbd_io;

	xbdi->xbdi_cont = xbdback_co_do_io;
	return xbdback_map_shm(xbd_io);
}

static void
xbdback_io_error(struct xbdback_io *xbd_io, int error)
{
	KASSERT(mutex_owned(&xbd_io->xio_xbdi->xbdi_lock));

	struct buf *bp = &xbd_io->xio_buf;

	bp->b_error = error;
	xbdback_iodone_locked(xbd_io->xio_xbdi, xbd_io, bp);
}

/*
 * Main xbdback I/O routine. It can either perform a flush operation or
 * schedule a read/write operation.
 */
static void *
xbdback_co_do_io(struct xbdback_instance *xbdi, void *obj)
{
	struct xbdback_io *xbd_io = obj;

	switch (xbd_io->xio_operation) {
	case BLKIF_OP_FLUSH_DISKCACHE:
	{
		int error;
		int force = 1;

		KASSERT(mutex_owned(&xbdi->xbdi_lock));
		mutex_exit(&xbdi->xbdi_lock);
		error = VOP_IOCTL(xbdi->xbdi_vp, DIOCCACHESYNC, &force, FWRITE,
		    kauth_cred_get());
		mutex_enter(&xbdi->xbdi_lock);
		if (error) {
			aprint_error("xbdback %s: DIOCCACHESYNC returned %d\n",
			    xbdi->xbdi_xbusd->xbusd_path, error);
			 if (error == EOPNOTSUPP || error == ENOTTY)
				error = BLKIF_RSP_EOPNOTSUPP;
			 else
				error = BLKIF_RSP_ERROR;
		} else
			error = BLKIF_RSP_OKAY;
		xbdback_send_reply(xbdi, xbd_io->xio_id,
		    xbd_io->xio_operation, error);
		xbdback_io_put(xbdi, xbd_io);
		xbdi_put(xbdi);
		xbdi->xbdi_cont = xbdback_co_main_incr;
		return xbdi;
	}
	case BLKIF_OP_READ:
	case BLKIF_OP_WRITE:
		xbd_io->xio_buf.b_data = (void *)
		    (xbd_io->xio_vaddr + xbd_io->xio_start_offset);

		if ((xbd_io->xio_buf.b_flags & B_READ) == 0) {
			mutex_enter(xbd_io->xio_buf.b_vp->v_interlock);
			xbd_io->xio_buf.b_vp->v_numoutput++;
			mutex_exit(xbd_io->xio_buf.b_vp->v_interlock);
		}
		/* will call xbdback_iodone() asynchronously when done */
		bdev_strategy(&xbd_io->xio_buf);
		xbdi->xbdi_cont = xbdback_co_main_incr;
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
 */
static void
xbdback_iodone(struct buf *bp)
{
	struct xbdback_io *xbd_io;
	struct xbdback_instance *xbdi;

	xbd_io = bp->b_private;
	KASSERT(bp == &xbd_io->xio_buf);
	xbdi = xbd_io->xio_xbdi;

	mutex_enter(&xbdi->xbdi_lock);
	xbdback_iodone_locked(xbdi, xbd_io, bp);
	mutex_exit(&xbdi->xbdi_lock);
}

/*
 * This gets reused by xbdback_io_error to report errors from other sources.
 */
static void
xbdback_iodone_locked(struct xbdback_instance *xbdi, struct xbdback_io *xbd_io,
    struct buf *bp)
{
	int status;

	XENPRINTF(("xbdback_io domain %d: iodone ptr 0x%lx\n",
		   xbdi->xbdi_domid, (long)xbd_io));

	KASSERT(mutex_owned(&xbdi->xbdi_lock));

	KASSERT(bp->b_error != 0 || xbd_io->xio_xv != NULL);
	if (xbd_io->xio_xv != NULL)
		xbdback_unmap_shm(xbd_io);

	if (bp->b_error != 0) {
		printf("xbd IO domain %d: error %d\n",
		       xbdi->xbdi_domid, bp->b_error);
		status = BLKIF_RSP_ERROR;
	} else
		status = BLKIF_RSP_OKAY;
	
	xbdback_send_reply(xbdi, xbd_io->xio_id, xbd_io->xio_operation, status);

	xbdi_put(xbdi);
	KASSERT(xbdi->xbdi_pendingreqs > 0);
	xbdi->xbdi_pendingreqs--;
	buf_destroy(&xbd_io->xio_buf);
	xbdback_io_put(xbdi, xbd_io);

	xbdback_wakeup_thread(xbdi);
}

/*
 * Wake up the per xbdback instance thread.
 */
static void
xbdback_wakeup_thread(struct xbdback_instance *xbdi)
{
	KASSERT(mutex_owned(&xbdi->xbdi_lock));

	/* only set RUN state when we are WAITING for work */
	if (xbdi->xbdi_status == WAITING)
	       xbdi->xbdi_status = RUN;
	cv_signal(&xbdi->xbdi_cv);
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

	KASSERT(mutex_owned(&xbdi->xbdi_lock));

	/*
	 * The ring can be accessed by the xbdback thread, xbdback_iodone()
	 * handler, or any handler that triggered the shm callback. So
	 * protect ring access via the xbdi_lock mutex.
	 */
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
	struct xbdback_instance *xbdi = xbd_io->xio_xbdi;
	int error;

#ifdef XENDEBUG_VBD
	int i;
	printf("xbdback_map_shm map grant ");
	for (i = 0; i < xbd_io->xio_nrma; i++) {
		printf("%u ", (u_int)xbd_io->xio_gref[i]);
	}
#endif

	KASSERT(mutex_owned(&xbdi->xbdi_lock));

	xbd_io->xio_xv = SLIST_FIRST(&xbdi->xbdi_va_free);
	KASSERT(xbd_io->xio_xv != NULL);
	SLIST_REMOVE_HEAD(&xbdi->xbdi_va_free, xv_next);
	xbd_io->xio_vaddr = xbd_io->xio_xv->xv_vaddr;

	error = xen_shm_map(xbd_io->xio_nrma, xbdi->xbdi_domid,
	    xbd_io->xio_gref, xbd_io->xio_vaddr, xbd_io->xio_gh, 
	    (xbd_io->xio_operation == BLKIF_OP_WRITE) ? XSHM_RO : 0);

	switch(error) {
	case 0:
#ifdef XENDEBUG_VBD
		printf("handle ");
		for (i = 0; i < xbd_io->xio_nrma; i++) {
			printf("%u ", (u_int)xbd_io->xio_gh[i]);
		}
		printf("\n");
#endif
		return xbd_io;
	default:
		if (ratecheck(&xbdi->xbdi_lasterr_time, &xbdback_err_intvl)) {
			printf("xbdback_map_shm: xen_shm error %d ", error);
		}
		/* this will also free xbd_io via xbdback_iodone() */
		xbdback_io_error(xbd_io, error);
		SLIST_INSERT_HEAD(&xbdi->xbdi_va_free, xbd_io->xio_xv, xv_next);
		xbd_io->xio_xv = NULL;
		/* do not retry */
		xbdi->xbdi_cont = xbdback_co_main_incr;
		return xbdi;
	}
}

/* unmap a request from our virtual address space (request is done) */
static void
xbdback_unmap_shm(struct xbdback_io *xbd_io)
{
	struct xbdback_instance *xbdi = xbd_io->xio_xbdi;

#ifdef XENDEBUG_VBD
	int i;
	printf("xbdback_unmap_shm handle ");
	for (i = 0; i < xbd_io->xio_nrma; i++) {
		printf("%u ", (u_int)xbd_io->xio_gh[i]);
	}
	printf("\n");
#endif

	KASSERT(xbd_io->xio_xv != NULL);
	xen_shm_unmap(xbd_io->xio_vaddr, xbd_io->xio_nrma,
	    xbd_io->xio_gh);
	SLIST_INSERT_HEAD(&xbdi->xbdi_va_free, xbd_io->xio_xv, xv_next);
	xbd_io->xio_xv = NULL;
	xbd_io->xio_vaddr = -1;
}

/* Obtain memory from a pool */
static struct xbdback_io *
xbdback_io_get(struct xbdback_instance *xbdi)
{
	struct xbdback_io *xbd_io = SLIST_FIRST(&xbdi->xbdi_io_free);
	KASSERT(xbd_io != NULL);
	SLIST_REMOVE_HEAD(&xbdi->xbdi_io_free, xio_next);
	return xbd_io;
}

/* Restore memory to a pool */
static void
xbdback_io_put(struct xbdback_instance *xbdi, struct xbdback_io *xbd_io)
{
	KASSERT(xbd_io != NULL);
	SLIST_INSERT_HEAD(&xbdi->xbdi_io_free, xbd_io, xio_next);
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
