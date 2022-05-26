/*	$NetBSD: fd.c,v 1.127 2022/05/26 14:27:43 tsutsui Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: fd.c,v 1.127 2022/05/26 14:27:43 tsutsui Exp $");

#include "opt_ddb.h"
#include "opt_m68k_arch.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
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
#include <sys/proc.h>
#include <sys/fdio.h>
#include <sys/rndsource.h>

#include <dev/cons.h>

#include <machine/cpu.h>

#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/dmacvar.h>
#include <arch/x68k/dev/fdreg.h>
#include <arch/x68k/dev/opmvar.h> /* for CT1 access */

#include "locators.h"
#include "ioconf.h"

#ifdef FDDEBUG
#define DPRINTF(x)      if (fddebug) printf x
int     fddebug = 0;
#else
#define DPRINTF(x)
#endif

#define FDUNIT(dev)	(minor(dev) / 8)
#define FDTYPE(dev)	(minor(dev) % 8)

/* (mis)use device use flag to identify format operation */
#define B_FORMAT B_DEVPRIVATE

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
	bus_space_tag_t sc_iot;		/* intio i/o space identifier */
	bus_space_handle_t sc_ioh;	/* intio io handle */

	struct callout sc_timo_ch;	/* timeout callout */
	struct callout sc_intr_ch;	/* pseudo-intr callout */

	bus_dma_tag_t sc_dmat;		/* intio DMA tag */
	bus_dmamap_t sc_dmamap;		/* DMA map */
	uint8_t *sc_addr;		/* physical address */
	struct dmac_channel_stat *sc_dmachan; /* intio DMA channel */
	struct dmac_dma_xfer *sc_xfer;	/* DMA transfer */
	int sc_read;

	struct fd_softc *sc_fd[4];	/* pointers to children */
	TAILQ_HEAD(drivehead, fd_softc) sc_drives;
	enum fdc_state sc_state;
	int sc_errors;			/* number of retries so far */
	uint8_t sc_status[7];		/* copy of registers */
};

static int fdcintr(void *);
static void fdcreset(struct fdc_softc *);

/* controller driver configuration */
static int fdcprobe(device_t, cfdata_t, void *);
static void fdcattach(device_t, device_t, void *);
static int fdprint(void *, const char *);

CFATTACH_DECL_NEW(fdc, sizeof(struct fdc_softc),
    fdcprobe, fdcattach, NULL, NULL);

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
	uint8_t	fillbyte;	/* format fill byte */
	uint8_t	interleave;	/* interleave factor (formatting) */
	const char *name;
};

/* The order of entries in the following table is important -- BEWARE! */
static struct fd_type fd_types[] = {
	{  8,2,16,3,0xff,0xdf,0x35,0x74,77,1232,1,FDC_500KBPS, 0xf6, 1,
	    "1.2MB/[1024bytes/sector]"    }, /* 1.2 MB japanese format */
	{ 18,2,36,2,0xff,0xcf,0x1b,0x6c,80,2880,1,FDC_500KBPS, 0xf6, 1,
	    "1.44MB"    }, /* 1.44MB diskette */
	{ 15,2,30,2,0xff,0xdf,0x1b,0x54,80,2400,1,FDC_500KBPS, 0xf6, 1,
	    "1.2MB"    }, /* 1.2 MB AT-diskettes */
	{  9,2,18,2,0xff,0xdf,0x23,0x50,40, 720,2,FDC_300KBPS, 0xf6, 1,
	    "360KB/AT" }, /* 360kB in 1.2MB drive */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,1,FDC_250KBPS, 0xf6, 1,
	    "360KB/PC" }, /* 360kB PC diskettes */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,80,1440,1,FDC_250KBPS, 0xf6, 1,
	    "720KB"    }, /* 3.5" 720kB diskette */
	{  9,2,18,2,0xff,0xdf,0x23,0x50,80,1440,1,FDC_300KBPS, 0xf6, 1,
	    "720KB/x"  }, /* 720kB in 1.2MB drive */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,2,FDC_250KBPS, 0xf6, 1,
	    "360KB/x"  }, /* 360kB in 720kB drive */
};

/* software state, per disk (with up to 4 disks per ctlr) */
struct fd_softc {
	device_t sc_dev;
	struct disk sc_dk;

	struct fd_type *sc_deftype;	/* default type descriptor */
	struct fd_type *sc_type;	/* current type descriptor */

#if 0	/* see comments in fd_motor_on() */
	struct callout sc_motoron_ch;
#endif
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
	uint8_t *sc_copybuf;	/* for secsize >=3 */
	uint8_t sc_part;	/* for secsize >=3 */
#define	SEC_P10	0x02		/* first part */
#define	SEC_P01	0x01		/* second part */
#define	SEC_P11	0x03		/* both part */

	krndsource_t	rnd_source;
};

/* floppy driver configuration */
static int fdprobe(device_t, cfdata_t, void *);
static void fdattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(fd, sizeof(struct fd_softc),
    fdprobe, fdattach, NULL, NULL);

static dev_type_open(fdopen);
static dev_type_close(fdclose);
static dev_type_read(fdread);
static dev_type_write(fdwrite);
static dev_type_ioctl(fdioctl);
static dev_type_strategy(fdstrategy);

const struct bdevsw fd_bdevsw = {
	.d_open = fdopen,
	.d_close = fdclose,
	.d_strategy = fdstrategy,
	.d_ioctl = fdioctl,
	.d_dump = nodump,
	.d_psize = nosize,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

const struct cdevsw fd_cdevsw = {
	.d_open = fdopen,
	.d_close = fdclose,
	.d_read = fdread,
	.d_write = fdwrite,
	.d_ioctl = fdioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

static void fdstart(struct fd_softc *);

struct dkdriver fddkdriver = {
	.d_strategy = fdstrategy
};

static void fd_set_motor(struct fdc_softc *, int);
static void fd_motor_off(void *);
#if 0
static void fd_motor_on(void *);
#endif
static int fdcresult(struct fdc_softc *);
static int out_fdc(bus_space_tag_t, bus_space_handle_t, uint8_t);
static void fdcstart(struct fdc_softc *);
static void fdcstatus(device_t, int, const char *);
static void fdctimeout(void *);
#if 0
static void fdcpseudointr(void *);
#endif
static void fdcretry(struct fdc_softc *);
static void fdfinish(struct fd_softc *, struct buf *);
static struct fd_type *fd_dev_to_type(struct fd_softc *, dev_t);
static int fdformat(dev_t, struct ne7_fd_formb *, struct lwp *);
static int fdcpoll(struct fdc_softc *);
static int fdgetdisklabel(struct fd_softc *, dev_t);
static void fd_do_eject(struct fdc_softc *, int);

static void fd_mountroot_hook(device_t);

/* DMA transfer routines */
inline static void fdc_dmastart(struct fdc_softc *, int, void *, vsize_t);
inline static void fdc_dmaabort(struct fdc_softc *);
static int fdcdmaintr(void *);
static int fdcdmaerrintr(void *);

inline static void
fdc_dmastart(struct fdc_softc *fdc, int read, void *addr, vsize_t count)
{
	int error;

	DPRINTF(("fdc_dmastart: %s, addr = %p, count = %ld\n",
	    read ? "read" : "write", (void *)addr, count));

	error = bus_dmamap_load(fdc->sc_dmat, fdc->sc_dmamap, addr, count,
	    0, BUS_DMA_NOWAIT);
	if (error) {
		panic("fdc_dmastart: cannot load dmamap");
	}

	bus_dmamap_sync(fdc->sc_dmat, fdc->sc_dmamap, 0, count,
	    read ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

	/*
	 * Note 1:
	 *  uPD72065 ignores A0 input (connected to x68k bus A1)
	 *  during DMA xfer access, but it's better to explicitly
	 *  specify FDC data register address for clarification.
	 * Note 2:
	 *  FDC is connected to LSB 8 bits of X68000 16 bit bus
	 *  (as BUS_SPACE_MAP_SHIFTED_ODD defined in bus.h)
	 *  so each FDC register is mapped at sparse odd address.
	 *
	 * XXX: No proper API to get DMA address of FDC register for DMAC.
	 */
	fdc->sc_xfer = dmac_prepare_xfer(fdc->sc_dmachan, fdc->sc_dmat,
	    fdc->sc_dmamap,
	    read ? DMAC_OCR_DIR_DTM : DMAC_OCR_DIR_MTD,
	    DMAC_SCR_MAC_COUNT_UP | DMAC_SCR_DAC_NO_COUNT,
	    fdc->sc_addr + fddata * 2 + 1);

	fdc->sc_read = read;
	dmac_start_xfer(fdc->sc_dmachan->ch_softc, fdc->sc_xfer);
}

inline static void
fdc_dmaabort(struct fdc_softc *fdc)
{

	dmac_abort_xfer(fdc->sc_dmachan->ch_softc, fdc->sc_xfer);
	bus_dmamap_unload(fdc->sc_dmat, fdc->sc_dmamap);
}

static int
fdcdmaintr(void *arg)
{
	struct fdc_softc *fdc = arg;

	bus_dmamap_sync(fdc->sc_dmat, fdc->sc_dmamap,
	    0, fdc->sc_dmamap->dm_mapsize,
	    fdc->sc_read ?
	    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
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
static int
fdcprobe(device_t parent, cfdata_t cf, void *aux)
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

	ia->ia_size = FDC_MAPSIZE;
	if (intio_map_allocate_region(parent, ia, INTIO_MAP_TESTONLY))
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
static int
fdprint(void *aux, const char *fdc)
{
	struct fdc_attach_args *fa = aux;

	if (fdc == NULL)
		aprint_normal(" drive %d", fa->fa_drive);
	return QUIET;
}

static void
fdcattach(device_t parent, device_t self, void *aux)
{
	struct fdc_softc *fdc = device_private(self);
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct intio_attach_args *ia = aux;
	struct fdc_attach_args fa;

	iot = ia->ia_bst;

	aprint_normal("\n");

	/* Re-map the I/O space. */
	if (bus_space_map(iot, ia->ia_addr, ia->ia_size,
	    BUS_SPACE_MAP_SHIFTED_ODD, &ioh) != 0) {
		aprint_error_dev(self, "unable to map I/O space\n");
		return;
	}

	callout_init(&fdc->sc_timo_ch, 0);
	callout_init(&fdc->sc_intr_ch, 0);

	fdc->sc_iot = iot;
	fdc->sc_ioh = ioh;
	fdc->sc_addr = (void *)ia->ia_addr;

	fdc->sc_dmat = ia->ia_dmat;
	fdc->sc_state = DEVIDLE;
	TAILQ_INIT(&fdc->sc_drives);

	/* Initialize DMAC channel */
	fdc->sc_dmachan = dmac_alloc_channel(parent, ia->ia_dma, "fdc",
	    ia->ia_dmaintr, fdcdmaintr, fdc,
	    ia->ia_dmaintr + 1, fdcdmaerrintr, fdc,
	    (DMAC_DCR_XRM_CSWH | DMAC_DCR_OTYP_EASYNC | DMAC_DCR_OPS_8BIT),
	    (DMAC_OCR_SIZE_BYTE | DMAC_OCR_REQG_EXTERNAL));

	if (bus_dmamap_create(fdc->sc_dmat, FDC_MAXIOSIZE, 1, DMAC_MAXSEGSZ,
	    0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &fdc->sc_dmamap)) {
		aprint_error_dev(self, "can't set up intio DMA map\n");
		return;
	}

	if (intio_intr_establish(ia->ia_intr, "fdc", fdcintr, fdc) != 0)
		panic("Could not establish interrupt (duplicated vector?).");
	intio_set_ivec(ia->ia_intr);

	/* reset */
	intio_disable_intr(SICILIAN_INTR_FDD);
	intio_enable_intr(SICILIAN_INTR_FDC);
	fdcresult(fdc);
	fdcreset(fdc);

	aprint_normal_dev(self, "uPD72065 FDC\n");
	out_fdc(iot, ioh, NE7CMD_SPECIFY);	/* specify command */
	out_fdc(iot, ioh, 0xd0);
	out_fdc(iot, ioh, 0x10);

	/* physical limit: four drives per controller. */
	for (fa.fa_drive = 0; fa.fa_drive < 4; fa.fa_drive++) {
		(void)config_found(self, (void *)&fa, fdprint, CFARGS_NONE);
	}

	intio_enable_intr(SICILIAN_INTR_FDC);
}

static void
fdcreset(struct fdc_softc *fdc)
{

	bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdsts, NE7CMD_RESET);
}

static int
fdcpoll(struct fdc_softc *fdc)
{
	int i = 25000;

	while (--i > 0) {
		if ((intio_get_sicilian_intr() & SICILIAN_STAT_FDC) != 0) {
			out_fdc(fdc->sc_iot, fdc->sc_ioh, NE7CMD_SENSEI);
			fdcresult(fdc);
			break;
		}
		DELAY(100);
	}
	return i;
}

static int
fdprobe(device_t parent, cfdata_t cf, void *aux)
{
	struct fdc_softc *fdc = device_private(parent);
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

	/* toss any interrupt status */
	for (n = 0; n < 4; n++) {
		out_fdc(iot, ioh, NE7CMD_SENSEI);
		(void)fdcresult(fdc);
	}
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
		if ((intio_get_sicilian_intr() & SICILIAN_STAT_FDC) != 0) {
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
	    fdctl, (type->rate << 4) | drive);
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
static void
fdattach(device_t parent, device_t self, void *aux)
{
	struct fdc_softc *fdc = device_private(parent);
	struct fd_softc *fd = device_private(self);
	struct fdc_attach_args *fa = aux;
	struct fd_type *type = &fd_types[0];	/* XXX 1.2MB */
	int drive = fa->fa_drive;

#if 0
	callout_init(&fd->sc_motoron_ch, 0);
#endif
	callout_init(&fd->sc_motoroff_ch, 0);

	fd->sc_dev = self;
	fd->sc_flags = 0;

	if (type)
		aprint_normal(": %s, %d cyl, %d head, %d sec\n", type->name,
		    type->cyls, type->heads, type->sectrac);
	else
		aprint_normal(": density unknown\n");

	bufq_alloc(&fd->sc_q, "disksort", BUFQ_SORT_CYLINDER);
	fd->sc_cylin = -1;
	fd->sc_drive = drive;
	fd->sc_deftype = type;
	fdc->sc_fd[drive] = fd;

	fd->sc_copybuf = malloc(PAGE_SIZE, M_DEVBUF, M_WAITOK);
	if (fd->sc_copybuf == 0)
		aprint_error("%s: WARNING!! malloc() failed.\n", __func__);
	fd->sc_flags |= FD_ALIVE;

	/*
	 * Initialize and attach the disk structure.
	 */
	disk_init(&fd->sc_dk, device_xname(fd->sc_dev), &fddkdriver);
	disk_attach(&fd->sc_dk);

	/*
	 * Establish a mountroot_hook anyway in case we booted
	 * with RB_ASKNAME and get selected as the boot device.
	 */
	mountroothook_establish(fd_mountroot_hook, fd->sc_dev);

	rnd_attach_source(&fd->rnd_source, device_xname(fd->sc_dev),
	    RND_TYPE_DISK, RND_FLAG_DEFAULT);
}

static struct fd_type *
fd_dev_to_type(struct fd_softc *fd, dev_t dev)
{
	size_t type = FDTYPE(dev);

	if (type >= __arraycount(fd_types))
		return NULL;
	return &fd_types[type];
}

static void
fdstrategy(struct buf *bp)
{
	struct fd_softc *fd;
	int unit;
	int sz;
	int s;

	unit = FDUNIT(bp->b_dev);
	fd = device_lookup_private(&fd_cd, unit);
	if (fd == NULL) {
		bp->b_error = EINVAL;
		goto done;
	}

	if (bp->b_blkno < 0 ||
	    ((bp->b_bcount % FDC_BSIZE) != 0 &&
	     (bp->b_flags & B_FORMAT) == 0)) {
		DPRINTF(("fdstrategy: unit=%d, blkno=%" PRId64 ", "
		    "bcount=%d\n", unit,
		    bp->b_blkno, bp->b_bcount));
		bp->b_error = EINVAL;
		goto done;
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
			goto done;
		}
		/* Otherwise, truncate request. */
		bp->b_bcount = sz << DEV_BSHIFT;
	}

	bp->b_rawblkno = bp->b_blkno;
	bp->b_cylinder = (bp->b_blkno / (FDC_BSIZE / DEV_BSIZE)) /
	    (fd->sc_type->seccyl * (1 << (fd->sc_type->secsize - 2)));

	DPRINTF(("fdstrategy: %s b_blkno %" PRId64 " b_bcount %d cylin %d\n",
	    bp->b_flags & B_READ ? "read" : "write",
	    bp->b_blkno, bp->b_bcount, bp->b_cylinder));
	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();
	bufq_put(fd->sc_q, bp);
	callout_stop(&fd->sc_motoroff_ch);		/* a good idea */
	if (fd->sc_active == 0)
		fdstart(fd);
#ifdef DIAGNOSTIC
	else {
		struct fdc_softc *fdc;

		fdc = device_private(device_parent(fd->sc_dev));
		if (fdc->sc_state == DEVIDLE) {
			printf("fdstrategy: controller inactive\n");
			fdcstart(fdc);
		}
	}
#endif
	splx(s);
	return;

 done:
	/* Toss transfer; we're done early. */
	biodone(bp);
}

static void
fdstart(struct fd_softc *fd)
{
	struct fdc_softc *fdc = device_private(device_parent(fd->sc_dev));
	int active = !TAILQ_EMPTY(&fdc->sc_drives);

	/* Link into controller queue. */
	fd->sc_active = 1;
	TAILQ_INSERT_TAIL(&fdc->sc_drives, fd, sc_drivechain);

	/* If controller not already active, start it. */
	if (!active)
		fdcstart(fdc);
}

static void
fdfinish(struct fd_softc *fd, struct buf *bp)
{
	struct fdc_softc *fdc = device_private(device_parent(fd->sc_dev));

	/*
	 * Move this drive to the end of the queue to give others a `fair'
	 * chance.  We only force a switch if N operations are completed while
	 * another drive is waiting to be serviced, since there is a long motor
	 * startup delay whenever we switch.
	 */
	(void)bufq_get(fd->sc_q);
	if (TAILQ_NEXT(fd, sc_drivechain) && ++fd->sc_ops >= 8) {
		fd->sc_ops = 0;
		TAILQ_REMOVE(&fdc->sc_drives, fd, sc_drivechain);
		if (bufq_peek(fd->sc_q) != NULL)
			TAILQ_INSERT_TAIL(&fdc->sc_drives, fd, sc_drivechain);
		else
			fd->sc_active = 0;
	}
	bp->b_resid = fd->sc_bcount;
	fd->sc_skip = 0;

	rnd_add_uint32(&fd->rnd_source, bp->b_blkno);

	biodone(bp);
	/* turn off motor 5s from now */
	callout_reset(&fd->sc_motoroff_ch, 5 * hz, fd_motor_off, fd);
	fdc->sc_state = DEVIDLE;
}

static int
fdread(dev_t dev, struct uio *uio, int flags)
{

	return physio(fdstrategy, NULL, dev, B_READ, minphys, uio);
}

static int
fdwrite(dev_t dev, struct uio *uio, int flags)
{

	return physio(fdstrategy, NULL, dev, B_WRITE, minphys, uio);
}

static void
fd_set_motor(struct fdc_softc *fdc, int reset)
{
	struct fd_softc *fd;
	int n;

	DPRINTF(("fd_set_motor:\n"));
	for (n = 0; n < 4; n++) {
		fd = fdc->sc_fd[n];
		if (fd != NULL && (fd->sc_flags & FD_MOTOR) != 0)
			bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdctl,
			    0x80 | (fd->sc_type->rate << 4)| n);
	}
}

static void
fd_motor_off(void *arg)
{
	struct fd_softc *fd = arg;
	struct fdc_softc *fdc = device_private(device_parent(fd->sc_dev));
	int s;

	DPRINTF(("fd_motor_off:\n"));

	s = splbio();
	fd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
	bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdctl,
	    (fd->sc_type->rate << 4) | fd->sc_drive);
#if 0
	fd_set_motor(fdc, 0); /* XXX */
#endif
	splx(s);
}

#if 0 /* on x68k motor on triggers interrupts by state change of ready line. */
static void
fd_motor_on(void *arg)
{
	struct fd_softc *fd = arg;
	struct fdc_softc *fdc = device_private(device_parent(fd->sc_dev));
	int s;

	DPRINTF(("fd_motor_on:\n"));

	s = splbio();
	fd->sc_flags &= ~FD_MOTOR_WAIT;
	if ((TAILQ_FIRST(&fdc->sc_drives) == fd) &&
	    (fdc->sc_state == MOTORWAIT))
		(void)fdcintr(fdc);
	splx(s);
}
#endif

static int
fdcresult(struct fdc_softc *fdc)
{
	bus_space_tag_t iot = fdc->sc_iot;
	bus_space_handle_t ioh = fdc->sc_ioh;
	uint8_t i;
	int j, n;

	n = 0;
	for (j = 100000; j != 0; j--) {
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

static int
out_fdc(bus_space_tag_t iot, bus_space_handle_t ioh, uint8_t x)
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

static int
fdopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit;
	struct fd_softc *fd;
	struct fd_type *type;
	struct fdc_softc *fdc;

	unit = FDUNIT(dev);
	fd = device_lookup_private(&fd_cd, unit);
	if (fd == NULL)
		return ENXIO;
	type = fd_dev_to_type(fd, dev);
	if (type == NULL)
		return ENXIO;

	if ((fd->sc_flags & FD_OPEN) != 0 &&
	    fd->sc_type != type)
		return EBUSY;

	fdc = device_private(device_parent(fd->sc_dev));
	if ((fd->sc_flags & FD_OPEN) == 0) {
		/* Lock eject button */
		bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdout,
		    0x40 | (1 << unit));
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

static int
fdclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = FDUNIT(dev);
	struct fd_softc *fd = device_lookup_private(&fd_cd, unit);
	struct fdc_softc *fdc = device_private(device_parent(fd->sc_dev));

	DPRINTF(("fdclose %d\n", unit));

	switch (mode) {
	case S_IFCHR:
		fd->sc_flags &= ~FD_COPEN;
		break;
	case S_IFBLK:
		fd->sc_flags &= ~FD_BOPEN;
		break;
	}

	/* clear flags */
	fd->sc_opts &= ~(FDOPT_NORETRY | FDOPT_SILENT);

	if ((fd->sc_flags & FD_OPEN) == 0) {
		bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdout,
		    (1 << unit));
		bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdout, 0);
	}
	return 0;
}

static void
fdcstart(struct fdc_softc *fdc)
{

#ifdef DIAGNOSTIC
	/*
	 * only got here if controller's drive queue was inactive; should
	 * be in idle state
	 */
	if (fdc->sc_state != DEVIDLE) {
		printf("fdcstart: not idle\n");
		return;
	}
#endif
	(void)fdcintr(fdc);
}


static void
fdcpstatus(int n, struct fdc_softc *fdc)
{
	char bits[64];

	switch (n) {
	case 0:
		printf("\n");
		break;
	case 2:
		snprintb(bits, sizeof(bits), NE7_ST0BITS, fdc->sc_status[0]);
		printf(" (st0 %s cyl %d)\n", bits, fdc->sc_status[1]);
		break;
	case 7:
		snprintb(bits, sizeof(bits), NE7_ST0BITS, fdc->sc_status[0]);
		printf(" (st0 %s", bits);
		snprintb(bits, sizeof(bits), NE7_ST1BITS, fdc->sc_status[1]);
		printf(" st1 %s", bits);
		snprintb(bits, sizeof(bits), NE7_ST2BITS, fdc->sc_status[2]);
		printf(" st2 %s", bits);
		printf(" cyl %d head %d sec %d)\n",
		    fdc->sc_status[3], fdc->sc_status[4], fdc->sc_status[5]);
		break;
#ifdef DIAGNOSTIC
	default:
		printf("\nfdcstatus: weird size");
		break;
#endif
	}
}

static void
fdcstatus(device_t dv, int n, const char *s)
{
	struct fdc_softc *fdc = device_private(device_parent(dv));

	if (n == 0) {
		out_fdc(fdc->sc_iot, fdc->sc_ioh, NE7CMD_SENSEI);
		(void)fdcresult(fdc);
		n = 2;
	}

	printf("%s: %s: state %d", device_xname(dv), s, fdc->sc_state);
	fdcpstatus(n, fdc);
}

static void
fdctimeout(void *arg)
{
	struct fdc_softc *fdc = arg;
	struct fd_softc *fd = TAILQ_FIRST(&fdc->sc_drives);
	int s;

	s = splbio();
	fdcstatus(fd->sc_dev, 0, "timeout");

	if (bufq_peek(fd->sc_q) != NULL)
		fdc->sc_state++;
	else
		fdc->sc_state = DEVIDLE;

	(void)fdcintr(fdc);
	splx(s);
}

#if 0
static void
fdcpseudointr(void *arg)
{
	int s;
	struct fdc_softc *fdc = arg;

	/* just ensure it has the right spl */
	s = splbio();
	(void)fdcintr(fdc);
	splx(s);
}
#endif

static int
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
	int tmp;
	struct fd_type *type;
	struct ne7_fd_formb *finfo = NULL;

 loop:
	fd = TAILQ_FIRST(&fdc->sc_drives);
	if (fd == NULL) {
		DPRINTF(("fdcintr: set DEVIDLE\n"));
		if (fdc->sc_state == DEVIDLE) {
			if ((intio_get_sicilian_intr() & SICILIAN_STAT_FDC)
			    != 0) {
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
	bp = bufq_peek(fd->sc_q);
	if (bp == NULL) {
		fd->sc_ops = 0;
		TAILQ_REMOVE(&fdc->sc_drives, fd, sc_drivechain);
		fd->sc_active = 0;
		goto loop;
	}

	if (bp->b_flags & B_FORMAT)
		finfo = (struct ne7_fd_formb *)bp->b_data;

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
				if (ofd != NULL &&
				    (ofd->sc_flags & FD_MOTOR) != 0) {
					callout_stop(&ofd->sc_motoroff_ch);
					ofd->sc_flags &=
					    ~(FD_MOTOR | FD_MOTOR_WAIT);
					break;
				}
			}
			fd->sc_flags |= FD_MOTOR | FD_MOTOR_WAIT;
			fd_set_motor(fdc, 0);
			fdc->sc_state = MOTORWAIT;
#if 0	/* no need to callout on x68k; motor on will trigger interrupts */
			/* allow .5s for motor to stabilize */
			callout_reset(&fd->sc_motoron_ch, hz / 2,
			    fd_motor_on, fd);
#endif
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

		out_fdc(iot, ioh, NE7CMD_SPECIFY);	/* specify command */
		out_fdc(iot, ioh, 0xd0);		/* XXX const */
		out_fdc(iot, ioh, 0x10);

		out_fdc(iot, ioh, NE7CMD_SEEK);		/* seek function */
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
		if (finfo != NULL)
			fd->sc_skip = (char *)&(finfo->fd_formb_cylno(0)) -
			    (char *)finfo;
		sectrac = type->sectrac;
		pos = fd->sc_blkno % (sectrac * (1 << (type->secsize - 2)));
		sec = pos / (1 << (type->secsize - 2));
		if (finfo != NULL || type->secsize == 2) {
			fd->sc_part = SEC_P11;
			nblks = (sectrac - sec) << (type->secsize - 2);
			nblks = uimin(nblks, fd->sc_bcount / FDC_BSIZE);
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
					nblks = uimin(nblks,
					    fd->sc_bcount / FDC_BSIZE - 1);
					DPRINTF(("nblks(2)"));
				}
			} else {
				fd->sc_part = SEC_P11;
				nblks = (sectrac - sec) << (type->secsize - 2);
				nblks = uimin(nblks, fd->sc_bcount / FDC_BSIZE);
				DPRINTF(("nblks(3)"));
			}
		} else {
			fd->sc_part = SEC_P01;
			nblks = 1;
			DPRINTF(("nblks(4)"));
		}
		nblks = uimin(nblks, FDC_MAXIOSIZE / FDC_BSIZE);
		DPRINTF((" %d\n", nblks));
		fd->sc_nblks = nblks;
		fd->sc_nbytes =
		    (finfo != NULL) ? bp->b_bcount : nblks * FDC_BSIZE;
		head = (fd->sc_blkno
		    % (type->seccyl * (1 << (type->secsize - 2))))
		    / (type->sectrac * (1 << (type->secsize - 2)));

#ifdef DIAGNOSTIC
		{
			int block;
			block = ((fd->sc_cylin * type->heads + head) *
			    type->sectrac + sec) * (1 << (type->secsize - 2));
			block += (fd->sc_part == SEC_P01) ? 1 : 0;
			if (block != fd->sc_blkno) {
				printf("C H R N: %d %d %d %d\n",
				    fd->sc_cylin, head, sec, type->secsize);
				printf("fdcintr: doio: block %d != blkno %"
				    PRId64 "\n",
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

		if (finfo == NULL && fd->sc_part != SEC_P11)
			goto docopy;

		fdc_dmastart(fdc, read, (char *)bp->b_data + fd->sc_skip,
		    fd->sc_nbytes);
		if (finfo != NULL) {
			/* formatting */
			if (out_fdc(iot, ioh, NE7CMD_FORMAT) < 0) {
				fdc->sc_errors = 4;
				fdcretry(fdc);
				goto loop;
			}
			out_fdc(iot, ioh, (head << 2) | fd->sc_drive);
			out_fdc(iot, ioh, finfo->fd_formb_secshift);
			out_fdc(iot, ioh, finfo->fd_formb_nsecs);
			out_fdc(iot, ioh, finfo->fd_formb_gaplen);
			out_fdc(iot, ioh, finfo->fd_formb_fillbyte);
		} else {
			if (read)
				out_fdc(iot, ioh, NE7CMD_READ);	/* READ */
			else
				out_fdc(iot, ioh, NE7CMD_WRITE); /* WRITE */
			out_fdc(iot, ioh, (head << 2) | fd->sc_drive);
			out_fdc(iot, ioh, bp->b_cylinder);	/* cylinder */
			out_fdc(iot, ioh, head);
			out_fdc(iot, ioh, sec + 1);		/* sector +1 */
			out_fdc(iot, ioh, type->secsize); /* sector size */
			out_fdc(iot, ioh, type->sectrac); /* sectors/track */
			out_fdc(iot, ioh, type->gap1);		/* gap1 size */
			out_fdc(iot, ioh, type->datalen); /* data length */
		}
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
		pos = fd->sc_blkno %
		    (type->sectrac * (1 << (type->secsize - 2)));
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
		{
			int block;
			block = ((fd->sc_cylin * type->heads + head) *
			    type->sectrac + sec) * (1 << (type->secsize - 2));
			block += (fd->sc_part == SEC_P01) ? 1 : 0;
			if (block != fd->sc_blkno) {
				printf("fdcintr: block %d != blkno %" PRId64
				    "\n",
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
			fdcstatus(fd->sc_dev, 2, "seek failed");
#endif
			fdcretry(fdc);
			goto loop;
		}
		fd->sc_cylin = bp->b_cylinder;
		goto doio;

	case IOTIMEDOUT:
		fdc_dmaabort(fdc);
	case SEEKTIMEDOUT:
	case RECALTIMEDOUT:
	case RESETTIMEDOUT:
		fdcretry(fdc);
		goto loop;

	case IOCOMPLETE: /* IO DONE, post-analyze */
		callout_stop(&fdc->sc_timo_ch);
		DPRINTF(("fdcintr: in IOCOMPLETE\n"));
		if ((tmp = fdcresult(fdc)) != 7 || (st0 & 0xf8) != 0) {
			fdc_dmaabort(fdc);
			fdcstatus(fd->sc_dev, tmp, bp->b_flags & B_READ ?
			    "read failed" : "write failed");
			printf("blkno %" PRId64 " nblks %d\n",
			    fd->sc_blkno, fd->sc_nblks);
			fdcretry(fdc);
			goto loop;
		}
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
		if (finfo == NULL && fd->sc_bcount > 0) {
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
			fdc_dmaabort(fdc);
			fdcstatus(fd->sc_dev, 7, bp->b_flags & B_READ ?
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
			(void)fdcresult(fdc);
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
			fdcstatus(fd->sc_dev, 2, "recalibrate failed");
#endif
			fdcretry(fdc);
			goto loop;
		}
		fd->sc_cylin = 0;
		goto doseek;

	case MOTORWAIT:
#if 0 /* on x68k motor on triggers interrupts by state change of ready line. */
		if (fd->sc_flags & FD_MOTOR_WAIT)
			return 1;		/* time's not up yet */
#else
		/* check drive ready by state change interrupt */
		KASSERT(fd->sc_flags & FD_MOTOR_WAIT);
		out_fdc(iot, ioh, NE7CMD_SENSEI);
		tmp = fdcresult(fdc);
		if (tmp != 2 || (st0 & 0xc0) != 0xc0 /* ready changed */) {
			printf("%s: unexpected interrupt during MOTORWAIT",
			    device_xname(fd->sc_dev));
			fdcpstatus(7, fdc);
			return 1;
		}
		fd->sc_flags &= ~FD_MOTOR_WAIT;
#endif
		goto doseek;

	default:
		fdcstatus(fd->sc_dev, 0, "stray interrupt");
		return 1;
	}
#ifdef DIAGNOSTIC
	panic("fdcintr: impossible");
#endif
#undef	st0
#undef	cyl
}

static void
fdcretry(struct fdc_softc *fdc)
{
	struct fd_softc *fd;
	struct buf *bp;

	DPRINTF(("fdcretry:\n"));
	fd = TAILQ_FIRST(&fdc->sc_drives);
	bp = bufq_peek(fd->sc_q);

	if (fd->sc_opts & FDOPT_NORETRY)
		goto fail;

	switch (fdc->sc_errors) {
	case 0:
		/* try again */
		fdc->sc_state = SEEKCOMPLETE;
		break;

	case 1:
	case 2:
	case 3:
		/* didn't work; try recalibrating */
		fdc->sc_state = DORECAL;
		break;

	case 4:
		/* still no go; reset the bastard */
		fdc->sc_state = DORESET;
		break;

	default:
	fail:
		if ((fd->sc_opts & FDOPT_SILENT) == 0) {
			diskerr(bp, "fd", "hard error", LOG_PRINTF,
			    fd->sc_skip, (struct disklabel *)NULL);
			fdcpstatus(7, fdc);
		}

		bp->b_error = EIO;
		fdfinish(fd, bp);
	}
	fdc->sc_errors++;
}

static int
fdioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct fd_softc *fd = device_lookup_private(&fd_cd, FDUNIT(dev));
	struct fdc_softc *fdc = device_private(device_parent(fd->sc_dev));
	struct fdformat_parms *form_parms;
	struct fdformat_cmd *form_cmd;
	struct ne7_fd_formb *fd_formb;
	int part = DISKPART(dev);
	struct disklabel buffer;
	int error;
	unsigned int scratch;
	int il[FD_MAX_NSEC + 1];
	int i, j;

	error = disk_ioctl(&fd->sc_dk, dev, cmd, addr, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	DPRINTF(("fdioctl:"));
	switch (cmd) {
	case DIOCWLABEL:
		DPRINTF(("DIOCWLABEL\n"));
		if ((flag & FWRITE) == 0)
			return EBADF;
		/* XXX do something */
		return 0;

	case DIOCWDINFO:
		DPRINTF(("DIOCWDINFO\n"));
		if ((flag & FWRITE) == 0)
			return EBADF;

		error = setdisklabel(&buffer, (struct disklabel *)addr,
		    0, NULL);
		if (error)
			return error;

		error = writedisklabel(dev, fdstrategy, &buffer, NULL);
		return error;

	case FDIOCGETFORMAT:
		DPRINTF(("FDIOCGETFORMAT\n"));
		form_parms = (struct fdformat_parms *)addr;
		form_parms->fdformat_version = FDFORMAT_VERSION;
		form_parms->nbps = 128 * (1 << fd->sc_type->secsize);
		form_parms->ncyl = fd->sc_type->cyls;
		form_parms->nspt = fd->sc_type->sectrac;
		form_parms->ntrk = fd->sc_type->heads;
		form_parms->stepspercyl = fd->sc_type->step;
		form_parms->gaplen = fd->sc_type->gap2;
		form_parms->fillbyte = fd->sc_type->fillbyte;
		form_parms->interleave = fd->sc_type->interleave;
		switch (fd->sc_type->rate) {
		case FDC_500KBPS:
			form_parms->xfer_rate = 500 * 1024;
			break;
		case FDC_300KBPS:
			form_parms->xfer_rate = 300 * 1024;
			break;
		case FDC_250KBPS:
			form_parms->xfer_rate = 250 * 1024;
			break;
		default:
			return EINVAL;
		}
		return 0;

	case FDIOCSETFORMAT:
		DPRINTF(("FDIOCSETFORMAT\n"));
		if((flag & FWRITE) == 0)
			return EBADF;	/* must be opened for writing */
		form_parms = (struct fdformat_parms *)addr;
		if (form_parms->fdformat_version != FDFORMAT_VERSION)
			return EINVAL;	/* wrong version of formatting prog */

		scratch = form_parms->nbps >> 7;
		if ((form_parms->nbps & 0x7f) || ffs(scratch) == 0 ||
		    scratch & ~(1 << (ffs(scratch) - 1)))
			/* not a power-of-two multiple of 128 */
			return EINVAL;

		switch (form_parms->xfer_rate) {
		case 500 * 1024:
			fd->sc_type->rate = FDC_500KBPS;
			break;
		case 300 * 1024:
			fd->sc_type->rate = FDC_300KBPS;
			break;
		case 250 * 1024:
			fd->sc_type->rate = FDC_250KBPS;
			break;
		default:
			return EINVAL;
		}

		if (form_parms->nspt > FD_MAX_NSEC ||
		    form_parms->fillbyte > 0xff ||
		    form_parms->interleave > 0xff)
			return EINVAL;
		fd->sc_type->sectrac = form_parms->nspt;
		if (form_parms->ntrk != 2 && form_parms->ntrk != 1)
			return EINVAL;
		fd->sc_type->heads = form_parms->ntrk;
		fd->sc_type->seccyl = form_parms->nspt * form_parms->ntrk;
		fd->sc_type->secsize = ffs(scratch)-1;
		fd->sc_type->gap2 = form_parms->gaplen;
		fd->sc_type->cyls = form_parms->ncyl;
		fd->sc_type->size = fd->sc_type->seccyl * form_parms->ncyl *
		    form_parms->nbps / DEV_BSIZE;
		fd->sc_type->step = form_parms->stepspercyl;
		fd->sc_type->fillbyte = form_parms->fillbyte;
		fd->sc_type->interleave = form_parms->interleave;
		return 0;

	case FDIOCFORMAT_TRACK:
		DPRINTF(("FDIOCFORMAT_TRACK\n"));
		if ((flag & FWRITE) == 0)
			return EBADF;	/* must be opened for writing */
		form_cmd = (struct fdformat_cmd *)addr;
		if (form_cmd->formatcmd_version != FDFORMAT_VERSION)
			return EINVAL;	/* wrong version of formatting prog */

		if (form_cmd->head >= fd->sc_type->heads ||
		    form_cmd->cylinder >= fd->sc_type->cyls) {
			return EINVAL;
		}

		fd_formb = malloc(sizeof(struct ne7_fd_formb),
		    M_TEMP, M_WAITOK);
		fd_formb->head = form_cmd->head;
		fd_formb->cyl = form_cmd->cylinder;
		fd_formb->transfer_rate = fd->sc_type->rate;
		fd_formb->fd_formb_secshift = fd->sc_type->secsize;
		fd_formb->fd_formb_nsecs = fd->sc_type->sectrac;
		fd_formb->fd_formb_gaplen = fd->sc_type->gap2;
		fd_formb->fd_formb_fillbyte = fd->sc_type->fillbyte;

		memset(il, 0, sizeof il);
		for (j = 0, i = 1; i <= fd_formb->fd_formb_nsecs; i++) {
			while (il[(j % fd_formb->fd_formb_nsecs) + 1])
				j++;
			il[(j % fd_formb->fd_formb_nsecs)+  1] = i;
			j += fd->sc_type->interleave;
		}
		for (i = 0; i < fd_formb->fd_formb_nsecs; i++) {
			fd_formb->fd_formb_cylno(i) = form_cmd->cylinder;
			fd_formb->fd_formb_headno(i) = form_cmd->head;
			fd_formb->fd_formb_secno(i) = il[i + 1];
			fd_formb->fd_formb_secsize(i) = fd->sc_type->secsize;
		}

		error = fdformat(dev, fd_formb, l);
		free(fd_formb, M_TEMP);
		return error;

	case FDIOCGETOPTS:		/* get drive options */
		DPRINTF(("FDIOCGETOPTS\n"));
		*(int *)addr = fd->sc_opts;
		return 0;

	case FDIOCSETOPTS:        	/* set drive options */
		DPRINTF(("FDIOCSETOPTS\n"));
		fd->sc_opts = *(int *)addr;
		return 0;

	case DIOCLOCK:
		/*
		 * Nothing to do here, really.
		 */
		return 0; /* XXX */

	case DIOCEJECT:
		DPRINTF(("DIOCEJECT\n"));
		if (*(int *)addr == 0) {
			/*
			 * Don't force eject: check that we are the only
			 * partition open. If so, unlock it.
			 */
			if ((fd->sc_dk.dk_openmask & ~(1 << part)) != 0 ||
			    fd->sc_dk.dk_bopenmask + fd->sc_dk.dk_copenmask !=
			    fd->sc_dk.dk_openmask) {
				return EBUSY;
			}
		}
		/* FALLTHROUGH */
	case ODIOCEJECT:
		DPRINTF(("ODIOCEJECT\n"));
		fd_do_eject(fdc, FDUNIT(dev));
		return 0;

	default:
		return ENOTTY;
	}

#ifdef DIAGNOSTIC
	panic("fdioctl: impossible");
#endif
}

static int
fdformat(dev_t dev, struct ne7_fd_formb *finfo, struct lwp *l)
{
	int rv = 0;
	struct fd_softc *fd = device_lookup_private(&fd_cd, FDUNIT(dev));
	struct fd_type *type = fd->sc_type;
	struct buf *bp;

	/* set up a buffer header for fdstrategy() */
	bp = getiobuf(NULL, false);
	if (bp == NULL)
		return ENOBUFS;

	bp->b_cflags = BC_BUSY;
	bp->b_flags = B_PHYS | B_FORMAT;
	bp->b_proc = l->l_proc;
	bp->b_dev = dev;

	/*
	 * calculate a fake blkno, so fdstrategy() would initiate a
	 * seek to the requested cylinder
	 */
	bp->b_blkno = (finfo->cyl * (type->sectrac * type->heads)
	    + finfo->head * type->sectrac) * (128 << type->secsize) / DEV_BSIZE;

	bp->b_bcount = sizeof(struct fd_idfield_data) * finfo->fd_formb_nsecs;
	bp->b_data = (void *)finfo;

#ifdef FD_DEBUG
	printf("fdformat: blkno %" PRIx64 " count %x\n",
	    bp->b_blkno, bp->b_bcount);
#endif

	/* now do the format */
	fdstrategy(bp);

	/* ...and wait for it to complete */
	rv = biowait(bp);
	putiobuf(bp);
	return rv;
}

static void
fd_do_eject(struct fdc_softc *fdc, int unit)
{

	bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdout, 0x20 | (1 << unit));
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

	lp->d_type        = DKTYPE_FLOPPY;
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

	return 0;
}

/*
 * Mountroot hook: prompt the user to enter the root file system
 * floppy.
 */
static void
fd_mountroot_hook(device_t dev)
{
	struct fd_softc *fd = device_private(dev);
	struct fdc_softc *fdc = device_private(device_parent(fd->sc_dev));
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
