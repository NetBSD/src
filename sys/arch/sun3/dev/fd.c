/*	$NetBSD: fd.c,v 1.19.2.1 2000/06/22 17:04:45 minoura Exp $	*/

/*-
 * Copyright (c) 1993, 1994, 1995 Charles M. Hannum.
 * Copyright (c) 1995 Paul Kranenburg.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/dkstat.h>
#include <sys/disk.h>
#include <sys/fdio.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/queue.h>
#include <sys/conf.h>

#include <dev/cons.h>

#include <vm/vm.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>

#include <sun3/dev/fdreg.h>
#include <sun3/dev/fdvar.h>

/*
 * Print a complaint when no fd children were specified
 * in the config file.  Better than a link error...
 *
 * XXX: Some folks say this driver should be split in two,
 * but that seems pointless with ONLY one type of child.
 * (Thankfully, no 3/80 boxes have floppy tapes!:)
 */
#include "fdc.h"
#if NFD == 0
#error "fdc but no fd?"
#endif

#define FDUNIT(dev)	(minor(dev) / 8)
#define FDTYPE(dev)	(minor(dev) % 8)

/* XXX misuse a flag to identify format operation */
#define B_FORMAT B_XXX

#ifdef FD_DEBUG
int	fdc_debug = 0;
#endif

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
};

/* software state, per controller */
struct fdc_softc {
	struct device	sc_dev;		/* boilerplate */
	caddr_t		sc_reg;

	struct callout sc_timo_ch;	/* timeout callout */
	struct callout sc_intr_ch;	/* pseudo-intr callout */

	struct fd_softc *sc_fd[4];	/* pointers to children */
	TAILQ_HEAD(drivehead, fd_softc) sc_drives;
	enum fdc_state	sc_state;
	int		sc_flags;
#define FDC_82077		0x01
#define FDC_NEEDHEADSETTLE	0x02
#define FDC_EIS			0x04
	int		sc_errors;		/* number of retries so far */
	int		sc_overruns;		/* number of DMA overruns */
	int		sc_cfg;			/* current configuration */
	int		sc_fcr;			/* current image of floppy ctrlr reg. */
	struct fdcio	sc_io;
#define sc_reg_msr	sc_io.fdcio_reg_msr
#define sc_reg_fifo	sc_io.fdcio_reg_fifo
#define sc_reg_fcr	sc_io.fdcio_reg_fcr
#define	sc_reg_fvr	sc_io.fdcio_reg_fvr
#define sc_reg_drs	sc_io.fdcio_reg_msr
#define sc_istate	sc_io.fdcio_istate
#define sc_data		sc_io.fdcio_data
#define sc_tc		sc_io.fdcio_tc
#define sc_nstat	sc_io.fdcio_nstat
#define sc_status	sc_io.fdcio_status
#define sc_intrcnt	sc_io.fdcio_intrcnt
};

/* controller driver configuration */
int	fdcmatch __P((struct device *, struct cfdata *, void *));
void	fdcattach __P((struct device *, struct device *, void *));

struct cfattach fdc_ca = {
	sizeof(struct fdc_softc), fdcmatch, fdcattach
};

extern struct cfdriver fdc_cd;

__inline struct fd_type *fd_dev_to_type __P((struct fd_softc *, dev_t));

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
	int	tracks;		/* total num of tracks */
	int	size;		/* size of disk in sectors */
	int	step;		/* steps per cylinder */
	int	rate;		/* transfer speed code */
	int	fillbyte;	/* format fill byte */
	int	interleave;	/* interleave factor (formatting) */
	char	*name;
};

/* The order of entries in the following table is important -- BEWARE! */
struct fd_type fd_types[] = {
	{ 18,2,36,2,0xff,0xcf,0x1b,0x6c,80,2880,1,FDC_500KBPS,0xf6,1, "1.44MB"    }, /* 1.44MB diskette */
	{ 15,2,30,2,0xff,0xdf,0x1b,0x54,80,2400,1,FDC_500KBPS,0xf6,1, "1.2MB"    }, /* 1.2 MB AT-diskettes */
	{  9,2,18,2,0xff,0xdf,0x23,0x50,40, 720,2,FDC_300KBPS,0xf6,1, "360KB/AT" }, /* 360kB in 1.2MB drive */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,1,FDC_250KBPS,0xf6,1, "360KB/PC" }, /* 360kB PC diskettes */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,80,1440,1,FDC_250KBPS,0xf6,1, "720KB"    }, /* 3.5" 720kB diskette */
	{  9,2,18,2,0xff,0xdf,0x23,0x50,80,1440,1,FDC_300KBPS,0xf6,1, "720KB/x"  }, /* 720kB in 1.2MB drive */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,2,FDC_250KBPS,0xf6,1, "360KB/x"  }, /* 360kB in 720kB drive */
};

/* software state, per disk (with up to 4 disks per ctlr) */
struct fd_softc {
	struct device	sc_dv;		/* generic device info */
	struct disk	sc_dk;		/* generic disk info */

	struct fd_type *sc_deftype;	/* default type descriptor */
	struct fd_type *sc_type;	/* current type descriptor */

	struct callout sc_motoron_ch;
	struct callout sc_motoroff_ch;

	daddr_t	sc_blkno;	/* starting block number */
	int sc_bcount;		/* byte count left */
	int sc_skip;		/* bytes already transferred */
	int sc_nblks;		/* number of blocks currently tranferring */
	int sc_nbytes;		/* number of bytes currently tranferring */

	int sc_drive;		/* physical unit number */
	int sc_flags;
#define	FD_OPEN		0x01		/* it's open */
#define	FD_MOTOR	0x02		/* motor should be on */
#define	FD_MOTOR_WAIT	0x04		/* motor coming up */
	int sc_cylin;		/* where we think the head is */
	int sc_opts;		/* user-set options */

	void	*sc_sdhook;	/* shutdownhook cookie */

	TAILQ_ENTRY(fd_softc) sc_drivechain;
	int sc_ops;		/* I/O ops since last switch */
	struct buf_queue sc_q;	/* pending I/O requests */
	int sc_active;		/* number of active I/O operations */
};

bdev_decl(fd);
cdev_decl(fd);

/* floppy driver configuration */
int	fdmatch __P((struct device *, struct cfdata *, void *));
void	fdattach __P((struct device *, struct device *, void *));

struct cfattach fd_ca = {
	sizeof(struct fd_softc), fdmatch, fdattach
};

extern struct cfdriver fd_cd;

void fdgetdisklabel __P((dev_t));
int fd_get_parms __P((struct fd_softc *));
void fdstrategy __P((struct buf *));
void fdstart __P((struct fd_softc *));
int fdprint __P((void *, const char *));

struct dkdriver fddkdriver = { fdstrategy };

struct	fd_type *fd_nvtotype __P((char *, int, int));
void	fd_set_motor __P((struct fdc_softc *fdc));
void	fd_motor_off __P((void *arg));
void	fd_motor_on __P((void *arg));
int	fdcresult __P((struct fdc_softc *fdc));
int	out_fdc __P((struct fdc_softc *fdc, u_char x));
void	fdcstart __P((struct fdc_softc *fdc));
void	fdcstatus __P((struct device *dv, int n, char *s));
void	fdc_reset __P((struct fdc_softc *fdc));
void	fdctimeout __P((void *arg));
void	fdcpseudointr __P((void *arg));
int	fdchwintr __P((void *));
int	fdcswintr __P((void *));
int	fdcstate __P((struct fdc_softc *));
void	fdcretry __P((struct fdc_softc *fdc));
void	fdfinish __P((struct fd_softc *fd, struct buf *bp));
int	fdformat __P((dev_t, struct ne7_fd_formb *, struct proc *));
void	fd_do_eject __P((struct fdc_softc *, int));
void	fd_mountroot_hook __P((struct device *));
static void fdconf __P((struct fdc_softc *));

static int fdc_softpend = 0;
#ifndef	FDC_SOFTPRI
#define	FDC_SOFTPRI	2
#endif
#define FD_SET_SWINTR()	{ fdc_softpend = 1; isr_soft_request(FDC_SOFTPRI); }

/*
 * The Floppy Control Register on the sun3x, not to be confused with the
 * Floppy ControllER Registers that this driver mostly insterfaces with,
 * controls some of the auxillary functions of the floppy drive.  These
 * include asserting the floppy eject and terminal data count (or TC) pins
 * of the floppy drive and controller chip respectively.
 *
 * Often it is necessary to toggle individual bits within this register
 * while keeping the others untouched.  However, the register does not
 * present its latched data to the processor when read.  This prevents the
 * use of a read-modify-write cycle that would normally be used to modify
 * individual bits.  To get around this we must keep a copy of register's
 * current value and always insure that when we wish to modify the register,
 * we actually modify the copy and synchronize the register to it.
 */
#define	FCR_REG_SYNC()	(*fdc->sc_reg_fcr = fdc->sc_fcr)

int
fdcmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct confargs *ca = aux;

	if (bus_peek(ca->ca_bustype, ca->ca_paddr, sizeof(u_char)) == -1)
		return (0);

	return (1);
}

/*
 * Arguments passed between fdcattach and fdprobe.
 */
struct fdc_attach_args {
	int fa_drive;
	struct bootpath *fa_bootpath;
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
fdprint(aux, fdc)
	void *aux;
	const char *fdc;
{
	register struct fdc_attach_args *fa = aux;

	if (!fdc)
		printf(" drive %d", fa->fa_drive);
	return (QUIET);
}

static void
fdconf(fdc)
	struct fdc_softc *fdc;
{
	int	vroom;

	if (out_fdc(fdc, NE7CMD_DUMPREG) || fdcresult(fdc) != 10)
		return;

	/*
	 * dumpreg[7] seems to be a motor-off timeout; set it to whatever
	 * the PROM thinks is appropriate.
	 */
	if ((vroom = fdc->sc_status[7]) == 0)
		vroom = 0x64;

	/* Configure controller to use FIFO and Implied Seek */
	out_fdc(fdc, NE7CMD_CFG);
	out_fdc(fdc, vroom);
	out_fdc(fdc, fdc->sc_cfg);
	out_fdc(fdc, 0); /* PRETRK */
	/* No result phase */
}

void
fdcattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct confargs *ca = aux;
	struct fdc_softc *fdc = (void *)self;
	struct fdc_attach_args fa;
	int pri, vec;
	char code;

	fdc->sc_reg = (caddr_t)bus_mapin(ca->ca_bustype, ca->ca_paddr,
		sizeof(union fdreg));

	callout_init(&fdc->sc_timo_ch);
	callout_init(&fdc->sc_intr_ch);

	fdc->sc_state = DEVIDLE;
	fdc->sc_istate = ISTATE_IDLE;
	fdc->sc_flags |= FDC_EIS;
	TAILQ_INIT(&fdc->sc_drives);

	/* Assume a 82072 */
	code = '2';

	if (code == '7') {
		panic("no 82077 fdc in this kernel");
		/* NOTREACHED */
	} else {
		fdc->sc_reg_msr = &((struct fdreg_72 *)fdc->sc_reg)->fd_msr;
		fdc->sc_reg_fifo = &((struct fdreg_72 *)fdc->sc_reg)->fd_fifo;

		fdc->sc_reg_fcr = ((volatile u_int8_t *) fdc->sc_reg)
			+ FDC_FCR_OFFSET;
		fdc->sc_reg_fvr = ((volatile u_int8_t *) fdc->sc_reg)
			+ FDC_FVR_OFFSET;
	}

	isr_add_autovect(fdcswintr, fdc, FDC_SOFTPRI);
	pri = ca->ca_intpri;
	vec = ca->ca_intvec;
	if (vec == -1) {
		/* Tell the FDC to fake an autovector. */
		vec = 0x18 + pri; /* XXX */
		isr_add_autovect(fdchwintr, fdc, pri);
	} else {
		/* An OBIO bus with vectors?  Weird exception. */
		isr_add_vectored(fdchwintr, fdc, pri, vec);
	}
	*fdc->sc_reg_fvr = vec;	/* Program controller w/ interrupt vector */

	printf(": (softpri %d) chip 8207%c\n", FDC_SOFTPRI, code);

#ifdef FD_DEBUG
	if (out_fdc(fdc, NE7CMD_VERSION) == 0 &&
	    fdcresult(fdc) == 1 && fdc->sc_status[0] == 0x90) {
		if (fdc_debug)
			printf("[version cmd]");
	}
#endif

	fdc_reset(fdc);
	/*
	 * Configure controller; enable FIFO, Implied seek, no POLL mode?.
	 * Note: CFG_EFIFO is active-low, initial threshold value: 8
	 */
	fdc->sc_cfg = CFG_EIS|/*CFG_EFIFO|*/CFG_POLL|(8 & CFG_THRHLD_MASK);
	fdconf(fdc);

	evcnt_attach_dynamic(&fdc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    fdc->sc_dev.dv_xname, "intr");

	/* physical limit: four drives per controller. */
	for (fa.fa_drive = 0; fa.fa_drive < 4; fa.fa_drive++) {
		fa.fa_deftype = NULL;		/* unknown */
	fa.fa_deftype = &fd_types[0];		/* XXX */
		(void)config_found(self, (void *)&fa, fdprint);
	}
}

int
fdmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct fdc_softc *fdc = (void *)parent;
	struct fdc_attach_args *fa = aux;
	int drive = fa->fa_drive;
	int n, ok;

	if (drive > 0)
		/* XXX - for now, punt > 1 drives */
		return (0);

	/* select drive and turn on motor */
	fdc->sc_fcr |= FCR_DSEL(drive) | FCR_MTRON;
	FCR_REG_SYNC();
	/* wait for motor to spin up */
	delay(250000);

	fdc->sc_nstat = 0;
	out_fdc(fdc, NE7CMD_RECAL);
	out_fdc(fdc, drive);
	/* wait for recalibrate */
	for (n = 0; n < 10000; n++) {
		delay(1000);
		if ((*fdc->sc_reg_msr & (NE7_RQM|NE7_DIO|NE7_CB)) == NE7_RQM) {
			/* wait a bit longer till device *really* is ready */
			delay(100000);
			if (out_fdc(fdc, NE7CMD_SENSEI))
				break;
			if (fdcresult(fdc) == 1 && fdc->sc_status[0] == 0x80)
				/*
				 * Got `invalid command'; we interpret it
				 * to mean that the re-calibrate hasn't in
				 * fact finished yet
				 */
				continue;
			break;
		}
	}
	n = fdc->sc_nstat;
#ifdef FD_DEBUG
	if (fdc_debug) {
		int i;
		printf("fdprobe: %d stati:", n);
		for (i = 0; i < n; i++)
			printf(" %x", fdc->sc_status[i]);
		printf("\n");
	}
#endif
	ok = (n == 2 && (fdc->sc_status[0] & 0xf8) == 0x20) ? 1 : 0;

	/* turn off motor */
	fdc->sc_fcr &= ~(FCR_DSEL(drive)|FCR_MTRON);
	FCR_REG_SYNC();

	return (ok);
}

/*
 * Controller is working, and drive responded.  Attach it.
 */
void
fdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fdc_softc *fdc = (void *)parent;
	struct fd_softc *fd = (void *)self;
	struct fdc_attach_args *fa = aux;
	struct fd_type *type = fa->fa_deftype;
	int drive = fa->fa_drive;

	callout_init(&fd->sc_motoron_ch);
	callout_init(&fd->sc_motoroff_ch);

	/* XXX Allow `flags' to override device type? */

	if (type)
		printf(": %s %d cyl, %d head, %d sec\n", type->name,
		    type->tracks, type->heads, type->sectrac);
	else
		printf(": density unknown\n");

	BUFQ_INIT(&fd->sc_q);
	fd->sc_cylin = -1;
	fd->sc_drive = drive;
	fd->sc_deftype = type;
	fdc->sc_fd[drive] = fd;

	/*
	 * Initialize and attach the disk structure.
	 */
	fd->sc_dk.dk_name = fd->sc_dv.dv_xname;
	fd->sc_dk.dk_driver = &fddkdriver;
	disk_attach(&fd->sc_dk);

#ifdef	sparc
	/*
	 * We're told if we're the boot device in fdcattach().
	 */
	if (fa->fa_bootpath)
		fa->fa_bootpath->dev = &fd->sc_dv;
#endif
#define	OUT_FDC(sc, c)	{                                \
	if (out_fdc((sc), (c)))                          \
		printf("fdc: specify command failed.\n");\
	}
	/* specify command */
	OUT_FDC(fdc, NE7CMD_SPECIFY);
	OUT_FDC(fdc, type->steprate);
	/*
	 * The '|1' in the following statement turns on the 'Non-DMA' bit
	 * specifier in the last byte of the SPECIFY command as described in the
	 * datasheet I have.  This is necessary for the driver to work on the
	 * sun3x, because the system will not respond to the chip's requests
	 * for DMA; there is no hardware on the motherboard to support it.
	 * By enabling this bit, we will force the chip to interrupt when its
	 * FIFO is full, at which point the interrupt handler will empty it and
	 * continue.  This is ``pseudo-DMA''.
	 * -J
	 */
	OUT_FDC(fdc, 6|1);	/* XXX head load time == 6ms */
#undef	OUT_FDC

	/*
	 * Establish a mountroot_hook anyway in case we booted
	 * with RB_ASKNAME and get selected as the boot device.
	 */
	mountroothook_establish(fd_mountroot_hook, &fd->sc_dv);

	/* Make sure the drive motor gets turned off at shutdown time. */
	fd->sc_sdhook = shutdownhook_establish(fd_motor_off, fd);
}

__inline struct fd_type *
fd_dev_to_type(fd, dev)
	struct fd_softc *fd;
	dev_t dev;
{
	int type = FDTYPE(dev);

	if (type > (sizeof(fd_types) / sizeof(fd_types[0])))
		return (NULL);
	return (type ? &fd_types[type - 1] : fd->sc_deftype);
}

void
fdstrategy(bp)
	register struct buf *bp;	/* IO operation to perform */
{
	struct fd_softc *fd;
	int unit = FDUNIT(bp->b_dev);
	int sz;
 	int s;

	/* Valid unit, controller, and request? */
	if (unit >= fd_cd.cd_ndevs ||
	    (fd = fd_cd.cd_devs[unit]) == 0 ||
	    bp->b_blkno < 0 ||
	    ((bp->b_bcount % FDC_BSIZE) != 0 &&
	     (bp->b_flags & B_FORMAT) == 0)) {
		bp->b_error = EINVAL;
		goto bad;
	}

	/* If it's a null transfer, return immediately. */
	if (bp->b_bcount == 0)
		goto done;

	sz = howmany(bp->b_bcount, FDC_BSIZE);

	if (bp->b_blkno + sz > fd->sc_type->size) {
		sz = fd->sc_type->size - bp->b_blkno;
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
 	bp->b_cylinder = bp->b_blkno / (FDC_BSIZE / DEV_BSIZE) / fd->sc_type->seccyl;

#ifdef FD_DEBUG
	if (fdc_debug > 1)
	    printf("fdstrategy: b_blkno %d b_bcount %ld blkno %d cylin %ld\n",
		    bp->b_blkno, bp->b_bcount, fd->sc_blkno, bp->b_cylinder);
#endif

	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();
	disksort_cylinder(&fd->sc_q, bp);
	callout_stop(&fd->sc_motoroff_ch);		/* a good idea */
	if (fd->sc_active == 0)
		fdstart(fd);
#ifdef DIAGNOSTIC
	else {
		struct fdc_softc *fdc = (void *)fd->sc_dv.dv_parent;
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
fdstart(fd)
	struct fd_softc *fd;
{
	struct fdc_softc *fdc = (void *)fd->sc_dv.dv_parent;
	int active = fdc->sc_drives.tqh_first != 0;

	/* Link into controller queue. */
	fd->sc_active = 1;
	TAILQ_INSERT_TAIL(&fdc->sc_drives, fd, sc_drivechain);

	/* If controller not already active, start it. */
	if (!active)
		fdcstart(fdc);
}

void
fdfinish(fd, bp)
	struct fd_softc *fd;
	struct buf *bp;
{
	struct fdc_softc *fdc = (void *)fd->sc_dv.dv_parent;

	/*
	 * Move this drive to the end of the queue to give others a `fair'
	 * chance.  We only force a switch if N operations are completed while
	 * another drive is waiting to be serviced, since there is a long motor
	 * startup delay whenever we switch.
	 */
	if (fd->sc_drivechain.tqe_next && ++fd->sc_ops >= 8) {
		fd->sc_ops = 0;
		TAILQ_REMOVE(&fdc->sc_drives, fd, sc_drivechain);
		if (BUFQ_NEXT(bp) != NULL) {
			TAILQ_INSERT_TAIL(&fdc->sc_drives, fd, sc_drivechain);
		} else
			fd->sc_active = 0;
	}
	bp->b_resid = fd->sc_bcount;
	fd->sc_skip = 0;
	BUFQ_REMOVE(&fd->sc_q, bp);

	biodone(bp);
	/* turn off motor 5s from now */
	callout_reset(&fd->sc_motoroff_ch, 5 * hz, fd_motor_off, fd);
	fdc->sc_state = DEVIDLE;
}

void
fdc_reset(fdc)
	struct fdc_softc *fdc;
{
	fdc->sc_fcr = 0;
	FCR_REG_SYNC();

	*fdc->sc_reg_drs = DRS_RESET;
	delay(10);
	*fdc->sc_reg_drs = 0;

#ifdef FD_DEBUG
	if (fdc_debug)
		printf("fdc reset\n");
#endif
}

void
fd_set_motor(fdc)
	struct fdc_softc *fdc;
{
	struct fd_softc *fd;
	int n;

	int on = 0;

	for (n = 0; n < 4; n++)
		if ((fd = fdc->sc_fd[n]) && (fd->sc_flags & FD_MOTOR))
			on = 1;
	if (on) {
		fdc->sc_fcr |= FCR_DSEL(0)|FCR_MTRON; /* XXX */
	} else {
		fdc->sc_fcr &= ~(FCR_DSEL(0)|FCR_MTRON); /* XXX */
	}
	FCR_REG_SYNC();
}

void
fd_motor_off(arg)
	void *arg;
{
	struct fd_softc *fd = arg;
	int s;

	s = splbio();
	fd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
	fd_set_motor((struct fdc_softc *)fd->sc_dv.dv_parent);
	splx(s);
}

void
fd_motor_on(arg)
	void *arg;
{
	struct fd_softc *fd = arg;
	struct fdc_softc *fdc = (void *)fd->sc_dv.dv_parent;
	int s;

	s = splbio();
	fd->sc_flags &= ~FD_MOTOR_WAIT;
	if ((fdc->sc_drives.tqh_first == fd) && (fdc->sc_state == MOTORWAIT))
		(void) fdcstate(fdc);
	splx(s);
}

int
fdcresult(fdc)
	struct fdc_softc *fdc;
{
	u_char i;
	int j = 100000,
	    n = 0;

	for (; j; j--) {
		i = *fdc->sc_reg_msr & (NE7_DIO | NE7_RQM | NE7_CB);
		if (i == NE7_RQM)
			return (fdc->sc_nstat = n);
		if (i == (NE7_DIO | NE7_RQM | NE7_CB)) {
			if (n >= sizeof(fdc->sc_status)) {
				log(LOG_ERR, "fdcresult: overrun\n");
				return (-1);
			}
			fdc->sc_status[n++] = *fdc->sc_reg_fifo;
		} else
			delay(10);
	}
	log(LOG_ERR, "fdcresult: timeout\n");
	return (fdc->sc_nstat = -1);
}

int
out_fdc(fdc, x)
	struct fdc_softc *fdc;
	u_char x;
{
	int i = 100000;

	while (((*fdc->sc_reg_msr & (NE7_DIO|NE7_RQM)) != NE7_RQM) && i-- > 0)
		delay(1);
	if (i <= 0)
		return (-1);

	*fdc->sc_reg_fifo = x;
	return (0);
}

int
fdopen(dev, flags, fmt, p)
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
 	int unit, pmask;
	struct fd_softc *fd;
	struct fd_type *type;

	unit = FDUNIT(dev);
	if (unit >= fd_cd.cd_ndevs)
		return (ENXIO);
	fd = fd_cd.cd_devs[unit];
	if (fd == 0)
		return (ENXIO);
	type = fd_dev_to_type(fd, dev);
	if (type == NULL)
		return (ENXIO);

	if ((fd->sc_flags & FD_OPEN) != 0 &&
	    fd->sc_type != type)
		return (EBUSY);

	fd->sc_type = type;
	fd->sc_cylin = -1;
	fd->sc_flags |= FD_OPEN;

	/*
	 * Only update the disklabel if we're not open anywhere else.
	 */
	if (fd->sc_dk.dk_openmask == 0)
		fdgetdisklabel(dev);

	pmask = (1 << DISKPART(dev));

	switch (fmt) {
	case S_IFCHR:
		fd->sc_dk.dk_copenmask |= pmask;
		break;

	case S_IFBLK:
		fd->sc_dk.dk_bopenmask |= pmask;
		break;
	}
	fd->sc_dk.dk_openmask =
	    fd->sc_dk.dk_copenmask | fd->sc_dk.dk_bopenmask;

	return (0);
}

int
fdclose(dev, flags, fmt, p)
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	struct fd_softc *fd = fd_cd.cd_devs[FDUNIT(dev)];
	int pmask = (1 << DISKPART(dev));

	fd->sc_flags &= ~FD_OPEN;
	fd->sc_opts &= ~(FDOPT_NORETRY|FDOPT_SILENT);

	switch (fmt) {
	case S_IFCHR:
		fd->sc_dk.dk_copenmask &= ~pmask;
		break;

	case S_IFBLK:
		fd->sc_dk.dk_bopenmask &= ~pmask;
		break;
	}
	fd->sc_dk.dk_openmask =
	    fd->sc_dk.dk_copenmask | fd->sc_dk.dk_bopenmask;

	return (0);
}

int
fdread(dev, uio, flag)
        dev_t dev;
        struct uio *uio;
	int flag;
{

        return (physio(fdstrategy, NULL, dev, B_READ, minphys, uio));
}

int
fdwrite(dev, uio, flag)
        dev_t dev;
        struct uio *uio;
	int flag;
{

        return (physio(fdstrategy, NULL, dev, B_WRITE, minphys, uio));
}

void
fdcstart(fdc)
	struct fdc_softc *fdc;
{

#ifdef DIAGNOSTIC
	/* only got here if controller's drive queue was inactive; should
	   be in idle state */
	if (fdc->sc_state != DEVIDLE) {
		printf("fdcstart: not idle\n");
		return;
	}
#endif
	(void) fdcstate(fdc);
}

void
fdcstatus(dv, n, s)
	struct device *dv;
	int n;
	char *s;
{
	struct fdc_softc *fdc = (void *)dv->dv_parent;
	char bits[64];
#if 0
	/*
	 * A 82072 seems to return <invalid command> on
	 * gratuitous Sense Interrupt commands.
	 */
	if (n == 0 && (fdc->sc_flags & FDC_82077)) {
		out_fdc(fdc, NE7CMD_SENSEI);
		(void) fdcresult(fdc);
		n = 2;
	}
#endif

	/* Just print last status */
	n = fdc->sc_nstat;

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
fdctimeout(arg)
	void *arg;
{
	struct fdc_softc *fdc = arg;
	struct fd_softc *fd = fdc->sc_drives.tqh_first;
	int s;

	s = splbio();
	fdcstatus(&fd->sc_dv, 0, "timeout");

	if (BUFQ_FIRST(&fd->sc_q) != NULL)
		fdc->sc_state++;
	else
		fdc->sc_state = DEVIDLE;

	(void) fdcstate(fdc);
	splx(s);
}

void
fdcpseudointr(arg)
	void *arg;
{
	struct fdc_softc *fdc = arg;
	int s;

	/* Just ensure it has the right spl. */
	s = splbio();
	(void) fdcstate(fdc);
	splx(s);
}


/*
 * hardware interrupt entry point: must be converted to `fast'
 * (in-window) handler.
 */
int
fdchwintr(arg)
	void *arg;
{
	struct fdc_softc *fdc = arg;

	/*
	 * This code was reverse engineered from the SPARC bsd_fdintr.s.
	 */
	switch (fdc->sc_istate) {
	case ISTATE_IDLE:
		return (0);
	case ISTATE_SENSEI:
		out_fdc(fdc, NE7CMD_SENSEI);
		fdcresult(fdc);
		fdc->sc_istate = ISTATE_DONE;
		FD_SET_SWINTR();
		return (1);
	case ISTATE_DMA:
		break;
	default:
		log(LOG_ERR, "fdc: stray hard interrupt.\n");
		fdc->sc_fcr &= ~(FCR_DSEL(0));	/* Does this help? */
		fdc->sc_istate = ISTATE_SPURIOUS;
		FD_SET_SWINTR();
		return (1);
	}

	for (;;) {
		register int msr;

		msr = *fdc->sc_reg_msr;

		if ((msr & NE7_RQM) == 0)
			break;

		if ((msr & NE7_NDM) == 0) {
			fdcresult(fdc);
			fdc->sc_istate = ISTATE_DONE;
			FD_SET_SWINTR();
			log(LOG_ERR, "fdc: overrun: tc = %d\n", fdc->sc_tc);
			break;
		}

		if (msr & NE7_DIO) {
			*fdc->sc_data++ = *fdc->sc_reg_fifo;
		} else {
			*fdc->sc_reg_fifo = *fdc->sc_data++;
		}
		if (--fdc->sc_tc == 0) {
			fdc->sc_fcr |= FCR_TC;
			FCR_REG_SYNC();
			fdc->sc_istate = ISTATE_DONE;
			delay(10);
			fdc->sc_fcr &= ~FCR_TC;
			FCR_REG_SYNC();
			fdcresult(fdc);
			FD_SET_SWINTR();
			break;
		}
	}
	return (1);
}

int
fdcswintr(arg)
	void *arg;
{
	struct fdc_softc *fdc = arg;
	int s;

	if (fdc_softpend == 0)
		return (0);

	isr_soft_clear(FDC_SOFTPRI);
	fdc_softpend = 0;

	if (fdc->sc_istate != ISTATE_DONE)
		return (0);

	fdc->sc_istate = ISTATE_IDLE;
	s = splbio();
	fdcstate(fdc);
	splx(s);
	return (1);
}

int
fdcstate(fdc)
	struct fdc_softc *fdc;
{
#define	st0	fdc->sc_status[0]
#define	st1	fdc->sc_status[1]
#define	cyl	fdc->sc_status[1]
#define OUT_FDC(fdc, c, s) \
    do { if (out_fdc(fdc, (c))) { (fdc)->sc_state = (s); goto loop; } } while(0)

	struct fd_softc *fd;
	struct buf *bp;
	int read, head, sec, nblks;
	struct fd_type *type;
	struct ne7_fd_formb *finfo = NULL;


	if (fdc->sc_istate != ISTATE_IDLE) {
		/* Trouble... */
		printf("fdc: spurious interrupt: state %d, istate=%d\n",
			fdc->sc_state, fdc->sc_istate);
		fdc->sc_istate = ISTATE_IDLE;
		if (fdc->sc_state == RESETCOMPLETE ||
		    fdc->sc_state == RESETTIMEDOUT) {
			panic("fdcintr: spurious interrupt can't be cleared");
		}
		goto doreset;
	}

loop:
	/* Is there a drive for the controller to do a transfer with? */
	fd = fdc->sc_drives.tqh_first;
	if (fd == NULL) {
		fdc->sc_state = DEVIDLE;
 		return (0);
	}

	/* Is there a transfer to this drive?  If not, deactivate drive. */
	bp = BUFQ_FIRST(&fd->sc_q);
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
		fdc->sc_errors = 0;
		fd->sc_skip = 0;
		fd->sc_bcount = bp->b_bcount;
		fd->sc_blkno = bp->b_blkno / (FDC_BSIZE / DEV_BSIZE);
		callout_stop(&fd->sc_motoroff_ch);
		if ((fd->sc_flags & FD_MOTOR_WAIT) != 0) {
			fdc->sc_state = MOTORWAIT;
			return (1);
		}
		if ((fd->sc_flags & FD_MOTOR) == 0) {
			/* Turn on the motor, being careful about pairing. */
			struct fd_softc *ofd = fdc->sc_fd[fd->sc_drive ^ 1];
			if (ofd && ofd->sc_flags & FD_MOTOR) {
				callout_stop(&ofd->sc_motoroff_ch);
				ofd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
			}
			fd->sc_flags |= FD_MOTOR | FD_MOTOR_WAIT;
			fd_set_motor(fdc);
			fdc->sc_state = MOTORWAIT;
			if (fdc->sc_flags & FDC_82077) { /* XXX */
				/* Allow .25s for motor to stabilize. */
				callout_reset(&fd->sc_motoron_ch, hz / 4,
				    fd_motor_on, fd);
			} else {
				fd->sc_flags &= ~FD_MOTOR_WAIT;
				goto loop;
			}
			return (1);
		}
		/* Make sure the right drive is selected. */
		fd_set_motor(fdc);

		/*FALLTHROUGH*/
	case DOSEEK:
	doseek:
		if ((fdc->sc_flags & FDC_EIS) &&
		    (bp->b_flags & B_FORMAT) == 0) {
			fd->sc_cylin = bp->b_cylinder;
			/* We use implied seek */
			goto doio;
		}

		if (fd->sc_cylin == bp->b_cylinder)
			goto doio;

		/* specify command */
		OUT_FDC(fdc, NE7CMD_SPECIFY, SEEKTIMEDOUT);
		OUT_FDC(fdc, fd->sc_type->steprate, SEEKTIMEDOUT);
		OUT_FDC(fdc, 6|1, SEEKTIMEDOUT);	/* XXX head load time == 6ms */

		fdc->sc_istate = ISTATE_SENSEI;
		/* seek function */
		OUT_FDC(fdc, NE7CMD_SEEK, SEEKTIMEDOUT);
		OUT_FDC(fdc, fd->sc_drive, SEEKTIMEDOUT); /* drive number */
		OUT_FDC(fdc, bp->b_cylinder * fd->sc_type->step, SEEKTIMEDOUT);

		fd->sc_cylin = -1;
		fdc->sc_state = SEEKWAIT;
		fdc->sc_nstat = 0;

		fd->sc_dk.dk_seek++;
		disk_busy(&fd->sc_dk);

		callout_reset(&fdc->sc_timo_ch, 4 * hz, fdctimeout, fdc);
		return (1);

	case DOIO:
	doio:
#ifdef	NOTYET
		/* Check to see if the disk has changed */
		if (fdc->sc_reg_dir & FDI_DCHG) {
			/*
			 * The disk in the drive has changed since
			 * the last transfer.  We need to see if its geometry
			 * has changed.
			 */
		}
#endif	/* NOTYET */

		if (finfo)
			fd->sc_skip = (char *)&(finfo->fd_formb_cylno(0)) -
				      (char *)finfo;
		type = fd->sc_type;
		sec = fd->sc_blkno % type->seccyl;
		nblks = type->seccyl - sec;
		nblks = min(nblks, fd->sc_bcount / FDC_BSIZE);
		nblks = min(nblks, FDC_MAXIOSIZE / FDC_BSIZE);
		fd->sc_nblks = nblks;
		fd->sc_nbytes = finfo ? bp->b_bcount : nblks * FDC_BSIZE;
		head = sec / type->sectrac;
		sec -= head * type->sectrac;
#ifdef DIAGNOSTIC
		{int block;
		 block = (fd->sc_cylin * type->heads + head) * type->sectrac + sec;
		 if (block != fd->sc_blkno) {
			 printf("fdcintr: block %d != blkno %d\n", block, fd->sc_blkno);
#ifdef DDB
			 Debugger();
#endif
		 }}
#endif
		read = bp->b_flags & B_READ;

		/* Setup for pseudo DMA */
		fdc->sc_data = bp->b_data + fd->sc_skip;
		fdc->sc_tc = fd->sc_nbytes;

		*fdc->sc_reg_drs = type->rate;
#ifdef FD_DEBUG
		if (fdc_debug > 1)
			printf("fdcintr: %s drive %d track %d head %d sec %d nblks %d\n",
				read ? "read" : "write", fd->sc_drive,
				fd->sc_cylin, head, sec, nblks);
#endif
		fdc->sc_state = IOCOMPLETE;
		fdc->sc_istate = ISTATE_DMA;
		fdc->sc_nstat = 0;
		if (finfo) {
			/* formatting */
			OUT_FDC(fdc, NE7CMD_FORMAT, IOTIMEDOUT);
			OUT_FDC(fdc, (head << 2) | fd->sc_drive, IOTIMEDOUT);
			OUT_FDC(fdc, finfo->fd_formb_secshift, IOTIMEDOUT);
			OUT_FDC(fdc, finfo->fd_formb_nsecs, IOTIMEDOUT);
			OUT_FDC(fdc, finfo->fd_formb_gaplen, IOTIMEDOUT);
			OUT_FDC(fdc, finfo->fd_formb_fillbyte, IOTIMEDOUT);
		} else {
			if (read)
				OUT_FDC(fdc, NE7CMD_READ, IOTIMEDOUT);
			else
				OUT_FDC(fdc, NE7CMD_WRITE, IOTIMEDOUT);
			OUT_FDC(fdc, (head << 2) | fd->sc_drive, IOTIMEDOUT);
			OUT_FDC(fdc, fd->sc_cylin, IOTIMEDOUT);	/*track*/
			OUT_FDC(fdc, head, IOTIMEDOUT);
			OUT_FDC(fdc, sec + 1, IOTIMEDOUT);	/*sector+1*/
			OUT_FDC(fdc, type->secsize, IOTIMEDOUT);/*sector size*/
			OUT_FDC(fdc, type->sectrac, IOTIMEDOUT);/*secs/track*/
			OUT_FDC(fdc, type->gap1, IOTIMEDOUT);	/*gap1 size*/
			OUT_FDC(fdc, type->datalen, IOTIMEDOUT);/*data length*/
		}

		disk_busy(&fd->sc_dk);

		/* allow 2 seconds for operation */
		callout_reset(&fdc->sc_timo_ch, 2 * hz, fdctimeout, fdc);
		return (1);				/* will return later */

	case SEEKWAIT:
		callout_stop(&fdc->sc_timo_ch);
		fdc->sc_state = SEEKCOMPLETE;
		if (fdc->sc_flags & FDC_NEEDHEADSETTLE) {
			/* allow 1/50 second for heads to settle */
			callout_reset(&fdc->sc_intr_ch, hz / 50,
			    fdcpseudointr, fdc);
			return (1);		/* will return later */
		}
		/*FALLTHROUGH*/
	case SEEKCOMPLETE:
		disk_unbusy(&fd->sc_dk, 0);	/* no data on seek */

		/* Make sure seek really happened. */
		if (fdc->sc_nstat != 2 || (st0 & 0xf8) != 0x20 ||
		    cyl != bp->b_cylinder * fd->sc_type->step) {
#ifdef FD_DEBUG
			if (fdc_debug)
				fdcstatus(&fd->sc_dv, 2, "seek failed");
#endif
			fdcretry(fdc);
			goto loop;
		}
		fd->sc_cylin = bp->b_cylinder;
		goto doio;

	case IOTIMEDOUT:
		fdc->sc_fcr |= FCR_TC;
		FCR_REG_SYNC();
		delay(10);
		fdc->sc_fcr &= ~FCR_TC;
		FCR_REG_SYNC();
		(void)fdcresult(fdc);
		/*FALLTHROUGH*/
	case SEEKTIMEDOUT:
	case RECALTIMEDOUT:
	case RESETTIMEDOUT:
		fdcretry(fdc);
		goto loop;

	case IOCOMPLETE: /* IO DONE, post-analyze */
		callout_stop(&fdc->sc_timo_ch);

		disk_unbusy(&fd->sc_dk, (bp->b_bcount - bp->b_resid));

		if (fdc->sc_nstat != 7 || (st0 & 0xf8) != 0 || st1 != 0) {
#ifdef FD_DEBUG
			if (fdc_debug) {
				fdcstatus(&fd->sc_dv, 7,
					bp->b_flags & B_READ
					? "read failed" : "write failed");
				printf("blkno %d nblks %d tc %d\n",
				       fd->sc_blkno, fd->sc_nblks, fdc->sc_tc);
			}
#endif
			if (fdc->sc_nstat == 7 &&
			    (st1 & ST1_OVERRUN) == ST1_OVERRUN) {

				/*
				 * Silently retry overruns if no other
				 * error bit is set. Adjust threshold.
				 */
				int thr = fdc->sc_cfg & CFG_THRHLD_MASK;
				if (thr < 15) {
					thr++;
					fdc->sc_cfg &= ~CFG_THRHLD_MASK;
					fdc->sc_cfg |= (thr & CFG_THRHLD_MASK);
#ifdef FD_DEBUG
					if (fdc_debug)
						printf("fdc: %d -> threshold\n", thr);
#endif
					fdconf(fdc);
					fdc->sc_overruns = 0;
				}
				if (++fdc->sc_overruns < 3) {
					fdc->sc_state = DOIO;
					goto loop;
				}
			}
			fdcretry(fdc);
			goto loop;
		}
		if (fdc->sc_errors) {
			diskerr(bp, "fd", "soft error", LOG_PRINTF,
			    fd->sc_skip / FDC_BSIZE, (struct disklabel *)NULL);
			printf("\n");
			fdc->sc_errors = 0;
		} else {
			if (--fdc->sc_overruns < -20) {
				int thr = fdc->sc_cfg & CFG_THRHLD_MASK;
				if (thr > 0) {
					thr--;
					fdc->sc_cfg &= ~CFG_THRHLD_MASK;
					fdc->sc_cfg |= (thr & CFG_THRHLD_MASK);
#ifdef FD_DEBUG
					if (fdc_debug)
						printf("fdc: %d -> threshold\n", thr);
#endif
					fdconf(fdc);
				}
				fdc->sc_overruns = 0;
			}
		}
		fd->sc_blkno += fd->sc_nblks;
		fd->sc_skip += fd->sc_nbytes;
		fd->sc_bcount -= fd->sc_nbytes;
		if (!finfo && fd->sc_bcount > 0) {
			bp->b_cylinder = fd->sc_blkno / fd->sc_type->seccyl;
			goto doseek;
		}
		fdfinish(fd, bp);
		goto loop;

	case DORESET:
	doreset:
		/* try a reset, keep motor on */
		fd_set_motor(fdc);
		delay(100);
		fdc_reset(fdc);
		fdc->sc_nstat = 0;
		fdc->sc_istate = ISTATE_SENSEI;
		fdc->sc_state = RESETCOMPLETE;
		callout_reset(&fdc->sc_timo_ch, hz / 2, fdctimeout, fdc);
		return (1);			/* will return later */

	case RESETCOMPLETE:
		callout_stop(&fdc->sc_timo_ch);
		fdconf(fdc);

		/* fall through */
	case DORECAL:
		fdc->sc_state = RECALWAIT;
		fdc->sc_istate = ISTATE_SENSEI;
		fdc->sc_nstat = 0;
		/* recalibrate function */
		OUT_FDC(fdc, NE7CMD_RECAL, RECALTIMEDOUT);
		OUT_FDC(fdc, fd->sc_drive, RECALTIMEDOUT);
		callout_reset(&fdc->sc_timo_ch, 5 * hz, fdctimeout, fdc);
		return (1);			/* will return later */

	case RECALWAIT:
		callout_stop(&fdc->sc_timo_ch);
		fdc->sc_state = RECALCOMPLETE;
		if (fdc->sc_flags & FDC_NEEDHEADSETTLE) {
			/* allow 1/30 second for heads to settle */
			callout_reset(&fdc->sc_intr_ch, hz / 30,
			    fdcpseudointr, fdc);
			return (1);		/* will return later */
		}

	case RECALCOMPLETE:
		if (fdc->sc_nstat != 2 || (st0 & 0xf8) != 0x20 || cyl != 0) {
#ifdef FD_DEBUG
			if (fdc_debug)
				fdcstatus(&fd->sc_dv, 2, "recalibrate failed");
#endif
			fdcretry(fdc);
			goto loop;
		}
		fd->sc_cylin = 0;
		goto doseek;

	case MOTORWAIT:
		if (fd->sc_flags & FD_MOTOR_WAIT)
			return (1);		/* time's not up yet */
		goto doseek;

	default:
		fdcstatus(&fd->sc_dv, 0, "stray interrupt");
		return (1);
	}
#ifdef DIAGNOSTIC
	panic("fdcintr: impossible");
#endif
#undef	st0
#undef	st1
#undef	cyl
}

void
fdcretry(fdc)
	struct fdc_softc *fdc;
{
	char bits[64];
	struct fd_softc *fd;
	struct buf *bp;

	fd = fdc->sc_drives.tqh_first;
	bp = BUFQ_FIRST(&fd->sc_q);

	fdc->sc_overruns = 0;
	if (fd->sc_opts & FDOPT_NORETRY)
		goto fail;

	switch (fdc->sc_errors) {
	case 0:
		/* try again */
		fdc->sc_state =
			(fdc->sc_flags & FDC_EIS) ? DOIO : DOSEEK;
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
	fail:
		if ((fd->sc_opts & FDOPT_SILENT) == 0) {
			diskerr(bp, "fd", "hard error", LOG_PRINTF,
				fd->sc_skip / FDC_BSIZE,
				(struct disklabel *)NULL);

			printf(" (st0 %s", bitmask_snprintf(fdc->sc_status[0],
				NE7_ST0BITS, bits, sizeof(bits)));
			printf(" st1 %s", bitmask_snprintf(fdc->sc_status[1],
				NE7_ST1BITS, bits, sizeof(bits)));
			printf(" st2 %s", bitmask_snprintf(fdc->sc_status[2],
				NE7_ST2BITS, bits, sizeof(bits)));
			printf(" cyl %d head %d sec %d)\n",
				fdc->sc_status[3], fdc->sc_status[4],
				fdc->sc_status[5]);
		}

		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		fdfinish(fd, bp);
	}
	fdc->sc_errors++;
}

int
fdsize(dev)
	dev_t dev;
{

	/* Swapping to floppies would not make sense. */
	return (-1);
}

int
fddump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{

	/* Not implemented. */
	return (EINVAL);
}

int
fdioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct fd_softc *fd = fd_cd.cd_devs[FDUNIT(dev)];
	struct fdformat_parms *form_parms;
	struct fdformat_cmd *form_cmd;
	struct ne7_fd_formb fd_formb;
	int il[FD_MAX_NSEC + 1];
	int i, j;
	int error;

	switch (cmd) {
	case DIOCGDINFO:
		*(struct disklabel *)addr = *(fd->sc_dk.dk_label);
		return 0;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return EBADF;
		/* XXX do something */
		return (0);

	case DIOCWDINFO:
		if ((flag & FWRITE) == 0)
			return (EBADF);

		error = setdisklabel(fd->sc_dk.dk_label,
				    (struct disklabel *)addr, 0,
				    fd->sc_dk.dk_cpulabel);
		if (error)
			return (error);

		error = writedisklabel(dev, fdstrategy,
				       fd->sc_dk.dk_label,
				       fd->sc_dk.dk_cpulabel);
		return (error);

	case DIOCLOCK:
		/*
		 * Nothing to do here, really.
		 */
		return (0);

	case DIOCEJECT:
		if (*(int *)addr == 0) {
			int part = DISKPART(dev);
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
		fd_do_eject((void *)fd->sc_dv.dv_parent, fd->sc_drive);
		return (0);

	case FDIOCGETFORMAT:
		form_parms = (struct fdformat_parms *)addr;
		form_parms->fdformat_version = FDFORMAT_VERSION;
		form_parms->nbps = 128 * (1 << fd->sc_type->secsize);
		form_parms->ncyl = fd->sc_type->tracks;
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
			return (EINVAL);
		}
		return (0);

	case FDIOCSETFORMAT:
		if ((flag & FWRITE) == 0)
			return (EBADF);	/* must be opened for writing */

		form_parms = (struct fdformat_parms *)addr;
		if (form_parms->fdformat_version != FDFORMAT_VERSION)
			return (EINVAL);/* wrong version of formatting prog */

		i = form_parms->nbps >> 7;
		if ((form_parms->nbps & 0x7f) || ffs(i) == 0 ||
		    i & ~(1 << (ffs(i)-1)))
			/* not a power-of-two multiple of 128 */
			return (EINVAL);

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
			return (EINVAL);
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
		fd->sc_type->secsize = ffs(i)-1;
		fd->sc_type->gap2 = form_parms->gaplen;
		fd->sc_type->tracks = form_parms->ncyl;
		fd->sc_type->size = fd->sc_type->seccyl * form_parms->ncyl *
			form_parms->nbps / DEV_BSIZE;
		fd->sc_type->step = form_parms->stepspercyl;
		fd->sc_type->fillbyte = form_parms->fillbyte;
		fd->sc_type->interleave = form_parms->interleave;
		return (0);

	case FDIOCFORMAT_TRACK:
		if((flag & FWRITE) == 0)
			/* must be opened for writing */
			return (EBADF);
		form_cmd = (struct fdformat_cmd *)addr;
		if (form_cmd->formatcmd_version != FDFORMAT_VERSION)
			/* wrong version of formatting prog */
			return (EINVAL);

		if (form_cmd->head >= fd->sc_type->heads ||
		    form_cmd->cylinder >= fd->sc_type->tracks) {
			return (EINVAL);
		}

		fd_formb.head = form_cmd->head;
		fd_formb.cyl = form_cmd->cylinder;
		fd_formb.transfer_rate = fd->sc_type->rate;
		fd_formb.fd_formb_secshift = fd->sc_type->secsize;
		fd_formb.fd_formb_nsecs = fd->sc_type->sectrac;
		fd_formb.fd_formb_gaplen = fd->sc_type->gap2;
		fd_formb.fd_formb_fillbyte = fd->sc_type->fillbyte;

		bzero(il, sizeof(il));
		for (j = 0, i = 1; i <= fd_formb.fd_formb_nsecs; i++) {
			while (il[(j%fd_formb.fd_formb_nsecs) + 1])
				j++;
			il[(j%fd_formb.fd_formb_nsecs) + 1] = i;
			j += fd->sc_type->interleave;
		}
		for (i = 0; i < fd_formb.fd_formb_nsecs; i++) {
			fd_formb.fd_formb_cylno(i) = form_cmd->cylinder;
			fd_formb.fd_formb_headno(i) = form_cmd->head;
			fd_formb.fd_formb_secno(i) = il[i+1];
			fd_formb.fd_formb_secsize(i) = fd->sc_type->secsize;
		}

		return fdformat(dev, &fd_formb, p);

	case FDIOCGETOPTS:		/* get drive options */
		*(int *)addr = fd->sc_opts;
		return (0);

	case FDIOCSETOPTS:		/* set drive options */
		fd->sc_opts = *(int *)addr;
		return (0);

#ifdef DEBUG
	case _IO('f', 100):
		{
		int i;
		struct fdc_softc *fdc = (struct fdc_softc *)
					fd->sc_dv.dv_parent;

		out_fdc(fdc, NE7CMD_DUMPREG);
		fdcresult(fdc);
		printf("dumpreg(%d regs): <", fdc->sc_nstat);
		for (i = 0; i < fdc->sc_nstat; i++)
			printf(" %x", fdc->sc_status[i]);
		printf(">\n");
		}

		return (0);
	case _IOW('f', 101, int):
		((struct fdc_softc *)fd->sc_dv.dv_parent)->sc_cfg &=
			~CFG_THRHLD_MASK;
		((struct fdc_softc *)fd->sc_dv.dv_parent)->sc_cfg |=
			(*(int *)addr & CFG_THRHLD_MASK);
		fdconf((struct fdc_softc *) fd->sc_dv.dv_parent);
		return (0);
	case _IO('f', 102):
		{
		int i;
		struct fdc_softc *fdc = (struct fdc_softc *)
					fd->sc_dv.dv_parent;
		out_fdc(fdc, NE7CMD_SENSEI);
		fdcresult(fdc);
		printf("sensei(%d regs): <", fdc->sc_nstat);
		for (i=0; i< fdc->sc_nstat; i++)
			printf(" 0x%x", fdc->sc_status[i]);
		}
		printf(">\n");
		return (0);
#endif
	default:
		return (ENOTTY);
	}

#ifdef DIAGNOSTIC
	panic("fdioctl: impossible");
#endif
}

int
fdformat(dev, finfo, p)
	dev_t dev;
	struct ne7_fd_formb *finfo;
	struct proc *p;
{
	int rv = 0, s;
	struct fd_softc *fd = fd_cd.cd_devs[FDUNIT(dev)];
	struct fd_type *type = fd->sc_type;
	struct buf *bp;

	/* set up a buffer header for fdstrategy() */
	bp = (struct buf *)malloc(sizeof(struct buf), M_TEMP, M_NOWAIT);
	if (bp == 0)
		return (ENOBUFS);

	PHOLD(p);
	bzero((void *)bp, sizeof(struct buf));
	bp->b_flags = B_BUSY | B_PHYS | B_FORMAT;
	bp->b_proc = p;
	bp->b_dev = dev;

	/*
	 * Calculate a fake blkno, so fdstrategy() would initiate a
	 * seek to the requested cylinder.
	 */
	bp->b_blkno = (finfo->cyl * (type->sectrac * type->heads)
		       + finfo->head * type->sectrac) * FDC_BSIZE / DEV_BSIZE;

	bp->b_bcount = sizeof(struct fd_idfield_data) * finfo->fd_formb_nsecs;
	bp->b_data = (caddr_t)finfo;

#ifdef FD_DEBUG
	if (fdc_debug)
		printf("fdformat: blkno %x count %ld\n",
			bp->b_blkno, bp->b_bcount);
#endif

	/* now do the format */
	fdstrategy(bp);

	/* ...and wait for it to complete */
	s = splbio();
	while (!(bp->b_flags & B_DONE)) {
		rv = tsleep((caddr_t)bp, PRIBIO, "fdform", 20 * hz);
		if (rv == EWOULDBLOCK)
			break;
	}
	splx(s);

	if (rv == EWOULDBLOCK) {
		/* timed out */
		rv = EIO;
		biodone(bp);
	}
	if (bp->b_flags & B_ERROR) {
		rv = bp->b_error;
	}
	PRELE(p);
	free(bp, M_TEMP);
	return (rv);
}

void
fdgetdisklabel(dev)
	dev_t dev;
{
	int unit = FDUNIT(dev), i;
	struct fd_softc *fd = fd_cd.cd_devs[unit];
	struct disklabel *lp = fd->sc_dk.dk_label;
	struct cpu_disklabel *clp = fd->sc_dk.dk_cpulabel;

	bzero(lp, sizeof(struct disklabel));
	bzero(lp, sizeof(struct cpu_disklabel));

	lp->d_type = DTYPE_FLOPPY;
	lp->d_secsize = FDC_BSIZE;
	lp->d_secpercyl = fd->sc_type->seccyl;
	lp->d_nsectors = fd->sc_type->sectrac;
	lp->d_ncylinders = fd->sc_type->tracks;
	lp->d_ntracks = fd->sc_type->heads;	/* Go figure... */
	lp->d_rpm = 3600;	/* XXX like it matters... */

	strncpy(lp->d_typename, "floppy", sizeof(lp->d_typename));
	strncpy(lp->d_packname, "fictitious", sizeof(lp->d_packname));
	lp->d_interleave = 1;

	lp->d_partitions[RAW_PART].p_offset = 0;
	lp->d_partitions[RAW_PART].p_size = lp->d_secpercyl * lp->d_ncylinders;
	lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;
	lp->d_npartitions = RAW_PART + 1;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);

	/*
	 * Call the generic disklabel extraction routine.  If there's
	 * not a label there, fake it.
	 */
	if (readdisklabel(dev, fdstrategy, lp, clp) != NULL) {
		strncpy(lp->d_packname, "default label",
		    sizeof(lp->d_packname));
		/*
		 * Reset the partition info; it might have gotten
		 * trashed in readdisklabel().
		 *
		 * XXX Why do we have to do this?  readdisklabel()
		 * should be safe...
		 */
		for (i = 0; i < MAXPARTITIONS; ++i) {
			lp->d_partitions[i].p_offset = 0;
			if (i == RAW_PART) {
				lp->d_partitions[i].p_size =
				    lp->d_secpercyl * lp->d_ncylinders;
				lp->d_partitions[i].p_fstype = FS_BSDFFS;
			} else {
				lp->d_partitions[i].p_size = 0;
				lp->d_partitions[i].p_fstype = FS_UNUSED;
			}
		}
		lp->d_npartitions = RAW_PART + 1;
	}
}

void
fd_do_eject(fdc, unit)
	struct fdc_softc *fdc;
	int unit;
{
	fdc->sc_fcr |= FCR_DSEL(unit)|FCR_EJECT;
	FCR_REG_SYNC();
	delay(10);
	fdc->sc_fcr &= ~(FCR_DSEL(unit)|FCR_EJECT);
	FCR_REG_SYNC();
}

#ifdef MEMORY_DISK_HOOKS_sun3x_not_yet
int	fd_read_md_image __P((size_t *, caddr_t *));
#endif

/* ARGSUSED */
void
fd_mountroot_hook(dev)
	struct device *dev;
{
	int c;

	fd_do_eject(fdc_cd.cd_devs[0], 0); /* XXX - doesn't check ``dev'' */
	printf("Insert filesystem floppy and press return.");
	for (;;) {
		c = cngetc();
		if ((c == '\r') || (c == '\n')) {
			printf("\n");
			break;
		}
	}
#ifdef MEMORY_DISK_HOOKS_sun3x_not_yet
	{
	extern int (*md_read_image) __P((size_t *, caddr_t *));
	md_read_image = fd_read_md_image;
	}
#endif
}

#ifdef MEMORY_DISK_HOOKS_sun3x_not_yet

#define FDMICROROOTSIZE ((2*18*80) << DEV_BSHIFT)

int
fd_read_md_image(sizep, addrp)
	size_t	*sizep;
	caddr_t	*addrp;
{
	struct buf buf, *bp = &buf;
	dev_t dev;
	off_t offset;
	caddr_t addr;

	dev = makedev(54,0);	/* XXX */

	MALLOC(addr, caddr_t, FDMICROROOTSIZE, M_DEVBUF, M_WAITOK);
	*addrp = addr;

	if (fdopen(dev, 0, S_IFCHR, NULL))
		panic("fd: mountroot: fdopen");

	offset = 0;

	for (;;) {
		bp->b_dev = dev;
		bp->b_error = 0;
		bp->b_resid = 0;
		bp->b_proc = NULL;
		bp->b_flags = B_BUSY | B_PHYS | B_RAW | B_READ;
		bp->b_blkno = btodb(offset);
		bp->b_bcount = DEV_BSIZE;
		bp->b_data = addr;
		fdstrategy(bp);
		while ((bp->b_flags & B_DONE) == 0) {
			tsleep((caddr_t)bp, PRIBIO + 1, "physio", 0);
		}
		if (bp->b_error)
			panic("fd: mountroot: fdread error %d", bp->b_error);

		if (bp->b_resid != 0)
			break;

		addr += DEV_BSIZE;
		offset += DEV_BSIZE;
		if (offset + DEV_BSIZE > FDMICROROOTSIZE)
			break;
	}
	(void)fdclose(dev, 0, S_IFCHR, NULL);
	*sizep = offset;
	fd_do_eject(fdc_cd.cd_devs[0], FDUNIT(dev)); /* XXX */
	return (0);
}
#endif
