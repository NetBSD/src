/*	$NetBSD: iwm.s,v 1.6.18.1 2017/12/03 11:36:24 jdolecek Exp $	*/

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
 * iwm.s -- low level routines for Sony floppy disk access.
 * The present implementation supports the 800K GCR format on non-DMA 
 * machines.
 *
 * The IWM and SWIM chips run in polled mode; they are not capable of
 * interrupting the CPU. That's why interrupts need only be blocked 
 * when there is simply no time for interrupt routine processing, 
 * i.e. during data transfers.
 * 
 * o  The local routines do not block any interrupts.
 *
 * o  The iwmXXX() routines that set/get IWM or drive settings are not
 *    time critical and do not block interrupts.
 *
 * o  The iwmXXX() routines that are called to perform data transfers
 *    block all interrupts because otherwise the current sector data
 *    would be lost.
 *    The old status register content is stored on the stack.
 *
 * o  We run at spl4 to give the NMI switch a chance. All currently 
 *    supported machines have no interrupt sources > 4 (SSC) -- the
 *    Q700 interrupt levels can be shifted around in A/UX mode,
 *    but we're not there, yet.
 *
 * o  As a special case iwmReadSectHdr() must run with interrupts disabled
 *    (it transfers data). Depending on the needs of the caller, it
 *    may be necessary to block interrupts after completion of the routine
 *    so interrupt handling is left to the caller.
 *
 * If we wanted to deal with incoming serial data / serial interrupts,
 * we would have to either call zshard(0) {mac68k/dev/zs.c} or 
 * zsc_intr_hard(0) {sys/dev/ic/z8530sc.c}. Or we would have to roll our 
 * own as both of the listed function calls look rather expensive compared 
 * to a 'tst.b REGADDR ; bne NN'.
 */

#include <m68k/asm.h>

#include <mac68k/obio/iwmreg.h>

#define USE_DELAY	0	/* "1" bombs for unknown reasons */

	
/*
 * References to global name space
 */
	.extern	_C_LABEL(TimeDBRA)	| in mac68k/macrom.c
	.extern _C_LABEL(Via1Base)	| in mac68k/machdep.c
	.extern	_C_LABEL(IWMBase)	| in iwm_fd.c
	

	.data
	
diskTo:
	/*
	 * Translation table from 'disk bytes' to 6 bit 'nibbles', 
	 * taken from the .Sony driver.
	 * This could be made a loadable table (via ioctls) to read
	 * e.g. ProDOS disks (there is a hook for such a table in .Sony).
	 */
	.byte	/* 90 */  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01
	.byte	/* 98 */  0xFF, 0xFF, 0x02, 0x03, 0xFF, 0x04, 0x05, 0x06
	.byte	/* A0 */  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x08
	.byte	/* A8 */  0xFF, 0xFF, 0xFF, 0x09, 0x0A, 0x0B, 0x0C, 0x0D
	.byte	/* B0 */  0xFF, 0xFF, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13
	.byte	/* B8 */  0xFF, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A
	.byte	/* C0 */  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	.byte	/* C8 */  0xFF, 0xFF, 0xFF, 0x1B, 0xFF, 0x1C, 0x1D, 0x1E
	.byte	/* D0 */  0xFF, 0xFF, 0xFF, 0x1F, 0xFF, 0xFF, 0x20, 0x21
	.byte	/* D8 */  0xFF, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28
	.byte	/* E0 */  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x2A, 0x2B
	.byte	/* E8 */  0xFF, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32
	.byte	/* F0 */  0xFF, 0xFF, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38
	.byte	/* F8 */  0xFF, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F

hdrLeadIn:	
	.byte	0xD5, 0xAA, 0x96
	
hdrLeadOut:
	.byte	0xDE, 0xAA, 0xFF

dataLeadIn:
	.byte	0xD5, 0xAA, 0xAD

dataLeadOut:
	.byte	0xDE, 0xAA, 0xFF, 0xFF
	

toDisk:
	/*
	 * Translation table from 6-bit nibbles [0x00..0x3f] to 'disk bytes'
	 */
	.byte	/* 00 */  0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6
	.byte	/* 08 */  0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB2, 0xB3
	.byte	/* 10 */  0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC
	.byte	/* 18 */  0xBD, 0xBE, 0xBF, 0xCB, 0xCD, 0xCE, 0xCF, 0xD3
	.byte	/* 20 */  0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE
	.byte	/* 28 */  0xDF, 0xE5, 0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC
	.byte	/* 30 */  0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xf5, 0xF6
	.byte	/* 38 */  0xF7, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF

syncPattern:
	/*
	 * This sync pattern creates 4 sync chars with 10 bits each that look
	 * like 0011111111b (i.e. 0x0FF). As the IWM ignores leading zero
	 * bits, it locks on 0xFF after the third sync byte.
	 * For convenience, the bytes of the sector data lead-in
	 * (D5 AA AD) follow.
	 */
	.byte	0xFF, 0x3F, 0xCF, 0xF3, 0xFC, 0xFF
	.byte	0xD5, 0xAA, 0xAD


	
	.text

/*
 * Register conventions:	
 *	%a0	IWM base address
 *	%a1	VIA1 base address
 *
 *	%d0	return value (0 == no error)
 *
 * Upper bits in data registers that are not cleared give nasty
 * (pseudo-) random errors when building an address. Make sure those
 *  registers are cleaned with a moveq before use!
 */


	
/**
 **	Export wrappers
 **/

/*
 * iwmQueryDrvFlags -- export wrapper for driveStat
 *
 * Parameters:	stack	l	drive selector
 *		stack	l	register selector
 * Returns:	%d0		flag
 */
ENTRY(iwmQueryDrvFlag)
	link	%a6,#0
	moveml	%d1/%a0-%a1,%sp@-
	movel	_C_LABEL(IWMBase),%a0
	movel	_C_LABEL(Via1Base),%a1

	movel	%a6@(8),%d0		| Get drive #
	beq	quDrv00
	cmpl	#1,%d0
	beq	quDrv01

	bra	quDone			| Invalid drive #

quDrv00:			
	tstb	%a0@(intDrive)		| SELECT; choose drive #0
	bra	queryDrv

quDrv01:		
	tstb	%a0@(extDrive)		| SELECT; choose drive #1

queryDrv:	
	movel	%a6@(12),%d0		| Get register #
	bsr	driveStat

quDone:	
	moveml	%sp@+,%d1/%a0-%a1	
	unlk	%a6
	rts


/*
 * iwmReadSectHdr -- read and decode the next available sector header.
 *
 * Parameters:	stack	l	Address of sector header struct (I/O)
 *				b	side (0, 1)
 *				b	track (0..79)
 *				b	sector (0..11)
 * Returns:	%d0		result code
 */
ENTRY(iwmReadSectHdr)
	link	%a6,#0
	moveml	%d1-%d5/%a0-%a4,%sp@-
	movel	%a6@(0x08),%a4		| Get param block address
	bsr	readSectHdr
	moveml	%sp@+,%d1-%d5/%a0-%a4	
	unlk	%a6
	rts
	


/**
 **	Exported functions
 **/
	
/*
 * iwmInit -- Initialize IWM chip.
 *
 * Parameters:	-
 * Returns:	%d0		result code
 */
ENTRY(iwmInit)
	link	%a6,#0
	moveml	%d2/%a0,%sp@-
	movel	_C_LABEL(IWMBase),%a0

	/*
	 * Reset IWM to known state (clear disk I/O latches)
	 */
	tstb	%a0@(ph0L)		| CA0
	tstb	%a0@(ph1L)		| CA1
	tstb	%a0@(ph2L)		| CA2
	tstb	%a0@(ph3L)		| LSTRB

	tstb	%a0@(mtrOff)		| ENABLE; make sure drive is off
	tstb	%a0@(intDrive)		| SELECT; choose drive 1
	moveq	#0x1F,%d0		| XXX was 0x17 -- WHY!?
	
	/*
	 * First do it quick...
	 */
	tstb	%a0@(q6H)
	andb	%a0@(q7L),%d0		| status register
	tstb	%a0@(q6L)
	cmpib	#iwmMode,%d0		| all is well??
	beq	initDone

	/*
	 * If this doesn't succeed (e.g. drive still running),
	 * we do it thoroughly.
	 */
	movel	#0x00080000,%d2		| ca. 500,000 retries = 1.5 sec
initLp:
	moveq	#initIWMErr,%d0		| Initialization error
	subql	#1,%d2
	bmi	initErr
	tstb	%a0@(mtrOff)		| disable drive
	tstb	%a0@(q6H)
	moveq	#0x3F,%d0
	andb	%a0@(q7L),%d0
	bclr	#5,%d0			| Reset bit 5 and set Z flag
					| according to previous state
	bne	initLp			| Loop if drive still on
	cmpib	#iwmMode,%d0
	beq	initDone
	moveb	#iwmMode,%a0@(q7H)	| Init IWM
	tstb	%a0@(q7L)
	bra	initLp
	
initDone:	
	tstb	%a0@(q6L)		| Prepare IWM for data
	moveq	#0,%d0			| noErr	

initErr:
	moveml	%sp@+,%d2/%a0
	unlk	%a6
	rts


/*
 * iwmCheckDrive -- Check if given drive is available and return bit vector
 *	with capabilities (SS/DS, disk inserted, ...)
 *
 * Parameters:	stack	l	Drive number (0,1)
 * Returns:	%d0	Bit	 0 - 0 = Drive is single sided
 *				 1 - 0 = Disk inserted
 *				 2 - 0 = Motor is running
 *				 3 - 0 = Disk is write protected
 *				 4 - 0 = Disk is DD
 *				31 - (-1) No drive / invalid drive #	
 */
ENTRY(iwmCheckDrive)
	link	%a6,#0
	moveml	%d1/%a0-%a1,%sp@-
	movel	_C_LABEL(IWMBase),%a0
	movel	_C_LABEL(Via1Base),%a1

	moveq	#-1,%d1			| no drive

	movel	%a6@(0x08),%d0		| check drive #
	beq	chkDrv00
	cmpl	#1,%d0
	beq	chkDrv01

	bra	chkDone			| invalid drive #

chkDrv00:			
	tstb	%a0@(intDrive)		| SELECT; choose drive #0
	bra	chkDrive

chkDrv01:		
	tstb	%a0@(extDrive)		| SELECT; choose drive #1
		
chkDrive:
	moveq	#-2,%d1			| error code
	moveq	#drvInstalled,%d0	| Drive installed?
	bsr	driveStat
	bmi	chkDone			| no drive

	moveq	#0,%d1			| Drive found
	tstb	%a0@(mtrOn)		| ENABLE; activate drive
	moveq	#singleSided,%d0	| Drive is single-sided?
	bsr	driveStat
	bpl	chkHasDisk
	/*
	 * Drive is double-sided -- this is not really a surprise as the
	 * old ss 400k drive needs disk speed control from the Macintosh 
	 * and we're not doing that here. Anyway - just in case...
	 * I am not sure m680x0 Macintoshes (x>0) support 400K drives at all
	 * due to their radically different sound support.
	 */ 
	bset	#0,%d1			| 1 = no.
chkHasDisk:
	moveq	#diskInserted,%d0	| Disk inserted?
	bsr	driveStat
	bpl	chkMotorOn
	bset	#1,%d1			| 1 = No.
	bra	chkDone
chkMotorOn:		
	moveq	#drvMotorState,%d0	| Motor is running?
	bsr	driveStat
	bpl	chkWrtProt
	bset	#2,%d1			| 1 = No.
chkWrtProt:	
	moveq	#writeProtected,%d0	| Disk is write protected?
	bsr	driveStat
	bpl	chkDD_HD
	bset	#3,%d1			| 1 = No.
chkDD_HD:
	moveq	#diskIsHD,%d0		| Disk is HD? (was "drive installed")
	bsr	driveStat
	bpl	chkDone
	bset	#4,%d1			| 1 = No.
chkDone:
	movel	%d1,%d0
	moveml	%sp@+,%d1/%a0-%a1	
	unlk	%a6
	rts
	

/*
 * iwmDiskEject -- post EJECT command and toggle LSTRB line to give a 
 * strobe signal.
 * IM III says pulse length = 500 ms, but we seem to get away with 
 * less delay; after all, we spin lock the CPU with it.
 *
 * Parameters:	stack	l	drive number (0,1)
 *		%a0		IWMBase
 *		%a1		VIABase
 * Returns:	%d0		result code
 */
ENTRY(iwmDiskEject)
	link	%a6,#0
	movel	_C_LABEL(IWMBase),%a0
	movel	_C_LABEL(Via1Base),%a1

	movel	%a6@(0x08),%d0		| Get drive #
	beq	ejDrv00
	cmpw	#1,%d0
	beq	ejDrv01

	bra	ejDone			| Invalid drive #

ejDrv00:			
	tstb	%a0@(intDrive)		| SELECT; choose drive #0
	bra	ejDisk

ejDrv01:		
	tstb	%a0@(extDrive)		| SELECT; choose drive #1
ejDisk:		
	tstb	%a0@(mtrOn)		| ENABLE; activate drive
	
	moveq	#motorOffCmd,%d0	| Motor off
 	bsr	driveCmd

	moveq	#diskInserted,%d0	| Disk inserted?
	bsr	driveStat
	bmi	ejDone
	
	moveq	#ejectDiskCmd,%d0	| Eject it
	bsr	selDriveReg

	tstb	%a0@(ph3H)		| LSTRB high
#if USE_DELAY
	movel	#1000,%sp@-		| delay 1 ms
	jsr	_C_LABEL(delay)
	addqw	#4,%sp			| clean up stack
#else
	movew	#1,%d0
	bsr	iwmDelay
#endif
	tstb	%a0@(ph3L)		| LSTRB low
	moveq	#0,%d0			| All's well...
ejDone:
	unlk	%a6
	rts


/*
 * iwmSelectDrive -- select internal (0) / external (1) drive.
 *
 * Parameters:	stack	l	drive ID (0/1)
 * Returns:	%d0		drive #
 */
ENTRY(iwmSelectDrive)
	link	%a6,#0
	moveml	%a0-%a1,%sp@-
	movel	_C_LABEL(IWMBase),%a0
	movel	_C_LABEL(Via1Base),%a1

	movel	%a6@(8),%d0		| Get drive #
	bne	extDrv
	tstb	%a0@(intDrive)
	bra	sdDone
extDrv:
	tstb	%a0@(extDrive)
sdDone:
	moveml	%sp@+,%a0-%a1	
	unlk	%a6
	rts


/*
 * iwmMotor -- switch drive motor on/off
 *
 * Parameters:	stack	l	drive ID (0/1)
 *		stack	l	on(1)/off(0)
 * Returns:	%d0		motor cmd
 */
ENTRY(iwmMotor)
	link	%a6,#0
	moveml	%a0-%a1,%sp@-
	movel	_C_LABEL(IWMBase),%a0
	movel	_C_LABEL(Via1Base),%a1

	movel	%a6@(8),%d0		| Get drive #
	bne	mtDrv1
	tstb	%a0@(intDrive)
	bra	mtSwitch
mtDrv1:
	tstb	%a0@(extDrive)
mtSwitch:
	movel	#motorOnCmd,%d0		| Motor ON
	tstl	%a6@(12)
	bne	mtON
	movel	#motorOffCmd,%d0
mtON:	
	bsr	driveCmd	

	moveml	%sp@+,%a0-%a1	
	unlk	%a6
	rts


/*
 * iwmSelectSide -- select side 0 (lower head) / side 1 (upper head).
 *
 * This MUST be called immediately before an actual read/write access.
 *
 * Parameters:	stack	l	side bit (0/1)
 * Returns:	-
 */
ENTRY(iwmSelectSide)
	link	%a6,#0
	moveml	%d1/%a0-%a1,%sp@-
	movel	_C_LABEL(IWMBase),%a0
	movel	_C_LABEL(Via1Base),%a1

	moveq	#0x0B,%d0		| Drive ready for reading?
	bsr	selDriveReg		| (undocumented)
ss01:	
	bsr	dstatus
	bmi	ss01

	moveq	#rdDataFrom0,%d0	| Lower head
	movel	%a6@(0x08),%d1		| Get side #
	beq	ssSide0
	moveq	#rdDataFrom1,%d0	| Upper head
ssSide0:
	bsr	driveStat

	moveml	%sp@+,%d1/%a0-%a1	
	unlk	%a6
	rts


/*
 * iwmTrack00 -- move head to track 00 for drive calibration.
 *
 * XXX Drive makes funny noises during restore. Tune delay/retry count?
 *
 * Parameters:	-
 * Returns:	%d0		result code
 */
ENTRY(iwmTrack00)
	link	%a6,#0
	moveml	%d1-%d4/%a0-%a1,%sp@-
	movel	_C_LABEL(IWMBase),%a0
	movel	_C_LABEL(Via1Base),%a1

	moveq	#motorOnCmd,%d0		| Switch drive motor on
	bsr	driveCmd

	moveq	#stepOutCmd,%d0		| Step out
	bsr	driveCmd

	movew	#100,%d2		| Max. tries
t0Retry:	
	moveq	#atTrack00,%d0		| Already at track 0?
	bsr	driveStat
	bpl	isTrack00		| Track 0 => Bit 7 = 0
	
	moveq	#doStepCmd,%d0		| otherwise step
	bsr	driveCmd
	movew	#80,%d4			| Retries
t0Still:
	moveq	#stillStepping,%d0	| Drive is still stepping?
	bsr	driveStat
	dbmi	%d4,t0Still

	cmpiw	#-1,%d4
	bne	t002
	
	moveq	#cantStepErr,%d0	| Not ready after many retries
	bra	t0Done
t002:

#if USE_DELAY
	movel	#15000,%sp@-
	jsr	_C_LABEL(delay)		| in mac68k/clock.c
	addqw	#4,%sp 
#else
	movew	#15,%d0
	bsr	iwmDelay
#endif
	
	dbra	%d2,t0Retry
	
	moveq	#tk0BadErr,%d0		| Can't find track 00!!
	bra	t0Done
	
isTrack00:
	moveq	#0,%d0
t0Done:
	moveml	%sp@+,%d1-%d4/%a0-%a1
	unlk	%a6
	rts

	
/*
 * iwmSeek -- do specified # of steps (positive - in, negative - out).
 *
 * Parameters:	stack	l	# of steps
 * returns:	%d0		result code
 */
ENTRY(iwmSeek)
	link	%a6,#0
	moveml	%d1-%d4/%a0-%a1,%sp@-
	movel	_C_LABEL(IWMBase),%a0
	movel	_C_LABEL(Via1Base),%a1

	moveq	#motorOnCmd,%d0		| Switch drive motor on
	bsr	driveCmd

	moveq	#stepInCmd,%d0		| Set step IN
	movel	%a6@(8),%d2		| Get # of steps from stack
	beq	stDone			| 0 steps? Nothing to do.
	bpl	stepOut
	
	moveq	#stepOutCmd,%d0		| Set step OUT
	negl	%d2			| Make # of steps positive
stepOut:
	subql	#1,%d2			| Loop exits for -1
	bsr	driveCmd		| Set direction
stLoop:
	moveq	#doStepCmd,%d0
	bsr	driveCmd		| Step one!
	movew	#80,%d4			| Retries
st01:
	moveq	#stillStepping, %d0	| Drive is still stepping?
	bsr	driveStat
	dbmi	%d4,st01

	cmpiw	#-1,%d4
	bne	st02
	
	moveq	#cantStepErr,%d2	| Not ready after many retries
	bra	stDone
st02:

#if USE_DELAY
	movel	#30,%sp@-
	jsr	_C_LABEL(delay)		| in mac68k/clock.c
	addqw	#4,%sp 
#else
	movew	_C_LABEL(TimeDBRA),%d4	| dbra loops per ms
	lsrw	#5,%d4			| DIV 32
st03:	dbra	%d4,st03		| makes ca. 30 us
#endif

	dbra	%d2,stLoop

	moveq	#0,%d2			| All is well
stDone:
	movel	%d2,%d0
	moveml	%sp@+,%d1-%d4/%a0-%a1	
	unlk	%a6
	rts


/*
 * iwmReadSector -- read and decode the next available sector.
 *
 * TODO:	Poll SCC as long as interrupts are disabled (see top comment)
 *		Add a branch for Verify (compare to buffer)
 *		Understand and document the checksum algorithm!
 *
 *		XXX make "sizeof cylCache_t" a symbolic constant
 *
 * Parameters:	%fp+08	l	Address of sector data buffer (512 bytes)
 *		%fp+12	l	Address of sector header struct (I/O)
 *		%fp+16	l	Address of cache buffer ptr array
 * Returns:	%d0		result code
 * Local:	%fp-2	w	CPU status register
 *		%fp-3	b	side,
 *		%fp-4	b	track,
 *		%fp-5	b	sector wanted
 *		%fp-6	b	retry count
 *		%fp-7	b	sector read
 */
ENTRY(iwmReadSector)
	link	%a6,#-8
	moveml	%d1-%d7/%a0-%a5,%sp@-

 	movel	_C_LABEL(Via1Base),%a1
	movel	%a6@(o_hdr),%a4		| Addr of sector header struct

	moveb	%a4@+,%a6@(-3)		| Save side bit,
	moveb	%a4@+,%a6@(-4)		| track#,
	moveb	%a4@,%a6@(-5)		| sector#
	moveb	#2*maxGCRSectors,%a6@(-6) | Max. retry count

	movew	%sr,%a6@(-2)		| Save CPU status register
	oriw	#0x0600,%sr		| Block all interrupts

rsNextSect:	
	movel	%a6@(o_hdr),%a4		| Addr of sector header struct
	bsr	readSectHdr		| Get next available SECTOR header
	bne	rsDone			| Return if error

	/*
	 * Is this the right track & side? If not, return with error
	 */
	movel	%a6@(o_hdr),%a4		| Sector header struct

	moveb	%a4@(o_side),%d1	| Get actual side
	lsrb	#3,%d1			| "Normalize" side bit (to bit 0)
	andb	#1,%d1
	moveb	%a6@(-3),%d2		| Get wanted side
	eorb	%d1,%d2			| Compare side bits
	bne	rsSeekErr		| Should be equal!

	moveb	%a6@(-4),%d1		| Get track# we want
	cmpb	%a4@(o_track),%d1	| Compare to the header we've read
	beq	rsGetSect
	
rsSeekErr:	
	moveq	#seekErr,%d0		| Wrong track or side found
	bra	rsDone	

	/*
	 * Check for sector data lead-in 'D5 AA AD'
	 * Registers:	
	 *	%a0 points to data register of IWM as set up by readSectHdr
	 *	%a2 points to 'diskTo' translation table
	 *	%a4 points to tags buffer
	 */
rsGetSect:
	moveb	%a4@(2),%a6@(-7)	| save sector number
	lea	%a4@(3),%a4		| Beginning of tag buffer
	moveq	#50,%d3			| Max. retries for sector lookup
rsLeadIn:	
	lea	dataLeadIn,%a3		| Sector data lead-in
	moveq	#0x03,%d4		| is 3 bytes long
rsLI1:	
	moveb	%a0@,%d2		| Get next byte
	bpl	rsLI1
	dbra	%d3,rsLI2
	moveq	#noDtaMkErr,%d0		| Can't find a data mark
	bra	rsDone

rsLI2:
	cmpb	%a3@+,%d2
	bne	rsLeadIn		| If ne restart scan
	subqw	#1,%d4
	bne	rsLI1

	/*
	 * We have found the lead-in. Now get the 12 tag bytes.
	 * (We leave %a3 pointing to 'dataLeadOut' for later.)
	 */
rsTagNyb0:
	moveb	%a0@,%d3		| Get a char,
	bpl	rsTagNyb0
	moveb	%a2@(0,%d3),%a4@+	| remap and store it

	moveq	#0,%d5			| Clear checksum registers
	moveq	#0,%d6
	moveq	#0,%d7
	moveq	#10,%d4			| Loop counter
	moveq	#0,%d3			| Data scratch reg

rsTags:	
rsTagNyb1:
	moveb	%a0@,%d3		| Get 2 bit nibbles
	bpl	rsTagNyb1
	moveb	%a2@(0,%d3),%d1		| Remap disk byte
	rolb	#2,%d1
	moveb	%d1,%d2
	andib	#0xC0,%d2		| Get top 2 bits for first byte
rsTagNyb2:	
	moveb	%a0@,%d3		| Get first 6 bit nibble	
	bpl	rsTagNyb2
	orb	%a2@(0,%d3),%d2		| Remap it and complete first byte

	moveb	%d7,%d3			| The X flag bit (a copy of the carry
	addb	%d7,%d3			| flag) is added with the next addx

	rolb	#1,%d7
	eorb	%d7,%d2
	moveb	%d2,%a4@+		| Store tag byte
	addxb	%d2,%d5			| See above
	
	rolb	#2,%d1
	moveb	%d1,%d2
	andib	#0xC0,%d2		| Get top 2 bits for second byte
rsTagNyb3:
	moveb	%a0@,%d3		| Get second 6 bit nibble
	bpl	rsTagNyb3
	orb	%a2@(0,%d3),%d2		| remap it and complete byte
	eorb	%d5,%d2
	moveb	%d2,%a4@+		| Store tag byte
	addxb	%d2,%d6

	rolb	#2,%d1
	andib	#0xC0,%d1		| Get top 2 bits for third byte
rsTagNyb4:
	moveb	%a0@,%d3		| Get third 6 bit nibble
	bpl	rsTagNyb4
	orb	%a2@(0,%d3),%d1		| remap it and complete byte
	eorb	%d6,%d1
	moveb	%d1,%a4@+		| Store tag byte
	addxb	%d1,%d7
	
	subqw	#3,%d4			| Update byte counter (four 6&2 encoded
	bpl	rsTags			| disk bytes make three data bytes).

	/*
	 * Jetzt sind wir hier...
	 * ...und Thomas D. hat noch was zu sagen...
	 *
	 * We begin to read in the actual sector data. 
	 * Compare sector # to what we wanted: If it matches, read directly
	 * to buffer, else read to track cache.
	 */
	movew	#0x01FE,%d4		| Loop counter
	moveq	#0,%d1			| Clear %d1
	moveb	%a6@(-7),%d1		| Get sector# we have read
	cmpb	%a6@(-5),%d1		| Compare to the sector# we want
	bne	rsToCache
	movel	%a6@(o_buf),%a4		| Sector data buffer
	bra	rsData
rsToCache:
	movel	%a6@(o_rslots),%a4	| Base address of slot array
	lslw	#3,%d1			| sizeof cylCacheSlot_t is 8 bytes
	movel	#-1,%a4@(o_valid,%d1)
	movel	%a4@(o_secbuf,%d1),%a4	| and get its buffer ptr
rsData:
rsDatNyb1:
	moveb	%a0@,%d3		| Get 2 bit nibbles
	bpl	rsDatNyb1
	moveb	%a2@(0,%d3),%d1		| Remap disk byte
	rolb	#2,%d1
	moveb	%d1,%d2
	andib	#0xC0,%d2		| Get top 2 bits for first byte
rsDatNyb2:	
	moveb	%a0@,%d3		| Get first 6 bit nibble	
	bpl	rsDatNyb2
	orb	%a2@(0,%d3),%d2		| Remap it and complete first byte

	moveb	%d7,%d3			| The X flag bit (a copy of the carry
	addb	%d7,%d3			| flag) is added with the next addx

	rolb	#1,%d7
	eorb	%d7,%d2
	moveb	%d2,%a4@+		| Store data byte
	addxb	%d2,%d5			| See above
	
	rolb	#2,%d1
	moveb	%d1,%d2
	andib	#0xC0,%d2		| Get top 2 bits for second byte
rsDatNyb3:
	moveb	%a0@,%d3		| Get second 6 bit nibble
	bpl	rsDatNyb3
	orb	%a2@(0,%d3),%d2		| Remap it and complete byte
	eorb	%d5,%d2
	moveb	%d2,%a4@+		| Store data byte
	addxb	%d2,%d6
	tstw	%d4
	beq	rsCkSum			| Data read, continue with checksums

	rolb	#2,%d1
	andib	#0xC0,%d1		| Get top 2 bits for third byte
rsDatNyb4:
	moveb	%a0@,%d3		| Get third 6 bit nibble
	bpl	rsDatNyb4
	orb	%a2@(0,%d3),%d1		| Remap it and complete byte
	eorb	%d6,%d1
	moveb	%d1,%a4@+		| Store data byte
	addxb	%d1,%d7
	subqw	#3,%d4			| Update byte counter
	bra	rsData

	/*
	 * Next read checksum bytes
	 * While reading the sector data, three separate checksums are
	 * maintained in %D5/%D6/%D7 for the 1st/2nd/3rd data byte of
	 * each group.
	 */
rsCkSum:
rsCkS1:
	moveb	%a0@,%d3		| Get 2 bit nibbles
	bpl	rsCkS1
	moveb	%a2@(0,%d3),%d1		| Remap disk byte
	bmi	rsBadCkSum		| Fault! (Bad read)
	rolb	#2,%d1
	moveb	%d1,%d2
	andib	#0xC0,%d2		| Get top 2 bits for first byte
rsCkS2:	
	moveb	%a0@,%d3		| Get first 6 bit nibble
	bpl	rsCkS2
	moveb	%a2@(0,%d3),%d3		| and remap it
	bmi	rsBadCkSum		| Fault! ( > 0x3f is bad read)
	orb	%d3,%d2			| Merge 6&2
	cmpb	%d2,%d5			| Compare first checksum to %D5
	bne	rsBadCkSum		| Fault! (Checksum)
	
	rolb	#2,%d1
	moveb	%d1,%d2
	andib	#0xC0,%d2		| Get top 2 bits for second byte
rsCkS3:	
	moveb	%a0@,%d3		| Get second 6 bit nibble
	bpl	rsCkS3
	moveb	%a2@(0,%d3),%d3		| and remap it
	bmi	rsBadCkSum		| Fault! (Bad read)
	orb	%d3,%d2			| Merge 6&2
	cmpb	%d2,%d6			| Compare second checksum to %D6
	bne	rsBadCkSum		| Fault! (Checksum)

	rolb	#2,%d1
	andib	#0xC0,%d1		| Get top 2 bits for second byte
rsCkS4:	
	moveb	%a0@,%d3		| Get third 6 bit nibble
	bpl	rsCkS4
	moveb	%a2@(0,%d3),%d3		| and remap it
	bmi	rsBadCkSum		| Fault! (Bad read)
	orb	%d3,%d1			| Merge 6&2
	cmpb	%d1,%d7			| Compare third checksum to %D7
	beq	rsLdOut			| Fault! (Checksum)
	
rsBadCkSum:
	moveq	#badDCkSum,%d0		| Bad data mark checksum
	bra	rsDone

rsBadDBtSlp:
	moveq	#badDBtSlp,%d0		| One of the data mark bit slip 
	bra	rsDone			| nibbles was incorrect

	/*
	 * We have gotten the checksums allright, now look for the
	 * sector data lead-out 'DE AA'
	 * (We have %a3 still pointing to 'dataLeadOut'; this part of the
	 * table is used for writing to disk, too.)
	 */
rsLdOut:
	moveq	#1,%d4			| Is two bytes long {1,0}
rsLdOut1:	
	moveb	%a0@,%d3		| Get token
	bpl	rsLdOut1
	cmpb	%a3@+,%d3
	bne	rsBadDBtSlp		| Fault!
	dbra	%d4,rsLdOut1
	moveq	#0,%d0			| OK.

	/*
	 * See if we got the sector we wanted. If not, and no error 
	 * occurred, mark buffer valid. Else ignore the sector. 
	 * Then, read on.
	 */
rsDone:
	movel	%a6@(o_hdr),%a4		| Addr of sector header struct
	moveb	%a4@(o_sector),%d1	| Get # of sector we have just read
	cmpb	%a6@(-5),%d1		| Compare to the sector we want
	beq	rsAllDone

	tstb	%d0			| Any error? Simply ignore data
	beq	rsBufValid
	lslw	#3,%d1			| sizeof cylCacheSlot_t is 8 bytes
	movel	%a6@(o_rslots),%a4
	clrl	%a4@(o_valid,%d1)	| Mark buffer content "invalid"
	
rsBufValid:
	subqb	#1,%a6@(-6)		| max. retries
	bne	rsNextSect
					| Sector not found, but
	tstb	%d0			| don't set error code if we 
	bne	rsAllDone		| already have one.
	moveq	#sectNFErr,%d0
rsAllDone:
	movew	%a6@(-2),%sr		| Restore interrupt mask
	moveml	%sp@+,%d1-%d7/%a0-%a5	
	unlk	%a6
	rts	
	
	
/*
 * iwmWriteSector -- encode and write data to the specified sector.
 *
 * TODO:	Poll SCC as long as interrupts are disabled (see top comment)
 *		Understand and document the checksum algorithm!
 *
 *		XXX Use registers more efficiently
 *
 * Parameters:	%fp+8	l	Address of sector header struct (I/O)
 *		%fp+12	l	Address of cache buffer ptr array
 * Returns:	%d0		result code
 *
 * Local:	%fp-2	w	CPU status register
 *		%fp-3	b	side,
 *		%fp-4	b	track,
 *		%fp-5	b	sector wanted
 *		%fp-6	b	retry count
 *		%fp-10	b	current slot
 */
ENTRY(iwmWriteSector)
	link	%a6,#-10
	moveml	%d1-%d7/%a0-%a5,%sp@-

 	movel	_C_LABEL(Via1Base),%a1
	movel	%a6@(o_hdr),%a4		| Addr of sector header struct

	moveb	%a4@+,%a6@(-3)		| Save side bit,
	moveb	%a4@+,%a6@(-4)		| track#,
	moveb	%a4@,%a6@(-5)		| sector#
	moveb	#maxGCRSectors,%a6@(-6)	| Max. retry count

	movew	%sr,%a6@(-2)		| Save CPU status register
	oriw	#0x0600,%sr		| Block all interrupts

wsNextSect:
	movel	%a6@(o_hdr),%a4
	bsr	readSectHdr		| Get next available sector header
	bne	wsAllDone		| Return if error

	/*
	 * Is this the right track & side? If not, return with error
	 */
	movel	%a6@(o_hdr),%a4		| Sector header struct

	moveb	%a4@(o_side),%d1	| Get side#
	lsrb	#3,%d1			| "Normalize" side bit...
	andb	#1,%d1
	moveb	%a6@(-3),%d2		| Get wanted side
	eorb	%d1,%d2			| Compare side bits
	bne	wsSeekErr

	moveb	%a6@(-4),%d1		| Get wanted track# 
	cmpb	%a4@(o_track),%d1	| Compare to the read header
	beq	wsCompSect

wsSeekErr:	
	moveq	#seekErr,%d0		| Wrong track or side
	bra	wsAllDone		
	
	/*
	 * Look up the current sector number in the cache.
	 * If the buffer is dirty ("valid"), write it to disk. If not, 
	 * loop over all the slots and return if all of them are clean.
	 *
	 * Alternatively, we could decrement a "dirty sectors" counter here.
	 */
wsCompSect:	
	moveq	#0,%d1			| Clear register
	moveb	%a4@(o_sector),%d1	| get the # of header read
	lslw	#3,%d1			| sizeof cylCacheSlot_t is 8 bytes
	movel	%a6@(o_wslots),%a4
	tstl	%a4@(o_valid,%d1)	| Sector dirty?
	bne	wsBufDirty
	
	moveq	#maxGCRSectors-1,%d2	| Any dirty sectors left?
wsChkDty:
	movew	%d2,%d1
	lslw	#3,%d1			| sizeof cylCacheSlot_t is 8 bytes
	tstl	%a4@(o_valid,%d1)
	bne	wsNextSect		| Sector dirty?
	dbra	%d2,wsChkDty	

	bra	wsAllDone		| We are through with this track.

	
	/*
	 * Write sync pattern and sector data lead-in 'D5 AA'. The 
	 * missing 'AD' is made up by piping 0x0B through the nibble 
	 * table (toDisk).
	 *
	 * To set up IWM for writing:
	 *
	 * access q6H & write first byte to q7H. 
	 * Then check bit 7 of q6L (status reg) for 'IWM ready' 
	 * and write subsequent bytes to q6H.	
	 *
	 * Registers:	
	 *	%a0	IWM base address (later: data register)
	 *	%a1	Via1Base
	 *	%a2	IWM handshake register
	 *	%a3	data (tags buffer, data buffer)
	 *	%a4	Sync pattern, 'toDisk' translation table
	 */
wsBufDirty:
	movel	_C_LABEL(IWMBase),%a0
	lea	%a4@(0,%d1),%a3
	movel	%a3,%a6@(-10)		| Save ptr to current slot
	tstb	%a0@(q6H)		| Enable writing to disk
	movel	%a6@(o_hdr),%a4		| Sector header struct
	lea	%a4@(o_Tags),%a3	| Point %a3 to tags buffer
	lea	syncPattern,%a4

	moveb	%a4@+,%a0@(q7H)		| Write first sync byte
	lea	%a0@(q6L),%a2		| Point %a2 to handshake register
	lea	%a0@(q6H),%a0		| Point %a0 to IWM data register
	
	moveq	#6,%d0			| Loop counter for sync bytes
	moveq	#0,%d2
	moveq	#0,%d3
	movel	#0x02010009,%d4		| Loop counters for tag/sector data

	/*
	 * Write 5 sync bytes and first byte of sector data lead-in
	 */
wsLeadIn:	
	moveb	%a4@+,%d1		| Get next sync byte
wsLI1:	
	tstb	%a2@			| IWM ready?
	bpl	wsLI1
	moveb	%d1,%a0@		| Write it to disk
	subqw	#1,%d0
	bne	wsLeadIn
	
	moveb	%a4@+,%d1		| Write 2nd byte of sector lead-in
	lea	toDisk,%a4		| Point %a4 to nibble translation table
wsLI2:	
	tstb	%a2@			| IWM ready?
	bpl	wsLI2
	moveb	%d1,%a0@		| Write it to disk

	moveq	#0,%d5			| Clear checksum registers
	moveq	#0,%d6
	moveq	#0,%d7

	moveq	#0x0B,%d1		| 3rd byte of sector data lead-in
					| (Gets translated to 0xAD)
	moveb	%a3@+,%d2		| Get 1st byte from tags buffer
	bra	wsDataEntry

	/*
	 * The following loop reads the content of the tags buffer (12 bytes) 
	 * and the data buffer (512 bytes). 
	 * Each pass reads out three bytes and 
	 * a) splits them 6&2 into three 6 bit nibbles and a fourth byte 
	 *    consisting of the three 2 bit nibbles
	 * b) encodes the nibbles with a table to disk bytes (bit 7 set, no 
	 *    more than two consecutive zero bits) and writes them to disk as
	 *
	 *    00mmnnoo		fragment 2 bit nibbles
	 *    00mmmmmm		6 bit nibble -- first byte
	 *    00nnnnnn		6 bit nibble -- second byte
	 *    00oooooo		6 bit nibble -- third byte
	 *
	 * c) adds up three 8 bit checksums, one for each of the bytes written.
	 */
wsSD1:
	movel	%a6@(-10),%a3		| Get ptr to current slot
	movel	%a3@(o_secbuf),%a3	| Get start of sector data buffer

wsData:
	addxb	%d2,%d7
	eorb	%d6,%d2
	moveb	%d2,%d3
	lsrw	#6,%d3			| Put 2 bit nibbles into place
wsRDY01:	
	tstb	%a2@			| IWM ready?
	bpl	wsRDY01
	moveb	%a4@(0,%d3),%a0@	| Translate nibble and write
	subqw	#3,%d4			| Update counter
	moveb	%d7,%d3
	addb	%d7,%d3			| Set X flag (Why?)
	rolb	#1,%d7
	andib	#0x3F,%d0
wsRDY02:
	tstb	%a2@			| IWM ready?
	bpl	wsRDY02
	moveb	%a4@(0,%d0),%a0@	| Translate nibble and write

	/*
	 * We enter with the last byte of the sector data lead-in
	 * between our teeth (%D1, that is).
	 */
wsDataEntry:	
	moveb	%a3@+,%d0		| Get first byte
	addxb	%d0,%d5
	eorb	%d7,%d0
	moveb	%d0,%d3			| Keep top two bits
	rolw	#2,%d3			| by shifting them to MSByte
	andib	#0x3F,%d1
wsRDY03:
	tstb	%a2@			| IWM ready?
	bpl	wsRDY03
	moveb	%a4@(0,%d1),%a0@	| Translate nibble and write

	moveb	%a3@+,%d1			| Get second byte
	addxb	%d1,%d6
	eorb	%d5,%d1
	moveb	%d1,%d3			| Keep top two bits
	rolw	#2,%d3			| by shifting them to MSByte
	andib	#0x3F,%d2
wsRDY04:
	tstb	%a2@			| IWM ready?
	bpl	wsRDY04
	moveb	%a4@(0,%d2),%a0@	| Translate nibble and write

	/*
	 * XXX We have a classic off-by-one error here: the last access
	 * reaches beyond the data buffer which bombs with memory
	 * protection. The value read isn't used anyway...
	 * Hopefully there is enough time for an additional check
	 * (exit the last loop cycle before the buffer access).
	 */
	tstl	%d4			| Last loop cycle?
	beq	wsSDDone		| Then get out while we can.
	
	moveb	%a3@+,%d2		| Get third byte
	tstw	%d4			| First write tag buffer,...
	bne	wsData
	
	swap	%d4			| ...then write data buffer
	bne	wsSD1
		
	/*
	 * Write nibbles for last 2 bytes, then
	 * split checksum bytes in 6&2 and write them to disk
	 */
wsSDDone:
	clrb	%d3			| No 513th byte
	lsrw	#6,%d3			| Set up 2 bit nibbles
wsRDY05:
	tstb	%a2@			| IWM ready?
	bpl	wsRDY05
	moveb	%a4@(0,%d3),%a0@	| Write fragments
	moveb	%d5,%d3
	rolw	#2,%d3
	moveb	%d6,%d3
	rolw	#2,%d3
	andib	#0x3F,%d0
wsRDY06:
	tstb	%a2@			| IWM ready?
	bpl	wsRDY06
	moveb	%a4@(0,%d0),%a0@	| Write 511th byte
	andib	#0x3F,%d1
wsRDY07:	
	tstb	%a2@			| IWM ready?
	bpl	wsRDY07
	moveb	%a4@(0,%d1),%a0@	| write 512th byte
	moveb	%d7,%d3
	lsrw	#6,%d3			| Get fragments ready
wsRDY08:	
	tstb	%a2@			| IWM ready?
	bpl	wsRDY08
	moveb	%a4@(0,%d3),%a0@	| Write fragments
	andib	#0x3F,%d5
wsRDY09:
	tstb	%a2@			| IWM ready?
	bpl	wsRDY09
	moveb	%a4@(0,%d5),%a0@	| Write first checksum byte
	andib	#0x3F,%D6
wsRDY10:
	tstb	%a2@			| IWM ready?
	bpl	wsRDY10
	moveb	%a4@(0,%d6),%a0@	| Write second checksum byte
	andib	#0x3F,%d7
wsRDY11:
	tstb	%a2@			| IWM ready?
	bpl	wsRDY11
	moveb	%a4@(0,%d7),%a0@	| Write third checksum byte

	/*
	 * Write sector data lead-out
	 */
	lea	dataLeadOut,%a4		| Sector data lead-out
	moveq	#3,%d2			| Four bytes long {3,2,1,0}
wsLeadOut:
	moveb	%a2@,%d1		| IWM ready?
	bpl	wsLeadOut
	moveb	%a4@+,%a0@		| Write lead-out
	dbf	%d2,wsLeadOut

	moveq	#0,%d0
	btst	#6,%d1			| Check IWM underrun bit
	bne	wsNoErr

	moveq	#wrUnderRun,%d0		| Could not write
					| fast enough to keep up with IWM
wsNoErr:	
	tstb	%a0@(0x0200)		| q7L -- Write OFF

wsDone:
	tstb	%d0			| Any error? Simply retry
	bne	wsBufInvalid
	
	movel	%a6@(-10),%a4		| Else, get ptr to current slot 
	clrl	%a4@(o_valid)		| Mark current buffer "clean"
	bra	wsNextSect
	
wsBufInvalid:
	subqb	#1,%a6@(-6)		| retries
	bne	wsNextSect
					| Sector not found, but
	tstb	%d0			| don't set error code if we 
	bne	wsAllDone		| already have one.
	moveq	#sectNFErr,%d0
	
wsAllDone:
	movew	%a6@(-2),%sr		| Restore interrupt mask
	moveml	%sp@+,%d1-%d7/%a0-%a5	
	unlk	%a6
	rts

	

/**
 **	Local functions
 **/

/*
 * iwmDelay
 *
 * In-kernel calls to delay() in mac68k/clock.c bomb
 *
 * Parameters:	%d0	delay in milliseconds 
 * Trashes:	%d0, %d1
 * Returns:	-		
 */
iwmDelay:
	/* TimeDBRA is ~8K for 040/33 machines, so we need nested loops */
id00:	movew	_C_LABEL(TimeDBRA),%d1	| dbra loops per ms
id01:	dbra	%d1,id01		|
	dbra	%d0,id00
	rts


/*
 * selDriveReg -- Select drive status/control register
 *
 * Parameters:	%d0	register #
 *			(bit 0 - CA2, bit 1 - SEL, bit 2 - CA0, bit 3 - CA1)
 *		%a0	IWM base address
 *		%a1	VIA base address
 * Returns:	%d0	register # (unchanged)
 */
selDriveReg:	
	tstb	%a0@(ph0H)		| default CA0 to 1 (says IM III)
	tstb	%a0@(ph1H)		| default CA1 to 1
	
	btst	#0,%d0			| bit 0 set => CA2 on
	beq	se00
	tstb	%a0@(ph2H)
	bra	se01
se00:
	tstb	%a0@(ph2L)

se01:
	btst	#1,%d0			| bit 1 set => SEL on (VIA 1)
	beq	se02
	bset	#vHeadSel,%a1@(vBufA)
	bra	se03
se02:
	bclr	#vHeadSel,%a1@(vBufA)

se03:
	btst	#2,%d0			| bit 2 set => CA0 on
	bne	se04
	tstb	%a0@(ph0L)
	
se04:
	btst	#3,%d0			| bit 3 set => CA1 on
	bne	se05
	tstb	%a0@(ph1L)
se05:
	rts

	

/*
 * dstatus -- check drive status (bit 7 - N flag) wrt. a previously
 * set status tag.
 *
 * Parameters:	%d0	register selector
 *		%a0	IWM base address
 * Returns:	%d0	status
 */
dstatus:
	tstb	%a0@(q6H)
	moveb	%a0@(q7L),%d0
	tstb	%a0@(q6L)		| leave in "read data reg" 
	tstb	%d0			| state for safety
	rts


/*
 * driveStat -- query drive status.
 *
 * Parameters:	%a0	IWMBase
 *		%a1	VIABase
 *		%d0	register selector
 * Returns:	%d0	status (Bit 7)
 */
driveStat:
	tstb	%a0@(mtrOn)		| ENABLE; turn drive on	
	bsr	selDriveReg
	bsr	dstatus
	rts

	
/*
 * dtrigger -- toggle LSTRB line to give drive a strobe signal
 * IM III says pulse length = 1 us < t < 1 ms
 *
 * Parameters:	%a0	IWMBase
 *		%a1	VIABase
 * Returns:	-
 */
dtrigger:
	tstb	%a0@(ph3H)		| LSTRB high
	moveb	%a1@(vBufA),%a1@(vBufA)	| intelligent nop seen in q700 ROM
	tstb	%a0@(ph3L)		| LSTRB low
	rts

	
/*
 * driveCmd -- send command to drive.
 *
 * Parameters:	%a0	IWMBase
 *		%a1	VIABase
 *		%d0	Command token
 * Returns:	-
 */
driveCmd:
	bsr	selDriveReg
	bsr	dtrigger
	rts

	
/*
 * readSectHdr -- read and decode the next available sector header.
 *
 * TODO:	Poll SCC as long as interrupts are disabled.
 *
 * Parameters:	%a4	sectorHdr_t address
 * Returns:	%d0	result code
 * Uses:	%d0-%d4, %a0, %a2-%a4
 */
readSectHdr:	
	moveq	#3,%d4			| Read 3 chars from IWM for sync
	movew	#1800,%d3		| Retries to sync to disk
	moveq	#0,%d2			| Clear scratch regs
	moveq	#0,%d1
	moveq	#0,%d0
 	movel	_C_LABEL(IWMBase),%a0	| IWM base address

	tstb	%a0@(q7L)
	lea	%a0@(q6L),%a0		| IWM data register
shReadSy:
	moveb	%a0@,%d2		| Read char
	dbra	%d3,shSeekSync
	
	moveq	#noNybErr,%d0		| Disk is blank?
	bra	shDone

shSeekSync:
	bpl	shReadSy		| No char at IWM, repeat read
	subqw	#1,%d4
	bne	shReadSy
	/*
	 * When we get here, the IWM should be in sync with the data
	 * stream from disk.
	 * Next look for sector header lead-in 'D5 AA 96'
	 */
	movew	#1500,%d3		| Retries to seek header
shLeadIn:	
	lea	hdrLeadIn,%a3		| Sector header lead-in bytes
	moveq	#0x03,%d4		| is 3 bytes long
shLI1:	
	moveb	%a0@,%d2		| Get next byte
	bpl	shLI1			| No char at IWM, repeat read
	dbra	%d3,shLI2
	moveq	#noAdrMkErr,%d0		| Can't find an address mark
	bra	shDone

shLI2:
	cmpb	%a3@+,%d2
	bne	shLeadIn		| If ne restart scan
	subqw	#1,%d4
	bne	shLI1
	/*
	 * We have found the lead-in. Now get the header information.
	 * Reg %d4 holds the checksum.
	 */
	lea	diskTo-0x90,%a2		| Translate disk bytes -> 6&2
shHdr1:	
	moveb	%a0@,%d0		| Get 1st char
	bpl	shHdr1
	moveb	%a2@(0,%d0),%d1		| and remap it
	moveb	%d1,%d4
	rorw	#6,%d1			| separate 2:6, drop hi bits
shHdr2:	
	moveb	%a0@,%d0		| Get 2nd char
	bpl	shHdr2
	moveb	%a2@(0,%d0),%d2		| and remap it
	eorb	%d2,%d4
shHdr3:
	moveb	%a0@,%d0		| Get 3rd char
	bpl	shHdr3
	moveb	%a2@(0,%d0),%d1		| and remap it
	eorb	%d1,%d4
	rolw	#6,%d1			|
shHdr4:
	moveb	%a0@,%d0		| Get 4th char
	bpl	shHdr4
	moveb	%a2@(0,%d0),%d3		| and remap it
	eorb	%d3,%d4
shHdr5:
	moveb	%a0@,%d0		| Get checksum byte
	bpl	shHdr5
	moveb	%a2@(0,%d0),%d5		| and remap it
	eorb	%d5,%d4
	bne	shCsErr			| Checksum ok?
	/*
	 * We now have in
	 * %d1/lsb	track number
	 * %d1/msb	bit 3 is side bit
	 * %d2		sector number
	 * %d3		???
	 * %d5		checksum (=0)
	 *
	 * Next check for lead-out.
	 */
	moveq	#1,%d4			| is 2 bytes long
shHdr6:	
	moveb	%a0@,%d0		| Get token
	bpl	shHdr6
	cmpb	%a3@+,%d0		| Check
	bne	shLOErr			| Fault!
	dbra	%d4,shHdr6
	movew	%d1,%d0			| Isolate side bit
	lsrw	#8,%d0
	moveb	%d0,%a4@+		| and store it
	moveb	%d1,%a4@+		| Store track number
	moveb	%d2,%a4@+		| and sector number
	moveq	#0,%d0			| All is well
	bra	shDone

shCsErr:
	moveq	#badCkSmErr,%d0		| Bad sector header checksum
	bra	shDone
shLOErr:
	moveq	#badBtSlpErr,%d0	| Bad address mark (no lead-out)
	
shDone:
	tstl	%d0			| Set flags
	rts
