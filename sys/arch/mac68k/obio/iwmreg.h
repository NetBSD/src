/*	$NetBSD: iwmreg.h,v 1.7.26.1 2007/03/12 05:49:02 rmind Exp $	*/

/*
 * Copyright (c) 1996-99 Hauke Fath.  All rights reserved.
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

#ifndef _LOCORE

/*
 * Configuration
 */

/* Virtual Drive flags register */
enum {
	IWM_DS_DISK	= 0x01,
	IWM_NO_DISK 	= 0x02,
	IWM_MOTOR_OFF 	= 0x04,
	IWM_WRITABLE	= 0x08,
	IWM_DD_DISK 	= 0x10,
	IWM_NO_DRIVE 	= 0x80000000
};

/*
 * Access
 */
enum {
	noErr = 0,		/* All went well			  */
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

#define IWM_TAG_SIZE 20		/* That's what "SonyEqu.a" says		 */


/* Buffer for sector header data */
struct sectorHdr {
	u_int8_t side;
	u_int8_t track;
	u_int8_t sector;
	u_int8_t Tags[IWM_TAG_SIZE];
};
typedef struct sectorHdr sectorHdr_t;

/*
 * Cylinder cache data structure
 * Cyl buffer is allocated in fdopen() and deallocated in fdclose().
 *
 * The "valid" flag is overloaded as "dirty" flag when writing
 * to disk.
 */
struct cylCacheSlot {
	unsigned char *secbuf;		/* ptr to one sector buffer */
	int32_t valid;			/* content valid for cur. cylinder? */
};
typedef struct cylCacheSlot cylCacheSlot_t;


#else /* _LOCORE */


/*
 * Assembler equates for IWM, kept here to ensure consistency.
 * Modelled after <sys/disklabel.h>
 */

/*
 * Offsets into data structures
 */
	/* sectorHdr_t */
	.equ	o_side,		0
	.equ	o_track,	1
	.equ	o_sector,	2
	.equ	o_Tags,		3

	/* cylCacheSlot_t */
	.equ	o_secbuf,	0
	.equ	o_valid,	4

/*
 * Parameter (a6) offsets from <mac68k/obio/iwm_fdvar.h>
 *
 * int iwmReadSector(sectorHdr_t *hdr, cylCacheSlot_t *r_slots, void *buf)
 * int iwmWriteSector(sectorHdr_t *hdr, cylCacheSlot_t *w_slots)
 */
	.equ	o_hdr,		 8
	.equ	o_rslots,	12
	.equ	o_wslots,	12
	.equ	o_buf,		16

/*
 * Offsets from IWM base address
 * Lines are set by any memory access to corresponding address (IM III-34/-44).
 * The SWIM has actual registers at these addresses, so writing to them
 * in IWM mode is a no-no. 
 */
	.equ	ph0L,		0x0000	/* CA0 off (0) */
	.equ	ph0H,		0x0200	/* CA0 on  (1) */
	.equ	ph1L,		0x0400	/* CA1 off (0) */
	.equ	ph1H,		0x0600	/* CA1 on  (1) */
	.equ	ph2L,		0x0800	/* CA2 off (0) */
	.equ	ph2H,		0x0A00	/* CA2 on  (1) */
	.equ	ph3L,		0x0C00	/* LSTRB off (low) */
	.equ	ph3H,		0x0E00	/* LSTRB on (high) */
	.equ	mtrOff,		0x1000	/* disk enable off */
	.equ	mtrOn,		0x1200	/* disk enable on */
	.equ	intDrive,	0x1400	/* select internal drive */
	.equ	extDrive,	0x1600	/* select external drive */
	.equ	q6L,		0x1800	/* Q6 off */
	.equ	q6H,		0x1A00	/* Q6 on */
	.equ	q7L,		0x1C00	/* Q7 off */
	.equ	q7H,		0x1E00	/* Q7 on */


/* 
 * VIA Disk SEL line
 */
	.equ	vBufA,		0x1E00	/* Offset from vBase to register A  */
					/* (IM III-43) */
	.equ	vHeadSel,	5	/* Multi-purpose line (SEL) */
	.equ	vHeadSelMask,	0x0020	/* Corresponding bit mask */


/*
 * Disk registers	
 *	bit 0 - CA2, bit 1 - SEL, bit 2 - CA0, bit 3 - CA1	   IM III name
 */	

	/* Status */
	.equ	stepDirection,	0x0000	/* Direction of next head step       */
					/* 0 = in, 1 = out           (DIRTN) */
	.equ	rdDataFrom0,	0x0001	/* Set up drive to read data from    */
					/* head 0                  (RDDATA0) */
	.equ	diskInserted,	0x0002	/* Disk inserted                     */
					/* 0 = yes, 1 = no           (CSTIN) */
	.equ	rdDataFrom1,	0x0003	/* Set up drive to read data from    */
					/* head 1                  (RDDATA1) */
	.equ	stillStepping,	0x0004	/* Drive is still stepping           */
					/* 0 = yes, 1 = no            (STEP) */
	.equ	writeProtected,	0x0006	/* Disk is locked                    */
					/* 0 = yes, 1 = no          (WRTPRT) */
	.equ	drvMotorState,	0x0008	/* Drive motor is on                 */
					/* 0 = yes, 1 = no         (MOTORON) */
	.equ	singleSided,	0x0009	/* Drive is single-sided             */
					/* 0 = yes, 1 = no           (SIDES) */
	.equ	atTrack00,	0x000A	/* Head is at track 00               */
					/* 0 = yes, 1 = no             (TK0) */
	.equ	headLoaded,	0x000B	/* Head loaded, drive is ready       */
					/* 0 = yes, 1 = no		 ()  */
	.equ	drvInstalled,	0x000D	/* Disk drive installed              */
					/* 0 = yes, 1 = no		 ()  */
	.equ	tachPulse,	0x000E	/* Tachometer pulse (60 /rev.)       */
					/* 0 = yes, 1 = no	     (TACH)  */
	.equ	diskIsHD,	0x000F	/* HD disk detected                  */
					/* 0 = yes, 1 = no	    (DRVIN)  */

	/* Commands */
	.equ	stepInCmd,	0x0000	/* Head step direction in    (DIRTN) */
	.equ	stepOutCmd,	0x0001	/* Head step direction out (DIRTN+1) */
	.equ	doStepCmd,	0x0004	/* Step head (STEP)                  */
	.equ	motorOnCmd,	0x0008	/* Switch drive motor on   (MOTORON) */
	.equ	motorOffCmd,	0x0009	/* Switch drive motor off (MOTOROFF) */
	.equ	ejectDiskCmd,	0x000D	/* Eject disk from drive     (EJECT) */
	

/*
 * Low level disk errors
 * For simplicity, they are given the MacOS names and numbers.
 */
	.equ	noErr,		  0	/* All went well */
	.equ	noDriveErr,	-64	/* Drive not installed */
	.equ	offLinErr,	-65	/* R/W requested for an offline drive */
	.equ	noNybErr,	-66	/* Disk is probably blank */
	.equ	noAdrMkErr,	-67	/* Can't find an address mark */
	.equ	dataVerErr,	-68	/* Read verify compare failed */
	.equ	badCkSmErr,	-69	/* Bad address mark checksum */
	.equ	badBtSlpErr,	-70	/* Bad address mark (no lead-out) */
	.equ	noDtaMkErr,	-71	/* Could not find a data mark */
	.equ	badDCkSum,	-72	/* Bad data mark checksum */
	.equ	badDBtSlp,	-73	/* One of the data mark bit slip */
					/* nibbles was incorrect. */
	.equ	wrUnderRun,	-74	/* Could not write fast enough to */
					/* keep up with the IWM */
	.equ	cantStepErr,	-75	/* Step handshake failed during seek */
	.equ	tk0BadErr,	-76	/* Track 00 sensor does not change */
					/* during head calibration */
	.equ	initIWMErr,	-77	/* Unable to initialize IWM */
	.equ	twoSideErr,	-78	/* Tried to access a double-sided disk */
					/* on a single-sided drive (400K drive) */
	.equ	spAdjErr,	-79	/* Can't adjust drive speed (400K drive) */
	.equ	seekErr,	-80	/* Wrong track number read in a  */
					/* sector's address field */
	.equ	sectNFErr,	-81	/* Sector number never found on a track */
	.equ	fmt1Err,	-82	/* Can't find sector 0 after  */
					/* track format */
	.equ	fmt2Err,	-83	/* Can't get enough sync */
	.equ	verErr,		-84	/* Track failed to verify */

/*
 * Misc constants
 */
	.equ	iwmMode,	0x17	/* SWIM switch */
	.equ	maxGCRSectors,	12	/* Max. sectors per track for GCR */
	  

#endif /* _LOCORE */

#endif /* _MAC68K_IWMREG_H */
