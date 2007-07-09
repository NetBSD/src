/*	$NetBSD: fd.c,v 1.76 2007/07/09 20:52:35 ad Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and Minoura Makoto.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Don Ahn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)fd.c	7.4 (Berkeley) 5/25/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fd.c,v 1.76 2007/07/09 20:52:35 ad Exp $");

#include "rnd.h"
#include "opt_ddb.h"
#include "opt_m680x0.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/uio.h>
#include <sys/syslog.h>
#include <sys/queue.h>
#include <sys/fdio.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/dmacvar.h>
#include <arch/x68k/dev/fdreg.h>
#include <arch/x68k/dev/opmvar.h> /* for CT1 access */

#include "locators.h"

#ifdef FDDEBUG
#define DPRINTF(x)      if (fddebug) printf x
int     fddebug = 0;
#else
#define DPRINTF(x)
#endif

#define FDUNIT(dev)	(minor(dev) / 8)
#define FDTYPE(dev)	(minor(dev) % 8)

enum fdc_state {
	DEVIDLE = 0,
	MOTORWAIT,
	DOSEEK,
	SEEKWAIT,
	SEEKTIMEDOUT,
	SEEKCOMPLETE,
	DOIO,
	IOCOMPLETE,
	IOTIMEDOUT,
	DORESET,
	RESETCOMPLETE,
	RESETTIMEDOUT,
	DORECAL,
	RECALWAIT,
	RECALTIMEDOUT,
	RECALCOMPLETE,
	DOCOPY,
	DOIOHALF,
	COPYCOMPLETE,
};

/* software state, per controller */
struct fdc_softc {
	struct device sc_dev;		/* boilerplate */

	bus_space_tag_t sc_iot;		/* intio i/o space identifier */
	bus_space_handle_t sc_ioh;	/* intio io handle */

	struct callout sc_timo_ch;	/* timeout callout */
	struct callout sc_intr_ch;	/* pseudo-intr callout */

	bus_dma_tag_t sc_dmat;		/* intio DMA tag */
	bus_dmamap_t sc_dmamap;		/* DMA map */
	u_int8_t *sc_addr;			/* physical address */
	struct dmac_channel_stat *sc_dmachan; /* intio DMA channel */
	struct dmac_dma_xfer *sc_xfer;	/* DMA transfer */

	struct fd_softc *sc_fd[4];	/* pointers to children */
	TAILQ_HEAD(drivehead, fd_softc) sc_drives;
	enum fdc_state sc_state;
	int sc_errors;			/* number of retries so far */
	u_char sc_status[7];		/* copy of registers */
} fdc_softc;

int fdcintr(void *);
void fdcreset(struct fdc_softc *);

/* controller driver configuration */
int fdcprobe(struct device *, struct cfdata *, void *);
void fdcattach(struct device *, struct device *, void *);
int fdprint(void *, const char *);

CFATTACH_DECL(fdc, sizeof(struct fdc_softc),
    fdcprobe, fdcattach, NULL, NULL);

extern struct cfdriver fdc_cd;

/*
 * Floppies come in various flavors, e.g., 1.2MB vs 1.44MB; here is how
 * we tell them apart.
 */
struct fd_type {
	int	sectrac;	/* sectors per track */
	int	heads;		/* number of heads */
	int	seccyl;		/* sectors per cylinder */
	int	secsize;	/* size code for sectors */
	int	datalen;	/* data len when secsize = 0 */
	int	steprate;	/* step rate and head unload time */
	int	gap1;		/* gap len between sectors */
	int	gap2;		/* formatting gap */
	int	cyls;		/* total num of cylinders */
	int	size;		/* size of disk in sectors */
	int	step;		/* steps per cylinder */
	int	rate;		/* transfer speed code */
	const char *name;
};

/* The order of entries in the following table is important -- BEWARE! */
struct fd_type fd_types[] = {
        {  8,2,16,3,0xff,0xdf,0x35,0x74,77,1232,1,FDC_500KBPS, "1.2MB/[1024bytes/sector]"    }, /* 1.2 MB japanese format */
        { 18,2,36,2,0xff,0xcf,0x1b,0x6c,80,2880,1,FDC_500KBPS,"1.44MB"    }, /* 1.44MB diskette */
        { 15,2,30,2,0xff,0xdf,0x1b,0x54,80,2400,1,FDC_500KBPS, "1.2MB"    }, /* 1.2 MB AT-diskettes */
        {  9,2,18,2,0xff,0xdf,0x23,0x50,40, 720,2,FDC_300KBPS, "360KB/AT" }, /* 360kB in 1.2MB drive */
        {  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,1,FDC_250KBPS, "360KB/PC" }, /* 360kB PC diskettes */
        {  9,2,18,2,0xff,0xdf,0x2a,0x50,80,1440,1,FDC_250KBPS, "720KB"    }, /* 3.5" 720kB diskette */
        {  9,2,18,2,0xff,0xdf,0x23,0x50,80,1440,1,FDC_300KBPS, "720KB/x"  }, /* 720kB in 1.2MB drive */
        {  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,2,FDC_250KBPS, "360KB/x"  }, /* 360kB in 720kB drive */
};

/* software state, per disk (with up to 4 disks per ctlr) */
struct fd_softc {
	struct device sc_dev;
	struct disk sc_dk;

	struct fd_type *sc_deftype;	/* default type descriptor */
	struct fd_type *sc_type;	/* current type descriptor */

	struct callout sc_motoron_ch;
	struct callout sc_motoroff_ch;

	daddr_t	sc_blkno;	/* starting block number */
	int sc_bcount;		/* byte count left */
	int sc_opts;		/* user-set options */
	int sc_skip;		/* bytes already transferred */
	int sc_nblks;		/* number of blocks currently transferring */
	int sc_nbytes;		/* number of bytes currently transferring */

	int sc_drive;		/* physical unit number */
	int sc_flags;
#define	FD_BOPEN	0x01		/* it's open */
#define	FD_COPEN	0x02		/* it's open */
#define	FD_OPEN		(FD_BOPEN|FD_COPEN)	/* it's open */
#define	FD_MOTOR	0x04		/* motor should be on */
#define	FD_MOTOR_WAIT	0x08		/* motor coming up */
#define	FD_ALIVE	0x10		/* alive */
	int sc_cylin;		/* where we think the head is */

	TAILQ_ENTRY(fd_softc) sc_drivechain;
	int sc_ops;		/* I/O ops since last switch */
	struct bufq_state *sc_q;/* pending I/O requests */
	int sc_active;		/* number of active I/O operations */
	u_char *sc_copybuf;	/* for secsize >=3 */
	u_char sc_part;		/* for secsize >=3 */
#define	SEC_P10	0x02		/* first part */
#define	SEC_P01	0x01		/* second part */
#define	SEC_P11	0x03		/* both part */

#if NRND > 0
	rndsource_element_t	rnd_source;
#endif
};

/* floppy driver configuration */
int fdprobe(struct device *, struct cfdata *, void *);
void fdattach(struct device *, struct device *, void *);

CFATTACH_DECL(fd, sizeof(struct fd_softc),
    fdprobe, fdattach, NULL, NULL);

extern struct cfdriver fd_cd;

dev_type_open(fdopen);
dev_type_close(fdclose);
dev_type_read(fdread);
dev_type_write(fdwrite);
dev_type_ioctl(fdioctl);
dev_type_strategy(fdstrategy);

const struct bdevsw fd_bdevsw = {
	fdopen, fdclose, fdstrategy, fdioctl, nodump, nosize, D_DISK
};

const struct cdevsw fd_cdevsw = {
	fdopen, fdclose, fdread, fdwrite, fdioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

void fdstart(struct fd_softc *);

struct dkdriver fddkdriver = { fdstrategy };

void fd_set_motor(struct fdc_softc *, int);
void fd_motor_off(void *);
void fd_motor_on(void *);
int fdcresult(struct fdc_softc *);
int out_fdc(bus_space_tag_t, bus_space_handle_t, u_char);
void fdcstart(struct fdc_softc *);
void fdcstatus(struct device *, int, const char *);
void fdctimeout(void *);
void fdcpseudointr(void *);
void fdcretry(struct fdc_softc *);
void fdfinish(struct fd_softc *, struct buf *);
inline struct fd_type *fd_dev_to_type(struct fd_softc *, dev_t);
static int fdcpoll(struct fdc_softc *);
static int fdgetdisklabel(struct fd_softc *, dev_t);
static void fd_do_eject(struct fdc_softc *, int);

void fd_mountroot_hook(struct device *);

/* DMA transfer routines */
inline static void fdc_dmastart(struct fdc_softc *, int, void *, vsize_t);
static int fdcdmaintr(void *);
static int fdcdmaerrintr(void *);

inline static void
fdc_dmastart(struct fdc_softc *fdc, int read, void *addr, vsize_t count)
{
	int error;

	DPRINTF(("fdc_dmastart: %s, addr = %p, count = %ld\n",
		 read ? "read" : "write", (void *) addr, count));

	error = bus_dmamap_load(fdc->sc_dmat, fdc->sc_dmamap, addr, count,
				0, BUS_DMA_NOWAIT);
	if (error) {
		panic ("fdc_dmastart: cannot load dmamap");
	}

	bus_dmamap_sync(fdc->sc_dmat, fdc->sc_dmamap, 0, count,
			read?BUS_DMASYNC_PREREAD:BUS_DMASYNC_PREWRITE);

	fdc->sc_xfer = dmac_prepare_xfer(fdc->sc_dmachan, fdc->sc_dmat,
					 fdc->sc_dmamap,
					 (read?
					  DMAC_OCR_DIR_DTM:DMAC_OCR_DIR_MTD),
					 (DMAC_SCR_MAC_COUNT_UP|
					  DMAC_SCR_DAC_NO_COUNT),
					 (u_int8_t*) (fdc->sc_addr +
						      fddata));	/* XXX */

	dmac_start_xfer(fdc->sc_dmachan->ch_softc, fdc->sc_xfer);
}

static int
fdcdmaintr(void *arg)
{
	struct fdc_softc *fdc = arg;

	bus_dmamap_unload(fdc->sc_dmat, fdc->sc_dmamap);

	return 0;
}

static int
fdcdmaerrintr(void *dummy)
{
	DPRINTF(("fdcdmaerrintr\n"));

	return 0;
}

/* ARGSUSED */
int
fdcprobe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct intio_attach_args *ia = aux;

	if (strcmp(ia->ia_name, "fdc") != 0)
		return 0;

	if (ia->ia_addr == INTIOCF_ADDR_DEFAULT)
		ia->ia_addr = FDC_ADDR;
	if (ia->ia_intr == INTIOCF_INTR_DEFAULT)
		ia->ia_intr = FDC_INTR;
	if (ia->ia_dma == INTIOCF_DMA_DEFAULT)
		ia->ia_dma = FDC_DMA;
	if (ia->ia_dmaintr == INTIOCF_DMAINTR_DEFAULT)
		ia->ia_dmaintr = FDC_DMAINTR;

	if ((ia->ia_intr & 0x03) != 0)
		return 0;

	ia->ia_size = 0x2000;
	if (intio_map_allocate_region (parent, ia, INTIO_MAP_TESTONLY))
		return 0;

	/* builtin device; always there */
	return 1;
}

/*
 * Arguments passed between fdcattach and fdprobe.
 */
struct fdc_attach_args {
	int fa_drive;
	struct fd_type *fa_deftype;
};

/*
 * Print the location of a disk drive (called just before attaching the
 * the drive).  If `fdc' is not NULL, the drive was found but was not
 * in the system config file; print the drive name as well.
 * Return QUIET (config_find ignores this if the device was configured) to
 * avoid printing `fdN not configured' messages.
 */
int
fdprint(void *aux, const char *fdc)
{
	struct fdc_attach_args *fa = aux;

	if (!fdc)
		aprint_normal(" drive %d", fa->fa_drive);
	return QUIET;
}

void
fdcattach(struct device *parent, struct device *self, void *aux)
{
	struct fdc_softc *fdc = (void *)self;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct intio_attach_args *ia = aux;
	struct fdc_attach_args fa;

	iot = ia->ia_bst;

	printf("\n");

	callout_init(&fdc->sc_timo_ch, 0);
	callout_init(&fdc->sc_intr_ch, 0);

	/* Re-map the I/O space. */
	bus_space_map(iot, ia->ia_addr, 0x2000, BUS_SPACE_MAP_SHIFTED, &ioh);

	fdc->sc_iot = iot;
	fdc->sc_ioh = ioh;
	fdc->sc_addr = (void *)ia->ia_addr;

	fdc->sc_dmat = ia->ia_dmat;
	fdc->sc_state = DEVIDLE;
	TAILQ_INIT(&fdc->sc_drives);

	/* Initialize DMAC channel */
	fdc->sc_dmachan = dmac_alloc_channel(parent, ia->ia_dma, "fdc",
					     ia->ia_dmaintr, fdcdmaintr, fdc,
					     ia->ia_dmaintr+1, fdcdmaerrintr,
					     fdc);
	if (bus_dmamap_create(fdc->sc_dmat, FDC_MAXIOSIZE, 1, DMAC_MAXSEGSZ,
			      0, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW,
			      &fdc->sc_dmamap)) {
		printf("%s: can't set up intio DMA map\n",
		    fdc->sc_dev.dv_xname);
		return;
	}

	if (intio_intr_establish(ia->ia_intr, "fdc", fdcintr, fdc))
		panic ("Could not establish interrupt (duplicated vector?).");
	intio_set_ivec(ia->ia_intr);

	/* reset */
	intio_disable_intr(SICILIAN_INTR_FDD);
	intio_enable_intr(SICILIAN_INTR_FDC);
	fdcresult(fdc);
	fdcreset(fdc);

	printf("%s: uPD72065 FDC\n", fdc->sc_dev.dv_xname);
	out_fdc(iot, ioh, NE7CMD_SPECIFY);/* specify command */
	out_fdc(iot, ioh, 0xd0);
	out_fdc(iot, ioh, 0x10);

	/* physical limit: four drives per controller. */
	for (fa.fa_drive = 0; fa.fa_drive < 4; fa.fa_drive++) {
		(void)config_found(self, (void *)&fa, fdprint);
	}

	intio_enable_intr(SICILIAN_INTR_FDC);
}

void
fdcreset(struct fdc_softc *fdc)
{
	bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdsts, NE7CMD_RESET);
}

static int
fdcpoll(struct fdc_softc *fdc)
{
	int i = 25000, n;
	while (--i > 0) {
		if ((intio_get_sicilian_intr() & SICILIAN_STAT_FDC)) {
			out_fdc(fdc->sc_iot, fdc->sc_ioh, NE7CMD_SENSEI);
			n = fdcresult(fdc);
			break;
		}
		DELAY(100);
	}
	return i;
}

int
fdprobe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct fdc_softc *fdc = (void *)parent;
	struct fd_type *type;
	struct fdc_attach_args *fa = aux;
	int drive = fa->fa_drive;
	bus_space_tag_t iot = fdc->sc_iot;
	bus_space_handle_t ioh = fdc->sc_ioh;
	int n = 0;
	int found = 0;
	int i;

	if (cf->cf_loc[FDCCF_UNIT] != FDCCF_UNIT_DEFAULT &&
	    cf->cf_loc[FDCCF_UNIT] != drive)
		return 0;

	type = &fd_types[0];	/* XXX 1.2MB */

	intio_disable_intr(SICILIAN_INTR_FDC);

	/* select drive and turn on motor */
	bus_space_write_1(iot, ioh, fdctl, 0x80 | (type->rate << 4)| drive);
	fdc_force_ready(FDCRDY);
	fdcpoll(fdc);

retry:
	out_fdc(iot, ioh, NE7CMD_RECAL);
	out_fdc(iot, ioh, drive);

	i = 25000;
	while (--i > 0) {
		if ((intio_get_sicilian_intr() & SICILIAN_STAT_FDC)) {
			out_fdc(iot, ioh, NE7CMD_SENSEI);
			n = fdcresult(fdc);
			break;
		}
		DELAY(100);
	}

#ifdef FDDEBUG
	{
		int _i;
		DPRINTF(("fdprobe: status"));
		for (_i = 0; _i < n; _i++)
			DPRINTF((" %x", fdc->sc_status[_i]));
		DPRINTF(("\n"));
	}
#endif

	if (n == 2) {
		if ((fdc->sc_status[0] & 0xf0) == 0x20)
			found = 1;
		else if ((fdc->sc_status[0] & 0xf0) == 0xc0)
			goto retry;
	}

	/* turn off motor */
	bus_space_write_1(fdc->sc_iot, fdc->sc_ioh,
			  fdctl, (type->rate << 4)| drive);
	fdc_force_ready(FDCSTBY);
	if (!found) {
		intio_enable_intr(SICILIAN_INTR_FDC);
		return 0;
	}

	return 1;
}

/*
 * Controller is working, and drive responded.  Attach it.
 */
void
fdattach(struct device *parent, struct device *self, void *aux)
{
	struct fdc_softc *fdc = (void *)parent;
	struct fd_softc *fd = (void *)self;
	struct fdc_attach_args *fa = aux;
	struct fd_type *type = &fd_types[0];	/* XXX 1.2MB */
	int drive = fa->fa_drive;

	callout_init(&fd->sc_motoron_ch, 0);
	callout_init(&fd->sc_motoroff_ch, 0);

	fd->sc_flags = 0;

	if (type)
		printf(": %s, %d cyl, %d head, %d sec\n", type->name,
		       type->cyls, type->heads, type->sectrac);
	else
		printf(": density unknown\n");

	bufq_alloc(&fd->sc_q, "disksort", BUFQ_SORT_CYLINDER);
	fd->sc_cylin = -1;
	fd->sc_drive = drive;
	fd->sc_deftype = type;
	fdc->sc_fd[drive] = fd;

	fd->sc_copybuf = (u_char *)malloc(PAGE_SIZE, M_DEVBUF, M_WAITOK);
	if (fd->sc_copybuf == 0)
		printf("fdprobe: WARNING!! malloc() failed.\n");
	fd->sc_flags |= FD_ALIVE;

	/*
	 * Initialize and attach the disk structure.
	 */
	fd->sc_dk.dk_name = fd->sc_dev.dv_xname;
	fd->sc_dk.dk_driver = &fddkdriver;
	disk_attach(&fd->sc_dk);

	/*
	 * Establish a mountroot_hook anyway in case we booted
	 * with RB_ASKNAME and get selected as the boot device.
	 */
	mountroothook_establish(fd_mountroot_hook, &fd->sc_dev);

#if NRND > 0
	rnd_attach_source(&fd->rnd_source, fd->sc_dev.dv_xname,
			  RND_TYPE_DISK, 0);
#endif
}

inline struct fd_type *
fd_dev_to_type(struct fd_softc *fd, dev_t dev)
{
	int type = FDTYPE(dev);

	if (type > (sizeof(fd_types) / sizeof(fd_types[0])))
		return NULL;
	return &fd_types[type];
}

void
fdstrategy(struct buf *bp)
{
	struct fd_softc *fd;
	int unit = FDUNIT(bp->b_dev);
	int sz;
	int s;

	if (unit >= fd_cd.cd_ndevs ||
	    (fd = fd_cd.cd_devs[unit]) == 0 ||
	    bp->b_blkno < 0 ||
	    (bp->b_bcount % FDC_BSIZE) != 0) {
		DPRINTF(("fdstrategy: unit=%d, blkno=%" PRId64 ", "
			 "bcount=%d\n", unit,
			 bp->b_blkno, bp->b_bcount));
		bp->b_error = EINVAL;
		goto bad;
	}

	/* If it's a null transfer, return immediately. */
	if (bp->b_bcount == 0)
		goto done;

	sz = howmany(bp->b_bcount, FDC_BSIZE);

	if (bp->b_blkno + sz >
	    (fd->sc_type->size << (fd->sc_type->secsize - 2))) {
		sz = (fd->sc_type->size << (fd->sc_type->secsize - 2))
		     - bp->b_blkno;
		if (sz == 0) {
			/* If exactly at end of disk, return EOF. */
			bp->b_resid = bp->b_bcount;
			goto done;
		}
		if (sz < 0) {
			/* If past end of disk, return EINVAL. */
			bp->b_error = EINVAL;
			goto bad;
		}
		/* Otherwise, truncate request. */
		bp->b_bcount = sz << DEV_BSHIFT;
	}

	bp->b_rawblkno = bp->b_blkno;
	bp->b_cylinder = bp->b_blkno / (FDC_BSIZE / DEV_BSIZE)
		/ (fd->sc_type->seccyl * (1 << (fd->sc_type->secsize - 2)));

	DPRINTF(("fdstrategy: %s b_blkno %" PRId64 " b_bcount %d cylin %d\n",
		 bp->b_flags & B_READ ? "read" : "write",
		 bp->b_blkno, bp->b_bcount, bp->b_cylinder));
	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();
	BUFQ_PUT(fd->sc_q, bp);
	callout_stop(&fd->sc_motoroff_ch);		/* a good idea */
	if (fd->sc_active == 0)
		fdstart(fd);
#ifdef DIAGNOSTIC
	else {
		struct fdc_softc *fdc = (void *)device_parent(&fd->sc_dev);
		if (fdc->sc_state == DEVIDLE) {
			printf("fdstrategy: controller inactive\n");
			fdcstart(fdc);
		}
	}
#endif
	splx(s);
	return;

bad:
	bp->b_flags |= B_ERROR;
done:
	/* Toss transfer; we're done early. */
	biodone(bp);
}

void
fdstart(struct fd_softc *fd)
{
	struct fdc_softc *fdc = (void *)device_parent(&fd->sc_dev);
	int active = fdc->sc_drives.tqh_first != 0;

	/* Link into controller queue. */
	fd->sc_active = 1;
	TAILQ_INSERT_TAIL(&fdc->sc_drives, fd, sc_drivechain);

	/* If controller not already active, start it. */
	if (!active)
		fdcstart(fdc);
}

void
fdfinish(struct fd_softc *fd, struct buf *bp)
{
	struct fdc_softc *fdc = (void *)device_parent(&fd->sc_dev);

	/*
	 * Move this drive to the end of the queue to give others a `fair'
	 * chance.  We only force a switch if N operations are completed while
	 * another drive is waiting to be serviced, since there is a long motor
	 * startup delay whenever we switch.
	 */
	(void)BUFQ_GET(fd->sc_q);
	if (fd->sc_drivechain.tqe_next && ++fd->sc_ops >= 8) {
		fd->sc_ops = 0;
		TAILQ_REMOVE(&fdc->sc_drives, fd, sc_drivechain);
		if (BUFQ_PEEK(fd->sc_q) != NULL) {
			TAILQ_INSERT_TAIL(&fdc->sc_drives, fd, sc_drivechain);
		} else
			fd->sc_active = 0;
	}
	bp->b_resid = fd->sc_bcount;
	fd->sc_skip = 0;

#if NRND > 0
	rnd_add_uint32(&fd->rnd_source, bp->b_blkno);
#endif

	biodone(bp);
	/* turn off motor 5s from now */
	callout_reset(&fd->sc_motoroff_ch, 5 * hz, fd_motor_off, fd);
	fdc->sc_state = DEVIDLE;
}

int
fdread(dev_t dev, struct uio *uio, int flags)
{

	return (physio(fdstrategy, NULL, dev, B_READ, minphys, uio));
}

int
fdwrite(dev_t dev, struct uio *uio, int flags)
{

	return (physio(fdstrategy, NULL, dev, B_WRITE, minphys, uio));
}

void
fd_set_motor(struct fdc_softc *fdc, int reset)
{
	struct fd_softc *fd;
	int n;

	DPRINTF(("fd_set_motor:\n"));
	for (n = 0; n < 4; n++)
		if ((fd = fdc->sc_fd[n]) && (fd->sc_flags & FD_MOTOR)) {
			bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdctl,
					  0x80 | (fd->sc_type->rate << 4)| n);
		}
}

void
fd_motor_off(void *arg)
{
	struct fd_softc *fd = arg;
	struct fdc_softc *fdc = (struct fdc_softc*) device_parent(&fd->sc_dev);
	int s;

	DPRINTF(("fd_motor_off:\n"));

	s = splbio();
	fd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
	bus_space_write_1 (fdc->sc_iot, fdc->sc_ioh, fdctl,
			   (fd->sc_type->rate << 4) | fd->sc_drive);
#if 0
	fd_set_motor(fdc, 0); /* XXX */
#endif
	splx(s);
}

void
fd_motor_on(void *arg)
{
	struct fd_softc *fd = arg;
	struct fdc_softc *fdc = (void *)device_parent(&fd->sc_dev);
	int s;

	DPRINTF(("fd_motor_on:\n"));

	s = splbio();
	fd->sc_flags &= ~FD_MOTOR_WAIT;
	if ((fdc->sc_drives.tqh_first == fd) && (fdc->sc_state == MOTORWAIT))
		(void) fdcintr(fdc);
	splx(s);
}

int
fdcresult(struct fdc_softc *fdc)
{
	bus_space_tag_t iot = fdc->sc_iot;
	bus_space_handle_t ioh = fdc->sc_ioh;
	u_char i;
	int j = 100000,
	    n = 0;

	for (; j; j--) {
		i = bus_space_read_1(iot, ioh, fdsts) &
		  (NE7_DIO | NE7_RQM | NE7_CB);

		if (i == NE7_RQM)
			return n;
		if (i == (NE7_DIO | NE7_RQM | NE7_CB)) {
			if (n >= sizeof(fdc->sc_status)) {
				log(LOG_ERR, "fdcresult: overrun\n");
				return -1;
			}
			fdc->sc_status[n++] =
			  bus_space_read_1(iot, ioh, fddata);
		}
		delay(10);
	}
	log(LOG_ERR, "fdcresult: timeout\n");
	return -1;
}

int
out_fdc(bus_space_tag_t iot, bus_space_handle_t ioh, u_char x)
{
	int i = 100000;

	while ((bus_space_read_1(iot, ioh, fdsts) & NE7_DIO) && i-- > 0);
	if (i <= 0)
		return -1;
	while ((bus_space_read_1(iot, ioh, fdsts) & NE7_RQM) == 0 && i-- > 0);
	if (i <= 0)
		return -1;
	bus_space_write_1(iot, ioh, fddata, x);
	return 0;
}

int
fdopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit;
	struct fd_softc *fd;
	struct fd_type *type;
	struct fdc_softc *fdc;

	unit = FDUNIT(dev);
	if (unit >= fd_cd.cd_ndevs)
		return ENXIO;
	fd = fd_cd.cd_devs[unit];
	if (fd == 0)
		return ENXIO;
	type = fd_dev_to_type(fd, dev);
	if (type == NULL)
		return ENXIO;

	if ((fd->sc_flags & FD_OPEN) != 0 &&
	    fd->sc_type != type)
		return EBUSY;

	fdc = (void *)device_parent(&fd->sc_dev);
	if ((fd->sc_flags & FD_OPEN) == 0) {
		/* Lock eject button */
		bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdout,
				  0x40 | ( 1 << unit));
		bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdout, 0x40);
	}

	fd->sc_type = type;
	fd->sc_cylin = -1;

	switch (mode) {
	case S_IFCHR:
		fd->sc_flags |= FD_COPEN;
		break;
	case S_IFBLK:
		fd->sc_flags |= FD_BOPEN;
		break;
	}

	fdgetdisklabel(fd, dev);

	return 0;
}

int
fdclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = FDUNIT(dev);
	struct fd_softc *fd = fd_cd.cd_devs[unit];
	struct fdc_softc *fdc = (void *)device_parent(&fd->sc_dev);

	DPRINTF(("fdclose %d\n", unit));

	switch (mode) {
	case S_IFCHR:
		fd->sc_flags &= ~FD_COPEN;
		break;
	case S_IFBLK:
		fd->sc_flags &= ~FD_BOPEN;
		break;
	}

	if ((fd->sc_flags & FD_OPEN) == 0) {
		bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdout,
				  ( 1 << unit));
		bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdout, 0);
	}
	return 0;
}

void
fdcstart(struct fdc_softc *fdc)
{

#ifdef DIAGNOSTIC
	/* only got here if controller's drive queue was inactive; should
	   be in idle state */
	if (fdc->sc_state != DEVIDLE) {
		printf("fdcstart: not idle\n");
		return;
	}
#endif
	(void) fdcintr(fdc);
}

void
fdcstatus(struct device *dv, int n, const char *s)
{
	struct fdc_softc *fdc = (void *)device_parent(dv);
	char bits[64];

	if (n == 0) {
		out_fdc(fdc->sc_iot, fdc->sc_ioh, NE7CMD_SENSEI);
		(void) fdcresult(fdc);
		n = 2;
	}

	printf("%s: %s: state %d", dv->dv_xname, s, fdc->sc_state);

	switch (n) {
	case 0:
		printf("\n");
		break;
	case 2:
		printf(" (st0 %s cyl %d)\n",
		    bitmask_snprintf(fdc->sc_status[0], NE7_ST0BITS,
		    bits, sizeof(bits)), fdc->sc_status[1]);
		break;
	case 7:
		printf(" (st0 %s", bitmask_snprintf(fdc->sc_status[0],
		    NE7_ST0BITS, bits, sizeof(bits)));
		printf(" st1 %s", bitmask_snprintf(fdc->sc_status[1],
		    NE7_ST1BITS, bits, sizeof(bits)));
		printf(" st2 %s", bitmask_snprintf(fdc->sc_status[2],
		    NE7_ST2BITS, bits, sizeof(bits)));
		printf(" cyl %d head %d sec %d)\n",
		    fdc->sc_status[3], fdc->sc_status[4], fdc->sc_status[5]);
		break;
#ifdef DIAGNOSTIC
	default:
		printf(" fdcstatus: weird size: %d\n", n);
		break;
#endif
	}
}

void
fdctimeout(void *arg)
{
	struct fdc_softc *fdc = arg;
	struct fd_softc *fd = fdc->sc_drives.tqh_first;
	int s;

	s = splbio();
	fdcstatus(&fd->sc_dev, 0, "timeout");

	if (BUFQ_PEEK(fd->sc_q) != NULL)
		fdc->sc_state++;
	else
		fdc->sc_state = DEVIDLE;

	(void) fdcintr(fdc);
	splx(s);
}

#if 0
void
fdcpseudointr(void *arg)
{
	int s;
	struct fdc_softc *fdc = arg;

	/* just ensure it has the right spl */
	s = splbio();
	(void) fdcintr(fdc);
	splx(s);
}
#endif

int
fdcintr(void *arg)
{
	struct fdc_softc *fdc = arg;
#define	st0	fdc->sc_status[0]
#define	cyl	fdc->sc_status[1]
	struct fd_softc *fd;
	struct buf *bp;
	bus_space_tag_t iot = fdc->sc_iot;
	bus_space_handle_t ioh = fdc->sc_ioh;
	int read, head, sec, pos, i, sectrac, nblks;
	int	tmp;
	struct fd_type *type;

loop:
	fd = fdc->sc_drives.tqh_first;
	if (fd == NULL) {
		DPRINTF(("fdcintr: set DEVIDLE\n"));
		if (fdc->sc_state == DEVIDLE) {
			if (intio_get_sicilian_intr() & SICILIAN_STAT_FDC) {
				out_fdc(iot, ioh, NE7CMD_SENSEI);
				if ((tmp = fdcresult(fdc)) != 2 ||
				    (st0 & 0xf8) != 0x20) {
					goto loop;
				}
			}
		}
		/* no drives waiting; end */
		fdc->sc_state = DEVIDLE;
		return 1;
	}

	/* Is there a transfer to this drive?  If not, deactivate drive. */
	bp = BUFQ_PEEK(fd->sc_q);
	if (bp == NULL) {
		fd->sc_ops = 0;
		TAILQ_REMOVE(&fdc->sc_drives, fd, sc_drivechain);
		fd->sc_active = 0;
		goto loop;
	}

	switch (fdc->sc_state) {
	case DEVIDLE:
		DPRINTF(("fdcintr: in DEVIDLE\n"));
		fdc->sc_errors = 0;
		fd->sc_skip = 0;
		fd->sc_bcount = bp->b_bcount;
		fd->sc_blkno = bp->b_blkno / (FDC_BSIZE / DEV_BSIZE);
		callout_stop(&fd->sc_motoroff_ch);
		if ((fd->sc_flags & FD_MOTOR_WAIT) != 0) {
			fdc->sc_state = MOTORWAIT;
			return 1;
		}
		if ((fd->sc_flags & FD_MOTOR) == 0) {
			/* Turn on the motor */
			/* being careful about other drives. */
			for (i = 0; i < 4; i++) {
				struct fd_softc *ofd = fdc->sc_fd[i];
				if (ofd && ofd->sc_flags & FD_MOTOR) {
					callout_stop(&ofd->sc_motoroff_ch);
					ofd->sc_flags &=
						~(FD_MOTOR | FD_MOTOR_WAIT);
					break;
				}
			}
			fd->sc_flags |= FD_MOTOR | FD_MOTOR_WAIT;
			fd_set_motor(fdc, 0);
			fdc->sc_state = MOTORWAIT;
			/* allow .5s for motor to stabilize */
			callout_reset(&fd->sc_motoron_ch, hz / 2,
			    fd_motor_on, fd);
			return 1;
		}
		/* Make sure the right drive is selected. */
		fd_set_motor(fdc, 0);

		/* fall through */
	case DOSEEK:
	doseek:
		DPRINTF(("fdcintr: in DOSEEK\n"));
		if (fd->sc_cylin == bp->b_cylinder)
			goto doio;

		out_fdc(iot, ioh, NE7CMD_SPECIFY);/* specify command */
		out_fdc(iot, ioh, 0xd0);	/* XXX const */
		out_fdc(iot, ioh, 0x10);

		out_fdc(iot, ioh, NE7CMD_SEEK);	/* seek function */
		out_fdc(iot, ioh, fd->sc_drive);	/* drive number */
		out_fdc(iot, ioh, bp->b_cylinder * fd->sc_type->step);

		fd->sc_cylin = -1;
		fdc->sc_state = SEEKWAIT;

		iostat_seek(fd->sc_dk.dk_stats);
		disk_busy(&fd->sc_dk);

		callout_reset(&fdc->sc_timo_ch, 4 * hz, fdctimeout, fdc);
		return 1;

	case DOIO:
	doio:
		DPRINTF(("fdcintr: DOIO: "));
		type = fd->sc_type;
		sectrac = type->sectrac;
		pos = fd->sc_blkno % (sectrac * (1 << (type->secsize - 2)));
		sec = pos / (1 << (type->secsize - 2));
		if (type->secsize == 2) {
			fd->sc_part = SEC_P11;
			nblks = (sectrac - sec) << (type->secsize - 2);
			nblks = min(nblks, fd->sc_bcount / FDC_BSIZE);
			DPRINTF(("nblks(0)"));
		} else if ((fd->sc_blkno % 2) == 0) {
			if (fd->sc_bcount & 0x00000200) {
				if (fd->sc_bcount == FDC_BSIZE) {
					fd->sc_part = SEC_P10;
					nblks = 1;
					DPRINTF(("nblks(1)"));
				} else {
					fd->sc_part = SEC_P11;
					nblks = (sectrac - sec) * 2;
					nblks = min(nblks, fd->sc_bcount
						    / FDC_BSIZE - 1);
					DPRINTF(("nblks(2)"));
				}
			} else {
				fd->sc_part = SEC_P11;
				nblks = (sectrac - sec)
					<< (type->secsize - 2);
				nblks = min(nblks, fd->sc_bcount / FDC_BSIZE);
				DPRINTF(("nblks(3)"));
			}
		} else {
			fd->sc_part = SEC_P01;
			nblks = 1;
			DPRINTF(("nblks(4)"));
		}
		nblks = min(nblks, FDC_MAXIOSIZE / FDC_BSIZE);
		DPRINTF((" %d\n", nblks));
		fd->sc_nblks = nblks;
		fd->sc_nbytes = nblks * FDC_BSIZE;
		head = (fd->sc_blkno
			% (type->seccyl * (1 << (type->secsize - 2))))
			 / (type->sectrac * (1 << (type->secsize - 2)));

#ifdef DIAGNOSTIC
		{int block;
		 block = ((fd->sc_cylin * type->heads + head) * type->sectrac
			  + sec) * (1 << (type->secsize - 2));
		 block += (fd->sc_part == SEC_P01) ? 1 : 0;
		 if (block != fd->sc_blkno) {
			 printf("C H R N: %d %d %d %d\n",
				fd->sc_cylin, head, sec, type->secsize);
			 printf("fdcintr: doio: block %d != blkno %" PRId64 "\n",
				block, fd->sc_blkno);
#ifdef DDB
			 Debugger();
#endif
		 }
		}
#endif
		read = bp->b_flags & B_READ;
		DPRINTF(("fdcintr: %s drive %d track %d "
		         "head %d sec %d nblks %d, skip %d\n",
			 read ? "read" : "write", fd->sc_drive, fd->sc_cylin,
			 head, sec, nblks, fd->sc_skip));
		DPRINTF(("C H R N: %d %d %d %d\n", fd->sc_cylin, head, sec,
			 type->secsize));

		if (fd->sc_part != SEC_P11)
			goto docopy;

		fdc_dmastart(fdc, read, (char *)bp->b_data + fd->sc_skip,
			     fd->sc_nbytes);
		if (read)
			out_fdc(iot, ioh, NE7CMD_READ);	/* READ */
		else
			out_fdc(iot, ioh, NE7CMD_WRITE); /* WRITE */
		out_fdc(iot, ioh, (head << 2) | fd->sc_drive);
		out_fdc(iot, ioh, bp->b_cylinder);	/* cylinder */
		out_fdc(iot, ioh, head);
		out_fdc(iot, ioh, sec + 1);		/* sector +1 */
		out_fdc(iot, ioh, type->secsize);	/* sector size */
		out_fdc(iot, ioh, type->sectrac);	/* sectors/track */
		out_fdc(iot, ioh, type->gap1);		/* gap1 size */
		out_fdc(iot, ioh, type->datalen);	/* data length */
		fdc->sc_state = IOCOMPLETE;

		disk_busy(&fd->sc_dk);

		/* allow 2 seconds for operation */
		callout_reset(&fdc->sc_timo_ch, 2 * hz, fdctimeout, fdc);
		return 1;				/* will return later */

	case DOCOPY:
	docopy:
		DPRINTF(("fdcintr: DOCOPY:\n"));
		type = fd->sc_type;
		head = (fd->sc_blkno
			% (type->seccyl * (1 << (type->secsize - 2))))
			 / (type->sectrac * (1 << (type->secsize - 2)));
		pos = fd->sc_blkno % (type->sectrac * (1 << (type->secsize - 2)));
		sec = pos / (1 << (type->secsize - 2));
		fdc_dmastart(fdc, B_READ, fd->sc_copybuf, 1024);
		out_fdc(iot, ioh, NE7CMD_READ);		/* READ */
		out_fdc(iot, ioh, (head << 2) | fd->sc_drive);
		out_fdc(iot, ioh, bp->b_cylinder);	/* cylinder */
		out_fdc(iot, ioh, head);
		out_fdc(iot, ioh, sec + 1);		/* sector +1 */
		out_fdc(iot, ioh, type->secsize);	/* sector size */
		out_fdc(iot, ioh, type->sectrac);	/* sectors/track */
		out_fdc(iot, ioh, type->gap1);		/* gap1 size */
		out_fdc(iot, ioh, type->datalen);	/* data length */
		fdc->sc_state = COPYCOMPLETE;
		/* allow 2 seconds for operation */
		callout_reset(&fdc->sc_timo_ch, 2 * hz, fdctimeout, fdc);
		return 1;				/* will return later */

	case DOIOHALF:
	doiohalf:
		DPRINTF((" DOIOHALF:\n"));

		type = fd->sc_type;
		sectrac = type->sectrac;
		pos = fd->sc_blkno % (sectrac * (1 << (type->secsize - 2)));
		sec = pos / (1 << (type->secsize - 2));
		head = (fd->sc_blkno
			% (type->seccyl * (1 << (type->secsize - 2))))
			 / (type->sectrac * (1 << (type->secsize - 2)));
#ifdef DIAGNOSTIC
		{int block;
		 block = ((fd->sc_cylin * type->heads + head) *
			 type->sectrac + sec)
			 * (1 << (type->secsize - 2));
		 block += (fd->sc_part == SEC_P01) ? 1 : 0;
		 if (block != fd->sc_blkno) {
			 printf("fdcintr: block %d != blkno %" PRId64 "\n",
				block, fd->sc_blkno);
#ifdef DDB
			 Debugger();
#endif
		 }
		}
#endif
		if ((read = bp->b_flags & B_READ)) {
			memcpy((char *)bp->b_data + fd->sc_skip, fd->sc_copybuf
			    + (fd->sc_part & SEC_P01 ? FDC_BSIZE : 0),
			    FDC_BSIZE);
			fdc->sc_state = IOCOMPLETE;
			goto iocomplete2;
		} else {
			memcpy((char *)fd->sc_copybuf
			    + (fd->sc_part & SEC_P01 ? FDC_BSIZE : 0),
			    (char *)bp->b_data + fd->sc_skip, FDC_BSIZE);
			fdc_dmastart(fdc, read, fd->sc_copybuf, 1024);
		}
		out_fdc(iot, ioh, NE7CMD_WRITE);	/* WRITE */
		out_fdc(iot, ioh, (head << 2) | fd->sc_drive);
		out_fdc(iot, ioh, bp->b_cylinder);	/* cylinder */
		out_fdc(iot, ioh, head);
		out_fdc(iot, ioh, sec + 1);		/* sector +1 */
		out_fdc(iot, ioh, fd->sc_type->secsize); /* sector size */
		out_fdc(iot, ioh, sectrac);		/* sectors/track */
		out_fdc(iot, ioh, fd->sc_type->gap1);	/* gap1 size */
		out_fdc(iot, ioh, fd->sc_type->datalen); /* data length */
		fdc->sc_state = IOCOMPLETE;
		/* allow 2 seconds for operation */
		callout_reset(&fdc->sc_timo_ch, 2 * hz, fdctimeout, fdc);
		return 1;				/* will return later */

	case SEEKWAIT:
		callout_stop(&fdc->sc_timo_ch);
		fdc->sc_state = SEEKCOMPLETE;
		/* allow 1/50 second for heads to settle */
#if 0
		callout_reset(&fdc->sc_intr_ch, hz / 50, fdcpseudointr, fdc);
#endif
		return 1;

	case SEEKCOMPLETE:
		/* Make sure seek really happened */
		DPRINTF(("fdcintr: SEEKCOMPLETE: FDC status = %x\n",
			 bus_space_read_1(fdc->sc_iot, fdc->sc_ioh, fdsts)));
		out_fdc(iot, ioh, NE7CMD_SENSEI);
		tmp = fdcresult(fdc);
		if ((st0 & 0xf8) == 0xc0) {
			DPRINTF(("fdcintr: first seek!\n"));
			fdc->sc_state = DORECAL;
			goto loop;
		} else if (tmp != 2 ||
			   (st0 & 0xf8) != 0x20 ||
			   cyl != bp->b_cylinder) {
#ifdef FDDEBUG
			fdcstatus(&fd->sc_dev, 2, "seek failed");
#endif
			fdcretry(fdc);
			goto loop;
		}
		fd->sc_cylin = bp->b_cylinder;
		goto doio;

	case IOTIMEDOUT:
#if 0
		isa_dmaabort(fdc->sc_drq);
#endif
	case SEEKTIMEDOUT:
	case RECALTIMEDOUT:
	case RESETTIMEDOUT:
		fdcretry(fdc);
		goto loop;

	case IOCOMPLETE: /* IO DONE, post-analyze */
		callout_stop(&fdc->sc_timo_ch);
		DPRINTF(("fdcintr: in IOCOMPLETE\n"));
		if ((tmp = fdcresult(fdc)) != 7 || (st0 & 0xf8) != 0) {
			printf("fdcintr: resnum=%d, st0=%x\n", tmp, st0);
#if 0
			isa_dmaabort(fdc->sc_drq);
#endif
			fdcstatus(&fd->sc_dev, 7, bp->b_flags & B_READ ?
				  "read failed" : "write failed");
			printf("blkno %" PRId64 " nblks %d\n",
			    fd->sc_blkno, fd->sc_nblks);
			fdcretry(fdc);
			goto loop;
		}
#if 0
		isa_dmadone(bp->b_flags & B_READ, bp->b_data + fd->sc_skip,
		    nblks * FDC_BSIZE, fdc->sc_drq);
#endif
	iocomplete2:
		if (fdc->sc_errors) {
			diskerr(bp, "fd", "soft error (corrected)", LOG_PRINTF,
			    fd->sc_skip / FDC_BSIZE, (struct disklabel *)NULL);
			printf("\n");
			fdc->sc_errors = 0;
		}
		fd->sc_blkno += fd->sc_nblks;
		fd->sc_skip += fd->sc_nbytes;
		fd->sc_bcount -= fd->sc_nbytes;
		DPRINTF(("fd->sc_bcount = %d\n", fd->sc_bcount));
		if (fd->sc_bcount > 0) {
			bp->b_cylinder = fd->sc_blkno
				/ (fd->sc_type->seccyl
				   * (1 << (fd->sc_type->secsize - 2)));
			goto doseek;
		}
		fdfinish(fd, bp);
		goto loop;

	case COPYCOMPLETE: /* IO DONE, post-analyze */
		DPRINTF(("fdcintr: COPYCOMPLETE:"));
		callout_stop(&fdc->sc_timo_ch);
		if ((tmp = fdcresult(fdc)) != 7 || (st0 & 0xf8) != 0) {
			printf("fdcintr: resnum=%d, st0=%x\n", tmp, st0);
#if 0
			isa_dmaabort(fdc->sc_drq);
#endif
			fdcstatus(&fd->sc_dev, 7, bp->b_flags & B_READ ?
				  "read failed" : "write failed");
			printf("blkno %" PRId64 " nblks %d\n",
			    fd->sc_blkno, fd->sc_nblks);
			fdcretry(fdc);
			goto loop;
		}
		goto doiohalf;

	case DORESET:
		DPRINTF(("fdcintr: in DORESET\n"));
		/* try a reset, keep motor on */
		fd_set_motor(fdc, 1);
		DELAY(100);
		fd_set_motor(fdc, 0);
		fdc->sc_state = RESETCOMPLETE;
		callout_reset(&fdc->sc_timo_ch, hz / 2, fdctimeout, fdc);
		return 1;			/* will return later */

	case RESETCOMPLETE:
		DPRINTF(("fdcintr: in RESETCOMPLETE\n"));
		callout_stop(&fdc->sc_timo_ch);
		/* clear the controller output buffer */
		for (i = 0; i < 4; i++) {
			out_fdc(iot, ioh, NE7CMD_SENSEI);
			(void) fdcresult(fdc);
		}

		/* fall through */
	case DORECAL:
		DPRINTF(("fdcintr: in DORECAL\n"));
		out_fdc(iot, ioh, NE7CMD_RECAL); /* recalibrate function */
		out_fdc(iot, ioh, fd->sc_drive);
		fdc->sc_state = RECALWAIT;
		callout_reset(&fdc->sc_timo_ch, 5 * hz, fdctimeout, fdc);
		return 1;			/* will return later */

	case RECALWAIT:
		DPRINTF(("fdcintr: in RECALWAIT\n"));
		callout_stop(&fdc->sc_timo_ch);
		fdc->sc_state = RECALCOMPLETE;
		/* allow 1/30 second for heads to settle */
#if 0
		callout_reset(&fdc->sc_intr_ch, hz / 30, fdcpseudointr, fdc);
#endif
		return 1;			/* will return later */

	case RECALCOMPLETE:
		DPRINTF(("fdcintr: in RECALCOMPLETE\n"));
		out_fdc(iot, ioh, NE7CMD_SENSEI);
		tmp = fdcresult(fdc);
		if ((st0 & 0xf8) == 0xc0) {
			DPRINTF(("fdcintr: first seek!\n"));
			fdc->sc_state = DORECAL;
			goto loop;
		} else if (tmp != 2 || (st0 & 0xf8) != 0x20 || cyl != 0) {
#ifdef FDDEBUG
			fdcstatus(&fd->sc_dev, 2, "recalibrate failed");
#endif
			fdcretry(fdc);
			goto loop;
		}
		fd->sc_cylin = 0;
		goto doseek;

	case MOTORWAIT:
		if (fd->sc_flags & FD_MOTOR_WAIT)
			return 1;		/* time's not up yet */
		goto doseek;

	default:
		fdcstatus(&fd->sc_dev, 0, "stray interrupt");
		return 1;
	}
#ifdef DIAGNOSTIC
	panic("fdcintr: impossible");
#endif
#undef	st0
#undef	cyl
}

void
fdcretry(struct fdc_softc *fdc)
{
	struct fd_softc *fd;
	struct buf *bp;
	char bits[64];

	DPRINTF(("fdcretry:\n"));
	fd = fdc->sc_drives.tqh_first;
	bp = BUFQ_PEEK(fd->sc_q);

	switch (fdc->sc_errors) {
	case 0:
		/* try again */
		fdc->sc_state = SEEKCOMPLETE;
		break;

	case 1: case 2: case 3:
		/* didn't work; try recalibrating */
		fdc->sc_state = DORECAL;
		break;

	case 4:
		/* still no go; reset the bastard */
		fdc->sc_state = DORESET;
		break;

	default:
		diskerr(bp, "fd", "hard error", LOG_PRINTF,
			fd->sc_skip, (struct disklabel *)NULL);
		printf(" (st0 %s", bitmask_snprintf(fdc->sc_status[0],
						    NE7_ST0BITS, bits,
						    sizeof(bits)));
		printf(" st1 %s", bitmask_snprintf(fdc->sc_status[1],
						   NE7_ST1BITS, bits,
						   sizeof(bits)));
		printf(" st2 %s", bitmask_snprintf(fdc->sc_status[2],
						   NE7_ST2BITS, bits,
						   sizeof(bits)));
		printf(" cyl %d head %d sec %d)\n",
		       fdc->sc_status[3],
		       fdc->sc_status[4],
		       fdc->sc_status[5]);

		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		fdfinish(fd, bp);
	}
	fdc->sc_errors++;
}

int
fdioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct fd_softc *fd = fd_cd.cd_devs[FDUNIT(dev)];
	struct fdc_softc *fdc = (void *)device_parent(&fd->sc_dev);
	int unit = FDUNIT(dev);
	int part = DISKPART(dev);
	struct disklabel buffer;
	int error;

	DPRINTF(("fdioctl:\n"));
	switch (cmd) {
	case DIOCGDINFO:
#if 1
		*(struct disklabel *)addr = *(fd->sc_dk.dk_label);
		return(0);
#else
		memset(&buffer, 0, sizeof(buffer));

		buffer.d_secpercyl = fd->sc_type->seccyl;
		buffer.d_type = DTYPE_FLOPPY;
		buffer.d_secsize = 128 << fd->sc_type->secsize;

		if (readdisklabel(dev, fdstrategy, &buffer, NULL) != NULL)
			return EINVAL;

		*(struct disklabel *)addr = buffer;
		return 0;
#endif

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = fd->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
		    &fd->sc_dk.dk_label->d_partitions[part];
		return(0);

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return EBADF;
		/* XXX do something */
		return 0;

	case DIOCWDINFO:
		if ((flag & FWRITE) == 0)
			return EBADF;

		error = setdisklabel(&buffer, (struct disklabel *)addr,
		                     0, NULL);
		if (error)
			return error;

		error = writedisklabel(dev, fdstrategy, &buffer, NULL);
		return error;

	case DIOCLOCK:
		/*
		 * Nothing to do here, really.
		 */
		return 0; /* XXX */

	case DIOCEJECT:
		if (*(int *)addr == 0) {
			/*
			 * Don't force eject: check that we are the only
			 * partition open. If so, unlock it.
			 */
			if ((fd->sc_dk.dk_openmask & ~(1 << part)) != 0 ||
			    fd->sc_dk.dk_bopenmask + fd->sc_dk.dk_copenmask !=
			    fd->sc_dk.dk_openmask) {
				return (EBUSY);
			}
		}
		/* FALLTHROUGH */
	case ODIOCEJECT:
		fd_do_eject(fdc, unit);
		return 0;

	default:
		return ENOTTY;
	}

#ifdef DIAGNOSTIC
	panic("fdioctl: impossible");
#endif
}

void
fd_do_eject(struct fdc_softc *fdc, int unit)
{
	bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdout,
			  0x20 | ( 1 << unit));
	DELAY(1); /* XXX */
	bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdout, 0x20);
}

/*
 * Build disk label. For now we only create a label from what we know
 * from 'sc'.
 */
static int
fdgetdisklabel(struct fd_softc *sc, dev_t dev)
{
	struct disklabel *lp;
	int part;

	DPRINTF(("fdgetdisklabel()\n"));

	part = DISKPART(dev);
	lp = sc->sc_dk.dk_label;
	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize     = 128 << sc->sc_type->secsize;
	lp->d_ntracks     = sc->sc_type->heads;
	lp->d_nsectors    = sc->sc_type->sectrac;
	lp->d_secpercyl   = lp->d_ntracks * lp->d_nsectors;
	lp->d_ncylinders  = sc->sc_type->size / lp->d_secpercyl;
	lp->d_secperunit  = sc->sc_type->size;

	lp->d_type        = DTYPE_FLOPPY;
	lp->d_rpm         = 300; 	/* XXX */
	lp->d_interleave  = 1;		/* FIXME: is this OK?		*/
	lp->d_bbsize      = 0;
	lp->d_sbsize      = 0;
	lp->d_npartitions = part + 1;
#define STEP_DELAY	6000	/* 6ms (6000us) delay after stepping	*/
	lp->d_trkseek     = STEP_DELAY; /* XXX */
	lp->d_magic       = DISKMAGIC;
	lp->d_magic2      = DISKMAGIC;
	lp->d_checksum    = dkcksum(lp);
	lp->d_partitions[part].p_size   = lp->d_secperunit;
	lp->d_partitions[part].p_fstype = FS_UNUSED;
	lp->d_partitions[part].p_fsize  = 1024;
	lp->d_partitions[part].p_frag   = 8;

	return(0);
}

#include <dev/cons.h>

/*
 * Mountroot hook: prompt the user to enter the root file system
 * floppy.
 */
void
fd_mountroot_hook(struct device *dev)
{
	struct fd_softc *fd = (void *)dev;
	struct fdc_softc *fdc = (void *)device_parent(&fd->sc_dev);
	int c;

	/* XXX device_unit() abuse */
	fd_do_eject(fdc, device_unit(dev));
	printf("Insert filesystem floppy and press return.");
	for (;;) {
		c = cngetc();
		if ((c == '\r') || (c == '\n')) {
			printf("\n");
			break;
		}
	}
}
