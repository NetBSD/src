/* $Id: iwmreg.h,v 1.1 1999/02/18 07:38:26 scottr Exp $ */

/*
 * Copyright (c) 1996-98 Hauke Fath.  All rights reserved.
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
#ifndef _MAC68K_IWMREG_H
#define _MAC68K_IWMREG_H

/*
 * iwmreg.h -- Interface declarations for iwm.
 */

/*
 * Configuration
 */
int 	iwmInit(void);

/* Virtual Drive flags register */
enum dFlags {
	IWM_DS_DISK	= 0x01,
	IWM_NO_DISK 	= 0x02,
	IWM_MOTOR_OFF 	= 0x04,
	IWM_WRITEABLE	= 0x08,
	IWM_DD_DISK 	= 0x10,
	IWM_NO_DRIVE 	= 0x80000000
};
int 	iwmCheckDrive(int drive);

/*
 * Access
 */
enum dErrors {
	noDriveErr = -64,	/* Drive not installed			  */
	offLinErr = -65,	/* R/W requested for an offline drive	  */
	noNybErr = -66,		/* Disk is probably blank		  */
	noAdrMkErr = -67,	/* Can't find an address mark		  */
	dataVerErr = -68,	/* Read verify compare failed		  */
	badCkSmErr = -69,	/* Bad address mark checksum		  */
	badBtSlpErr = -70,	/* Bad address mark (no lead-out)	  */
	noDtaMkErr = -71,	/* Could not find a data mark		  */
	badDCkSum = -72,	/* Bad data mark checksum		  */
	badDBtSlp = -73,	/* One of the data mark bit slip 	  */
				/* nibbles was incorrect.		  */
	wrUnderRun = -74,	/* Could not write fast enough to  	  */
				/* keep up with the IWM			  */
	cantStepErr = -75,	/*  Step handshake failed during seek	  */
	tk0BadErr = -76,	/* Track 00 sensor does not change 	  */
				/* during head calibration		  */
	initIWMErr = -77,	/* Unable to initialize IWM		  */
	twoSideErr = -78,	/* Tried to access a double-sided disk    */
				/* on a single-sided drive (400K drive)	  */
	spAdjErr = -79,		/* Can't adjust drive speed (400K drive)  */
	seekErr = -80,		/* Wrong track number read in a sector's  */
				/* address field		 	  */
	sectNFErr = -81,	/* Sector number never found on a track	  */
	fmt1Err = -82,		/* Can't find sector 0 after track format */
	fmt2Err = -83,		/* Can't get enough sync		  */
	verErr = -84		/* Track failed to verify		  */
};
typedef enum dErrors dErrors_t;

/*
 * This is byte-aligned (just in case anyone's interested...)
 */
struct sectorHdr {
	u_int8_t side;
	u_int8_t track;
	u_int8_t sector;
	u_int8_t Tags[13];	/* XXX Looks like it, although IM	*/
				/*     specifies 12 tag bytes	 	*/
};
typedef struct sectorHdr sectorHdr_t;

int	iwmSelectDrive __P((int drive));
int	iwmSelectSide __P((int side));
int	iwmTrack00 __P((void));
int	iwmSeek __P((int steps));

int	iwmReadSector __P((caddr_t buf, sectorHdr_t *hdr));
int	iwmWriteSector __P((caddr_t buf, sectorHdr_t *hdr));

int	iwmDiskEject __P((int drive));		/* drive = [0..1]	 */
int	iwmMotor __P((int drive, int onOff));	/* on(1)/off(0)		 */

/*
 * Debugging only
 */
int	iwmQueryDrvFlag __P((int drive, int reg)); /* reg = [0..15]	 */

/* Make sure we run at splhigh when calling! */
int	iwmReadSectHdr __P((sectorHdr_t *hdr));

int	iwmReadRawSector __P((int ID, caddr_t buf));
int	iwmWriteRawSector __P((int ID, caddr_t buf));
int	iwmReadRawTrack __P((int mode, caddr_t buf));

#endif /* _MAC68K_IWMREG_H */
