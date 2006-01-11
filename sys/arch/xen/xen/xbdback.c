/*      $NetBSD: xbdback.c,v 1.4.2.10 2006/01/11 17:03:37 tron Exp $      */

/*
 * Copyright (c) 2005 Manuel Bouyer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>

#include <machine/pmap.h>
#include <machine/hypervisor.h>
#include <machine/xen.h>
#include <machine/evtchn.h>
#include <machine/ctrl_if.h>
#include <machine/xen_shm.h>

#ifdef XENDEBUG_VBD
#define XENPRINTF(x) printf x
#else
#define XENPRINTF(x)
#endif

#define    u16 uint16_t

/*
 * Backend block device driver for Xen
 */

/* Max number of pages per request. The request may not be page aligned */
#define BLKIF_MAX_PAGES_PER_REQUEST (BLKIF_MAX_SEGMENTS_PER_REQUEST + 1)

/* Values are expressed in 512-byte sectors */
#define VBD_BSIZE 512
#define VBD_MAXSECT ((PAGE_SIZE / VBD_BSIZE) - 1)

struct xbd_vbd;
struct xbdback_request;
struct xbdback_io;
struct xbdback_fragment;
struct xbdback_instance;

/* state of a xbdback instance */
typedef enum {CONNECTED, DISCONNECTING, DISCONNECTED} xbdback_state_t;

/*
 * Since there are a variety of conditions that can block our I/O
 * processing, which isn't allowed to suspend its thread's execution,
 * such things will be done in a sort of continuation-passing style.
 * 
 * Return value is NULL to indicate that execution has blocked; if
 * it's finished, set xbdi->cont (see below) to NULL and the return
 * doesn't matter.  Otherwise it's passed as the second parameter to
 * the new value of xbdi->cont.
 */
typedef void *(* xbdback_cont_t)(struct xbdback_instance *, void *);

/* we keep the xbdback instances in a linked list */
struct xbdback_instance {
	SLIST_ENTRY(xbdback_instance) next;
	domid_t domid;		/* attached to this domain */
	uint32_t handle;	/* domain-specific handle */
	volatile xbdback_state_t status;
	/* parameters for the communication */
	unsigned int evtchn;
	paddr_t ma_ring;
	/* private parameters for communication */
	blkif_ring_t *blk_ring;
	BLKIF_RING_IDX resp_prod; /* our current reply index */
	BLKIF_RING_IDX req_cons; /* our current request index */
	/* disconnection must be postponed until all I/O is done */
	volatile unsigned refcnt;
	uint8_t disconnect_rspid; /* request id of the disconnect request */
	/* 
	 * State for I/O processing/coalescing follows; this has to
	 * live here instead of on the stack because of the
	 * continuation-ness (see above).
	 */
	BLKIF_RING_IDX req_prod; /* limit on request indices */
	xbdback_cont_t cont, cont_aux;
	SIMPLEQ_ENTRY(xbdback_instance) on_hold; /* waiting on resources */
	/* _request state */
	struct xbdback_request *req; /* if NULL, ignore following */
	blkif_request_t *xen_req;
	int segno;
	struct xbd_vbd *req_vbd;
	/* _io state */
	struct xbdback_io *io; /* if NULL, ignore next field */
	daddr_t next_sector;
	unsigned long last_fas, this_fas;
	/* other state */
	int same_page; /* are we merging two segments on the same page? */

	SLIST_HEAD(, xbd_vbd) vbds; /* list of virtual block devices */
};
/* Manipulation of the above reference count. */
/* XXXjld@panix.com: not MP-safe, and move the i386 asm elsewhere. */
#define xbdi_get(xbdip) (++(xbdip)->refcnt)
#define xbdi_put(xbdip)                                      \
do {                                                         \
	__asm __volatile("decl %0"                           \
	    : "=m"((xbdip)->refcnt) : "m"((xbdip)->refcnt)); \
	if (0 == (xbdip)->refcnt)                            \
               xbdback_finish_disconnect(xbdip);             \
} while (/* CONSTCOND */ 0)

/* each xbdback instance has a list of vbds associated with it */
struct xbd_vbd {
	SLIST_ENTRY(xbd_vbd) next; 
	blkif_vdev_t vdev; /* interface-specific ID */
	int flags;
#define VBD_F_RO 0x01 /* device is read-only */
	int type; /* VDISK_TYPE_foo */
	/* for now we allow only one extent per vbd */
	dev_t dev; /* underlying device */
	const struct bdevsw *bdevsw; /* pointer to the device's bdevsw */
	struct vnode *vp;
	int start;
	int size;
};

SLIST_HEAD(, xbdback_instance) xbdback_instances;

/*
 * For each request from a guest, a xbdback_request is allocated from
 * a pool.  This will describe the request until completion.  The
 * request may require multiple IO operations to perform, so the
 * per-IO information is not stored here.
 */
struct xbdback_request {
	struct xbdback_instance *rq_xbdi; /* our xbd instance */
	uint8_t rq_operation;
	unsigned long rq_id;
	int rq_iocount; /* reference count; or, number of outstanding I/O's */
	int rq_ioerrs;
};

/*
 * For each I/O operation associated with one of those requests, an
 * xbdback_io is allocated from a pool.  It may correspond to multiple
 * Xen disk requests, or parts of them, if several arrive at once that
 * can be coalesced.
 */
struct xbdback_io {
	struct buf xio_buf; /* our I/O */
	/* The instance pointer is duplicated for convenience. */
	struct xbdback_instance *xio_xbdi; /* our xbd instance */
	SLIST_HEAD(, xbdback_fragment) xio_rq; /* xbd requests involved */
	vaddr_t xio_vaddr; /* the virtual address to map the request at */
	paddr_t xio_ma[XENSHM_MAX_PAGES_PER_REQUEST]; /* guest pages to map */
	uint16_t xio_nrma; /* number of guest pages */
	uint16_t xio_mapped;
};

/*
 * Rather than have the xbdback_io keep an array of the
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
 * Wrap our pools with a chain of xbdback_instances whose I/O
 * processing has blocked for want of memory from that pool.
 */
struct xbdback_pool {
	struct pool p;
	SIMPLEQ_HEAD(xbdback_iqueue, xbdback_instance) q;
	struct timeval last_warning;
} xbdback_request_pool, xbdback_io_pool, xbdback_fragment_pool;
static struct xbdback_iqueue xbdback_shmq;
static int xbdback_shmcb; /* have we already registered a callback? */

struct timeval xbdback_poolsleep_intvl = { 5, 0 };
struct timeval xbdback_fragio_intvl = { 60, 0 };

static void xbdback_ctrlif_rx(ctrl_msg_t *, unsigned long);
static int  xbdback_evthandler(void *);
static void xbdback_finish_disconnect(struct xbdback_instance *);

static struct xbdback_instance *xbdif_lookup(domid_t, uint32_t);
static struct xbd_vbd * vbd_lookup(struct xbdback_instance *, blkif_vdev_t);

static void *xbdback_co_main(struct xbdback_instance *, void *);
static void *xbdback_co_main_loop(struct xbdback_instance *, void *);
static void *xbdback_co_main_incr(struct xbdback_instance *, void *);
static void *xbdback_co_main_done(struct xbdback_instance *, void *);
static void *xbdback_co_main_done2(struct xbdback_instance *, void *);

static void *xbdback_co_io(struct xbdback_instance *, void *);
static void *xbdback_co_io_gotreq(struct xbdback_instance *, void *);
static void *xbdback_co_io_loop(struct xbdback_instance *, void *);
static void *xbdback_co_io_gotio(struct xbdback_instance *, void *);
static void *xbdback_co_io_gotio2(struct xbdback_instance *, void *);
static void *xbdback_co_io_gotfrag(struct xbdback_instance *, void *);
static void *xbdback_co_io_gotfrag2(struct xbdback_instance *, void *);

static void *xbdback_co_flush(struct xbdback_instance *, void *);
static void *xbdback_co_flush_done(struct xbdback_instance *, void *);

static void *xbdback_co_probe(struct xbdback_instance *, void *);
static void *xbdback_co_probe_gotio(struct xbdback_instance *, void *);
static void *xbdback_co_probe_gotvm(struct xbdback_instance *, void *);

static int  xbdback_shm_callback(void *);
static void xbdback_io_error(struct xbdback_io *, int);
static void xbdback_do_io(struct xbdback_io *);
static void xbdback_iodone(struct buf *);
static void xbdback_send_reply(struct xbdback_instance *, int , int , int);

static void *xbdback_map_shm(struct xbdback_io *);
static void xbdback_unmap_shm(struct xbdback_io *);

static void *xbdback_pool_get(struct xbdback_pool *,
			      struct xbdback_instance *);
static void xbdback_pool_put(struct xbdback_pool *, void *);
static void xbdback_trampoline(struct xbdback_instance *, void *);


void
xbdback_init()
{
	ctrl_msg_t cmsg;
	blkif_be_driver_status_t st;

	if ( !(xen_start_info.flags & SIF_INITDOMAIN) &&
	     !(xen_start_info.flags & SIF_BLK_BE_DOMAIN) )
		return;

	XENPRINTF(("xbdback_init\n"));

	/*
	 * initialize the backend driver, register the control message handler
	 * and send driver up message.
	 */
	SLIST_INIT(&xbdback_instances);
	SIMPLEQ_INIT(&xbdback_shmq);
	xbdback_shmcb = 0;
	pool_init(&xbdback_request_pool.p, sizeof(struct xbdback_request),
	    0, 0, 0, "xbbrp", NULL);
	SIMPLEQ_INIT(&xbdback_request_pool.q);
	pool_init(&xbdback_io_pool.p, sizeof(struct xbdback_io),
	    0, 0, 0, "xbbip", NULL);
	SIMPLEQ_INIT(&xbdback_io_pool.q);
	pool_init(&xbdback_fragment_pool.p, sizeof(struct xbdback_fragment),
	    0, 0, 0, "xbbfp", NULL);
	SIMPLEQ_INIT(&xbdback_fragment_pool.q);
	/* we allocate enough to handle a whole ring at once */
	if (pool_prime(&xbdback_request_pool.p, BLKIF_RING_SIZE) != 0)
		printf("xbdback: failed to prime request pool\n");
	if (pool_prime(&xbdback_io_pool.p, BLKIF_RING_SIZE) != 0)
		printf("xbdback: failed to prime io pool\n");
	if (pool_prime(&xbdback_fragment_pool.p,
            BLKIF_MAX_SEGMENTS_PER_REQUEST * BLKIF_RING_SIZE) != 0)
		printf("xbdback: failed to prime fragment pool\n");

	(void)ctrl_if_register_receiver(CMSG_BLKIF_BE, xbdback_ctrlif_rx,
	    CALLBACK_IN_BLOCKING_CONTEXT);

	cmsg.type      = CMSG_BLKIF_BE;
	cmsg.subtype   = CMSG_BLKIF_BE_DRIVER_STATUS;
	cmsg.length    = sizeof(blkif_be_driver_status_t);
	st.status      = BLKIF_DRIVER_STATUS_UP;
	memcpy(cmsg.msg, &st, sizeof(st));
	ctrl_if_send_message_block(&cmsg, NULL, 0, 0);
}

static void
xbdback_ctrlif_rx(ctrl_msg_t *msg, unsigned long id)
{
	struct xbdback_instance *xbdi;
	struct xbd_vbd *vbd;
	int error;

	XENPRINTF(("xbdback msg %d\n", msg->subtype));
	switch (msg->subtype) {
	case CMSG_BLKIF_BE_CREATE:
	{
		blkif_be_create_t *req = (blkif_be_create_t *)&msg->msg[0];
		if (msg->length != sizeof(blkif_be_create_t))
			goto error;
		if (xbdif_lookup(req->domid, req->blkif_handle) != NULL) {
			req->status = BLKIF_BE_STATUS_INTERFACE_EXISTS;
			goto end;
		}
		xbdi = malloc(sizeof(struct xbdback_instance), M_DEVBUF,
		    M_NOWAIT | M_ZERO);
		if (xbdi == NULL) {
			req->status = BLKIF_BE_STATUS_OUT_OF_MEMORY;
			goto end;
		}
		xbdi->domid = req->domid;
		xbdi->handle = req->blkif_handle;
		xbdi->status = DISCONNECTED;
		xbdi->refcnt = 1;
		xbdi->blk_ring = NULL;
		SLIST_INIT(&xbdi->vbds);
		SLIST_INSERT_HEAD(&xbdback_instances, xbdi, next);
		
		req->status = BLKIF_BE_STATUS_OKAY;
		break;
	}
	case CMSG_BLKIF_BE_DESTROY:
	{
		blkif_be_destroy_t *req = (blkif_be_destroy_t *)&msg->msg[0];
		if (msg->length != sizeof(blkif_be_destroy_t))
			goto error;
		xbdi = xbdif_lookup(req->domid, req->blkif_handle);
		if (xbdi == NULL) {
			req->status = BLKIF_BE_STATUS_INTERFACE_NOT_FOUND;
			goto end;
		}
		if (xbdi->status != DISCONNECTED) {
			req->status = BLKIF_BE_STATUS_INTERFACE_CONNECTED;
			goto end;
		}
		if (xbdi->blk_ring != NULL)
			uvm_km_free(kernel_map, (vaddr_t)xbdi->blk_ring,
			    PAGE_SIZE);
		SLIST_REMOVE(&xbdback_instances, xbdi, xbdback_instance, next);
		free(xbdi, M_DEVBUF);
		req->status = BLKIF_BE_STATUS_OKAY;
		break;
	}
	case CMSG_BLKIF_BE_CONNECT:
	{
		blkif_be_connect_t *req = (blkif_be_connect_t *)&msg->msg[0];
		vaddr_t ring_addr;
		char evname[16];

		if (msg->length != sizeof(blkif_be_connect_t))
			goto error;
		xbdi = xbdif_lookup(req->domid, req->blkif_handle);
		if (xbdi == NULL) {
			req->status = BLKIF_BE_STATUS_INTERFACE_NOT_FOUND;
			goto end;
		}
		if (xbdi->status != DISCONNECTED) {
			req->status = BLKIF_BE_STATUS_INTERFACE_CONNECTED;
			goto end;
		}
		if (xbdi->blk_ring == NULL) {
			ring_addr = uvm_km_alloc(kernel_map, PAGE_SIZE);
			if (ring_addr == 0) {
				req->status = BLKIF_BE_STATUS_OUT_OF_MEMORY;
				goto end;
			}
		} else {
			ring_addr = (vaddr_t)xbdi->blk_ring;
			xbdi->blk_ring = NULL;
		}

		xbdi->ma_ring = req->shmem_frame << PAGE_SHIFT;
		error = pmap_remap_pages(pmap_kernel(), ring_addr,
		    xbdi->ma_ring, 1, PMAP_WIRED | PMAP_CANFAIL, req->domid);
		if (error) {
			uvm_km_free(kernel_map, ring_addr, PAGE_SIZE);
			if (error == ENOMEM)
				req->status = BLKIF_BE_STATUS_OUT_OF_MEMORY;
			else if (error == EFAULT)
				req->status = BLKIF_BE_STATUS_MAPPING_ERROR;
			else req->status = BLKIF_BE_STATUS_ERROR;
			goto end;
		}
		xbdi->blk_ring = (void *)ring_addr;
		xbdi->evtchn = req->evtchn;
		snprintf(evname, sizeof(evname), "xbdback%d", xbdi->domid);
		event_set_handler(xbdi->evtchn,
		    xbdback_evthandler, xbdi, IPL_BIO, evname);
		printf("xbd backend %d for domain %d using event channel %d\n",
		    xbdi->handle, xbdi->domid, xbdi->evtchn);
		hypervisor_enable_event(xbdi->evtchn);
		xbdi->status = CONNECTED;
		req->status = BLKIF_BE_STATUS_OKAY;
		break;
	}
	case CMSG_BLKIF_BE_DISCONNECT:
	{
		blkif_be_disconnect_t *req =
		    (blkif_be_disconnect_t *)&msg->msg[0];
		int s;

		if (msg->length != sizeof(blkif_be_disconnect_t))
			goto error;
		xbdi = xbdif_lookup(req->domid, req->blkif_handle);
		if (xbdi == NULL) {
			req->status = BLKIF_BE_STATUS_INTERFACE_NOT_FOUND;
			goto end;
		}
		hypervisor_mask_event(xbdi->evtchn);
		event_remove_handler(xbdi->evtchn, xbdback_evthandler, xbdi);
		xbdi->status = DISCONNECTING;
		xbdi->disconnect_rspid = msg->id;
		s = splbio();
		xbdi_put(xbdi);
		splx(s);
		return;
	}
	case CMSG_BLKIF_BE_VBD_CREATE:
	{
		blkif_be_vbd_create_t *req =
		    (blkif_be_vbd_create_t *)&msg->msg[0];
		if (msg->length != sizeof(blkif_be_vbd_create_t))
			goto error;
		xbdi = xbdif_lookup(req->domid, req->blkif_handle);
		if (xbdi == NULL) {
			req->status = BLKIF_BE_STATUS_INTERFACE_NOT_FOUND;
			goto end;
		}
		vbd = malloc(sizeof(struct xbd_vbd), M_DEVBUF,
		    M_NOWAIT | M_ZERO);
		if (vbd == NULL) {
			req->status = BLKIF_BE_STATUS_OUT_OF_MEMORY;
			goto end;
		}
		vbd->vdev = req->vdevice;
		if (req->readonly)
			vbd->flags |= VBD_F_RO;

		SLIST_INSERT_HEAD(&xbdi->vbds, vbd, next);
		req->status = BLKIF_BE_STATUS_OKAY;
		break;
	}
	case CMSG_BLKIF_BE_VBD_DESTROY:
	{
		blkif_be_vbd_destroy_t *req =
		    (blkif_be_vbd_destroy_t *)&msg->msg[0];
		if (msg->length != sizeof(blkif_be_vbd_destroy_t))
			goto error;
		xbdi = xbdif_lookup(req->domid, req->blkif_handle);
		if (xbdi == NULL) {
			req->status = BLKIF_BE_STATUS_INTERFACE_NOT_FOUND;
			goto end;
		}
		vbd = vbd_lookup(xbdi, req->vdevice);
		if (vbd == NULL) {
			req->status = BLKIF_BE_STATUS_VBD_NOT_FOUND;
			goto end;
		}
		if (vbd->size) {
			printf("xbd backend: detach device %s%d%c "
			    "for domain %d\n", devsw_blk2name(major(vbd->dev)),
			    DISKUNIT(vbd->dev), DISKPART(vbd->dev) + 'a',
			    xbdi->domid);
			vbd->start = vbd->size = vbd->dev = 0;
			vn_close(vbd->vp, FREAD, NOCRED, NULL);
		}
		SLIST_REMOVE(&xbdi->vbds, vbd, xbd_vbd, next);
		free(vbd, M_DEVBUF);

		req->status = BLKIF_BE_STATUS_OKAY;
		break;
	}
	case CMSG_BLKIF_BE_VBD_GROW:
	{
		blkif_be_vbd_grow_t *req = (blkif_be_vbd_grow_t *)&msg->msg[0];
		const char *devname;
		int major;
		struct partinfo dpart;

		if (msg->length != sizeof(blkif_be_vbd_grow_t))
			goto error;
		xbdi = xbdif_lookup(req->domid, req->blkif_handle);
		if (xbdi == NULL) {
			req->status = BLKIF_BE_STATUS_INTERFACE_NOT_FOUND;
			goto end;
		}
		vbd = vbd_lookup(xbdi, req->vdevice);
		if (vbd == NULL) {
			req->status = BLKIF_BE_STATUS_VBD_NOT_FOUND;
			goto end;
		}
		if (vbd->size != 0) {
			req->status = BLKIF_BE_STATUS_VBD_EXISTS;
			goto end;
		}
		if (req->extent.sector_start != 0) {
			req->status = BLKIF_BE_STATUS_EXTENT_NOT_FOUND;
			goto end;
		}
		major = major(req->extent.device);
		devname = devsw_blk2name(major);
		if (devname == NULL) {
			printf("xbdback VBD grow domain %d: unknwon device "
			    "0x%x\n", xbdi->domid, req->extent.device);
			req->status = BLKIF_BE_STATUS_EXTENT_NOT_FOUND;
			goto end;
		}
		vbd->dev = req->extent.device;
		vbd->bdevsw = bdevsw_lookup(vbd->dev);
		if (vbd->bdevsw == NULL) {
			printf("xbdback VBD grow domain %d: no bdevsw for "
			    "device 0x%x\n", xbdi->domid, req->extent.device);
			req->status = BLKIF_BE_STATUS_EXTENT_NOT_FOUND;
			goto end;
		}
		error = bdevvp(vbd->dev, &vbd->vp);
		if (error) {
			printf("xbdback VBD grow domain %d: can't open "
			    "device 0x%x (error %d)\n", xbdi->domid,
			    req->extent.device, error);
			req->status = BLKIF_BE_STATUS_EXTENT_NOT_FOUND;
			goto end;
		}
		error = VOP_OPEN(vbd->vp, FREAD, NOCRED, 0);
		if (error) {
			printf("xbdback VBD grow domain %d: can't open2 "
			    "device 0x%x (error %d)\n", xbdi->domid,
			    req->extent.device, error);
			vput(vbd->vp);
			req->status = BLKIF_BE_STATUS_EXTENT_NOT_FOUND;
			goto end;
		}
		VOP_UNLOCK(vbd->vp, 0);
		error = VOP_IOCTL(vbd->vp, DIOCGPART, &dpart, FREAD, 0, NULL);
		if (error) {
			printf("xbdback VBD grow domain %d: can't ioctl "
			    "device 0x%x (error %d)\n", xbdi->domid,
			    req->extent.device, error);
			vbd->start = vbd->size = vbd->dev = 0;
			vn_close(vbd->vp, FREAD, NOCRED, NULL);
			vbd->vp = NULL;
			req->status = BLKIF_BE_STATUS_EXTENT_NOT_FOUND;
			goto end;
		}
		vbd->size = req->extent.sector_length * (512 / DEV_BSIZE);
		if (vbd->size == 0 || vbd->size > dpart.part->p_size);
			vbd->size = dpart.part->p_size;
		printf("xbd backend: attach device %s%d%c (size %d) "
		    "for domain %d\n", devname, DISKUNIT(vbd->dev),
		    DISKPART(vbd->dev) + 'a', vbd->size, xbdi->domid);
		req->status = BLKIF_BE_STATUS_OKAY;
		break;
	}
	case CMSG_BLKIF_BE_VBD_SHRINK:
	{
		blkif_be_vbd_shrink_t *req =
		    (blkif_be_vbd_shrink_t *)&msg->msg[0];
		if (msg->length != sizeof(blkif_be_vbd_shrink_t))
			goto error;
		xbdi = xbdif_lookup(req->domid, req->blkif_handle);
		if (xbdi == NULL) {
			req->status = BLKIF_BE_STATUS_INTERFACE_NOT_FOUND;
			goto end;
		}
		vbd = vbd_lookup(xbdi, req->vdevice);
		if (vbd == NULL) {
			req->status = BLKIF_BE_STATUS_VBD_NOT_FOUND;
			goto end;
		}
		if (vbd->size == 0) {
			req->status = BLKIF_BE_STATUS_VBD_NOT_FOUND;
			goto end;
		}
		printf("xbd backend: detach device %s%d%c "
		    "for domain %d\n", devsw_blk2name(major(vbd->dev)),
		    DISKUNIT(vbd->dev), DISKPART(vbd->dev) + 'a',
		    xbdi->domid);
		vbd->start = vbd->size = vbd->dev = 0;
		vn_close(vbd->vp, FREAD, NOCRED, NULL);
		req->status = BLKIF_BE_STATUS_OKAY;
		break;
	}
	default:
error:
		printf("xbdback: wrong message subtype %d len %d\n",
		    msg->subtype, msg->length);
		msg->length = 0;
	}
end:
	XENPRINTF(("xbdback msg rep size %d\n", msg->length));
	ctrl_if_send_response(msg);
	return;
}

static void xbdback_finish_disconnect(struct xbdback_instance *xbdi)
{
	ctrl_msg_t cmsg;
	blkif_be_disconnect_t *pdisc = (blkif_be_disconnect_t *)&cmsg.msg;
	vaddr_t ring_addr;
	
	KASSERT(xbdi->status == DISCONNECTING);

       	ring_addr = (vaddr_t)xbdi->blk_ring;
	pmap_remove(pmap_kernel(), ring_addr, ring_addr + PAGE_SIZE);
	xbdi->status = DISCONNECTED;

	memset(&cmsg, 0, sizeof(cmsg));
	cmsg.type = CMSG_BLKIF_BE;
	cmsg.subtype = CMSG_BLKIF_BE_DISCONNECT;
	cmsg.id = xbdi->disconnect_rspid;
	cmsg.length = sizeof(blkif_be_disconnect_t);
	pdisc->domid = xbdi->domid;
	pdisc->blkif_handle = xbdi->handle;
	pdisc->status = BLKIF_BE_STATUS_OKAY;

	ctrl_if_send_response(&cmsg);
}

static struct xbdback_instance *
xbdif_lookup(domid_t dom , uint32_t handle)
{
	struct xbdback_instance *xbdi;

	SLIST_FOREACH(xbdi, &xbdback_instances, next) {
		if (xbdi->domid == dom && xbdi->handle == handle)
			return xbdi;
	}
	return NULL;
}

static struct xbd_vbd *
vbd_lookup(struct xbdback_instance *xbdi , blkif_vdev_t vdev)
{
	struct xbd_vbd *vbd;

	SLIST_FOREACH(vbd, &xbdi->vbds, next) {
		if (vbd->vdev == vdev)
			return vbd;
	}
	return NULL;
}

static int
xbdback_evthandler(void *arg)
{
	struct xbdback_instance *xbdi = arg;

	if (xbdi->cont == NULL) {
		xbdi->cont = xbdback_co_main;
		xbdback_trampoline(xbdi, xbdi);
	}
	return 1;
}

static void *
xbdback_co_main(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;
	xbdi->req_prod = xbdi->blk_ring->req_prod;
	x86_lfence(); /* ensure we see all requests up to req_prod */
	/*
	 * note that we'll eventually get a full ring of request.
	 * in this case, MASK_BLKIF_IDX(req_cons) == MASK_BLKIF_IDX(req_prod)
	 */
	xbdi->cont = xbdback_co_main_loop;
	return xbdi;
}

static void *
xbdback_co_main_loop(struct xbdback_instance *xbdi, void *obj) 
{
	blkif_request_t *req;

	(void)obj;
	if (xbdi->req_cons != xbdi->req_prod) {
		req = xbdi->xen_req = &xbdi->blk_ring->ring[
		    MASK_BLKIF_IDX(xbdi->req_cons)].req;
		XENPRINTF(("xbdback op %d req_cons 0x%x req_prod 0x%x/0x%x "
		    "resp_prod 0x%x/0x%x\n", req->operation,
			xbdi->req_cons,
			xbdi->req_prod, MASK_BLKIF_IDX(xbdi->req_prod),
			xbdi->blk_ring->resp_prod,
			MASK_BLKIF_IDX(xbdi->blk_ring->resp_prod)));
		switch(req->operation) {
		case BLKIF_OP_PROBE:
			xbdi->cont = xbdback_co_probe;
			break;
		case BLKIF_OP_READ:
		case BLKIF_OP_WRITE:
			xbdi->cont = xbdback_co_io;
			break;
		default:
			printf("xbdback_evthandler domain %d: unknown "
			    "operation %d\n", xbdi->domid, req->operation);
			xbdback_send_reply(xbdi, req->id, req->operation,
			    BLKIF_RSP_ERROR);
			xbdi->cont = xbdback_co_main_incr;
			break;
		}
	} else {
		xbdi->cont = xbdback_co_main_done;
	}
	return xbdi;
}

static void *
xbdback_co_main_incr(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;
	xbdi->req_cons++;
	xbdi->cont = xbdback_co_main_loop;
	return xbdi;
}

static void *
xbdback_co_main_done(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;
	if (xbdi->io != NULL) {
		xbdi->cont = xbdback_co_flush;
		xbdi->cont_aux = xbdback_co_main_done2;
	} else {
		xbdi->cont = xbdback_co_main_done2;
	}
	return xbdi;
}

static void *
xbdback_co_main_done2(struct xbdback_instance *xbdi, void *obj)
{

	if (xbdi->req_prod == xbdi->blk_ring->req_prod) {
		xbdi->cont = NULL;
	} else {
		xbdi->cont = xbdback_co_main;
	}
	return xbdi;
}

static void *
xbdback_co_io(struct xbdback_instance *xbdi, void *obj)
{	
	int error;

	(void)obj;
	if (xbdi->xen_req->nr_segments < 1 ||
	    xbdi->xen_req->nr_segments > BLKIF_MAX_SEGMENTS_PER_REQUEST ) {
		printf("xbdback_io domain %d: %d segments\n",
		       xbdi->domid, xbdi->xen_req->nr_segments);
		error = EINVAL;
		goto end;
	}
	xbdi->req_vbd = vbd_lookup(xbdi, xbdi->xen_req->device);
	if (xbdi->req_vbd == NULL) {
		printf("xbdback_io domain %d: unknown vbd %d\n",
		       xbdi->domid, xbdi->xen_req->device);
		error = EINVAL;
		goto end;
	}
	if (xbdi->xen_req->operation == BLKIF_OP_WRITE) {
		if (xbdi->req_vbd->flags & VBD_F_RO) {
			error = EROFS;
			goto end;
		}
	}

	xbdi->segno = 0;

	xbdi->cont = xbdback_co_io_gotreq;
	return xbdback_pool_get(&xbdback_request_pool, xbdi);
 end:
	xbdback_send_reply(xbdi, xbdi->xen_req->id, xbdi->xen_req->operation,
	    error);
	xbdi->cont = xbdback_co_main_incr;
	return xbdi;
}

static void *
xbdback_co_io_gotreq(struct xbdback_instance *xbdi, void *obj)
{
	struct xbdback_request *xrq;

	xrq = xbdi->req = obj;
	
	xrq->rq_xbdi = xbdi;
	xrq->rq_iocount = 0;
	xrq->rq_ioerrs = 0;
	xrq->rq_id = xbdi->xen_req->id;
	xrq->rq_operation = xbdi->xen_req->operation;

	/* 
	 * Request-level reasons not to coalesce: different device,
	 * different op, or noncontiguous disk sectors (vs. previous
	 * request handed to us).
	 */
	xbdi->cont = xbdback_co_io_loop;
	if (xbdi->io != NULL) {
		struct xbdback_request *last_req;
		last_req = SLIST_FIRST(&xbdi->io->xio_rq)->car;
		XENPRINTF(("xbdback_io domain %d: hoping for sector %ld;"
		    " got %ld\n", xbdi->domid, (long)xbdi->next_sector,
		    (long)xbdi->xen_req->sector_number));
		if (xrq->rq_operation != last_req->rq_operation
		    || xbdi->xen_req->sector_number != xbdi->next_sector) {
			XENPRINTF(("xbdback_io domain %d: segment break\n",
			    xbdi->domid));
			xbdi->next_sector = xbdi->xen_req->sector_number;
			xbdi->cont_aux = xbdi->cont; 
			xbdi->cont = xbdback_co_flush;
		}
	} else {
		xbdi->next_sector = xbdi->xen_req->sector_number;
	}
	return xbdi;
}


static void *
xbdback_co_io_loop(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;
	if (xbdi->segno < xbdi->xen_req->nr_segments) {
		unsigned long this_fas, last_fas;
		/* 
		 * Segment-level reason to coalesce: handling full
		 * pages, or adjacent sector ranges from the same page
		 * (and yes, this latter does happen).  But not if the
		 * array of client pseudo-physical pages is full.
		 */
		this_fas = xbdi->xen_req->frame_and_sects[xbdi->segno];
		XENPRINTF(("xbdback_io domain %d: frame_and_sects[%d]=0%lo\n",
			   xbdi->domid, xbdi->segno, this_fas));
		last_fas = xbdi->last_fas = xbdi->this_fas;
		xbdi->this_fas = this_fas;
		if (xbdi->io != NULL) {
			if (blkif_last_sect(last_fas) == VBD_MAXSECT
			    && blkif_first_sect(this_fas) == 0
			    && xbdi->io->xio_nrma
			        < XENSHM_MAX_PAGES_PER_REQUEST) {
				xbdi->same_page = 0;
			} else if (blkif_last_sect(last_fas) + 1
				       == blkif_first_sect(this_fas)
				   && (last_fas & ~PAGE_MASK)
				       == (this_fas & ~PAGE_MASK)) {
				static struct timeval gluetimer;
				if (ratecheck(&gluetimer,
					      &xbdback_fragio_intvl))
					printf("xbdback: domain %d sending"
					    " excessively fragmented I/O\n",
					    xbdi->domid);
				XENPRINTF(("xbdback_io domain %d: glue same "
				    "page", xbdi->domid));
				xbdi->same_page = 1;
			} else {
				xbdi->cont_aux = xbdback_co_io_loop;
				xbdi->cont = xbdback_co_flush;
				return xbdi;
			}
		} else
			xbdi->same_page = 0;

		if (xbdi->io == NULL) {
			xbdi->cont = xbdback_co_io_gotio;
			return xbdback_pool_get(&xbdback_io_pool, xbdi);
		} else {
			xbdi->cont = xbdback_co_io_gotio2;
		}
	} else {
		/* done with the loop over segments; get next request */
		xbdi->cont = xbdback_co_main_incr;
	}
	return xbdi;			
}


static void *
xbdback_co_io_gotio(struct xbdback_instance *xbdi, void *obj)

{
	struct xbdback_io *xbd_io;
	vaddr_t start_offset; /* start offset in vm area */
	int buf_flags;

	xbdi_get(xbdi);
	
	xbd_io = xbdi->io = obj;
	xbd_io->xio_xbdi = xbdi;
	SLIST_INIT(&xbd_io->xio_rq);
	xbd_io->xio_nrma = 0;
	xbd_io->xio_mapped = 0;

	start_offset = blkif_first_sect(xbdi->this_fas) * VBD_BSIZE;
	
	if (xbdi->xen_req->operation == BLKIF_OP_WRITE) {
		buf_flags = B_WRITE | B_CALL;
	} else {
		buf_flags = B_READ | B_CALL;
	}

	BUF_INIT(&xbd_io->xio_buf);
	xbd_io->xio_buf.b_flags = buf_flags;
	xbd_io->xio_buf.b_iodone = xbdback_iodone;
	xbd_io->xio_buf.b_proc = NULL;
	xbd_io->xio_buf.b_vp = xbdi->req_vbd->vp;
	xbd_io->xio_buf.b_dev = xbdi->req_vbd->dev;
	xbd_io->xio_buf.b_blkno = xbdi->next_sector;
	xbd_io->xio_buf.b_bcount = 0;
	xbd_io->xio_buf.b_data = (void *)start_offset;
	xbd_io->xio_buf.b_private = xbd_io;

	xbdi->cont = xbdback_co_io_gotio2;
	return xbdi;
}


static void *
xbdback_co_io_gotio2(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;
	if (xbdi->segno == 0 || SLIST_EMPTY(&xbdi->io->xio_rq)) {
		/* if this is the first segment of a new request */
		/* or if it's the first segment of the io */
		xbdi->cont = xbdback_co_io_gotfrag;
		return xbdback_pool_get(&xbdback_fragment_pool, xbdi);
	}
	xbdi->cont = xbdback_co_io_gotfrag2;
	return xbdi;
}


static void *
xbdback_co_io_gotfrag(struct xbdback_instance *xbdi, void *obj)
{
	struct xbdback_fragment *xbd_fr;

	xbd_fr = obj;
	xbd_fr->car = xbdi->req;
	SLIST_INSERT_HEAD(&xbdi->io->xio_rq, xbd_fr, cdr);
	++xbdi->req->rq_iocount;

	xbdi->cont = xbdback_co_io_gotfrag2;
	return xbdi;
}

static void *
xbdback_co_io_gotfrag2(struct xbdback_instance *xbdi, void *obj)
{
	struct xbdback_io *xbd_io;
	int seg_size;
	unsigned long this_fas;

	this_fas = xbdi->this_fas;
	xbd_io = xbdi->io;
	seg_size = blkif_last_sect(this_fas) - blkif_first_sect(this_fas) + 1;

	if (seg_size < 0) {
		printf("xbdback_io domain %d: negative-size request\n",
		       xbdi->domid);
		xbdback_io_error(xbdi->io, EINVAL);
		xbdi->io = NULL;
		xbdi->cont = xbdback_co_main_incr;
		return xbdi;
	}
	
	if (!xbdi->same_page) {
		XENPRINTF(("xbdback_io domain %d: appending page 0%lo\n",
			   xbdi->domid, (this_fas & ~PAGE_MASK)));
		xbd_io->xio_ma[xbd_io->xio_nrma++] = (this_fas
		     & ~PAGE_MASK);
	}

	xbd_io->xio_buf.b_bcount += (daddr_t)(seg_size * VBD_BSIZE);
	XENPRINTF(("xbdback_io domain %d: start sect %d size %d\n",
	    xbdi->domid, (int)xbdi->next_sector, seg_size));
	
	/* Finally, the end of the segment loop! */
	xbdi->next_sector += seg_size;
	++xbdi->segno;
	xbdi->cont = xbdback_co_io_loop;
	return xbdi;
}


static void *
xbdback_co_flush(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;
	XENPRINTF(("xbdback_io domain %d: flush sect %ld size %d ptr 0x%lx\n",
	    xbdi->domid, (long)xbdi->io->xio_buf.b_blkno,
	    (int)xbdi->io->xio_buf.b_bcount, (long)xbdi->io));
	xbdi->cont = xbdback_co_flush_done;
	return xbdback_map_shm(xbdi->io);
}

static void *
xbdback_co_flush_done(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;
	xbdback_do_io(xbdi->io);
	xbdi->io = NULL;
	xbdi->cont = xbdi->cont_aux;
	return xbdi;
}

static void
xbdback_io_error(struct xbdback_io *xbd_io, int error)
{
	xbd_io->xio_buf.b_flags |= B_ERROR;
	xbd_io->xio_buf.b_error = error;
	xbdback_iodone(&xbd_io->xio_buf);
}

static void
xbdback_do_io(struct xbdback_io *xbd_io)
{
	xbd_io->xio_buf.b_data =
	    (void *)((vaddr_t)xbd_io->xio_buf.b_data + xbd_io->xio_vaddr);
#ifdef DIAGNOSTIC
	{
	vaddr_t bdata = (vaddr_t)xbd_io->xio_buf.b_data;
	int nsegs =
	    ((((bdata + xbd_io->xio_buf.b_bcount - 1) & ~PAGE_MASK) -
	    (bdata & ~PAGE_MASK)) >> PAGE_SHIFT) + 1;
	if ((bdata & ~PAGE_MASK) != (xbd_io->xio_vaddr & ~PAGE_MASK)) {
		printf("xbdback_do_io vaddr 0x%lx bdata 0x%lx\n",
		    xbd_io->xio_vaddr, bdata);
		panic("xbdback_do_io: bdata page change");
	}
	if (nsegs > xbd_io->xio_nrma) {
		printf("xbdback_do_io vaddr 0x%lx bcount 0x%x doesn't fit in "
		    " %d pages\n", bdata, xbd_io->xio_buf.b_bcount,
		    xbd_io->xio_nrma);
		panic("xbdback_do_io: not enough pages");
	}
	}
#endif
	if ((xbd_io->xio_buf.b_flags & B_READ) == 0)
		xbd_io->xio_buf.b_vp->v_numoutput++;
	DEV_STRATEGY(&xbd_io->xio_buf);
}

/* This gets reused by xbdback_io_error to report errors from other sources. */
static void
xbdback_iodone(struct buf *bp)
{
	struct xbdback_io *xbd_io;
	struct xbdback_instance *xbdi;
	int errp;

	xbd_io = bp->b_private;
	xbdi = xbd_io->xio_xbdi;

	XENPRINTF(("xbdback_io domain %d: iodone ptr 0x%lx\n",
		   xbdi->domid, (long)xbd_io));

	if (xbd_io->xio_mapped)
		xbdback_unmap_shm(xbd_io);

	if (bp->b_flags & B_ERROR) {
		printf("xbd IO domain %d: error %d\n",
		       xbdi->domid, bp->b_error);
		errp = 1;
	} else
		errp = 0;

	
	/* for each constituent xbd request */
	while(!SLIST_EMPTY(&xbd_io->xio_rq)) {
		struct xbdback_fragment *xbd_fr;
		struct xbdback_request *xbd_req;
		struct xbdback_instance *rxbdi;
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

		XENPRINTF(("xbdback_io domain %d: end request %lu error=%d\n",
		    xbdi->domid, xbd_req->rq_id, error));
		xbdback_send_reply(xbdi, xbd_req->rq_id,
		    xbd_req->rq_operation, error);
		xbdback_pool_put(&xbdback_request_pool, xbd_req);
	}
	xbdi_put(xbdi);
	xbdback_pool_put(&xbdback_io_pool, xbd_io);
}

static void *
xbdback_co_probe(struct xbdback_instance *xbdi, void *obj)
{
	(void)obj;
	/*
	 * There should be only one page in the request. Map it and store
	 * the reply.
	 */
	if (xbdi->xen_req->nr_segments != 1) {
		printf("xbdback_probe: %d segments\n",
		    xbdi->xen_req->nr_segments);
		xbdback_send_reply(xbdi, xbdi->xen_req->id,
		    xbdi->xen_req->operation, EINVAL);
		xbdi->cont = xbdback_co_main_incr;
		return xbdi;
	}
	xbdi->cont = xbdback_co_probe_gotio;
	return xbdback_pool_get(&xbdback_io_pool, xbdi);
}

static void *
xbdback_co_probe_gotio(struct xbdback_instance *xbdi, void *obj)
{
	struct xbdback_io *xbd_io;

	xbd_io = xbdi->io = obj;

	xbd_io->xio_xbdi = xbdi;
	xbd_io->xio_nrma = 1;
	xbd_io->xio_ma[0] = (xbdi->xen_req->frame_and_sects[0] & ~PAGE_MASK);
	xbd_io->xio_mapped = 0;

	xbdi->cont = xbdback_co_probe_gotvm;
	xbdi->cont_aux = xbdback_co_main_incr;
	return xbdback_map_shm(xbdi->io);
}

static void *
xbdback_co_probe_gotvm(struct xbdback_instance *xbdi, void *obj)
{
	struct xbd_vbd *vbd;
	blkif_request_t *req;
	vdisk_t *vdisk_reply;
	int i;

	vdisk_reply = (void *)xbdi->io->xio_vaddr;
	req = xbdi->xen_req;
	
	i = 0;
	SLIST_FOREACH(vbd, &xbdi->vbds, next) {
		if (i >= PAGE_SIZE / sizeof(vdisk_t)) {
			printf("xbdback_probe domain %d: too many VBDs\n",
			    xbdi->domid);
			break;
		}
		XENPRINTF(("xbdback_probe: reply %d\n", i));
		vdisk_reply[i].capacity = vbd->size;
		vdisk_reply[i].device = vbd->vdev;
		vdisk_reply[i].info = VDISK_TYPE_DISK | VDISK_FLAG_VIRT;
		if (vbd->flags & VBD_F_RO)
			vdisk_reply[i].info |= VDISK_FLAG_RO;
		i++;
	}
	xbdback_unmap_shm(xbdi->io);
	XENPRINTF(("xbdback_probe: nreplies=%d\n", i));
	xbdback_send_reply(xbdi, req->id, req->operation, i);
	xbdback_pool_put(&xbdback_io_pool, xbdi->io);
	xbdi->io = NULL;
	xbdi->cont = xbdback_co_main_incr;
	return xbdi;
}

/*
 * called once a request has completed. Place the reply in the ring and
 * notify the guest OS
 */
static void
xbdback_send_reply(struct xbdback_instance *xbdi, int id, int op, int status)
{
	blkif_response_t *resp;

	resp = &xbdi->blk_ring->ring[MASK_BLKIF_IDX(xbdi->resp_prod)].resp;
	resp->id        = id;
	resp->operation = op;
	resp->status    = status;
	xbdi->resp_prod++;
	x86_lfence(); /* ensure guest see all our replies */
	xbdi->blk_ring->resp_prod = xbdi->resp_prod;
	hypervisor_notify_via_evtchn(xbdi->evtchn);
}

/*
 * Map a request into our virtual address space.  The xbd_req->rq_ma
 * array is to be filled out by the caller.
 */
static void *
xbdback_map_shm(struct xbdback_io *xbd_io)
{
	struct xbdback_instance *xbdi;
	int error, s;

	KASSERT(xbd_io->xio_mapped == 0);

	xbdi = xbd_io->xio_xbdi;
	error = xen_shm_map(xbd_io->xio_ma, xbd_io->xio_nrma,
	    xbd_io->xio_xbdi->domid, &xbd_io->xio_vaddr, 0);

	switch(error) {
	case 0:
		xbd_io->xio_mapped = 1;
		return (void *)xbd_io->xio_vaddr;
	case ENOMEM:
		s = splvm();
		if (!xbdback_shmcb) {
			if (xen_shm_callback(xbdback_shm_callback, xbdi)
			    != 0) {
				splx(s);
				panic("xbdback_co_probe_gotio: "
				      "xen_shm_callback failed");
			}
			xbdback_shmcb = 1;
		}
		SIMPLEQ_INSERT_TAIL(&xbdback_shmq, xbdi, on_hold);
		splx(s);
		return NULL;
	default:
		printf("xbdback_co_probe_gotio: xen_shm error %d",
		       error);
		xbdback_io_error(xbdi->io, error);
		xbdi->io = NULL;
		xbdi->cont = xbdi->cont_aux;
		return xbdi;
	}
}

static int
xbdback_shm_callback(void *arg)
{
        int error, s;

	s = splvm();
	while(!SIMPLEQ_EMPTY(&xbdback_shmq)) {
		struct xbdback_instance *xbdi;
		struct xbdback_io *xbd_io;
		
		xbdi = SIMPLEQ_FIRST(&xbdback_shmq);
		xbd_io = xbdi->io;
		KASSERT(xbd_io->xio_mapped == 0);
		
		switch((error = xen_shm_map(xbd_io->xio_ma, xbd_io->xio_nrma,
		    xbdi->domid, &xbd_io->xio_vaddr, XSHM_CALLBACK))) {
		case ENOMEM:
			splx(s);
			return -1; /* will try again later */
		case 0:
			xbd_io->xio_mapped = 1;
			SIMPLEQ_REMOVE_HEAD(&xbdback_shmq, on_hold);
			splx(s);
			xbdback_trampoline(xbdi, xbdi);
			s = splvm();
			break;
		default:
			SIMPLEQ_REMOVE_HEAD(&xbdback_shmq, on_hold);
			splx(s);
			printf("xbdback_shm_callback: xen_shm error %d\n",
			       error);
			xbdi->cont = xbdi->cont_aux;
			xbdback_io_error(xbd_io, error);
			xbdback_trampoline(xbdi, xbdi);
			s = splvm();
			break;
		}
	}
	xbdback_shmcb = 0;
	splx(s);
	return 0;
}

/* unmap a request from our virtual address space (request is done) */
static void
xbdback_unmap_shm(struct xbdback_io *xbd_io)
{
	KASSERT(xbd_io->xio_mapped == 1);
	xbd_io->xio_mapped = 0;
	xen_shm_unmap(xbd_io->xio_vaddr, xbd_io->xio_ma,
	    xbd_io->xio_nrma, xbd_io->xio_xbdi->domid);
	xbd_io->xio_vaddr = -1;
}

/* Obtain memory from a pool, in cooperation with the continuations. */
static void *xbdback_pool_get(struct xbdback_pool *pp,
			      struct xbdback_instance *xbdi)
{
	int s;
	void *item;

	item = pool_get(&pp->p, PR_NOWAIT);
	if (item == NULL) {
		if (ratecheck(&pp->last_warning, &xbdback_poolsleep_intvl))
			printf("xbdback_pool_get: %s is full",
			       pp->p.pr_wchan);
		s = splvm();
		SIMPLEQ_INSERT_TAIL(&pp->q, xbdi, on_hold);
		splx(s);
	}
	return item;
}

/*
 * Restore memory to a pool... unless an xbdback instance had been
 * waiting for it, in which case that gets the memory first.
 */
static void xbdback_pool_put(struct xbdback_pool *pp, void *item)
{
	int s;
	
	s = splvm();
	if (SIMPLEQ_EMPTY(&pp->q)) {
		splx(s);
		pool_put(&pp->p, item);
	} else {
		struct xbdback_instance *xbdi = SIMPLEQ_FIRST(&pp->q);
		SIMPLEQ_REMOVE_HEAD(&pp->q, on_hold);
		splx(s);
		xbdback_trampoline(xbdi, item);
	}
}

static void xbdback_trampoline(struct xbdback_instance *xbdi, void *obj)
{
	xbdback_cont_t cont;

	while(obj != NULL && xbdi->cont != NULL) {
		cont = xbdi->cont;
#ifdef DIAGNOSTIC
		xbdi->cont = (xbdback_cont_t)0xDEADBEEF;
#endif
		obj = (*cont)(xbdi, obj);
#ifdef DIAGNOSTIC
		if (xbdi->cont == (xbdback_cont_t)0xDEADBEEF) {
			printf("xbdback_trampoline: 0x%lx didn't set "
			       "xbdi->cont!\n2", (long)cont);
			panic("xbdback_trampoline: bad continuation");
		}
#endif
	}
}
