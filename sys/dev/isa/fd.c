/*	$NetBSD: fd.c,v 1.80 2008/01/31 18:45:45 dyoung Exp $	*/

/*-
 * Copyright (c) 1998, 2003 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: fd.c,v 1.80 2008/01/31 18:45:45 dyoung Exp $");

#include "rnd.h"
#include "opt_ddb.h"

/*
 * XXX This driver should be properly MI'd some day, but this allows us
 * XXX to eliminate a lot of code duplication for now.
 */
#if !defined(alpha) && !defined(algor) && !defined(atari) && \
    !defined(bebox) && !defined(evbmips) && !defined(i386) && \
    !defined(prep) && !defined(sandpoint) && !defined(x86_64)
#error platform not supported by this driver, yet
#endif

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
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <prop/proplib.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <sys/cpu.h>
#include <sys/bus.h>

#include "locators.h"

#if defined(atari)
/*
 * On the atari, it is configured as fdcisa
 */
#define	FDCCF_DRIVE		FDCISACF_DRIVE
#define	FDCCF_DRIVE_DEFAULT	FDCISACF_DRIVE_DEFAULT

#define	fd_cd	fdisa_cd
#endif /* atari */

#include <sys/intr.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/isa/fdreg.h>
#include <dev/isa/fdcvar.h>

#if defined(i386)

#include <dev/ic/mc146818reg.h>			/* for NVRAM access */
#include <i386/isa/nvram.h>

#include "mca.h"
#if NMCA > 0
#include <machine/mca_machdep.h>		/* for MCA_system */
#endif

#endif /* i386 */

#include <dev/isa/fdvar.h>

#define FDUNIT(dev)	(minor(dev) / 8)
#define FDTYPE(dev)	(minor(dev) % 8)

/* (mis)use device use flag to identify format operation */
#define B_FORMAT B_DEVPRIVATE

/* controller driver configuration */
int fdprint(void *, const char *);

#if NMCA > 0
/* MCA - specific entries */
const struct fd_type mca_fd_types[] = {
	{ 18,2,36,2,0xff,0x0f,0x1b,0x6c,80,2880,1,FDC_500KBPS,0xf6,1, "1.44MB"    }, /* 1.44MB diskette - XXX try 16ms step rate */
	{  9,2,18,2,0xff,0x4f,0x2a,0x50,80,1440,1,FDC_250KBPS,0xf6,1, "720KB"    }, /* 3.5 inch 720kB diskette - XXX try 24ms step rate */
};
#endif /* NMCA > 0 */

/* The order of entries in the following table is important -- BEWARE! */

#if defined(atari)
const struct fd_type fd_types[] = {
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,1,FDC_250KBPS,0xf6,1, "360KB/PC" }, /* 360kB PC diskettes */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,80,1440,1,FDC_250KBPS,0xf6,1, "720KB"    }, /* 3.5 inch 720kB diskette */
	{ 18,2,36,2,0xff,0xcf,0x1b,0x6c,80,2880,1,FDC_500KBPS,0xf6,1, "1.44MB"   }, /* 1.44MB diskette */
};
#else
const struct fd_type fd_types[] = {
	{ 18,2,36,2,0xff,0xcf,0x1b,0x6c,80,2880,1,FDC_500KBPS,0xf6,1, "1.44MB"   }, /* 1.44MB diskette */
	{ 15,2,30,2,0xff,0xdf,0x1b,0x54,80,2400,1,FDC_500KBPS,0xf6,1, "1.2MB"    }, /* 1.2 MB AT-diskettes */
	{  9,2,18,2,0xff,0xdf,0x23,0x50,40, 720,2,FDC_300KBPS,0xf6,1, "360KB/AT" }, /* 360kB in 1.2MB drive */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,1,FDC_250KBPS,0xf6,1, "360KB/PC" }, /* 360kB PC diskettes */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,80,1440,1,FDC_250KBPS,0xf6,1, "720KB"    }, /* 3.5 inch 720kB diskette */
	{  9,2,18,2,0xff,0xdf,0x23,0x50,80,1440,1,FDC_300KBPS,0xf6,1, "720KB/x"  }, /* 720kB in 1.2MB drive */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,2,FDC_250KBPS,0xf6,1, "360KB/x"  }, /* 360kB in 720kB drive */
};
#endif /* defined(atari) */

void fdcfinishattach(device_t);
int fdprobe(device_t, struct cfdata *, void *);
void fdattach(device_t, device_t, void *);

extern struct cfdriver fd_cd;

#ifdef atari
CFATTACH_DECL(fdisa, sizeof(struct fd_softc),
    fdprobe, fdattach, NULL, NULL);
#else
CFATTACH_DECL(fd, sizeof(struct fd_softc),
    fdprobe, fdattach, NULL, NULL);
#endif

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

void fdgetdisklabel(struct fd_softc *);
int fd_get_parms(struct fd_softc *);
void fdstart(struct fd_softc *);

struct dkdriver fddkdriver = { fdstrategy, NULL };

#if defined(i386)
const struct fd_type *fd_nvtotype(const char *, int, int);
#endif /* i386 */
void fd_set_motor(struct fdc_softc *fdc, int reset);
void fd_motor_off(void *arg);
void fd_motor_on(void *arg);
int fdcresult(struct fdc_softc *fdc);
void fdcstart(struct fdc_softc *fdc);
void fdcstatus(device_t, int, const char *);
void fdctimeout(void *arg);
void fdcpseudointr(void *arg);
void fdcretry(struct fdc_softc *fdc);
void fdfinish(struct fd_softc *fd, struct buf *bp);
static const struct fd_type *fd_dev_to_type(struct fd_softc *, dev_t);
int fdformat(dev_t, struct ne7_fd_formb *, struct lwp *);
static void fd_set_properties(struct fd_softc *fd);

void	fd_mountroot_hook(device_t);

/*
 * Arguments passed between fdcattach and fdprobe.
 */
struct fdc_attach_args {
	int fa_drive;
	const struct fd_type *fa_deftype;
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
fdcattach(struct fdc_softc *fdc)
{
	callout_init(&fdc->sc_timo_ch, 0);
	callout_init(&fdc->sc_intr_ch, 0);

	fdc->sc_state = DEVIDLE;
	TAILQ_INIT(&fdc->sc_drives);

	fdc->sc_maxiosize = isa_dmamaxsize(fdc->sc_ic, fdc->sc_drq);

	if (isa_drq_alloc(fdc->sc_ic, fdc->sc_drq) != 0) {
		aprint_normal_dev(&fdc->sc_dev, "can't reserve drq %d\n",
		    fdc->sc_drq);
		return;
	}

	if (isa_dmamap_create(fdc->sc_ic, fdc->sc_drq, fdc->sc_maxiosize,
	    BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW)) {
		aprint_normal_dev(&fdc->sc_dev, "can't set up ISA DMA map\n");
		return;
	}

	config_interrupts(&fdc->sc_dev, fdcfinishattach);
}

void
fdcfinishattach(device_t self)
{
	struct fdc_softc *fdc = device_private(self);
	bus_space_tag_t iot = fdc->sc_iot;
	bus_space_handle_t ioh = fdc->sc_ioh;
	struct fdc_attach_args fa;
#if defined(i386)
	int type;
#endif

	/*
	 * Reset the controller to get it into a known state. Not all
	 * probes necessarily need do this to discover the controller up
	 * front, so don't assume anything.
	 */

	bus_space_write_1(iot, ioh, fdout, 0);
	delay(100);
	bus_space_write_1(iot, ioh, fdout, FDO_FRST);

	/* see if it can handle a command */
	if (out_fdc(iot, ioh, NE7CMD_SPECIFY) < 0) {
		aprint_normal_dev(&fdc->sc_dev, "can't reset controller\n");
		return;
	}
	out_fdc(iot, ioh, 0xdf);
	out_fdc(iot, ioh, 2);

#if defined(i386)
	/*
	 * The NVRAM info only tells us about the first two disks on the
	 * `primary' floppy controller.
	 */
	/* XXX device_unit() abuse */
	if (device_unit(&fdc->sc_dev) == 0)
		type = mc146818_read(NULL, NVRAM_DISKETTE); /* XXX softc */
	else
		type = -1;
#endif /* i386 */

	/* physical limit: four drives per controller. */
	fdc->sc_state = PROBING;
	for (fa.fa_drive = 0; fa.fa_drive < 4; fa.fa_drive++) {
		if (fdc->sc_known) {
			if (fdc->sc_present & (1 << fa.fa_drive)) {
				fa.fa_deftype = fdc->sc_knownfds[fa.fa_drive];
				config_found(&fdc->sc_dev, (void *)&fa,
				    fdprint);
			}
		} else {
#if defined(i386)
			if (type >= 0 && fa.fa_drive < 2)
				fa.fa_deftype =
				    fd_nvtotype(device_xname(&fdc->sc_dev),
				        type, fa.fa_drive);
			else
				fa.fa_deftype = NULL;		/* unknown */
#elif defined(atari)
			/*
			 * Atari has a different ordening, defaults to 1.44
			 */
			fa.fa_deftype = &fd_types[2];
#else
			/*
			 * Default to 1.44MB on Alpha and BeBox.  How do we tell
			 * on these platforms?
			 */
			fa.fa_deftype = &fd_types[0];
#endif /* i386 */
			(void)config_found(&fdc->sc_dev, (void *)&fa, fdprint);
		}
	}
	fdc->sc_state = DEVIDLE;
}

int
fdprobe(device_t parent, struct cfdata *match, void *aux)
{
	struct fdc_softc *fdc = device_private(parent);
	struct cfdata *cf = match;
	struct fdc_attach_args *fa = aux;
	int drive = fa->fa_drive;
	bus_space_tag_t iot = fdc->sc_iot;
	bus_space_handle_t ioh = fdc->sc_ioh;
	int n;
	int s;

	if (cf->cf_loc[FDCCF_DRIVE] != FDCCF_DRIVE_DEFAULT &&
	    cf->cf_loc[FDCCF_DRIVE] != drive)
		return 0;
	/*
	 * XXX
	 * This is to work around some odd interactions between this driver
	 * and SMC Ethernet cards.
	 */
	if (cf->cf_loc[FDCCF_DRIVE] == FDCCF_DRIVE_DEFAULT && drive >= 2)
		return 0;

	/* Use PNP information if available */
	if (fdc->sc_known)
		return 1;

	s = splbio();
	/* toss any interrupt status */
	for (n = 0; n < 4; n++) {
		out_fdc(iot, ioh, NE7CMD_SENSEI);
		(void) fdcresult(fdc);
	}
	/* select drive and turn on motor */
	bus_space_write_1(iot, ioh, fdout, drive | FDO_FRST | FDO_MOEN(drive));
	/* wait for motor to spin up */
	(void) tsleep(fdc, PWAIT, "fdprobe1", hz / 4);
	out_fdc(iot, ioh, NE7CMD_RECAL);
	out_fdc(iot, ioh, drive);
	/* wait for recalibrate, up to 2s */
	if (tsleep(fdc, PWAIT, "fdprobe2", 2 * hz) != EWOULDBLOCK) {
#ifdef FD_DEBUG
		/* XXX */
		printf("fdprobe: got intr\n");
#endif
	}
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
	splx(s);

#if defined(bebox)	/* XXX What is this about? --thorpej@NetBSD.org */
	if (n != 2 || (fdc->sc_status[1] != 0))
		return 0;
#else
	if (n != 2 || (fdc->sc_status[0] & 0xf8) != 0x20)
		return 0;
#endif /* bebox */

	return 1;
}

/*
 * Controller is working, and drive responded.  Attach it.
 */
void
fdattach(device_t parent, device_t self, void *aux)
{
	struct fdc_softc *fdc = device_private(parent);
	struct fd_softc *fd = device_private(self);
	struct fdc_attach_args *fa = aux;
	const struct fd_type *type = fa->fa_deftype;
	int drive = fa->fa_drive;

	callout_init(&fd->sc_motoron_ch, 0);
	callout_init(&fd->sc_motoroff_ch, 0);

	/* XXX Allow `flags' to override device type? */

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

	/*
	 * Initialize and attach the disk structure.
	 */
	disk_init(&fd->sc_dk, device_xname(&fd->sc_dev), &fddkdriver);
	disk_attach(&fd->sc_dk);

	/*
	 * Establish a mountroot hook.
	 */
	mountroothook_establish(fd_mountroot_hook, &fd->sc_dev);

	/* Needed to power off if the motor is on when we halt. */
	fd->sc_sdhook = shutdownhook_establish(fd_motor_off, fd);

#if NRND > 0
	rnd_attach_source(&fd->rnd_source, device_xname(&fd->sc_dev),
			  RND_TYPE_DISK, 0);
#endif

	fd_set_properties(fd);
}

#if defined(i386)
/*
 * Translate nvram type into internal data structure.  Return NULL for
 * none/unknown/unusable.
 */
const struct fd_type *
fd_nvtotype(const char *fdc, int nvraminfo, int drive)
{
	int type;

	type = (drive == 0 ? nvraminfo : nvraminfo << 4) & 0xf0;
	switch (type) {
	case NVRAM_DISKETTE_NONE:
		return NULL;
	case NVRAM_DISKETTE_12M:
		return &fd_types[1];
	case NVRAM_DISKETTE_TYPE5:
	case NVRAM_DISKETTE_TYPE6:
		/* XXX We really ought to handle 2.88MB format. */
	case NVRAM_DISKETTE_144M:
#if NMCA > 0
		if (MCA_system)
			return &mca_fd_types[0];
		else
#endif /* NMCA > 0 */
			return &fd_types[0];
	case NVRAM_DISKETTE_360K:
		return &fd_types[3];
	case NVRAM_DISKETTE_720K:
#if NMCA > 0
		if (MCA_system)
			return &mca_fd_types[1];
		else
#endif /* NMCA > 0 */
			return &fd_types[4];
	default:
		printf("%s: drive %d: unknown device type 0x%x\n",
		    fdc, drive, type);
		return NULL;
	}
}
#endif /* i386 */

static const struct fd_type *
fd_dev_to_type(struct fd_softc *fd, dev_t dev)
{
	u_int type = FDTYPE(dev);

	if (type > __arraycount(fd_types))
		return NULL;
	return type ? &fd_types[type - 1] : fd->sc_deftype;
}

void
fdstrategy(struct buf *bp)
{
	struct fd_softc *fd = device_lookup(&fd_cd, FDUNIT(bp->b_dev));
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
 	bp->b_cylinder =
	    bp->b_blkno / (FDC_BSIZE / DEV_BSIZE) / fd->sc_type->seccyl;

#ifdef FD_DEBUG
	printf("fdstrategy: b_blkno %llu b_bcount %d blkno %llu cylin %d "
	    "sz %d\n", (unsigned long long)bp->b_blkno, bp->b_bcount,
	    (unsigned long long)fd->sc_blkno, bp->b_cylinder, sz);
#endif

	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();
	BUFQ_PUT(fd->sc_q, bp);
	callout_stop(&fd->sc_motoroff_ch);		/* a good idea */
	if (fd->sc_active == 0)
		fdstart(fd);
#ifdef DIAGNOSTIC
	else {
		struct fdc_softc *fdc =
		    device_private(device_parent(&fd->sc_dev));
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
fdstart(struct fd_softc *fd)
{
	struct fdc_softc *fdc = device_private(device_parent(&fd->sc_dev));
	int active = !TAILQ_EMPTY(&fdc->sc_drives);

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
	struct fdc_softc *fdc = device_private(device_parent(&fd->sc_dev));

	/*
	 * Move this drive to the end of the queue to give others a `fair'
	 * chance.  We only force a switch if N operations are completed while
	 * another drive is waiting to be serviced, since there is a long motor
	 * startup delay whenever we switch.
	 */
	(void)BUFQ_GET(fd->sc_q);
	if (TAILQ_NEXT(fd, sc_drivechain) && ++fd->sc_ops >= 8) {
		fd->sc_ops = 0;
		TAILQ_REMOVE(&fdc->sc_drives, fd, sc_drivechain);
		if (BUFQ_PEEK(fd->sc_q) != NULL)
			TAILQ_INSERT_TAIL(&fdc->sc_drives, fd, sc_drivechain);
		else
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
	u_char status;
	int n;

	if ((fd = TAILQ_FIRST(&fdc->sc_drives)) != NULL)
		status = fd->sc_drive;
	else
		status = 0;
	if (!reset)
		status |= FDO_FRST | FDO_FDMAEN;
	for (n = 0; n < 4; n++)
		if ((fd = fdc->sc_fd[n]) && (fd->sc_flags & FD_MOTOR))
			status |= FDO_MOEN(n);
	bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, fdout, status);
}

void
fd_motor_off(void *arg)
{
	struct fd_softc *fd = arg;
	struct fdc_softc *fdc;
	int s;

	fdc = device_private(device_parent(&fd->sc_dev));

	s = splbio();
	fd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
	fd_set_motor(fdc, 0);
	splx(s);
}

void
fd_motor_on(void *arg)
{
	struct fd_softc *fd = arg;
	struct fdc_softc *fdc = device_private(device_parent(&fd->sc_dev));
	int s;

	s = splbio();
	fd->sc_flags &= ~FD_MOTOR_WAIT;
	if (TAILQ_FIRST(&fdc->sc_drives) == fd && fdc->sc_state == MOTORWAIT)
		(void)fdcintr(fdc);
	splx(s);
}

int
fdcresult(struct fdc_softc *fdc)
{
	bus_space_tag_t iot = fdc->sc_iot;
	bus_space_handle_t ioh = fdc->sc_ioh;
	u_char i;
	u_int j = 100000,
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
	u_char i;
	u_int j = 100000;

	for (; j; j--) {
		i = bus_space_read_1(iot, ioh, fdsts) &
		    (NE7_DIO | NE7_RQM);
		if (i == NE7_RQM) {
			bus_space_write_1(iot, ioh, fddata, x);
			return 0;
		}
		delay(10);
	}
	return -1;
}

int
fdopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct fd_softc *fd;
	const struct fd_type *type;

	fd = device_lookup(&fd_cd, FDUNIT(dev));
	if (fd == NULL)
		return (ENXIO);

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

	fd_set_properties(fd);

	return 0;
}

int
fdclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct fd_softc *fd = device_lookup(&fd_cd, FDUNIT(dev));

	fd->sc_flags &= ~FD_OPEN;
	fd->sc_opts &= ~(FDOPT_NORETRY|FDOPT_SILENT);
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
fdcstatus(device_t dv, int n, const char *s)
{
	struct fdc_softc *fdc = device_private(device_parent(dv));
	char bits[64];

	if (n == 0) {
		out_fdc(fdc->sc_iot, fdc->sc_ioh, NE7CMD_SENSEI);
		(void) fdcresult(fdc);
		n = 2;
	}

	aprint_normal_dev(dv, "%s", s);

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
fdctimeout(void *arg)
{
	struct fdc_softc *fdc = arg;
	struct fd_softc *fd = TAILQ_FIRST(&fdc->sc_drives);
	int s;

	s = splbio();
#ifdef DEBUG
	log(LOG_ERR, "fdctimeout: state %d\n", fdc->sc_state);
#endif
	fdcstatus(&fd->sc_dev, 0, "timeout");

	if (BUFQ_PEEK(fd->sc_q) != NULL)
		fdc->sc_state++;
	else
		fdc->sc_state = DEVIDLE;

	(void) fdcintr(fdc);
	splx(s);
}

void
fdcpseudointr(void *arg)
{
	int s;

	/* Just ensure it has the right spl. */
	s = splbio();
	(void) fdcintr(arg);
	splx(s);
}

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
	int read, head, sec, i, nblks;
	struct fd_type *type;
	struct ne7_fd_formb *finfo = NULL;

	if (fdc->sc_state == PROBING) {
#ifdef DEBUG
		printf("fdcintr: got probe interrupt\n");
#endif
		wakeup(fdc);
		return 1;
	}

loop:
	/* Is there a drive for the controller to do a transfer with? */
	fd = TAILQ_FIRST(&fdc->sc_drives);
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

		out_fdc(iot, ioh, NE7CMD_SPECIFY);/* specify command */
		out_fdc(iot, ioh, fd->sc_type->steprate);
		out_fdc(iot, ioh, 6);		/* XXX head load time == 6ms */

		out_fdc(iot, ioh, NE7CMD_SEEK);	/* seek function */
		out_fdc(iot, ioh, fd->sc_drive); /* drive number */
		out_fdc(iot, ioh, bp->b_cylinder * fd->sc_type->step);

		fd->sc_cylin = -1;
		fdc->sc_state = SEEKWAIT;

		iostat_seek(fd->sc_dk.dk_stats);
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
		nblks = min(nblks, fdc->sc_maxiosize / FDC_BSIZE);
		fd->sc_nblks = nblks;
		fd->sc_nbytes = finfo ? bp->b_bcount : nblks * FDC_BSIZE;
		head = sec / type->sectrac;
		sec -= head * type->sectrac;
#ifdef DIAGNOSTIC
		{
			int block;
			block = (fd->sc_cylin * type->heads + head)
			    * type->sectrac + sec;
			if (block != fd->sc_blkno) {
				printf("fdcintr: block %d != blkno "
				    "%" PRId64 "\n", block, fd->sc_blkno);
#ifdef DDB
				 Debugger();
#endif
			}
		}
#endif
		read = bp->b_flags & B_READ ? DMAMODE_READ : DMAMODE_WRITE;
		isa_dmastart(fdc->sc_ic, fdc->sc_drq,
		    (char *)bp->b_data + fd->sc_skip, fd->sc_nbytes,
		    NULL, read | DMAMODE_DEMAND, BUS_DMA_NOWAIT);
		bus_space_write_1(iot, fdc->sc_fdctlioh, 0, type->rate);
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
		callout_reset(&fdc->sc_intr_ch, hz / 50, fdcpseudointr, fdc);
		return 1;

	case SEEKCOMPLETE:
		/* no data on seek */
		disk_unbusy(&fd->sc_dk, 0, 0);

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
		isa_dmaabort(fdc->sc_ic, fdc->sc_drq);
	case SEEKTIMEDOUT:
	case RECALTIMEDOUT:
	case RESETTIMEDOUT:
		fdcretry(fdc);
		goto loop;

	case IOCOMPLETE: /* IO DONE, post-analyze */
		callout_stop(&fdc->sc_timo_ch);

		disk_unbusy(&fd->sc_dk, (bp->b_bcount - bp->b_resid),
		    (bp->b_flags & B_READ));

		if (fdcresult(fdc) != 7 || (st0 & 0xf8) != 0) {
			isa_dmaabort(fdc->sc_ic, fdc->sc_drq);
#ifdef FD_DEBUG
			fdcstatus(&fd->sc_dev, 7, bp->b_flags & B_READ ?
			    "read failed" : "write failed");
			printf("blkno %llu nblks %d\n",
			    (unsigned long long)fd->sc_blkno, fd->sc_nblks);
#endif
			fdcretry(fdc);
			goto loop;
		}
		isa_dmadone(fdc->sc_ic, fdc->sc_drq);
		if (fdc->sc_errors) {
			diskerr(bp, "fd", "soft error (corrected)", LOG_PRINTF,
			    fd->sc_skip / FDC_BSIZE, NULL);
			printf("\n");
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
		out_fdc(iot, ioh, NE7CMD_RECAL); /* recalibrate function */
		out_fdc(iot, ioh, fd->sc_drive);
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
fdcretry(struct fdc_softc *fdc)
{
	char bits[64];
	struct fd_softc *fd;
	struct buf *bp;

	fd = TAILQ_FIRST(&fdc->sc_drives);
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
				fd->sc_skip / FDC_BSIZE, NULL);

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
fdioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct fd_softc *fd = device_lookup(&fd_cd, FDUNIT(dev));
	struct fdformat_parms *form_parms;
	struct fdformat_cmd *form_cmd;
	struct ne7_fd_formb *fd_formb;
	struct disklabel buffer;
	int error;
	unsigned int scratch;
	int il[FD_MAX_NSEC + 1];
	int i, j;
#ifdef __HAVE_OLD_DISKLABEL
	struct disklabel newlabel;
#endif

	switch (cmd) {
	case DIOCGDINFO:
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDINFO:
#endif
		memset(&buffer, 0, sizeof(buffer));

		buffer.d_secpercyl = fd->sc_type->seccyl;
		buffer.d_type = DTYPE_FLOPPY;
		buffer.d_secsize = FDC_BSIZE;

		if (readdisklabel(dev, fdstrategy, &buffer, NULL) != NULL)
			return EINVAL;

#ifdef __HAVE_OLD_DISKLABEL
		if (cmd == ODIOCGDINFO) {
			if (buffer.d_npartitions > OLDMAXPARTITIONS)
				return ENOTTY;
			memcpy(addr, &buffer, sizeof (struct olddisklabel));
		} else
#endif
		*(struct disklabel *)addr = buffer;
		return 0;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return EBADF;
		/* XXX do something */
		return 0;

	case DIOCWDINFO:
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCWDINFO:
#endif
	{
		struct disklabel *lp;

		if ((flag & FWRITE) == 0)
			return EBADF;
#ifdef __HAVE_OLD_DISKLABEL
		if (cmd == ODIOCWDINFO) {
			memset(&newlabel, 0, sizeof newlabel);
			memcpy(&newlabel, addr, sizeof (struct olddisklabel));
			lp = &newlabel;
		} else
#endif
		lp = (struct disklabel *)addr;

		error = setdisklabel(&buffer, lp, 0, NULL);
		if (error)
			return error;

		error = writedisklabel(dev, fdstrategy, &buffer, NULL);
		return error;
	}

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

		memset(il, 0, sizeof il);
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

		error = fdformat(dev, fd_formb, l);
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
fdformat(dev_t dev, struct ne7_fd_formb *finfo, struct lwp *l)
{
	int rv = 0;
	struct fd_softc *fd = device_lookup(&fd_cd, FDUNIT(dev));
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
		       + finfo->head * type->sectrac) * FDC_BSIZE / DEV_BSIZE;

	bp->b_bcount = sizeof(struct fd_idfield_data) * finfo->fd_formb_nsecs;
	bp->b_data = (void *)finfo;

#ifdef DEBUG
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

/*
 * Mountroot hook: prompt the user to enter the root file system
 * floppy.
 */
void
fd_mountroot_hook(device_t dev)
{
	int c;

	printf("Insert filesystem floppy and press return.");
	cnpollc(1);
	for (;;) {
		c = cngetc();
		if ((c == '\r') || (c == '\n')) {
			printf("\n");
			break;
		}
	}
	cnpollc(0);
}

static void
fd_set_properties(struct fd_softc *fd)
{
	prop_dictionary_t disk_info, odisk_info, geom;
	const struct fd_type *fdt;
	int secsize;

	fdt = fd->sc_type;
	if (fdt == NULL) {
		fdt = fd->sc_deftype;
		if (fdt == NULL)
			return;
	}

	disk_info = prop_dictionary_create();

	geom = prop_dictionary_create();

	prop_dictionary_set_uint64(geom, "sectors-per-unit",
	    fdt->size);

	switch (fdt->secsize) {
	case 2:
		secsize = 512;
		break;
	case 3:
		secsize = 1024;
		break;
	default:
		secsize = 0;
	}

	prop_dictionary_set_uint32(geom, "sector-size",
	    secsize);

	prop_dictionary_set_uint16(geom, "sectors-per-track",
	    fdt->sectrac);

	prop_dictionary_set_uint16(geom, "tracks-per-cylinder",
	    fdt->heads);

	prop_dictionary_set_uint64(geom, "cylinders-per-unit",
	    fdt->cyls);

	prop_dictionary_set(disk_info, "geometry", geom);
	prop_object_release(geom);

	prop_dictionary_set(device_properties(&fd->sc_dev),
	    "disk-info", disk_info);

	/*
	 * Don't release disk_info here; we keep a reference to it.
	 * disk_detach() will release it when we go away.
	 */

	odisk_info = fd->sc_dk.dk_info;
	fd->sc_dk.dk_info = disk_info;
	if (odisk_info)
		prop_object_release(odisk_info);
}
