/*	$NetBSD: hdfd.c,v 1.57.6.2 2008/01/09 01:45:31 matt Exp $	*/

/*-
 * Copyright (c) 1996 Leo Weppelman
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

/*-
 * Copyright (c) 1993, 1994, 1995, 1996
 *	Charles M. Hannum.  All rights reserved.
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

/*
 * Floppy formatting facilities merged from FreeBSD fd.c driver:
 *	Id: fd.c,v 1.53 1995/03/12 22:40:56 joerg Exp
 * which carries the same copyright/redistribution notice as shown above with
 * the addition of the following statement before the "Redistribution and
 * use ..." clause:
 *
 * Copyright (c) 1993, 1994 by
 *  jc@irbs.UUCP (John Capo)
 *  vak@zebub.msk.su (Serge Vakulenko)
 *  ache@astral.msk.su (Andrew A. Chernov)
 *
 * Copyright (c) 1993, 1994, 1995 by
 *  joerg_wunsch@uriah.sax.de (Joerg Wunsch)
 *  dufault@hda.com (Peter Dufault)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hdfd.c,v 1.57.6.2 2008/01/09 01:45:31 matt Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/malloc.h>
#include <sys/uio.h>
#include <sys/syslog.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/fdio.h>
#include <sys/conf.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/iomap.h>
#include <machine/mfp.h>

#include <atari/dev/hdfdreg.h>
#include <atari/atari/intr.h>
#include <atari/atari/device.h>

#include "locators.h"

/*
 * {b,c}devsw[] function prototypes
 */
dev_type_open(fdopen);
dev_type_close(fdclose);
dev_type_read(fdread);
dev_type_write(fdwrite);
dev_type_ioctl(fdioctl);
dev_type_strategy(fdstrategy);

volatile u_char	*fdio_addr;

#define wrt_fdc_reg(reg, val)	{ fdio_addr[reg] = val; }
#define rd_fdc_reg(reg)		( fdio_addr[reg] )

#define	fdc_ienable()		MFP2->mf_ierb |= IB_DCHG;

/*
 * Interface to the pseudo-DMA handler
 */
void	fddma_intr(void);
void *	fddmaaddr  = NULL;
int	fddmalen   = 0;

extern void	mfp_hdfd_nf __P((void)), mfp_hdfd_fifo __P((void));

/*
 * Argument to fdcintr.....
 */
static void	*intr_arg = NULL; /* XXX: arg. to intr_establish() */



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
};

/* software state, per controller */
struct fdc_softc {
	struct device	sc_dev;		/* boilerplate */

	struct callout sc_timo_ch;	/* timeout callout */
	struct callout sc_intr_ch;	/* pseudo-intr callout */

	struct fd_softc	*sc_fd[4];	/* pointers to children */
	TAILQ_HEAD(drivehead, fd_softc) sc_drives;
	enum fdc_state	sc_state;
	int		sc_errors;	/* number of retries so far */
	int		sc_overruns;	/* number of overruns so far */
	u_char		sc_status[7];	/* copy of registers */
};

/* controller driver configuration */
int	fdcprobe __P((struct device *, struct cfdata *, void *));
int	fdprint __P((void *, const char *));
void	fdcattach __P((struct device *, struct device *, void *));

CFATTACH_DECL(fdc, sizeof(struct fdc_softc),
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
	int	tracks;		/* total num of tracks */
	int	size;		/* size of disk in sectors */
	int	step;		/* steps per cylinder */
	int	rate;		/* transfer speed code */
	u_char	fillbyte;	/* format fill byte */
	u_char	interleave;	/* interleave factor (formatting) */
	const char *name;
};

/*
 * The order of entries in the following table is important -- BEWARE!
 * The order of the types is the same as for the TT/Falcon....
 */
struct fd_type fd_types[] = {
        /* 360kB in 720kB drive */
        {  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,2,FDC_125KBPS,0xf6,1,"360KB"  },
        /* 3.5" 720kB diskette */
        {  9,2,18,2,0xff,0xdf,0x2a,0x50,80,1440,1,FDC_125KBPS,0xf6,1,"720KB"  },
        /* 1.44MB diskette */
        { 18,2,36,2,0xff,0xcf,0x1b,0x6c,80,2880,1,FDC_250KBPS,0xf6,1,"1.44MB" },
};

/* software state, per disk (with up to 4 disks per ctlr) */
struct fd_softc {
	struct device	sc_dev;
	struct disk	sc_dk;

	struct fd_type	*sc_deftype;	/* default type descriptor */
	struct fd_type	*sc_type;	/* current type descriptor */

	struct callout	sc_motoron_ch;
	struct callout	sc_motoroff_ch;

	daddr_t		sc_blkno;	/* starting block number */
	int		sc_bcount;	/* byte count left */
 	int		sc_opts;	/* user-set options */
	int		sc_skip;	/* bytes already transferred */
	int		sc_nblks;	/* #blocks currently transferring */
	int		sc_nbytes;	/* #bytes currently transferring */

	int		sc_drive;	/* physical unit number */
	int		sc_flags;
#define	FD_OPEN		0x01		/* it's open */
#define	FD_MOTOR	0x02		/* motor should be on */
#define	FD_MOTOR_WAIT	0x04		/* motor coming up */
#define	FD_HAVELAB	0x08		/* got a disklabel */
	int		sc_cylin;	/* where we think the head is */

	void		*sc_sdhook;	/* saved shutdown hook for drive. */

	TAILQ_ENTRY(fd_softc) sc_drivechain;
	int		sc_ops;		/* I/O ops since last switch */
	struct bufq_state *sc_q;	/* pending I/O requests */
	int		sc_active;	/* number of active I/O operations */
};

/* floppy driver configuration */
int	fdprobe __P((struct device *, struct cfdata *, void *));
void	fdattach __P((struct device *, struct device *, void *));

CFATTACH_DECL(hdfd, sizeof(struct fd_softc),
    fdprobe, fdattach, NULL, NULL);

extern struct cfdriver hdfd_cd;

const struct bdevsw fd_bdevsw = {
	fdopen, fdclose, fdstrategy, fdioctl, nodump, nosize, D_DISK
};

const struct cdevsw fd_cdevsw = {
	fdopen, fdclose, fdread, fdwrite, fdioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

void	fdstart __P((struct fd_softc *));

struct dkdriver fddkdriver = { fdstrategy };

void	fd_set_motor __P((struct fdc_softc *fdc, int reset));
void	fd_motor_off __P((void *arg));
void	fd_motor_on __P((void *arg));
int	fdcresult __P((struct fdc_softc *fdc));
int	out_fdc __P((u_char x));
void	fdc_ctrl_intr __P((struct clockframe));
void	fdcstart __P((struct fdc_softc *fdc));
void	fdcstatus __P((struct device *dv, int n, const char *s));
void	fdctimeout __P((void *arg));
void	fdcpseudointr __P((void *arg));
int	fdcintr __P((void *));
void	fdcretry __P((struct fdc_softc *fdc));
void	fdfinish __P((struct fd_softc *fd, struct buf *bp));
int	fdformat __P((dev_t, struct ne7_fd_formb *, struct proc *));

static void	fdgetdisklabel __P((struct fd_softc *, dev_t));
static void	fdgetdefaultlabel __P((struct fd_softc *, struct disklabel *,
		    int));

inline struct fd_type *fd_dev_to_type __P((struct fd_softc *, dev_t));

int
fdcprobe(parent, cfp, aux)
	struct device	*parent;
	struct cfdata	*cfp;
	void		*aux;
{
	static int	fdc_matched = 0;
	bus_space_tag_t mb_tag;
	bus_space_handle_t handle;

	/* Match only once */
	if(strcmp("fdc", aux) || fdc_matched)
		return(0);

	if (!atari_realconfig)
		return 0;

	if ((mb_tag = mb_alloc_bus_space_tag()) == NULL)
		return 0;

	if (bus_space_map(mb_tag, FD_IOBASE, FD_IOSIZE, 0, &handle)) {
		printf("fdcprobe: cannot map io-area\n");
		mb_free_bus_space_tag(mb_tag);
		return (0);
	}
	fdio_addr = bus_space_vaddr(mb_tag, handle);	/* XXX */

#ifdef FD_DEBUG
	printf("fdcprobe: I/O mapping done va: %p\n", fdio_addr);
#endif

	/* reset */
	wrt_fdc_reg(fdout, 0);
	delay(100);
	wrt_fdc_reg(fdout, FDO_FRST);

	/* see if it can handle a command */
	if (out_fdc(NE7CMD_SPECIFY) < 0)
		goto out;
	out_fdc(0xdf);
	out_fdc(7);

	fdc_matched = 1;

 out:
	if (fdc_matched == 0) {
		bus_space_unmap(mb_tag, handle, FD_IOSIZE);
		mb_free_bus_space_tag(mb_tag);
	}

	return fdc_matched;
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
fdprint(aux, fdc)
	void *aux;
	const char *fdc;
{
	register struct fdc_attach_args *fa = aux;

	if (!fdc)
		aprint_normal(" drive %d", fa->fa_drive);
	return QUIET;
}

void
fdcattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fdc_softc	*fdc = (void *)self;
	struct fdc_attach_args	fa;
	int			has_fifo;

	has_fifo = 0;

	fdc->sc_state = DEVIDLE;
	TAILQ_INIT(&fdc->sc_drives);

	out_fdc(NE7CMD_CONFIGURE);
	if (out_fdc(0) == 0) {
		out_fdc(0x1a);	/* No polling, fifo depth = 10	*/
		out_fdc(0);

		/* Retain configuration across resets	*/
		out_fdc(NE7CMD_LOCK);
		(void)fdcresult(fdc);
		has_fifo = 1;
	}
	else {
		(void)rd_fdc_reg(fddata);
		printf(": no fifo");
	}

	printf("\n");

	callout_init(&fdc->sc_timo_ch, 0);
	callout_init(&fdc->sc_intr_ch, 0);

	if (intr_establish(22, USER_VEC|FAST_VEC, 0,
			   (hw_ifun_t)(has_fifo ? mfp_hdfd_fifo : mfp_hdfd_nf),
			   NULL) == NULL) {
		printf("fdcattach: Can't establish interrupt\n");
		return;
	}

	/*
	 * Setup the interrupt logic.
	 */
	MFP2->mf_iprb  = (u_int8_t)~IB_DCHG;
	MFP2->mf_imrb |= IB_DCHG;
	MFP2->mf_aer  |= 0x10; /* fdc int low->high */

	/* physical limit: four drives per controller. */
	for (fa.fa_drive = 0; fa.fa_drive < 4; fa.fa_drive++) {
		/*
		 * XXX: Choose something sensible as a default...
		 */
		fa.fa_deftype = &fd_types[2]; /* 1.44MB */
		(void)config_found(self, (void *)&fa, fdprint);
	}
}

int
fdprobe(parent, cfp, aux)
	struct device	*parent;
	struct cfdata	*cfp;
	void		*aux;
{
	struct fdc_softc	*fdc = (void *)parent;
	struct fdc_attach_args	*fa = aux;
	int			drive = fa->fa_drive;
	int			n;

	if (cfp->cf_loc[FDCCF_UNIT] != FDCCF_UNIT_DEFAULT &&
	    cfp->cf_loc[FDCCF_UNIT] != drive)
		return 0;
	/*
	 * XXX
	 * This is to work around some odd interactions between this driver
	 * and SMC Ethernet cards.
	 */
	if (cfp->cf_loc[FDCCF_UNIT] == FDCCF_UNIT_DEFAULT && drive >= 2)
		return 0;

	/* select drive and turn on motor */
	wrt_fdc_reg(fdout, drive | FDO_FRST | FDO_MOEN(drive));

	/* wait for motor to spin up */
	delay(250000);
	out_fdc(NE7CMD_RECAL);
	out_fdc(drive);

	/* wait for recalibrate */
	delay(2000000);
	out_fdc(NE7CMD_SENSEI);
	n = fdcresult(fdc);

#ifdef FD_DEBUG
	{
		int i;
		printf("fdprobe: status");
		for (i = 0; i < n; i++)
			printf(" %x", fdc->sc_status[i]);
		printf("\n");
	}
#endif
	intr_arg = (void*)fdc;
	if (n != 2 || (fdc->sc_status[0] & 0xf8) != 0x20)
		return 0;
	/* turn off motor */
	wrt_fdc_reg(fdout, FDO_FRST);

	return 1;
}

/*
 * Controller is working, and drive responded.  Attach it.
 */
void
fdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fdc_softc	*fdc  = (void *)parent;
	struct fd_softc		*fd   = (void *)self;
	struct fdc_attach_args	*fa   = aux;
	struct fd_type		*type = fa->fa_deftype;
	int			drive = fa->fa_drive;

	callout_init(&fd->sc_motoron_ch, 0);
	callout_init(&fd->sc_motoroff_ch, 0);

	/* XXX Allow `flags' to override device type? */

	if (type)
		printf(": %s %d cyl, %d head, %d sec\n", type->name,
		    type->tracks, type->heads, type->sectrac);
	else
		printf(": density unknown\n");

	bufq_alloc(&fd->sc_q, "disksort", BUFQ_SORT_CYLINDER);
	fd->sc_cylin      = -1;
	fd->sc_drive      = drive;
	fd->sc_deftype    = type;
	fdc->sc_fd[drive] = fd;

	/*
	 * Initialize and attach the disk structure.
	 */
	disk_init(&fd->sc_dk, fd->sc_dev.dv_xname, &fddkdriver);
	disk_attach(&fd->sc_dk);

	/* Needed to power off if the motor is on when we halt. */
	fd->sc_sdhook = shutdownhook_establish(fd_motor_off, fd);
}

/*
 * This is called from the assembly part of the interrupt handler
 * when it is clear that the interrupt was not related to shoving
 * data.
 */
void
fdc_ctrl_intr(frame)
	struct clockframe frame;
{
	int	s;

	/*
	 * Disable further interrupts. The fdcintr() routine
	 * explicitly enables them when needed.
	 */
	MFP2->mf_ierb &= ~IB_DCHG;

	/*
	 * Set fddmalen to zero so no pseudo-DMA transfers will
	 * occur.
	 */
	fddmalen = 0;

	if (!BASEPRI(frame.cf_sr)) {
		/*
		 * We don't want to stay on ipl6.....
		 */
		add_sicallback((si_farg)fdcpseudointr, intr_arg, 0);
	}
	else {
		s = splbio();
		(void) fdcintr(intr_arg);
		splx(s);
	}
}

inline struct fd_type *
fd_dev_to_type(fd, dev)
	struct fd_softc *fd;
	dev_t dev;
{
	int type = FDTYPE(dev);

	if (type > (sizeof(fd_types) / sizeof(fd_types[0])))
		return NULL;
	return type ? &fd_types[type - 1] : fd->sc_deftype;
}

void
fdstrategy(bp)
	register struct buf *bp;	/* IO operation to perform */
{
	struct fd_softc *fd = hdfd_cd.cd_devs[FDUNIT(bp->b_dev)];
	int sz;
 	int s;

	/* Valid unit, controller, and request? */
	if (bp->b_blkno < 0 ||
	    ((bp->b_bcount % FDC_BSIZE) != 0 &&
	     (bp->b_flags & B_FORMAT) == 0)) {
		bp->b_error = EINVAL;
		goto done;
	}

	/* If it's a null transfer, return immediately. */
	if (bp->b_bcount == 0)
		goto done;

	sz = howmany(bp->b_bcount, FDC_BSIZE);

	if (bp->b_blkno + sz > fd->sc_type->size) {
		sz = fd->sc_type->size - bp->b_blkno;
		if (sz == 0) {
			/* If exactly at end of disk, return EOF. */
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
 	bp->b_cylinder = bp->b_blkno / (FDC_BSIZE/DEV_BSIZE) / fd->sc_type->seccyl;

#ifdef FD_DEBUG
	printf("fdstrategy: b_blkno %d b_bcount %ld blkno %qd cylin %ld sz"
		" %d\n", bp->b_blkno, bp->b_bcount, (long)fd->sc_blkno,
		bp->b_cylinder, sz);
#endif

	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();
	BUFQ_PUT(fd->sc_q, bp);
	callout_stop(&fd->sc_motoroff_ch);		/* a good idea */
	if (fd->sc_active == 0)
		fdstart(fd);
#ifdef DIAGNOSTIC
	else {
		struct fdc_softc *fdc = (void *) device_parent(&fd->sc_dev);
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
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

void
fdstart(fd)
	struct fd_softc *fd;
{
	struct fdc_softc *fdc = (void *) device_parent(&fd->sc_dev);
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
	struct fdc_softc *fdc = (void *) device_parent(&fd->sc_dev);

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
		if (BUFQ_PEEK(fd->sc_q) != NULL)
			TAILQ_INSERT_TAIL(&fdc->sc_drives, fd, sc_drivechain);
		else
			fd->sc_active = 0;
	}
	bp->b_resid = fd->sc_bcount;
	fd->sc_skip = 0;

	biodone(bp);
	/* turn off motor 5s from now */
	callout_reset(&fd->sc_motoroff_ch, 5 * hz, fd_motor_off, fd);
	fdc->sc_state = DEVIDLE;
}

int
fdread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	return (physio(fdstrategy, NULL, dev, B_READ, minphys, uio));
}

int
fdwrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	return (physio(fdstrategy, NULL, dev, B_WRITE, minphys, uio));
}

void
fd_set_motor(fdc, reset)
	struct fdc_softc *fdc;
	int reset;
{
	struct fd_softc *fd;
	u_char status;
	int n;

	if ((fd = fdc->sc_drives.tqh_first) != NULL)
		status = fd->sc_drive;
	else
		status = 0;
	if (!reset)
		status |= FDO_FRST | FDO_FDMAEN;
	for (n = 0; n < 4; n++)
		if ((fd = fdc->sc_fd[n]) && (fd->sc_flags & FD_MOTOR))
			status |= FDO_MOEN(n);
	wrt_fdc_reg(fdout, status);
}

void
fd_motor_off(arg)
	void *arg;
{
	struct fd_softc *fd = arg;
	int s;

	s = splbio();
	fd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
	fd_set_motor((struct fdc_softc *) device_parent(&fd->sc_dev), 0);
	splx(s);
}

void
fd_motor_on(arg)
	void *arg;
{
	struct fd_softc *fd = arg;
	struct fdc_softc *fdc = (void *) device_parent(&fd->sc_dev);
	int s;

	s = splbio();
	fd->sc_flags &= ~FD_MOTOR_WAIT;
	if ((fdc->sc_drives.tqh_first == fd) && (fdc->sc_state == MOTORWAIT))
		(void) fdcintr(fdc);
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
		i = rd_fdc_reg(fdsts) & (NE7_DIO | NE7_RQM | NE7_CB);
		if (i == NE7_RQM)
			return n;
		if (i == (NE7_DIO | NE7_RQM | NE7_CB)) {
			if (n >= sizeof(fdc->sc_status)) {
				log(LOG_ERR, "fdcresult: overrun\n");
				return -1;
			}
			fdc->sc_status[n++] = rd_fdc_reg(fddata);
		}
		else delay(10);
	}
	log(LOG_ERR, "fdcresult: timeout\n");
	return -1;
}

int
out_fdc(x)
	u_char x;
{
	int i = 100000;

	while (((rd_fdc_reg(fdsts) & (NE7_DIO|NE7_RQM)) != NE7_RQM) && i-- > 0)
		delay(1);
	if (i <= 0)
		return -1;
	wrt_fdc_reg(fddata, x);
	return 0;
}

int
fdopen(dev, flags, mode, l)
	dev_t dev;
	int flags;
	int mode;
	struct lwp *l;
{
 	int unit;
	struct fd_softc *fd;
	struct fd_type *type;

	unit = FDUNIT(dev);
	if (unit >= hdfd_cd.cd_ndevs)
		return ENXIO;
	fd = hdfd_cd.cd_devs[unit];
	if (fd == 0)
		return ENXIO;
	type = fd_dev_to_type(fd, dev);
	if (type == NULL)
		return ENXIO;

	if ((fd->sc_flags & FD_OPEN) != 0 &&
	    fd->sc_type != type)
		return EBUSY;

	fd->sc_type = type;
	fd->sc_cylin = -1;
	fd->sc_flags |= FD_OPEN;
	fdgetdisklabel(fd, dev);

	return 0;
}

int
fdclose(dev, flags, mode, l)
	dev_t dev;
	int flags;
	int mode;
	struct lwp *l;
{
	struct fd_softc *fd = hdfd_cd.cd_devs[FDUNIT(dev)];

	fd->sc_flags &= ~(FD_OPEN|FD_HAVELAB);
	fd->sc_opts  &= ~(FDOPT_NORETRY|FDOPT_SILENT);
	return 0;
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
	(void) fdcintr(fdc);
}

void
fdcstatus(dv, n, s)
	struct device *dv;
	int n;
	const char *s;
{
	struct fdc_softc *fdc = (void *) device_parent(dv);
	char bits[64];

	if (n == 0) {
		out_fdc(NE7CMD_SENSEI);
		(void) fdcresult(fdc);
		n = 2;
	}

	printf("%s: %s", dv->dv_xname, s);

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
		printf("\nfdcstatus: weird size");
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
	fdcstatus(&fd->sc_dev, 0, "timeout");

	if (BUFQ_PEEK(fd->sc_q) != NULL)
		fdc->sc_state++;
	else
		fdc->sc_state = DEVIDLE;

	(void) fdcintr(fdc);
	splx(s);
}

void
fdcpseudointr(arg)
	void *arg;
{
	int s;

	/* Just ensure it has the right spl. */
	s = splbio();
	(void) fdcintr(arg);
	splx(s);
}

int
fdcintr(arg)
	void *arg;
{
	struct fdc_softc	*fdc = arg;
#define	st0	fdc->sc_status[0]
#define	st1	fdc->sc_status[1]
#define	cyl	fdc->sc_status[1]

	struct fd_softc		*fd;
	struct buf		*bp;
	int			read, head, sec, i, nblks;
	struct fd_type		*type;
	struct ne7_fd_formb	*finfo = NULL;

loop:
	/* Is there a drive for the controller to do a transfer with? */
	fd = fdc->sc_drives.tqh_first;
	if (fd == NULL) {
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

	if (bp->b_flags & B_FORMAT)
		finfo = (struct ne7_fd_formb *)bp->b_data;

	switch (fdc->sc_state) {
	case DEVIDLE:
		fdc->sc_errors = 0;
		fdc->sc_overruns = 0;
		fd->sc_skip = 0;
		fd->sc_bcount = bp->b_bcount;
		fd->sc_blkno = bp->b_blkno / (FDC_BSIZE / DEV_BSIZE);
		callout_stop(&fd->sc_motoroff_ch);
		if ((fd->sc_flags & FD_MOTOR_WAIT) != 0) {
			fdc->sc_state = MOTORWAIT;
			return 1;
		}
		if ((fd->sc_flags & FD_MOTOR) == 0) {
			/* Turn on the motor, being careful about pairing. */
			struct fd_softc *ofd = fdc->sc_fd[fd->sc_drive ^ 1];
			if (ofd && ofd->sc_flags & FD_MOTOR) {
				callout_stop(&ofd->sc_motoroff_ch);
				ofd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
			}
			fd->sc_flags |= FD_MOTOR | FD_MOTOR_WAIT;
			fd_set_motor(fdc, 0);
			fdc->sc_state = MOTORWAIT;
			/* Allow .25s for motor to stabilize. */
			callout_reset(&fd->sc_motoron_ch, hz / 4,
			    fd_motor_on, fd);
			return 1;
		}
		/* Make sure the right drive is selected. */
		fd_set_motor(fdc, 0);

		/* fall through */
	case DOSEEK:
	doseek:
		if (fd->sc_cylin == bp->b_cylinder)
			goto doio;

		out_fdc(NE7CMD_SPECIFY);/* specify command */
		out_fdc(fd->sc_type->steprate);
		out_fdc(0x7);	/* XXX head load time == 6ms - non-DMA */

		fdc_ienable();

		out_fdc(NE7CMD_SEEK);	/* seek function */
		out_fdc(fd->sc_drive);	/* drive number */
		out_fdc(bp->b_cylinder * fd->sc_type->step);

		fd->sc_cylin = -1;
		fdc->sc_state = SEEKWAIT;

		iostat_seek(fd->sc_dk.dk_stats);
		disk_busy(&fd->sc_dk);

		callout_reset(&fdc->sc_timo_ch, 4 * hz, fdctimeout, fdc);
		return 1;

	case DOIO:
	doio:
		if (finfo)
			fd->sc_skip = (char *)&(finfo->fd_formb_cylno(0)) -
				      (char *)finfo;

		type  = fd->sc_type;
		sec   = fd->sc_blkno % type->seccyl;
		head  = sec / type->sectrac;
		sec  -= head * type->sectrac;
		nblks = type->sectrac - sec;
		nblks = min(nblks, fd->sc_bcount / FDC_BSIZE);
		nblks = min(nblks, FDC_MAXIOSIZE / FDC_BSIZE);
		fd->sc_nblks  = nblks;
		fd->sc_nbytes = finfo ? bp->b_bcount : nblks * FDC_BSIZE;
#ifdef DIAGNOSTIC
		{
		     int block;

		     block = (fd->sc_cylin * type->heads + head)
				* type->sectrac + sec;
		     if (block != fd->sc_blkno) {
			 printf("fdcintr: block %d != blkno %qd\n",
						block, fd->sc_blkno);
#ifdef DDB
			 Debugger();
#endif
		     }
		}
#endif
		read = bp->b_flags & B_READ ? 1 : 0;

		/*
		 * Setup pseudo-DMA address & count
		 */
		fddmaaddr = (char *)bp->b_data + fd->sc_skip;
		fddmalen  = fd->sc_nbytes;

		wrt_fdc_reg(fdctl, type->rate);
#ifdef FD_DEBUG
		printf("fdcintr: %s drive %d track %d head %d sec %d"
			" nblks %d\n", read ? "read" : "write",
			fd->sc_drive, fd->sc_cylin, head, sec, nblks);
#endif
		fdc_ienable();

		if (finfo) {
			/* formatting */
			if (out_fdc(NE7CMD_FORMAT) < 0) {
				fdc->sc_errors = 4;
				fdcretry(fdc);
				goto loop;
			}
			out_fdc((head << 2) | fd->sc_drive);
			out_fdc(finfo->fd_formb_secshift);
			out_fdc(finfo->fd_formb_nsecs);
			out_fdc(finfo->fd_formb_gaplen);
			out_fdc(finfo->fd_formb_fillbyte);
		} else {
			if (read)
				out_fdc(NE7CMD_READ);	/* READ */
			else
				out_fdc(NE7CMD_WRITE);	/* WRITE */
			out_fdc((head << 2) | fd->sc_drive);
			out_fdc(fd->sc_cylin);		/* track	 */
			out_fdc(head);			/* head		 */
			out_fdc(sec + 1);		/* sector +1	 */
			out_fdc(type->secsize);		/* sector size   */
			out_fdc(sec + nblks);		/* last sectors	 */
			out_fdc(type->gap1);		/* gap1 size	 */
			out_fdc(type->datalen);		/* data length	 */
		}
		fdc->sc_state = IOCOMPLETE;

		disk_busy(&fd->sc_dk);

		/* allow 2 seconds for operation */
		callout_reset(&fdc->sc_timo_ch, 2 * hz, fdctimeout, fdc);
		return 1;				/* will return later */

	case SEEKWAIT:
		callout_stop(&fdc->sc_timo_ch);
		fdc->sc_state = SEEKCOMPLETE;
		/* allow 1/50 second for heads to settle */
		callout_reset(&fdc->sc_intr_ch, hz / 50, fdcpseudointr, fdc);
		return 1;

	case SEEKCOMPLETE:
		/* no data on seek */
		disk_unbusy(&fd->sc_dk, 0, 0);

		/* Make sure seek really happened. */
		out_fdc(NE7CMD_SENSEI);
		if (fdcresult(fdc) != 2 || (st0 & 0xf8) != 0x20 ||
		    cyl != bp->b_cylinder * fd->sc_type->step) {
#ifdef FD_DEBUG
			fdcstatus(&fd->sc_dev, 2, "seek failed");
#endif
			fdcretry(fdc);
			goto loop;
		}
		fd->sc_cylin = bp->b_cylinder;
		goto doio;

	case IOTIMEDOUT:
	case SEEKTIMEDOUT:
	case RECALTIMEDOUT:
	case RESETTIMEDOUT:
		fdcretry(fdc);
		goto loop;

	case IOCOMPLETE: /* IO DONE, post-analyze */
		callout_stop(&fdc->sc_timo_ch);

		disk_unbusy(&fd->sc_dk, (bp->b_bcount - bp->b_resid),
		    (bp->b_flags & B_READ));

		if (fdcresult(fdc) != 7 || (st1 & 0x37) != 0) {
			/*
			 * As the damn chip doesn't seem to have a FIFO,
			 * accept a few overruns as a fact of life *sigh*
			 */
			if ((st1 & 0x10) && (++fdc->sc_overruns < 4)) {
				fdc->sc_state = DOSEEK;
				goto loop;
			}
#ifdef FD_DEBUG
			fdcstatus(&fd->sc_dev, 7, bp->b_flags & B_READ ?
			    "read failed" : "write failed");
			printf("blkno %qd nblks %d\n",
			    fd->sc_blkno, fd->sc_nblks);
#endif
			fdcretry(fdc);
			goto loop;
		}
		if (fdc->sc_errors) {
			diskerr(bp, "fd", "soft error", LOG_PRINTF,
			    fd->sc_skip / FDC_BSIZE, (struct disklabel *)NULL);
			printf("\n");
			fdc->sc_errors = 0;
		}
		fdc->sc_overruns = 0;
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
		/* try a reset, keep motor on */
		fd_set_motor(fdc, 1);
		delay(100);
		fd_set_motor(fdc, 0);
		fdc->sc_state = RESETCOMPLETE;
		callout_reset(&fdc->sc_timo_ch, hz / 2, fdctimeout, fdc);
		return 1;			/* will return later */

	case RESETCOMPLETE:
		callout_stop(&fdc->sc_timo_ch);
		/* clear the controller output buffer */
		for (i = 0; i < 4; i++) {
			out_fdc(NE7CMD_SENSEI);
			(void) fdcresult(fdc);
		}

		/* fall through */
	case DORECAL:
		fdc_ienable();

		out_fdc(NE7CMD_RECAL);	/* recalibrate function */
		out_fdc(fd->sc_drive);
		fdc->sc_state = RECALWAIT;
		callout_reset(&fdc->sc_timo_ch, 5 * hz, fdctimeout, fdc);
		return 1;			/* will return later */

	case RECALWAIT:
		callout_stop(&fdc->sc_timo_ch);
		fdc->sc_state = RECALCOMPLETE;
		/* allow 1/30 second for heads to settle */
		callout_reset(&fdc->sc_intr_ch, hz / 30, fdcpseudointr, fdc);
		return 1;			/* will return later */

	case RECALCOMPLETE:
		out_fdc(NE7CMD_SENSEI);
		if (fdcresult(fdc) != 2 || (st0 & 0xf8) != 0x20 || cyl != 0) {
#ifdef FD_DEBUG
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
	bp = BUFQ_PEEK(fd->sc_q);

	if (fd->sc_opts & FDOPT_NORETRY)
	    goto fail;

	switch (fdc->sc_errors) {
	case 0:
		/* try again */
		fdc->sc_state = DOSEEK;
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

			printf(" (st0 %s",
			       bitmask_snprintf(fdc->sc_status[0],
						NE7_ST0BITS, bits,
						sizeof(bits)));
			printf(" st1 %s",
			       bitmask_snprintf(fdc->sc_status[1],
						NE7_ST1BITS, bits,
						sizeof(bits)));
			printf(" st2 %s",
			       bitmask_snprintf(fdc->sc_status[2],
						NE7_ST2BITS, bits,
						sizeof(bits)));
			printf(" cyl %d head %d sec %d)\n",
			       fdc->sc_status[3],
			       fdc->sc_status[4],
			       fdc->sc_status[5]);
		}
		bp->b_error = EIO;
		fdfinish(fd, bp);
	}
	fdc->sc_errors++;
}

int
fdioctl(dev, cmd, addr, flag, l)
	dev_t dev;
	u_long cmd;
	void *addr;
	int flag;
	struct lwp *l;
{
	struct fd_softc		*fd;
	struct disklabel	buffer;
	int			error;
	struct fdformat_parms	*form_parms;
	struct fdformat_cmd	*form_cmd;
	struct ne7_fd_formb	*fd_formb;
	unsigned int		scratch;
	int			il[FD_MAX_NSEC + 1];
	register int		i, j;

	fd = hdfd_cd.cd_devs[FDUNIT(dev)];

	switch (cmd) {
	case DIOCGDINFO:
		fdgetdisklabel(fd, dev);
		*(struct disklabel *)addr = *(fd->sc_dk.dk_label);
		return 0;

	case DIOCGPART:
		fdgetdisklabel(fd, dev);
		((struct partinfo *)addr)->disklab = fd->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
			      &fd->sc_dk.dk_label->d_partitions[RAW_PART];
		return(0);

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return EBADF;
		/* XXX do something */
		return 0;

	case DIOCSDINFO:
	case DIOCWDINFO:
		if ((flag & FWRITE) == 0)
		    return EBADF;

		fd->sc_flags &= ~FD_HAVELAB;   /* Invalid */
		error = setdisklabel(&buffer, (struct disklabel *)addr, 0,NULL);
		if (error)
		    return error;

		if (cmd == DIOCWDINFO)
		    error = writedisklabel(dev, fdstrategy, &buffer, NULL);
		return error;

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
		case FDC_125KBPS:
			form_parms->xfer_rate = 125 * 1024;
			break;
		default:
			return EINVAL;
		}
		return 0;

	case FDIOCSETFORMAT:
		if((flag & FWRITE) == 0)
			return EBADF;	/* must be opened for writing */
		form_parms = (struct fdformat_parms *)addr;
		if (form_parms->fdformat_version != FDFORMAT_VERSION)
			return EINVAL;	/* wrong version of formatting prog */

		scratch = form_parms->nbps >> 7;
		if ((form_parms->nbps & 0x7f) || ffs(scratch) == 0 ||
		    scratch & ~(1 << (ffs(scratch)-1)))
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
		case 125 * 1024:
			fd->sc_type->rate = FDC_125KBPS;
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
		fd->sc_type->tracks = form_parms->ncyl;
		fd->sc_type->size = fd->sc_type->seccyl * form_parms->ncyl *
			form_parms->nbps / DEV_BSIZE;
		fd->sc_type->step = form_parms->stepspercyl;
		fd->sc_type->fillbyte = form_parms->fillbyte;
		fd->sc_type->interleave = form_parms->interleave;
		return 0;

	case FDIOCFORMAT_TRACK:
		if((flag & FWRITE) == 0)
			return EBADF;	/* must be opened for writing */
		form_cmd = (struct fdformat_cmd *)addr;
		if (form_cmd->formatcmd_version != FDFORMAT_VERSION)
			return EINVAL;	/* wrong version of formatting prog */

		if (form_cmd->head >= fd->sc_type->heads ||
		    form_cmd->cylinder >= fd->sc_type->tracks) {
			return EINVAL;
		}

		fd_formb = malloc(sizeof(struct ne7_fd_formb),
		    M_TEMP, M_NOWAIT);
		if (fd_formb == 0)
			return ENOMEM;

		fd_formb->head = form_cmd->head;
		fd_formb->cyl = form_cmd->cylinder;
		fd_formb->transfer_rate = fd->sc_type->rate;
		fd_formb->fd_formb_secshift = fd->sc_type->secsize;
		fd_formb->fd_formb_nsecs = fd->sc_type->sectrac;
		fd_formb->fd_formb_gaplen = fd->sc_type->gap2;
		fd_formb->fd_formb_fillbyte = fd->sc_type->fillbyte;

		bzero(il,sizeof il);
		for (j = 0, i = 1; i <= fd_formb->fd_formb_nsecs; i++) {
			while (il[(j%fd_formb->fd_formb_nsecs)+1])
				j++;
			il[(j%fd_formb->fd_formb_nsecs)+1] = i;
			j += fd->sc_type->interleave;
		}
		for (i = 0; i < fd_formb->fd_formb_nsecs; i++) {
			fd_formb->fd_formb_cylno(i) = form_cmd->cylinder;
			fd_formb->fd_formb_headno(i) = form_cmd->head;
			fd_formb->fd_formb_secno(i) = il[i+1];
			fd_formb->fd_formb_secsize(i) = fd->sc_type->secsize;
		}
		
		error = fdformat(dev, fd_formb, l->l_proc);
		free(fd_formb, M_TEMP);
		return error;

	case FDIOCGETOPTS:		/* get drive options */
		*(int *)addr = fd->sc_opts;
		return 0;

	case FDIOCSETOPTS:		/* set drive options */
		fd->sc_opts = *(int *)addr;
		return 0;


	default:
		return ENOTTY;
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
	int rv = 0;
	struct fd_softc *fd = hdfd_cd.cd_devs[FDUNIT(dev)];
	struct fd_type *type = fd->sc_type;
	struct buf *bp;

	/* set up a buffer header for fdstrategy() */
	bp = getiobuf(NULL, false);
	if(bp == 0)
		return ENOBUFS;
	bzero((void *)bp, sizeof(struct buf));
	bp->b_flags = B_PHYS | B_FORMAT;
	bp->b_cflags |= BC_BUSY;
	bp->b_proc = p;
	bp->b_dev = dev;

	/*
	 * calculate a fake blkno, so fdstrategy() would initiate a
	 * seek to the requested cylinder
	 */
	bp->b_blkno = (finfo->cyl * (type->sectrac * type->heads)
		       + finfo->head * type->sectrac) * FDC_BSIZE / DEV_BSIZE;

	bp->b_bcount = sizeof(struct fd_idfield_data) * finfo->fd_formb_nsecs;
	bp->b_data = (void *)finfo;

#ifdef DEBUG
	printf("fdformat: blkno %x count %lx\n", bp->b_blkno, bp->b_bcount);
#endif

	/* now do the format */
	fdstrategy(bp);

	/* ...and wait for it to complete */
	mutex_enter(bp->b_objlock);
	while(!(bp->b_oflags & BO_DONE)) {
		rv = cv_timedwait(&bp->b_done, bp->b_objlock, 20 * hz);
		if (rv == EWOULDBLOCK)
			break;
	}
	mutex_exit(bp->b_objlock);
       
	if (rv == EWOULDBLOCK) {
		/* timed out */
		rv = EIO;
		biodone(bp);
	} else if (bp->b_error != 0) {
		rv = bp->b_error;
	}
	putiobuf(bp);
	return rv;
}


/*
 * Obtain a disklabel. Either a real one from the disk or, if there
 * is none, a fake one.
 */
static void
fdgetdisklabel(fd, dev)
struct fd_softc *fd;
dev_t		dev;
{
	struct disklabel	*lp;
	struct cpu_disklabel	cpulab;

	if (fd->sc_flags & FD_HAVELAB)
		return; /* Already got one */

	lp   = fd->sc_dk.dk_label;

	bzero(lp, sizeof(*lp));
	bzero(&cpulab, sizeof(cpulab));

	lp->d_secpercyl  = fd->sc_type->seccyl;
	lp->d_type       = DTYPE_FLOPPY;
	lp->d_secsize    = FDC_BSIZE;
	lp->d_secperunit = fd->sc_type->size;

	/*
	 * If there is no label on the disk: fake one
	 */
	if (readdisklabel(dev, fdstrategy, lp, &cpulab) != NULL)
		fdgetdefaultlabel(fd, lp, RAW_PART);
	fd->sc_flags |= FD_HAVELAB;

	if ((FDC_BSIZE * fd->sc_type->size)
		< (lp->d_secsize * lp->d_secperunit)) {
		/*
		 * XXX: Ignore these fields. If you drop a vnddisk
		 *	on more than one floppy, you'll get disturbing
		 *	sounds!
		 */
		lp->d_secpercyl  = fd->sc_type->seccyl;
		lp->d_type       = DTYPE_FLOPPY;
		lp->d_secsize    = FDC_BSIZE;
		lp->d_secperunit = fd->sc_type->size;
	}
}

/*
 * Build defaultdisk label. For now we only create a label from what we
 * know from 'sc'.
 */
static void
fdgetdefaultlabel(fd, lp, part)
	struct fd_softc  *fd;
	struct disklabel *lp;
	int part;
{
	bzero(lp, sizeof(struct disklabel));

	lp->d_secsize     = 128 * (1 << fd->sc_type->secsize);
	lp->d_ntracks     = fd->sc_type->heads;
	lp->d_nsectors    = fd->sc_type->sectrac;
	lp->d_secpercyl   = lp->d_ntracks * lp->d_nsectors;
	lp->d_ncylinders  = fd->sc_type->size / lp->d_secpercyl;
	lp->d_secperunit  = fd->sc_type->size;

	lp->d_type        = DTYPE_FLOPPY;
	lp->d_rpm         = 300; 	/* good guess I suppose.	*/
	lp->d_interleave  = 1;		/* FIXME: is this OK?		*/
	lp->d_bbsize      = 0;
	lp->d_sbsize      = 0;
	lp->d_npartitions = part + 1;
	lp->d_trkseek     = 6000; 	/* Who cares...			*/
	lp->d_magic       = DISKMAGIC;
	lp->d_magic2      = DISKMAGIC;
	lp->d_checksum    = dkcksum(lp);
	lp->d_partitions[part].p_size   = lp->d_secperunit;
	lp->d_partitions[part].p_fstype = FS_UNUSED;
	lp->d_partitions[part].p_fsize  = 1024;
	lp->d_partitions[part].p_frag   = 8;
}
