/*	$NetBSD: ld_ataraid.c,v 1.2 2003/01/27 20:18:11 thorpej Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Support for ATA RAID logical disks.
 *
 * Note that all the RAID happens in software here; the ATA RAID
 * controllers we're dealing with (Promise, etc.) only support
 * configuration data on the component disks, with the BIOS supporting
 * booting from the RAID volumes.
 */

#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/dkio.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <miscfs/specfs/specdev.h>

#include <dev/ldvar.h>

#include <dev/ata/ata_raidvar.h>

struct ld_ataraid_softc {
	struct ld_softc sc_ld;

	struct ataraid_array_info *sc_aai;
	struct vnode *sc_vnodes[ATA_RAID_MAX_DISKS];

	void	(*sc_iodone)(struct buf *);
};

static int	ld_ataraid_match(struct device *, struct cfdata *, void *);
static void	ld_ataraid_attach(struct device *, struct device *, void *);

static int	ld_ataraid_dump(struct ld_softc *, void *, int, int);

static int	ld_ataraid_start_span(struct ld_softc *, struct buf *);

static int	ld_ataraid_start_raid0(struct ld_softc *, struct buf *);
static void	ld_ataraid_iodone_raid0(struct buf *);

CFATTACH_DECL(ld_ataraid, sizeof(struct ld_ataraid_softc),
    ld_ataraid_match, ld_ataraid_attach, NULL, NULL);

static int ld_ataraid_initialized;
static struct pool ld_ataraid_cbufpl;

struct cbuf {
	struct buf	cb_buf;		/* new I/O buf */
	struct buf	*cb_obp;	/* ptr. to original I/O buf */
	struct ld_ataraid_softc *cb_sc;	/* pointer to ld softc */
	u_int		cb_comp;	/* target component */
	SIMPLEQ_ENTRY(cbuf) cb_q;	/* fifo of component buffers */
};

#define	CBUF_GET()	pool_get(&ld_ataraid_cbufpl, PR_NOWAIT);
#define	CBUF_PUT(cbp)	pool_put(&ld_ataraid_cbufpl, (cbp))

static int
ld_ataraid_match(struct device *parent, struct cfdata *match, void *aux)
{

	return (1);
}

static void
ld_ataraid_attach(struct device *parent, struct device *self, void *aux)
{
	struct ld_ataraid_softc *sc = (void *) self;
	struct ld_softc *ld = &sc->sc_ld;
	struct ataraid_array_info *aai = aux;
	const char *level;
	struct vnode *vp;
	char unklev[32];
	u_int i;

	if (ld_ataraid_initialized == 0) {
		ld_ataraid_initialized = 1;
		pool_init(&ld_ataraid_cbufpl, sizeof(struct cbuf), 0,
		    0, 0, "ldcbuf", NULL);
	}

	sc->sc_aai = aai;	/* this data persists */

	ld->sc_flags = LDF_ENABLED;
	ld->sc_maxxfer = MAXPHYS * aai->aai_width;	/* XXX */
	ld->sc_secperunit = aai->aai_capacity;
	ld->sc_secsize = 512;				/* XXX */
	ld->sc_maxqueuecnt = 128;			/* XXX */
	ld->sc_dump = ld_ataraid_dump;

	switch (aai->aai_level) {
	case AAI_L_SPAN:
		level = "SPAN";
		ld->sc_start = ld_ataraid_start_span;
		sc->sc_iodone = ld_ataraid_iodone_raid0;
		break;

	case AAI_L_RAID0:
		level = "RAID0";
		ld->sc_start = ld_ataraid_start_raid0;
		sc->sc_iodone = ld_ataraid_iodone_raid0;
		break;

	case AAI_L_RAID1:
		level = "RAID1";
		break;

	case AAI_L_RAID0 | AAI_L_RAID1:
		level = "RAID0+1";
		break;

	default:
		sprintf(unklev, "<unknown level 0x%x>", aai->aai_level);
		level = unklev;
	}

	aprint_naive(": ATA %s array\n", level);
	aprint_normal(": %s ATA %s array\n",
	    ata_raid_type_name(aai->aai_type), level);

	if (ld->sc_start == NULL) {
		aprint_error("%s: unsupported array type\n",
		    ld->sc_dv.dv_xname);
		return;
	}

	/*
	 * We get a geometry from the device; use it.
	 */
	ld->sc_nheads = aai->aai_heads;
	ld->sc_nsectors = aai->aai_sectors;
	ld->sc_ncylinders = aai->aai_cylinders;

	/*
	 * Configure all the component disks.
	 */
	for (i = 0; i < aai->aai_ndisks; i++) {
		struct ataraid_disk_info *adi = &aai->aai_disks[i];
		int bmajor, error;
		dev_t dev;

		bmajor = devsw_name2blk(adi->adi_dev->dv_xname, NULL, 0);
		dev = MAKEDISKDEV(bmajor, adi->adi_dev->dv_unit, RAW_PART);
		error = bdevvp(dev, &vp);
		if (error)
			goto bad;
		error = VOP_OPEN(vp, FREAD|FWRITE, NOCRED, 0);
		if (error) {
			vput(vp);
			/*
			 * XXX This is bogus.  We should just mark the
			 * XXX component as FAILED, and write-back new
			 * XXX config blocks.
			 */
			goto bad;
		}

		VOP_UNLOCK(vp, 0);
		sc->sc_vnodes[i] = vp;
	}

	ldattach(ld);
	return;
 bad:
	for (i = 0; i < aai->aai_ndisks; i++) {
		vp = sc->sc_vnodes[i];
		sc->sc_vnodes[i] = NULL;
		(void) vn_close(vp, FREAD|FWRITE, NOCRED, curproc);
	}
}

static struct cbuf *
ld_ataraid_make_cbuf(struct ld_ataraid_softc *sc, struct buf *bp,
    u_int comp, daddr_t bn, caddr_t addr, long bcount)
{
	struct cbuf *cbp;

	cbp = CBUF_GET();
	if (cbp == NULL)
		return (NULL);
	cbp->cb_buf.b_flags = bp->b_flags | B_CALL;
	cbp->cb_buf.b_iodone = sc->sc_iodone;
	cbp->cb_buf.b_proc = bp->b_proc;
	cbp->cb_buf.b_vp = sc->sc_vnodes[comp];
	cbp->cb_buf.b_dev = sc->sc_vnodes[comp]->v_rdev;
	cbp->cb_buf.b_blkno = bn + sc->sc_aai->aai_offset;
	cbp->cb_buf.b_data = addr;
	LIST_INIT(&cbp->cb_buf.b_dep);
	cbp->cb_buf.b_bcount = bcount;

	/* Context for iodone */
	cbp->cb_obp = bp;
	cbp->cb_sc = sc;
	cbp->cb_comp = comp;

	return (cbp);
}

static int
ld_ataraid_start_span(struct ld_softc *ld, struct buf *bp)
{
	struct ld_ataraid_softc *sc = (void *) ld;
	struct ataraid_array_info *aai = sc->sc_aai;
	struct ataraid_disk_info *adi;
	SIMPLEQ_HEAD(, cbuf) cbufq;
	struct cbuf *cbp;
	caddr_t addr;
	daddr_t bn;
	long bcount, rcount;
	u_int comp;
	int s;

	/* Allocate component buffers. */
	SIMPLEQ_INIT(&cbufq);
	addr = bp->b_data;

	/* Find the first component. */
	comp = 0;
	adi = &aai->aai_disks[comp];
	bn = bp->b_rawblkno;
	while (bn >= adi->adi_compsize) {
		bn -= adi->adi_compsize;
		adi = &aai->aai_disks[++comp];
	}

	s = splbio();		/* XXX big hammer */

	bp->b_resid = bp->b_bcount;

	for (bcount = bp->b_bcount; bcount > 0; bcount -= rcount) {
		rcount = bp->b_bcount;
		if ((adi->adi_compsize - bn) < btodb(rcount))
			rcount = dbtob(adi->adi_compsize - bn);

		cbp = ld_ataraid_make_cbuf(sc, bp, comp, bn, addr, rcount);
		if (cbp == NULL) {
			/* Free the already allocated component buffers. */
			while ((cbp = SIMPLEQ_FIRST(&cbufq)) != NULL) {
				SIMPLEQ_REMOVE_HEAD(&cbufq, cb_q);
				CBUF_PUT(cbp);
			}

			splx(s);

			/* Notify the upper layer that we are out of memory. */
			return (ENOMEM);
		}

		/*
		 * For a span, we always know we advance to the next disk,
		 * and always start at offset 0 on that disk.
		 */
		adi = &aai->aai_disks[++comp];
		bn = 0;

		SIMPLEQ_INSERT_TAIL(&cbufq, cbp, cb_q);
		addr += rcount;
	}

	/* Now fire off the requests. */
	while ((cbp = SIMPLEQ_FIRST(&cbufq)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&cbufq, cb_q);
		if ((cbp->cb_buf.b_flags & B_READ) == 0)
			cbp->cb_buf.b_vp->v_numoutput++;
		VOP_STRATEGY(&cbp->cb_buf);
	}

	splx(s);

	return (0);
}

static int
ld_ataraid_start_raid0(struct ld_softc *ld, struct buf *bp)
{
	struct ld_ataraid_softc *sc = (void *) ld;
	struct ataraid_array_info *aai = sc->sc_aai;
	SIMPLEQ_HEAD(, cbuf) cbufq;
	struct cbuf *cbp;
	caddr_t addr;
	daddr_t bn, cbn, tbn, off;
	long bcount, rcount;
	u_int comp;
	int s;

	/* Allocate component buffers. */
	SIMPLEQ_INIT(&cbufq);
	addr = bp->b_data;
	bn = bp->b_rawblkno;

	s = splbio();		/* XXX big hammer */

	bp->b_resid = bp->b_bcount;

	for (bcount = bp->b_bcount; bcount > 0; bcount -= rcount) {
		tbn = bn / aai->aai_interleave;
		off = bn % aai->aai_interleave;

		if (__predict_false(tbn == aai->aai_capacity /
					   aai->aai_interleave)) {
			/* Last stripe. */
			daddr_t sz = (aai->aai_capacity -
				      (tbn * aai->aai_interleave)) /
				     aai->aai_width;
			comp = off / sz;
			cbn = ((tbn / aai->aai_width) * aai->aai_interleave) +
			    (off % sz);
			rcount = min(bcount, dbtob(sz));
		} else {
			comp = tbn % aai->aai_width;
			cbn = ((tbn / aai->aai_width) * aai->aai_interleave) +
			    off;
			rcount = min(bcount, dbtob(aai->aai_interleave - off));
		}

		cbp = ld_ataraid_make_cbuf(sc, bp, comp, cbn, addr, rcount);
		if (cbp == NULL) {
			/* Free the already allocated component buffers. */
			while ((cbp = SIMPLEQ_FIRST(&cbufq)) != NULL) {
				SIMPLEQ_REMOVE_HEAD(&cbufq, cb_q);
				CBUF_PUT(cbp);
			}

			splx(s);

			/* Notify the upper layer that we are out of memory. */
			return (ENOMEM);
		}
		SIMPLEQ_INSERT_TAIL(&cbufq, cbp, cb_q);
		bn += btodb(rcount);
		addr += rcount;
	}

	/* Now fire off the requests. */
	while ((cbp = SIMPLEQ_FIRST(&cbufq)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&cbufq, cb_q);
		if ((cbp->cb_buf.b_flags & B_READ) == 0)
			cbp->cb_buf.b_vp->v_numoutput++;
		VOP_STRATEGY(&cbp->cb_buf);
	}

	splx(s);

	return (0);
}

/*
 * Called at interrupt time.  Mark the component as done and if all
 * components are done, take an "interrupt".
 */
static void
ld_ataraid_iodone_raid0(struct buf *vbp)
{
	struct cbuf *cbp = (struct cbuf *) vbp;
	struct buf *bp = cbp->cb_obp;
	struct ld_ataraid_softc *sc = cbp->cb_sc;
	long count;
	int s;

	s = splbio();

	if (cbp->cb_buf.b_flags & B_ERROR) {
		bp->b_flags |= B_ERROR;
		bp->b_error = cbp->cb_buf.b_error ?
		    cbp->cb_buf.b_error : EIO;

		/* XXX Update component config blocks. */

		printf("%s: error %d on component %d\n",
		    sc->sc_ld.sc_dv.dv_xname, bp->b_error, cbp->cb_comp);
	}
	count = cbp->cb_buf.b_bcount;
	CBUF_PUT(cbp);

	/* If all done, "interrupt". */
	bp->b_resid -= count;
	if (bp->b_resid < 0)
		panic("ld_ataraid_iodone_raid0: count");
	if (bp->b_resid == 0)
		lddone(&sc->sc_ld, bp);
	splx(s);
}

static int
ld_ataraid_dump(struct ld_softc *sc, void *data, int blkno, int blkcnt)
{

	return (EIO);
}
