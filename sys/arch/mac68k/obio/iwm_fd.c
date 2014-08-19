/*	$NetBSD: iwm_fd.c,v 1.46.24.2 2014/08/20 00:03:11 tls Exp $	*/

/*
 * Copyright (c) 1997, 1998 Hauke Fath.  All rights reserved.
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
 * iwm_fd.c -- Sony (floppy disk) driver for m68k Macintoshes
 *
 * The present implementation supports the GCR format (800K) on
 * non-{DMA,IOP} machines.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iwm_fd.c,v 1.46.24.2 2014/08/20 00:03:11 tls Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/event.h>

#define FSTYPENAMES
#define DKTYPENAMES
#include <sys/disklabel.h>

#include <sys/disk.h>
#include <sys/dkbad.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/conf.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <mac68k/obio/iwmreg.h>
#include <mac68k/obio/iwm_fdvar.h>

/**
 **	Private functions
 **/
static int map_iwm_base(vm_offset_t);

/* Autoconfig */
int	iwm_match(device_t, cfdata_t, void *);
void	iwm_attach(device_t, device_t, void *);
int	iwm_print(void *, const char *);
int	fd_match(device_t, cfdata_t, void *);
void	fd_attach(device_t, device_t, void *);
int	fd_print(void *, const char *);

/* Disklabel stuff */
static void fdGetDiskLabel(fd_softc_t *, dev_t);
static void fdPrintDiskLabel(struct disklabel *);

static fdInfo_t *getFDType(short);
static fdInfo_t *fdDeviceToType(fd_softc_t *, dev_t);

static void fdstart(fd_softc_t *);
static void remap_geometry(daddr_t, int, diskPosition_t *);
static void motor_off(void *);
static int seek(fd_softc_t *, int);
static int checkTrack(diskPosition_t *, int);
static int initCylinderCache(fd_softc_t *);
static void invalidateCylinderCache(fd_softc_t *);

static int fdstart_Init(fd_softc_t *);
static int fdstart_Seek(fd_softc_t *);
static int fdstart_Read(fd_softc_t *);
static int fdstart_Write(fd_softc_t *);
static int fdstart_Flush(fd_softc_t *);
static int fdstart_IOFinish(fd_softc_t *);
static int fdstart_IOErr(fd_softc_t *);
static int fdstart_Fault(fd_softc_t *);
static int fdstart_Exit(fd_softc_t *);


/**
 **	Driver debugging
 **/

#ifdef DEBUG
#define IWM_DEBUG
#endif


static void hexDump(u_char *, int);

/*
 * Stuff taken from Egan/Teixeira ch 8: 'if(TRACE_FOO)' debug output 
 * statements don't break indentation, and when DEBUG is not defined, 
 * the compiler code optimizer drops them as dead code.
 */
#ifdef IWM_DEBUG
#define M_TRACE_CONFIG	0x0001
#define M_TRACE_OPEN	0x0002
#define M_TRACE_CLOSE	0x0004
#define M_TRACE_READ	0x0008
#define M_TRACE_WRITE	0x0010
#define M_TRACE_STRAT	(M_TRACE_READ | M_TRACE_WRITE)
#define M_TRACE_IOCTL	0x0020
#define M_TRACE_STEP	0x0040
#define M_TRACE_ALL	0xFFFF

#define TRACE_CONFIG	(iwmDebugging & M_TRACE_CONFIG)
#define TRACE_OPEN	(iwmDebugging & M_TRACE_OPEN)
#define TRACE_CLOSE	(iwmDebugging & M_TRACE_CLOSE)
#define TRACE_READ	(iwmDebugging & M_TRACE_READ)
#define TRACE_WRITE	(iwmDebugging & M_TRACE_WRITE)
#define TRACE_STRAT	(iwmDebugging & M_TRACE_STRAT)
#define TRACE_IOCTL	(iwmDebugging & M_TRACE_IOCTL)
#define TRACE_STEP	(iwmDebugging & M_TRACE_STEP)
#define TRACE_ALL	(iwmDebugging & M_TRACE_ALL)

 /* -1 = all active */
int     iwmDebugging = 0 /* | M_TRACE_OPEN | M_TRACE_STRAT | M_TRACE_IOCTL */ ;

#else
#define TRACE_CONFIG	0
#define TRACE_OPEN	0
#define TRACE_CLOSE	0
#define TRACE_READ	0
#define TRACE_WRITE	0
#define TRACE_STRAT	0
#define TRACE_IOCTL	0
#define TRACE_STEP	0
#define TRACE_ALL	0
#endif

#define DISABLED 	0



/**
 ** Module-global Variables
 **/

/* The IWM base address */
u_long IWMBase;

/*
 * Table of supported disk types.
 * The table order seems to be pretty standardized across NetBSD ports, but
 * then, they are all MFM... So we roll our own for now.
 */
static fdInfo_t fdTypes[] = {
	{1, 80, 512, 10, 10,  800, 12, 2, IWM_GCR, "400K Sony"},
	{2, 80, 512, 10, 20, 1600, 12, 2, IWM_GCR, "800K Sony"}
};

/* Table of GCR disk zones for one side (see IM II-211, The Disk Driver) */
static diskZone_t diskZones[] = {
	{16, 12,   0, 191},
	{16, 11, 192, 367},
	{16, 10, 368, 527},
	{16,  9, 528, 671},
	{16,  8, 672, 799}
};

/* Drive format codes/indexes */
enum {
	IWM_400K_GCR = 0,
	IWM_800K_GCR = 1,
	IWM_720K_MFM = 2,
	IWM_1440K_MFM = 3
};


/**
 ** Autoconfiguration code
 **/

/*
 * Autoconfig data structures
 *
 * These data structures (see <sys/device.h>) are referenced in
 * compile/$KERNEL/ioconf.c, which is generated by config(8).
 * Their names are formed like {device}_{ca,cd}.
 *
 * {device}_ca  
 * is used for dynamically allocating driver data, probing and
 * attaching a device;
 *
 * {device}_cd
 * references all found devices of a type.
 */

extern struct cfdriver iwm_cd;
extern struct cfdriver fd_cd;

/* IWM floppy disk controller */
CFATTACH_DECL_NEW(iwm, sizeof(iwm_softc_t),
    iwm_match, iwm_attach, NULL, NULL);

/* Attached floppy disk drives */
CFATTACH_DECL_NEW(fd, sizeof(fd_softc_t),
    fd_match, fd_attach, NULL, NULL);

dev_type_open(fdopen);
dev_type_close(fdclose);
dev_type_read(fdread);
dev_type_write(fdwrite);
dev_type_ioctl(fdioctl);
dev_type_strategy(fdstrategy);

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

/* disk(9) framework device switch */
struct dkdriver fd_dkDriver = {
	fdstrategy
};

/***  Configure the IWM controller  ***/

/*
 * iwm_match
 *
 * Is the IWM chip present? Here, *aux is a ptr to struct confargs 
 * (see <mac68k/mac68k/autoconf.h>), which does not hold any information 
 * to match against. After all, that's what the obio concept is 
 * about: Onboard components that are present depending (only) 
 * on machine type.
 */
int
iwm_match(device_t parent, cfdata_t match, void *aux)
{
	int matched;
	extern u_long IOBase;		/* from mac68k/machdep.c */
	extern u_long IWMBase;
	
	if (0 == map_iwm_base(IOBase)) {
		/* 
		 * Unknown machine HW:
		 * The SWIM II/III chips that are present in post-Q700
		 * '040 Macs have dropped the IWM register structure.
		 * We know next to nothing about the SWIM.
		 */
		matched = 0;
		if (TRACE_CONFIG)
			printf("IWM or SWIM not found: Unknown location (SWIM II?).\n");
	} else {
		matched = 1;
		if (TRACE_CONFIG) {
			printf("iwm: IWMBase mapped to 0x%lx in VM.\n", 
			    IWMBase);
		}
	}
	return matched;
}


/*
 * iwm_attach
 *
 * The IWM is present, initialize it. Then look up the connected drives
 * and attach them.
 */
void
iwm_attach(device_t parent, device_t self, void *aux)
{
	int iwmErr;
	iwm_softc_t *iwm;
	iwmAttachArgs_t ia;

	printf(": Apple GCR floppy disk controller\n");
	iwm = device_private(self);

	iwmErr = iwmInit();
	if (TRACE_CONFIG)
		printf("initIWM() says %d.\n", iwmErr);

	if (0 == iwmErr) {
		/* Set up the IWM softc */
		iwm->maxRetries = 10;

		/* Look for attached drives */
		for (ia.unit = 0; ia.unit < IWM_MAX_DRIVE; ia.unit++) {
			iwm->fd[ia.unit] = NULL;
			ia.driveType = getFDType(ia.unit);
			if (NULL != ia.driveType)
				config_found(self, (void *)&ia,
				    fd_print);
		}
		if (TRACE_CONFIG)
			printf("iwm: Initialization completed.\n");
	} else {
		printf("iwm: Chip revision not supported (%d)\n", iwmErr);
	}
}


/*
 * iwm_print -- print device configuration.
 *
 * If the device is not configured 'controller' it is NULL and 
 * we print a message in the *Attach routine; the return value 
 * of *Print() is ignored.
 */
int
iwm_print(void *aux, const char *controller)
{
	return UNCONF;
}


/*
 * map_iwm_base
 *
 * Map physical IO address of IWM to VM address
 */
static int
map_iwm_base(vm_offset_t base)
{
	int known;
	extern u_long IWMBase;
	
	switch (current_mac_model->class) {
	case MACH_CLASSQ:
	case MACH_CLASSQ2:
	case MACH_CLASSP580:
		IWMBase = base + 0x1E000;
		known = 1;
		break;
	case MACH_CLASSII:
	case MACH_CLASSPB:
	case MACH_CLASSDUO:
	case MACH_CLASSIIci:
	case MACH_CLASSIIsi:
	case MACH_CLASSIIvx:
	case MACH_CLASSLC:
		IWMBase = base + 0x16000;
		known = 1;
		break;
	case MACH_CLASSIIfx:
	case MACH_CLASSAV:
	default:
		/* 
		 * Neither IIfx/Q9[05]0 style IOP controllers nor 
		 * Q[68]40AV DMA based controllers are supported. 
		 */
		if (TRACE_CONFIG)
			printf("Unknown floppy controller chip.\n");
		IWMBase = 0L;
		known = 0;
		break;
	}
	return known;
}


/***  Configure Sony disk drive(s)  ***/

/*
 * fd_match
 */
int
fd_match(device_t parent, cfdata_t match, void *aux)
{
	int matched, cfUnit;
	struct cfdata *cfp;
	iwmAttachArgs_t *fdParams;

	cfp = match;
	fdParams = aux;
	cfUnit = cfp->cf_loc[IWMCF_DRIVE];
	matched = (cfUnit == fdParams->unit || cfUnit == -1) ? 1 : 0;
	if (TRACE_CONFIG) {
		printf("fdMatch() drive %d ? cfUnit = %d\n",
		    fdParams->unit, cfUnit);
	}
	return matched;
}


/*
 * fd_attach
 *
 * We have checked that the IWM is fine and the drive is present,
 * so we can attach it.
 */
void
fd_attach(device_t parent, device_t self, void *aux)
{
	iwm_softc_t *iwm;
	fd_softc_t *fd;
	iwmAttachArgs_t *ia;
	int driveInfo;

	iwm = device_private(parent);
	fd = device_private(self);
	ia = aux;

	driveInfo = iwmCheckDrive(ia->unit);

	fd->currentType = ia->driveType;
	fd->unit = ia->unit;
	fd->defaultType = &fdTypes[IWM_800K_GCR];
	fd->stepDirection = 0;

	iwm->fd[ia->unit] = fd;		/* iwm has ptr to this drive */
	iwm->drives++;

	bufq_alloc(&fd->bufQueue, "disksort", BUFQ_SORT_CYLINDER);
	callout_init(&fd->motor_ch, 0);

	printf(" drive %d: ", fd->unit);

	if (IWM_NO_DISK & driveInfo) {
		printf("(drive empty)\n");
	} else
		if (!(IWM_DD_DISK & driveInfo)) {
			printf("(HD disk -- not supported)\n");
			iwmDiskEject(fd->unit);	/* XXX */
		} else {
			printf("%s %d cyl, %d head(s)\n",
			    fd->currentType->description,
			    fd->currentType->tracks,
			    fd->currentType->heads);
		}
	if (TRACE_CONFIG) {
		int reg, flags, spl;

		/* List contents of drive status registers */
		spl = spl6();
		for (reg = 0; reg < 0x10; reg++) {
			flags = iwmQueryDrvFlag(fd->unit, reg);
			printf("iwm: Drive register 0x%x = 0x%x\n", reg, flags);
		}
		splx(spl);
	}
	disk_init(&fd->diskInfo, device_xname(fd->sc_dev), &fd_dkDriver);
	disk_attach(&fd->diskInfo);
}


/*
 * fdPrint -- print device configuration.
 *
 * If the device is not configured 'controller' refers to a name string
 * we print here.
 * Else it is NULL and we print a message in the *Attach routine; the
 * return value of *Print() is ignored.
 */
int
fd_print(void *aux, const char *controller)
{
	iwmAttachArgs_t *ia;

	ia = aux;
	if (NULL != controller)
		aprint_normal("fd%d at %s", ia->unit, controller);
	return UNCONF;
}

/**
 ** Implementation section of driver interface
 **
 ** The prototypes for these functions are set up automagically
 ** by macros in mac68k/conf.c. Their names are generated from {fd}
 ** and {open,close,strategy,dump,size,read,write}. The driver entry
 ** points are then plugged into bdevsw[] and cdevsw[].
 **/


/*
 * fdopen
 *
 * Open a floppy disk device.
 */
int
fdopen(dev_t dev, int flags, int devType, struct lwp *l)
{
	fd_softc_t *fd;
	fdInfo_t *info;
	int partitionMask;
	int fdType, fdUnit;
	int ierr, err;
	iwm_softc_t *iwm = device_lookup_private(&iwm_cd, 0); /* XXX */
	info = NULL;		/* XXX shut up egcs */
	fd = NULL;		/* XXX shut up gcc3 */

	/*
	 * See <device.h> for struct cfdriver, <disklabel.h> for
	 * DISKUNIT() and <atari/atari/device.h> for getsoftc().
	 */
	fdType = minor(dev) % MAXPARTITIONS;
	fdUnit = minor(dev) / MAXPARTITIONS;
	if (TRACE_OPEN)
		printf("iwm: Open drive %d", fdUnit);

	/* Check if device # is valid */
	err = (iwm->drives < fdUnit) ? ENXIO : 0;
	if (!err) {
		(void)iwmSelectDrive(fdUnit);
		if (TRACE_OPEN)
			printf(".\n Get softc");

		/* Get fd state */
		fd = iwm->fd[fdUnit];
		err = (NULL == fd) ? ENXIO : 0;
	}
	if (!err) {
		if (fd->state & IWM_FD_IS_OPEN) {
			/* 
			 * Allow multiple open calls only if for identical
			 * floppy format. 
			 */
			if (TRACE_OPEN)
				printf(".\n Drive already opened!\n");
			err = (fd->partition == fdType) ? 0 : ENXIO;
		} else {
			if (TRACE_OPEN)
				printf(".\n Get format info");

			/* Get format type */
			info = fdDeviceToType(fd, dev);
			if (NULL == info) {
				err = ENXIO;
				if (TRACE_OPEN)
					printf(".\n No such drive.\n");
			}
		}
	}
	if (!err && !(fd->state & IWM_FD_IS_OPEN)) {
		if (TRACE_OPEN)
			printf(".\n Set diskInfo flags.\n");

		fd->writeLabel = 0;		/* XXX currently unused */
		fd->partition = fdType;
		fd->currentType = info;
		fd->drvFlags = iwmCheckDrive(fd->unit);

		if (fd->drvFlags & IWM_NO_DISK) {
			err = EIO;
#ifdef DIAGNOSTIC
			printf(" Drive %d is empty.\n", fd->unit);
#endif
		} else {
			if (!(fd->drvFlags & IWM_WRITABLE) && (flags & FWRITE)) {

				err = EPERM;
#ifdef DIAGNOSTIC
				printf(" Disk is write protected.\n");
#endif
			} else {
				if (!(fd->drvFlags & IWM_DD_DISK)) {
					err = ENXIO;
#ifdef DIAGNOSTIC
					printf(" HD format not supported.\n");
#endif
					(void)iwmDiskEject(fd->unit);
				} else {
					/* We're open now! */
					fd->state |= IWM_FD_IS_OPEN;
					err = initCylinderCache(fd);
				}
			}
		}
	}
	if (!err) {
		/*
		 * Later, we might not want to recalibrate the drive when it
		 * is already open. For now, it doesn't hurt.
		 */
		if (TRACE_OPEN)
			printf(" Seek track 00 says");

		memset(&fd->pos, 0, sizeof(diskPosition_t));
		ierr = seek(fd, IWM_SEEK_RECAL);
		if (TRACE_OPEN)
			printf(" %d.\n", ierr);
		err = (0 == ierr) ? 0 : EIO;		
	}
	if (!err) {
		/*
		 * Update disklabel if we are not yet open.
		 * (We shouldn't be: We are synchronous.)
		 */
		if (fd->diskInfo.dk_openmask == 0)
			fdGetDiskLabel(fd, dev);

		partitionMask = (1 << fdType);

		switch (devType) {
		case S_IFCHR:
			fd->diskInfo.dk_copenmask |= partitionMask;
			break;

		case S_IFBLK:
			fd->diskInfo.dk_bopenmask |= partitionMask;
			break;
		}
		fd->diskInfo.dk_openmask =
		    fd->diskInfo.dk_copenmask | fd->diskInfo.dk_bopenmask;
	}
	if (TRACE_OPEN)
		printf("iwm: fdopen() says %d.\n", err);
	return err;
}


/*
 * fdclose
 */
int
fdclose(dev_t dev, int flags, int devType, struct lwp *l)
{
	fd_softc_t *fd;
	int partitionMask, fdUnit, fdType;
	iwm_softc_t *iwm = device_lookup_private(&iwm_cd, 0);

	if (TRACE_CLOSE)
		printf("iwm: Closing driver.");
	fdUnit = minor(dev) / MAXPARTITIONS;
	fdType = minor(dev) % MAXPARTITIONS;
	fd = iwm->fd[fdUnit];
	/* release cylinder cache memory */
	if (fd->cbuf != NULL)
		     free(fd->cbuf, M_DEVBUF);
	
	partitionMask = (1 << fdType);

	/* Set state flag. */
	fd->state &= ~IWM_FD_IS_OPEN;

	switch (devType) {
	case S_IFCHR:
		fd->diskInfo.dk_copenmask &= ~partitionMask;
		break;

	case S_IFBLK:
		fd->diskInfo.dk_bopenmask &= ~partitionMask;
		break;
	}
	fd->diskInfo.dk_openmask =
	    fd->diskInfo.dk_copenmask | fd->diskInfo.dk_bopenmask;
	return 0;
}


/*
 * fdioctl
 *
 * We deal with all the disk-specific ioctls in <sys/dkio.h> here even if
 * we do not support them.
 */
int
fdioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	int result, fdUnit, fdType;
	fd_softc_t *fd;
	iwm_softc_t *iwm = device_lookup_private(&iwm_cd, 0);

	if (TRACE_IOCTL)
		printf("iwm: Execute ioctl... ");

	/* Check if device # is valid and get its softc */
	fdUnit = minor(dev) / MAXPARTITIONS;
	fdType = minor(dev) % MAXPARTITIONS;
	if (fdUnit >= iwm->drives) {
		if (TRACE_IOCTL) {
			printf("iwm: Wanted device no (%d) is >= %d.\n",
			    fdUnit, iwm->drives);
		}
		return ENXIO;
	}
	fd = iwm->fd[fdUnit];
	result = 0;

	switch (cmd) {
	case DIOCGDINFO:
		if (TRACE_IOCTL)
			printf(" DIOCGDINFO: Get in-core disklabel.\n");
		*(struct disklabel *) data = *(fd->diskInfo.dk_label);
		result = 0;
		break;

	case DIOCSDINFO:
		if (TRACE_IOCTL)
			printf(" DIOCSDINFO: Set in-core disklabel.\n");
		result = ((flags & FWRITE) == 0) ? EBADF : 0;
		if (result == 0)
			result = setdisklabel(fd->diskInfo.dk_label,
			    (struct disklabel *)data, 0,
			    fd->diskInfo.dk_cpulabel);
		break;

	case DIOCWDINFO:
		if (TRACE_IOCTL)
			printf(" DIOCWDINFO: Set in-core disklabel "
			    "& update disk.\n");
		result = ((flags & FWRITE) == 0) ? EBADF : 0;

		if (result == 0)
			result = setdisklabel(fd->diskInfo.dk_label,
			    (struct disklabel *)data, 0,
			    fd->diskInfo.dk_cpulabel);
		if (result == 0)
			result = writedisklabel(dev, fdstrategy,
			    fd->diskInfo.dk_label,
			    fd->diskInfo.dk_cpulabel);
		break;

	case DIOCGPART:
		if (TRACE_IOCTL)
			printf(" DIOCGPART: Get disklabel & partition table.\n");
		((struct partinfo *)data)->disklab = fd->diskInfo.dk_label;
		((struct partinfo *)data)->part =
		    &fd->diskInfo.dk_label->d_partitions[fdType];
		result = 0;
		break;

	case DIOCRFORMAT:
	case DIOCWFORMAT:
		if (TRACE_IOCTL)
			printf(" DIOC{R,W}FORMAT: No formatter support (yet?).\n");
		result = EINVAL;
		break;

	case DIOCSSTEP:
		if (TRACE_IOCTL)
			printf(" DIOCSSTEP: IWM does step handshake.\n");
		result = EINVAL;
		break;

	case DIOCSRETRIES:
		if (TRACE_IOCTL)
			printf(" DIOCSRETRIES: Set max. # of retries.\n");
		if (*(int *)data < 0)
			result = EINVAL;
		else {
			iwm->maxRetries = *(int *)data;
			result = 0;
		}
		break;

	case DIOCWLABEL:
		if (TRACE_IOCTL)
			printf(" DIOCWLABEL: Set write access to disklabel.\n");
		result = ((flags & FWRITE) == 0) ? EBADF : 0;

		if (result == 0)
			fd->writeLabel = *(int *)data;
		break;

	case DIOCSBAD:
		if (TRACE_IOCTL)
			printf(" DIOCSBAD: No bad144-style handling.\n");
		result = EINVAL;
		break;

	case ODIOCEJECT:
	case DIOCEJECT:
		/* XXX Eject disk only when unlocked */
		if (TRACE_IOCTL)
			printf(" DIOCEJECT: Eject disk from unit %d.\n",
			    fd->unit);
		result = iwmDiskEject(fd->unit);
		break;

	case DIOCLOCK:
		/* XXX Use lock to prevent ejectimg a mounted disk */
		if (TRACE_IOCTL)
			printf(" DIOCLOCK: No need to (un)lock Sony drive.\n");
		result = 0;
		break;

	default:
		if (TRACE_IOCTL)
			printf(" Not a disk related ioctl!\n");
		result = ENOTTY;
		break;
	}
	return result;
}


/*
 * fdread
 */
int
fdread(dev_t dev, struct uio *uio, int flags)
{
	return physio(fdstrategy, NULL, dev, B_READ, minphys, uio);
}


/*
 * fdwrite
 */
int
fdwrite(dev_t dev, struct uio *uio, int flags)
{
	return physio(fdstrategy, NULL, dev, B_WRITE, minphys, uio);
}


/*
 * fdstrategy
 *
 * Entry point for read and write requests. The strategy routine usually
 * queues io requests and kicks off the next transfer if the device is idle;
 * but we get no interrupts from the IWM and have to do synchronous
 * transfers - no queue.
 */
void
fdstrategy(struct buf *bp)
{
	int fdUnit, err, done, spl;
	int sectSize, transferSize;
	diskPosition_t physDiskLoc;
	fd_softc_t *fd;
	iwm_softc_t *iwm = device_lookup_private(&iwm_cd, 0);

	err = 0;
	done = 0;
	sectSize = 0;		/* XXX shut up gcc3 */
	fd = NULL;		/* XXX shut up gcc3 */

	fdUnit = minor(bp->b_dev) / MAXPARTITIONS;
	if (TRACE_STRAT) {
		printf("iwm: fdstrategy()...\n");
		printf("     struct buf is at %p\n", bp);
		printf("     Allocated buffer size (b_bufsize): 0x0%x\n",
		    bp->b_bufsize);
		printf("     Base address of buffer (b_data): %p\n",
		    bp->b_data);
		printf("     Bytes to be transferred (b_bcount): 0x0%x\n",
		    bp->b_bcount);
		printf("     Remaining I/O (b_resid): 0x0%x\n",
		    bp->b_resid);
	}
	/* Check for valid fd unit, controller and io request */

	if (fdUnit >= iwm->drives) {
		if (TRACE_STRAT)
			printf(" No such unit (%d)\n", fdUnit);
		err = EINVAL;
	}
	if (!err) {
		fd = iwm->fd[fdUnit];
		err = (NULL == fd) ? EINVAL : 0;
	}
	if (!err) {
		sectSize = fd->currentType->sectorSize;
		if (bp->b_blkno < 0
		    || (bp->b_bcount % sectSize) != 0) {
			if (TRACE_STRAT)
				printf(" Illegal transfer size: "
				    "block %lld, %d bytes\n",
				    (long long) bp->b_blkno, bp->b_bcount);
			err = EINVAL;
		}
	}
	if (!err) {
		/* Null transfer: Return, nothing to do. */
		if (0 == bp->b_bcount) {
			if (TRACE_STRAT)
				printf(" Zero transfer length.\n");
			done = 1;
		}
	}
	if (!err && !done) {
		/* What to do if we touch the boundaries of the disk? */
		transferSize = (bp->b_bcount + (sectSize - 1)) / sectSize;
		if (bp->b_blkno + transferSize > fd->currentType->secPerDisk) {
			if (TRACE_STRAT) {
				printf("iwm: Transfer beyond end of disk!\n" \
				    " (Starting block %lld, # of blocks %d," \
				    " last disk block %d).\n",
				    (long long) bp->b_blkno, transferSize,
				    fd->currentType->secPerDisk);
			}
			/* Return EOF if we are exactly at the end of the
			 * disk, EINVAL if we try to reach past the end; else
			 * truncate the request. */
			transferSize = fd->currentType->secPerDisk -
			    bp->b_blkno;
			if (0 == transferSize) {
				bp->b_resid = bp->b_bcount;
				done = 1;
			} else
				if (0 > transferSize)
					err = EINVAL;
				else
					bp->b_bcount = transferSize << DEV_BSHIFT;
		}
	}
	if (!err && !done) {
		/*
		 * Calculate cylinder # for disksort().
		 *
		 * XXX Shouldn't we use the (fake) logical cyl no here?
		 */
		remap_geometry(bp->b_blkno, fd->currentType->heads,
		    &physDiskLoc);
		bp->b_rawblkno = bp->b_blkno;
		bp->b_cylinder = physDiskLoc.track;

		if (TRACE_STRAT) {
			printf(" This job starts at b_blkno %lld; ",
			    (long long) bp->b_blkno);
			printf("it gets sorted for cylinder # %d.\n",
			    bp->b_cylinder);
		}
		spl = splbio();
		callout_stop(&fd->motor_ch);
		bufq_put(fd->bufQueue, bp);
		if (fd->sc_active == 0)
			fdstart(fd);
		splx(spl);
	}
	/* Clean up, if necessary */
	else {
		if (TRACE_STRAT)
			printf(" fdstrategy() finished early, err = %d.\n",
			    err);
		if (err)
			bp->b_error = err;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
	}
	/* Comment on results */
	if (TRACE_STRAT) {
		printf("iwm: fdstrategy() done.\n");
		printf("     We have b_resid = %d bytes left, " \
		    "b_error is %d;\n", bp->b_resid, bp->b_error);
		printf("     b_flags are 0x0%x.\n", bp->b_flags);
	}
}



/* ======================================================================== */


/*
 * fdstart
 *
 * we are called from the strategy() routine to perform a data transfer.
 *
 * The disk(9) framework demands we run at splbio(); our caller
 * takes care of that.
 *
 * Wish we had pascalish local functions here...
 */

/* fdstart FSM states */
enum {
	state_Init = 0,
	state_Seek,
	state_Read,
	state_Write,
	state_Flush,
	state_IOFinish,
	state_IOErr,
	state_Fault,
	state_Exit,
	state_Done
};

static void
fdstart(fd_softc_t *fd)
{
	int st;

	static const char *stateDesc[] = {
		"Init",
		"Seek",
		"Read",
		"Write",
		"Flush",
		"IOFinish",
		"IOErr",
		"Fault",
		"Exit",
		"Done"
	};
	int (*state[])(fd_softc_t *) = {
		fdstart_Init,
		fdstart_Seek,
		fdstart_Read,
		fdstart_Write,
		fdstart_Flush,
		fdstart_IOFinish,
		fdstart_IOErr,
		fdstart_Fault,
		fdstart_Exit
	};
	
	st = state_Init;
	do {
		if (TRACE_STRAT)
			printf(" fdstart state %d [%s] ", 
			    st, stateDesc[st]);

		st = (*state[st])(fd);

		if (TRACE_STRAT)
			printf(".\n");
	} while (st != state_Done);
}


/*
 * fdstart_Init 
 *
 * Set up things
 */
static int
fdstart_Init(fd_softc_t *fd)
{
	struct buf *bp;
	
	/*
	 * Get the first entry from the queue. This is the buf we gave to
	 * fdstrategy(); disksort() put it into our softc.
	 */
	bp = bufq_peek(fd->bufQueue);
	if (NULL == bp) {
		if (TRACE_STRAT)
			printf("Queue empty: Nothing to do");
		return state_Done;
	}
	fd->ioDirection = bp->b_flags & B_READ;

	disk_busy(&fd->diskInfo);
	if (!(fd->state & IWM_FD_MOTOR_ON)) {
		iwmMotor(fd->unit, 1);
		fd->state |= IWM_FD_MOTOR_ON;
	}
	fd->current_buffer = bp->b_data;

	/* XXX - assumes blocks of 512 bytes */
	fd->startBlk = bp->b_blkno;

	fd->iwmErr = 0;
	fd->ioRetries = 0;		/* XXX */
	fd->seekRetries = 0;
	fd->bytesDone = 0;
	fd->bytesLeft = bp->b_bcount;
	return state_Seek;
}


/*
 * fdstart_Seek
 */
static int
fdstart_Seek(fd_softc_t *fd)
{
	int state;

	/* Calculate the side/track/sector our block is at. */
	if (TRACE_STRAT)
		printf(" Remap block %lld ", (long long) fd->startBlk);
	remap_geometry(fd->startBlk,
	    fd->currentType->heads, &fd->pos);
	if (TRACE_STRAT)
		printf("to c%d_h%d_s%d ", fd->pos.track,
		    fd->pos.side, fd->pos.sector);

	if (fd->cachedSide != fd->pos.side) {
		if (TRACE_STRAT)
			printf(" (invalidate cache) ");
		invalidateCylinderCache(fd);
		fd->cachedSide = fd->pos.side;
	}

	/*
	 * If necessary, seek to wanted track. Note that
	 * seek() performs any necessary retries.
	 */
	if (fd->pos.track != fd->pos.oldTrack &&
	    0 != (fd->iwmErr = seek(fd, IWM_SEEK_VANILLA))) {
		state = state_Fault;
	} else {
		state = (fd->ioDirection == IWM_WRITE)
		    ? state_Write : state_Read;
	}
	return state;
}


/*
 * fdstart_Read
 *
 * Transfer a sector from disk. Get it from the track cache, if available;
 * otherwise, while we are at it, store in the cache all the sectors we find 
 * on the way.
 * 
 * Track buffering reads:
 * o  Look if the sector is already cached.
 * o  Else, read sectors into track cache until we meet the header of 
 *    the sector we want.
 * o  Read that sector directly to fs buffer and return.
 */
static int
fdstart_Read(fd_softc_t *fd)
{
	int i;
	diskPosition_t *pos;
	sectorHdr_t *shdr;
	iwm_softc_t *iwm = device_lookup_private(&iwm_cd, 0); /* XXX */
	
	/* Initialize retry counters */
	fd->seekRetries = 0;
	fd->sectRetries = 0;
	pos = &fd->pos;
	shdr = &fd->sHdr;

	if (TRACE_STRAT)
		printf("<%s c%d_h%d_s%d> ", 
		    fd->ioDirection ? "Read" : "Write",
		    pos->track, pos->side, pos->sector);

	/* Sector already cached? */
	i = pos->sector;
	if (fd->r_slots[i].valid) {
		if (TRACE_STRAT)
			printf("(cached)"); 
		memcpy(fd->current_buffer, fd->r_slots[i].secbuf, 
		    fd->currentType->sectorSize);
		return state_IOFinish;
	}

	/* Get sector from disk */
	shdr->side = pos->side;
	shdr->sector = pos->sector;
	shdr->track = pos->track;

	(void)iwmSelectSide(pos->side);
	fd->iwmErr = iwmReadSector(&fd->sHdr, fd->r_slots,
	    fd->current_buffer);

	/* Check possible error conditions */
	if (TRACE_STRAT)
		printf("c%d_h%d_s%d_err(%d)_sr%d ",
		    shdr->track, shdr->side >> 3,
		    shdr->sector, fd->iwmErr, fd->sectRetries);

	/* IWM IO error? */
	if (fd->iwmErr != 0)
		return state_IOErr;

	/* Bad seek? Retry */
	if (shdr->track != pos->track) {
		if (TRACE_STRAT) {
			printf("Wanted track %d, got %d, %d seek retries.\n",
			    pos->track, shdr->track, fd->seekRetries);
		}
		if (iwm->maxRetries > fd->seekRetries++) {
			fd->iwmErr = seek(fd, IWM_SEEK_RECAL);
			if (TRACE_STRAT) {
				printf("[%d]", fd->seekRetries);
				(void)checkTrack(&fd->pos, 1);
			}
		} else
			fd->iwmErr = seekErr;
		return (0 == fd->iwmErr) ? state_Read : state_Fault;
	}

	/* Sector not found? */
	if (shdr->sector != pos->sector) {
		if (TRACE_STRAT)
			printf("c%d_h%d_s%d sect not found, %d retries ", 
			    shdr->track, shdr->side >> 3, 
			    shdr->sector, fd->sectRetries);
		fd->iwmErr = noAdrMkErr;
		return state_Fault;
	}

	/* Success */
	return state_IOFinish;
}


/*
 * fdstart_Write
 *
 * Insert a sector into a write buffer slot and mark the slot dirty.
 */
static int
fdstart_Write(fd_softc_t *fd)
{
	int i;
	
	/* XXX let's see... */
	fd->sHdr.side = fd->pos.side;
	fd->sHdr.sector = fd->pos.sector;
	fd->sHdr.track = fd->pos.track;

	i = fd->pos.sector;
	fd->w_slots[i].secbuf = fd->current_buffer;
	fd->w_slots[i].valid = 1;	/* "valid" is a dirty buffer here */
	
	if (TRACE_STRAT)
		printf("<%s c%d_h%d_s%d> (cached) ", 
		    fd->ioDirection ? "Read" : "Write",
		    fd->pos.track, fd->pos.side, fd->pos.sector);
	return state_IOFinish;
}



/*
 * fdstart_Flush
 * 
 * Flush dirty buffers in the track cache to disk.
 */
static int
fdstart_Flush(fd_softc_t *fd)
{
	int state;
	int i, dcnt;
	diskPosition_t *pos;
	sectorHdr_t *shdr;
	iwm_softc_t *iwm = device_lookup_private(&iwm_cd, 0); /* XXX */

	dcnt = 0;
	pos = &fd->pos;
	shdr = &fd->sHdr;

	if (TRACE_STRAT) {
		for (i=0; i < IWM_MAX_GCR_SECTORS; i++)
			if (fd->w_slots[i].valid) {
				printf("|%d", i);
				dcnt++;
			}
		printf("|\n");
		
		printf(" <%s c%d_h%d_#s%d>\n", 
		    fd->ioDirection ? "Read" : "Write",
		    pos->track, pos->side, dcnt);
	}	
	(void)iwmSelectSide(pos->side);
	fd->iwmErr = iwmWriteSector(&fd->sHdr, fd->w_slots);

	switch (fd->iwmErr) {
	case noErr:		/* Success */
#ifdef DIAGNOSTIC
		/* XXX Panic if buffer not clean? */
		for (i=0; i<IWM_MAX_GCR_SECTORS; i++)
			if (0 != fd->w_slots[i].valid)
				printf("Oops! <c%d_h%d_s%d> not flushed.\n",
				    fd->pos.track, fd->pos.side, 
				    fd->pos.sector);
#endif
		if (TRACE_STRAT) 
			printf("(Cache flushed, re-initialize) ");
		for (i=0; i < IWM_MAX_GCR_SECTORS; i++) {
			fd->w_slots[i].valid = 0;
			fd->w_slots[i].secbuf = NULL;
		}
		fd->seekRetries = 0;
		state = state_Exit;
		break;
		
	case seekErr:		/* Bad seek? Retry */
		if (TRACE_STRAT) {
			printf("Wanted track %d, got %d, %d seek retries.\n",
			    pos->track, shdr->track, fd->seekRetries);
		}
		if (iwm->maxRetries > fd->seekRetries++) {
			fd->iwmErr = seek(fd, IWM_SEEK_RECAL);
			if (TRACE_STRAT) {
				printf("[%d]", fd->seekRetries);
			}
		}
		state = (0 == fd->iwmErr) ? state_Exit : state_Fault;
		break;
		
	default:		/* General IWM IO error? */
		state = state_IOErr;
	}
	return state;
}


/*
 * fdstart_IOFinish 
 *
 * Prepare for next block, if any is available
 */
static int
fdstart_IOFinish(fd_softc_t *fd)
{
	int state;

	if (DISABLED && TRACE_STRAT)
		printf("%s c%d_h%d_s%d ok ", 
		    fd->ioDirection ? "Read" : "Write", 
		    fd->sHdr.track, fd->sHdr.side >> 3, fd->sHdr.sector);

	fd->bytesDone += fd->currentType->sectorSize;
	fd->bytesLeft -= fd->currentType->sectorSize;
	fd->current_buffer += fd->currentType->sectorSize;
	/*
	 * Instead of recalculating the chs mapping for
	 * each and every sector, check for
	 * 'current sector# <= max sector#' and recalculate
	 * after overflow.
	 */
	fd->startBlk++;
	if (fd->bytesLeft > 0) {
		if (++fd->pos.sector < fd->pos.maxSect) {
			if (TRACE_STRAT)
				printf("continue");
			state = (fd->ioDirection == IWM_WRITE)
			    ? state_Write : state_Read;
		}
		else {
			/* 
			 * Invalidate read cache when changing track;
			 * flush write cache to disk.
			 */
			if (fd->ioDirection == IWM_WRITE) {
				if (TRACE_STRAT)
					printf("flush ");
				state = (state_Exit == fdstart_Flush(fd))
				    ? state_Seek : state_IOErr;
			}
			else {
				if (TRACE_STRAT)
					printf("step ");
				invalidateCylinderCache(fd);
				state = state_Seek;
			}
		}
	} else {
		state = (fd->ioDirection == IWM_WRITE)
		    ? state_Flush : state_Exit;
	}
	return state;
}	


/*
 * fdstart_IOErr
 *
 * Bad IO, repeat
 */
static int
fdstart_IOErr(fd_softc_t *fd)
{
	int state;
	iwm_softc_t *iwm = device_lookup_private(&iwm_cd, 0); /* XXX */
	
#ifdef DIAGNOSTIC
	printf("iwm%sSector() err = %d, %d retries, on c%d_h%d_s%d.\n",
	    fd->ioDirection ? "Read" : "Write", 
	    fd->iwmErr, fd->ioRetries, fd->pos.track,
	    fd->pos.side, fd->pos.sector);
#endif
	/* XXX Do statistics */
	if (fd->ioRetries++ < iwm->maxRetries)
		state = (fd->ioDirection == IWM_WRITE)
		    ? state_Flush : state_Read;
	else 
		state = state_Fault;
	return state;
}


/*
 * fdstart_Fault
 *
 * A non-recoverable error
 */
static int
fdstart_Fault(fd_softc_t *fd)
{
#ifdef DIAGNOSTIC
	printf("Seek retries %d, IO retries %d, sect retries %d :\n" \
	    "\t\t only found c%d_h%d_s%d \n",
	    fd->seekRetries, fd->ioRetries, fd->sectRetries, 
	    fd->sHdr.track, fd->sHdr.side >> 3, fd->sHdr.sector);
	printf("A non-recoverable error: %d ", fd->iwmErr);
#else
	/* ARGSUSED */
#endif
	return state_Exit;
}


/*
 * fdstart_Exit
 *
 * We are done, for good or bad
 */
static int
fdstart_Exit(fd_softc_t *fd)
{
	struct buf *bp;
#ifdef DIAGNOSTIC
	int i;
#endif
	
	invalidateCylinderCache(fd);

#ifdef DIAGNOSTIC
	/* XXX Panic if buffer not clean? */
	for (i=0; i<IWM_MAX_GCR_SECTORS; i++)
		if (0 != fd->w_slots[i].valid)
			printf("Oops! <c%d_h%d_s%d> not flushed.\n",
			    fd->pos.track, fd->pos.side, fd->pos.sector);
#endif

	bp = bufq_get(fd->bufQueue);

	bp->b_resid = fd->bytesLeft;
	bp->b_error = (0 == fd->iwmErr) ? 0 : EIO;

	if (TRACE_STRAT) {
		printf(" fdstart() finished job; fd->iwmErr = %d, b_error = %d",
		    fd->iwmErr, bp->b_error);
		if (DISABLED)
			hexDump(bp->b_data, bp->b_bcount);
	}
	if (DISABLED && TRACE_STRAT)
		printf(" Next buf (bufQueue first) at %p\n",
		    bufq_peek(fd->bufQueue));
	disk_unbusy(&fd->diskInfo, bp->b_bcount - bp->b_resid,
	    (bp->b_flags & B_READ));
	biodone(bp);
	/* 
	 * Stop motor after 10s
	 * 
	 * XXX Unloading the module while the timeout is still
	 *     running WILL crash the machine. 
	 */
	callout_reset(&fd->motor_ch, 10 * hz, motor_off, fd);

	return state_Done;
}


/*
 * remap_geometry
 *
 * Remap the rigid UN*X view of a disk's cylinder/sector geometry
 * to our zone recorded real Sony drive by splitting the disk
 * into zones.
 *
 * Loop {
 * 	Look if logical block number is in current zone
 *	NO:	Add # of tracks for current zone to track counter
 *		Process next zone
 *
 *	YES:	Subtract (number of first sector of current zone times heads)
 *		from logical block number, then break up the difference
 *		in tracks/side/sectors (spt is constant within a zone).
 *		Done
 * }
 */
static void
remap_geometry(daddr_t block, int heads, diskPosition_t *loc)
{
	int zone, spt;
	extern diskZone_t diskZones[];

	spt = 0;		/* XXX Shut up egcs warning */
	loc->oldTrack = loc->track;
	loc->track = 0;

	for (zone = 0; zone < IWM_GCR_DISK_ZONES; zone++) {
		if (block >= heads * (diskZones[zone].lastBlock + 1)) {
			/* Process full zones */
			loc->track += diskZones[zone].tracks;
		} else {
			/* Process partial zone */
			spt = diskZones[zone].sectPerTrack;
			block -= heads * diskZones[zone].firstBlock;
			loc->track += block / (spt * heads);
			loc->sector = (block % spt);
			loc->side = (block % (spt * heads)) / spt;
			break;
		}
	}
	loc->maxSect = spt;
}


/*
 * motor_off
 *
 * Callback for timeout()
 */
static void
motor_off(void *param)
{
	int spl;
	fd_softc_t *fd;

	fd = param;
	if (TRACE_STRAT)
		printf("iwm: Switching motor OFF (timeout).\n");
	spl = spl6();
	(void)iwmMotor(fd->unit, 0);
	fd->state &= ~IWM_FD_MOTOR_ON;
	splx(spl);
}


/*
 * fdGetDiskLabel
 *
 * Set up disk label with parameters from current disk type.
 * Then call the generic disklabel read routine which tries to
 * read a label from disk and insert it. If it doesn't exist use
 * our defaults.
 */
static void
fdGetDiskLabel(fd_softc_t *fd, dev_t dev)
{
	const char *msg;
	int fdType;
	struct disklabel *lp;
	struct cpu_disklabel *clp;

	if (TRACE_IOCTL)
		printf("iwm: fdGetDiskLabel() for disk %" PRIu64 ".\n",
		    (dev_t) (minor(dev) / MAXPARTITIONS));
	fdType = minor(dev) % MAXPARTITIONS;
	lp = fd->diskInfo.dk_label;
	clp = fd->diskInfo.dk_cpulabel;
	memset(lp, 0, sizeof(struct disklabel));
	memset(clp, 0, sizeof(struct cpu_disklabel));
	/*
	 * How to describe a drive with a variable # of sectors per
	 * track (8..12) and variable rpm (300..550)? Apple came up
	 * with ZBR in 1983! Un*x drive management sucks.
	 */
	lp->d_type = DTYPE_FLOPPY;
	lp->d_rpm = 300;
	lp->d_secsize = fd->currentType->sectorSize;
	lp->d_ntracks = fd->currentType->heads;
	lp->d_ncylinders = fd->currentType->tracks;
	lp->d_nsectors = fd->currentType->secPerTrack;
	lp->d_secpercyl = fd->currentType->secPerCyl;
	lp->d_secperunit = fd->currentType->secPerDisk;
	lp->d_interleave = fd->currentType->interleave;
	lp->d_trkseek = fd->currentType->stepRate;

	strcpy(lp->d_typename, dktypenames[DTYPE_FLOPPY]);
	strncpy(lp->d_packname, "fictitious", sizeof(lp->d_packname));

	lp->d_npartitions = fdType + 1;
	lp->d_partitions[fdType].p_offset = 0;
	lp->d_partitions[fdType].p_size = lp->d_secperunit;
	lp->d_partitions[fdType].p_fstype = FS_BSDFFS;
	lp->d_partitions[fdType].p_fsize = 512;
	lp->d_partitions[fdType].p_frag = 8;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);
	/*
	 * Call the generic disklabel extraction routine.  If we don't
	 * find a label on disk, keep our faked one.
	 */
	if (TRACE_OPEN)
		printf(" now calling readdisklabel()...\n");

	msg = readdisklabel(dev, fdstrategy, lp, clp);
	if (msg == NULL) {
		strncpy(lp->d_packname, "default label",
		    sizeof(lp->d_packname));	/* XXX - ?? */
	}
#ifdef IWM_DEBUG
	else
		printf("iwm: %s.\n", msg);
#endif
	if (TRACE_OPEN)
		fdPrintDiskLabel(lp);
}



/*
 * initCylinderCache
 *
 * Allocate cylinder cache and set up pointers to sectors.
 */
static int
initCylinderCache(fd_softc_t *fd)
{
	int i;
	int err;
	int secsize;
	
	err = 0;
	secsize = fd->currentType->sectorSize;
	fd->cachedSide = 0;
	
	fd->cbuf = (unsigned char *) malloc(IWM_MAX_GCR_SECTORS
	    * secsize, M_DEVBUF, M_WAITOK);
	if (NULL == fd->cbuf) 
		err = ENOMEM;
	else
		for (i=0; i < IWM_MAX_GCR_SECTORS; i++) {
			fd->w_slots[i].valid = 0;
			fd->w_slots[i].secbuf = NULL;
			
			fd->r_slots[i].valid = 0;
			fd->r_slots[i].secbuf = fd->cbuf + i * secsize;
		}
	return err;
}


/*
 * invalidateCylinderCache
 *
 * Switching cylinders (tracks?) invalidates the read cache.
 */
static void
invalidateCylinderCache(fd_softc_t *fd)
{
	int i;
	
	fd->cachedSide = 0;
	for (i=0; i < IWM_MAX_GCR_SECTORS; i++) {
		fd->r_slots[i].valid = 0;
	}
}


/*
 * getFDType
 *
 * return pointer to disk format description
 */
static fdInfo_t *
getFDType(short unit)
{
	int driveFlags;
	fdInfo_t *thisType;
	extern fdInfo_t fdTypes[];

	driveFlags = iwmCheckDrive(unit);
/*
 * Drive flags are: Bit  0 - 1 = Drive is double sided
 *			 1 - 1 = No disk inserted
 *			 2 - 1 = Motor is off
 *			 3 - 1 = Disk is writable
 *			 4 - 1 = Disk is DD (800/720K)
 *			31 - 1 = No drive / invalid drive #
 */
	if (TRACE_CONFIG) {
		printf("iwm: Drive %d says 0x0%x (%d)\n",
		    unit, driveFlags, driveFlags);
	}
	if (driveFlags < 0)
		thisType = NULL;/* no such drive	 */
	else
		if (driveFlags & 0x01)
			thisType = &fdTypes[1];	/* double sided		 */
		else
			thisType = &fdTypes[0];	/* single sided		 */

	return thisType;
}


/*
 * fdDeviceToType
 *
 * maps the minor device number (elsewhere: partition type) to
 * a corresponding disk format.
 * This is currently:
 * 	fdXa	default (800K GCR)
 *	fdXb	400K GCR
 *	fdXc	800K GCR
 */
static fdInfo_t *
fdDeviceToType(fd_softc_t *fd, dev_t dev)
{
	int type;
	fdInfo_t *thisInfo;
	/* XXX This broke with egcs 1.0.2 */
	/* extern fdInfo_t	fdTypes[]; */

	type = minor(dev) % MAXPARTITIONS;	/* 1,2,... */
	if (type > sizeof(fdTypes) / sizeof(fdTypes[0]))
		thisInfo = NULL;
	else
		thisInfo = (type == 0) ? fd->defaultType : &fdTypes[type - 1];
	return thisInfo;
}


/*
 * seek
 *
 * Step to given track; optionally restore to track zero before
 * and/or verify correct track.
 * Note that any necessary retries are done here.
 * We keep the current position on disk in a 'struct diskPosition'.
 */
static int
seek(fd_softc_t *fd, int style)
{
	int state, done;
	int err, ierr;
	int steps;

	diskPosition_t *loc;
	sectorHdr_t hdr;
	char action[32];
	iwm_softc_t *iwm = device_lookup_private(&iwm_cd, 0); /* XXX */

	const char *stateDesc[] = {
		"Init",
		"Seek",
		"Recalibrate",
		"Verify",
		"Exit"
	};
	enum {
		seek_state_Init = 0,
		seek_state_Seek,
		seek_state_Recalibrate,
		seek_state_Verify,
		seek_state_Exit
	};
	/* XXX egcs */
	done = err = ierr = 0;
	fd->seekRetries = 0;
	fd->verifyRetries = 0;	

	loc = &fd->pos;

	state = seek_state_Init;
	do {
		if (TRACE_STEP)
			printf(" seek state %d [%s].\n",
			    state, stateDesc[state]);
		switch (state) {

		case seek_state_Init:
			if (TRACE_STEP)
				printf("Current track is %d, new track %d.\n",
				    loc->oldTrack, loc->track);
			memset(&hdr, 0, sizeof(hdr));
			err = ierr = 0;
			fd->seekRetries = 0;
			fd->verifyRetries = 0;
			state = (style == IWM_SEEK_RECAL)
			    ? seek_state_Recalibrate : seek_state_Seek;
			done = 0;
			break;

		case seek_state_Recalibrate:
			ierr = iwmTrack00();
			if (ierr == 0) {
				loc->oldTrack = 0;
				state = seek_state_Seek;
			} else {
				strncpy(action, "Recalibrate (track 0)",
				    sizeof(action));
				state = seek_state_Exit;
			}
			break;

		case seek_state_Seek:
			ierr = 0;
			steps = loc->track - loc->oldTrack;

			if (steps != 0)
				ierr = iwmSeek(steps);
			if (ierr == 0) {
				/* No error or nothing to do */
				state = (style == IWM_SEEK_VERIFY)
				    ? seek_state_Verify : seek_state_Exit;
			} else {
				if (fd->seekRetries++ < iwm->maxRetries)
					state = seek_state_Recalibrate;
				else {
					strncpy(action, "Seek retries",
					    sizeof(action));
					state = seek_state_Exit;
				}
			}
			break;

		case seek_state_Verify:
			ierr = checkTrack(loc, TRACE_STEP);
			if (ierr == 0 && loc->track == hdr.track)
				state = seek_state_Exit;
			else {
				if (fd->verifyRetries++ < iwm->maxRetries)
					state = seek_state_Recalibrate;
				else {
					strncpy(action, "Verify retries",
					    sizeof(action));
					state = seek_state_Exit;
				}
			}
			break;

		case seek_state_Exit:
			if (ierr == 0) {
				loc->oldTrack = loc->track;
				err = 0;
				/* Give the head some time to settle down */
				delay(3000);
			} else {
#ifdef DIAGNOSTIC
				printf(" seek() action \"%s\", err = %d.\n",
				    action, ierr);
#endif
				err = EIO;
			}
			done = 1;
			break;
		}
	} while (!done);
	return err;
}


/*
 * checkTrack
 *
 * After positioning, get a sector header for validation
 */
static int
checkTrack(diskPosition_t *loc, int debugFlag)
{
	int spl;
	int iwmErr;
	sectorHdr_t hdr;

	spl = spl6();
	iwmSelectSide(loc->side);
	iwmErr = iwmReadSectHdr(&hdr);
	splx(spl);
	if (debugFlag) {
		printf("Seeked for %d, got at %d, Hdr read err %d.\n",
		    loc->track, hdr.track, iwmErr);
	}
	return iwmErr;
}


/* Debugging stuff */

static void
hexDump(u_char *buf, int len)
{
	int i, j;
	u_char ch;

	printf("\nDump %d from %p:\n", len, buf);
	i = j = 0;
	if (NULL != buf) do {
		printf("%04x: ", i);
		for (j = 0; j < 8; j++)
			printf("%02x ", buf[i + j]);
		printf(" ");
		for (j = 8; j < 16; j++)
			printf("%02x ", buf[i + j]);
		printf(" ");
		for (j = 0; j < 16; j++) {
			ch = buf[i + j];
			if (ch > 31 && ch < 127)
				printf("%c", ch);
			else
				printf(".");
		}
		printf("\n");
		i += 16;
	} while (len > i);
}


static void
fdPrintDiskLabel(struct disklabel *lp)
{
	int i;

	printf("iwm: Disklabel entries of current floppy.\n");
	printf("\t d_type:\t%d (%s)\n", lp->d_type,
	    dktypenames[lp->d_type]);
	printf("\t d_typename:\t%s\n", lp->d_typename);
	printf("\t d_packname:\t%s\n", lp->d_packname);

	printf("\t d_secsize:\t%d\n", lp->d_secsize);
	printf("\t d_nsectors:\t%d\n", lp->d_nsectors);
	printf("\t d_ntracks:\t%d\n", lp->d_ntracks);
	printf("\t d_ncylinders:\t%d\n", lp->d_ncylinders);
	printf("\t d_secpercyl:\t%d\n", lp->d_secpercyl);
	printf("\t d_secperunit:\t%d\n", lp->d_secperunit);

	printf("\t d_rpm: \t%d\n", lp->d_rpm);
	printf("\t d_interleave:\t%d\n", lp->d_interleave);
	printf("\t d_trkseek:\t%d [ms]\n", lp->d_trkseek);

	printf(" d_npartitions:\t%d\n", lp->d_npartitions);
	for (i = 0; i < lp->d_npartitions; i++) {
		printf("\t d_partitions[%d].p_offset:\t%d\n", i,
		    lp->d_partitions[i].p_offset);
		printf("\t d_partitions[%d].p_size:\t%d\n", i,
		    lp->d_partitions[i].p_size);
		printf("\t d_partitions[%d].p_fstype:\t%d (%s)\n", i,
		    lp->d_partitions[i].p_fstype,
		    fstypenames[lp->d_partitions[i].p_fstype]);
		printf("\t d_partitions[%d].p_frag:\t%d\n", i,
		    lp->d_partitions[i].p_frag);
		printf("\n");
	}
}
