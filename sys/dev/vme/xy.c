/*	$NetBSD: xy.c,v 1.25.2.3 2001/05/01 12:27:49 he Exp $	*/

/*
 *
 * Copyright (c) 1995 Charles D. Cranor
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
 *      This product includes software developed by Charles D. Cranor.
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

/*
 *
 * x y . c   x y l o g i c s   4 5 0 / 4 5 1   s m d   d r i v e r
 *
 * author: Chuck Cranor <chuck@ccrc.wustl.edu>
 * started: 14-Sep-95
 * references: [1] Xylogics Model 753 User's Manual
 *                 part number: 166-753-001, Revision B, May 21, 1988.
 *                 "Your Partner For Performance"
 *             [2] other NetBSD disk device drivers
 *	       [3] Xylogics Model 450 User's Manual
 *		   part number: 166-017-001, Revision B, 1983.
 *	       [4] Addendum to Xylogics Model 450 Disk Controller User's
 *			Manual, Jan. 1985.
 *	       [5] The 451 Controller, Rev. B3, September 2, 1986.
 *	       [6] David Jones <dej@achilles.net>'s unfinished 450/451 driver
 *
 */

#undef XYC_DEBUG		/* full debug */
#undef XYC_DIAG			/* extra sanity checks */
#if defined(DIAGNOSTIC) && !defined(XYC_DIAG)
#define XYC_DIAG		/* link in with master DIAG option */
#endif

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/dkbad.h>
#include <sys/conf.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#if defined(__sparc__) || defined(sun3)
#include <dev/sun/disklabel.h>
#endif

#include <dev/vme/vmereg.h>
#include <dev/vme/vmevar.h>

#include <dev/vme/xyreg.h>
#include <dev/vme/xyvar.h>
#include <dev/vme/xio.h>

#include "locators.h"

/*
 * macros
 */

/*
 * XYC_GO: start iopb ADDR (DVMA addr in a u_long) on XYC
 */
#define XYC_GO(XYC, ADDR) { \
	(XYC)->xyc_addr_lo = ((ADDR) & 0xff); \
	(ADDR) = ((ADDR) >> 8); \
	(XYC)->xyc_addr_hi = ((ADDR) & 0xff); \
	(ADDR) = ((ADDR) >> 8); \
	(XYC)->xyc_reloc_lo = ((ADDR) & 0xff); \
	(ADDR) = ((ADDR) >> 8); \
	(XYC)->xyc_reloc_hi = (ADDR); \
	(XYC)->xyc_csr = XYC_GBSY; /* go! */ \
}

/*
 * XYC_DONE: don't need IORQ, get error code and free (done after xyc_cmd)
 */

#define XYC_DONE(SC,ER) { \
	if ((ER) == XY_ERR_AOK) { \
		(ER) = (SC)->ciorq->errno; \
		(SC)->ciorq->mode = XY_SUB_FREE; \
		wakeup((SC)->ciorq); \
	} \
	}

/*
 * XYC_ADVANCE: advance iorq's pointers by a number of sectors
 */

#define XYC_ADVANCE(IORQ, N) { \
	if (N) { \
		(IORQ)->sectcnt -= (N); \
		(IORQ)->blockno += (N); \
		(IORQ)->dbuf += ((N)*XYFM_BPS); \
	} \
}

/*
 * note - addresses you can sleep on:
 *   [1] & of xy_softc's "state" (waiting for a chance to attach a drive)
 *   [2] & an iorq (waiting for an XY_SUB_WAIT iorq to finish)
 */


/*
 * function prototypes
 * "xyc_*" functions are internal, all others are external interfaces
 */

extern int pil_to_vme[];	/* from obio.c */

/* internals */
struct xy_iopb *xyc_chain __P((struct xyc_softc *, struct xy_iorq *));
int	xyc_cmd __P((struct xyc_softc *, int, int, int, int, int, char *, int));
char   *xyc_e2str __P((int));
int	xyc_entoact __P((int));
int	xyc_error __P((struct xyc_softc *, struct xy_iorq *,
		   struct xy_iopb *, int));
int	xyc_ioctlcmd __P((struct xy_softc *, dev_t dev, struct xd_iocmd *));
void	xyc_perror __P((struct xy_iorq *, struct xy_iopb *, int));
int	xyc_piodriver __P((struct xyc_softc *, struct xy_iorq *));
int	xyc_remove_iorq __P((struct xyc_softc *));
int	xyc_reset __P((struct xyc_softc *, int, struct xy_iorq *, int,
			struct xy_softc *));
inline void xyc_rqinit __P((struct xy_iorq *, struct xyc_softc *,
			    struct xy_softc *, int, u_long, int,
			    caddr_t, struct buf *));
void	xyc_rqtopb __P((struct xy_iorq *, struct xy_iopb *, int, int));
void	xyc_start __P((struct xyc_softc *, struct xy_iorq *));
int	xyc_startbuf __P((struct xyc_softc *, struct xy_softc *, struct buf *));
int	xyc_submit_iorq __P((struct xyc_softc *, struct xy_iorq *, int));
void	xyc_tick __P((void *));
int	xyc_unbusy __P((struct xyc *, int));
void	xyc_xyreset __P((struct xyc_softc *, struct xy_softc *));
int	xy_dmamem_alloc(bus_dma_tag_t, bus_dmamap_t, bus_dma_segment_t *,
			int *, bus_size_t, caddr_t *, bus_addr_t *);
void	xy_dmamem_free(bus_dma_tag_t, bus_dmamap_t, bus_dma_segment_t *,
			int, bus_size_t, caddr_t);

/* machine interrupt hook */
int	xycintr __P((void *));

/* autoconf */
int	xycmatch __P((struct device *, struct cfdata *, void *));
void	xycattach __P((struct device *, struct device *, void *));
int	xymatch __P((struct device *, struct cfdata *, void *));
void	xyattach __P((struct device *, struct device *, void *));
static	int xyc_probe __P((void *, bus_space_tag_t, bus_space_handle_t));

static	void xydummystrat __P((struct buf *));
int	xygetdisklabel __P((struct xy_softc *, void *));

bdev_decl(xy);
cdev_decl(xy);

/*
 * cfattach's: device driver interface to autoconfig
 */

struct cfattach xyc_ca = {
	sizeof(struct xyc_softc), xycmatch, xycattach
};

struct cfattach xy_ca = {
	sizeof(struct xy_softc), xymatch, xyattach
};

extern struct cfdriver xy_cd;

struct xyc_attach_args {	/* this is the "aux" args to xyattach */
	int	driveno;	/* unit number */
	int	fullmode;	/* submit mode */
	int	booting;	/* are we booting or not? */
};

/*
 * dkdriver
 */

struct dkdriver xydkdriver = { xystrategy };

/*
 * start: disk label fix code (XXX)
 */

static void *xy_labeldata;

static void
xydummystrat(bp)
	struct buf *bp;
{
	if (bp->b_bcount != XYFM_BPS)
		panic("xydummystrat");
	bcopy(xy_labeldata, bp->b_data, XYFM_BPS);
	bp->b_flags |= B_DONE;
	bp->b_flags &= ~B_BUSY;
}

int
xygetdisklabel(xy, b)
	struct xy_softc *xy;
	void *b;
{
	char *err;
#if defined(__sparc__) || defined(sun3)
	struct sun_disklabel *sdl;
#endif

	/* We already have the label data in `b'; setup for dummy strategy */
	xy_labeldata = b;

	/* Required parameter for readdisklabel() */
	xy->sc_dk.dk_label->d_secsize = XYFM_BPS;

	err = readdisklabel(MAKEDISKDEV(0, xy->sc_dev.dv_unit, RAW_PART),
					xydummystrat,
				xy->sc_dk.dk_label, xy->sc_dk.dk_cpulabel);
	if (err) {
		printf("%s: %s\n", xy->sc_dev.dv_xname, err);
		return(XY_ERR_FAIL);
	}

#if defined(__sparc__) || defined(sun3)
	/* Ok, we have the label; fill in `pcyl' if there's SunOS magic */
	sdl = (struct sun_disklabel *)xy->sc_dk.dk_cpulabel->cd_block;
	if (sdl->sl_magic == SUN_DKMAGIC) {
		xy->pcyl = sdl->sl_pcylinders;
	} else
#endif
	{
		printf("%s: WARNING: no `pcyl' in disk label.\n",
			xy->sc_dev.dv_xname);
		xy->pcyl = xy->sc_dk.dk_label->d_ncylinders +
			xy->sc_dk.dk_label->d_acylinders;
		printf("%s: WARNING: guessing pcyl=%d (ncyl+acyl)\n",
		xy->sc_dev.dv_xname, xy->pcyl);
	}

	xy->ncyl = xy->sc_dk.dk_label->d_ncylinders;
	xy->acyl = xy->sc_dk.dk_label->d_acylinders;
	xy->nhead = xy->sc_dk.dk_label->d_ntracks;
	xy->nsect = xy->sc_dk.dk_label->d_nsectors;
	xy->sectpercyl = xy->nhead * xy->nsect;
	xy->sc_dk.dk_label->d_secsize = XYFM_BPS; /* not handled by
                                          	  * sun->bsd */
	return(XY_ERR_AOK);
}

/*
 * end: disk label fix code (XXX)
 */

/*
 * Shorthand for allocating, mapping and loading a DMA buffer
 */
int
xy_dmamem_alloc(tag, map, seg, nsegp, len, kvap, dmap)
	bus_dma_tag_t		tag;
	bus_dmamap_t		map;
	bus_dma_segment_t	*seg;
	int			*nsegp;
	bus_size_t		len;
	caddr_t			*kvap;
	bus_addr_t		*dmap;
{
	int nseg;
	int error;

	if ((error = bus_dmamem_alloc(tag, len, 0, 0,
				      seg, 1, &nseg, BUS_DMA_NOWAIT)) != 0) {
		return (error);
	}

	if ((error = bus_dmamap_load_raw(tag, map,
					seg, nseg, len, BUS_DMA_NOWAIT)) != 0) {
		bus_dmamem_free(tag, seg, nseg);
		return (error);
	}

	if ((error = bus_dmamem_map(tag, seg, nseg,
				    len, kvap,
				    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		bus_dmamap_unload(tag, map);
		bus_dmamem_free(tag, seg, nseg);
		return (error);
	}

	*dmap = map->dm_segs[0].ds_addr;
	*nsegp = nseg;
	return (0);
}

void
xy_dmamem_free(tag, map, seg, nseg, len, kva)
	bus_dma_tag_t		tag;
	bus_dmamap_t		map;
	bus_dma_segment_t	*seg;
	int			nseg;
	bus_size_t		len;
	caddr_t			kva;
{

	bus_dmamap_unload(tag, map);
	bus_dmamem_unmap(tag, kva, len);
	bus_dmamem_free(tag, seg, nseg);
}


/*
 * a u t o c o n f i g   f u n c t i o n s
 */

/*
 * xycmatch: determine if xyc is present or not.   we do a
 * soft reset to detect the xyc.
 */
int
xyc_probe(arg, tag, handle)
	void *arg;
	bus_space_tag_t tag;
	bus_space_handle_t handle;
{
	struct xyc *xyc = (void *)handle; /* XXX */

	return ((xyc_unbusy(xyc, XYC_RESETUSEC) != XY_ERR_FAIL) ? 0 : EIO);
}

int xycmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct vme_attach_args	*va = aux;
	vme_chipset_tag_t	ct = va->va_vct;
	vme_am_t		mod;
	int error;

	mod = VME_AM_A16 | VME_AM_MBO | VME_AM_SUPER | VME_AM_DATA;
	if (vme_space_alloc(ct, va->r[0].offset, sizeof(struct xyc), mod))
		return (0);

	error = vme_probe(ct, va->r[0].offset, sizeof(struct xyc),
			  mod, VME_D16, xyc_probe, 0);
	vme_space_free(va->va_vct, va->r[0].offset, sizeof(struct xyc), mod);

	return (error == 0);
}

/*
 * xycattach: attach controller
 */
void
xycattach(parent, self, aux)
	struct device *parent, *self;
	void   *aux;

{
	struct xyc_softc	*xyc = (void *) self;
	struct vme_attach_args	*va = aux;
	vme_chipset_tag_t	ct = va->va_vct;
	bus_space_tag_t		bt;
	bus_space_handle_t	bh;
	vme_intr_handle_t	ih;
	vme_am_t		mod;
	struct xyc_attach_args	xa;
	int			lcv, res, error;
	bus_dma_segment_t	seg;
	int			rseg;
	vme_mapresc_t resc;

	/* get addressing and intr level stuff from autoconfig and load it
	 * into our xyc_softc. */

	mod = VME_AM_A16 | VME_AM_MBO | VME_AM_SUPER | VME_AM_DATA;

	if (vme_space_alloc(ct, va->r[0].offset, sizeof(struct xyc), mod))
		panic("xyc: vme alloc");

	if (vme_space_map(ct, va->r[0].offset, sizeof(struct xyc),
			  mod, VME_D16, 0, &bt, &bh, &resc) != 0)
		panic("xyc: vme_map");

	xyc->xyc = (struct xyc *) bh; /* XXX */
	xyc->ipl = va->ilevel;
	xyc->vector = va->ivector;
	xyc->no_ols = 0; /* XXX should be from config */

	for (lcv = 0; lcv < XYC_MAXDEV; lcv++)
		xyc->sc_drives[lcv] = (struct xy_softc *) 0;

	/*
	 * allocate and zero buffers
	 * check boundaries of the KVA's ... all IOPBs must reside in
 	 * the same 64K region.
	 */

	/* Get DMA handle for misc. transfers */
	if ((error = vme_dmamap_create(
				ct,		/* VME chip tag */
				MAXPHYS,	/* size */
				VME_AM_A24,	/* address modifier */
				VME_D16,	/* data size */
				0,		/* swap */
				1,		/* nsegments */
				MAXPHYS,	/* maxsegsz */
				0,		/* boundary */
				BUS_DMA_NOWAIT,
				&xyc->reqs[lcv].dmamap)) != 0) {

		printf("%s: DMA buffer map create error %d\n",
			xyc->sc_dev.dv_xname, error);
		return;
	}

	/* Get DMA handle for mapping iorq descriptors */
	if ((error = vme_dmamap_create(
				ct,		/* VME chip tag */
				XYC_MAXIOPB * sizeof(struct xy_iopb),
				VME_AM_A24,	/* address modifier */
				VME_D16,	/* data size */
				0,		/* swap */
				1,		/* nsegments */
				XYC_MAXIOPB * sizeof(struct xy_iopb),
				64*1024,	/* boundary */
				BUS_DMA_NOWAIT,
				&xyc->iopmap)) != 0) {

		printf("%s: DMA buffer map create error %d\n",
			xyc->sc_dev.dv_xname, error);
		return;
	}

	/* Get DMA buffer for iorq descriptors */
	if ((error = xy_dmamem_alloc(xyc->dmatag, xyc->iopmap, &seg, &rseg,
				     XYC_MAXIOPB * sizeof(struct xy_iopb),
				     (caddr_t *)&xyc->iopbase,
				     (bus_addr_t *)&xyc->dvmaiopb)) != 0) {
		printf("%s: DMA buffer alloc error %d\n",
			xyc->sc_dev.dv_xname, error);
		return;
	}

	bzero(xyc->iopbase, XYC_MAXIOPB * sizeof(struct xy_iopb));

	xyc->reqs = (struct xy_iorq *)
	    malloc(XYC_MAXIOPB * sizeof(struct xy_iorq), M_DEVBUF, M_NOWAIT);
	if (xyc->reqs == NULL)
		panic("xyc malloc");
	bzero(xyc->reqs, XYC_MAXIOPB * sizeof(struct xy_iorq));

	/*
	 * init iorq to iopb pointers, and non-zero fields in the
	 * iopb which never change.
	 */

	for (lcv = 0; lcv < XYC_MAXIOPB; lcv++) {
		xyc->xy_chain[lcv] = NULL;
		xyc->reqs[lcv].iopb = &xyc->iopbase[lcv];
		xyc->reqs[lcv].dmaiopb = &xyc->dvmaiopb[lcv];
		xyc->iopbase[lcv].asr = 1;	/* always the same */
		xyc->iopbase[lcv].eef = 1;	/* always the same */
		xyc->iopbase[lcv].ecm = XY_ECM;	/* always the same */
		xyc->iopbase[lcv].aud = 1;	/* always the same */
		xyc->iopbase[lcv].relo = 1;	/* always the same */
		xyc->iopbase[lcv].thro = XY_THRO;/* always the same */

		if ((error = vme_dmamap_create(
				ct,		/* VME chip tag */
				MAXPHYS,	/* size */
				VME_AM_A24,	/* address modifier */
				VME_D16,	/* data size */
				0,		/* swap */
				1,		/* nsegments */
				MAXPHYS,	/* maxsegsz */
				0,		/* boundary */
				BUS_DMA_NOWAIT,
				&xyc->reqs[lcv].dmamap)) != 0) {

			printf("%s: DMA buffer map create error %d\n",
				xyc->sc_dev.dv_xname, error);
			return;
		}
	}
	xyc->ciorq = &xyc->reqs[XYC_CTLIOPB];    /* short hand name */
	xyc->ciopb = &xyc->iopbase[XYC_CTLIOPB]; /* short hand name */
	xyc->xy_hand = 0;

	/* read controller parameters and insure we have a 450/451 */

	error = xyc_cmd(xyc, XYCMD_ST, 0, 0, 0, 0, 0, XY_SUB_POLL);
	res = xyc->ciopb->ctyp;
	XYC_DONE(xyc, error);
	if (res != XYCT_450) {
		if (error)
			printf(": %s: ", xyc_e2str(error));
		printf(": doesn't identify as a 450/451\n");
		return;
	}
	printf(": Xylogics 450/451");
	if (xyc->no_ols)
		printf(" [OLS disabled]"); /* 450 doesn't overlap seek right */
	printf("\n");
	if (error) {
		printf("%s: error: %s\n", xyc->sc_dev.dv_xname,
				xyc_e2str(error));
		return;
	}
	if ((xyc->xyc->xyc_csr & XYC_ADRM) == 0) {
		printf("%s: 24 bit addressing turned off\n",
			xyc->sc_dev.dv_xname);
		printf("please set hardware jumpers JM1-JM2=in, JM3-JM4=out\n");
		printf("to enable 24 bit mode and this driver\n");
		return;
	}

	/* link in interrupt with higher level software */
	vme_intr_map(ct, va->ilevel, va->ivector, &ih);
	vme_intr_establish(ct, ih, IPL_BIO, xycintr, xyc);
	evcnt_attach_dynamic(&xyc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    xyc->sc_dev.dv_xname, "intr");

	callout_init(&xyc->sc_tick_ch);

	/* now we must look for disks using autoconfig */
	xa.fullmode = XY_SUB_POLL;
	xa.booting = 1;

	for (xa.driveno = 0; xa.driveno < XYC_MAXDEV; xa.driveno++)
		(void) config_found(self, (void *) &xa, NULL);

	/* start the watchdog clock */
	callout_reset(&xyc->sc_tick_ch, XYC_TICKCNT, xyc_tick, xyc);

}

/*
 * xymatch: probe for disk.
 *
 * note: we almost always say disk is present.   this allows us to
 * spin up and configure a disk after the system is booted (we can
 * call xyattach!).
 */
int
xymatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct xyc_attach_args *xa = aux;

	/* looking for autoconf wildcard or exact match */

	if (cf->cf_loc[XYCCF_DRIVE] != XYCCF_DRIVE_DEFAULT &&
	    cf->cf_loc[XYCCF_DRIVE] != xa->driveno)
		return 0;

	return 1;

}

/*
 * xyattach: attach a disk.   this can be called from autoconf and also
 * from xyopen/xystrategy.
 */
void
xyattach(parent, self, aux)
	struct device *parent, *self;
	void   *aux;

{
	struct xy_softc *xy = (void *) self, *oxy;
	struct xyc_softc *xyc = (void *) parent;
	struct xyc_attach_args *xa = aux;
	int     spt, mb, blk, lcv, fmode, s = 0, newstate;
	struct dkbad *dkb;
	int			rseg, error;
	bus_dma_segment_t	seg;
	caddr_t			dmaddr;
	caddr_t			buf;

	/*
	 * Always re-initialize the disk structure.  We want statistics
	 * to start with a clean slate.
	 */
	bzero(&xy->sc_dk, sizeof(xy->sc_dk));
	xy->sc_dk.dk_driver = &xydkdriver;
	xy->sc_dk.dk_name = xy->sc_dev.dv_xname;

	/* if booting, init the xy_softc */

	if (xa->booting) {
		xy->state = XY_DRIVE_UNKNOWN;	/* to start */
		xy->flags = 0;
		xy->parent = xyc;

		/* init queue of waiting bufs */

		BUFQ_INIT(&xy->xyq);

		xy->xyrq = &xyc->reqs[xa->driveno];

	}
	xy->xy_drive = xa->driveno;
	fmode = xa->fullmode;
	xyc->sc_drives[xa->driveno] = xy;

	/* if not booting, make sure we are the only process in the attach for
	 * this drive.   if locked out, sleep on it. */

	if (!xa->booting) {
		s = splbio();
		while (xy->state == XY_DRIVE_ATTACHING) {
			if (tsleep(&xy->state, PRIBIO, "xyattach", 0)) {
				splx(s);
				return;
			}
		}
		printf("%s at %s",
			xy->sc_dev.dv_xname, xy->parent->sc_dev.dv_xname);
	}

	/* we now have control */
	xy->state = XY_DRIVE_ATTACHING;
	newstate = XY_DRIVE_UNKNOWN;

	buf = NULL;
	if ((error = xy_dmamem_alloc(xyc->dmatag, xyc->auxmap, &seg, &rseg,
				     XYFM_BPS,
				     (caddr_t *)&buf,
				     (bus_addr_t *)&dmaddr)) != 0) {
		printf("%s: DMA buffer alloc error %d\n",
			xyc->sc_dev.dv_xname, error);
		return;
	}

	/* first try and reset the drive */
	error = xyc_cmd(xyc, XYCMD_RST, 0, xy->xy_drive, 0, 0, 0, fmode);
	XYC_DONE(xyc, error);
	if (error == XY_ERR_DNRY) {
		printf(" drive %d: off-line\n", xa->driveno);
		goto done;
	}
	if (error) {
		printf(": ERROR 0x%02x (%s)\n", error, xyc_e2str(error));
		goto done;
	}
	printf(" drive %d: ready", xa->driveno);

	/*
	 * now set drive parameters (to semi-bogus values) so we can read the
	 * disk label.
	 */
	xy->pcyl = xy->ncyl = 1;
	xy->acyl = 0;
	xy->nhead = 1;
	xy->nsect = 1;
	xy->sectpercyl = 1;
	for (lcv = 0; lcv < 126; lcv++)	/* init empty bad144 table */
		xy->dkb.bt_bad[lcv].bt_cyl =
			xy->dkb.bt_bad[lcv].bt_trksec = 0xffff;

	/* read disk label */
	for (xy->drive_type = 0 ; xy->drive_type <= XYC_MAXDT ;
						xy->drive_type++) {
		error = xyc_cmd(xyc, XYCMD_RD, 0, xy->xy_drive, 0, 1,
						dmaddr, fmode);
		XYC_DONE(xyc, error);
		if (error == XY_ERR_AOK) break;
	}

	if (error != XY_ERR_AOK) {
		printf("\n%s: reading disk label failed: %s\n",
			xy->sc_dev.dv_xname, xyc_e2str(error));
		goto done;
	}
	printf(" (drive type %d)\n", xy->drive_type);

	newstate = XY_DRIVE_NOLABEL;

	xy->hw_spt = spt = 0; /* XXX needed ? */
	/* Attach the disk: must be before getdisklabel to malloc label */
	disk_attach(&xy->sc_dk);

	if (xygetdisklabel(xy, buf) != XY_ERR_AOK)
		goto done;

	/* inform the user of what is up */
	printf("%s: <%s>, pcyl %d\n", xy->sc_dev.dv_xname,
		buf, xy->pcyl);
	mb = xy->ncyl * (xy->nhead * xy->nsect) / (1048576 / XYFM_BPS);
	printf("%s: %dMB, %d cyl, %d head, %d sec, %d bytes/sec\n",
		xy->sc_dev.dv_xname, mb, xy->ncyl, xy->nhead, xy->nsect,
		XYFM_BPS);

	/*
	 * 450/451 stupidity: the drive type is encoded into the format
	 * of the disk.   the drive type in the IOPB must match the drive
	 * type in the format, or you will not be able to do I/O to the
	 * disk (you get header not found errors).  if you have two drives
	 * of different sizes that have the same drive type in their
	 * formatting then you are out of luck.
	 *
	 * this problem was corrected in the 753/7053.
	 */

	for (lcv = 0 ; lcv < XYC_MAXDEV ; lcv++) {
		oxy = xyc->sc_drives[lcv];
		if (oxy == NULL || oxy == xy) continue;
		if (oxy->drive_type != xy->drive_type) continue;
		if (xy->nsect != oxy->nsect || xy->pcyl != oxy->pcyl ||
			xy->nhead != oxy->nhead) {
			printf("%s: %s and %s must be the same size!\n",
				xyc->sc_dev.dv_xname, xy->sc_dev.dv_xname,
				oxy->sc_dev.dv_xname);
			panic("xy drive size mismatch");
		}
	}


	/* now set the real drive parameters! */

	blk = (xy->nsect - 1) +
		((xy->nhead - 1) * xy->nsect) +
		((xy->pcyl - 1) * xy->nsect * xy->nhead);
	error = xyc_cmd(xyc, XYCMD_SDS, 0, xy->xy_drive, blk, 0, 0, fmode);
	XYC_DONE(xyc, error);
	if (error) {
		printf("%s: write drive size failed: %s\n",
			xy->sc_dev.dv_xname, xyc_e2str(error));
		goto done;
	}
	newstate = XY_DRIVE_ONLINE;

	/*
	 * read bad144 table. this table resides on the first sector of the
	 * last track of the disk (i.e. second cyl of "acyl" area).
	 */

	blk = (xy->ncyl + xy->acyl - 1) * (xy->nhead * xy->nsect) +
								/* last cyl */
	    (xy->nhead - 1) * xy->nsect;	/* last head */
	error = xyc_cmd(xyc, XYCMD_RD, 0, xy->xy_drive, blk, 1,
						dmaddr, fmode);
	XYC_DONE(xyc, error);
	if (error) {
		printf("%s: reading bad144 failed: %s\n",
			xy->sc_dev.dv_xname, xyc_e2str(error));
		goto done;
	}

	/* check dkbad for sanity */
	dkb = (struct dkbad *) buf;
	for (lcv = 0; lcv < 126; lcv++) {
		if ((dkb->bt_bad[lcv].bt_cyl == 0xffff ||
				dkb->bt_bad[lcv].bt_cyl == 0) &&
		     dkb->bt_bad[lcv].bt_trksec == 0xffff)
			continue;	/* blank */
		if (dkb->bt_bad[lcv].bt_cyl >= xy->ncyl)
			break;
		if ((dkb->bt_bad[lcv].bt_trksec >> 8) >= xy->nhead)
			break;
		if ((dkb->bt_bad[lcv].bt_trksec & 0xff) >= xy->nsect)
			break;
	}
	if (lcv != 126) {
		printf("%s: warning: invalid bad144 sector!\n",
			xy->sc_dev.dv_xname);
	} else {
		bcopy(buf, &xy->dkb, XYFM_BPS);
	}

done:
	if (buf != NULL) {
		xy_dmamem_free(xyc->dmatag, xyc->auxmap,
				&seg, rseg, XYFM_BPS, buf);
	}

	xy->state = newstate;
	if (!xa->booting) {
		wakeup(&xy->state);
		splx(s);
	}
}

/*
 * end of autoconfig functions
 */

/*
 * { b , c } d e v s w   f u n c t i o n s
 */

/*
 * xyclose: close device
 */
int
xyclose(dev, flag, fmt, p)
	dev_t   dev;
	int     flag, fmt;
	struct proc *p;

{
	struct xy_softc *xy = xy_cd.cd_devs[DISKUNIT(dev)];
	int     part = DISKPART(dev);

	/* clear mask bits */

	switch (fmt) {
	case S_IFCHR:
		xy->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		xy->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	xy->sc_dk.dk_openmask = xy->sc_dk.dk_copenmask | xy->sc_dk.dk_bopenmask;

	return 0;
}

/*
 * xydump: crash dump system
 */
int
xydump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{
	int     unit, part;
	struct xy_softc *xy;

	unit = DISKUNIT(dev);
	if (unit >= xy_cd.cd_ndevs)
		return ENXIO;
	part = DISKPART(dev);

	xy = xy_cd.cd_devs[unit];

	printf("%s%c: crash dump not supported (yet)\n", xy->sc_dev.dv_xname,
	    'a' + part);

	return ENXIO;

	/* outline: globals: "dumplo" == sector number of partition to start
	 * dump at (convert to physical sector with partition table)
	 * "dumpsize" == size of dump in clicks "physmem" == size of physical
	 * memory (clicks, ctob() to get bytes) (normal case: dumpsize ==
	 * physmem)
	 *
	 * dump a copy of physical memory to the dump device starting at sector
	 * "dumplo" in the swap partition (make sure > 0).   map in pages as
	 * we go.   use polled I/O.
	 *
	 * XXX how to handle NON_CONTIG? */

}

/*
 * xyioctl: ioctls on XY drives.   based on ioctl's of other netbsd disks.
 */
int
xyioctl(dev, command, addr, flag, p)
	dev_t   dev;
	u_long  command;
	caddr_t addr;
	int     flag;
	struct proc *p;

{
	struct xy_softc *xy;
	struct xd_iocmd *xio;
	int     error, s, unit;
#ifdef __HAVE_OLD_DISKLABEL
	struct disklabel newlabel, *lp;
#endif
	unit = DISKUNIT(dev);

	if (unit >= xy_cd.cd_ndevs || (xy = xy_cd.cd_devs[unit]) == NULL)
		return (ENXIO);

	/* switch on ioctl type */

	switch (command) {
	case DIOCSBAD:		/* set bad144 info */
		if ((flag & FWRITE) == 0)
			return EBADF;
		s = splbio();
		bcopy(addr, &xy->dkb, sizeof(xy->dkb));
		splx(s);
		return 0;

	case DIOCGDINFO:	/* get disk label */
		bcopy(xy->sc_dk.dk_label, addr, sizeof(struct disklabel));
		return 0;
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDINFO:
		newlabel = *(xy->sc_dk.dk_label);
		if (newlabel.d_npartitions > OLDMAXPARTITIONS)
			return ENOTTY;
		memcpy(addr, &newlabel, sizeof (struct olddisklabel));
		return 0;
#endif

	case DIOCGPART:	/* get partition info */
		((struct partinfo *) addr)->disklab = xy->sc_dk.dk_label;
		((struct partinfo *) addr)->part =
		    &xy->sc_dk.dk_label->d_partitions[DISKPART(dev)];
		return 0;

	case DIOCSDINFO:	/* set disk label */
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCSDINFO:
		if (command == ODIOCSDINFO) {
			memset(&newlabel, 0, sizeof newlabel);
			memcpy(&newlabel, addr, sizeof (struct olddisklabel));
			lp = &newlabel;
		} else
#endif
		lp = (struct disklabel *)addr;

		if ((flag & FWRITE) == 0)
			return EBADF;
		error = setdisklabel(xy->sc_dk.dk_label,
		    lp, /* xy->sc_dk.dk_openmask : */ 0,
		    xy->sc_dk.dk_cpulabel);
		if (error == 0) {
			if (xy->state == XY_DRIVE_NOLABEL)
				xy->state = XY_DRIVE_ONLINE;
		}
		return error;

	case DIOCWLABEL:	/* change write status of disk label */
		if ((flag & FWRITE) == 0)
			return EBADF;
		if (*(int *) addr)
			xy->flags |= XY_WLABEL;
		else
			xy->flags &= ~XY_WLABEL;
		return 0;

	case DIOCWDINFO:	/* write disk label */
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCWDINFO:
		if (command == ODIOCWDINFO) {
			memset(&newlabel, 0, sizeof newlabel);
			memcpy(&newlabel, addr, sizeof (struct olddisklabel));
			lp = &newlabel;
		} else
#endif
		lp = (struct disklabel *)addr;

		if ((flag & FWRITE) == 0)
			return EBADF;
		error = setdisklabel(xy->sc_dk.dk_label,
		    lp, /* xy->sc_dk.dk_openmask : */ 0,
		    xy->sc_dk.dk_cpulabel);
		if (error == 0) {
			if (xy->state == XY_DRIVE_NOLABEL)
				xy->state = XY_DRIVE_ONLINE;

			/* Simulate opening partition 0 so write succeeds. */
			xy->sc_dk.dk_openmask |= (1 << 0);
			error = writedisklabel(MAKEDISKDEV(major(dev), DISKUNIT(dev), RAW_PART),
			    xystrategy, xy->sc_dk.dk_label,
			    xy->sc_dk.dk_cpulabel);
			xy->sc_dk.dk_openmask =
			    xy->sc_dk.dk_copenmask | xy->sc_dk.dk_bopenmask;
		}
		return error;

	case DIOSXDCMD:
		xio = (struct xd_iocmd *) addr;
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (error);
		return (xyc_ioctlcmd(xy, dev, xio));

	default:
		return ENOTTY;
	}
}

/*
 * xyopen: open drive
 */

int
xyopen(dev, flag, fmt, p)
	dev_t   dev;
	int     flag, fmt;
	struct proc *p;
{
	int     unit, part;
	struct xy_softc *xy;
	struct xyc_attach_args xa;

	/* first, could it be a valid target? */

	unit = DISKUNIT(dev);
	if (unit >= xy_cd.cd_ndevs || (xy = xy_cd.cd_devs[unit]) == NULL)
		return (ENXIO);
	part = DISKPART(dev);

	/* do we need to attach the drive? */

	if (xy->state == XY_DRIVE_UNKNOWN) {
		xa.driveno = xy->xy_drive;
		xa.fullmode = XY_SUB_WAIT;
		xa.booting = 0;
		xyattach((struct device *) xy->parent,
						(struct device *) xy, &xa);
		if (xy->state == XY_DRIVE_UNKNOWN) {
			return (EIO);
		}
	}
	/* check for partition */

	if (part != RAW_PART &&
	    (part >= xy->sc_dk.dk_label->d_npartitions ||
		xy->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
		return (ENXIO);
	}
	/* set open masks */

	switch (fmt) {
	case S_IFCHR:
		xy->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		xy->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	xy->sc_dk.dk_openmask = xy->sc_dk.dk_copenmask | xy->sc_dk.dk_bopenmask;

	return 0;
}

int
xyread(dev, uio, flags)
	dev_t   dev;
	struct uio *uio;
	int flags;
{

	return (physio(xystrategy, NULL, dev, B_READ, minphys, uio));
}

int
xywrite(dev, uio, flags)
	dev_t   dev;
	struct uio *uio;
	int flags;
{

	return (physio(xystrategy, NULL, dev, B_WRITE, minphys, uio));
}


/*
 * xysize: return size of a partition for a dump
 */

int
xysize(dev)
	dev_t   dev;

{
	struct xy_softc *xysc;
	int     unit, part, size, omask;

	/* valid unit? */
	unit = DISKUNIT(dev);
	if (unit >= xy_cd.cd_ndevs || (xysc = xy_cd.cd_devs[unit]) == NULL)
		return (-1);

	part = DISKPART(dev);
	omask = xysc->sc_dk.dk_openmask & (1 << part);

	if (omask == 0 && xyopen(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);

	/* do it */
	if (xysc->sc_dk.dk_label->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;	/* only give valid size for swap partitions */
	else
		size = xysc->sc_dk.dk_label->d_partitions[part].p_size *
		    (xysc->sc_dk.dk_label->d_secsize / DEV_BSIZE);
	if (omask == 0 && xyclose(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);
	return (size);
}

/*
 * xystrategy: buffering system interface to xy.
 */

void
xystrategy(bp)
	struct buf *bp;

{
	struct xy_softc *xy;
	int     s, unit;
	struct xyc_attach_args xa;
	struct disklabel *lp;
	daddr_t blkno;

	unit = DISKUNIT(bp->b_dev);

	/* check for live device */

	if (unit >= xy_cd.cd_ndevs || (xy = xy_cd.cd_devs[unit]) == 0 ||
	    bp->b_blkno < 0 ||
	    (bp->b_bcount % xy->sc_dk.dk_label->d_secsize) != 0) {
		bp->b_error = EINVAL;
		goto bad;
	}
	/* do we need to attach the drive? */

	if (xy->state == XY_DRIVE_UNKNOWN) {
		xa.driveno = xy->xy_drive;
		xa.fullmode = XY_SUB_WAIT;
		xa.booting = 0;
		xyattach((struct device *)xy->parent, (struct device *)xy, &xa);
		if (xy->state == XY_DRIVE_UNKNOWN) {
			bp->b_error = EIO;
			goto bad;
		}
	}
	if (xy->state != XY_DRIVE_ONLINE && DISKPART(bp->b_dev) != RAW_PART) {
		/* no I/O to unlabeled disks, unless raw partition */
		bp->b_error = EIO;
		goto bad;
	}
	/* short circuit zero length request */

	if (bp->b_bcount == 0)
		goto done;

	/* check bounds with label (disksubr.c).  Determine the size of the
	 * transfer, and make sure it is within the boundaries of the
	 * partition. Adjust transfer if needed, and signal errors or early
	 * completion. */

	lp = xy->sc_dk.dk_label;

	if (bounds_check_with_label(bp, lp,
		(xy->flags & XY_WLABEL) != 0) <= 0)
		goto done;

	/*
	 * Now convert the block number to absolute and put it in
	 * terms of the device's logical block size.
	 */
	blkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);
	if (DISKPART(bp->b_dev) != RAW_PART)
		blkno += lp->d_partitions[DISKPART(bp->b_dev)].p_offset;

	bp->b_rawblkno = blkno;

	/*
	 * now we know we have a valid buf structure that we need to do I/O
	 * on.
	 */
	s = splbio();		/* protect the queues */

	disksort_blkno(&xy->xyq, bp);

	/* start 'em up */

	xyc_start(xy->parent, NULL);

	/* done! */

	splx(s);
	return;

bad:				/* tells upper layers we have an error */
	bp->b_flags |= B_ERROR;
done:				/* tells upper layers we are done with this
				 * buf */
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}
/*
 * end of {b,c}devsw functions
 */

/*
 * i n t e r r u p t   f u n c t i o n
 *
 * xycintr: hardware interrupt.
 */
int
xycintr(v)
	void   *v;

{
	struct xyc_softc *xycsc = v;

	/* kick the event counter */

	xycsc->sc_intrcnt.ev_count++;

	/* remove as many done IOPBs as possible */

	xyc_remove_iorq(xycsc);

	/* start any iorq's already waiting */

	xyc_start(xycsc, NULL);

	return (1);
}
/*
 * end of interrupt function
 */

/*
 * i n t e r n a l   f u n c t i o n s
 */

/*
 * xyc_rqinit: fill out the fields of an I/O request
 */

inline void
xyc_rqinit(rq, xyc, xy, md, blk, cnt, db, bp)
	struct xy_iorq *rq;
	struct xyc_softc *xyc;
	struct xy_softc *xy;
	int     md;
	u_long  blk;
	int     cnt;
	caddr_t db;
	struct buf *bp;
{
	rq->xyc = xyc;
	rq->xy = xy;
	rq->ttl = XYC_MAXTTL + 10;
	rq->mode = md;
	rq->tries = rq->errno = rq->lasterror = 0;
	rq->blockno = blk;
	rq->sectcnt = cnt;
	rq->dbuf = db;
	rq->buf = bp;
}

/*
 * xyc_rqtopb: load up an IOPB based on an iorq
 */

void
xyc_rqtopb(iorq, iopb, cmd, subfun)
	struct xy_iorq *iorq;
	struct xy_iopb *iopb;
	int     cmd, subfun;

{
	u_long  block, dp;

	/* normal IOPB case, standard stuff */

	/* chain bit handled later */
	iopb->ien = (XY_STATE(iorq->mode) == XY_SUB_POLL) ? 0 : 1;
	iopb->com = cmd;
	iopb->errno = 0;
	iopb->errs = 0;
	iopb->done = 0;
	if (iorq->xy) {
		iopb->unit = iorq->xy->xy_drive;
		iopb->dt = iorq->xy->drive_type;
	} else {
		iopb->unit = 0;
		iopb->dt = 0;
	}
	block = iorq->blockno;
	if (iorq->xy == NULL || block == 0) {
		iopb->sect = iopb->head = iopb->cyl = 0;
	} else {
		iopb->sect = block % iorq->xy->nsect;
		block = block / iorq->xy->nsect;
		iopb->head = block % iorq->xy->nhead;
		block = block / iorq->xy->nhead;
		iopb->cyl = block;
	}
	iopb->scnt = iorq->sectcnt;
	dp = (u_long) iorq->dbuf;
	if (iorq->dbuf == NULL) {
		iopb->dataa = 0;
		iopb->datar = 0;
	} else {
		iopb->dataa = (dp & 0xffff);
		iopb->datar = ((dp & 0xff0000) >> 16);
	}
	iopb->subfn = subfun;
}


/*
 * xyc_unbusy: wait for the xyc to go unbusy, or timeout.
 */

int
xyc_unbusy(xyc, del)

struct xyc *xyc;
int del;

{
	while (del-- > 0) {
		if ((xyc->xyc_csr & XYC_GBSY) == 0)
			break;
		DELAY(1);
	}
	return(del == 0 ? XY_ERR_FAIL : XY_ERR_AOK);
}

/*
 * xyc_cmd: front end for POLL'd and WAIT'd commands.  Returns 0 or error.
 * note that NORM requests are handled seperately.
 */
int
xyc_cmd(xycsc, cmd, subfn, unit, block, scnt, dptr, fullmode)
	struct xyc_softc *xycsc;
	int     cmd, subfn, unit, block, scnt;
	char   *dptr;
	int     fullmode;

{
	int     submode = XY_STATE(fullmode);
	struct xy_iorq *iorq = xycsc->ciorq;
	struct xy_iopb *iopb = xycsc->ciopb;

	/*
	 * is someone else using the control iopq wait for it if we can
	 */
start:
	if (submode == XY_SUB_WAIT && XY_STATE(iorq->mode) != XY_SUB_FREE) {
		if (tsleep(iorq, PRIBIO, "xyc_cmd", 0))
                                return(XY_ERR_FAIL);
		goto start;
	}

	if (XY_STATE(iorq->mode) != XY_SUB_FREE) {
		DELAY(1000000);		/* XY_SUB_POLL: steal the iorq */
		iorq->mode = XY_SUB_FREE;
		printf("%s: stole control iopb\n", xycsc->sc_dev.dv_xname);
	}

	/* init iorq/iopb */

	xyc_rqinit(iorq, xycsc,
	    (unit == XYC_NOUNIT) ? NULL : xycsc->sc_drives[unit],
	    fullmode, block, scnt, dptr, NULL);

	/* load IOPB from iorq */

	xyc_rqtopb(iorq, iopb, cmd, subfn);

	/* submit it for processing */

	xyc_submit_iorq(xycsc, iorq, fullmode);	/* error code will be in iorq */

	return(XY_ERR_AOK);
}

/*
 * xyc_startbuf
 * start a buffer for running
 */

int
xyc_startbuf(xycsc, xysc, bp)
	struct xyc_softc *xycsc;
	struct xy_softc *xysc;
	struct buf *bp;

{
	int     partno, error;
	struct xy_iorq *iorq;
	struct xy_iopb *iopb;
	u_long  block;

	iorq = xysc->xyrq;
	iopb = iorq->iopb;

	/* get buf */

	if (bp == NULL)
		panic("xyc_startbuf null buf");

	partno = DISKPART(bp->b_dev);
#ifdef XYC_DEBUG
	printf("xyc_startbuf: %s%c: %s block %d\n", xysc->sc_dev.dv_xname,
	    'a' + partno, (bp->b_flags & B_READ) ? "read" : "write", bp->b_blkno);
	printf("xyc_startbuf: b_bcount %d, b_data 0x%x\n",
	    bp->b_bcount, bp->b_data);
#endif

	/*
	 * load request.
	 *
	 * note that iorq points to the buffer as mapped into DVMA space,
	 * where as the bp->b_data points to its non-DVMA mapping.
	 */

	block = bp->b_rawblkno;

	error = bus_dmamap_load(xycsc->dmatag, iorq->dmamap,
			bp->b_data, bp->b_bcount, 0, BUS_DMA_NOWAIT);
	if (error != 0) {
		printf("%s: warning: cannot load DMA map\n",
			xycsc->sc_dev.dv_xname);
		return (XY_ERR_FAIL);	/* XXX: need some sort of
					 * call-back scheme here? */
	}

	bus_dmamap_sync(xycsc->dmatag, iorq->dmamap, 0,
			iorq->dmamap->dm_mapsize, (bp->b_flags & B_READ)
				? BUS_DMASYNC_PREREAD
				: BUS_DMASYNC_PREWRITE);

	/* init iorq and load iopb from it */
	xyc_rqinit(iorq, xycsc, xysc, XY_SUB_NORM | XY_MODE_VERBO, block,
		   bp->b_bcount / XYFM_BPS,
		   (caddr_t)iorq->dmamap->dm_segs[0].ds_addr,
		   bp);

	xyc_rqtopb(iorq, iopb, (bp->b_flags & B_READ) ? XYCMD_RD : XYCMD_WR, 0);

	/* Instrumentation. */
	disk_busy(&xysc->sc_dk);

	return (XY_ERR_AOK);
}


/*
 * xyc_submit_iorq: submit an iorq for processing.  returns XY_ERR_AOK
 * if ok.  if it fail returns an error code.  type is XY_SUB_*.
 *
 * note: caller frees iorq in all cases except NORM
 *
 * return value:
 *   NORM: XY_AOK (req pending), XY_FAIL (couldn't submit request)
 *   WAIT: XY_AOK (success), <error-code> (failed)
 *   POLL: <same as WAIT>
 *   NOQ : <same as NORM>
 *
 * there are three sources for i/o requests:
 * [1] xystrategy: normal block I/O, using "struct buf" system.
 * [2] autoconfig/crash dump: these are polled I/O requests, no interrupts.
 * [3] open/ioctl: these are I/O requests done in the context of a process,
 *                 and the process should block until they are done.
 *
 * software state is stored in the iorq structure.  each iorq has an
 * iopb structure.  the hardware understands the iopb structure.
 * every command must go through an iopb.  a 450 handles one iopb at a
 * time, where as a 451 can take them in chains.  [the 450 claims it
 * can handle chains, but is appears to be buggy...]   iopb are allocated
 * in DVMA space at boot up time.  each disk gets one iopb, and the
 * controller gets one (for POLL and WAIT commands).  what happens if
 * the iopb is busy?  for i/o type [1], the buffers are queued at the
 * "buff" layer and * picked up later by the interrupt routine.  for case
 * [2] we can only be blocked if there is a WAIT type I/O request being
 * run.   since this can only happen when we are crashing, we wait a sec
 * and then steal the IOPB.  for case [3] the process can sleep
 * on the iorq free list until some iopbs are avaliable.
 */


int
xyc_submit_iorq(xycsc, iorq, type)
	struct xyc_softc *xycsc;
	struct xy_iorq *iorq;
	int     type;

{
	struct xy_iopb *dmaiopb;

#ifdef XYC_DEBUG
	printf("xyc_submit_iorq(%s, addr=0x%x, type=%d)\n",
		xycsc->sc_dev.dv_xname, iorq, type);
#endif

	/* first check and see if controller is busy */
	if ((xycsc->xyc->xyc_csr & XYC_GBSY) != 0) {
#ifdef XYC_DEBUG
		printf("xyc_submit_iorq: XYC not ready (BUSY)\n");
#endif
		if (type == XY_SUB_NOQ)
			return (XY_ERR_FAIL);	/* failed */
		switch (type) {
		case XY_SUB_NORM:
			return XY_ERR_AOK;	/* success */
		case XY_SUB_WAIT:
			while (iorq->iopb->done == 0) {
				(void) tsleep(iorq, PRIBIO, "xyciorq", 0);
			}
			return (iorq->errno);
		case XY_SUB_POLL:		/* steal controller */
			(void)xycsc->xyc->xyc_rsetup; /* RESET */
			if (xyc_unbusy(xycsc->xyc,XYC_RESETUSEC) == XY_ERR_FAIL)
				panic("xyc_submit_iorq: stuck xyc");
			printf("%s: stole controller\n",
				xycsc->sc_dev.dv_xname);
			break;
		default:
			panic("xyc_submit_iorq adding");
		}
	}

	dmaiopb = xyc_chain(xycsc, iorq);	 /* build chain */
	if (dmaiopb == NULL) { /* nothing doing? */
		if (type == XY_SUB_NORM || type == XY_SUB_NOQ)
			return(XY_ERR_AOK);
		panic("xyc_submit_iorq: xyc_chain failed!\n");
	}

	XYC_GO(xycsc->xyc, (u_long)dmaiopb);

	/* command now running, wrap it up */
	switch (type) {
	case XY_SUB_NORM:
	case XY_SUB_NOQ:
		return (XY_ERR_AOK);	/* success */
	case XY_SUB_WAIT:
		while (iorq->iopb->done == 0) {
			(void) tsleep(iorq, PRIBIO, "xyciorq", 0);
		}
		return (iorq->errno);
	case XY_SUB_POLL:
		return (xyc_piodriver(xycsc, iorq));
	default:
		panic("xyc_submit_iorq wrap up");
	}
	panic("xyc_submit_iorq");
	return 0;	/* not reached */
}


/*
 * xyc_chain: build a chain.  return dvma address of first element in
 * the chain.   iorq != NULL: means we only want that item on the chain.
 */

struct xy_iopb *
xyc_chain(xycsc, iorq)
	struct xyc_softc *xycsc;
	struct xy_iorq *iorq;

{
	int togo, chain, hand;

	bzero(xycsc->xy_chain, sizeof(xycsc->xy_chain));

	/*
	 * promote control IOPB to the top
	 */
	if (iorq == NULL) {
		if ((XY_STATE(xycsc->reqs[XYC_CTLIOPB].mode) == XY_SUB_POLL ||
		     XY_STATE(xycsc->reqs[XYC_CTLIOPB].mode) == XY_SUB_WAIT) &&
		     xycsc->iopbase[XYC_CTLIOPB].done == 0)
			iorq = &xycsc->reqs[XYC_CTLIOPB];
	}

	/*
	 * special case: if iorq != NULL then we have a POLL or WAIT request.
	 * we let these take priority and do them first.
	 */
	if (iorq) {
		xycsc->xy_chain[0] = iorq;
		iorq->iopb->chen = 0;
		return(iorq->dmaiopb);
	}

	/*
	 * NORM case: do round robin and maybe chain (if allowed and possible)
	 */
	chain = 0;
	hand = xycsc->xy_hand;
	xycsc->xy_hand = (xycsc->xy_hand + 1) % XYC_MAXIOPB;

	for (togo = XYC_MAXIOPB; togo > 0;
	     togo--, hand = (hand + 1) % XYC_MAXIOPB) {
		struct xy_iopb *iopb, *prev_iopb, *dmaiopb;

		if (XY_STATE(xycsc->reqs[hand].mode) != XY_SUB_NORM ||
		    xycsc->iopbase[hand].done)
			continue;   /* not ready-for-i/o */

		xycsc->xy_chain[chain] = &xycsc->reqs[hand];
		iopb = xycsc->xy_chain[chain]->iopb;
		iopb->chen = 0;
		if (chain != 0) {
			/* adding a link to a chain */
			prev_iopb = xycsc->xy_chain[chain-1]->iopb;
			prev_iopb->chen = 1;
			dmaiopb = xycsc->xy_chain[chain]->dmaiopb;
			prev_iopb->nxtiopb = ((u_long)dmaiopb) & 0xffff;
		} else {
			/* head of chain */
			iorq = xycsc->xy_chain[chain];
		}
		chain++;

		/* quit if chaining dis-allowed */
		if (xycsc->no_ols)
			break;
	}

	return(iorq ? iorq->dmaiopb : NULL);
}

/*
 * xyc_piodriver
 *
 * programmed i/o driver.   this function takes over the computer
 * and drains off the polled i/o request.   it returns the status of the iorq
 * the caller is interesting in.
 */
int
xyc_piodriver(xycsc, iorq)
	struct xyc_softc *xycsc;
	struct xy_iorq  *iorq;

{
	int     nreset = 0;
	int     retval = 0;
	u_long  res;
#ifdef XYC_DEBUG
	printf("xyc_piodriver(%s, 0x%x)\n", xycsc->sc_dev.dv_xname, iorq);
#endif

	while (iorq->iopb->done == 0) {

		res = xyc_unbusy(xycsc->xyc, XYC_MAXTIME);

		/* we expect some progress soon */
		if (res == XY_ERR_FAIL && nreset >= 2) {
			xyc_reset(xycsc, 0, XY_RSET_ALL, XY_ERR_FAIL, 0);
#ifdef XYC_DEBUG
			printf("xyc_piodriver: timeout\n");
#endif
			return (XY_ERR_FAIL);
		}
		if (res == XY_ERR_FAIL) {
			if (xyc_reset(xycsc, 0,
				      (nreset++ == 0) ? XY_RSET_NONE : iorq,
				      XY_ERR_FAIL,
				      0) == XY_ERR_FAIL)
				return (XY_ERR_FAIL);	/* flushes all but POLL
							 * requests, resets */
			continue;
		}

		xyc_remove_iorq(xycsc);	 /* may resubmit request */

		if (iorq->iopb->done == 0)
			xyc_start(xycsc, iorq);
	}

	/* get return value */

	retval = iorq->errno;

#ifdef XYC_DEBUG
	printf("xyc_piodriver: done, retval = 0x%x (%s)\n",
	    iorq->errno, xyc_e2str(iorq->errno));
#endif

	/* start up any bufs that have queued */

	xyc_start(xycsc, NULL);

	return (retval);
}

/*
 * xyc_xyreset: reset one drive.   NOTE: assumes xyc was just reset.
 * we steal iopb[XYC_CTLIOPB] for this, but we put it back when we are done.
 */
void
xyc_xyreset(xycsc, xysc)
	struct xyc_softc *xycsc;
	struct xy_softc *xysc;

{
	struct xy_iopb tmpiopb;
	struct xy_iopb *iopb;
	int     del;

	iopb = xycsc->ciopb;

	/* Save contents */
	bcopy(iopb, &tmpiopb, sizeof(struct xy_iopb));

	iopb->chen = iopb->done = iopb->errs = 0;
	iopb->ien = 0;
	iopb->com = XYCMD_RST;
	iopb->unit = xysc->xy_drive;

	XYC_GO(xycsc->xyc, (u_long)xycsc->ciorq->dmaiopb);

	del = XYC_RESETUSEC;
	while (del > 0) {
		if ((xycsc->xyc->xyc_csr & XYC_GBSY) == 0)
			break;
		DELAY(1);
		del--;
	}

	if (del <= 0 || iopb->errs) {
		printf("%s: off-line: %s\n", xycsc->sc_dev.dv_xname,
		    xyc_e2str(iopb->errno));
		del = xycsc->xyc->xyc_rsetup;
		if (xyc_unbusy(xycsc->xyc, XYC_RESETUSEC) == XY_ERR_FAIL)
			panic("xyc_reset");
	} else {
		xycsc->xyc->xyc_csr = XYC_IPND;	/* clear IPND */
	}

	/* Restore contents */
	bcopy(&tmpiopb, iopb, sizeof(struct xy_iopb));
}


/*
 * xyc_reset: reset everything: requests are marked as errors except
 * a polled request (which is resubmitted)
 */
int
xyc_reset(xycsc, quiet, blastmode, error, xysc)
	struct xyc_softc *xycsc;
	int     quiet, error;
	struct xy_iorq *blastmode;
	struct xy_softc *xysc;

{
	int     del = 0, lcv, retval = XY_ERR_AOK;

	/* soft reset hardware */

	if (!quiet)
		printf("%s: soft reset\n", xycsc->sc_dev.dv_xname);
	del = xycsc->xyc->xyc_rsetup;
	del = xyc_unbusy(xycsc->xyc, XYC_RESETUSEC);
	if (del == XY_ERR_FAIL) {
		blastmode = XY_RSET_ALL;	/* dead, flush all requests */
		retval = XY_ERR_FAIL;
	}
	if (xysc)
		xyc_xyreset(xycsc, xysc);

	/* fix queues based on "blast-mode" */

	for (lcv = 0; lcv < XYC_MAXIOPB; lcv++) {
		register struct xy_iorq *iorq = &xycsc->reqs[lcv];

		if (XY_STATE(iorq->mode) != XY_SUB_POLL &&
		    XY_STATE(iorq->mode) != XY_SUB_WAIT &&
		    XY_STATE(iorq->mode) != XY_SUB_NORM)
			/* is it active? */
			continue;

		if (blastmode == XY_RSET_ALL ||
				blastmode != iorq) {
			/* failed */
			iorq->errno = error;
			xycsc->iopbase[lcv].done = xycsc->iopbase[lcv].errs = 1;
			switch (XY_STATE(iorq->mode)) {
			case XY_SUB_NORM:
			    iorq->buf->b_error = EIO;
			    iorq->buf->b_flags |= B_ERROR;
			    iorq->buf->b_resid = iorq->sectcnt * XYFM_BPS;

			    bus_dmamap_sync(xycsc->dmatag, iorq->dmamap, 0,
					iorq->dmamap->dm_mapsize,
					(iorq->buf->b_flags & B_READ)
						? BUS_DMASYNC_POSTREAD
						: BUS_DMASYNC_POSTWRITE);
                
			    bus_dmamap_unload(xycsc->dmatag, iorq->dmamap);

			    BUFQ_REMOVE(&iorq->xy->xyq, iorq->buf);
			    disk_unbusy(&xycsc->reqs[lcv].xy->sc_dk,
				(xycsc->reqs[lcv].buf->b_bcount -
				xycsc->reqs[lcv].buf->b_resid));
			    biodone(iorq->buf);
			    iorq->mode = XY_SUB_FREE;
			    break;
			case XY_SUB_WAIT:
			    wakeup(iorq);
			case XY_SUB_POLL:
			    iorq->mode =
				XY_NEWSTATE(iorq->mode, XY_SUB_DONE);
			    break;
			}

		} else {

			/* resubmit, no need to do anything here */
		}
	}

	/*
	 * now, if stuff is waiting, start it.
	 * since we just reset it should go
	 */
	xyc_start(xycsc, NULL);

	return (retval);
}

/*
 * xyc_start: start waiting buffers
 */

void
xyc_start(xycsc, iorq)
	struct xyc_softc *xycsc;
	struct xy_iorq *iorq;

{
	int lcv;
	struct xy_softc *xy;

	if (iorq == NULL) {
		for (lcv = 0; lcv < XYC_MAXDEV ; lcv++) {
			if ((xy = xycsc->sc_drives[lcv]) == NULL) continue;
			if (BUFQ_FIRST(&xy->xyq) == NULL) continue;
			if (xy->xyrq->mode != XY_SUB_FREE) continue;
			xyc_startbuf(xycsc, xy, BUFQ_FIRST(&xy->xyq));
		}
	}
	xyc_submit_iorq(xycsc, iorq, XY_SUB_NOQ);
}

/*
 * xyc_remove_iorq: remove "done" IOPB's.
 */

int
xyc_remove_iorq(xycsc)
	struct xyc_softc *xycsc;

{
	int     errno, rq, comm, errs;
	struct xyc *xyc = xycsc->xyc;
	u_long  addr;
	struct xy_iopb *iopb;
	struct xy_iorq *iorq;
	struct buf *bp;

	if (xyc->xyc_csr & XYC_DERR) {
		/*
		 * DOUBLE ERROR: should never happen under normal use. This
		 * error is so bad, you can't even tell which IOPB is bad, so
		 * we dump them all.
		 */
		errno = XY_ERR_DERR;
		printf("%s: DOUBLE ERROR!\n", xycsc->sc_dev.dv_xname);
		if (xyc_reset(xycsc, 0, XY_RSET_ALL, errno, 0) != XY_ERR_AOK) {
			printf("%s: soft reset failed!\n",
				xycsc->sc_dev.dv_xname);
			panic("xyc_remove_iorq: controller DEAD");
		}
		return (XY_ERR_AOK);
	}

	/*
	 * get iopb that is done, loop down the chain
	 */

	if (xyc->xyc_csr & XYC_ERR) {
		xyc->xyc_csr = XYC_ERR; /* clear error condition */
	}
	if (xyc->xyc_csr & XYC_IPND) {
		xyc->xyc_csr = XYC_IPND; /* clear interrupt */
	}

	for (rq = 0; rq < XYC_MAXIOPB; rq++) {
		iorq = xycsc->xy_chain[rq];
		if (iorq == NULL) break; /* done ! */
		if (iorq->mode == 0 || XY_STATE(iorq->mode) == XY_SUB_DONE)
			continue;	/* free, or done */
		iopb = iorq->iopb;
		if (iopb->done == 0)
			continue;	/* not done yet */

		comm = iopb->com;
		errs = iopb->errs;

		if (errs)
			iorq->errno = iopb->errno;
		else
			iorq->errno = 0;

		/* handle non-fatal errors */

		if (errs &&
		    xyc_error(xycsc, iorq, iopb, comm) == XY_ERR_AOK)
			continue;	/* AOK: we resubmitted it */


		/* this iorq is now done (hasn't been restarted or anything) */

		if ((iorq->mode & XY_MODE_VERBO) && iorq->lasterror)
			xyc_perror(iorq, iopb, 0);

		/* now, if read/write check to make sure we got all the data
		 * we needed. (this may not be the case if we got an error in
		 * the middle of a multisector request).   */

		if ((iorq->mode & XY_MODE_B144) != 0 && errs == 0 &&
		    (comm == XYCMD_RD || comm == XYCMD_WR)) {
			/* we just successfully processed a bad144 sector
			 * note: if we are in bad 144 mode, the pointers have
			 * been advanced already (see above) and are pointing
			 * at the bad144 sector.   to exit bad144 mode, we
			 * must advance the pointers 1 sector and issue a new
			 * request if there are still sectors left to process
			 *
			 */
			XYC_ADVANCE(iorq, 1);	/* advance 1 sector */

			/* exit b144 mode */
			iorq->mode = iorq->mode & (~XY_MODE_B144);

			if (iorq->sectcnt) {	/* more to go! */
				iorq->lasterror = iorq->errno = iopb->errno = 0;
				iopb->errs = iopb->done = 0;
				iorq->tries = 0;
				iopb->scnt = iorq->sectcnt;
				iopb->cyl = iorq->blockno /
						iorq->xy->sectpercyl;
				iopb->head =
					(iorq->blockno / iorq->xy->nhead) %
						iorq->xy->nhead;
				iopb->sect = iorq->blockno % XYFM_BPS;
				addr = (u_long) iorq->dbuf;
				iopb->dataa = (addr & 0xffff);
				iopb->datar = ((addr & 0xff0000) >> 16);
				/* will resubit at end */
				continue;
			}
		}
		/* final cleanup, totally done with this request */

		switch (XY_STATE(iorq->mode)) {
		case XY_SUB_NORM:
			bp = iorq->buf;
			if (errs) {
				bp->b_error = EIO;
				bp->b_flags |= B_ERROR;
				bp->b_resid = iorq->sectcnt * XYFM_BPS;
			} else {
				bp->b_resid = 0;	/* done */
			}
			bus_dmamap_sync(xycsc->dmatag, iorq->dmamap, 0,
					iorq->dmamap->dm_mapsize,
					(iorq->buf->b_flags & B_READ)
						? BUS_DMASYNC_POSTREAD
						: BUS_DMASYNC_POSTWRITE);
                
			bus_dmamap_unload(xycsc->dmatag, iorq->dmamap);

			BUFQ_REMOVE(&iorq->xy->xyq, bp);
			disk_unbusy(&iorq->xy->sc_dk,
			    (bp->b_bcount - bp->b_resid));
			iorq->mode = XY_SUB_FREE;
			biodone(bp);
			break;
		case XY_SUB_WAIT:
			iorq->mode = XY_NEWSTATE(iorq->mode, XY_SUB_DONE);
			wakeup(iorq);
			break;
		case XY_SUB_POLL:
			iorq->mode = XY_NEWSTATE(iorq->mode, XY_SUB_DONE);
			break;
		}
	}

	return (XY_ERR_AOK);
}

/*
 * xyc_perror: print error.
 * - if still_trying is true: we got an error, retried and got a
 *   different error.  in that case lasterror is the old error,
 *   and errno is the new one.
 * - if still_trying is not true, then if we ever had an error it
 *   is in lasterror. also, if iorq->errno == 0, then we recovered
 *   from that error (otherwise iorq->errno == iorq->lasterror).
 */
void
xyc_perror(iorq, iopb, still_trying)
	struct xy_iorq *iorq;
	struct xy_iopb *iopb;
	int     still_trying;

{

	int     error = iorq->lasterror;

	printf("%s", (iorq->xy) ? iorq->xy->sc_dev.dv_xname
	    : iorq->xyc->sc_dev.dv_xname);
	if (iorq->buf)
		printf("%c: ", 'a' + DISKPART(iorq->buf->b_dev));
	if (iopb->com == XYCMD_RD || iopb->com == XYCMD_WR)
		printf("%s %d/%d/%d: ",
			(iopb->com == XYCMD_RD) ? "read" : "write",
			iopb->cyl, iopb->head, iopb->sect);
	printf("%s", xyc_e2str(error));

	if (still_trying)
		printf(" [still trying, new error=%s]", xyc_e2str(iorq->errno));
	else
		if (iorq->errno == 0)
			printf(" [recovered in %d tries]", iorq->tries);

	printf("\n");
}

/*
 * xyc_error: non-fatal error encountered... recover.
 * return AOK if resubmitted, return FAIL if this iopb is done
 */
int
xyc_error(xycsc, iorq, iopb, comm)
	struct xyc_softc *xycsc;
	struct xy_iorq *iorq;
	struct xy_iopb *iopb;
	int     comm;

{
	int     errno = iorq->errno;
	int     erract = xyc_entoact(errno);
	int     oldmode, advance;
#ifdef __sparc__
	int i;
#endif

	if (erract == XY_ERA_RSET) {	/* some errors require a reset */
		oldmode = iorq->mode;
		iorq->mode = XY_SUB_DONE | (~XY_SUB_MASK & oldmode);
		/* make xyc_start ignore us */
		xyc_reset(xycsc, 1, XY_RSET_NONE, errno, iorq->xy);
		iorq->mode = oldmode;
	}
	/* check for read/write to a sector in bad144 table if bad: redirect
	 * request to bad144 area */

	if ((comm == XYCMD_RD || comm == XYCMD_WR) &&
	    (iorq->mode & XY_MODE_B144) == 0) {
		advance = iorq->sectcnt - iopb->scnt;
		XYC_ADVANCE(iorq, advance);
#ifdef __sparc__
		if ((i = isbad(&iorq->xy->dkb, iorq->blockno / iorq->xy->sectpercyl,
			    (iorq->blockno / iorq->xy->nsect) % iorq->xy->nhead,
			    iorq->blockno % iorq->xy->nsect)) != -1) {
			iorq->mode |= XY_MODE_B144;	/* enter bad144 mode &
							 * redirect */
			iopb->errno = iopb->done = iopb->errs = 0;
			iopb->scnt = 1;
			iopb->cyl = (iorq->xy->ncyl + iorq->xy->acyl) - 2;
			/* second to last acyl */
			i = iorq->xy->sectpercyl - 1 - i;	/* follow bad144
								 * standard */
			iopb->head = i / iorq->xy->nhead;
			iopb->sect = i % iorq->xy->nhead;
			/* will resubmit when we come out of remove_iorq */
			return (XY_ERR_AOK);	/* recovered! */
		}
#endif
	}

	/*
	 * it isn't a bad144 sector, must be real error! see if we can retry
	 * it?
	 */
	if ((iorq->mode & XY_MODE_VERBO) && iorq->lasterror)
		xyc_perror(iorq, iopb, 1);	/* inform of error state
						 * change */
	iorq->lasterror = errno;

	if ((erract == XY_ERA_RSET || erract == XY_ERA_HARD)
	    && iorq->tries < XYC_MAXTRIES) {	/* retry? */
		iorq->tries++;
		iorq->errno = iopb->errno = iopb->done = iopb->errs = 0;
		/* will resubmit at end of remove_iorq */
		return (XY_ERR_AOK);	/* recovered! */
	}

	/* failed to recover from this error */
	return (XY_ERR_FAIL);
}

/*
 * xyc_tick: make sure xy is still alive and ticking (err, kicking).
 */
void
xyc_tick(arg)
	void   *arg;

{
	struct xyc_softc *xycsc = arg;
	int     lcv, s, reset = 0;

	/* reduce ttl for each request if one goes to zero, reset xyc */
	s = splbio();
	for (lcv = 0; lcv < XYC_MAXIOPB; lcv++) {
		if (xycsc->reqs[lcv].mode == 0 ||
		    XY_STATE(xycsc->reqs[lcv].mode) == XY_SUB_DONE)
			continue;
		xycsc->reqs[lcv].ttl--;
		if (xycsc->reqs[lcv].ttl == 0)
			reset = 1;
	}
	if (reset) {
		printf("%s: watchdog timeout\n", xycsc->sc_dev.dv_xname);
		xyc_reset(xycsc, 0, XY_RSET_NONE, XY_ERR_FAIL, NULL);
	}
	splx(s);

	/* until next time */

	callout_reset(&xycsc->sc_tick_ch, XYC_TICKCNT, xyc_tick, xycsc);
}

/*
 * xyc_ioctlcmd: this function provides a user level interface to the
 * controller via ioctl.   this allows "format" programs to be written
 * in user code, and is also useful for some debugging.   we return
 * an error code.   called at user priority.
 *
 * XXX missing a few commands (see the 7053 driver for ideas)
 */
int
xyc_ioctlcmd(xy, dev, xio)
	struct xy_softc *xy;
	dev_t   dev;
	struct xd_iocmd *xio;

{
	int     s, rqno, dummy = 0;
	caddr_t dvmabuf = NULL, buf = NULL;
	struct xyc_softc *xycsc;
	int			rseg, error;
	bus_dma_segment_t	seg;

	/* check sanity of requested command */

	switch (xio->cmd) {

	case XYCMD_NOP:	/* no op: everything should be zero */
		if (xio->subfn || xio->dptr || xio->dlen ||
		    xio->block || xio->sectcnt)
			return (EINVAL);
		break;

	case XYCMD_RD:		/* read / write sectors (up to XD_IOCMD_MAXS) */
	case XYCMD_WR:
		if (xio->subfn || xio->sectcnt > XD_IOCMD_MAXS ||
		    xio->sectcnt * XYFM_BPS != xio->dlen || xio->dptr == NULL)
			return (EINVAL);
		break;

	case XYCMD_SK:		/* seek: doesn't seem useful to export this */
		return (EINVAL);

		break;

	default:
		return (EINVAL);/* ??? */
	}

	xycsc = xy->parent;

	/* create DVMA buffer for request if needed */
	if (xio->dlen) {
		if ((error = xy_dmamem_alloc(xycsc->dmatag, xycsc->auxmap,
					     &seg, &rseg,
					     xio->dlen, &buf,
					     (bus_addr_t *)&dvmabuf)) != 0) {
			return (error);
		}

		if (xio->cmd == XYCMD_WR) {
			if ((error = copyin(xio->dptr, buf, xio->dlen)) != 0) {
				bus_dmamem_unmap(xycsc->dmatag, buf, xio->dlen);
				bus_dmamem_free(xycsc->dmatag, &seg, rseg);
				return (error);
			}
		}
	}
	/* do it! */

	error = 0;
	s = splbio();
	rqno = xyc_cmd(xycsc, xio->cmd, xio->subfn, xy->xy_drive, xio->block,
	    xio->sectcnt, dvmabuf, XY_SUB_WAIT);
	if (rqno == XY_ERR_FAIL) {
		error = EIO;
		goto done;
	}
	xio->errno = xycsc->ciorq->errno;
	xio->tries = xycsc->ciorq->tries;
	XYC_DONE(xycsc, dummy);

	if (xio->cmd == XYCMD_RD)
		error = copyout(buf, xio->dptr, xio->dlen);

done:
	splx(s);
	if (dvmabuf) {
		xy_dmamem_free(xycsc->dmatag, xycsc->auxmap, &seg, rseg,
				xio->dlen, buf);
	}
	return (error);
}

/*
 * xyc_e2str: convert error code number into an error string
 */
char *
xyc_e2str(no)
	int     no;
{
	switch (no) {
	case XY_ERR_FAIL:
		return ("Software fatal error");
	case XY_ERR_DERR:
		return ("DOUBLE ERROR");
	case XY_ERR_AOK:
		return ("Successful completion");
	case XY_ERR_IPEN:
		return("Interrupt pending");
	case XY_ERR_BCFL:
		return("Busy conflict");
	case XY_ERR_TIMO:
		return("Operation timeout");
	case XY_ERR_NHDR:
		return("Header not found");
	case XY_ERR_HARD:
		return("Hard ECC error");
	case XY_ERR_ICYL:
		return("Illegal cylinder address");
	case XY_ERR_ISEC:
		return("Illegal sector address");
	case XY_ERR_SMAL:
		return("Last sector too small");
	case XY_ERR_SACK:
		return("Slave ACK error (non-existent memory)");
	case XY_ERR_CHER:
		return("Cylinder and head/header error");
	case XY_ERR_SRTR:
		return("Auto-seek retry successful");
	case XY_ERR_WPRO:
		return("Write-protect error");
	case XY_ERR_UIMP:
		return("Unimplemented command");
	case XY_ERR_DNRY:
		return("Drive not ready");
	case XY_ERR_SZER:
		return("Sector count zero");
	case XY_ERR_DFLT:
		return("Drive faulted");
	case XY_ERR_ISSZ:
		return("Illegal sector size");
	case XY_ERR_SLTA:
		return("Self test A");
	case XY_ERR_SLTB:
		return("Self test B");
	case XY_ERR_SLTC:
		return("Self test C");
	case XY_ERR_SOFT:
		return("Soft ECC error");
	case XY_ERR_SFOK:
		return("Soft ECC error recovered");
	case XY_ERR_IHED:
		return("Illegal head");
	case XY_ERR_DSEQ:
		return("Disk sequencer error");
	case XY_ERR_SEEK:
		return("Seek error");
	default:
		return ("Unknown error");
	}
}

int
xyc_entoact(errno)

int errno;

{
  switch (errno) {
    case XY_ERR_FAIL:	case XY_ERR_DERR:	case XY_ERR_IPEN:
    case XY_ERR_BCFL:	case XY_ERR_ICYL:	case XY_ERR_ISEC:
    case XY_ERR_UIMP:	case XY_ERR_SZER:	case XY_ERR_ISSZ:
    case XY_ERR_SLTA:	case XY_ERR_SLTB:	case XY_ERR_SLTC:
    case XY_ERR_IHED:	case XY_ERR_SACK:	case XY_ERR_SMAL:

	return(XY_ERA_PROG); /* program error ! */

    case XY_ERR_TIMO:	case XY_ERR_NHDR:	case XY_ERR_HARD:
    case XY_ERR_DNRY:	case XY_ERR_CHER:	case XY_ERR_SEEK:
    case XY_ERR_SOFT:

	return(XY_ERA_HARD); /* hard error, retry */

    case XY_ERR_DFLT:	case XY_ERR_DSEQ:

	return(XY_ERA_RSET); /* hard error reset */

    case XY_ERR_SRTR:	case XY_ERR_SFOK:	case XY_ERR_AOK:

	return(XY_ERA_SOFT); /* an FYI error */

    case XY_ERR_WPRO:

	return(XY_ERA_WPRO); /* write protect */
  }

  return(XY_ERA_PROG); /* ??? */
}
