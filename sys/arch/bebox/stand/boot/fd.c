/*	$NetBSD: fd.c,v 1.3.16.1 2001/09/13 01:13:29 thorpej Exp $	*/

/*-
 * Copyright (C) 1997-1998 Kazuki Sakamoto (sakamoto@netbsd.org)
 * All rights reserved.
 *
 * Floppy Disk Drive standalone device driver
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
 *      This product includes software developed by Kazuki Sakamoto.
 * 4. The name of the author may not be used to endorse or promote products
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

#include <sys/param.h>
#include <stand.h>
#include "boot.h"

/*---------------------------------------------------------------------------*
 *			Floppy Disk Controller Define			     *
 *---------------------------------------------------------------------------*/
/* Floppy Disk Controller Registers */
int FDC_PORT[] = {				/* fdc base I/O port */
		0x3f0, /* primary */
		};
#define	FDC_DOR(x)	(FDC_PORT[x] + 0x2)	/* motor drive control bits */
#define	FDC_STATUS(x)	(FDC_PORT[x] + 0x4)	/* fdc main status register */
#define	FDC_DATA(x)	(FDC_PORT[x] + 0x5)	/* fdc data register */
#define	FDC_RATE(x)	(FDC_PORT[x] + 0x7)	/* transfer rate register */

#define	FDC_IRQ		6
#define	FD_DMA_CHAN	2

/* fdc main status register */
#define	RQM	  0x80	/* the host can transfer data if set */
#define	DIO	  0x40	/* direction of data transfer. write required if set */
#define	NON_DMA   0x20  /* fdc have date for transfer in non dma mode */
#define	CMD_BUSY  0x10	/* command busy if set */

/* fdc result status */
#define	ST0_IC_MASK	0xc0	/* interrupt code  00:normal terminate */
#define	ST1_EN		0x80	/* end of cylinder */

/* fdc digtal output register */
#define	DOR_DMAEN	0x08	/* DRQ, nDACK, TC and FINTR output enable */
#define	DOR_RESET	0x04	/* fdc software reset */

/* fdc command */
#define	CMD_RECALIBRATE	0x07	/* recalibrate */
#define	CMD_SENSE_INT	0x08	/* sense interrupt status */
#define	CMD_DRV_SENSE	0x04	/* sense drive status */
#define	CMD_SEEK	0x0f	/* seek */
#define	CMD_FORMAT	0x4d	/* format */
#define	CMD_READ	0x46	/* read e6 */
#define	CMD_WRITE	0xc5	/* write */
#define	CMD_VERIFY	0xf6	/* verify */
#define	CMD_READID	0x4a	/* readID */
#define	CMD_SPECIFY	0x03	/* specify */
#define	CMD_CONFIG	0x13	/* config */
#define	CMD_VERSION	0x10	/* version */

/* command specify value */
#define	SPECIFY1	((0x0d<<4)|0x0f)
#define	SPECIFY2	((0x01<<1)|0)	/* DMA MODE */

/* fdc result */
#define	STATUS_MAX	16	/* result status max number */
#define	RESULT_VERSION	0x90	/* enhanced controller */
#define	RESULT_SEEK	0x20	/* seek & recalibrate complete flag on status0 */

/*---------------------------------------------------------------------------*
 *			     Floppy Disk Type Define	 		     *
 *---------------------------------------------------------------------------*/
struct	fdd_type {
	int	seccount;	/* sector per track */
	int	secsize;	/* byte per sector (uPD765 paramater) */
	int	datalen;	/* data length */
	int	gap;		/* gap */
	int	gaplen;		/* gap length */
	int	cylinder;	/* track per media */
	int	maxseccount;	/* media max sector count */
	int	step;		/* seek step */
	int	rate;		/* drive rate (250 or 500kbps) */
	int	heads;		/* heads */
	int	f_gap;		/* format gap */
	int	mselect;	/* drive mode select */
	char	*type_name;	/* media type name */
};
typedef struct	fdd_type FDDTYPE;

#define	FDTYPE_MAX	5
FDDTYPE fdd_types[FDTYPE_MAX] = {
	{ 18,2,0xff,0x1b,0x54,80,2880,1,0,2,0x6c,0,"2HQ" }, /* 2HD (PC/AT) */
	{  8,3,0xff,0x35,0x74,77,1232,1,0,2,0x54,1,"2HD" }, /* 2HD (98) */
	{ 15,2,0xff,0x1b,0x54,80,2400,1,0,2,0x54,1,"2HC" }, /* 2HC */
	{  9,2,0xff,0x23,0x50,80,1440,1,2,2,0x50,1,"2DD9" },/* 2DD 9 sector */
	{  8,2,0xff,0x3a,0x50,80,1280,1,2,2,0x50,1,"2DD8" },/* 2DD 8 sector */
};

int	fdsectors[] = {128, 256, 512, 1024, 2048, 4096};
#define	SECTOR_MAX	4096
#define	FDBLK	(fdsectors[un->un_type->secsize])

#define	START_CYL	0
#define	START_SECTOR	1

#define	DELAY(x)	delay(100000 * x)		/* about 100ms */
#define	INT_TIMEOUT	3000000

/*---------------------------------------------------------------------------*
 *			FDC Device Driver Define			     *
 *---------------------------------------------------------------------------*/
#define	CTLR_MAX	1
#define	UNIT_MAX	2

struct	fd_unit {
	int	ctlr;
	int	unit;
	int	part;
	u_int	un_flags;		/* unit status flag */
	int	stat[STATUS_MAX];	/* result code */
	FDDTYPE	*un_type;		/* floppy type (pointer) */
};
typedef	struct fd_unit FD_UNIT;
FD_UNIT	fd_unit[CTLR_MAX][UNIT_MAX];

/*
 *	un_flags flags
 */
#define	INT_ALIVE	0x00000001	/* Device is Alive and Available */
#define	INT_READY	0x00000002	/* Device is Ready */
#define	INT_BUSY	0x00000004	/* Device is busy */

/*---------------------------------------------------------------------------*
 *				Misc define				     *
 *---------------------------------------------------------------------------*/
#define	TIMEOUT		10000000
#define	ND_TIMEOUT	10000000

#define	SUCCESS		0
#define	FAIL		-1

/*
 *	function declaration
 */
int fdc_out __P((int, int));
int fdc_in __P((int, u_char *));
int fdc_intr_wait __P((void));
int fd_check __P((FD_UNIT *));
void motor_on __P((int, int));
void motor_off __P((int, int));
void fdReset __P((int));
void fdRecalibrate __P((int, int));
void fdSpecify __P((int));
void fdDriveStatus __P((int, int, int, int *));
int fdSeek __P((int, int, int));
int fdSenseInt __P((int, int *));
int fdReadWrite __P((FD_UNIT *, int, int, int, int, u_char *));
void irq_init __P((void));
int irq_polling __P((int, int));
void dma_setup __P((u_char *, int, int, int));
int dma_finished __P((int));

/*===========================================================================*
 *				   fdinit				     *
 *===========================================================================*/
int
fdinit(un)
	FD_UNIT	*un;
{
	int ctlr = un->ctlr;
	u_char result;

#if 0
	irq_init();
#endif
	fdReset(ctlr);

	if (fdc_out(ctlr, CMD_VERSION) != SUCCESS) {  /* version check */
		printf ("fdc%d:fatal error: CMD_VERSION cmd fail\n", ctlr);
		return (FAIL);
	}
	if (fdc_in(ctlr, &result) != SUCCESS) {
		printf ("fdc%d:fatal error: CMD_VERSION exec fail\n", ctlr);
		return (FAIL);
	}
	if (result != (u_char)RESULT_VERSION) {
		printf ("fdc%d:fatal error: unknown version fdc\n", ctlr);
		return (FAIL);
	}

	un->un_flags = INT_ALIVE;
	return (SUCCESS);
}

/*===========================================================================*
 *				   fdopen				     *
 *===========================================================================*/
int
fdopen(f, ctlr, unit, part)
	struct open_file *f;
	int ctlr, unit, part;
{
	FD_UNIT	*un;
	int *stat = un->stat;

	if (ctlr >= CTLR_MAX)
		return (ENXIO);
	if (unit >= UNIT_MAX)
		return (ENXIO);
	un = &fd_unit[ctlr][unit];

	if (!(un->un_flags & INT_ALIVE)) {
		if (fdinit(un) != SUCCESS)
			return (ENXIO);
	}

	motor_on(ctlr, unit);

	fdRecalibrate(ctlr, unit);
	fdSenseInt(ctlr, stat);
	if (stat[1] != START_CYL) {
		printf("fdc%d: unit:%d recalibrate failed. status:0x%x cyl:%d\n",
			ctlr, unit, stat[0], stat[1]);
		motor_off(ctlr, unit);
		return (EIO);
	}

	if (fd_check(un) != SUCCESS)	/* research disk type */
		return (EIO);

	f->f_devdata = (void *)un;
	return (SUCCESS);
}

/*===========================================================================*
 *				   fdclose				     *
 *===========================================================================*/
int
fdclose(f)
	struct open_file *f;
{
	FD_UNIT *un = f->f_devdata;

	fdRecalibrate(un->ctlr, un->unit);
	fdSenseInt(un->ctlr, un->stat);
	motor_off(un->ctlr, un->unit);
	un->un_flags = 0;
	return (SUCCESS);
}

/*===========================================================================*
 *				   fdioctl				     *
 *===========================================================================*/
int
fdioctl(f, cmd, arg)
	struct open_file *f;
	u_long cmd;
	void *arg;
{
	switch (cmd) {
	default:
		return (EIO);
	}

	return (SUCCESS);
}

/*===========================================================================*
 *				   fdstrategy				     *
 *===========================================================================*/
int
fdstrategy(devdata, func, blk, size, buf, rsize)
	void *devdata;	/* device uniq data */
	int func;	/* function (read or write) */
	daddr_t blk;	/* block number */
	size_t size;	/* request size in bytes */
	void *buf;	/* buffer */
	size_t *rsize;	/* bytes transferred */
{
	int sectrac, cyl, head, sec;
	FD_UNIT *un = devdata;
	int ctlr = un->ctlr;
	int unit = un->unit;
	int *stat = un->stat;
	long nblock, blknum;
	int fd_skip = 0;
	char *cbuf = (char *)buf;

	if (un->un_flags & INT_BUSY) {
		return (ENXIO);
	}
	fdDriveStatus(ctlr, unit, 0, stat);

	nblock = un->un_type->maxseccount;
	sectrac = un->un_type->seccount;	/* sector per track */
	*rsize = 0;

	while (fd_skip < size) {
		blknum = (u_long)blk * DEV_BSIZE/FDBLK + fd_skip/FDBLK;
		cyl = blknum / (sectrac * 2);
		fdSeek(ctlr, unit, cyl);
		fdSenseInt(ctlr, stat);
		if (!(stat[0] & RESULT_SEEK)) {
			printf("fdc%d: unit:%d seek failed."
				"status:0x%x cyl:%d pcyl:%d\n",
				ctlr, unit, stat[0], cyl, stat[1]);
			goto bad;
		}

		sec = blknum % (sectrac * 2);
		head = sec / sectrac;
		sec = sec % sectrac + 1;

		if (fdReadWrite(un, func, cyl, head, sec, cbuf) == FAIL) {
			printf("fdc%d: unit%d fdReadWrite error [%s]\n",
			    ctlr, unit, (func==F_READ?"READ":"WRITE"));
			goto bad;
		}

		*rsize += FDBLK;
		cbuf += FDBLK;
		fd_skip += FDBLK;
	}
	return (SUCCESS);

bad:
	return (FAIL);
}

/*===========================================================================*
 *				   fd_check				     *
 *===========================================================================*/
/*
 *	this function is Check floppy disk Type
 */
int
fd_check(un)
	FD_UNIT	*un;
{
	int ctlr = un->ctlr;
	int unit = un->unit;
	int *stat = un->stat;
	int type;
	static u_char sec_buff[SECTOR_MAX];

	un->un_type = (FDDTYPE *)FAIL;
	for (type = 0; type < FDTYPE_MAX; type++) {
		un->un_type = &fdd_types[type];

		/* try read start sector */
		outb(FDC_RATE(ctlr), un->un_type->rate);   /* rate set */
		fdSpecify(ctlr);
		fdSeek(ctlr, unit, START_CYL);
		fdSenseInt(ctlr, stat);
		if (!(stat[0] & RESULT_SEEK) || stat[1] != START_CYL) {
			printf("fdc%d: unit:%d seek failed. status:0x%x\n",
				ctlr, unit, stat[0]);
			goto bad;
		}
		if (fdReadWrite(un, F_READ,
		    START_CYL, 0, START_SECTOR, sec_buff) == FAIL) {
			continue;	/* bad disk type */
		}
		break;
	}
	if (un->un_type == (FDDTYPE *)FAIL) {
		printf("fdc%d: unit:%d check disk type failed.\n",
		ctlr, unit);
		goto bad;
	}
	return (SUCCESS);
bad:
	return (FAIL);
}

/*
 * for FDC routines.
 */
/*===========================================================================*
 *				fdc_out					     *
 *===========================================================================*/
int
fdc_out(ctlr, cmd)
	int ctlr;	/* controller no */
	int cmd;	/* cmd */
{
	volatile int status;
	int time_out;

	time_out = TIMEOUT;
	while (((status = inb(FDC_STATUS(ctlr))) & (RQM | DIO))
		!= (RQM | 0) && time_out-- > 0);
	if (time_out <= 0) {
		printf("fdc_out: timeout  status = 0x%x\n", status);
		return (FAIL);
	}

	outb(FDC_DATA(ctlr), cmd);

	return (SUCCESS);
}

/*===========================================================================*
 *				fdc_in					     *
 *===========================================================================*/
int
fdc_in(ctlr, data)
	int ctlr;	/* controller no */
	u_char *data;
{
	volatile int status;
	int time_out;

	time_out = TIMEOUT;
	while ((status = inb(FDC_STATUS(ctlr)) & (RQM | DIO))
	    != (RQM | DIO) && time_out-- > 0) {
		if (status == RQM) {
			printf("fdc_in:error:ready for output\n");
			return (FAIL);
		}
	}

	if (time_out <= 0) {
		printf("fdc_in:input ready timeout\n");
		return (FAIL);
	}

	if (data) *data = (u_char)inb(FDC_DATA(ctlr));

	return (SUCCESS);
}

/*===========================================================================*
 *                              fdc_intr_wait                                *
 *===========================================================================*/
int
fdc_intr_wait()
{
	return (irq_polling(FDC_IRQ, INT_TIMEOUT));	/* wait interrupt */
}

/*===========================================================================*
 *		   	     fdc command function	 	    	     *
 *===========================================================================*/
void
motor_on(ctlr, unit)
	int ctlr;
	int unit;
{
	outb(FDC_DOR(ctlr), DOR_RESET | DOR_DMAEN | unit
		| (1 << (unit + 4)));	/* reset & unit motor on */
	DELAY(1);		/* wait 100msec */
}

void
motor_off(ctlr, unit)
	int ctlr;
	int unit;
{
        outb(FDC_DOR(ctlr), DOR_RESET);		/* reset & motor off */
	if (fdc_intr_wait() == FAIL)		/* wait interrupt */
		printf("fdc: motor off failed.\n");
}

void
fdReset(ctlr)
{
	outb(FDC_DOR(ctlr), 0); /* fdc reset */
	DELAY(3);
	outb(FDC_DOR(ctlr), DOR_RESET);
	DELAY(8);
}

void
fdRecalibrate(ctlr, unit)
	int ctlr;
	int unit;
{
	fdc_out(ctlr, CMD_RECALIBRATE);
	fdc_out(ctlr, unit);

	if (fdc_intr_wait() == FAIL)   /* wait interrupt */
		printf("fdc: recalibrate Timeout\n");
}

void
fdSpecify(ctlr)
	int	ctlr;
{
	fdc_out(ctlr, CMD_SPECIFY);
	fdc_out(ctlr, SPECIFY1);
	fdc_out(ctlr, SPECIFY2);
}

void
fdDriveStatus(ctlr, unit, head, stat)
	int	ctlr;
	register int	unit, head;
	register int	*stat;
{
	u_char result;

	fdc_out(ctlr, CMD_DRV_SENSE);
	fdc_out(ctlr, (head << 2) | unit);
	fdc_in(ctlr, &result);
	*stat = (int)(result & 0xff);
}

int
fdSeek(ctlr, unit, cyl)
	int ctlr;
	int unit;
	int cyl;
{
	int ret_val = 0;

	fdc_out(ctlr, CMD_SEEK);
	fdc_out(ctlr, unit);
	fdc_out(ctlr, cyl);

        if (fdc_intr_wait() == FAIL) {		/* wait interrupt */
		printf("fdc: fdSeek Timeout\n");
		ret_val = FAIL;
	}

	return(ret_val);
}

int
fdSenseInt(ctlr, stat)
	int ctlr;
	int *stat;
{
	u_char result;

	fdc_out(ctlr, CMD_SENSE_INT);

	fdc_in(ctlr, &result);
	*stat++ = (int)(result & 0xff);
	fdc_in(ctlr, &result);
	*stat++ = (int)(result & 0xff);

	return (0);
}

int
fdReadWrite(un, func, cyl, head, sec, adrs)
	FD_UNIT	*un;
	int func;
	int cyl;
	int head;
	int sec;
	u_char *adrs;
{
	int i;
	int ctlr = un->ctlr;
	int unit = un->unit;
	int *stat = un->stat;
	u_char result;

#if 0
printf("%s:", (func == F_READ ? "READ" : "WRITE"));
printf("cyl = %d", cyl);
printf("head = %d", head);
printf("sec = %d", sec);
printf("secsize = %d", un->un_type->secsize);
printf("seccount = %d", un->un_type->seccount);
printf("gap = %d", un->un_type->gap);
printf("datalen = %d\n", un->un_type->datalen);
#endif

	dma_setup(adrs, FDBLK, func, FD_DMA_CHAN);
	fdc_out(ctlr, (func == F_READ ? CMD_READ : CMD_WRITE));
	fdc_out(ctlr, (head<<2) | unit);
	fdc_out(ctlr, cyl);			/* cyl */
	fdc_out(ctlr, head);			/* head */
	fdc_out(ctlr, sec);			/* sec */
	fdc_out(ctlr, un->un_type->secsize);	/* secsize */
	fdc_out(ctlr, un->un_type->seccount);	/* EOT (end of track) */
	fdc_out(ctlr, un->un_type->gap);	/* GAP3 */
	fdc_out(ctlr, un->un_type->datalen);	/* DTL (data length) */

	if (fdc_intr_wait() == FAIL) {  /* wait interrupt */
		printf("fdc: DMA transfer Timeout\n");
		return (FAIL);
	}

	for (i = 0; i < 7; i++) {
		fdc_in(ctlr, &result);
		stat[i] = (int)(result & 0xff);
	}
	if (stat[0] & ST0_IC_MASK) {	/* not normal terminate */
		if ((stat[1] & ~ST1_EN) || stat[2])
		goto bad;
	}
	if (!dma_finished(FD_DMA_CHAN)) {
		printf("DMA not finished\n");
		goto bad;
	}
	return (SUCCESS);

bad:
	printf("       func: %s\n", (func == F_READ ? "F_READ" : "F_WRITE"));
	printf("	st0 = 0x%x\n", stat[0]);
	printf("	st1 = 0x%x\n", stat[1]);
	printf("	st2 = 0x%x\n", stat[2]);
	printf("	  c = 0x%x\n", stat[3]);
	printf("	  h = 0x%x\n", stat[4]);
	printf("	  r = 0x%x\n", stat[5]);
	printf("	  n = 0x%x\n", stat[6]);
	return (FAIL);
}

/*-----------------------------------------------------------------------
 * Interrupt Controller Operation Functions
 *-----------------------------------------------------------------------
 */

/* 8259A interrupt controller register */
#define	INT_CTL0	0x20
#define	INT_CTL1	0x21
#define	INT2_CTL0	0xA0
#define	INT2_CTL1	0xA1

#define	CASCADE_IRQ	2

#define	ICW1_AT		0x11    /* edge triggered, cascade, need ICW4 */
#define	ICW4_AT		0x01    /* not SFNM, not buffered, normal EOI, 8086 */
#define	OCW3_PL		0x0e	/* polling mode */
#define	OCW2_CLEAR	0x20	/* interrupt clear */

/*
 * IRC programing sequence
 *
 * after reset
 * 1.	ICW1 (write port:INT_CTL0 data:bit4=1)
 * 2.	ICW2 (write port:INT_CTL1)
 * 3.	ICW3 (write port:INT_CTL1)
 * 4.	ICW4 (write port:INT_CTL1)
 *
 * after ICW
 *	OCW1 (write port:INT_CTL1)
 *	OCW2 (write port:INT_CTL0 data:bit3=0,bit4=0)
 *	OCW3 (write port:INT_CTL0 data:bit3=1,bit4=0)
 *
 *	IMR  (read port:INT_CTL1)
 *	IRR  (read port:INT_CTL0)	OCW3(bit1=1,bit0=0)
 *	ISR  (read port:INT_CTL0)	OCW3(bit1=1,bit0=1)
 *	PL   (read port:INT_CTL0)	OCW3(bit2=1,bit1=1)
 */

u_int INT_MASK;
u_int INT2_MASK;

/*===========================================================================*
 *                             irq initialize                                *
 *===========================================================================*/
void
irq_init()
{
	outb(INT_CTL0, ICW1_AT);		/* ICW1 */
	outb(INT_CTL1, 0);			/* ICW2 for master */
	outb(INT_CTL1, (1 << CASCADE_IRQ));	/* ICW3 tells slaves */
	outb(INT_CTL1, ICW4_AT);		/* ICW4 */

	outb(INT_CTL1, (INT_MASK = ~(1 << CASCADE_IRQ)));
				/* IRQ mask(exclusive of cascade) */

	outb(INT2_CTL0, ICW1_AT);		/* ICW1 */
	outb(INT2_CTL1, 8); 			/* ICW2 for slave */
	outb(INT2_CTL1, CASCADE_IRQ);		/* ICW3 is slave nr */
	outb(INT2_CTL1, ICW4_AT);		/* ICW4 */

	outb(INT2_CTL1, (INT2_MASK = ~0));	/* IRQ 8-15 mask */
}

/*===========================================================================*
 *                           irq polling check                               *
 *===========================================================================*/
int
irq_polling(irq_no, timeout)
	int	irq_no;
	int	timeout;
{
	int	irc_no;
	int	data;
	int	ret;

	if (irq_no > 8) irc_no = 1;
		else irc_no = 0;

	outb(irc_no ? INT2_CTL1 : INT_CTL1, ~(1 << (irq_no >> (irc_no * 3))));

	while (--timeout > 0) {
		outb(irc_no ? INT2_CTL0 : INT_CTL0, OCW3_PL);
						/* set polling mode */
		data = inb(irc_no ? INT2_CTL0 : INT_CTL0);
		if (data & 0x80) {	/* if interrupt request */
			if ((irq_no >> (irc_no * 3)) == (data & 0x7)) {
				ret = SUCCESS;
				break;
			}
		}
	}
	if (!timeout) ret = FAIL;

	if (irc_no) {				/* interrupt clear */
		outb(INT2_CTL0, OCW2_CLEAR | (irq_no >> 3));
		outb(INT_CTL0, OCW2_CLEAR | CASCADE_IRQ);
	} else {
		outb(INT_CTL0, OCW2_CLEAR | irq_no);
	}

	outb(INT_CTL1, INT_MASK);
	outb(INT2_CTL1, INT2_MASK);

	return (ret);
}

/*---------------------------------------------------------------------------*
 *			DMA Controller Define			 	     *
 *---------------------------------------------------------------------------*/
/* DMA Controller Registers */
#define	DMA_ADDR	0x004    /* port for low 16 bits of DMA address */
#define	DMA_LTOP	0x081    /* port for top low 8bit DMA addr(ch2) */
#define	DMA_HTOP	0x481    /* port for top high 8bit DMA addr(ch2) */
#define	DMA_COUNT	0x005    /* port for DMA count (count =  bytes - 1) */
#define	DMA_DEVCON	0x008    /* DMA device control register */
#define	DMA_SR		0x008    /* DMA status register */
#define	DMA_RESET	0x00D    /* DMA software reset register */
#define	DMA_FLIPFLOP	0x00C    /* DMA byte pointer flip-flop */
#define	DMA_MODE	0x00B    /* DMA mode port */
#define	DMA_INIT	0x00A    /* DMA init port */

#define	DMA_RESET_VAL	0x06
/* DMA channel commands. */
#define	DMA_READ	0x46    /* DMA read opcode */
#define	DMA_WRITE	0x4A    /* DMA write opcode */

/*===========================================================================*
 *				dma_setup				     *
 *===========================================================================*/
void
dma_setup(buf, size, func, chan)
	u_char *buf;
	int size;
	int func;
	int chan;
{
	u_long pbuf = local_to_PCI((u_long)buf);

#if 0
	outb(DMA_RESET, 0);
	DELAY(1);
	outb(DMA_DEVCON, 0x00);
	outb(DMA_INIT, DMA_RESET_VAL);	/* reset the dma controller */
#endif
	outb(DMA_MODE, func == F_READ ? DMA_READ : DMA_WRITE);
	outb(DMA_FLIPFLOP, 0);		/* write anything to reset it */

	outb(DMA_ADDR, (int)pbuf >>  0);
	outb(DMA_ADDR, (int)pbuf >>  8);
	outb(DMA_LTOP, (int)pbuf >> 16);
	outb(DMA_HTOP, (int)pbuf >> 24);

	outb(DMA_COUNT, (size - 1) >> 0);
	outb(DMA_COUNT, (size - 1) >> 8);
	outb(DMA_INIT, chan);		/* some sort of enable */
}

int
dma_finished(chan)
	int chan;
{
	return ((inb(DMA_SR) & 0x0f) == (1 << chan));
}
