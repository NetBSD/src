/*	$NetBSD: fdreg.h,v 1.1.1.1 1996/05/05 12:17:03 oki Exp $	*/

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
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
 *	@(#)fdreg.h	7.1 (Berkeley) 5/9/91
 */

/*
 * x680x0 floppy controller registers and bitfields
 */

/* uses NEC72065 controller */
#include <dev/ic/nec765reg.h>

/* Status registers returned as result of operation. */
#define ST0             0x00	/* status register 0 */
#define ST1             0x01	/* status register 1 */
#define ST2             0x02	/* status register 2 */
#define ST3             0x00	/* status register 3 (return by DRIVE_SENSE) */
#define ST_CYL          0x03	/* slot where controller reports cylinder */
#define ST_HEAD         0x04	/* slot where controller reports head */
#define ST_SEC          0x05	/* slot where controller reports sector */
#define ST_PCN          0x01	/* slot where controller reports present cyl */

/* Fields within the I/O ports. */
/* FDD registers */
#define EJECT		0x20
#define MOTOR_ON	0x80

/* Floppy disk controller command bytes. */
#define FDC_SEEK        0x0F	/* command the drive to seek */
#define FDC_READ        0xE6	/* command the drive to read */
#define FDC_WRITE       0xC5	/* command the drive to write */
#define FDC_SENSE       0x08	/* command the controller to tell its status */
#define FDC_RECALIBRATE 0x07	/* command the drive to go to cyl 0 */
#define FDC_SPECIFY     0x03	/* command the drive to accept params */
#define FDC_READ_ID     0x4A	/* command the drive to read sector identity */
#define FDC_FORMAT      0x4D	/* command the drive to format a track */
#define FDC_RESET       0x36	/* reset command for fdc */

/* Main status register. */
#define NE7_D0B	0x01	/* Diskette drive 0 is seeking, thus busy */
#define NE7_D1B	0x02	/* Diskette drive 1 is seeking, thus busy */
#define NE7_D2B	0x01	/* Diskette drive 2 is seeking, thus busy */
#define NE7_D3B	0x02	/* Diskette drive 3 is seeking, thus busy */
#define NE7_CB	0x10	/* Diskette Controller Busy */
#define NE7_NDM	0x20	/* Diskette Controller in Non Dma Mode */
#define NE7_DIO	0x40	/* Diskette Controller Data register I/O */
#define NE7_RQM	0x80	/* Diskette Controller ReQuest for Master */

/* registers */
#define	fdout	2	/* Digital Output Register (W) */
#define	FDO_FDSEL	0x03	/*  floppy device select */
#define	FDO_FRST	0x04	/*  floppy controller reset */
#define	FDO_FDMAEN	0x08	/*  enable floppy DMA and Interrupt */
#define	FDO_MOEN(n)	((1 << n) * 0x10)	/* motor enable */

#define	fdsts	4	/* NEC 765 Main Status Register (R) */
#define	fddata	5	/* NEC 765 Data Register (R/W) */

#define	fdctl	7	/* Control Register (W) */
#define	FDC_500KBPS	0x00	/* 500KBPS MFM drive transfer rate */
#define	FDC_300KBPS	0x01	/* 300KBPS MFM drive transfer rate */
#define	FDC_250KBPS	0x02	/* 250KBPS MFM drive transfer rate */
#define	FDC_125KBPS	0x03	/* 125KBPS FM drive transfer rate */

#define	fdin	7	/* Digital Input Register (R) */
#define	FDI_DCHG	0x80	/* diskette has been changed */

#define	FDC_BSIZE	512
#define	FDC_NPORT	8
#define	FDC_MAXIOSIZE	NBPG	/* XXX should be MAXBSIZE */
