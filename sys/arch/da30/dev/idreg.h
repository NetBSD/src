/*	$NetBSD: idreg.h,v 1.4 1995/03/28 18:15:21 jtc Exp $	*/

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)idreg.h	7.1 (Berkeley) 5/9/91
 */

/*
 * Disk Controller register definitions.
 */
struct idc {
    u_char	data;		/* data register (R/W - 16 bits) */
    u_char	error;		/* error register (R), precomp (W) */
    u_char	seccnt;		/* sector count (R/W) */
    u_char	sector;		/* first sector number (R/W) */
    u_char	cyl_lo;		/* cylinder address, low byte (R/W) */
    u_char	cyl_hi;		/* cylinder address, high byte (R/W)*/
    u_char	sdh;		/* sector size/drive/head (R/W)*/
    u_char	csr;		/* command/status register (W)	 */

    u_char	hidata;		/* high byte of data */
    u_char	dummy9;
    u_char	dummya;
    u_char	dummyb;
    u_char	dummyc;
    u_char	dummyd;
    u_char	ccr;		/* fixed disk controller control (W)*/
#define  IDCTL_4BIT	 0x8	/* use four head bits (id1003) */
#define  IDCTL_RST	 0x4	/* reset the controller */
#define  IDCTL_IDS	 0x2	/* disable controller interrupts */
    u_char	digin;		/* disk controller input(via 1015) (R)*/
};

/*
 * Status Bits.
 */
#define	IDCS_BUSY	0x80		/* Controller busy bit. */
#define	IDCS_READY	0x40		/* Selected drive is ready */
#define	IDCS_WRTFLT	0x20		/* Write fault */
#define	IDCS_SEEKCMPLT	0x10		/* Seek complete */
#define	IDCS_DRQ	0x08		/* Data request bit. */
#define	IDCS_ECCCOR	0x04		/* ECC correction made in data */
#define	IDCS_INDEX	0x02		/* Index pulse from selected drive */
#define	IDCS_ERR	0x01		/* Error detect bit. */

#define IDCS_BITS	"\020\010busy\007rdy\006wrtflt\005seekdone\004drq\003ecc_cor\002index\001err"

#define IDERR_BITS	"\020\010badblk\007uncorr\006id_crc\005no_id\003abort\002tr000\001no_dam"

/*
 * Commands for Disk Controller.
 */
#define	IDCC_RESTORE	0x10		/* disk restore code -- resets cntlr */

#define	IDCC_READ	0x20		/* disk read code */
#define	IDCC_WRITE	0x30		/* disk write code */
#define	 IDCC__LONG	 0x02		 /* modifier -- access ecc bytes */
#define	 IDCC__NORETRY	 0x01		 /* modifier -- no retrys */

#define	IDCC_FORMAT	0x50		/* disk format code */
#define	IDCC_DIAGNOSE	0x90		/* controller diagnostic */
#define	IDCC_IDC	0x91		/* initialize drive command */

#define	IDCC_EXTDCMD	0xE0		/* send extended command */
#define IDCC_STANDBY	0xE2		/* stop the disk */
#define	IDCC_READP	0xEC		/* read parameters from controller */
#define	IDCC_CACHEC	0xEF		/* cache control */

#define	ID_STEP		0		/* winchester- default 35us step */

#define	IDSD_IBM	0xa0		/* forced to 512 byte sector, ecc */


#ifdef _KERNEL
/*
 * read parameters command returns this:
 */
struct idparams {
	/* drive info */
	short	idp_config;	/* general configuration */
	short	idp_fixedcyl;	/* number of non-removable cylinders */
	short	idp_removcyl;	/* number of removable cylinders */
	short	idp_heads;	/* number of heads */
	short	idp_unfbytespertrk; /* number of unformatted bytes/track */
	short	idp_unfbytes;	/* number of unformatted bytes/sector */
	short	idp_sectors;	/* number of sectors */
	short	idp_minisg;	/* minimum bytes in inter-sector gap*/
	short	idp_minplo;	/* minimum bytes in postamble */
	short	idp_vendstat;	/* number of words of vendor status */
	/* controller info */
	char	idp_cnsn[20];	/* controller serial number */
	short	idp_cntype;	/* controller type */
#define	IDTYPE_SINGLEPORTSECTOR	1 /* single port, single sector buffer */
#define	IDTYPE_DUALPORTMULTI	2 /* dual port, multiple sector buffer */
#define	IDTYPE_DUALPORTMULTICACHE 3 /* above plus track cache */
	short	idp_cnsbsz;	/* sector buffer size, in sectors */
	short	idp_necc;	/* ecc bytes appended */
	char	idp_rev[8];	/* firmware revision */
	char	idp_model[40];	/* model name */
	short	idp_nsecperint;	/* sectors per interrupt */
	short	idp_usedmovsd;	/* can use double word read/write? */
};

/*
 * id driver entry points
 */
void idstrategy(struct buf *);
int idintr();
int idopen(dev_t, int, int, struct proc *);
int idclose(dev_t dev, int flags, int fmt);
int idioctl(dev_t, int, caddr_t, int, struct proc *);
/* int idformat(struct buf *bp); */
int idsize(dev_t);
int iddump(dev_t);

#endif KERNEL
