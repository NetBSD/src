/* $NetBSD: xbd.c,v 1.4 2004/04/24 19:32:37 cl Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: xbd.c,v 1.4 2004/04/24 19:32:37 cl Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/buf.h>
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

#include <uvm/uvm.h>

#include <dev/dkvar.h>
#include <machine/xbdvar.h>
#include <machine/xen.h>
#include <machine/hypervisor.h>
#include <machine/hypervisor-ifs/hypervisor-if.h>
#include <machine/hypervisor-ifs/vbd.h>
#include <machine/events.h>


int xbd_match(struct device *, struct cfdata *, void *);
void xbd_attach(struct device *, struct device *, void *);

CFATTACH_DECL(xbd, sizeof(struct xbd_softc),
    xbd_match, xbd_attach, NULL, NULL);

extern struct cfdriver xbd_cd;

dev_type_open(xbdopen);
dev_type_close(xbdclose);
dev_type_read(xbdread);
dev_type_write(xbdwrite);
dev_type_ioctl(xbdioctl);
dev_type_strategy(xbdstrategy);
dev_type_dump(xbddump);
dev_type_size(xbdsize);

const struct bdevsw xbd_bdevsw = {
	xbdopen, xbdclose, xbdstrategy, xbdioctl,
	xbddump, xbdsize, D_DISK
};

const struct cdevsw xbd_cdevsw = {
	xbdopen, xbdclose, xbdread, xbdwrite, xbdioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};


static int	xbdstart(struct dk_softc *, struct buf *);
static int	xbd_response_handler(void *);

static int	xbdinit(struct xbd_softc *, xen_disk_t *);

/* Pseudo-disk Interface */
static struct dk_intf dkintf_esdi = {
	DTYPE_ESDI,
	"Xen Virtual ESDI",
	xbdopen,
	xbdclose,
	xbdstrategy,
	xbdstart,
};
static struct dk_intf dkintf_scsi = {
	DTYPE_SCSI,
	"Xen Virtual SCSI",
	xbdopen,
	xbdclose,
	xbdstrategy,
	xbdstart,
};


#if defined(XBDDEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#ifdef DEBUG
int xbddebug = 0;

#define XBDB_FOLLOW	0x1
#define XBDB_IO		0x2
#define XBDB_SETUP	0x4

#define IFDEBUG(x,y)		if (xbddebug & (x)) y
#define DPRINTF(x,y)		IFDEBUG(x, printf y)
#define DPRINTF_FOLLOW(y)	DPRINTF(XBDB_FOLLOW, y)
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

static struct bufq_state bufq;
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
static xen_disk_t *vbd_info;

static blk_ring_t *blk_ring = NULL;
static BLK_RING_IDX resp_cons; /* Response consumer for comms ring. */
static BLK_RING_IDX req_prod;  /* Private request producer.         */

#define STATE_ACTIVE    0
#define STATE_SUSPENDED 1
#define STATE_CLOSED    2
static unsigned int state = STATE_SUSPENDED;


#define XBDUNIT(x)		DISKUNIT(x)
#define GETXBD_SOFTC(_xs, x)	if (!((_xs) = getxbd_softc(x))) return ENXIO

static struct xbd_softc *
getxbd_softc(dev_t dev)
{
	int	unit = XBDUNIT(dev);

	DPRINTF_FOLLOW(("getxbd_softc(0x%x): unit = %d\n", dev, unit));
	return device_lookup(&xbd_cd, unit);
}

static int
get_vbd_info(xen_disk_t *disk_info)
{
	int err;
	block_io_op_t op; 

	/* Probe for disk information. */
	memset(&op, 0, sizeof(op)); 
	op.cmd = BLOCK_IO_OP_VBD_PROBE; 
	op.u.probe_params.domain = 0; 
	op.u.probe_params.xdi.max = MAX_VBDS;
	op.u.probe_params.xdi.disks = disk_info;
	op.u.probe_params.xdi.count = 0;

	err = HYPERVISOR_block_io_op(&op);
	if (err) {
		printf("WARNING: Could not probe disks (%d)\n", err);
		DIAGPANIC(("get_vbd_info: Could not probe disks (%d)", err));
		return -1;
	}

	return op.u.probe_params.xdi.count;
}

static void
reset_interface(void)
{
	block_io_op_t op; 

	op.cmd = BLOCK_IO_OP_RESET;
	if (HYPERVISOR_block_io_op(&op) != 0)
		printf("xbd: Possible blkdev trouble: couldn't reset ring\n");
}

static void
init_interface(void)
{
	block_io_op_t op; 

	reset_interface();

	if (blk_ring == NULL) {
		op.cmd = BLOCK_IO_OP_RING_ADDRESS;
		(void)HYPERVISOR_block_io_op(&op);

		blk_ring = (blk_ring_t *)uvm_km_valloc_align(kernel_map,
		    PAGE_SIZE, PAGE_SIZE);
		pmap_kenter_ma((vaddr_t)blk_ring, op.u.ring_mfn << PAGE_SHIFT,
		    VM_PROT_READ|VM_PROT_WRITE);
		DPRINTF(XBDB_SETUP, ("init_interface: "
		    "ring va %p and wired to %p\n",
		    blk_ring, (void *)(op.u.ring_mfn << PAGE_SHIFT)));

		blk_ring->req_prod = blk_ring->resp_prod =
			resp_cons = req_prod = 0;

		event_set_handler(_EVENT_BLKDEV, &xbd_response_handler,
		    NULL, IPL_BIO);
		hypervisor_enable_event(_EVENT_BLKDEV);
#if 0
		event_set_handler(_EVENT_VBD_UPD, &xbd_update_handler,
		    NULL, IPL_BIO);
		hypervisor_enable_event(_EVENT_VBD_UPD);
#endif
	}

	__insn_barrier();
	state = STATE_ACTIVE;
}

static void
signal_requests_to_xen(void)
{
	block_io_op_t op; 

	DPRINTF(XBDB_IO, ("signal_requests_to_xen: %d -> %d\n",
	    blk_ring->req_prod, req_prod));
	blk_ring->req_prod = req_prod;

	op.cmd = BLOCK_IO_OP_SIGNAL; 
	HYPERVISOR_block_io_op(&op);
	return;
}

int
xbd_scan(struct device *self, struct xbd_attach_args *xbda, cfprint_t print)
{
	struct xbdreq *xr;
	int i;

	init_interface();

	MALLOC(xr, struct xbdreq *, BLK_RING_SIZE * sizeof(struct xbdreq),
	    M_DEVBUF, M_WAITOK | M_ZERO);
#ifdef DEBUG
	xbd_allxr = xr;
#endif

	for (i = 0; i < BLK_RING_SIZE; i++)
		PUT_XBDREQ(&xr[i]);

	MALLOC(vbd_info, xen_disk_t *, MAX_VBDS * sizeof(xen_disk_t),
	    M_DEVBUF, M_WAITOK);
	nr_vbds  = get_vbd_info(vbd_info);
	if (nr_vbds <= 0)
		goto out;

	for (i = 0; i < nr_vbds; i++) {
		xbda->xa_disk = i;
		config_found(self, xbda, print);
	}

	return 0;

 out:
	FREE(vbd_info, M_DEVBUF);
	vbd_info = NULL;
	FREE(xr, M_DEVBUF);
#ifdef DEBUG
	xbd_allxr = NULL;
#endif
	SLIST_INIT(&xbdreqs);
	return 0;
}

int
xbd_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct xbd_attach_args *xa = (struct xbd_attach_args *)aux;

	if (strcmp(xa->xa_device, "xbd") == 0)
		return 1;
	return 0;
}

void
xbd_attach(struct device *parent, struct device *self, void *aux)
{
	struct xbd_attach_args *xbda = (struct xbd_attach_args *)aux;
	struct xbd_softc *xs = (struct xbd_softc *)self;

	aprint_normal(": Xen Virtual Block Device");

	simple_lock_init(&xs->sc_slock);
	dk_sc_init(&xs->sc_dksc, xs, xs->sc_dev.dv_xname);
	xbdinit(xs, &vbd_info[xbda->xa_disk]);
}

int
xbdopen(dev_t dev, int flags, int fmt, struct proc *p)
{
	struct	xbd_softc *xs;

	DPRINTF_FOLLOW(("xbdopen(0x%04x, %d)\n", dev, flags));
	GETXBD_SOFTC(xs, dev);
	return dk_open(xs->sc_di, &xs->sc_dksc, dev, flags, fmt, p);
}

int
xbdclose(dev_t dev, int flags, int fmt, struct proc *p)
{
	struct	xbd_softc *xs;

	DPRINTF_FOLLOW(("xbdclose(%d, %d)\n", dev, flags));
	GETXBD_SOFTC(xs, dev);
	return dk_close(xs->sc_di, &xs->sc_dksc, dev, flags, fmt, p);
}

void
xbdstrategy(struct buf *bp)
{
	struct	xbd_softc *xs = getxbd_softc(bp->b_dev);

	DPRINTF_FOLLOW(("xbdstrategy(%p): b_bcount = %ld\n", bp,
	    (long)bp->b_bcount));
	/* XXXrcd: Should we test for (xs != NULL)? */
	dk_strategy(xs->sc_di, &xs->sc_dksc, bp);
	return;
}

int
xbdsize(dev_t dev)
{
	struct xbd_softc *xs = getxbd_softc(dev);

	DPRINTF_FOLLOW(("xbdsize(%d)\n", dev));
	if (!xs)
		return -1;
	return dk_size(xs->sc_di, &xs->sc_dksc, dev);
}

static void
map_align(struct xbdreq *xr)
{
	int s;

	s = splvm();
	xr->xr_aligned = uvm_km_kmemalloc1(kmem_map, NULL,
	    xr->xr_bqueue, XEN_BSIZE, UVM_UNKNOWN_OFFSET,
	    0/*  UVM_KMF_NOWAIT */);
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
	    "size 0x%04lx\n", xr, xr->xr_bp, xr->xr_bp->b_data,
	    xr->xr_aligned, xr->xr_bp->b_bcount));
	s = splvm();
	uvm_km_free(kmem_map, xr->xr_aligned, xr->xr_bp->b_bcount);
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
	blk_ring_req_entry_t *ring_req;
	int breq, nr_sectors;

	/* Fill out a communications ring structure. */
	ring_req = &blk_ring->ring[MASK_BLK_IDX(req_prod)].req;
	ring_req->id = (unsigned long)xr;
	ring_req->operation = pxr->xr_bp->b_flags & B_READ ? XEN_BLOCK_READ :
		XEN_BLOCK_WRITE;
	ring_req->sector_number = (xen_sector_t)pxr->xr_bn;
	ring_req->device = pxr->xr_sc->sc_xd_device;

	DPRINTF(XBDB_IO, ("fill_ring(%d): bp %p sector %llu pxr %p xr %p\n",
	    MASK_BLK_IDX(req_prod), pxr->xr_bp, (unsigned long long)pxr->xr_bn,
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
		ma = (xpmap_phys_to_machine_mapping[pa >> PAGE_SHIFT] <<
		    PAGE_SHIFT) + off;
		DIAGCONDPANIC((ma & (XEN_BSIZE - 1)) != 0,
		    ("xbd request ma not sector aligned"));

		if (pxr->xr_bqueue > PAGE_SIZE - off)
			breq = PAGE_SIZE - off;
		else
			breq = pxr->xr_bqueue;
		nr_sectors = breq >> XEN_BSHIFT;
		DIAGCONDPANIC(nr_sectors >= XEN_BSIZE,
		    ("xbd request nr_sectors >= XEN_BSIZE"));

		DPRINTF(XBDB_IO, ("fill_ring(%d): va 0x%08lx pa 0x%08lx "
		    "ma 0x%08lx, sectors %d, left %ld/%ld\n",
		    MASK_BLK_IDX(req_prod), addr, pa, ma, nr_sectors,
		    pxr->xr_bqueue >> XEN_BSHIFT, pxr->xr_bqueue));

		ring_req->buffer_and_sects[ring_req->nr_segments++] =
			ma | nr_sectors;
		addr += PAGE_SIZE;
		pxr->xr_bqueue -= breq;
		pxr->xr_bn += nr_sectors;
		xr->xr_breq += breq;
		off = 0;
		if (ring_req->nr_segments == MAX_BLK_SEGS)
			break;
	}
	pxr->xr_data = addr;

	req_prod = BLK_RING_INC(req_prod);
}

static void
xbdresume(void)
{
	struct xbdreq *pxr, *xr;

	while ((pxr = SIMPLEQ_FIRST(&xbdr_suspended)) != NULL) {
		DPRINTF(XBDB_IO, ("xbdstart: resuming xbdreq %p for bp %p\n",
		    pxr, pxr->xr_bp));
		/* XXXcl: Should check if a previous transfer failed */
		while (__predict_true(pxr->xr_bqueue > 0)) {
			GET_XBDREQ(xr);
			if (__predict_false(xr == NULL))
				goto out;
			xr->xr_parent = pxr;
			fill_ring(xr);
		}
		DPRINTF(XBDB_IO, ("xbdstart: resumed xbdreq %p for bp %p\n",
		    pxr, pxr->xr_bp));
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
	struct	partition *pp;
	daddr_t	bn;
	int ret, runqueue;

	DPRINTF_FOLLOW(("xbdstart(%p, %p)\n", dksc, bp));

	runqueue = 1;
	ret = -1;

	GETXBD_SOFTC(xs, bp->b_dev);
	dksc = &xs->sc_dksc;
	/* XXXrcd:
	 * Translate partition relative blocks to absolute blocks,
	 * this probably belongs (somehow) in dksubr.c, since it
	 * is independant of the underlying code...  This will require
	 * that the interface be expanded slightly, though.
	 */
	bn = bp->b_blkno;
	if (DISKPART(bp->b_dev) != RAW_PART) {
		pp = &xs->sc_dksc.sc_dkdev.dk_label->
			d_partitions[DISKPART(bp->b_dev)];
		bn += pp->p_offset;
	}

	DPRINTF(XBDB_IO, ("xbdstart: addr %p, sector %llu, "
	    "count %ld [%s]\n", bp->b_data, (unsigned long long)bn,
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
	} else if (CANGET_XBDREQ() && BUFQ_PEEK(&bufq) != NULL) {
		/* 
		 * We have enough resources to start another bp and
		 * there are additional bps on the queue, dk_start
		 * will call us again and we'll run the queue then.
		 */
		runqueue = 0;
	}

 out:
	if (runqueue && blk_ring->req_prod != req_prod)
		signal_requests_to_xen();

	return ret;
}

static int
xbd_response_handler(void *arg)
{
	struct buf *bp;
	struct xbd_softc *xs;
	blk_ring_resp_entry_t *ring_resp;
	struct xbdreq *pxr, *xr;
	int i, s;

	s = splbio();
	if (s != IPL_BIO)
		printf("xbd_response_handler: called at %d != IPL_BIO\n", s);
	for (i = resp_cons; i != blk_ring->resp_prod; i = BLK_RING_INC(i)) {
		ring_resp = &blk_ring->ring[MASK_BLK_IDX(i)].resp;
		xr = (struct xbdreq *)ring_resp->id;
		pxr = xr->xr_parent;

		DPRINTF(XBDB_IO, ("xbd_response_handler(%d): pxr %p xr %p "
		    "bdone %04lx breq %04lx\n", i, pxr, xr, pxr->xr_bdone,
		    xr->xr_breq));
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
			DPRINTF(XBDB_IO, ("xbd_response_handler(%d): "
			    "completed bp %p\n", i, bp));
			if (bp->b_flags & B_ERROR)
				bp->b_resid = bp->b_bcount;
			else
				bp->b_resid = 0;

			if (pxr->xr_aligned)
				unmap_align(pxr);

			PUT_XBDREQ(pxr);
			disk_unbusy(&xs->sc_dksc.sc_dkdev,
			    (bp->b_bcount - bp->b_resid),
			    (bp->b_flags & B_READ));
			biodone(bp);
			if (!SIMPLEQ_EMPTY(&xbdr_suspended))
				xbdresume();
			dk_iodone(xs->sc_di, &xs->sc_dksc);
		}
	}
	resp_cons = i;
	/* check if xbdresume queued any requests */
	if (blk_ring->req_prod != req_prod)
		signal_requests_to_xen();
	splx(s);
	return 0;
}

/* XXX: we should probably put these into dksubr.c, mostly */
int
xbdread(dev_t dev, struct uio *uio, int flags)
{
	struct	xbd_softc *xs;
	struct	dk_softc *dksc;

	DPRINTF_FOLLOW(("xbdread(%d, %p, %d)\n", dev, uio, flags));
	GETXBD_SOFTC(xs, dev);
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
	GETXBD_SOFTC(xs, dev);
	dksc = &xs->sc_dksc;
	if ((dksc->sc_flags & DKF_INITED) == 0)
		return ENXIO;
	/* XXX see the comments about minphys in ccd.c */
	return physio(xbdstrategy, NULL, dev, B_WRITE, minphys, uio);
}

int
xbdioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct	xbd_softc *xs;
	struct	dk_softc *dksc;
	int	ret;

	DPRINTF_FOLLOW(("xbdioctl(%d, %08lx, %p, %d, %p)\n",
	    dev, cmd, data, flag, p));
	GETXBD_SOFTC(xs, dev);
	dksc = &xs->sc_dksc;

	if ((ret = lockmgr(&dksc->sc_lock, LK_EXCLUSIVE, NULL)) != 0)
		return ret;

	switch (cmd) {
	default:
		ret = dk_ioctl(xs->sc_di, dksc, dev, cmd, data, flag, p);
		break;
	}

	lockmgr(&dksc->sc_lock, LK_RELEASE, NULL);
	return ret;
}

int
xbddump(dev_t dev, daddr_t blkno, caddr_t va, size_t size)
{
	struct	xbd_softc *xs;

	DPRINTF_FOLLOW(("xbddump(%d, %" PRId64 ", %p, %lu)\n", dev, blkno, va,
	    (unsigned long)size));
	GETXBD_SOFTC(xs, dev);
	return dk_dump(xs->sc_di, &xs->sc_dksc, dev, blkno, va, size);
}

static int
xbdinit(struct xbd_softc *xs, xen_disk_t *xd)
{
	struct dk_geom *pdg;
	char buf[9];
	int ret;

	ret = 0;

	xs->sc_dksc.sc_size = xd->capacity;
	xs->sc_xd_device = xd->device;

	switch (XEN_MAJOR(xd->device)) {
	case XEN_SCSI_DISK0_MAJOR:
	case XEN_SCSI_DISK1_MAJOR ... XEN_SCSI_DISK7_MAJOR:
	case XEN_SCSI_DISK8_MAJOR ... XEN_SCSI_DISK15_MAJOR:
	case XEN_SCSI_CDROM_MAJOR:
		xs->sc_di = &dkintf_scsi;
		break;
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
	default:
		xs->sc_di = &dkintf_esdi;
		break;
	}

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
	 * XXX this assumes that we can just memcpy struct bufq_state
	 *     to share it between devices.
	 * XXX we reference count the usage in case so we can de-alloc
	 *     the bufq if all devices are deconfigured.
	 */
	if (bufq_users == 0)
		bufq_alloc(&bufq, BUFQ_FCFS);
	bufq_users++;
	memcpy(&xs->sc_dksc.sc_bufq, &bufq, sizeof(struct bufq_state));

	xs->sc_dksc.sc_flags |= DKF_INITED;

	/* Attach the disk. */
	disk_attach(&xs->sc_dksc.sc_dkdev);

	/* Try and read the disklabel. */
	dk_getdisklabel(xs->sc_di, &xs->sc_dksc, 0 /* XXX ? */);

	format_bytes(buf, sizeof(buf), (uint64_t)xs->sc_dksc.sc_size *
	    pdg->pdg_secsize);
	printf(" %s\n", buf);

/*   out: */
	return ret;
}
