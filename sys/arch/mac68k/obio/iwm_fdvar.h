/*	$NetBSD: iwm_fdvar.h,v 1.4 2000/01/21 23:29:06 thorpej Exp $	*/

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
 * 3. The name of the author may not be used to endorse or promote products
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
#ifndef _MAC68K_FDVAR_H
#define _MAC68K_FDVAR_H

/**
 **	Constants
 **/

enum {
	IWM_MAX_DRIVE = 2,		/* Attachable drives */
	IWM_GCR_DISK_ZONES = 5,		/* Zones on GCR disk */
	IWM_MAX_GCR_SECTORS = 12,	/* Max. sectors per GCR track */
	IWM_MAX_FLOPPY_SECT = 50,	/* Larger than the highest sector */
					/* number likely to occur */
};


/* Physical track format codes */
enum {
	IWM_GCR,		/* Apple's Group Code Recording format */
	IWM_MFM_DD,		/* Standard MFM on DD disk (250 KBit/s)	*/
	IWM_MFM_HD		/* Standard MFM on HD disk (500 KBit/s)	*/
};

/* Drive softc flags */
enum {
	IWM_FD_IS_OPEN	= 0x00000001,
	IWM_FD_MOTOR_ON = 0x00000002
};

/* seek() behaviour */
enum {
	IWM_SEEK_VANILLA,
	IWM_SEEK_RECAL,
	IWM_SEEK_VERIFY
};

/* I/O direction */
enum {
	IWM_WRITE = 0,
	IWM_READ
};


/**
 **	Data Types
 **/

/*
 * Floppy disk format information
 *
 * XXX How to describe ZBR here? UN*X disk drive handling -- clinging
 *     tenaciously to the trailing edge of technology...
 */
struct fdInfo {
	short	heads;			/* # of heads the drive has */
	short	tracks;			/* # of tracks per side (cyl's)	*/
	short	sectorSize;		/* Bytes per sector */
	short	secPerTrack;		/* fake	*/
	short	secPerCyl;		/* fake	*/
	short	secPerDisk;		/* # of sectors per __disk__ */
	short	stepRate;		/* in ms (is a software delay) */
	short	interleave;		/* Sector interleave */
	short	physFormat;		/* GCR, MFM DD, MFM HD */
	char	*description;
};
typedef struct fdInfo fdInfo_t;

/*
 * Current physical location on Sony GCR disk
 */
struct diskPosition {
	short	track;
	short	oldTrack;
	short	side;
	short	sector;
	short	maxSect;		/* Highest sector # for this track */
};
typedef struct diskPosition diskPosition_t;

/*
 * Zone recording scheme (per disk surface/head)
 */
struct diskZone {
	short	tracks;			/* # of tracks per zone	*/
	short	sectPerTrack;
	short	firstBlock;
	short	lastBlock;
};
typedef struct diskZone diskZone_t;

/*
 * Arguments passed between iwmAttach() and the fd probe routines.
 */
struct iwmAttachArgs {
	fdInfo_t *driveType;		/* Default drive parameters */
	short	unit;			/* Current drive # */
};
typedef struct iwmAttachArgs iwmAttachArgs_t;

/*
 * Software state per disk: the IWM can have max. 2 drives. Newer
 * machines don't even have a port for an external drive.
 *
 */
struct fd_softc {
	struct device devInfo;		/* generic device info */
	struct disk diskInfo;		/* generic disk info */
	struct buf_queue bufQueue;	/* queue of buf's */
	int sc_active;			/* number of active requests */

/* private stuff here */
/* errors & retries in current I/O job */
	int	iwmErr;			/* Last IO error */
	int	ioRetries;
	int	seekRetries;
	int	sectRetries;
	int	verifyRetries;
	
/* hardware info */
	int	drvFlags;		/* Copy of drive flags */
	short   stepDirection;		/* Current step direction */
	diskPosition_t pos;		/* Physical position on disk */
	
	
/* drive info */
	short	unit;			/* Drive # as seen by IWM */
	short	partition;		/* "Partition" info {a,b,c,...} */
	fdInfo_t *defaultType;		/* default floppy format */
	fdInfo_t *currentType;		/* current floppy format */
	int     state;			/* XXX */

/* data transfer info */
	int	ioDirection;		/* Read/write */
	daddr_t	startBlk;		/* Starting block # */
	int	bytesLeft;		/* Bytes left to transfer */
	int	bytesDone;		/* Bytes transferred */
	caddr_t current_buffer; 	/* target of current data transfer */
	unsigned char *cbuf;		/* ptr to cylinder cache */
	int	cachedSide;		/* Which head is cached? */
	cylCacheSlot_t r_slots[IWM_MAX_GCR_SECTORS];
	cylCacheSlot_t w_slots[IWM_MAX_GCR_SECTORS];
	int	writeLabel;		/* Write access to disklabel? */
	sectorHdr_t sHdr;		/* current sector header */
};
typedef struct fd_softc fd_softc_t;

/*
 * Software state of IWM controller
 *
 * SWIM/MFM mode may have some state to keep here.
 */
struct iwm_softc {
	struct device devInfo;		/* generic device info */
	int	drives;			/* # of attached fd's */
	fd_softc_t *fd[IWM_MAX_DRIVE];	/* ptrs to children */

	int	state;			/* make that an enum? */
	u_char	modeReg;		/* Copy of IWM mode register */
	short	maxRetries;		/* I/O retries */
	int	errors;
	int	underruns;		/* data not delivered in time */
};
typedef struct iwm_softc iwm_softc_t;


/**
 **     Exported functions
 **/

/* 
 * IWM Loadable Kernel Module : Exported functions 
 */
#ifdef _LKM
int	fdModInit __P((void));
void	fdModFree __P((void));
#endif

/*
 * This is the exported driver interface
 * (bdevsw[] & cdevsw[] function prototypes)
 *
 * (see <sys/conf.h>
 */
dev_type_open(fdopen);
dev_type_close(fdclose);
dev_type_strategy(fdstrategy);
dev_type_read(fdread);
dev_type_write(fdwrite);
dev_type_ioctl(fdioctl);
dev_type_size(fdsize);
dev_type_dump(fddump);


int 	iwmInit __P((void));
int 	iwmCheckDrive __P((int32_t drive));
int	iwmSelectDrive __P((int32_t drive));
int	iwmSelectSide __P((int32_t side));
int	iwmTrack00 __P((void));
int	iwmSeek __P((int32_t steps));

int     iwmReadSector __P((sectorHdr_t *hdr, cylCacheSlot_t *r_slots, 
			   caddr_t buf));
int	iwmWriteSector __P((sectorHdr_t *hdr, cylCacheSlot_t *w_slots));

int	iwmDiskEject __P((int32_t drive));		/* drive = [0..1] */
int	iwmMotor __P((int32_t drive, int32_t onOff));	/* on(1)/off(0)	*/

/*
 * Debugging only
 */
int	iwmQueryDrvFlag __P((int32_t drive, int32_t reg)); /* reg = [0..15] */

/* Make sure we run at splhigh when calling! */
int	iwmReadSectHdr __P((sectorHdr_t *hdr));

#if 0 /* XXX not yet */
int	iwmReadRawSector __P((int32_t ID, caddr_t buf));
int	iwmWriteRawSector __P((int32_t ID, caddr_t buf));
int	iwmReadRawTrack __P((int32_t mode, caddr_t buf));
#endif

#endif /* _MAC68K_FDVAR_H */
