/*	$NetBSD: fd.c,v 1.25.2.1 2006/05/24 10:56:34 yamt Exp $	*/
/*	$OpenBSD: fd.c,v 1.6 1998/10/03 21:18:57 millert Exp $	*/
/*	NetBSD: fd.c,v 1.78 1995/07/04 07:23:09 mycroft Exp 	*/

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
__KERNEL_RCSID(0, "$NetBSD: fd.c,v 1.25.2.1 2006/05/24 10:56:34 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/uio.h>
#include <sys/syslog.h>
#include <sys/queue.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <arc/jazz/fdreg.h>
#include <arc/jazz/fdcvar.h>

#include "ioconf.h"
#include "locators.h"

#define FDUNIT(dev)	DISKUNIT(dev)
#define FDTYPE(dev)	DISKPART(dev)

/* controller driver configuration */
int fdprint(void *, const char *);

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
	/* 1.44MB diskette */
	{ 18,2,36,2,0xff,0xcf,0x1b,0x6c,80,2880,1,FDC_500KBPS,"1.44MB"    },
	/* 1.2 MB AT-diskettes */
	{ 15,2,30,2,0xff,0xdf,0x1b,0x54,80,2400,1,FDC_500KBPS, "1.2MB"    },
	/* 360kB in 1.2MB drive */
	{  9,2,18,2,0xff,0xdf,0x23,0x50,40, 720,2,FDC_300KBPS, "360KB/AT" },
	/* 360kB PC diskettes */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,1,FDC_250KBPS, "360KB/PC" },
	/* 3.5" 720kB diskette */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,80,1440,1,FDC_250KBPS, "720KB"    },
	/* 720kB in 1.2MB drive */
	{  9,2,18,2,0xff,0xdf,0x23,0x50,80,1440,1,FDC_300KBPS, "720KB/x"  },
	/* 360kB in 720kB drive */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,2,FDC_250KBPS, "360KB/x"  },
};

/* software state, per disk (with up to 4 disks per ctlr) */
struct fd_softc {
	struct device sc_dev;
	struct disk sc_dk;

	const struct fd_type *sc_deftype; /* default type descriptor */
	struct fd_type *sc_type;	/* current type descriptor */
	struct fd_type sc_type_copy;	/* copy for fiddling when formatting */

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
#define	FD_OPEN		0x01		/* it's open */
#define	FD_MOTOR	0x02		/* motor should be on */
#define	FD_MOTOR_WAIT	0x04		/* motor coming up */
	int sc_cylin;		/* where we think the head is */

	void *sc_sdhook;	/* saved shutdown hook for drive. */

	TAILQ_ENTRY(fd_softc) sc_drivechain;
	int sc_ops;		/* I/O ops since last switch */
	struct bufq_state *sc_q;/* pending I/O requests */
	int sc_active;		/* number of active I/O operations */
};

/* floppy driver configuration */
int fdprobe(struct device *, struct cfdata *, void *);
void fdattach(struct device *, struct device *, void *);

CFATTACH_DECL(fd, sizeof(struct fd_softc), fdprobe, fdattach, NULL, NULL);

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
void fdstrategy(struct buf *);
void fdstart(struct fd_softc *);

struct dkdriver fddkdriver = { fdstrategy };

#if 0
const struct fd_type *fd_nvtotype(char *, int, int);
#endif
void fd_set_motor(struct fdc_softc *, int);
void fd_motor_off(void *);
void fd_motor_on(void *);
int fdcresult(struct fdc_softc *);
void fdcstart(struct fdc_softc *);
void fdcstatus(struct device *, int, const char *);
void fdctimeout(void *);
void fdcpseudointr(void *);
void fdcretry(struct fdc_softc *);
void fdfinish(struct fd_softc *, struct buf *);
inline const struct fd_type *fd_dev_to_type(struct fd_softc *, dev_t);
void fd_mountroot_hook(struct device *);

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
	struct fdc_attach_args fa;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int type;

	iot = fdc->sc_iot;
	ioh = fdc->sc_ioh;
	callout_init(&fdc->sc_timo_ch);
	callout_init(&fdc->sc_intr_ch);

	fdc->sc_state = DEVIDLE;
	TAILQ_INIT(&fdc->sc_drives);

	/*
	 * No way yet to determine default disk types.
	 * we assume 1.44 3.5" type for the moment.
	 */
	type = 0;

	/* physical limit: two drives per controller. */
	for (fa.fa_drive = 0; fa.fa_drive < 2; fa.fa_drive++) {
		fa.fa_deftype = &fd_types[type];
		(void)config_found(&fdc->sc_dev, (void *)&fa, fdprint);
	}
}

int
fdprobe(struct device *parent, struct cfdata *match, void *aux)
{
	struct fdc_softc *fdc = (void *)parent;
	struct cfdata *cf = match;
	struct fdc_attach_args *fa = aux;
	int drive = fa->fa_drive;
	bus_space_tag_t iot = fdc->sc_iot;
	bus_space_handle_t ioh = fdc->sc_ioh;
	int n;

	if (cf->cf_loc[FDCCF_DRIVE] != FDCCF_DRIVE_DEFAULT &&
	    cf->cf_loc[FDCCF_DRIVE] != drive)
		return 0;

	/* select drive and turn on motor */
	bus_space_write_1(iot, ioh, FDOUT, drive | FDO_FRST | FDO_MOEN(drive));
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
	if (n != 2 || (fdc->sc_status[0] & 0xf8) != 0x20)
		return 0;
	/* turn off motor */
	bus_space_write_1(iot, ioh, FDOUT, FDO_FRST);

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
	const struct fd_type *type = fa->fa_deftype;
	int drive = fa->fa_drive;

	callout_init(&fd->sc_motoron_ch);
	callout_init(&fd->sc_motoroff_ch);

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
	fd->sc_dk.dk_name = fd->sc_dev.dv_xname;
	fd->sc_dk.dk_driver = &fddkdriver;
	disk_attach(&fd->sc_dk);

	/* Establish a mountroot hook. */
	mountroothook_establish(fd_mountroot_hook, &fd->sc_dev);

	/* Needed to power off if the motor is on when we halt. */
	fd->sc_sdhook = shutdownhook_establish(fd_motor_off, fd);
}

#if 0
/*
 * Translate nvram type into internal data structure.  Return NULL for
 * none/unknown/unusable.
 */
const struct fd_type *
fd_nvtotype(char *fdc, int nvraminfo, int drive)
{
	int type;

	type = (drive == 0 ? nvraminfo : nvraminfo << 4) & 0xf0;
#if 0
	switch (type) {
	case NVRAM_DISKETTE_NONE:
		return NULL;
	case NVRAM_DISKETTE_12M:
		return &fd_types[1];
	case NVRAM_DISKETTE_TYPE5:
	case NVRAM_DISKETTE_TYPE6:
		/* XXX We really ought to handle 2.88MB format. */
	case NVRAM_DISKETTE_144M:
		return &fd_types[0];
	case NVRAM_DISKETTE_360K:
		return &fd_types[3];
	case NVRAM_DISKETTE_720K:
		return &fd_types[4];
	default:
		printf("%s: drive %d: unknown device type 0x%x\n",
		    fdc, drive, type);
		return NULL;
	}
#else
	return &fd_types[0]; /* Use only 1.44 for now */
#endif
}
#endif

inline const struct fd_type *
fd_dev_to_type(struct fd_softc *fd, dev_t dev)
{
	int type = FDTYPE(dev);

	if (type > (sizeof(fd_types) / sizeof(fd_types[0])))
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
	    (bp->b_bcount % FDC_BSIZE) != 0) {
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
	bp->b_cylinder =
	    bp->b_blkno / (FDC_BSIZE / DEV_BSIZE) / fd->sc_type->seccyl;

#ifdef FD_DEBUG
	printf("fdstrategy: b_blkno %" PRId64 " b_bcount %ld blkno %" PRId64
	    " cylin %ld sz %d\n",
	    bp->b_blkno, bp->b_bcount, fd->sc_blkno, bp->b_cylinder, sz);
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

bad:
	bp->b_flags |= B_ERROR;
done:
	/* Toss transfer; we're done early. */
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

void
fdstart(struct fd_softc *fd)
{
	struct fdc_softc *fdc = (void *) device_parent(&fd->sc_dev);
	int active = TAILQ_FIRST(&fdc->sc_drives) != 0;

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
fdread(dev_t dev, struct uio *uio, int flags)
{

	return physio(fdstrategy, NULL, dev, B_READ, minphys, uio);
}

int
fdwrite(dev_t dev, struct uio *uio, int flags)
{

	return physio(fdstrategy, NULL, dev, B_WRITE, minphys, uio);
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
	bus_space_write_1(fdc->sc_iot, fdc->sc_ioh, FDOUT, status);
}

void
fd_motor_off(void *arg)
{
	struct fd_softc *fd = arg;
	int s;

	s = splbio();
	fd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
	fd_set_motor((struct fdc_softc *) device_parent(&fd->sc_dev), 0);
	splx(s);
}

void
fd_motor_on(void *arg)
{
	struct fd_softc *fd = arg;
	struct fdc_softc *fdc = (void *) device_parent(&fd->sc_dev);
	int s;

	s = splbio();
	fd->sc_flags &= ~FD_MOTOR_WAIT;
	if ((TAILQ_FIRST(&fdc->sc_drives) == fd) &&
	    (fdc->sc_state == MOTORWAIT))
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
		i = bus_space_read_1(iot, ioh, FDSTS) &
		    (NE7_DIO | NE7_RQM | NE7_CB);
		if (i == NE7_RQM)
			return n;
		if (i == (NE7_DIO | NE7_RQM | NE7_CB)) {
			if (n >= sizeof(fdc->sc_status)) {
				log(LOG_ERR, "fdcresult: overrun\n");
				return -1;
			}
			fdc->sc_status[n++] =
			    bus_space_read_1(iot, ioh, FDDATA);
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

	while ((bus_space_read_1(iot, ioh, FDSTS) & NE7_DIO) && i-- > 0);
	if (i <= 0)
		return -1;
	while ((bus_space_read_1(iot, ioh, FDSTS) & NE7_RQM) == 0 && i-- > 0);
	if (i <= 0)
		return -1;
	bus_space_write_1(iot, ioh, FDDATA, x);
	return 0;
}

int
fdopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct fd_softc *fd;
	const struct fd_type *type;

	fd = device_lookup(&fd_cd, FDUNIT(dev));
	if (fd == NULL)
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
fdclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct fd_softc *fd = device_lookup(&fd_cd, FDUNIT(dev));

	fd->sc_flags &= ~FD_OPEN;
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
	struct fdc_softc *fdc = (void *) device_parent(dv);
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
		sec = fd->sc_blkno % type->seccyl;
		nblks = type->seccyl - sec;
		nblks = min(nblks, fd->sc_bcount / FDC_BSIZE);
		nblks = min(nblks, fdc->sc_maxiosize / FDC_BSIZE);
		fd->sc_nblks = nblks;
		fd->sc_nbytes = nblks * FDC_BSIZE;
		head = sec / type->sectrac;
		sec -= head * type->sectrac;
#ifdef DIAGNOSTIC
		{
			int block;
			block = (fd->sc_cylin * type->heads + head) *
			    type->sectrac + sec;
			if (block != fd->sc_blkno) {
				printf("fdcintr: block %d != blkno %" PRId64
				    "\n", block, fd->sc_blkno);
#ifdef DDB
				 Debugger();
#endif
			}
		}
#endif
		read = (bp->b_flags & B_READ) != 0;
		FDCDMA_START(fdc, bp->b_data + fd->sc_skip,
		    fd->sc_nbytes, read);
		bus_space_write_1(iot, ioh, FDCTL, type->rate);
#ifdef FD_DEBUG
		printf("fdcintr: %s drive %d track %d head %d sec %d nblks %d\n",
		    read ? "read" : "write", fd->sc_drive, fd->sc_cylin, head,
		    sec, nblks);
#endif
		if (read)
			out_fdc(iot, ioh, NE7CMD_READ);	/* READ */
		else
			out_fdc(iot, ioh, NE7CMD_WRITE);/* WRITE */
		out_fdc(iot, ioh, (head << 2) | fd->sc_drive);
		out_fdc(iot, ioh, fd->sc_cylin);	/* track */
		out_fdc(iot, ioh, head);
		out_fdc(iot, ioh, sec + 1);		/* sector + 1 */
		out_fdc(iot, ioh, type->secsize);	/* sector size */
		out_fdc(iot, ioh, type->sectrac);	/* sectors/track */
		out_fdc(iot, ioh, type->gap1);		/* gap1 size */
		out_fdc(iot, ioh, type->datalen);	/* data length */
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
		FDCDMA_ABORT(fdc);

	case SEEKTIMEDOUT:
	case RECALTIMEDOUT:
	case RESETTIMEDOUT:
		fdcretry(fdc);
		goto loop;

	case IOCOMPLETE: /* IO DONE, post-analyze */
		callout_stop(&fdc->sc_timo_ch);

		disk_unbusy(&fd->sc_dk, (bp->b_bcount - bp->b_resid),
		    (bp->b_flags & B_READ));

		i = fdcresult(fdc);
		if (i != 7 || (st0 & 0xf8) != 0) {
			FDCDMA_ABORT(fdc);
#ifdef FD_DEBUG
			fdcstatus(&fd->sc_dev, 7, bp->b_flags & B_READ ?
			    "read failed" : "write failed");
			printf("blkno %" PRId64 " nblks %d\n",
			    fd->sc_blkno, fd->sc_nblks);
#endif
			fdcretry(fdc);
			goto loop;
		}
		FDCDMA_DONE(fdc);
		if (fdc->sc_errors) {
			diskerr(bp, "fd", "soft error (corrected)", LOG_PRINTF,
			    fd->sc_skip / FDC_BSIZE, (struct disklabel *)NULL);
			printf("\n");
			fdc->sc_errors = 0;
		}
		fd->sc_blkno += fd->sc_nblks;
		fd->sc_skip += fd->sc_nbytes;
		fd->sc_bcount -= fd->sc_nbytes;
		if (fd->sc_bcount > 0) {
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
	struct fd_softc *fd;
	struct buf *bp;
	char bits[64];

	fd = TAILQ_FIRST(&fdc->sc_drives);
	bp = BUFQ_PEEK(fd->sc_q);

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
		diskerr(bp, "fd", "hard error", LOG_PRINTF,
		    fd->sc_skip / FDC_BSIZE, (struct disklabel *)NULL);

		printf(" (st0 %s", bitmask_snprintf(fdc->sc_status[0],
		    NE7_ST0BITS, bits, sizeof(bits)));
		printf(" st1 %s", bitmask_snprintf(fdc->sc_status[1],
		    NE7_ST1BITS, bits, sizeof(bits)));
		printf(" st2 %s", bitmask_snprintf(fdc->sc_status[2],
		    NE7_ST2BITS, bits, sizeof(bits)));
		printf(" cyl %d head %d sec %d)\n",
		    fdc->sc_status[3], fdc->sc_status[4], fdc->sc_status[5]);

		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		fdfinish(fd, bp);
	}
	fdc->sc_errors++;
}

int
fdioctl(dev_t dev, u_long cmd, caddr_t addr, int flag, struct lwp *l)
{
	struct fd_softc *fd = device_lookup(&fd_cd, FDUNIT(dev));
	struct disklabel buffer;
	int error;

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

		error = setdisklabel(&buffer, (struct disklabel *)addr,
		    0, NULL);
		if (error)
			return error;

		error = writedisklabel(dev, fdstrategy, &buffer, NULL);
		return error;

	default:
		return ENOTTY;
	}

#ifdef DIAGNOSTIC
	panic("fdioctl: impossible");
#endif
}

/*
 * Mountroot hook: prompt the user to enter the root file system floppy.
 */
void
fd_mountroot_hook(struct device *dev)
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
