/*	$NetBSD: fd.c,v 1.28 2000/03/23 06:35:15 thorpej Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *	from: fd.c,v 1.104 1997/01/09 04:30:08 mycroft Exp
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
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/uio.h>
#include <sys/syslog.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/fdio.h>

#include <machine/cpu.h>
#include <machine/irqhandler.h>
#include <machine/conf.h>
#include <machine/io.h>
#include <machine/katelib.h>
#include <machine/bus.h>
#include <arm32/iomd/iomdreg.h>
#include <arm32/mainbus/piocvar.h>
#include <arm32/mainbus/fdreg.h>

#include "locators.h"

#define NE7CMD_CONFIGURE 0x13

#define FDUNIT(dev)	(minor(dev) / 8)
#define FDTYPE(dev)	(minor(dev) % 8)

/* XXX misuse a flag to identify format operation */
#define B_FORMAT B_XXX

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
	struct device sc_dev;		/* boilerplate */
#ifdef NEWCONFIG
	struct isadev sc_id;
#endif
	void *sc_ih;

	bus_space_tag_t sc_iot;		/* ISA i/o space identifier */
	bus_space_handle_t   sc_ioh;	/* ISA io handle */

	struct callout sc_timo_ch;	/* timeout callout */
	struct callout sc_intr_ch;	/* pseudo-intr callout */

	int sc_drq;

	struct fd_softc *sc_fd[4];	/* pointers to children */
	TAILQ_HEAD(drivehead, fd_softc) sc_drives;
	enum fdc_state sc_state;
	int sc_errors;			/* number of retries so far */
	u_char sc_status[7];		/* copy of registers */
};

/* controller driver configuration */
int fdcprobe __P((struct device *, struct cfdata *, void *));
int fdprint __P((void *, const char *));
#ifdef NEWCONFIG
void fdcforceintr __P((void *));
#endif
void fdcattach __P((struct device *, struct device *, void *));

struct cfattach fdc_ca = {
	sizeof(struct fdc_softc), fdcprobe, fdcattach
};

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
	u_char	fillbyte;	/* format fill byte */
	u_char	interleave;	/* interleave factor (formatting) */
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
	struct device sc_dev;
	struct disk sc_dk;

	struct fd_type *sc_deftype;	/* default type descriptor */
	struct fd_type *sc_type;	/* current type descriptor */
	struct fd_type sc_type_copy;	/* copy for fiddling when formatting */

	struct callout sc_motor_ch;

	daddr_t	sc_blkno;	/* starting block number */
	int sc_bcount;		/* byte count left */
 	int sc_opts;			/* user-set options */
	int sc_skip;		/* bytes already transferred */
	int sc_nblks;		/* number of blocks currently tranferring */
	int sc_nbytes;		/* number of bytes currently tranferring */

	int sc_drive;		/* physical unit number */
	int sc_flags;
#define	FD_OPEN		0x01		/* it's open */
#define	FD_MOTOR	0x02		/* motor should be on */
#define	FD_MOTOR_WAIT	0x04		/* motor coming up */
	int sc_cylin;		/* where we think the head is */

	void *sc_sdhook;	/* saved shutdown hook for drive. */

	TAILQ_ENTRY(fd_softc) sc_drivechain;
	int sc_ops;		/* I/O ops since last switch */
	struct buf_queue sc_q;	/* pending I/O requests */
	int sc_active;		/* number of active I/O operations */
};

/* floppy driver configuration */
int fdprobe __P((struct device *, struct cfdata *, void *));
void fdattach __P((struct device *, struct device *, void *));

static fiqhandler_t fiqhandler;

void floppy_read_fiq __P((void));
void floppy_write_fiq __P((void));

struct cfattach fd_ca = {
	sizeof(struct fd_softc), fdprobe, fdattach
};

extern struct cfdriver fd_cd;

void fdgetdisklabel __P((struct fd_softc *));
int fd_get_parms __P((struct fd_softc *));
void fdstrategy __P((struct buf *));
void fdstart __P((struct fd_softc *));

struct dkdriver fddkdriver = { fdstrategy };

struct fd_type *fd_nvtotype __P((char *, int, int));
void fd_set_motor __P((struct fdc_softc *fdc, int reset));
void fd_motor_off __P((void *arg));
void fd_motor_on __P((void *arg));
int fdcresult __P((struct fdc_softc *fdc));
int out_fdc __P((bus_space_tag_t iot, bus_space_handle_t ioh, u_char x));
void fdcstart __P((struct fdc_softc *fdc));
void fdcstatus __P((struct device *dv, int n, char *s));
void fdctimeout __P((void *arg));
void fdcpseudointr __P((void *arg));
int fdcintr __P((void *));
void fdcretry __P((struct fdc_softc *fdc));
void fdfinish __P((struct fd_softc *fd, struct buf *bp));
__inline struct fd_type *fd_dev_to_type __P((struct fd_softc *, dev_t));
int fdformat __P((dev_t, struct ne7_fd_formb *, struct proc *));

int
fdcprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pioc_attach_args *pa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int rv;

	if (pa->pa_name && strcmp(pa->pa_name, "fdc") != 0)
		return(0);

	iot = pa->pa_iot;
	rv = 0;

	/* Map the i/o space. */
	if (bus_space_map(iot, pa->pa_iobase + pa->pa_offset, FDC_NPORT, 0, &ioh))
		return 0;

	/* reset */
	bus_space_write_2(iot, ioh, fdout, 0);
	delay(100);
	bus_space_write_2(iot, ioh, fdout, FDO_FRST);

	/* see if it can handle a command */
	if (out_fdc(iot, ioh, NE7CMD_SPECIFY) < 0)
		goto out;
	out_fdc(iot, ioh, 0xdf);
	out_fdc(iot, ioh, 2);

#ifdef NEWCONFIG
	if (pa->pa_iobase == PIOCCF_BASE_DEFAULT || pa->pa_drq == PIOCCF_DACK_DEFAULT)
		return 0;

	if (pa->pa_irq == PIOCCF_IRQ_DEFAULT) {
		pa->pa_irq = isa_discoverintr(fdcforceintr, aux);
		if (pa->pa_irq == IRQNONE)
			goto out;

		/* reset it again */
		bus_space_write_2(iot, ioh, fdout, 0);
		delay(100);
		bus_space_write_2(iot, ioh, fdout, FDO_FRST);
	}
#endif

	rv = 1;
	pa->pa_iosize = FDC_NPORT;

 out:
	bus_space_unmap(iot, ioh, FDC_NPORT);
	return rv;
}

#ifdef NEWCONFIG
/*
 * XXX This is broken, and needs fixing.  In general, the interface needs
 * XXX to change.
 */
void
fdcforceintr(aux)
	void *aux;
{
	struct isa_attach_args *ia = aux;
	int iobase = ia->ia_iobase;

	/* the motor is off; this should generate an error with or
	   without a disk drive present */
	out_fdc(iot, ioh, NE7CMD_SEEK);
	out_fdc(iot, ioh, 0);
	out_fdc(iot, ioh, 0);
}
#endif

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
		printf(" drive %d", fa->fa_drive);
	return QUIET;
}

void
fdcattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fdc_softc *fdc = (void *)self;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct pioc_attach_args *pa = aux;
	struct fdc_attach_args fa;
	int type;

	iot = pa->pa_iot;

	/* Re-map the I/O space. */
	if (bus_space_map(iot, pa->pa_iobase + pa->pa_offset, FDC_NPORT, 0, &ioh))
		panic("fdcattach: couldn't map I/O ports");

	fdc->sc_iot = iot;
	fdc->sc_ioh = ioh;

	fdc->sc_drq = pa->pa_iobase + pa->pa_offset + pa->pa_drq;
	fdc->sc_state = DEVIDLE;
	TAILQ_INIT(&fdc->sc_drives);

	printf("\n");

	callout_init(&fdc->sc_timo_ch); 
	callout_init(&fdc->sc_intr_ch);

#ifdef NEWCONFIG
	at_setup_dmachan(fdc->sc_drq, FDC_MAXIOSIZE);
	isa_establish(&fdc->sc_id, &fdc->sc_dev);
#endif
	fdc->sc_ih = intr_claim(pa->pa_irq, IPL_BIO, "fdc",
	    fdcintr, fdc);
	if (!fdc->sc_ih)
		panic("%s: Cannot claim IRQ %d\n", self->dv_xname, pa->pa_irq);

#if 0
	/*
	 * The NVRAM info only tells us about the first two disks on the
	 * `primary' floppy controller.
	 */
	if (fdc->sc_dev.dv_unit == 0)
		type = mc146818_read(NULL, NVRAM_DISKETTE); /* XXX softc */
	else
		type = -1;
#endif
	type = 0x10;	/* XXX - hardcoded for 1 floppy */

	/* physical limit: four drives per controller. */
	for (fa.fa_drive = 0; fa.fa_drive < 4; fa.fa_drive++) {
		if (type >= 0 && fa.fa_drive < 2)
			fa.fa_deftype = fd_nvtotype(fdc->sc_dev.dv_xname,
			    type, fa.fa_drive);
		else
			fa.fa_deftype = NULL;		/* unknown */
		(void)config_found(self, (void *)&fa, fdprint);
	}
}

int
fdprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct fdc_softc *fdc = (void *)parent;
	struct fdc_attach_args *fa = aux;
	int drive = fa->fa_drive;
	bus_space_tag_t iot = fdc->sc_iot;
	bus_space_handle_t ioh = fdc->sc_ioh;
	int n;

	if (cf->cf_loc[FDCCF_DRIVE] != FDCCF_DRIVE_DEFAULT
	  && cf->cf_loc[FDCCF_DRIVE] != drive)
		return 0;
	/*
	 * XXX
	 * This is to work around some odd interactions between this driver
	 * and SMC Ethernet cards.
	 */

	/* Don't need this for arm32 port but leave for the time being (it won't hurt) */

	if (cf->cf_loc[FDCCF_DRIVE] == FDCCF_DRIVE_DEFAULT && drive >= 2)
		return 0;

	/* select drive and turn on motor */
	bus_space_write_2(iot, ioh, fdout, drive | FDO_FRST | FDO_MOEN(drive));
	/* wait for motor to spin up */
	delay(250000);
	out_fdc(iot, ioh, NE7CMD_RECAL);
	out_fdc(iot, ioh, drive);
	/* wait for recalibrate */
	delay(2000000);
	out_fdc(iot, ioh, NE7CMD_SENSEI);
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
	/* turn off motor */
	bus_space_write_1(iot, ioh, fdout, FDO_FRST);

	if (n != 2 || (fdc->sc_status[0] & 0xf8) != 0x20)
		return 0;

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
	struct fdc_softc *fdc = (void *)parent;
	struct fd_softc *fd = (void *)self;
	struct fdc_attach_args *fa = aux;
	struct fd_type *type = fa->fa_deftype;
	int drive = fa->fa_drive;

	callout_init(&fd->sc_motor_ch);

	/* XXX Allow `flags' to override device type? */

	if (type)
		printf(": %s %d cyl, %d head, %d sec\n", type->name,
		    type->cyls, type->heads, type->sectrac);
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
	fd->sc_dk.dk_name = fd->sc_dev.dv_xname;
	fd->sc_dk.dk_driver = &fddkdriver;
	disk_attach(&fd->sc_dk);

#ifdef NEWCONFIG
	/* XXX Need to do some more fiddling with sc_dk. */
	dk_establish(&fd->sc_dk, &fd->sc_dev);
#endif
	/* Needed to power off if the motor is on when we halt. */

}

/*
 * Translate nvram type into internal data structure.  Return NULL for
 * none/unknown/unusable.
 */
struct fd_type *
fd_nvtotype(fdc, nvraminfo, drive)
	char *fdc;
	int nvraminfo, drive;
{
	int type;

	type = (drive == 0 ? nvraminfo : nvraminfo << 4) & 0xf0;
	switch (type) {
#ifndef RC7500
	case 0x00 :
		return NULL;
#else
	case 0x00 :
#endif	/* !RC7500 */
	case 0x10 :
		return &fd_types[0];
	default:
		printf("%s: drive %d: unknown device type 0x%x\n",
		    fdc, drive, type);
		return NULL;
	}
}

__inline struct fd_type *
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
	struct fd_softc *fd = fd_cd.cd_devs[FDUNIT(bp->b_dev)];
	int sz;
 	int s;

	/* Valid unit, controller, and request? */
	if (bp->b_blkno < 0 ||
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
	printf("fdstrategy: b_blkno %d b_bcount %d blkno %d cylin %d sz %d\n",
	    bp->b_blkno, bp->b_bcount, fd->sc_blkno, bp->b_cylinder, sz);
#endif

	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();
	disksort_cylinder(&fd->sc_q, bp);
	callout_stop(&fd->sc_motor_ch);		/* a good idea */
	if (fd->sc_active == 0)
		fdstart(fd);
#ifdef DIAGNOSTIC
	else {
		struct fdc_softc *fdc = (void *)fd->sc_dev.dv_parent;
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
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

void
fdstart(fd)
	struct fd_softc *fd;
{
	struct fdc_softc *fdc = (void *)fd->sc_dev.dv_parent;
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
	struct fdc_softc *fdc = (void *)fd->sc_dev.dv_parent;

	/*
	 * Move this drive to the end of the queue to give others a `fair'
	 * chance.  We only force a switch if N operations are completed while
	 * another drive is waiting to be serviced, since there is a long motor
	 * startup delay whenever we switch.
	 */
	if (fd->sc_drivechain.tqe_next && ++fd->sc_ops >= 8) {
		fd->sc_ops = 0;
		TAILQ_REMOVE(&fdc->sc_drives, fd, sc_drivechain);
		if (BUFQ_NEXT(bp) != NULL)
			TAILQ_INSERT_TAIL(&fdc->sc_drives, fd, sc_drivechain);
		else
			fd->sc_active = 0;
	}
	bp->b_resid = fd->sc_bcount;
	fd->sc_skip = 0;
	BUFQ_REMOVE(&fd->sc_q, bp);

	biodone(bp);
	/* turn off motor 5s from now */
	callout_reset(&fd->sc_motor_ch, 5 * hz, fd_motor_off, fd);
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
	bus_space_write_2(fdc->sc_iot, fdc->sc_ioh, fdout, status);
}

void
fd_motor_off(arg)
	void *arg;
{
	struct fd_softc *fd = arg;
	int s;

	s = splbio();
	fd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
	fd_set_motor((struct fdc_softc *)fd->sc_dev.dv_parent, 0);
	splx(s);
}

void
fd_motor_on(arg)
	void *arg;
{
	struct fd_softc *fd = arg;
	struct fdc_softc *fdc = (void *)fd->sc_dev.dv_parent;
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
out_fdc(iot, ioh, x)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_char x;
{
	int i = 100000;

	while ((bus_space_read_1(iot, ioh, fdsts) & NE7_DIO) && i-- > 0);
	if (i <= 0)
		return -1;
	while ((bus_space_read_1(iot, ioh, fdsts) & NE7_RQM) == 0 && i-- > 0);
	if (i <= 0)
		return -1;
	bus_space_write_2(iot, ioh, fddata, x);
	return 0;
}

int
fdopen(dev, flags, mode, p)
	dev_t dev;
	int flags;
	int mode;
	struct proc *p;
{
 	int unit;
	struct fd_softc *fd;
	struct fd_type *type;

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
	    memcmp(fd->sc_type, type, sizeof(*type)))
		return EBUSY;

	fd->sc_type_copy = *type;
	fd->sc_type = &fd->sc_type_copy;
	fd->sc_cylin = -1;
	fd->sc_flags |= FD_OPEN;

	return 0;
}

int
fdclose(dev, flags, mode, p)
	dev_t dev;
	int flags;
	int mode;
	struct proc *p;
{
	struct fd_softc *fd = fd_cd.cd_devs[FDUNIT(dev)];

	fd->sc_flags &= ~FD_OPEN;
	fd->sc_opts &= ~(FDOPT_NORETRY|FDOPT_SILENT);
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
	char *s;
{
	struct fdc_softc *fdc = (void *)dv->dv_parent;
	char bits[64];

	if (n == 0) {
		out_fdc(fdc->sc_iot, fdc->sc_ioh, NE7CMD_SENSEI);
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
#ifdef DEBUG
	log(LOG_ERR,"fdctimeout: state %d\n", fdc->sc_state);
#endif
	fdcstatus(&fd->sc_dev, 0, "timeout");

	if (BUFQ_FIRST(&fd->sc_q) != NULL)
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
	struct fdc_softc *fdc = arg;
#define	st0	fdc->sc_status[0]
#define	cyl	fdc->sc_status[1]
	struct fd_softc *fd;
	struct buf *bp;
	bus_space_tag_t iot = fdc->sc_iot;
	bus_space_handle_t ioh = fdc->sc_ioh;
	int read, head, sec, i, nblks;
	struct fd_type *type;
	struct ne7_fd_formb *finfo = NULL;

loop:
	/* Is there a drive for the controller to do a transfer with? */
	fd = fdc->sc_drives.tqh_first;
	if (fd == NULL) {
		fdc->sc_state = DEVIDLE;
 		return 1;
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
		callout_stop(&fd->sc_motor_ch);
		if ((fd->sc_flags & FD_MOTOR_WAIT) != 0) {
			fdc->sc_state = MOTORWAIT;
			return 1;
		}
		if ((fd->sc_flags & FD_MOTOR) == 0) {
			/* Turn on the motor, being careful about pairing. */
			struct fd_softc *ofd = fdc->sc_fd[fd->sc_drive ^ 1];
			if (ofd && ofd->sc_flags & FD_MOTOR) {
				callout_stop(&ofd->sc_motor_ch);
				ofd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
			}
			fd->sc_flags |= FD_MOTOR | FD_MOTOR_WAIT;
			fd_set_motor(fdc, 0);
			fdc->sc_state = MOTORWAIT;
			/* Allow .25s for motor to stabilize. */
			callout_reset(&fd->sc_motor_ch, hz / 4,
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

#if 1
		out_fdc(iot, ioh, NE7CMD_CONFIGURE);/* configure command */
		out_fdc(iot, ioh, 0);
		out_fdc(iot, ioh, 0x18);
		out_fdc(iot, ioh, 0);
#endif
		out_fdc(iot, ioh, NE7CMD_SPECIFY);/* specify command */
		out_fdc(iot, ioh, fd->sc_type->steprate);
		out_fdc(iot, ioh, 6);		/* XXX head load time == 6ms */

		out_fdc(iot, ioh, NE7CMD_SEEK);	/* seek function */
		out_fdc(iot, ioh, fd->sc_drive);	/* drive number */
		out_fdc(iot, ioh, bp->b_cylinder * fd->sc_type->step);

		fd->sc_cylin = -1;
		fdc->sc_state = SEEKWAIT;

		fd->sc_dk.dk_seek++;
		disk_busy(&fd->sc_dk);

		callout_reset(&fdc->sc_timo_ch, 4 * hz, fdctimeout, fdc);
		return 1;

	case DOIO:
	doio:
		type = fd->sc_type;
		if (finfo)
			fd->sc_skip = (char *)&(finfo->fd_formb_cylno(0)) -
				      (char *)finfo;
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
			 printf("fdcintr: block %d != blkno %d\n",	
				block, fd->sc_blkno);
#ifdef DDB
			 Debugger();
#endif
		 }}
#endif
		read = bp->b_flags & B_READ;
#ifdef NEWCONFIG
		at_dma(read, bp->b_data + fd->sc_skip, fd->sc_nbytes,
		       fdc->sc_drq);
#else
/*		isa_dmastart(read, bp->b_data + fd->sc_skip, fd->sc_nbytes,
		       fdc->sc_drq);*/
		if (read)
			fiqhandler.fh_func = floppy_read_fiq;
		else
			fiqhandler.fh_func = floppy_write_fiq;
		fiqhandler.fh_r9 = IOMD_BASE + (IOMD_FIQRQ << 2);
		fiqhandler.fh_r10 = fd->sc_nbytes;
		fiqhandler.fh_r11 = (u_int)(bp->b_data + fd->sc_skip);
		fiqhandler.fh_r12 = fdc->sc_drq;
/*		fiqhandler.fh_r13 = 0;*/
		fiqhandler.fh_mask = 0x01;
#ifdef FD_DEBUG
		printf("fdc-doio:r9=%x r10=%x r11=%x r12=%x data=%x skip=%x\n", fiqhandler.fh_r9,
		    fiqhandler.fh_r10, fiqhandler.fh_r11,
		    fiqhandler.fh_r12, (u_int)bp->b_data, fd->sc_skip);
#endif
		if (fiq_claim(&fiqhandler) == -1)
			panic("%s: Cannot claim FIQ vector\n", fdc->sc_dev.dv_xname);
#endif
		bus_space_write_2(iot, ioh, fdctl, type->rate);
#ifdef FD_DEBUG
		printf("fdcintr: %s drive %d track %d head %d sec %d nblks %d\n",
			read ? "read" : "write", fd->sc_drive, fd->sc_cylin,
			head, sec, nblks);
#endif
		if (finfo) {
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
			out_fdc(iot, ioh, fd->sc_cylin); /* track */
			out_fdc(iot, ioh, head);
			out_fdc(iot, ioh, sec + 1);	 /* sector +1 */
			out_fdc(iot, ioh, type->secsize);/* sector size */
			out_fdc(iot, ioh, type->sectrac);/* sectors/track */
			out_fdc(iot, ioh, type->gap1);	 /* gap1 size */
			out_fdc(iot, ioh, type->datalen);/* data length */
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
#if 0
		callout_reset(&fdc->sc_intr_ch, hz / 50, fdcpseudointr, fdc);
#endif
		return 1;

	case SEEKCOMPLETE:
		disk_unbusy(&fd->sc_dk, 0);	/* no data on seek */

		/* Make sure seek really happened. */
		out_fdc(iot, ioh, NE7CMD_SENSEI);
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
#ifdef NEWCONFIG
		at_dma_abort(fdc->sc_drq);
#else
/*		isa_dmaabort(fdc->sc_drq);*/
		if (fiq_release(&fiqhandler) == -1)
			panic("%s: Cannot release FIQ vector\n", fdc->sc_dev.dv_xname);
#endif
	case SEEKTIMEDOUT:
	case RECALTIMEDOUT:
	case RESETTIMEDOUT:
		fdcretry(fdc);
		goto loop;

	case IOCOMPLETE: /* IO DONE, post-analyze */
		callout_stop(&fdc->sc_timo_ch);

		disk_unbusy(&fd->sc_dk, (bp->b_bcount - bp->b_resid));

		if (fdcresult(fdc) != 7 || (st0 & 0xf8) != 0) {
#ifdef NEWCONFIG
			at_dma_abort(fdc->sc_drq);
#else
/*			isa_dmaabort(fdc->sc_drq);*/
			if (fiq_release(&fiqhandler) == -1)
				panic("%s: Cannot release FIQ vector\n", fdc->sc_dev.dv_xname);
#endif
#ifdef FD_DEBUG
			fdcstatus(&fd->sc_dev, 7, bp->b_flags & B_READ ?
			    "read failed" : "write failed");
			printf("blkno %d nblks %d\n",
			    fd->sc_blkno, fd->sc_nblks);
#endif
			fdcretry(fdc);
			goto loop;
		}
#ifdef NEWCONFIG
		at_dma_terminate(fdc->sc_drq);
#else
/*		read = bp->b_flags & B_READ ? DMAMODE_READ : DMAMODE_WRITE;
		isa_dmadone(read, bp->b_data + fd->sc_skip, fd->sc_nbytes,
		    fdc->sc_drq);*/
		if (fiq_release(&fiqhandler) == -1)
			panic("%s: Cannot release FIQ vector\n", fdc->sc_dev.dv_xname);
#endif
		if (fdc->sc_errors) {
#if 0
			diskerr(bp, "fd", "soft error (corrected)", LOG_PRINTF,
			    fd->sc_skip / FDC_BSIZE, (struct disklabel *)NULL);
			printf("\n");
#endif
			fdc->sc_errors = 0;
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
			out_fdc(iot, ioh, NE7CMD_SENSEI);
			(void) fdcresult(fdc);
		}

		/* fall through */
	case DORECAL:
		out_fdc(iot, ioh, NE7CMD_RECAL);	/* recalibrate function */
		out_fdc(iot, ioh, fd->sc_drive);
		fdc->sc_state = RECALWAIT;
		callout_reset(&fdc->sc_timo_ch, 5 * hz, fdctimeout, fdc);
		return 1;			/* will return later */

	case RECALWAIT:
		callout_stop(&fdc->sc_timo_ch);
		fdc->sc_state = RECALCOMPLETE;
		/* allow 1/30 second for heads to settle */
#if 0
		callout_reset(&fdc->sc_intr_ch, hz / 30, fdcpseudointr, fdc);
#endif
		return 1;			/* will return later */

	case RECALCOMPLETE:
		out_fdc(iot, ioh, NE7CMD_SENSEI);
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
	return -1;
}

int
fddump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{

	/* Not implemented. */
	return ENXIO;
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
	struct disklabel buffer;
	int error;
	unsigned int scratch;
	int il[FD_MAX_NSEC + 1];
	register int i, j;

	switch (cmd) {
	case DIOCGDINFO:
		memset(&buffer, 0, sizeof(buffer));

		buffer.d_secpercyl = fd->sc_type->seccyl;
		buffer.d_type = DTYPE_FLOPPY;
		buffer.d_secsize = FDC_BSIZE;

		if (readdisklabel(dev, fdstrategy, &buffer, NULL) != NULL)
			return EINVAL;

		*(struct disklabel *)addr = buffer;
		return 0;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return EBADF;
		/* XXX do something */
		return 0;

	case DIOCWDINFO:
		if ((flag & FWRITE) == 0)
			return EBADF;

		error = setdisklabel(&buffer, (struct disklabel *)addr, 0, NULL);
		if (error)
			return error;

		error = writedisklabel(dev, fdstrategy, &buffer, NULL);
		return error;

	case FDIOCGETFORMAT:
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
		if((flag & FWRITE) == 0)
			return EBADF;	/* must be opened for writing */
		form_cmd = (struct fdformat_cmd *)addr;
		if (form_cmd->formatcmd_version != FDFORMAT_VERSION)
			return EINVAL;	/* wrong version of formatting prog */

		if (form_cmd->head >= fd->sc_type->heads ||
		    form_cmd->cylinder >= fd->sc_type->cyls) {
			return EINVAL;
		}

		fd_formb.head = form_cmd->head;
		fd_formb.cyl = form_cmd->cylinder;
		fd_formb.transfer_rate = fd->sc_type->rate;
		fd_formb.fd_formb_secshift = fd->sc_type->secsize;
		fd_formb.fd_formb_nsecs = fd->sc_type->sectrac;
		fd_formb.fd_formb_gaplen = fd->sc_type->gap2;
		fd_formb.fd_formb_fillbyte = fd->sc_type->fillbyte;

		memset(il, 0, sizeof il);
		for (j = 0, i = 1; i <= fd_formb.fd_formb_nsecs; i++) {
			while (il[(j%fd_formb.fd_formb_nsecs)+1])
				j++;
			il[(j%fd_formb.fd_formb_nsecs)+1] = i;
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
	int rv = 0, s;
	struct fd_softc *fd = fd_cd.cd_devs[FDUNIT(dev)];
	struct fd_type *type = fd->sc_type;
	struct buf *bp;

	/* set up a buffer header for fdstrategy() */
	bp = (struct buf *)malloc(sizeof(struct buf), M_TEMP, M_NOWAIT);
	if(bp == 0)
		return ENOBUFS;
	PHOLD(p);
	memset((void *)bp, 0, sizeof(struct buf));
	bp->b_flags = B_BUSY | B_PHYS | B_FORMAT;
	bp->b_proc = p;
	bp->b_dev = dev;

	/*
	 * calculate a fake blkno, so fdstrategy() would initiate a
	 * seek to the requested cylinder
	 */
	bp->b_blkno = (finfo->cyl * (type->sectrac * type->heads)
		       + finfo->head * type->sectrac) * FDC_BSIZE / DEV_BSIZE;

	bp->b_bcount = sizeof(struct fd_idfield_data) * finfo->fd_formb_nsecs;
	bp->b_data = (caddr_t)finfo;

#ifdef DEBUG
	printf("fdformat: blkno %x count %lx\n", bp->b_blkno, bp->b_bcount);
#endif

	/* now do the format */
	fdstrategy(bp);

	/* ...and wait for it to complete */
	s = splbio();
	while(!(bp->b_flags & B_DONE)) {
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
	if(bp->b_flags & B_ERROR) {
		rv = bp->b_error;
	}
	PRELE(p);
	free(bp, M_TEMP);
	return rv;
}

#include "md.h"
#if NMD > 0

#include <dev/md.h>

int load_memory_disc_from_floppy __P((struct md_conf *md, dev_t dev));

int
load_memory_disc_from_floppy(md, dev)
	struct md_conf *md;
	dev_t dev;
{
	struct buf *bp;
	int loop;
	int s;
	int type;
	int floppysize;

	if (major(dev) != 17)	/* XXX - nice if the major was defined elsewhere */
		return(EINVAL);

	if (md->md_type == MD_UNCONFIGURED || md->md_addr == 0)
		return(EBUSY);

	type = FDTYPE(dev) - 1;
	if (type < 0) type = 0;
	floppysize = fd_types[type].size << (fd_types[type].secsize + 7);
        
	if (md->md_size < floppysize) {
		printf("Memory disc is not big enough for floppy image\n");
		return(EINVAL);
	}

/* We have the memory disk ! */

	printf("Loading memory disc : %4dK ", 0);

/* obtain a buffer */

	bp = geteblk(fd_types[type].sectrac * DEV_BSIZE);
    
/* request no partition relocation by driver on I/O operations */

	bp->b_dev = dev;

	s = spl0();

	if (fdopen(bp->b_dev, 0, 0, curproc) != 0) {
		brelse(bp);		
		printf("Cannot open floppy device\n");
			return(EINVAL);
	}

	for (loop = 0;
	    loop < (floppysize / DEV_BSIZE / fd_types[type].sectrac);
	    ++loop) {
		printf("\x08\x08\x08\x08\x08\x08%4dK ",
		    loop * fd_types[type].sectrac * DEV_BSIZE / 1024);
		bp->b_blkno = loop * fd_types[type].sectrac;
		bp->b_bcount = fd_types[type].sectrac * DEV_BSIZE;
		bp->b_flags = B_BUSY | B_READ;
		bp->b_error = 0;
		bp->b_resid = 0;
		fdstrategy(bp);

		if (biowait(bp))
			panic("Cannot load floppy image\n");
                                                 
		memcpy((caddr_t)md->md_addr + loop * fd_types[type].sectrac
		    * DEV_BSIZE, (caddr_t)bp->b_data,
		    fd_types[type].sectrac * DEV_BSIZE);
	}
	printf("\x08\x08\x08\x08\x08\x08%4dK done\n",
	    loop * fd_types[type].sectrac * DEV_BSIZE / 1024);
        
	fdclose(bp->b_dev, 0, 0, curproc);

	brelse(bp);

	splx(s);
	return(0);
}

#endif
