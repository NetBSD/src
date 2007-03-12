/* $NetBSD: xbd.c,v 1.36.14.1 2007/03/12 05:51:49 rmind Exp $ */

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian Limpach.
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
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xbd.c,v 1.36.14.1 2007/03/12 05:51:49 rmind Exp $");

#include "xbd_hypervisor.h"
#include "rnd.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/lock.h>
#include <sys/conf.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/kernel.h>

#include <uvm/uvm.h>

#if NRND > 0
#include <sys/rnd.h>
#endif

#include <dev/dkvar.h>
#include <machine/xbdvar.h>

#include <machine/xen.h>
#include <machine/hypervisor.h>
#include <machine/evtchn.h>
#include <machine/ctrl_if.h>


static void	control_send(blkif_request_t *, blkif_response_t *);
static void	send_interface_connect(void);

static void xbd_attach(struct device *, struct device *, void *);
static int xbd_detach(struct device *, int);

#if NXBD_HYPERVISOR > 0
int xbd_match(struct device *, struct cfdata *, void *);
CFATTACH_DECL(xbd_hypervisor, sizeof(struct xbd_softc),
    xbd_match, xbd_attach, xbd_detach, NULL);

extern struct cfdriver xbd_cd;
#endif

#if NWD > 0
int xbd_wd_match(struct device *, struct cfdata *, void *);
CFATTACH_DECL(wd_xen, sizeof(struct xbd_softc),
    xbd_wd_match, xbd_attach, xbd_detach, NULL);

extern struct cfdriver wd_cd;
#endif

#if NSD > 0
int xbd_sd_match(struct device *, struct cfdata *, void *);
CFATTACH_DECL(sd_xen, sizeof(struct xbd_softc),
    xbd_sd_match, xbd_attach, xbd_detach, NULL);

extern struct cfdriver sd_cd;
#endif

#if NCD > 0
int xbd_cd_match(struct device *, struct cfdata *, void *);
CFATTACH_DECL(cd_xen, sizeof(struct xbd_softc),
    xbd_cd_match, xbd_attach, xbd_detach, NULL);

extern struct cfdriver cd_cd;
#endif


dev_type_open(xbdopen);
dev_type_close(xbdclose);
dev_type_read(xbdread);
dev_type_write(xbdwrite);
dev_type_ioctl(xbdioctl);
dev_type_ioctl(xbdioctl_cdev);
dev_type_strategy(xbdstrategy);
dev_type_dump(xbddump);
dev_type_size(xbdsize);

#if NXBD_HYPERVISOR > 0
const struct bdevsw xbd_bdevsw = {
	xbdopen, xbdclose, xbdstrategy, xbdioctl,
	xbddump, xbdsize, D_DISK
};

const struct cdevsw xbd_cdevsw = {
	xbdopen, xbdclose, xbdread, xbdwrite, xbdioctl_cdev,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

static dev_t xbd_major;
#endif

#if NWD > 0
const struct bdevsw wd_bdevsw = {
	xbdopen, xbdclose, xbdstrategy, xbdioctl,
	xbddump, xbdsize, D_DISK
};

const struct cdevsw wd_cdevsw = {
	xbdopen, xbdclose, xbdread, xbdwrite, xbdioctl_cdev,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

static dev_t xbd_wd_major;
static dev_t xbd_wd_cdev_major;
#endif

#if NSD > 0
const struct bdevsw sd_bdevsw = {
	xbdopen, xbdclose, xbdstrategy, xbdioctl,
	xbddump, xbdsize, D_DISK
};

const struct cdevsw sd_cdevsw = {
	xbdopen, xbdclose, xbdread, xbdwrite, xbdioctl_cdev,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

static dev_t xbd_sd_major;
static dev_t xbd_sd_cdev_major;
#endif

#if NCD > 0
const struct bdevsw cd_bdevsw = {
	xbdopen, xbdclose, xbdstrategy, xbdioctl,
	xbddump, xbdsize, D_DISK
};

const struct cdevsw cd_cdevsw = {
	xbdopen, xbdclose, xbdread, xbdwrite, xbdioctl_cdev,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

static dev_t xbd_cd_major;
static dev_t xbd_cd_cdev_major;
#endif

static struct dkdriver xbddkdriver = {
	.d_strategy = xbdstrategy,
	.d_minphys = minphys,
};

static int	xbdstart(struct dk_softc *, struct buf *);
static int	xbd_response_handler(void *);

static int	xbdinit(struct xbd_softc *, vdisk_t *, struct dk_intf *);

/* Pseudo-disk Interface */
static struct dk_intf dkintf_esdi = {
	DTYPE_ESDI,
	"Xen Virtual ESDI",
	xbdopen,
	xbdclose,
	xbdstrategy,
	xbdstart,
};
#if NSD > 0
static struct dk_intf dkintf_scsi = {
	DTYPE_SCSI,
	"Xen Virtual SCSI",
	xbdopen,
	xbdclose,
	xbdstrategy,
	xbdstart,
};
#endif

#if NXBD_HYPERVISOR > 0
static struct xbd_attach_args xbd_ata = {
	.xa_device = "xbd",
	.xa_dkintf = &dkintf_esdi,
};
#endif

#if NWD > 0
static struct xbd_attach_args wd_ata = {
	.xa_device = "wd",
	.xa_dkintf = &dkintf_esdi,
};
#endif

#if NSD > 0
static struct xbd_attach_args sd_ata = {
	.xa_device = "sd",
	.xa_dkintf = &dkintf_scsi,
};
#endif

#if NCD > 0
static struct xbd_attach_args cd_ata = {
	.xa_device = "cd",
	.xa_dkintf = &dkintf_esdi,
};
#endif

#if defined(XBDDEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#ifdef DEBUG
#define XBDB_FOLLOW	0x1
#define XBDB_IO		0x2
#define XBDB_SETUP	0x4
#define XBDB_HOTPLUG	0x8

#define IFDEBUG(x,y)		if (xbddebug & (x)) y
#define DPRINTF(x,y)		IFDEBUG(x, printf y)
#define DPRINTF_FOLLOW(y)	DPRINTF(XBDB_FOLLOW, y)

int xbddebug = 0;

#define	DEBUG_MARK_UNUSED(_xr)	(_xr)->xr_sc = (void *)0xdeadbeef

struct xbdreq *xbd_allxr;
#else
#define IFDEBUG(x,y)
#define DPRINTF(x,y)
#define DPRINTF_FOLLOW(y)
#define	DEBUG_MARK_UNUSED(_xr)
#endif

#ifdef DIAGNOSTIC
#define DIAGPANIC(x)		panic x 
#define DIAGCONDPANIC(x,y)	if (x) panic y
#else
#define DIAGPANIC(x)
#define DIAGCONDPANIC(x,y)
#endif


struct xbdreq {
	union {
		SLIST_ENTRY(xbdreq) _unused;	/* ptr. to next free xbdreq */
		SIMPLEQ_ENTRY(xbdreq) _suspended;
					/* link when on suspended queue. */
	} _link;
	struct xbdreq		*xr_parent;	/* ptr. to parent xbdreq */
	struct buf		*xr_bp;		/* ptr. to original I/O buf */
	daddr_t			xr_bn;		/* block no. to process */
	long			xr_bqueue;	/* bytes left to queue */
	long			xr_bdone;	/* bytes left */
	vaddr_t			xr_data;	/* ptr. to data to be proc. */
	vaddr_t			xr_aligned;	/* ptr. to aligned data */
	long			xr_breq;	/* bytes in this req. */
	struct xbd_softc	*xr_sc;		/* ptr. to xbd softc */
};
#define	xr_unused	_link._unused
#define	xr_suspended	_link._suspended

SLIST_HEAD(,xbdreq) xbdreqs =
	SLIST_HEAD_INITIALIZER(xbdreqs);
static SIMPLEQ_HEAD(, xbdreq) xbdr_suspended =
	SIMPLEQ_HEAD_INITIALIZER(xbdr_suspended);

#define	CANGET_XBDREQ() (!SLIST_EMPTY(&xbdreqs))

#define	GET_XBDREQ(_xr) do {				\
	(_xr) = SLIST_FIRST(&xbdreqs);			\
	if (__predict_true(_xr))			\
		SLIST_REMOVE_HEAD(&xbdreqs, xr_unused);	\
} while (/*CONSTCOND*/0)

#define	PUT_XBDREQ(_xr) do {				\
	DEBUG_MARK_UNUSED(_xr);				\
	SLIST_INSERT_HEAD(&xbdreqs, _xr, xr_unused);	\
} while (/*CONSTCOND*/0)

static struct bufq_state *bufq;
static int bufq_users = 0;

#define XEN_MAJOR(_dev)	((_dev) >> 8)
#define XEN_MINOR(_dev)	((_dev) & 0xff)

#define	XEN_SCSI_DISK0_MAJOR	8
#define	XEN_SCSI_DISK1_MAJOR	65
#define	XEN_SCSI_DISK2_MAJOR	66
#define	XEN_SCSI_DISK3_MAJOR	67
#define	XEN_SCSI_DISK4_MAJOR	68
#define	XEN_SCSI_DISK5_MAJOR	69
#define	XEN_SCSI_DISK6_MAJOR	70
#define	XEN_SCSI_DISK7_MAJOR	71
#define	XEN_SCSI_DISK8_MAJOR	128
#define	XEN_SCSI_DISK9_MAJOR	129
#define	XEN_SCSI_DISK10_MAJOR	130
#define	XEN_SCSI_DISK11_MAJOR	131
#define	XEN_SCSI_DISK12_MAJOR	132
#define	XEN_SCSI_DISK13_MAJOR	133
#define	XEN_SCSI_DISK14_MAJOR	134
#define	XEN_SCSI_DISK15_MAJOR	135
#define	XEN_SCSI_CDROM_MAJOR	11

#define	XEN_IDE0_MAJOR		3
#define	XEN_IDE1_MAJOR		22
#define	XEN_IDE2_MAJOR		33
#define	XEN_IDE3_MAJOR		34
#define	XEN_IDE4_MAJOR		56
#define	XEN_IDE5_MAJOR		57
#define	XEN_IDE6_MAJOR		88
#define	XEN_IDE7_MAJOR		89
#define	XEN_IDE8_MAJOR		90
#define	XEN_IDE9_MAJOR		91

#define	XEN_BSHIFT	9		/* log2(XEN_BSIZE) */
#define	XEN_BSIZE	(1 << XEN_BSHIFT)

#define MAX_VBDS 64
static int nr_vbds;
static vdisk_t *vbd_info;

static blkif_ring_t *blk_ring = NULL;
static BLKIF_RING_IDX resp_cons; /* Response consumer for comms ring. */
static BLKIF_RING_IDX req_prod;  /* Private request producer.         */
static BLKIF_RING_IDX last_req_prod;  /* Request producer at last trap. */

/* 
 * The "recovery ring" allows re-issuing requests that had been made but
 * not performed when the domain was suspended.  It contains a shadow of
 * the actual request ring, except that it stores pseudo-physical
 * addresses (which persist across save/restore or migration) instead of
 * machine addresses, and its elements won't necessarily be in the same
 * order as the other ring, as the backend need not respond to requests
 * in the same order that they were made.  Because of this latter, the
 * recovery ring slots are dynamically allocated with a free list
 * threaded through the "id" field of the unused entries.  And, as a
 * result of that, the request id in the actual ring will be an index
 * into the recovery ring so the slot can be freed when we get the
 * response, while the "id" field in the recovery ring element contains
 * the cast pointer to our higher-level request structure.
 */
static blkif_request_t *rec_ring = NULL;
#define REC_RING_NIL (PAGE_SIZE - 1)
#define REC_RING_ISALLOC(idx) (rec_ring[(idx)].id >= PAGE_SIZE)
static unsigned int rec_ring_free = REC_RING_NIL;
static int recovery = 0;
/* There is no simplelock; this is for Xen 2, and thus uniprocessor. */

static void signal_requests_to_xen(void);

#define STATE_CLOSED		0
#define STATE_DISCONNECTED	1
#define STATE_CONNECTED		2
static unsigned int state = STATE_CLOSED;

static int in_autoconf; /* are we still in autoconf ? */
static unsigned int blkif_evtchn = 0;
static unsigned int blkif_handle = 0;

static int blkif_control_rsp_valid = 0;
static blkif_response_t blkif_control_rsp;

/* block device interface info. */
struct xbd_ctrl {

	cfprint_t xc_cfprint;
	struct device *xc_parent;
};

static struct xbd_ctrl blkctrl;

#define XBDUNIT(x)		DISKUNIT(x)
#define GETXBD_SOFTC(_xs, x)	if (!((_xs) = getxbd_softc(x))) return ENXIO
#define GETXBD_SOFTC_CDEV(_xs, x) do {			\
	dev_t bx = devsw_chr2blk((x));			\
	if (bx == NODEV)				\
		return ENXIO;				\
	if (!((_xs) = getxbd_softc(bx)))		\
		return ENXIO;				\
} while (/*CONSTCOND*/0)

static void
rec_ring_init(void)
{
	int i;

	KASSERT(rec_ring == NULL);
	KASSERT(rec_ring_free == REC_RING_NIL);
	
	rec_ring = malloc(BLKIF_RING_SIZE * sizeof(blkif_request_t),
	    M_DEVBUF, 0);
	for (i = BLKIF_RING_SIZE - 1; i >= 0; --i) {
		rec_ring[i].id = rec_ring_free;
		rec_ring_free = i;
	}
}

/* Given an actual request, save it on the rec_ring and change its id. */
static void
rec_ring_log(blkif_request_t *rreq) 
{
	unsigned long idx, i;
	
	KASSERT(rreq->id >= PAGE_SIZE);
	if (rec_ring_free == REC_RING_NIL)
		panic("xbd: recovery ring exhausted");
	idx = rec_ring_free;
	KASSERT(!REC_RING_ISALLOC(idx));
	rec_ring_free = rec_ring[idx].id;
	
	rec_ring[idx].operation = rreq->operation;
	rec_ring[idx].nr_segments = rreq->nr_segments;
	rec_ring[idx].device = rreq->device;
	rec_ring[idx].id = rreq->id;
	rreq->id = idx;
	rec_ring[idx].device = rreq->device;
	
	for (i = 0; i < rreq->nr_segments; ++i)
		rec_ring[idx].frame_and_sects[i] =
		    xpmap_mtop(rreq->frame_and_sects[i]);	
}

/* 
 * Given a response to a request processed as above, free the rec_ring
 * slot and return the original id.
 */
static unsigned long
rec_ring_retire(blkif_response_t* resp)
{
	unsigned long idx, rid;
	
	idx = resp->id;
	KASSERT(idx < BLKIF_RING_SIZE);
	rid = rec_ring[idx].id;
	
	rec_ring[idx].id = rec_ring_free;
	rec_ring_free = idx;

	return rid;
}

/* Given the index of an allocated rec_ring slot, recover it. */
static void
rec_ring_recover(unsigned long idx, blkif_request_t *req)
{
	unsigned long i;

	KASSERT(idx < BLKIF_RING_SIZE);
	KASSERT(REC_RING_ISALLOC(idx));
	
	req->operation = rec_ring[idx].operation;
	req->nr_segments = rec_ring[idx].nr_segments;
	req->device = rec_ring[idx].device;
	req->id = idx;
	req->sector_number = rec_ring[idx].sector_number;

	for (i = 0; i < req->nr_segments; ++i)
		req->frame_and_sects[i] =
		    xpmap_ptom(rec_ring[idx].frame_and_sects[i]);
}

static struct xbd_softc *
getxbd_softc(dev_t dev)
{
	int	unit = XBDUNIT(dev);

	DPRINTF_FOLLOW(("getxbd_softc(0x%x): major = %d unit = %d\n", dev,
	    major(dev), unit));
#if NXBD_HYPERVISOR > 0
	if (major(dev) == xbd_major)
		return device_lookup(&xbd_cd, unit);
#endif
#if NWD > 0
	if (major(dev) == xbd_wd_major || major(dev) == xbd_wd_cdev_major)
		return device_lookup(&wd_cd, unit);
#endif
#if NSD > 0
	if (major(dev) == xbd_sd_major || major(dev) == xbd_sd_cdev_major)
		return device_lookup(&sd_cd, unit);
#endif
#if NCD > 0
	if (major(dev) == xbd_cd_major || major(dev) == xbd_cd_cdev_major)
		return device_lookup(&cd_cd, unit);
#endif
	return NULL;
}

static int
get_vbd_info(vdisk_t *disk_info)
{
	vdisk_t *buf;
	int nr;
	blkif_request_t req;
	blkif_response_t rsp;
	paddr_t pa;

	buf = (vdisk_t *)uvm_km_alloc(kmem_map, PAGE_SIZE, PAGE_SIZE,
	    UVM_KMF_WIRED);
	pmap_extract(pmap_kernel(), (vaddr_t)buf, &pa);
	/* Probe for disk information. */
	memset(&req, 0, sizeof(req));
	req.operation = BLKIF_OP_PROBE;
	req.nr_segments = 1;
	req.frame_and_sects[0] = xpmap_ptom_masked(pa) | 7;

	control_send(&req, &rsp);
	nr = rsp.status > MAX_VBDS ? MAX_VBDS : rsp.status;

	if (rsp.status < 0)
		printf("WARNING: Could not probe disks (%d)\n", rsp.status);

	memcpy(disk_info, buf, nr * sizeof(vdisk_t));

	uvm_km_free(kmem_map, (vaddr_t)buf, PAGE_SIZE, UVM_KMF_WIRED);

	return nr;
}

static struct xbd_attach_args *
get_xbda(vdisk_t *xd)
{

	switch (XEN_MAJOR(xd->device)) {
#if NSD > 0
	case XEN_SCSI_DISK0_MAJOR:
	case XEN_SCSI_DISK1_MAJOR ... XEN_SCSI_DISK7_MAJOR:
	case XEN_SCSI_DISK8_MAJOR ... XEN_SCSI_DISK15_MAJOR:
		if (xd->capacity == 0)
			return NULL;
		return &sd_ata;
	case XEN_SCSI_CDROM_MAJOR:
		return &cd_ata;
#endif
#if NWD > 0
	case XEN_IDE0_MAJOR:
	case XEN_IDE1_MAJOR:
	case XEN_IDE2_MAJOR:
	case XEN_IDE3_MAJOR:
	case XEN_IDE4_MAJOR:
	case XEN_IDE5_MAJOR:
	case XEN_IDE6_MAJOR:
	case XEN_IDE7_MAJOR:
	case XEN_IDE8_MAJOR:
	case XEN_IDE9_MAJOR:
		switch (VDISK_TYPE(xd->info)) {
		case VDISK_TYPE_CDROM:
			return &cd_ata;
		case VDISK_TYPE_DISK:
			if (xd->capacity == 0)
				return NULL;
			return &wd_ata;
		default:
			return NULL;
		}
		break;
#endif
	default:
		if (xd->capacity == 0)
			return NULL;
		return &xbd_ata;
	}
	return NULL;
}

static void
free_interface(void)
{

	/* Prevent new requests being issued until we fix things up. */
	// simple_lock(&blkif_io_lock);
	recovery = 1;
	state = STATE_DISCONNECTED;
	// simple_unlock(&blkif_io_lock);

	/* Free resources associated with old device channel. */
	if (blk_ring) {
		uvm_km_free(kmem_map, (vaddr_t)blk_ring, PAGE_SIZE,
		    UVM_KMF_WIRED);
		blk_ring = NULL;
	}

	if (blkif_evtchn) {
		event_remove_handler(blkif_evtchn, &xbd_response_handler, NULL);
		blkif_evtchn = 0;
	}
}

static void
close_interface(void){
}

static void
disconnect_interface(void)
{
	
	if (blk_ring == NULL)
		blk_ring = (blkif_ring_t *)uvm_km_alloc(kmem_map,
		    PAGE_SIZE, PAGE_SIZE, UVM_KMF_WIRED);
	memset(blk_ring, 0, PAGE_SIZE);
	blk_ring->req_prod = blk_ring->resp_prod = resp_cons = req_prod =
		last_req_prod = 0;
	if (rec_ring == NULL)
		rec_ring_init();
	state = STATE_DISCONNECTED;
	send_interface_connect();
}

static void
reset_interface(void)
{

	printf("xbd: recovering virtual block device driver\n");
	free_interface();
	disconnect_interface();
}

static void
recover_interface(void)
{
	unsigned long i;

	for (i = 0; i < BLKIF_RING_SIZE; ++i)
		if (REC_RING_ISALLOC(i)) {
			rec_ring_recover(i,
			    &blk_ring->ring[MASK_BLKIF_IDX(req_prod)].req);
			++req_prod;
		}
	recovery = 0;
	x86_sfence();
	signal_requests_to_xen();
}

static void
connect_interface(blkif_fe_interface_status_t *status)
{
	// unsigned long flags;
	struct xbd_attach_args *xbda;
	vdisk_t *xd;
	int i;

	blkif_evtchn = status->evtchn;

	aprint_verbose("xbd: using event channel %d\n", blkif_evtchn);

	event_set_handler(blkif_evtchn, &xbd_response_handler, NULL, IPL_BIO,
	    "xbd");
	hypervisor_enable_event(blkif_evtchn);

	if (recovery) {
		recover_interface();
		state = STATE_CONNECTED;
	} else {
		/* Transition to connected in case we need to do 
		 *  a partition probe on a whole disk. */
		state = STATE_CONNECTED;

		/* Probe for discs attached to the interface. */
		// xlvbd_init();
		MALLOC(vbd_info, vdisk_t *, MAX_VBDS * sizeof(vdisk_t),
		    M_DEVBUF, M_WAITOK | M_ZERO);
		nr_vbds  = get_vbd_info(vbd_info);
		if (nr_vbds <= 0)
			goto out;

		for (i = 0; i < nr_vbds; i++) {
			xd = &vbd_info[i];
			xbda = get_xbda(xd);
			if (xbda) {
				xbda->xa_xd = xd;
				config_found_ia(blkctrl.xc_parent, "xendevbus", xbda,
				    blkctrl.xc_cfprint);
			}
		}
	}
#if 0
	/* Kick pending requests. */
	save_and_cli(flags);
	// simple_lock(&blkif_io_lock);
	kick_pending_request_queues();
	// simple_unlock(&blkif_io_lock);
	restore_flags(flags);
#endif
	if (in_autoconf) {
		in_autoconf = 0;
		config_pending_decr();
	}
	return;

 out:
	FREE(vbd_info, M_DEVBUF);
	vbd_info = NULL;
	if (in_autoconf) {
		in_autoconf = 0;
		config_pending_decr();
	}
	return;
}

static struct device *
find_device(vdisk_t *xd)
{
	struct device *dv;
	struct xbd_softc *xs = NULL;

	for (dv = alldevs.tqh_first; dv != NULL; dv = dv->dv_list.tqe_next) {
		if (!device_is_a(dv, "xbd"))
			continue;
		xs = (struct xbd_softc *)dv;
		if (xd == NULL || xs->sc_xd_device == xd->device)
			break;
	}
	return dv;
}

static void
vbd_update(void)
{
	struct xbd_attach_args *xbda;
	struct device *dev;
	vdisk_t *xd;
	vdisk_t *vbd_info_update, *vbd_info_old;
	int i, j, new_nr_vbds;
	extern int hypervisor_print(void *, const char *);

	MALLOC(vbd_info_update, vdisk_t *, MAX_VBDS *
	    sizeof(vdisk_t), M_DEVBUF, M_WAITOK | M_ZERO);

	new_nr_vbds  = get_vbd_info(vbd_info_update);

	if (memcmp(vbd_info, vbd_info_update, MAX_VBDS *
	    sizeof(vdisk_t)) == 0) {
		FREE(vbd_info_update, M_DEVBUF);
		return;
	}

	for (i = 0, j = 0; i < nr_vbds && j < new_nr_vbds;)  {
		if (vbd_info[i].device > vbd_info_update[j].device) {
			DPRINTF(XBDB_HOTPLUG,
			    ("delete device %x size %lx\n",
				vbd_info[i].device,
				(u_long)vbd_info[i].capacity));
			xd = &vbd_info[i];
			dev = find_device(xd);
			if (dev)
				config_detach(dev, DETACH_FORCE);
			i++;
		} else if (vbd_info_update[j].device > vbd_info[i].device) {
			DPRINTF(XBDB_HOTPLUG,
			    ("add device %x size %lx\n",
				vbd_info_update[j].device,
				(u_long)vbd_info_update[j].capacity));
			xd = &vbd_info_update[j];
			xbda = get_xbda(xd);
			if (xbda) {
				xbda->xa_xd = xd;
				config_found_ia(blkctrl.xc_parent, "xendevbus",
				    xbda, blkctrl.xc_cfprint);
			}
			j++;
		} else {
			i++; j++;
			DPRINTF(XBDB_HOTPLUG,
			    ("update device %x size %lx size %lx\n",
				vbd_info_update[i].device,
				(u_long)vbd_info[j].capacity,
				(u_long)vbd_info_update[i].capacity));
		}
	}
	for (; i < nr_vbds; i++) {
		DPRINTF(XBDB_HOTPLUG,
		    ("delete device %x size %lx\n",
			vbd_info[i].device,
			(u_long)vbd_info[i].capacity));
		xd = &vbd_info[i];
		dev = find_device(xd);
		if (dev)
			config_detach(dev, DETACH_FORCE);
	}
	for (; j < new_nr_vbds; j++) {
		DPRINTF(XBDB_HOTPLUG,
		    ("add device %x size %lx\n",
			vbd_info_update[j].device,
			(u_long)vbd_info_update[j].capacity));
		xd = &vbd_info_update[j];
		xbda = get_xbda(xd);
		if (xbda) {
			xbda->xa_xd = xd;
			config_found_ia(blkctrl.xc_parent, "xendevbus", xbda,
			    blkctrl.xc_cfprint);
		}
	}

	nr_vbds = new_nr_vbds;

	vbd_info_old = vbd_info;
	vbd_info = vbd_info_update;
	FREE(vbd_info_old, M_DEVBUF);
}

static void
unexpected(blkif_fe_interface_status_t *status)
{

	printf("Unexpected blkif status %d in state %d\n", 
	    status->status, state);
}

static void
blkif_status(blkif_fe_interface_status_t *status)
{

	if (status->handle != blkif_handle) {
		printf("Invalid blkif: handle=%u", status->handle);
		return;
	}

	switch (status->status) {
	case BLKIF_INTERFACE_STATUS_CLOSED:
		switch (state) {
		case STATE_CLOSED:
			unexpected(status);
			break;
		case STATE_DISCONNECTED:
		case STATE_CONNECTED:
			unexpected(status);
			close_interface();
			break;
		}
		break;

	case BLKIF_INTERFACE_STATUS_DISCONNECTED:
		switch (state) {
		case STATE_CLOSED:
			disconnect_interface();
			break;
		case STATE_DISCONNECTED:
		case STATE_CONNECTED:
//			unexpected(status);
			reset_interface();
			break;
		}
		break;

	case BLKIF_INTERFACE_STATUS_CONNECTED:
		switch (state) {
		case STATE_CLOSED:
			unexpected(status);
			disconnect_interface();
			connect_interface(status);
			break;
		case STATE_DISCONNECTED:
			connect_interface(status);
			break;
		case STATE_CONNECTED:
			unexpected(status);
			connect_interface(status);
			break;
		}
		break;

	case BLKIF_INTERFACE_STATUS_CHANGED:
		switch (state) {
		case STATE_CLOSED:
		case STATE_DISCONNECTED:
			unexpected(status);
			break;
		case STATE_CONNECTED:
			vbd_update();
			break;
		}
		break;

	default:
		printf(" Invalid blkif status: %d\n", status->status);
		break;
	}
}


static void
xbd_ctrlif_rx(ctrl_msg_t *msg, unsigned long id)
{
	switch (msg->subtype) {
	case CMSG_BLKIF_FE_INTERFACE_STATUS:
		if (msg->length != sizeof(blkif_fe_interface_status_t))
			goto parse_error;
		blkif_status((blkif_fe_interface_status_t *)
		    &msg->msg[0]);
		break;        
	default:
		goto parse_error;
	}

	ctrl_if_send_response(msg);
	return;

 parse_error:
	msg->length = 0;
	ctrl_if_send_response(msg);
}

#if 0
static void
enable_update_events(struct device *self)
{

	kthread_create(xbd_update_create_kthread, self);
	event_set_handler(_EVENT_VBD_UPD, &xbd_update_handler, self, IPL_BIO,
	    "xbdup");
	hypervisor_enable_event(_EVENT_VBD_UPD);
}
#endif

static void
signal_requests_to_xen(void)
{

	DPRINTF(XBDB_IO, ("signal_requests_to_xen: %x -> %x\n",
		    blk_ring->req_prod, req_prod));
	blk_ring->req_prod = req_prod;
	last_req_prod = req_prod;

	hypervisor_notify_via_evtchn(blkif_evtchn);
	return;
}

static void
control_send(blkif_request_t *req, blkif_response_t *rsp)
{
	unsigned long flags;
	struct xbdreq *xr;
	blkif_request_t *ring_req;

 retry:
	while ((req_prod - resp_cons) == BLKIF_RING_SIZE) {
		/* XXX where is the wakeup ? */
		tsleep((void *) &req_prod, PUSER | PCATCH,
		    "blkfront", 0);
	}

	save_and_cli(flags);
	// simple_lock(&blkif_io_lock);
	if ((req_prod - resp_cons) == BLKIF_RING_SIZE) {
		// simple_unlock(&blkif_io_lock);
		restore_flags(flags);
		goto retry;
	}

	ring_req = &blk_ring->ring[MASK_BLKIF_IDX(req_prod)].req;
	*ring_req = *req;

	GET_XBDREQ(xr);
	ring_req->id = (unsigned long)xr;

	rec_ring_log(ring_req);
	req_prod++;
	signal_requests_to_xen();

	// simple_unlock(&blkif_io_lock);
	restore_flags(flags);

	while (!blkif_control_rsp_valid) {
		tsleep((void *)&blkif_control_rsp_valid, PUSER | PCATCH,
		    "blkfront", 0);
	}

	memcpy(rsp, &blkif_control_rsp, sizeof(*rsp));
	blkif_control_rsp_valid = 0;
}

/* Send a driver status notification to the domain controller. */
static void
send_driver_status(int ok)
{
	ctrl_msg_t cmsg = {
		.type    = CMSG_BLKIF_FE,
		.subtype = CMSG_BLKIF_FE_DRIVER_STATUS,
		.length  = sizeof(blkif_fe_driver_status_t),
	};
	blkif_fe_driver_status_t *msg = (blkif_fe_driver_status_t *)cmsg.msg;
    
	msg->status = ok ? BLKIF_DRIVER_STATUS_UP : BLKIF_DRIVER_STATUS_DOWN;

	ctrl_if_send_message_block(&cmsg, NULL, 0, 0);
}

/* Tell the controller to bring up the interface. */
static void
send_interface_connect(void)
{
	ctrl_msg_t cmsg = {
		.type    = CMSG_BLKIF_FE,
		.subtype = CMSG_BLKIF_FE_INTERFACE_CONNECT,
		.length  = sizeof(blkif_fe_interface_connect_t),
	};
	blkif_fe_interface_connect_t *msg =
		(blkif_fe_interface_connect_t *)cmsg.msg;
    	paddr_t pa;

	pmap_extract(pmap_kernel(), (vaddr_t)blk_ring, &pa);

	msg->handle = 0;
	msg->shmem_frame = xpmap_ptom_masked(pa) >> PAGE_SHIFT;

	ctrl_if_send_message_block(&cmsg, NULL, 0, 0);
}


int
xbd_scan(struct device *self, struct xbd_attach_args *mainbus_xbda,
    cfprint_t print)
{
	struct xbdreq *xr;
	int i;

	blkctrl.xc_parent = self;
	blkctrl.xc_cfprint = print;

#if NXBD_HYPERVISOR > 0
	xbd_major = devsw_name2blk("xbd", NULL, 0);
#endif
#if NWD > 0
	xbd_wd_major = devsw_name2blk("wd", NULL, 0);
	/* XXX Also handle the cdev majors since stuff like
	 * read_sector calls strategy on the cdev.  This only works if
	 * all the majors we care about are different.
	 */
	xbd_wd_cdev_major = major(devsw_blk2chr(makedev(xbd_wd_major, 0)));
#endif
#if NSD > 0
	xbd_sd_major = devsw_name2blk("sd", NULL, 0);
	xbd_sd_cdev_major = major(devsw_blk2chr(makedev(xbd_sd_major, 0)));
#endif
#if NCD > 0
	xbd_cd_major = devsw_name2blk("cd", NULL, 0);
	xbd_cd_cdev_major = major(devsw_blk2chr(makedev(xbd_cd_major, 0)));
#endif

	MALLOC(xr, struct xbdreq *, BLKIF_RING_SIZE * sizeof(struct xbdreq),
	    M_DEVBUF, M_WAITOK | M_ZERO);
#ifdef DEBUG
	xbd_allxr = xr;
#endif
	for (i = 0; i < BLKIF_RING_SIZE - 1; i++)
		PUT_XBDREQ(&xr[i]);

	(void)ctrl_if_register_receiver(CMSG_BLKIF_FE, xbd_ctrlif_rx,
	    CALLBACK_IN_BLOCKING_CONTEXT);

	in_autoconf = 1;
	config_pending_incr();

	send_driver_status(1);

	return 0;
}

#if NXBD_HYPERVISOR > 0
int
xbd_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct xbd_attach_args *xa = (struct xbd_attach_args *)aux;

	if (strcmp(xa->xa_device, "xbd") == 0)
		return 1;
	return 0;
}
#endif

#if NWD > 0
int
xbd_wd_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct xbd_attach_args *xa = (struct xbd_attach_args *)aux;

	if (strcmp(xa->xa_device, "wd") == 0)
		return 1;
	return 0;
}
#endif

#if NSD > 0
int
xbd_sd_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct xbd_attach_args *xa = (struct xbd_attach_args *)aux;

	if (strcmp(xa->xa_device, "sd") == 0)
		return 1;
	return 0;
}
#endif

#if NCD > 0
int
xbd_cd_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct xbd_attach_args *xa = (struct xbd_attach_args *)aux;

	if (strcmp(xa->xa_device, "cd") == 0)
		return 1;
	return 0;
}
#endif

static void
xbd_attach(struct device *parent, struct device *self, void *aux)
{
	struct xbd_attach_args *xbda = (struct xbd_attach_args *)aux;
	struct xbd_softc *xs = (struct xbd_softc *)self;

	aprint_normal(": Xen Virtual Block Device");

	simple_lock_init(&xs->sc_slock);
	dk_sc_init(&xs->sc_dksc, xs, xs->sc_dev.dv_xname);
	xs->sc_dksc.sc_dkdev.dk_driver = &xbddkdriver;
	xbdinit(xs, xbda->xa_xd, xbda->xa_dkintf);

#if NRND > 0
	rnd_attach_source(&xs->sc_rnd_source, xs->sc_dev.dv_xname,
	    RND_TYPE_DISK, 0);
#endif
}

static int
xbd_detach(struct device *dv, int flags)
{
	struct	xbd_softc *xs = (struct	xbd_softc *)dv;
	int bmaj, cmaj, mn, i;

	/* 
	 * Mark disk about to be removed (between now and when the xs
	 * will be freed).
	 */
	xs->sc_shutdown = 1;

	/* And give it some time to settle if it's busy. */
	if (xs->sc_dksc.sc_dkdev.dk_stats->io_busy > 0)
		tsleep(&xs, PWAIT, "xbdetach", hz);

	/* locate the major number */
	bmaj = bdevsw_lookup_major(&xbd_bdevsw);
	cmaj = cdevsw_lookup_major(&xbd_cdevsw);

	for (i = 0; i < MAXPARTITIONS; i++) {
		mn = DISKMINOR(device_unit(dv), i);
		vdevgone(bmaj, mn, mn, VBLK);
		vdevgone(cmaj, mn, mn, VCHR);
	}

	/* Delete all of our wedges. */
	dkwedge_delall(&xs->sc_dksc.sc_dkdev);

#if 0
	s = splbio();
	/* Kill off any queued buffers. */
	bufq_drain(&sd->buf_queue);
	bufq_free(&sd->buf_queue);

	splx(s);
#endif

	/* Detach the disk. */
	disk_detach(&xs->sc_dksc.sc_dkdev);

#if NRND > 0
	/* Unhook the entropy source. */
	rnd_detach_source(&xs->sc_rnd_source);
#endif
	return 0;
}

int
xbdopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct	xbd_softc *xs;

	DPRINTF_FOLLOW(("xbdopen(0x%04x, %d)\n", dev, flags));
	switch (fmt) {
	case S_IFCHR:
		GETXBD_SOFTC_CDEV(xs, dev);
		break;
	case S_IFBLK:
		GETXBD_SOFTC(xs, dev);
		break;
	default:
		return ENXIO;
	}
	return dk_open(xs->sc_di, &xs->sc_dksc, dev, flags, fmt, l);
}

int
xbdclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct	xbd_softc *xs;

	DPRINTF_FOLLOW(("xbdclose(%d, %d)\n", dev, flags));
	switch (fmt) {
	case S_IFCHR:
		GETXBD_SOFTC_CDEV(xs, dev);
		break;
	case S_IFBLK:
		GETXBD_SOFTC(xs, dev);
		break;
	default:
		return ENXIO;
	}
	return dk_close(xs->sc_di, &xs->sc_dksc, dev, flags, fmt, l);
}

void
xbdstrategy(struct buf *bp)
{
	struct	xbd_softc *xs = getxbd_softc(bp->b_dev);

	DPRINTF_FOLLOW(("xbdstrategy(%p): b_bcount = %ld\n", bp,
	    (long)bp->b_bcount));

	if (xs == NULL || xs->sc_shutdown) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		biodone(bp);
		return;
	}

	dk_strategy(xs->sc_di, &xs->sc_dksc, bp);
	return;
}

int
xbdsize(dev_t dev)
{
	struct xbd_softc *xs = getxbd_softc(dev);

	DPRINTF_FOLLOW(("xbdsize(%d)\n", dev));
	if (xs == NULL || xs->sc_shutdown)
		return -1;
	return dk_size(xs->sc_di, &xs->sc_dksc, dev);
}

static void
map_align(struct xbdreq *xr)
{
	int s;

	s = splvm();
	xr->xr_aligned = uvm_km_alloc(kmem_map, xr->xr_bqueue, XEN_BSIZE,
	    UVM_KMF_WIRED);
	splx(s);
	DPRINTF(XBDB_IO, ("map_align(%p): bp %p addr %p align 0x%08lx "
	    "size 0x%04lx\n", xr, xr->xr_bp, xr->xr_bp->b_data,
	    xr->xr_aligned, xr->xr_bqueue));
	xr->xr_data = xr->xr_aligned;
	if ((xr->xr_bp->b_flags & B_READ) == 0)
		memcpy((void *)xr->xr_aligned, xr->xr_bp->b_data,
		    xr->xr_bqueue);
}

static void
unmap_align(struct xbdreq *xr)
{
	int s;

	if (xr->xr_bp->b_flags & B_READ)
		memcpy(xr->xr_bp->b_data, (void *)xr->xr_aligned,
		    xr->xr_bp->b_bcount);
	DPRINTF(XBDB_IO, ("unmap_align(%p): bp %p addr %p align 0x%08lx "
	    "size 0x%04x\n", xr, xr->xr_bp, xr->xr_bp->b_data,
	    xr->xr_aligned, xr->xr_bp->b_bcount));
	s = splvm();
	uvm_km_free(kmem_map, xr->xr_aligned, xr->xr_bp->b_bcount,
	    UVM_KMF_WIRED);
	splx(s);
	xr->xr_aligned = (vaddr_t)0;
}

static void
fill_ring(struct xbdreq *xr)
{
	struct xbdreq *pxr = xr->xr_parent;
	paddr_t pa;
	unsigned long ma;
	vaddr_t addr, off;
	blkif_request_t *ring_req;
	int breq, nr_sectors, fsect, lsect;

	/* Fill out a communications ring structure. */
	ring_req = &blk_ring->ring[MASK_BLKIF_IDX(req_prod)].req;
	ring_req->id = (unsigned long)xr;
	ring_req->operation = pxr->xr_bp->b_flags & B_READ ? BLKIF_OP_READ :
		BLKIF_OP_WRITE;
	ring_req->sector_number = pxr->xr_bn;
	ring_req->device = pxr->xr_sc->sc_xd_device;

	DPRINTF(XBDB_IO, ("fill_ring(%d): bp %p sector %llu pxr %p xr %p\n",
		    MASK_BLKIF_IDX(req_prod), pxr->xr_bp,
		    (unsigned long long)pxr->xr_bn,
		    pxr, xr));

	xr->xr_breq = 0;
	ring_req->nr_segments = 0;
	addr = trunc_page(pxr->xr_data);
	off = pxr->xr_data - addr;
	while (pxr->xr_bqueue > 0) {
#if 0
		pmap_extract(vm_map_pmap(&bp->b_proc->p_vmspace->vm_map),
		    addr, &pa);
#else
		pmap_extract(pmap_kernel(), addr, &pa);
#endif
		ma = xpmap_ptom_masked(pa);
		DIAGCONDPANIC((ma & (XEN_BSIZE - 1)) != 0,
		    ("xbd request ma not sector aligned"));

		if (pxr->xr_bqueue > PAGE_SIZE - off)
			breq = PAGE_SIZE - off;
		else
			breq = pxr->xr_bqueue;

		nr_sectors = breq >> XEN_BSHIFT;
		DIAGCONDPANIC(nr_sectors >= XEN_BSIZE,
		    ("xbd request nr_sectors >= XEN_BSIZE"));

		fsect = off >> XEN_BSHIFT;
		lsect = fsect + nr_sectors - 1;
		DIAGCONDPANIC(fsect > 7, ("xbd request fsect > 7"));
		DIAGCONDPANIC(lsect > 7, ("xbd request lsect > 7"));

		DPRINTF(XBDB_IO, ("fill_ring(%d): va 0x%08lx pa 0x%08lx "
		    "ma 0x%08lx, sectors %d, left %ld/%ld\n",
		    MASK_BLKIF_IDX(req_prod), addr, pa, ma, nr_sectors,
		    pxr->xr_bqueue >> XEN_BSHIFT, pxr->xr_bqueue));

		ring_req->frame_and_sects[ring_req->nr_segments++] =
			ma | (fsect<<3) | lsect;
		addr += PAGE_SIZE;
		pxr->xr_bqueue -= breq;
		pxr->xr_bn += nr_sectors;
		xr->xr_breq += breq;
		off = 0;
		if (ring_req->nr_segments == BLKIF_MAX_SEGMENTS_PER_REQUEST)
			break;
	}
	pxr->xr_data = addr;
	rec_ring_log(ring_req);

	req_prod++;
}

static void
xbdresume(void)
{
	struct xbdreq *pxr, *xr;
	struct xbd_softc *xs;
	struct buf *bp;

	while ((pxr = SIMPLEQ_FIRST(&xbdr_suspended)) != NULL) {
		DPRINTF(XBDB_IO, ("xbdstart: resuming xbdreq %p for bp %p\n",
		    pxr, pxr->xr_bp));
		bp = pxr->xr_bp;
		xs = getxbd_softc(bp->b_dev);
		if (xs == NULL || xs->sc_shutdown) {
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
		}
		if (bp->b_flags & B_ERROR) {
			pxr->xr_bdone -= pxr->xr_bqueue;
			pxr->xr_bqueue = 0;
			if (pxr->xr_bdone == 0) {
				bp->b_resid = bp->b_bcount;
				if (pxr->xr_aligned)
					unmap_align(pxr);
				PUT_XBDREQ(pxr);
				if (xs) {
					disk_unbusy(&xs->sc_dksc.sc_dkdev,
					    (bp->b_bcount - bp->b_resid),
					    (bp->b_flags & B_READ));
#if NRND > 0
					rnd_add_uint32(&xs->sc_rnd_source,
					    bp->b_blkno);
#endif
				}
				biodone(bp);
			}
			continue;
		}
		while (__predict_true(pxr->xr_bqueue > 0)) {
			GET_XBDREQ(xr);
			if (__predict_false(xr == NULL))
				goto out;
			xr->xr_parent = pxr;
			fill_ring(xr);
		}
		DPRINTF(XBDB_IO, ("xbdstart: resumed xbdreq %p for bp %p\n",
		    pxr, bp));
		SIMPLEQ_REMOVE_HEAD(&xbdr_suspended, xr_suspended);
	}

 out:
	return;
}

static int
xbdstart(struct dk_softc *dksc, struct buf *bp)
{
	struct	xbd_softc *xs;
	struct xbdreq *pxr, *xr;
	daddr_t	bn;
	int ret, runqueue;

	DPRINTF_FOLLOW(("xbdstart(%p, %p)\n", dksc, bp));

	runqueue = 1;
	ret = -1;

	xs = getxbd_softc(bp->b_dev);
	if (xs == NULL || xs->sc_shutdown) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		biodone(bp);
		return 0;
	}
	dksc = &xs->sc_dksc;

	bn = bp->b_rawblkno;

	DPRINTF(XBDB_IO, ("xbdstart: addr %p, sector %llu, "
	    "count %d [%s]\n", bp->b_data, (unsigned long long)bn,
	    bp->b_bcount, bp->b_flags & B_READ ? "read" : "write"));

	GET_XBDREQ(pxr);
	if (__predict_false(pxr == NULL))
		goto out;

	disk_busy(&dksc->sc_dkdev); /* XXX: put in dksubr.c */
	/*
	 * We have a request slot, return 0 to make dk_start remove
	 * the bp from the work queue.
	 */
	ret = 0;

	pxr->xr_bp = bp;
	pxr->xr_parent = pxr;
	pxr->xr_bn = bn;
	pxr->xr_bqueue = bp->b_bcount;
	pxr->xr_bdone = bp->b_bcount;
	pxr->xr_data = (vaddr_t)bp->b_data;
	pxr->xr_sc = xs;

	if (pxr->xr_data & (XEN_BSIZE - 1))
		map_align(pxr);

	fill_ring(pxr);

	while (__predict_false(pxr->xr_bqueue > 0)) {
		GET_XBDREQ(xr);
		if (__predict_false(xr == NULL))
			break;
		xr->xr_parent = pxr;
		fill_ring(xr);
	}

	if (__predict_false(pxr->xr_bqueue > 0)) {
		SIMPLEQ_INSERT_TAIL(&xbdr_suspended, pxr,
		    xr_suspended);
		DPRINTF(XBDB_IO, ("xbdstart: suspended xbdreq %p "
		    "for bp %p\n", pxr, bp));
	} else if (CANGET_XBDREQ() && BUFQ_PEEK(bufq) != NULL) {
		/* 
		 * We have enough resources to start another bp and
		 * there are additional bps on the queue, dk_start
		 * will call us again and we'll run the queue then.
		 */
		runqueue = 0;
	}

 out:
	if (runqueue && last_req_prod != req_prod)
		signal_requests_to_xen();

	return ret;
}

static int
xbd_response_handler(void *arg)
{
	struct buf *bp;
	struct xbd_softc *xs;
	blkif_response_t *ring_resp;
	struct xbdreq *pxr, *xr;
	BLKIF_RING_IDX i, rp;

	if (__predict_false(recovery) ||
	   __predict_false(state == STATE_CLOSED)) {
		printf ("xbd: dropping event due to recovery or closedness\n");
		return 0;
	}

	rp = blk_ring->resp_prod;
	x86_lfence(); /* Ensure we see queued responses up to 'rp'. */

	for (i = resp_cons; i != rp; i++) {
		ring_resp = &blk_ring->ring[MASK_BLKIF_IDX(i)].resp;
		xr = (struct xbdreq *)rec_ring_retire(ring_resp);

		switch (ring_resp->operation) {
		case BLKIF_OP_READ:
		case BLKIF_OP_WRITE:
			pxr = xr->xr_parent;

			DPRINTF(XBDB_IO, ("xbd_response_handler(%d): pxr %p "
				    "xr %p bdone %04lx breq %04lx\n", i, pxr,
				    xr, pxr->xr_bdone, xr->xr_breq));
			pxr->xr_bdone -= xr->xr_breq;
			DIAGCONDPANIC(pxr->xr_bdone < 0,
			    ("xbd_response_handler: pxr->xr_bdone < 0"));

			if (__predict_false(ring_resp->status)) {
				pxr->xr_bp->b_flags |= B_ERROR;
				pxr->xr_bp->b_error = EIO;
			}

			if (xr != pxr) {
				PUT_XBDREQ(xr);
				if (!SIMPLEQ_EMPTY(&xbdr_suspended))
					xbdresume();
			}

			if (pxr->xr_bdone == 0) {
				bp = pxr->xr_bp;
				xs = getxbd_softc(bp->b_dev);
				if (xs == NULL) { /* don't fail bp if we're shutdown */
					bp->b_flags |= B_ERROR;
					bp->b_error = EIO;
				}
				DPRINTF(XBDB_IO, ("xbd_response_handler(%d): "
					    "completed bp %p\n", i, bp));
				if (bp->b_flags & B_ERROR)
					bp->b_resid = bp->b_bcount;
				else
					bp->b_resid = 0;

				if (pxr->xr_aligned)
					unmap_align(pxr);

				PUT_XBDREQ(pxr);
				if (xs) {
					disk_unbusy(&xs->sc_dksc.sc_dkdev,
					    (bp->b_bcount - bp->b_resid),
					    (bp->b_flags & B_READ));
#if NRND > 0
					rnd_add_uint32(&xs->sc_rnd_source,
					    bp->b_blkno);
#endif
				}
				biodone(bp);
				if (!SIMPLEQ_EMPTY(&xbdr_suspended))
					xbdresume();
				/* XXX possible lockup if this was the only
				 * active device and requests were held back in
				 * the queue.
				 */
				if (xs)
					dk_iodone(xs->sc_di, &xs->sc_dksc);
			}
			break;
		case BLKIF_OP_PROBE:
			PUT_XBDREQ(xr);
			memcpy(&blkif_control_rsp, ring_resp,
			    sizeof(*ring_resp));
			blkif_control_rsp_valid = 1;
			wakeup((void *)&blkif_control_rsp_valid);
			break;
		default:
			panic("unknown response");
		}
	}
	resp_cons = i;
	/* check if xbdresume queued any requests */
	if (last_req_prod != req_prod)
		signal_requests_to_xen();
	return 0;
}

/* XXX: we should probably put these into dksubr.c, mostly */
int
xbdread(dev_t dev, struct uio *uio, int flags)
{
	struct	xbd_softc *xs;
	struct	dk_softc *dksc;

	DPRINTF_FOLLOW(("xbdread(%d, %p, %d)\n", dev, uio, flags));
	GETXBD_SOFTC_CDEV(xs, dev);
	dksc = &xs->sc_dksc;
	if ((dksc->sc_flags & DKF_INITED) == 0)
		return ENXIO;
	/* XXX see the comments about minphys in ccd.c */
	return physio(xbdstrategy, NULL, dev, B_READ, minphys, uio);
}

/* XXX: we should probably put these into dksubr.c, mostly */
int
xbdwrite(dev_t dev, struct uio *uio, int flags)
{
	struct	xbd_softc *xs;
	struct	dk_softc *dksc;

	DPRINTF_FOLLOW(("xbdwrite(%d, %p, %d)\n", dev, uio, flags));
	GETXBD_SOFTC_CDEV(xs, dev);
	dksc = &xs->sc_dksc;
	if ((dksc->sc_flags & DKF_INITED) == 0)
		return ENXIO;
	/* XXX see the comments about minphys in ccd.c */
	return physio(xbdstrategy, NULL, dev, B_WRITE, minphys, uio);
}

int
xbdioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct	xbd_softc *xs;
	struct	dk_softc *dksc;
	int	error;
	struct	disk *dk;

	DPRINTF_FOLLOW(("xbdioctl(%d, %08lx, %p, %d, %p)\n",
	    dev, cmd, data, flag, l));
	GETXBD_SOFTC(xs, dev);
	dksc = &xs->sc_dksc;
	dk = &dksc->sc_dkdev;

	KASSERT(bufq == dksc->sc_bufq);

	switch (cmd) {
	case DIOCSSTRATEGY:
		error = EOPNOTSUPP;
		break;
	default:
		error = dk_ioctl(xs->sc_di, dksc, dev, cmd, data, flag, l);
		break;
	}

	return error;
}

int
xbdioctl_cdev(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	dev_t bdev;

	bdev = devsw_chr2blk(dev);
	if (bdev == NODEV)
		return ENXIO;
	return xbdioctl(bdev, cmd, data, flag, l);
}

int
xbddump(dev_t dev, daddr_t blkno, void *va, size_t size)
{
	struct	xbd_softc *xs;

	DPRINTF_FOLLOW(("xbddump(%d, %" PRId64 ", %p, %lu)\n", dev, blkno, va,
	    (unsigned long)size));
	GETXBD_SOFTC(xs, dev);
	return dk_dump(xs->sc_di, &xs->sc_dksc, dev, blkno, va, size);
}

static int
xbdinit(struct xbd_softc *xs, vdisk_t *xd, struct dk_intf *dkintf)
{
	struct dk_geom *pdg;
	char buf[9];
	int ret;

	ret = 0;

	xs->sc_dksc.sc_size = xd->capacity;
	xs->sc_xd_device = xd->device;
	xs->sc_di = dkintf;
	xs->sc_shutdown = 0;

	/*
	 * XXX here we should probe the underlying device.  If we
	 *     are accessing a partition of type RAW_PART, then
	 *     we should populate our initial geometry with the
	 *     geometry that we discover from the device.
	 */
	pdg = &xs->sc_dksc.sc_geom;
	pdg->pdg_secsize = DEV_BSIZE;
	pdg->pdg_ntracks = 1;
	pdg->pdg_nsectors = 1024 * (1024 / pdg->pdg_secsize);
	pdg->pdg_ncylinders = xs->sc_dksc.sc_size / pdg->pdg_nsectors;

	/*
	 * We have one shared bufq for all devices because otherwise
	 * requests can stall if there were no free request slots
	 * available in xbdstart and this device had no requests
	 * in-flight which would trigger a dk_start from the interrupt
	 * handler.
	 * XXX we reference count the usage in case so we can de-alloc
	 *     the bufq if all devices are deconfigured.
	 */
	if (bufq_users == 0) {
		bufq_alloc(&bufq, "fcfs", 0);
		bufq_users = 1;
	}
	xs->sc_dksc.sc_bufq = bufq;

	xs->sc_dksc.sc_flags |= DKF_INITED;

	/* Attach the disk. */
	disk_attach(&xs->sc_dksc.sc_dkdev);

	/* Try and read the disklabel. */
	dk_getdisklabel(xs->sc_di, &xs->sc_dksc, 0 /* XXX ? */);

	format_bytes(buf, sizeof(buf), (uint64_t)xs->sc_dksc.sc_size *
	    pdg->pdg_secsize);
	printf(" %s\n", buf);

	/* Discover wedges on this disk. */
	dkwedge_discover(&xs->sc_dksc.sc_dkdev);

/*   out: */
	return ret;
}


void
xbd_suspend(void)
{

}

void
xbd_resume(void)
{

	send_driver_status(1);
}
