/*	$NetBSD: fd.c,v 1.131 2007/05/11 17:48:16 jnemeth Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

/*-
 * Copyright (c) 1993, 1994, 1995 Charles M. Hannum.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fd.c,v 1.131 2007/05/11 17:48:16 jnemeth Exp $");

#include "opt_ddb.h"
#include "opt_md.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/fdio.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/queue.h>
#include <sys/conf.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/intr.h>

#include <sparc/sparc/auxreg.h>
#include <sparc/dev/fdreg.h>
#include <sparc/dev/fdvar.h>

#define FDUNIT(dev)	(minor(dev) / 8)
#define FDTYPE(dev)	(minor(dev) % 8)

/* XXX misuse a flag to identify format operation */
#define B_FORMAT B_XXX

#define FD_DEBUG
#ifdef FD_DEBUG
int	fdc_debug = 0;
#endif

enum fdc_state {
	DEVIDLE = 0,
	MOTORWAIT,	/*  1 */
	DOSEEK,		/*  2 */
	SEEKWAIT,	/*  3 */
	SEEKTIMEDOUT,	/*  4 */
	SEEKCOMPLETE,	/*  5 */
	DOIO,		/*  6 */
	IOCOMPLETE,	/*  7 */
	IOTIMEDOUT,	/*  8 */
	IOCLEANUPWAIT,	/*  9 */
	IOCLEANUPTIMEDOUT,/*10 */
	DORESET,	/* 11 */
	RESETCOMPLETE,	/* 12 */
	RESETTIMEDOUT,	/* 13 */
	DORECAL,	/* 14 */
	RECALWAIT,	/* 15 */
	RECALTIMEDOUT,	/* 16 */
	RECALCOMPLETE,	/* 17 */
	DODSKCHG,	/* 18 */
	DSKCHGWAIT,	/* 19 */
	DSKCHGTIMEDOUT,	/* 20 */
};

/* software state, per controller */
struct fdc_softc {
	struct device	sc_dev;		/* boilerplate */
	bus_space_tag_t	sc_bustag;

	struct callout sc_timo_ch;	/* timeout callout */
	struct callout sc_intr_ch;	/* pseudo-intr callout */

	struct fd_softc *sc_fd[4];	/* pointers to children */
	TAILQ_HEAD(drivehead, fd_softc) sc_drives;
	enum fdc_state	sc_state;
	int		sc_flags;
#define FDC_82077		0x01
#define FDC_NEEDHEADSETTLE	0x02
#define FDC_EIS			0x04
#define FDC_NEEDMOTORWAIT	0x08
	int		sc_errors;		/* number of retries so far */
	int		sc_overruns;		/* number of DMA overruns */
	int		sc_cfg;			/* current configuration */
	struct fdcio	sc_io;
#define sc_handle	sc_io.fdcio_handle
#define sc_reg_msr	sc_io.fdcio_reg_msr
#define sc_reg_fifo	sc_io.fdcio_reg_fifo
#define sc_reg_dor	sc_io.fdcio_reg_dor
#define sc_reg_dir	sc_io.fdcio_reg_dir
#define sc_reg_drs	sc_io.fdcio_reg_msr
#define sc_itask	sc_io.fdcio_itask
#define sc_istatus	sc_io.fdcio_istatus
#define sc_data		sc_io.fdcio_data
#define sc_tc		sc_io.fdcio_tc
#define sc_nstat	sc_io.fdcio_nstat
#define sc_status	sc_io.fdcio_status
#define sc_intrcnt	sc_io.fdcio_intrcnt

	void		*sc_sicookie;	/* softintr(9) cookie */
};

extern	struct fdcio	*fdciop;	/* I/O descriptor used in fdintr.s */

/* controller driver configuration */
int	fdcmatch_mainbus(struct device *, struct cfdata *, void *);
int	fdcmatch_obio(struct device *, struct cfdata *, void *);
void	fdcattach_mainbus(struct device *, struct device *, void *);
void	fdcattach_obio(struct device *, struct device *, void *);

int	fdcattach(struct fdc_softc *, int);

CFATTACH_DECL(fdc_mainbus, sizeof(struct fdc_softc),
    fdcmatch_mainbus, fdcattach_mainbus, NULL, NULL);

CFATTACH_DECL(fdc_obio, sizeof(struct fdc_softc),
    fdcmatch_obio, fdcattach_obio, NULL, NULL);

inline struct fd_type *fd_dev_to_type(struct fd_softc *, dev_t);

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
	int	cylinders;	/* total num of cylinders */
	int	size;		/* size of disk in sectors */
	int	step;		/* steps per cylinder */
	int	rate;		/* transfer speed code */
	int	fillbyte;	/* format fill byte */
	int	interleave;	/* interleave factor (formatting) */
	const char *name;
};

/* The order of entries in the following table is important -- BEWARE! */
struct fd_type fd_types[] = {
	{ 18,2,36,2,0xff,0xcf,0x1b,0x54,80,2880,1,FDC_500KBPS,0xf6,1, "1.44MB"    }, /* 1.44MB diskette */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,80,1440,1,FDC_250KBPS,0xf6,1, "720KB"    }, /* 3.5" 720kB diskette */
	{  9,2,18,2,0xff,0xdf,0x2a,0x50,40, 720,2,FDC_250KBPS,0xf6,1, "360KB/x"  }, /* 360kB in 720kB drive */
	{  8,2,16,3,0xff,0xdf,0x35,0x74,77,1232,1,FDC_500KBPS,0xf6,1, "1.2MB/NEC" } /* 1.2 MB japanese format */
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
	int sc_nblks;		/* number of blocks currently transferring */
	int sc_nbytes;		/* number of bytes currently transferring */

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
	struct bufq_state *sc_q;/* pending I/O requests */
	int sc_active;		/* number of active I/O requests */
};

/* floppy driver configuration */
int	fdmatch(struct device *, struct cfdata *, void *);
void	fdattach(struct device *, struct device *, void *);

CFATTACH_DECL(fd, sizeof(struct fd_softc),
    fdmatch, fdattach, NULL, NULL);

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

void fdgetdisklabel(dev_t);
int fd_get_parms(struct fd_softc *);
void fdstart(struct fd_softc *);
int fdprint(void *, const char *);

struct dkdriver fddkdriver = { fdstrategy };

struct	fd_type *fd_nvtotype(char *, int, int);
void	fd_set_motor(struct fdc_softc *);
void	fd_motor_off(void *);
void	fd_motor_on(void *);
int	fdcresult(struct fdc_softc *);
int	fdc_wrfifo(struct fdc_softc *, uint8_t);
void	fdcstart(struct fdc_softc *);
void	fdcstatus(struct fdc_softc *, const char *);
void	fdc_reset(struct fdc_softc *);
int	fdc_diskchange(struct fdc_softc *);
void	fdctimeout(void *);
void	fdcpseudointr(void *);
int	fdc_c_hwintr(void *);
void	fdchwintr(void);
void	fdcswintr(void *);
int	fdcstate(struct fdc_softc *);
void	fdcretry(struct fdc_softc *);
void	fdfinish(struct fd_softc *, struct buf *);
int	fdformat(dev_t, struct ne7_fd_formb *, struct proc *);
void	fd_do_eject(struct fd_softc *);
void	fd_mountroot_hook(struct device *);
static int fdconf(struct fdc_softc *);
static void establish_chip_type(
		struct fdc_softc *,
		bus_space_tag_t,
		bus_addr_t,
		bus_size_t,
		bus_space_handle_t);

#ifdef MEMORY_DISK_HOOKS
int	fd_read_md_image(size_t *, caddr_t *);
#endif

#define OBP_FDNAME	(CPU_ISSUN4M ? "SUNW,fdtwo" : "fd")

int
fdcmatch_mainbus(struct device *parent, struct cfdata *match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	/*
	 * Floppy controller is on mainbus on sun4c.
	 */
	if (!CPU_ISSUN4C)
		return (0);

	/* sun4c PROMs call the controller "fd" */
	if (strcmp("fd", ma->ma_name) != 0)
		return (0);

	return (bus_space_probe(ma->ma_bustag,
				ma->ma_paddr,
				1,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL));
}

int
fdcmatch_obio(struct device *parent, struct cfdata *match, void *aux)
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa;

	/*
	 * Floppy controller is on obio on sun4m.
	 */
	if (uoba->uoba_isobio4 != 0)
		return (0);

	sa = &uoba->uoba_sbus;

	/* sun4m PROMs call the controller "SUNW,fdtwo" */
	if (strcmp("SUNW,fdtwo", sa->sa_name) != 0)
		return (0);

	return (bus_space_probe(sa->sa_bustag,
			sbus_bus_addr(sa->sa_bustag,
					sa->sa_slot, sa->sa_offset),
			1,	/* probe size */
			0,	/* offset */
			0,	/* flags */
			NULL, NULL));
}

static void
establish_chip_type(struct fdc_softc *fdc,
		    bus_space_tag_t tag, bus_addr_t addr, bus_size_t size,
		    bus_space_handle_t handle)
{
	uint8_t v;

	/*
	 * This hack from Chris Torek: apparently DOR really
	 * addresses MSR/DRS on a 82072.
	 * We used to rely on the VERSION command to tell the
	 * difference (which did not work).
	 */

	/* First, check the size of the register bank */
	if (size < 8)
		/* It isn't a 82077 */
		return;

	/* Then probe the DOR register offset */
	if (bus_space_probe(tag, addr,
			    1,			/* probe size */
			    FDREG77_DOR,	/* offset */
			    0,			/* flags */
			    NULL, NULL) == 0) {

		/* It isn't a 82077 */
		return;
	}

	v = bus_space_read_1(tag, handle, FDREG77_DOR);
	if (v == NE7_RQM) {
		/*
		 * Value in DOR looks like it's really MSR
		 */
		bus_space_write_1(tag, handle, FDREG77_DOR, FDC_250KBPS);
		v = bus_space_read_1(tag, handle, FDREG77_DOR);
		if (v == NE7_RQM) {
			/*
			 * The value in the DOR didn't stick;
			 * it isn't a 82077
			 */
			return;
		}
	}

	fdc->sc_flags |= FDC_82077;
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
	register struct fdc_attach_args *fa = aux;

	if (!fdc)
		aprint_normal(" drive %d", fa->fa_drive);
	return (QUIET);
}

/*
 * Configure several parameters and features on the FDC.
 * Return 0 on success.
 */
static int
fdconf(struct fdc_softc *fdc)
{
	int	vroom;

	if (fdc_wrfifo(fdc, NE7CMD_DUMPREG) || fdcresult(fdc) != 10)
		return (-1);

	/*
	 * dumpreg[7] seems to be a motor-off timeout; set it to whatever
	 * the PROM thinks is appropriate.
	 */
	if ((vroom = fdc->sc_status[7]) == 0)
		vroom = 0x64;

	/* Configure controller to use FIFO and Implied Seek */
	if (fdc_wrfifo(fdc, NE7CMD_CFG) != 0)
		return (-1);
	if (fdc_wrfifo(fdc, vroom) != 0)
		return (-1);
	if (fdc_wrfifo(fdc, fdc->sc_cfg) != 0)
		return (-1);
	if (fdc_wrfifo(fdc, 0) != 0)	/* PRETRK */
		return (-1);
	/* No result phase for the NE7CMD_CFG command */

	if ((fdc->sc_flags & FDC_82077) != 0) {
		/* Lock configuration across soft resets. */
		if (fdc_wrfifo(fdc, NE7CMD_LOCK | CFG_LOCK) != 0 ||
		    fdcresult(fdc) != 1) {
#ifdef DEBUG
			printf("fdconf: CFGLOCK failed");
#endif
			return (-1);
		}
	}

	return (0);
#if 0
	if (fdc_wrfifo(fdc, NE7CMD_VERSION) == 0 &&
	    fdcresult(fdc) == 1 && fdc->sc_status[0] == 0x90) {
		if (fdc_debug)
			printf("[version cmd]");
	}
#endif
}

void
fdcattach_mainbus(struct device *parent, struct device *self, void *aux)
{
	struct fdc_softc *fdc = (void *)self;
	struct mainbus_attach_args *ma = aux;

	fdc->sc_bustag = ma->ma_bustag;

	if (bus_space_map(
			ma->ma_bustag,
			ma->ma_paddr,
			ma->ma_size,
			BUS_SPACE_MAP_LINEAR,
			&fdc->sc_handle) != 0) {
		printf("%s: cannot map registers\n", self->dv_xname);
		return;
	}

	establish_chip_type(fdc,
			    ma->ma_bustag,
			    ma->ma_paddr,
			    ma->ma_size,
			    fdc->sc_handle);

	if (fdcattach(fdc, ma->ma_pri) != 0)
		bus_space_unmap(ma->ma_bustag, fdc->sc_handle, ma->ma_size);
}

void
fdcattach_obio(struct device *parent, struct device *self, void *aux)
{
	struct fdc_softc *fdc = (void *)self;
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;

	if (sa->sa_nintr == 0) {
		printf(": no interrupt line configured\n");
		return;
	}

	fdc->sc_bustag = sa->sa_bustag;

	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot, sa->sa_offset, sa->sa_size,
			 BUS_SPACE_MAP_LINEAR, &fdc->sc_handle) != 0) {
		printf("%s: cannot map control registers\n",
			self->dv_xname);
		return;
	}

	establish_chip_type(fdc,
		sa->sa_bustag,
		sbus_bus_addr(sa->sa_bustag, sa->sa_slot, sa->sa_offset),
		sa->sa_size,
		fdc->sc_handle);

	if (strcmp(prom_getpropstring(sa->sa_node, "status"), "disabled") == 0) {
		printf(": no drives attached\n");
		return;
	}

	if (fdcattach(fdc, sa->sa_pri) != 0)
		bus_space_unmap(sa->sa_bustag, fdc->sc_handle, sa->sa_size);
}

int
fdcattach(struct fdc_softc *fdc, int pri)
{
	struct fdc_attach_args fa;
	int drive_attached;
	char code;

	callout_init(&fdc->sc_timo_ch);
	callout_init(&fdc->sc_intr_ch);

	fdc->sc_state = DEVIDLE;
	fdc->sc_itask = FDC_ITASK_NONE;
	fdc->sc_istatus = FDC_ISTATUS_NONE;
	fdc->sc_flags |= FDC_EIS;
	TAILQ_INIT(&fdc->sc_drives);

	if ((fdc->sc_flags & FDC_82077) != 0) {
		fdc->sc_reg_msr = FDREG77_MSR;
		fdc->sc_reg_fifo = FDREG77_FIFO;
		fdc->sc_reg_dor = FDREG77_DOR;
		fdc->sc_reg_dir = FDREG77_DIR;
		code = '7';
		fdc->sc_flags |= FDC_NEEDMOTORWAIT;
	} else {
		fdc->sc_reg_msr = FDREG72_MSR;
		fdc->sc_reg_fifo = FDREG72_FIFO;
		fdc->sc_reg_dor = 0;
		code = '2';
	}

	/*
	 * Configure controller; enable FIFO, Implied seek, no POLL mode?.
	 * Note: CFG_EFIFO is active-low, initial threshold value: 8
	 */
	fdc->sc_cfg = CFG_EIS|/*CFG_EFIFO|*/CFG_POLL|(8 & CFG_THRHLD_MASK);
	if (fdconf(fdc) != 0) {
		printf(": no drives attached\n");
		return (-1);
	}

	fdciop = &fdc->sc_io;
	if (bus_intr_establish2(fdc->sc_bustag, pri, IPL_BIO,
				fdc_c_hwintr, fdc, fdchwintr) == NULL) {
		printf("\n%s: cannot register interrupt handler\n",
			fdc->sc_dev.dv_xname);
		return (-1);
	}

	fdc->sc_sicookie = softintr_establish(IPL_BIO, fdcswintr, fdc);
	if (fdc->sc_sicookie == NULL) {
		printf("\n%s: cannot register soft interrupt handler\n",
			fdc->sc_dev.dv_xname);
		return (-1);
	}
	printf(" softpri %d: chip 8207%c\n", IPL_SOFTFDC, code);

	evcnt_attach_dynamic(&fdc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    fdc->sc_dev.dv_xname, "intr");

	/* physical limit: four drives per controller. */
	drive_attached = 0;
	for (fa.fa_drive = 0; fa.fa_drive < 4; fa.fa_drive++) {
		fa.fa_deftype = NULL;		/* unknown */
	fa.fa_deftype = &fd_types[0];		/* XXX */
		if (config_found(&fdc->sc_dev, (void *)&fa, fdprint) != NULL)
			drive_attached = 1;
	}

	if (drive_attached == 0) {
		/* XXX - dis-establish interrupts here */
		/* return (-1); */
	}

	return (0);
}

int
fdmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct fdc_softc *fdc = (void *)parent;
	bus_space_tag_t t = fdc->sc_bustag;
	bus_space_handle_t h = fdc->sc_handle;
	struct fdc_attach_args *fa = aux;
	int drive = fa->fa_drive;
	int n, ok;

	if (drive > 0)
		/* XXX - for now, punt on more than one drive */
		return (0);

	if ((fdc->sc_flags & FDC_82077) != 0) {
		/* select drive and turn on motor */
		bus_space_write_1(t, h, fdc->sc_reg_dor,
				  drive | FDO_FRST | FDO_MOEN(drive));
		/* wait for motor to spin up */
		delay(250000);
	} else {
		auxregbisc(AUXIO4C_FDS, 0);
	}
	fdc->sc_nstat = 0;
	fdc_wrfifo(fdc, NE7CMD_RECAL);
	fdc_wrfifo(fdc, drive);

	/* Wait for recalibration to complete */
	for (n = 0; n < 10000; n++) {
		uint8_t v;

		delay(1000);
		v = bus_space_read_1(t, h, fdc->sc_reg_msr);
		if ((v & (NE7_RQM|NE7_DIO|NE7_CB)) == NE7_RQM) {
			/* wait a bit longer till device *really* is ready */
			delay(100000);
			if (fdc_wrfifo(fdc, NE7CMD_SENSEI))
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
			printf(" 0x%x", fdc->sc_status[i]);
		printf("\n");
	}
#endif
	ok = (n == 2 && (fdc->sc_status[0] & 0xf8) == 0x20) ? 1 : 0;

	/* turn off motor */
	if ((fdc->sc_flags & FDC_82077) != 0) {
		/* deselect drive and turn motor off */
		bus_space_write_1(t, h, fdc->sc_reg_dor, FDO_FRST | FDO_DS);
	} else {
		auxregbisc(0, AUXIO4C_FDS);
	}

	return (ok);
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
	struct fd_type *type = fa->fa_deftype;
	int drive = fa->fa_drive;

	callout_init(&fd->sc_motoron_ch);
	callout_init(&fd->sc_motoroff_ch);

	/* XXX Allow `flags' to override device type? */

	if (type)
		printf(": %s %d cyl, %d head, %d sec\n", type->name,
		    type->cylinders, type->heads, type->sectrac);
	else
		printf(": density unknown\n");

	bufq_alloc(&fd->sc_q, "disksort", BUFQ_SORT_CYLINDER);
	fd->sc_cylin = -1;
	fd->sc_drive = drive;
	fd->sc_deftype = type;
	fdc->sc_fd[drive] = fd;

	fdc_wrfifo(fdc, NE7CMD_SPECIFY);
	fdc_wrfifo(fdc, type->steprate);
	/* XXX head load time == 6ms */
	fdc_wrfifo(fdc, 6 | NE7_SPECIFY_NODMA);

	/*
	 * Initialize and attach the disk structure.
	 */
	fd->sc_dk.dk_name = fd->sc_dv.dv_xname;
	fd->sc_dk.dk_driver = &fddkdriver;
	disk_attach(&fd->sc_dk);

	/*
	 * Establish a mountroot_hook anyway in case we booted
	 * with RB_ASKNAME and get selected as the boot device.
	 */
	mountroothook_establish(fd_mountroot_hook, &fd->sc_dv);

	/* Make sure the drive motor gets turned off at shutdown time. */
	fd->sc_sdhook = shutdownhook_establish(fd_motor_off, fd);
}

inline struct fd_type *
fd_dev_to_type(struct fd_softc *fd, dev_t dev)
{
	int type = FDTYPE(dev);

	if (type > (sizeof(fd_types) / sizeof(fd_types[0])))
		return (NULL);
	return (type ? &fd_types[type - 1] : fd->sc_deftype);
}

void
fdstrategy(struct buf *bp)
{
	struct fd_softc *fd;
	int unit = FDUNIT(bp->b_dev);
	int sz;
 	int s;

	/* Valid unit, controller, and request? */
	if (unit >= fd_cd.cd_ndevs ||
	    (fd = fd_cd.cd_devs[unit]) == 0 ||
	    bp->b_blkno < 0 ||
	    (((bp->b_bcount % FD_BSIZE(fd)) != 0 ||
	      (bp->b_blkno * DEV_BSIZE) % FD_BSIZE(fd) != 0) &&
	     (bp->b_flags & B_FORMAT) == 0)) {
		bp->b_error = EINVAL;
		goto bad;
	}

	/* If it's a null transfer, return immediately. */
	if (bp->b_bcount == 0)
		goto done;

	sz = howmany(bp->b_bcount, DEV_BSIZE);

	if (bp->b_blkno + sz > (fd->sc_type->size * DEV_BSIZE) / FD_BSIZE(fd)) {
		sz = (fd->sc_type->size * DEV_BSIZE) / FD_BSIZE(fd)
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
 	bp->b_cylinder = (bp->b_blkno * DEV_BSIZE) /
		      (FD_BSIZE(fd) * fd->sc_type->seccyl);

#ifdef FD_DEBUG
	if (fdc_debug > 1)
	    printf("fdstrategy: b_blkno %lld b_bcount %d blkno %lld cylin %d\n",
		    (long long)bp->b_blkno, bp->b_bcount,
		    (long long)fd->sc_blkno, bp->b_cylinder);
#endif

	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();
	BUFQ_PUT(fd->sc_q, bp);
	callout_stop(&fd->sc_motoroff_ch);		/* a good idea */
	if (fd->sc_active == 0)
		fdstart(fd);
#ifdef DIAGNOSTIC
	else {
		struct fdc_softc *fdc = (void *)device_parent(&fd->sc_dv);
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
	struct fdc_softc *fdc = (void *)device_parent(&fd->sc_dv);
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
	struct fdc_softc *fdc = (void *)device_parent(&fd->sc_dv);

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

	biodone(bp);
	/* turn off motor 5s from now */
	callout_reset(&fd->sc_motoroff_ch, 5 * hz, fd_motor_off, fd);
	fdc->sc_state = DEVIDLE;
}

void
fdc_reset(struct fdc_softc *fdc)
{
	bus_space_tag_t t = fdc->sc_bustag;
	bus_space_handle_t h = fdc->sc_handle;

	if ((fdc->sc_flags & FDC_82077) != 0) {
		bus_space_write_1(t, h, fdc->sc_reg_dor,
				  FDO_FDMAEN | FDO_MOEN(0));
	}

	bus_space_write_1(t, h, fdc->sc_reg_drs, DRS_RESET);
	delay(10);
	bus_space_write_1(t, h, fdc->sc_reg_drs, 0);

	if ((fdc->sc_flags & FDC_82077) != 0) {
		bus_space_write_1(t, h, fdc->sc_reg_dor,
				  FDO_FRST | FDO_FDMAEN | FDO_DS);
	}
#ifdef FD_DEBUG
	if (fdc_debug)
		printf("fdc reset\n");
#endif
}

void
fd_set_motor(struct fdc_softc *fdc)
{
	struct fd_softc *fd;
	u_char status;
	int n;

	if ((fdc->sc_flags & FDC_82077) != 0) {
		status = FDO_FRST | FDO_FDMAEN;
		if ((fd = fdc->sc_drives.tqh_first) != NULL)
			status |= fd->sc_drive;

		for (n = 0; n < 4; n++)
			if ((fd = fdc->sc_fd[n]) && (fd->sc_flags & FD_MOTOR))
				status |= FDO_MOEN(n);
		bus_space_write_1(fdc->sc_bustag, fdc->sc_handle,
				  fdc->sc_reg_dor, status);
	} else {

		for (n = 0; n < 4; n++) {
			if ((fd = fdc->sc_fd[n]) != NULL  &&
			    (fd->sc_flags & FD_MOTOR) != 0) {
				auxregbisc(AUXIO4C_FDS, 0);
				return;
			}
		}
		auxregbisc(0, AUXIO4C_FDS);
	}
}

void
fd_motor_off(void *arg)
{
	struct fd_softc *fd = arg;
	int s;

	s = splbio();
	fd->sc_flags &= ~(FD_MOTOR | FD_MOTOR_WAIT);
	fd_set_motor((struct fdc_softc *)device_parent(&fd->sc_dv));
	splx(s);
}

void
fd_motor_on(void *arg)
{
	struct fd_softc *fd = arg;
	struct fdc_softc *fdc = (void *)device_parent(&fd->sc_dv);
	int s;

	s = splbio();
	fd->sc_flags &= ~FD_MOTOR_WAIT;
	if ((fdc->sc_drives.tqh_first == fd) && (fdc->sc_state == MOTORWAIT))
		(void) fdcstate(fdc);
	splx(s);
}

/*
 * Get status bytes off the FDC after a command has finished
 * Returns the number of status bytes read; -1 on error.
 * The return value is also stored in `sc_nstat'.
 */
int
fdcresult(struct fdc_softc *fdc)
{
	bus_space_tag_t t = fdc->sc_bustag;
	bus_space_handle_t h = fdc->sc_handle;
	int j, n = 0;

	for (j = 10000; j; j--) {
		uint8_t v = bus_space_read_1(t, h, fdc->sc_reg_msr);
		v &= (NE7_DIO | NE7_RQM | NE7_CB);
		if (v == NE7_RQM)
			return (fdc->sc_nstat = n);
		if (v == (NE7_DIO | NE7_RQM | NE7_CB)) {
			if (n >= sizeof(fdc->sc_status)) {
				log(LOG_ERR, "fdcresult: overrun\n");
				return (-1);
			}
			fdc->sc_status[n++] =
				bus_space_read_1(t, h, fdc->sc_reg_fifo);
		} else
			delay(1);
	}

	log(LOG_ERR, "fdcresult: timeout\n");
	return (fdc->sc_nstat = -1);
}

/*
 * Write a command byte to the FDC.
 * Returns 0 on success; -1 on failure (i.e. timeout)
 */
int
fdc_wrfifo(struct fdc_softc *fdc, uint8_t x)
{
	bus_space_tag_t t = fdc->sc_bustag;
	bus_space_handle_t h = fdc->sc_handle;
	int i;

	for (i = 100000; i-- > 0;) {
		uint8_t v = bus_space_read_1(t, h, fdc->sc_reg_msr);
		if ((v & (NE7_DIO|NE7_RQM)) == NE7_RQM) {
			/* The chip is ready */
			bus_space_write_1(t, h, fdc->sc_reg_fifo, x);
			return (0);
		}
		delay(1);
	}
	return (-1);
}

int
fdc_diskchange(struct fdc_softc *fdc)
{

	if (CPU_ISSUN4M && (fdc->sc_flags & FDC_82077) != 0) {
		bus_space_tag_t t = fdc->sc_bustag;
		bus_space_handle_t h = fdc->sc_handle;
		uint8_t v = bus_space_read_1(t, h, fdc->sc_reg_dir);
		return ((v & FDI_DCHG) != 0);
	} else if (CPU_ISSUN4C) {
		return ((*AUXIO4C_REG & AUXIO4C_FDC) != 0);
	}
	return (0);
}

int
fdopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
 	int unit, pmask;
	struct fd_softc *fd;
	struct fd_type *type;

	unit = FDUNIT(dev);
	if (unit >= fd_cd.cd_ndevs)
		return (ENXIO);
	fd = fd_cd.cd_devs[unit];
	if (fd == NULL)
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
fdclose(dev_t dev, int flags, int fmt, struct lwp *l)
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
fdread(dev_t dev, struct uio *uio, int flag)
{

        return (physio(fdstrategy, NULL, dev, B_READ, minphys, uio));
}

int
fdwrite(dev_t dev, struct uio *uio, int flag)
{

        return (physio(fdstrategy, NULL, dev, B_WRITE, minphys, uio));
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
	(void) fdcstate(fdc);
}

void
fdcstatus(struct fdc_softc *fdc, const char *s)
{
	struct fd_softc *fd = fdc->sc_drives.tqh_first;
	int n;
	char bits[64];

	/* Just print last status */
	n = fdc->sc_nstat;

#if 0
	/*
	 * A 82072 seems to return <invalid command> on
	 * gratuitous Sense Interrupt commands.
	 */
	if (n == 0 && (fdc->sc_flags & FDC_82077) != 0) {
		fdc_wrfifo(fdc, NE7CMD_SENSEI);
		(void) fdcresult(fdc);
		n = 2;
	}
#endif

	printf("%s: %s: state %d",
		fd ? fd->sc_dv.dv_xname : "fdc", s, fdc->sc_state);

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
	struct fd_softc *fd;
	int s;

	s = splbio();
	fd = fdc->sc_drives.tqh_first;
	if (fd == NULL) {
		printf("%s: timeout but no I/O pending: state %d, istatus=%d\n",
			fdc->sc_dev.dv_xname,
			fdc->sc_state, fdc->sc_istatus);
		fdc->sc_state = DEVIDLE;
		goto out;
	}

	if (BUFQ_PEEK(fd->sc_q) != NULL)
		fdc->sc_state++;
	else
		fdc->sc_state = DEVIDLE;

	(void) fdcstate(fdc);
out:
	splx(s);

}

void
fdcpseudointr(void *arg)
{
	struct fdc_softc *fdc = arg;
	int s;

	/* Just ensure it has the right spl. */
	s = splbio();
	(void) fdcstate(fdc);
	splx(s);
}


/*
 * hardware interrupt entry point: used only if no `fast trap' * (in-window)
 * handler is available. Unfortunately, we have no reliable way to
 * determine that the interrupt really came from the floppy controller;
 * just hope that the other devices that share this interrupt level
 * can do better..
 */
int
fdc_c_hwintr(void *arg)
{
	struct fdc_softc *fdc = arg;
	bus_space_tag_t t = fdc->sc_bustag;
	bus_space_handle_t h = fdc->sc_handle;

	switch (fdc->sc_itask) {
	case FDC_ITASK_NONE:
		return (0);
	case FDC_ITASK_SENSEI:
		if (fdc_wrfifo(fdc, NE7CMD_SENSEI) != 0 || fdcresult(fdc) == -1)
			fdc->sc_istatus = FDC_ISTATUS_ERROR;
		else
			fdc->sc_istatus = FDC_ISTATUS_DONE;
		softintr_schedule(fdc->sc_sicookie);
		return (1);
	case FDC_ITASK_RESULT:
		if (fdcresult(fdc) == -1)
			fdc->sc_istatus = FDC_ISTATUS_ERROR;
		else
			fdc->sc_istatus = FDC_ISTATUS_DONE;
		softintr_schedule(fdc->sc_sicookie);
		return (1);
	case FDC_ITASK_DMA:
		/* Proceed with pseudo-DMA below */
		break;
	default:
		printf("fdc: stray hard interrupt: itask=%d\n", fdc->sc_itask);
		fdc->sc_istatus = FDC_ISTATUS_SPURIOUS;
		softintr_schedule(fdc->sc_sicookie);
		return (1);
	}

	/*
	 * Pseudo DMA in progress
	 */
	for (;;) {
		uint8_t msr;

		msr = bus_space_read_1(t, h, fdc->sc_reg_msr);

		if ((msr & NE7_RQM) == 0)
			/* That's all this round */
			break;

		if ((msr & NE7_NDM) == 0) {
			fdcresult(fdc);
			fdc->sc_istatus = FDC_ISTATUS_DONE;
			softintr_schedule(fdc->sc_sicookie);
#ifdef FD_DEBUG
			if (fdc_debug > 1)
				printf("fdc: overrun: tc = %d\n", fdc->sc_tc);
#endif
			break;
		}

		/* Another byte can be transferred */
		if ((msr & NE7_DIO) != 0)
			*fdc->sc_data =
				bus_space_read_1(t, h, fdc->sc_reg_fifo);
		else
			bus_space_write_1(t, h, fdc->sc_reg_fifo,
					  *fdc->sc_data);

		fdc->sc_data++;
		if (--fdc->sc_tc == 0) {
			fdc->sc_istatus = FDC_ISTATUS_DONE;
			FTC_FLIP;
			fdcresult(fdc);
			softintr_schedule(fdc->sc_sicookie);
			break;
		}
	}
	return (1);
}

void
fdcswintr(void *arg)
{
	struct fdc_softc *fdc = arg;

	if (fdc->sc_istatus == FDC_ISTATUS_NONE)
		/* This (software) interrupt is not for us */
		return;

	switch (fdc->sc_istatus) {
	case FDC_ISTATUS_ERROR:
		printf("fdc: ierror status: state %d\n", fdc->sc_state);
		break;
	case FDC_ISTATUS_SPURIOUS:
		printf("fdc: spurious interrupt: state %d\n", fdc->sc_state);
		break;
	}

	fdcstate(fdc);
	return;
}

int
fdcstate(struct fdc_softc *fdc)
{

#define	st0	fdc->sc_status[0]
#define	st1	fdc->sc_status[1]
#define	cyl	fdc->sc_status[1]
#define FDC_WRFIFO(fdc, c) do {			\
	if (fdc_wrfifo(fdc, (c))) {		\
		goto xxx;			\
	}					\
} while(0)

	struct fd_softc *fd;
	struct buf *bp;
	int read, head, sec, nblks;
	struct fd_type *type;
	struct ne7_fd_formb *finfo = NULL;

	if (fdc->sc_istatus == FDC_ISTATUS_ERROR) {
		/* Prevent loop if the reset sequence produces errors */
		if (fdc->sc_state != RESETCOMPLETE &&
		    fdc->sc_state != RECALWAIT &&
		    fdc->sc_state != RECALCOMPLETE)
			fdc->sc_state = DORESET;
	}

	/* Clear I task/status field */
	fdc->sc_istatus = FDC_ISTATUS_NONE;
	fdc->sc_itask = FDC_ITASK_NONE;

loop:
	/* Is there a drive for the controller to do a transfer with? */
	fd = fdc->sc_drives.tqh_first;
	if (fd == NULL) {
		fdc->sc_state = DEVIDLE;
 		return (0);
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
		fd->sc_blkno = (bp->b_blkno * DEV_BSIZE) / FD_BSIZE(fd);
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
			if ((fdc->sc_flags & FDC_NEEDMOTORWAIT) != 0) { /*XXX*/
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

		if (fdc_diskchange(fdc))
			goto dodskchg;

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

		fd->sc_cylin = -1;
		fdc->sc_state = SEEKWAIT;
		fdc->sc_nstat = 0;

		iostat_seek(fd->sc_dk.dk_stats);

		disk_busy(&fd->sc_dk);
		callout_reset(&fdc->sc_timo_ch, 4 * hz, fdctimeout, fdc);

		/* specify command */
		FDC_WRFIFO(fdc, NE7CMD_SPECIFY);
		FDC_WRFIFO(fdc, fd->sc_type->steprate);
		/* XXX head load time == 6ms */
		FDC_WRFIFO(fdc, 6 | NE7_SPECIFY_NODMA);

		fdc->sc_itask = FDC_ITASK_SENSEI;
		/* seek function */
		FDC_WRFIFO(fdc, NE7CMD_SEEK);
		FDC_WRFIFO(fdc, fd->sc_drive); /* drive number */
		FDC_WRFIFO(fdc, bp->b_cylinder * fd->sc_type->step);
		return (1);

	case DODSKCHG:
	dodskchg:
		/*
		 * Disk change: force a seek operation by going to cyl 1
		 * followed by a recalibrate.
		 */
		disk_busy(&fd->sc_dk);
		callout_reset(&fdc->sc_timo_ch, 4 * hz, fdctimeout, fdc);
		fd->sc_cylin = -1;
		fdc->sc_nstat = 0;
		fdc->sc_state = DSKCHGWAIT;

		fdc->sc_itask = FDC_ITASK_SENSEI;
		/* seek function */
		FDC_WRFIFO(fdc, NE7CMD_SEEK);
		FDC_WRFIFO(fdc, fd->sc_drive); /* drive number */
		FDC_WRFIFO(fdc, 1 * fd->sc_type->step);
		return (1);

	case DSKCHGWAIT:
		callout_stop(&fdc->sc_timo_ch);
		disk_unbusy(&fd->sc_dk, 0, 0);
		if (fdc->sc_nstat != 2 || (st0 & 0xf8) != 0x20 ||
		    cyl != 1 * fd->sc_type->step) {
			fdcstatus(fdc, "dskchg seek failed");
			fdc->sc_state = DORESET;
		} else
			fdc->sc_state = DORECAL;

		if (fdc_diskchange(fdc)) {
			printf("%s: cannot clear disk change status\n",
				fdc->sc_dev.dv_xname);
			fdc->sc_state = DORESET;
		}
		goto loop;

	case DOIO:
	doio:
		if (finfo != NULL)
			fd->sc_skip = (char *)&(finfo->fd_formb_cylno(0)) -
				      (char *)finfo;
		type = fd->sc_type;
		sec = fd->sc_blkno % type->seccyl;
		nblks = type->seccyl - sec;
		nblks = min(nblks, fd->sc_bcount / FD_BSIZE(fd));
		nblks = min(nblks, FDC_MAXIOSIZE / FD_BSIZE(fd));
		fd->sc_nblks = nblks;
		fd->sc_nbytes = finfo ? bp->b_bcount : nblks * FD_BSIZE(fd);
		head = sec / type->sectrac;
		sec -= head * type->sectrac;
#ifdef DIAGNOSTIC
		{int block;
		 block = (fd->sc_cylin * type->heads + head) * type->sectrac + sec;
		 if (block != fd->sc_blkno) {
			 printf("fdcintr: block %d != blkno %d\n", block, (int)fd->sc_blkno);
#ifdef DDB
			 Debugger();
#endif
		 }}
#endif
		read = bp->b_flags & B_READ;

		/* Setup for pseudo DMA */
		fdc->sc_data = bp->b_data + fd->sc_skip;
		fdc->sc_tc = fd->sc_nbytes;

		bus_space_write_1(fdc->sc_bustag, fdc->sc_handle,
				  fdc->sc_reg_drs, type->rate);
#ifdef FD_DEBUG
		if (fdc_debug > 1)
			printf("fdcstate: doio: %s drive %d "
				"track %d head %d sec %d nblks %d\n",
				finfo ? "format" :
					(read ? "read" : "write"),
				fd->sc_drive, fd->sc_cylin, head, sec, nblks);
#endif
		fdc->sc_state = IOCOMPLETE;
		fdc->sc_itask = FDC_ITASK_DMA;
		fdc->sc_nstat = 0;

		disk_busy(&fd->sc_dk);

		/* allow 3 seconds for operation */
		callout_reset(&fdc->sc_timo_ch, 3 * hz, fdctimeout, fdc);

		if (finfo != NULL) {
			/* formatting */
			FDC_WRFIFO(fdc, NE7CMD_FORMAT);
			FDC_WRFIFO(fdc, (head << 2) | fd->sc_drive);
			FDC_WRFIFO(fdc, finfo->fd_formb_secshift);
			FDC_WRFIFO(fdc, finfo->fd_formb_nsecs);
			FDC_WRFIFO(fdc, finfo->fd_formb_gaplen);
			FDC_WRFIFO(fdc, finfo->fd_formb_fillbyte);
		} else {
			if (read)
				FDC_WRFIFO(fdc, NE7CMD_READ);
			else
				FDC_WRFIFO(fdc, NE7CMD_WRITE);
			FDC_WRFIFO(fdc, (head << 2) | fd->sc_drive);
			FDC_WRFIFO(fdc, fd->sc_cylin);	/*track*/
			FDC_WRFIFO(fdc, head);
			FDC_WRFIFO(fdc, sec + 1);	/*sector+1*/
			FDC_WRFIFO(fdc, type->secsize);/*sector size*/
			FDC_WRFIFO(fdc, type->sectrac);/*secs/track*/
			FDC_WRFIFO(fdc, type->gap1);	/*gap1 size*/
			FDC_WRFIFO(fdc, type->datalen);/*data length*/
		}

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
		/* no data on seek */
		disk_unbusy(&fd->sc_dk, 0, 0);

		/* Make sure seek really happened. */
		if (fdc->sc_nstat != 2 || (st0 & 0xf8) != 0x20 ||
		    cyl != bp->b_cylinder * fd->sc_type->step) {
#ifdef FD_DEBUG
			if (fdc_debug)
				fdcstatus(fdc, "seek failed");
#endif
			fdcretry(fdc);
			goto loop;
		}
		fd->sc_cylin = bp->b_cylinder;
		goto doio;

	case IOTIMEDOUT:
		/*
		 * Try to abort the I/O operation without resetting
		 * the chip first.  Poke TC and arrange to pick up
		 * the timed out I/O command's status.
		 */
		fdc->sc_itask = FDC_ITASK_RESULT;
		fdc->sc_state = IOCLEANUPWAIT;
		fdc->sc_nstat = 0;
		/* 1/10 second should be enough */
		callout_reset(&fdc->sc_timo_ch, hz / 10, fdctimeout, fdc);
		FTC_FLIP;
		return (1);

	case IOCLEANUPTIMEDOUT:
	case SEEKTIMEDOUT:
	case RECALTIMEDOUT:
	case RESETTIMEDOUT:
	case DSKCHGTIMEDOUT:
		fdcstatus(fdc, "timeout");

		/* All other timeouts always roll through to a chip reset */
		fdcretry(fdc);

		/* Force reset, no matter what fdcretry() says */
		fdc->sc_state = DORESET;
		goto loop;

	case IOCLEANUPWAIT: /* IO FAILED, cleanup succeeded */
		callout_stop(&fdc->sc_timo_ch);
		disk_unbusy(&fd->sc_dk, (bp->b_bcount - bp->b_resid),
		    (bp->b_flags & B_READ));
		fdcretry(fdc);
		goto loop;

	case IOCOMPLETE: /* IO DONE, post-analyze */
		callout_stop(&fdc->sc_timo_ch);

		disk_unbusy(&fd->sc_dk, (bp->b_bcount - bp->b_resid),
		    (bp->b_flags & B_READ));

		if (fdc->sc_nstat != 7 || st1 != 0 ||
		    ((st0 & 0xf8) != 0 &&
		     ((st0 & 0xf8) != 0x20 || (fdc->sc_cfg & CFG_EIS) == 0))) {
#ifdef FD_DEBUG
			if (fdc_debug) {
				fdcstatus(fdc,
					bp->b_flags & B_READ
					? "read failed" : "write failed");
				printf("blkno %lld nblks %d nstat %d tc %d\n",
				       (long long)fd->sc_blkno, fd->sc_nblks,
				       fdc->sc_nstat, fdc->sc_tc);
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
			    fd->sc_skip / FD_BSIZE(fd),
			    (struct disklabel *)NULL);
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
		if (finfo == NULL && fd->sc_bcount > 0) {
			bp->b_cylinder = fd->sc_blkno / fd->sc_type->seccyl;
			goto doseek;
		}
		fdfinish(fd, bp);
		goto loop;

	case DORESET:
		/* try a reset, keep motor on */
		fd_set_motor(fdc);
		delay(100);
		fdc->sc_nstat = 0;
		fdc->sc_itask = FDC_ITASK_SENSEI;
		fdc->sc_state = RESETCOMPLETE;
		callout_reset(&fdc->sc_timo_ch, hz / 2, fdctimeout, fdc);
		fdc_reset(fdc);
		return (1);			/* will return later */

	case RESETCOMPLETE:
		callout_stop(&fdc->sc_timo_ch);
		fdconf(fdc);

		/* FALLTHROUGH */
	case DORECAL:
		fdc->sc_state = RECALWAIT;
		fdc->sc_itask = FDC_ITASK_SENSEI;
		fdc->sc_nstat = 0;
		callout_reset(&fdc->sc_timo_ch, 5 * hz, fdctimeout, fdc);
		/* recalibrate function */
		FDC_WRFIFO(fdc, NE7CMD_RECAL);
		FDC_WRFIFO(fdc, fd->sc_drive);
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
				fdcstatus(fdc, "recalibrate failed");
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
		fdcstatus(fdc, "stray interrupt");
		return (1);
	}
#ifdef DIAGNOSTIC
	panic("fdcintr: impossible");
#endif

xxx:
	/*
	 * We get here if the chip locks up in FDC_WRFIFO()
	 * Cancel any operation and schedule a reset
	 */
	callout_stop(&fdc->sc_timo_ch);
	fdcretry(fdc);
	(fdc)->sc_state = DORESET;
	goto loop;

#undef	st0
#undef	st1
#undef	cyl
}

void
fdcretry(struct fdc_softc *fdc)
{
	struct fd_softc *fd;
	struct buf *bp;
	int error = EIO;

	fd = fdc->sc_drives.tqh_first;
	bp = BUFQ_PEEK(fd->sc_q);

	fdc->sc_overruns = 0;
	if (fd->sc_opts & FDOPT_NORETRY)
		goto fail;

	switch (fdc->sc_errors) {
	case 0:
		if (fdc->sc_nstat == 7 &&
		    (fdc->sc_status[0] & 0xd8) == 0x40 &&
		    (fdc->sc_status[1] & 0x2) == 0x2) {
			printf("%s: read-only medium\n", fd->sc_dv.dv_xname);
			error = EROFS;
			goto failsilent;
		}
		/* try again */
		fdc->sc_state =
			(fdc->sc_flags & FDC_EIS) ? DOIO : DOSEEK;
		break;

	case 1: case 2: case 3:
		/* didn't work; try recalibrating */
		fdc->sc_state = DORECAL;
		break;

	case 4:
		if (fdc->sc_nstat == 7 &&
		    fdc->sc_status[0] == 0 &&
		    fdc->sc_status[1] == 0 &&
		    fdc->sc_status[2] == 0) {
			/*
			 * We've retried a few times and we've got
			 * valid status and all three status bytes
			 * are zero.  Assume this condition is the
			 * result of no disk loaded into the drive.
			 */
			printf("%s: no medium?\n", fd->sc_dv.dv_xname);
			error = ENODEV;
			goto failsilent;
		}

		/* still no go; reset the bastard */
		fdc->sc_state = DORESET;
		break;

	default:
	fail:
		if ((fd->sc_opts & FDOPT_SILENT) == 0) {
			diskerr(bp, "fd", "hard error", LOG_PRINTF,
				fd->sc_skip / FD_BSIZE(fd),
				(struct disklabel *)NULL);
			printf("\n");
			fdcstatus(fdc, "controller status");
		}

	failsilent:
		bp->b_flags |= B_ERROR;
		bp->b_error = error;
		fdfinish(fd, bp);
	}
	fdc->sc_errors++;
}

int
fdioctl(dev_t dev, u_long cmd, caddr_t addr, int flag, struct lwp *l)
{
	struct fd_softc *fd;
	struct fdc_softc *fdc;
	struct fdformat_parms *form_parms;
	struct fdformat_cmd *form_cmd;
	struct ne7_fd_formb *fd_formb;
	int il[FD_MAX_NSEC + 1];
	int unit;
	int i, j;
	int error;

	unit = FDUNIT(dev);
	if (unit >= fd_cd.cd_ndevs)
		return (ENXIO);

	fd = fd_cd.cd_devs[FDUNIT(dev)];
	fdc = (struct fdc_softc *)device_parent(&fd->sc_dv);

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
		fd_do_eject(fd);
		return (0);

	case FDIOCGETFORMAT:
		form_parms = (struct fdformat_parms *)addr;
		form_parms->fdformat_version = FDFORMAT_VERSION;
		form_parms->nbps = 128 * (1 << fd->sc_type->secsize);
		form_parms->ncyl = fd->sc_type->cylinders;
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
		fd->sc_type->cylinders = form_parms->ncyl;
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
		    form_cmd->cylinder >= fd->sc_type->cylinders) {
			return (EINVAL);
		}

		fd_formb = malloc(sizeof(struct ne7_fd_formb),
		    M_TEMP, M_NOWAIT);
		if (fd_formb == 0)
			return (ENOMEM);

		fd_formb->head = form_cmd->head;
		fd_formb->cyl = form_cmd->cylinder;
		fd_formb->transfer_rate = fd->sc_type->rate;
		fd_formb->fd_formb_secshift = fd->sc_type->secsize;
		fd_formb->fd_formb_nsecs = fd->sc_type->sectrac;
		fd_formb->fd_formb_gaplen = fd->sc_type->gap2;
		fd_formb->fd_formb_fillbyte = fd->sc_type->fillbyte;

		bzero(il, sizeof il);
		for (j = 0, i = 1; i <= fd_formb->fd_formb_nsecs; i++) {
			while (il[(j%fd_formb->fd_formb_nsecs) + 1])
				j++;
			il[(j%fd_formb->fd_formb_nsecs) + 1] = i;
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
		return (0);

	case FDIOCSETOPTS:		/* set drive options */
		fd->sc_opts = *(int *)addr;
		return (0);

#ifdef FD_DEBUG
	case _IO('f', 100):
		fdc_wrfifo(fdc, NE7CMD_DUMPREG);
		fdcresult(fdc);
		printf("fdc: dumpreg(%d regs): <", fdc->sc_nstat);
		for (i = 0; i < fdc->sc_nstat; i++)
			printf(" 0x%x", fdc->sc_status[i]);
		printf(">\n");
		return (0);

	case _IOW('f', 101, int):
		fdc->sc_cfg &= ~CFG_THRHLD_MASK;
		fdc->sc_cfg |= (*(int *)addr & CFG_THRHLD_MASK);
		fdconf(fdc);
		return (0);

	case _IO('f', 102):
		fdc_wrfifo(fdc, NE7CMD_SENSEI);
		fdcresult(fdc);
		printf("fdc: sensei(%d regs): <", fdc->sc_nstat);
		for (i=0; i< fdc->sc_nstat; i++)
			printf(" 0x%x", fdc->sc_status[i]);
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
fdformat(dev_t dev, struct ne7_fd_formb *finfo, struct proc *p)
{
	int rv = 0;
	struct fd_softc *fd = fd_cd.cd_devs[FDUNIT(dev)];
	struct fd_type *type = fd->sc_type;
	struct buf *bp;

	/* set up a buffer header for fdstrategy() */
	bp = getiobuf_nowait();
	if (bp == NULL)
		return (ENOBUFS);

	bp->b_vp = NULL;
	bp->b_flags = B_BUSY | B_PHYS | B_FORMAT;
	bp->b_proc = p;
	bp->b_dev = dev;

	/*
	 * Calculate a fake blkno, so fdstrategy() would initiate a
	 * seek to the requested cylinder.
	 */
	bp->b_blkno = ((finfo->cyl * (type->sectrac * type->heads)
		       + finfo->head * type->sectrac) * FD_BSIZE(fd))
		      / DEV_BSIZE;

	bp->b_bcount = sizeof(struct fd_idfield_data) * finfo->fd_formb_nsecs;
	bp->b_data = (caddr_t)finfo;

#ifdef FD_DEBUG
	if (fdc_debug) {
		int i;

		printf("fdformat: blkno 0x%llx count %d\n",
			(unsigned long long)bp->b_blkno, bp->b_bcount);

		printf("\tcyl:\t%d\n", finfo->cyl);
		printf("\thead:\t%d\n", finfo->head);
		printf("\tnsecs:\t%d\n", finfo->fd_formb_nsecs);
		printf("\tsshft:\t%d\n", finfo->fd_formb_secshift);
		printf("\tgaplen:\t%d\n", finfo->fd_formb_gaplen);
		printf("\ttrack data:");
		for (i = 0; i < finfo->fd_formb_nsecs; i++) {
			printf(" [c%d h%d s%d]",
					finfo->fd_formb_cylno(i),
					finfo->fd_formb_headno(i),
					finfo->fd_formb_secno(i) );
			if (finfo->fd_formb_secsize(i) != 2)
				printf("<sz:%d>", finfo->fd_formb_secsize(i));
		}
		printf("\n");
	}
#endif

	/* now do the format */
	fdstrategy(bp);

	/* ...and wait for it to complete */
	rv = biowait(bp);
	putiobuf(bp);
	return (rv);
}

void
fdgetdisklabel(dev_t dev)
{
	int unit = FDUNIT(dev), i;
	struct fd_softc *fd = fd_cd.cd_devs[unit];
	struct disklabel *lp = fd->sc_dk.dk_label;
	struct cpu_disklabel *clp = fd->sc_dk.dk_cpulabel;

	bzero(lp, sizeof(struct disklabel));
	bzero(lp, sizeof(struct cpu_disklabel));

	lp->d_type = DTYPE_FLOPPY;
	lp->d_secsize = FD_BSIZE(fd);
	lp->d_secpercyl = fd->sc_type->seccyl;
	lp->d_nsectors = fd->sc_type->sectrac;
	lp->d_ncylinders = fd->sc_type->cylinders;
	lp->d_ntracks = fd->sc_type->heads;	/* Go figure... */
	lp->d_secperunit = lp->d_secpercyl * lp->d_ncylinders;
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
fd_do_eject(struct fd_softc *fd)
{
	struct fdc_softc *fdc = (void *)device_parent(&fd->sc_dv);

	if (CPU_ISSUN4C) {
		auxregbisc(AUXIO4C_FDS, AUXIO4C_FEJ);
		delay(10);
		auxregbisc(AUXIO4C_FEJ, AUXIO4C_FDS);
		return;
	}
	if (CPU_ISSUN4M && (fdc->sc_flags & FDC_82077) != 0) {
		bus_space_tag_t t = fdc->sc_bustag;
		bus_space_handle_t h = fdc->sc_handle;
		uint8_t dor = FDO_FRST | FDO_FDMAEN | FDO_MOEN(0);

		bus_space_write_1(t, h, fdc->sc_reg_dor, dor | FDO_EJ);
		delay(10);
		bus_space_write_1(t, h, fdc->sc_reg_dor, FDO_FRST | FDO_DS);
		return;
	}
}

/* ARGSUSED */
void
fd_mountroot_hook(struct device *dev)
{
	int c;

	fd_do_eject((struct fd_softc *)dev);
	printf("Insert filesystem floppy and press return.");
	for (;;) {
		c = cngetc();
		if ((c == '\r') || (c == '\n')) {
			printf("\n");
			break;
		}
	}
}

#ifdef MEMORY_DISK_HOOKS

#define FDMICROROOTSIZE ((2*18*80) << DEV_BSHIFT)

int
fd_read_md_image(size_t	*sizep, caddr_t	*addrp)
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
	fd_do_eject(fd_cd.cd_devs[FDUNIT(dev)]);
	return (0);
}
#endif /* MEMORY_DISK_HOOKS */
