/*      $NetBSD: xbdback.c,v 1.2 2005/03/09 22:39:21 bouyer Exp $      */

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

/* Values are exprimed in 512-byte sectors */
#define VBD_BSIZE 512

struct xbd_vbd;

/* state of a xbdback instance */
typedef enum {CONNECTED, DISCONNECTED} xbdback_state_t;

/* we keep the xbdback instances in a linked list */
struct xbdback_instance {
	SLIST_ENTRY(xbdback_instance) next; 
	domid_t domid;		/* attached to this domain */
	uint32_t handle;	/* domain-specific handle */
	xbdback_state_t status;
	/* parameters for the communication */
	unsigned int evtchn;
	unsigned int irq;
	paddr_t ma_ring;
	/* private parameters for communication */
	blkif_ring_t *blk_ring;
	BLKIF_RING_IDX resp_prod; /* our current reply index */
	BLKIF_RING_IDX req_cons; /* our current request index */

	SLIST_HEAD(, xbd_vbd) vbds; /* list of virtual block devices */
};

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
 * For each request from a guest, a xbdback_request is allocated from a pool.
 * This will describe the request until completion.
 */
struct xbdback_request {
	struct buf rq_buf; /* our I/O */
	vaddr_t rq_vaddr; /* the virtual address to map the request at */
	struct xbdback_instance *rq_xbdi; /* our xbd instance */
	/* from the request: */
	paddr_t rq_ma[BLKIF_MAX_PAGES_PER_REQUEST]; /* machine address to map */
	int rq_nrsegments; /* number of ma entries */
	int rq_id;
	int rq_operation;
};

struct pool xbdback_request_pool;

static void xbdback_ctrlif_rx(ctrl_msg_t *, unsigned long);
static int  xbdback_evthandler(void *);

static struct xbdback_instance *xbdif_lookup(domid_t, uint32_t);
static struct xbd_vbd * vbd_lookup(struct xbdback_instance *, blkif_vdev_t);

static int  xbdback_io(struct xbdback_instance *, blkif_request_t *);
static int  xbdback_shm_callback(void *);
static void xbdback_do_io(struct xbdback_request *);
static void xbdback_iodone(struct buf *);
static int  xbdback_probe(struct xbdback_instance *, blkif_request_t *);
static void xbdback_send_reply(struct xbdback_instance *, int , int , int);

static int  xbdback_map_shm(blkif_request_t *, struct xbdback_request *);
static void xbdback_unmap_shm(struct xbdback_request *);

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
	pool_init(&xbdback_request_pool, sizeof(struct xbdback_request),
	    0, 0, 0, "xbbp", NULL);
	/* we allocate enouth to handle a whole ring at once */
	if (pool_prime(&xbdback_request_pool, BLKIF_RING_SIZE) != 0)
		printf("xbdback: failed to prime pool\n");

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
		if (xbdi->status == CONNECTED) {
			req->status = BLKIF_BE_STATUS_INTERFACE_CONNECTED;
			goto end;
		}
		SLIST_REMOVE(&xbdback_instances, xbdi, xbdback_instance, next);
		free(xbdi, M_DEVBUF);
		req->status = BLKIF_BE_STATUS_OKAY;
		break;
	}
	case CMSG_BLKIF_BE_CONNECT:
	{
		blkif_be_connect_t *req = (blkif_be_connect_t *)&msg->msg[0];
		vaddr_t ring_addr;

		if (msg->length != sizeof(blkif_be_connect_t))
			goto error;
		xbdi = xbdif_lookup(req->domid, req->blkif_handle);
		if (xbdi == NULL) {
			req->status = BLKIF_BE_STATUS_INTERFACE_NOT_FOUND;
			goto end;
		}
		if (xbdi->status == CONNECTED) {
			req->status = BLKIF_BE_STATUS_INTERFACE_CONNECTED;
			goto end;
		}
		ring_addr = uvm_km_alloc(kernel_map, PAGE_SIZE);
		if (ring_addr == 0) {
			req->status = BLKIF_BE_STATUS_OUT_OF_MEMORY;
			goto end;
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
		xbdi->irq = bind_evtchn_to_irq(xbdi->evtchn);
		event_set_handler(xbdi->irq, xbdback_evthandler, xbdi, IPL_BIO);
		printf("xbd backend %d for domain %d interrupting at irq %d\n",
		    xbdi->handle, xbdi->domid, xbdi->irq);
		hypervisor_enable_irq(xbdi->irq);
		req->status = BLKIF_BE_STATUS_OKAY;
		break;
	}
	case CMSG_BLKIF_BE_DISCONNECT:
	{
		blkif_be_disconnect_t *req =
		    (blkif_be_disconnect_t *)&msg->msg[0];
		vaddr_t ring_addr;

		if (msg->length != sizeof(blkif_be_disconnect_t))
			goto error;
		xbdi = xbdif_lookup(req->domid, req->blkif_handle);
		if (xbdi == NULL) {
			req->status = BLKIF_BE_STATUS_INTERFACE_NOT_FOUND;
			goto end;
		}
		hypervisor_disable_irq(xbdi->irq);
		event_remove_handler(xbdi->irq, xbdback_evthandler, xbdi);
		unbind_evtchn_to_irq(xbdi->evtchn);
		ring_addr = (vaddr_t)xbdi->blk_ring;
		pmap_remove(pmap_kernel(), ring_addr, ring_addr + PAGE_SIZE);
		uvm_km_free(kernel_map, ring_addr, PAGE_SIZE);
		req->status = BLKIF_BE_STATUS_OKAY;
		break;
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
	BLKIF_RING_IDX req_prod;
	blkif_request_t *req;
	int error;

	req_prod = xbdi->blk_ring->req_prod;
	__insn_barrier(); /* ensure we see all requests up to req_prod */
	req_prod = MASK_BLKIF_IDX(req_prod);

	for (; xbdi->req_cons != req_prod;
	    xbdi->req_cons = MASK_BLKIF_IDX(xbdi->req_cons + 1)) {
		req = &xbdi->blk_ring->ring[xbdi->req_cons].req;
		switch(req->operation) {
		case BLKIF_OP_PROBE:
			error = xbdback_probe(xbdi, req);
			break;
		case BLKIF_OP_READ:
		case BLKIF_OP_WRITE:
			error = xbdback_io(xbdi, req);
			break;
		default:
			printf("xbdback_evthandler domain %d: unknown "
			    "operation %d\n", xbdi->domid, req->operation);
			error = EINVAL;
		}
		switch (error) {
		case 0:
			/* reply has already been sent, or will be */
			break;
		default:
			xbdback_send_reply(xbdi, req->id, req->operation,
			    BLKIF_RSP_ERROR);
			break;
		}
	}
	return 1;
}

static int
xbdback_io(struct xbdback_instance *xbdi, blkif_request_t *req)
{
	struct xbd_vbd *vbd;
	struct xbdback_request *xbd_req;
	int i, error = 0;
	vaddr_t start_offset; /* start offset in vm area */
	int req_size;

	if (req->nr_segments < 1 ||
	    req->nr_segments > BLKIF_MAX_SEGMENTS_PER_REQUEST ) {
		printf("xbdback_io domain %d: %d segments\n",
		    xbdi->domid, req->nr_segments);
		return EINVAL;
	}
	vbd = vbd_lookup(xbdi, req->device);
	if (vbd == NULL) {
		printf("xbdback_io domain %d: unknown vbd %d\n",
		    xbdi->domid, req->device);
		return EINVAL;
	}

	xbd_req = pool_get(&xbdback_request_pool, PR_NOWAIT);
	if (xbd_req == NULL) {
		return ENOMEM;
	}

	xbd_req->rq_xbdi = xbdi;

	start_offset =
	    blkif_first_sect(req->frame_and_sects[0]) * VBD_BSIZE;
	req_size = blkif_last_sect(req->frame_and_sects[0]) - 
	    blkif_first_sect(req->frame_and_sects[0]) + 1;
	XENPRINTF(("xbdback_io domain %d: frame_and_sects[0]=0x%lx\n",
	     xbdi->domid, req->frame_and_sects[0]));
	for (i = 1; i < req->nr_segments; i++) {
		XENPRINTF(("xbdback_io domain %d: frame_and_sects[%d]=0x%lx\n",
		     xbdi->domid, i, req->frame_and_sects[i]));
		req_size += blkif_last_sect(req->frame_and_sects[i]) - 
		    blkif_first_sect(req->frame_and_sects[i]) + 1;
	}
	if (req_size < 0) {
		printf("xbdback_io domain %d: negative-size request\n",
		    xbdi->domid);
		error = EINVAL;
		goto end;
	}
	req_size = req_size * VBD_BSIZE;
	XENPRINTF(("xbdback_io domain %d: start sect %d start %d size %d\n",
	    xbdi->domid, (int)req->sector_number, (int)start_offset, req_size));

	BUF_INIT(&xbd_req->rq_buf);
	if (req->operation == BLKIF_OP_WRITE) {
		if (vbd->flags & VBD_F_RO) {
			error = EROFS;
			goto end;
		}
		xbd_req->rq_buf.b_flags = B_WRITE | B_CALL;
	} else {
		xbd_req->rq_buf.b_flags = B_READ | B_CALL;
	}
	xbd_req->rq_buf.b_iodone = xbdback_iodone;
	xbd_req->rq_buf.b_proc = NULL;
	xbd_req->rq_buf.b_vp = vbd->vp;
	xbd_req->rq_buf.b_blkno = req->sector_number;
	xbd_req->rq_buf.b_bcount = (daddr_t)req_size;
	xbd_req->rq_buf.b_data = (void *)start_offset;
	xbd_req->rq_buf.b_private = xbd_req;
	switch(xbdback_map_shm(req, xbd_req)) {
	case 0:
		xbdback_do_io(xbd_req);
		return 0;
	case ENOMEM:
		if (xen_shm_callback(xbdback_shm_callback, xbd_req) == 0)
			return 0;
		error = ENOMEM;
		break;
	default:
		error = EINVAL;
		break;
	}
end:
	pool_put(&xbdback_request_pool, xbd_req);
	return error;
}

static int
xbdback_shm_callback(void *arg)
{
	struct xbdback_request *xbd_req = arg;

	switch(xen_shm_map(xbd_req->rq_ma, xbd_req->rq_nrsegments,
	    xbd_req->rq_xbdi->domid, &xbd_req->rq_vaddr, XSHM_CALLBACK)) {
	case ENOMEM:
		return -1; /* will try again later */
	case 0:
		xbdback_do_io(xbd_req);
		return 0;
	default:
		xbdback_send_reply(xbd_req->rq_xbdi, xbd_req->rq_id,
		    xbd_req->rq_operation, BLKIF_RSP_ERROR);
		pool_put(&xbdback_request_pool, xbd_req);
		return 0;
	}
}

static void
xbdback_do_io(struct xbdback_request *xbd_req)
{
	xbd_req->rq_buf.b_data =
	    (void *)((vaddr_t)xbd_req->rq_buf.b_data + xbd_req->rq_vaddr);
	if ((xbd_req->rq_buf.b_flags & B_READ) == 0)
		xbd_req->rq_buf.b_vp->v_numoutput++;
	XENPRINTF(("xbdback_io domain %d: start reqyest\n",
	    xbd_req->rq_xbdi->domid));
	VOP_STRATEGY(xbd_req->rq_buf.b_vp, &xbd_req->rq_buf);
}

static void
xbdback_iodone(struct buf *bp)
{
	struct xbdback_request *xbd_req = bp->b_private;
	struct xbdback_instance *xbdi = xbd_req->rq_xbdi;
	int error;

	xbdback_unmap_shm(xbd_req);
	if (bp->b_flags & B_ERROR) {
		printf("xbd IO domain %d: error %d\n",
		    xbdi->domid, bp->b_error);
		error = BLKIF_RSP_ERROR;
	} else
		error = BLKIF_RSP_OKAY;
	XENPRINTF(("xbdback_io domain %d: end reqest error=%d\n",
	    xbdi->domid, error));
	xbdback_send_reply(xbdi, xbd_req->rq_id,
	    xbd_req->rq_operation, error);
	pool_put(&xbdback_request_pool, xbd_req);
}

static int
xbdback_probe(struct xbdback_instance *xbdi, blkif_request_t *req)
{
	struct xbd_vbd *vbd;
	struct xbdback_request *xbd_req;
	vdisk_t *vdisk_reply;
	int i;

	/*
	 * There should be only one page in the request. Map it and store
	 * the reply
	 */
	if (req->nr_segments != 1) {
		printf("xbdback_probe: %d segments\n", req->nr_segments);
		return EINVAL;
	}
	xbd_req = pool_get(&xbdback_request_pool, PR_NOWAIT);
	if (xbd_req == NULL) {
		panic("xbd_req"); /* XXX */
	}
	xbd_req->rq_xbdi = xbdi;
	if (xbdback_map_shm(req, xbd_req) < 0) {
		pool_put(&xbdback_request_pool, xbd_req);
		return EINVAL;
	}
	vdisk_reply = (void *)xbd_req->rq_vaddr;

	i = 0;
	SLIST_FOREACH(vbd, &xbdi->vbds, next) {
		if (i >= PAGE_SIZE / sizeof(vdisk_t)) {
			printf("xbdback_probe domain %d: to many VBD\n",
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
	xbdback_unmap_shm(xbd_req);
	XENPRINTF(("xbdback_probe: nreplies=%d\n", i));
	xbdback_send_reply(xbdi, req->id, req->operation, i);
	pool_put(&xbdback_request_pool, xbd_req);
	return 0;
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
	__insn_barrier(); /* ensure guest see all our replies */
	xbdi->blk_ring->resp_prod = xbdi->resp_prod;
	hypervisor_notify_via_evtchn(xbdi->evtchn);
}

/* map a request into our virtual address space */
static int
xbdback_map_shm(blkif_request_t *req, struct xbdback_request *xbd_req)
{
	int i;

	xbd_req->rq_nrsegments = req->nr_segments;
	xbd_req->rq_id = req->id;
	xbd_req->rq_operation = req->operation;

#ifdef DIAGNOSTIC
	if (req->nr_segments <= 0 ||
	    req->nr_segments > BLKIF_MAX_SEGMENTS_PER_REQUEST) {
		printf("xbdback_map_shm: %d segments\n", req->nr_segments);
		panic("xbdback_map_shm");
	}
#endif

	for (i = 0; i < req->nr_segments; i++) {
		xbd_req->rq_ma[i] = (req->frame_and_sects[i] & ~PAGE_MASK);
	}
	return xen_shm_map(xbd_req->rq_ma, req->nr_segments,
		xbd_req->rq_xbdi->domid, &xbd_req->rq_vaddr, 0);
}

/* unmap a request from our virtual address space (request is done) */
static void
xbdback_unmap_shm(struct xbdback_request *xbd_req)
{
	xen_shm_unmap(xbd_req->rq_vaddr, xbd_req->rq_ma,
	    xbd_req->rq_nrsegments, xbd_req->rq_xbdi->domid);
	xbd_req->rq_vaddr = -1;
}
