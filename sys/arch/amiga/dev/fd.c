/*
 * Copyright (c) 1994 Brad Pepers
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Brad Pepers.
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
 */

/*
 * floppy interface
 */

#include "fd.h"
#if NFD > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/dkstat.h>
#include <sys/disklabel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#include <amiga/dev/device.h>
#include <amiga/amiga/cia.h>
#include <amiga/amiga/custom.h>

#define UNIT(x)		(minor(x) & 3)
#define	b_cylin		b_resid
#define FDBLK 512
#define MAX_SECTS 22
#define IMMED_WRITE 0

int fdattach();
struct	driver fddriver = {
	fdattach, "fd",
};

/* defines */
#define MFM_SYNC	0x4489
#define DSKLEN_DMAEN    (1<<15)
#define DSKLEN_WRITE    (1<<14)

/* drive type values */
#define FD_NONE		0x00000000
#define FD_DD_3		0xffffffff	/* double-density 3.5" (880K) */
#define FD_HD_3		0x55555555	/* high-density 3.5" (1760K) */

struct fd_type {
	int id;
	char *name;
	int tracks;
	int heads;
	int read_size;
	int write_size;
	int sect_mult;
	int precomp1;
	int precomp2;
	int step_delay;
	int side_time;
	int settle_time;
};

struct fd_type drive_types[] = {
/*	    id       name      tr he  rdsz   wrsz  sm pc1  pc2 sd  st st  */
	{ FD_DD_3, "DD 3.5", 80, 2, 14716, 13630, 1, 80, 161, 3, 18, 1 },
	{ FD_HD_3, "HD 3.5", 80, 2, 29432, 27260, 2, 80, 161, 3, 18, 1 },
	{ FD_NONE, "No Drive", 0, }
};
int num_dr_types = sizeof(drive_types) / sizeof(drive_types[0]);

/*
 * Per drive structure.
 * N per controller (presently 4) (DRVS_PER_CTLR)
 */
#define DRVS_PER_CTLR 4
struct fd_data {
	int fdu;		/* This unit number */
	struct buf head;	/* Head of buf chain      */
	struct buf rhead;	/* Raw head of buf chain  */
	int type;		/* Drive type */
	struct fd_type *ft;	/* Pointer to type descriptor */
	int flags;
#define	FDF_OPEN	0x01	/* it's open		*/
	int skip;
	int sects;		/* number of sectors in a track */
	int size;		/* size of disk in sectors */
	int side;		/* current side disk is on */
	int dir;		/* current direction of stepping */
	int cyl;		/* current cylinder disk is on */
	int buf_track;
	int buf_dirty;
	char *buf_data;
	char *buf_labels;
	int write_cnt;
};

/*
 * Per controller structure.
 */
struct fdc_data
{
	int fdcu;		/* our unit number */
	struct fd_data *fd;	/* drive we are currently doing work for */
	int motor_fdu;	/* drive that has its motor on */
	int state;
	int saved;
	int retry;
	struct fd_data fd_data[DRVS_PER_CTLR];
};
struct fdc_data fdc_data[NFD];

/*
 * Throughout this file the following conventions will be used:
 *
 * fd is a pointer to the fd_data struct for the drive in question
 * fdc is a pointer to the fdc_data struct for the controller
 * fdu is the floppy drive unit number
 * fdcu is the floppy controller unit number
 * fdsu is the floppy drive unit number on that controller. (sub-unit)
 */
typedef int	fdu_t;
typedef int	fdcu_t;
typedef int	fdsu_t;
typedef	struct fd_data *fd_p;
typedef struct fdc_data *fdc_p;

/*
 * protos.
 */
static int delay __P((int));
void encode __P((u_long, u_long *, u_long *));
void fd_step __P((void));
void fd_seek __P((fd_p, int));
void correct __P((u_long *));
void fd_probe __P((fd_p));
void fd_turnon __P((fdc_p, fdu_t));
void fd_turnoff __P((fdc_p));
void track_read __P((fdc_p, fd_p, int));
void fd_timeout __P((fdc_p));
void fd_motor_to __P((fdcu_t));
void fd_motor_on __P((fdc_p, fdu_t));
void track_write __P((fdc_p, fd_p));
void amiga_write __P((fd_p));
void fd_calibrate __P((fd_p));
void encode_block __P((u_long *, u_char *, int, u_long *));
void fd_select_dir __P((fd_p, int));
void fd_pseudointr __P((fdc_p));
void fd_select_side __P((fd_p, int));

u_long scan_sync __P((u_long, u_long, int));
u_long encode_long __P((u_long, u_long *));
u_long loop_read_id __P((int));
u_long get_drive_id __P((int));

int fdstate __P((fdc_p));
int retrier __P((fdc_p));
int amiga_read __P((fd_p));
int get_drive_type __P((u_long));

/* device routines */
int Fdopen __P((dev_t, int));
int fdsize __P((dev_t));
int fdioctl __P((dev_t, int, caddr_t, int, struct proc *));
int fdclose __P((dev_t, int));
int fdattach __P((struct amiga_device *));

void fdintr __P((fdcu_t));
void fdstart __P((fdc_p));
void fdstrategy __P((struct buf *bp));

#define DEVIDLE		0
#define FINDWORK	1
#define	DOSEEK		2
#define DO_IO	 	3
#define	DONE_IO		4
#define	WAIT_READ	5
#define	WAIT_WRITE	6
#define DELAY_WRITE	7
#define RECALCOMPLETE	8
#define	STARTRECAL	9
#define	RESETCTLR	10
#define	SEEKWAIT	11
#define	RECALWAIT	12
#define	MOTORWAIT	13

#undef DEBUG

#ifdef DEBUG 

char *fdstates[] =
{
"DEVIDLE",
"FINDWORK",
"DOSEEK",
"DO_IO",
"DONE_IO",
"WAIT_READ",
"WAIT_WRITE",
"DELAY_WRITE",
"RECALCOMPLETE",
"STARTRECAL",
"RESETCTLR",
"SEEKWAIT",
"RECALWAIT",
"MOTORWAIT",
};

#define TRACE0(arg) if (fd_debug == 1) printf(arg)
#define TRACE1(arg1,arg2) if (fd_debug == 1) printf(arg1,arg2)

#else	/* !DEBUG */

#define TRACE0(arg)
#define TRACE1(arg1,arg2)

#endif	/* !DEBUG */

extern int hz;

unsigned char *raw_buf = NULL;
#ifdef DEBUG
int fd_debug = 1;
#else
int fd_debug = 0;
#endif

/*
 * Floppy Support Routines
 */
#define MOTOR_ON		(ciab.prb &= ~CIAB_PRB_MTR)
#define MOTOR_OFF		(ciab.prb |= CIAB_PRB_MTR)
#define SELECT(mask)		(ciab.prb &= ~mask)
#define DESELECT(mask)		(ciab.prb |= mask)
#define SELMASK(drive)		(1 << (3 + (drive & 3)))

/*
 * Delay for a number of milliseconds
 *	- tried ciab.tod but seems to miss values and screw up
 *	- stupid busy loop for now
 */
static int
delay(delay_ms)
	int delay_ms;
{
	long cnt, inner;
	int val;

	for (cnt = 0; cnt < delay_ms; cnt++)
		for (inner = 0; inner < 500; inner++)
			val += inner * cnt;
	return(val);
}

/*
 * motor control stuff
 */
void
fd_motor_to(fdcu)
	fdcu_t fdcu;
{
	printf("timeout starting motor\n");	/* XXXX */
	fdc_data[fdcu].motor_fdu = -2;
}

void
fd_motor_on(fdc, fdu)
	fdc_p fdc;
	fdu_t fdu;
{
	int i;
	int cnt;		/* XXXX not used? */

	cnt = 0;		/* XXXX not used? */

	/* deselect all drives */
	for (i = 0; i < DRVS_PER_CTLR; i++)
		DESELECT(SELMASK(i));

	/* turn on the unit's motor */
	MOTOR_ON;
	SELECT(SELMASK(fdu));

	timeout((timeout_t)fd_motor_to, (caddr_t)fdc->fdcu, hz);
	while (ciaa.pra & CIAA_PRA_RDY)
		;
	untimeout((timeout_t)fd_motor_to, (caddr_t)fdc->fdcu);
	fdc->motor_fdu = fdu;
}

void
fd_turnoff(fdc)
	fdc_p fdc;
{
	int i;

	if (fdc->motor_fdu != -1) {
		/* deselect all drives */
		for (i = 0; i < DRVS_PER_CTLR; i++)
			DESELECT(SELMASK(i));

		/* turn off the unit's motor */
		MOTOR_OFF;
		SELECT(SELMASK(fdc->motor_fdu));
		MOTOR_ON;
		DESELECT(SELMASK(fdc->motor_fdu));
	}

	fdc->motor_fdu = -1;
}

void
fd_turnon(fdc, fdu)
	fdc_p fdc;
	fdu_t fdu;
{
	if (fdc->motor_fdu == fdu)
		return;

	fd_turnoff(fdc);
	fd_motor_on(fdc, fdu);
}

/*
 * Step the drive once in its current direction
 */
void
fd_step()
{
	ciab.prb &= ~CIAB_PRB_STEP;
	ciab.prb |= CIAB_PRB_STEP;
}

/*
 * Select the side to use for a particular drive.
 * The drive must have been calibrated at some point before this.
 * The drive must also be active and the motor must be running.
 */
void
fd_select_side(fd, side)
	fd_p fd;
	int side;
{
	if (fd->side == side)
		return;

	/* select the requested side */
	if (side == 0)
		ciab.prb &= ~CIAB_PRB_SIDE;
	else
		ciab.prb |= CIAB_PRB_SIDE;
	delay(fd->ft->side_time);
	fd->side = side;
}

/*
 * Select the direction to use for the current particular drive.
 */
void
fd_select_dir(fd, dir)
	fd_p fd;
	int dir;
{
	if (fd->dir == dir)
		return;

	/* select the requested direction */
	if (dir == 0)
		ciab.prb &= ~CIAB_PRB_DIR;
	else
		ciab.prb |= CIAB_PRB_DIR;
	delay(fd->ft->settle_time);
	fd->dir = dir;
}

/*
 * Seek the drive to track 0.
 * The drive must be active and the motor must be running.
 * Returns standard floppy error code. /* XXXX doesn't return anything
 */
void
fd_calibrate(fd)
	fd_p fd;
{
	fd_select_dir(fd, 1);

	/* loop until we hit track 0 */
	while (ciaa.pra & CIAA_PRA_TK0) {
		fd_step();
		delay(4);
	}

	/* set known values */
	fd->cyl = 0;
}

/*
 * Seek the drive to the requested track.
 * The drive must be active and the motor must be running.
 */
void
fd_seek(fd, track)
	fd_p fd;
	int track;
{
	int cyl, side;
	int dir, cnt;
	int delay_time;

	cyl = track >> 1;
	side = (track % 2) ^ 1;

	if (fd->cyl == -1)
		fd_calibrate(fd);

	fd_select_side(fd, side);

	if (cyl < fd->cyl) {
		dir = 1;
		cnt = fd->cyl - cyl;
	} else {
		dir = 0;
		cnt = cyl - fd->cyl;
	}

	fd_select_dir(fd, dir);

	if (cnt) {
		while (cnt) {
			delay_time = fd->ft->step_delay;
			if (dir != fd->dir)
				delay_time += fd->ft->settle_time;
			fd_step();
			delay(delay_time);
			--cnt;
		}
		delay(fd->ft->settle_time);
	}

	fd->cyl = cyl;
}

void
encode(data, dest, csum)
	u_long data;
	u_long *dest, *csum;
{
	u_long data2;

	data &= 0x55555555;
	data2 = data ^ 0x55555555;
	data |= ((data2 >> 1) | 0x80000000) & (data2 << 1);

	if (*(dest - 1) & 0x00000001)
		data &= 0x7FFFFFFF;

	*csum ^= data;
	*dest = data;
}

u_long
encode_long(data, dest)
	u_long data;
	u_long *dest;
{
	u_long csum;

	csum = 0;

	encode(data >> 1, dest, &csum);
	encode(data, dest + 1, &csum);

	return(csum & 0x55555555);
}

void
encode_block(dest, from, len, csum)
	u_long *dest, *csum;
	u_char *from;
	int len;
{
	int cnt, to_cnt = 0;
	u_long data, *src;

	to_cnt = 0;
	src = (u_long *)from;

	/* odd bits */
	for (cnt = 0; cnt < len / 4; cnt++) {
		data = src[cnt] >> 1;
		encode(data, dest + to_cnt++, csum);
	}

	/* even bits */
	for (cnt = 0; cnt < len / 4; cnt++) {
		data = src[cnt];
		encode(data, dest + to_cnt++, csum);
	}

	*csum &= 0x55555555;
}

void
correct(raw)
	u_long *raw;
{
	u_char data, *ptr;

	ptr = (u_char *)raw;

	data = *ptr;
	if (*(ptr - 1) & 0x01) {	/* XXXX will choke on old GVP's */
		*ptr = data & 0x7f;
		return;
	}

	if (data & 0x40) 
		return;

	*ptr |= 0x80;
}

/*
 * amiga_write converts track/labels data to raw track data
 */
void
amiga_write(fd)
	fd_p fd;
{
	u_long *raw, csum, format;
	u_char *data, *labels;
	int cnt, track;

	raw = (u_long *)raw_buf;	/* XXXX never used while intr? */
					/* XXXX never waits after here? */
	data = fd->buf_data;
	labels = fd->buf_labels;
	track = fd->buf_track;

	/* gap space */
	for (cnt = 0; cnt < 414; cnt++)
		*raw++ = 0xaaaaaaaa;

	/* sectors */
	for (cnt = 0; cnt < 11; cnt++) {
		*raw = 0xaaaaaaaa;
		correct(raw);
		++raw;

		*raw++ = 0x44894489;

		format = 0xff000000 | (track << 16) | (cnt << 8) | (11 - cnt);
		csum = encode_long(format,raw);
		raw += 2;

		encode_block(raw, labels + cnt * 16, 16, &csum);
		raw += 8;
		csum = encode_long(csum, raw);
		raw += 2;

		csum = 0;
		encode_block(raw+2, data + cnt * 512, 512, &csum);
		csum = encode_long(csum, raw);
		raw += 256 + 2;
	}
}

#define get_word(raw) (*(u_short *)(raw))
#define get_long(raw) (*(u_long *)(raw))

#define decode_long(raw) \
    (((get_long(raw) & 0x55555555) << 1) | \
    (get_long((raw)+4) & 0x55555555))

#define MFM_NOSYNC	1
#define MFM_HEADER	2
#define MFM_DATA	3
#define MFM_TRACK	4

/*
 * scan_sync - looks for the next start of sector marked by a sync. When
 *	sector = 10, can't be certain of a starting sync.
 */
u_long
scan_sync(raw, end, sect)
	u_long raw, end;
	int sect;
{
	u_short data;

	if (sect != 10) {
		while (raw < end) {
			data = get_word(raw);
			if (data == 0x4489)
				break;
			raw += 2;
		}
		if (raw > end)
			return(0);
	}

	while (raw < end) {
		data = get_word(raw);
		if (data != 0x4489)
			break;
		raw += 2;
	}
	if (raw > end)
		return(0);
	return(raw);
}

/*
 * amiga_read reads a raw track of data into a track buffer
 */
int
amiga_read(fd)
	fd_p fd;
{
	u_char *track_data, *label_data;
	u_long raw, end, val1, val2, csum, data_csum;
	u_long *data, *labels;
	int scnt, cnt, format, tnum, sect, snext;

	track_data = fd->buf_data;
	label_data = fd->buf_labels;
	raw = (u_long)raw_buf;		/* XXXX see above about glb */

	end = raw + fd->ft->read_size;

	for (scnt = fd->sects-1; scnt >= 0; scnt--) {
		if ((raw = scan_sync(raw, end, scnt)) == 0) {
			/* XXXX */
			printf("can't find sync for sector %d\n", scnt);
			return(1);
		}

		val1 = decode_long(raw);

		format = (val1 >> 24) & 0xFF;
		tnum   = (val1 >> 16) & 0xFF;
		sect   = (val1 >>  8) & 0xFF;
		snext  = (val1)       & 0xFF;

		labels = (u_long *)(label_data + (sect << 4));

		csum = 0;
		val1 = get_long(raw);
		raw += 4;
		csum ^= val1;
		val1 = get_long(raw);
		raw += 4;
		csum ^= val1;

		for (cnt = 0; cnt < 4; cnt++) {
			val1 = get_long(raw+16);
			csum ^= val1;
			val1 &= 0x55555555;
			val2 = get_long(raw);
			raw += 4;
			csum ^= val2;
			val2 &= 0x55555555;
			val2 = val2 << 1;
			val1 |= val2;
			*labels++ = val1;
		}

		csum &= 0x55555555;
		raw += 16;
		val1 = decode_long(raw);
		raw += 8;
		if (val1 != csum) {
			/* XXXX */
			printf("MFM_HEADER %d: %08x,%08x\n", scnt,
			    val1, csum);
			return(MFM_HEADER);
		}

		/* verify track */
		if (tnum != fd->buf_track) {
			/* XXXX */
			printf("MFM_TRACK %d: %d, %d\n", scnt, tnum,
			    fd->buf_track);
			return(MFM_TRACK);
		}

		data_csum = decode_long(raw);
		raw += 8;
		data = (u_long *)(track_data + (sect << 9));

		csum = 0;
		for (cnt = 0; cnt < 128; cnt++) {
			val1 = get_long(raw + 512);
			csum ^= val1;
			val1 &= 0x55555555;
			val2 = get_long(raw);
			raw += 4;
			csum ^= val2;
			val2 &= 0x55555555;
			val2 = val2 << 1;
			val1 |= val2;
			*data++ = val1;
		}

		csum &= 0x55555555;
		raw += 512;

		if (data_csum != csum) {
			printf(
			    "MFM_DATA: f=%d t=%d s=%d sn=%d sc=%d %ld, %ld\n", 
			    format, tnum, sect, snext, scnt, data_csum, csum);
			return(MFM_DATA);
		}
	}
	return(0);
}

/*
 * Return unit ID number of given disk
 * XXXX This function doesn't return anything.
 */
u_long
loop_read_id(unit)
	int unit;
{
	u_long id;
	int i;

	id = 0;

	/* loop and read disk ID */
	for (i = 0; i < 32; i++) {
		SELECT(SELMASK(unit));

		/* read and store value of DSKRDY */
		id <<= 1;		/* XXXX 0 << 1? */
		id |= (ciaa.pra & CIAA_PRA_RDY) ? 0 : 1;

		DESELECT(SELMASK(unit));
	}
}

u_long
get_drive_id(unit)
	int unit;
{
	int i, t;
	u_long id;
	u_char mask1, mask2;
	volatile u_char *a_ptr;
	volatile u_char *b_ptr;

	id = 0;
	a_ptr = &ciaa.pra;
	b_ptr = &ciab.prb;
	mask1 = ~(1 << (3 + unit));
	mask2 = 1 << (3 + unit);

	*b_ptr &= ~CIAB_PRB_MTR;
	*b_ptr &= mask1;
	*b_ptr |= mask2;
	*b_ptr |= CIAB_PRB_MTR;
	*b_ptr &= mask1;
	*b_ptr |= mask2;

	for (i = 0; i < 32; i++) {
		*b_ptr &= mask1;
		t = (*a_ptr) & CIAA_PRA_RDY;
		id = (id << 1) | (t ? 0 : 1);
		*b_ptr |= mask2;
	}
	return(id);
#if 0
  /* set up for ID */
  MOTOR_ON;
  SELECT(SELMASK(unit));
  DESELECT(SELMASK(unit));
  MOTOR_OFF;
  SELECT(SELMASK(unit));
  DESELECT(SELMASK(unit));

  return loop_read_id(unit); /* XXXX gotta fix loop_read_id() if use */
#endif
}

int
get_drive_type(u_long id)
{
	int type;

	for (type = 0; type < num_dr_types; type++)
		if (drive_types[type].id == id)
			return(type);
	return(-1);
}

void
fd_probe(fd)
	fd_p fd;
{
	u_long id;
	int type, data;

	fd->ft = NULL;

	id = get_drive_id(fd->fdu);
	type = get_drive_type(id);

	if (type == -1) {
		/* XXXX */
		printf("fd_probe: unsupported drive type %08x found\n", id);
		return;
	}

	fd->type = type;
	fd->ft = &drive_types[type];
	if (fd->ft->tracks == 0) {
		/* XXXX */
		printf("no drive type %d\n", type);
	}
	fd->side = -1;
	fd->dir = -1;
	fd->cyl = -1;

	fd->sects = 11 * drive_types[type].sect_mult;
	fd->size = fd->sects * 
	    drive_types[type].tracks * 
	    drive_types[type].heads;
	fd->flags = 0;
}

void
track_read(fdc, fd, track)
	fdc_p fdc;
	fd_p fd;
	int track;
{
	u_long len;

	fd->buf_track = track;
	fdc->state = WAIT_READ;
	timeout((timeout_t)fd_timeout, (caddr_t)fdc, 2 * hz);

	fd_seek(fd, track);

	len = fd->ft->read_size >> 1;

	/* setup adkcon bits correctly */
	custom.adkcon = ADKF_MSBSYNC;
	custom.adkcon = ADKF_SETCLR | ADKF_WORDSYNC | ADKF_FAST;

	custom.dsksync = MFM_SYNC;

	custom.dsklen = 0;
	delay(fd->ft->side_time);

	custom.dskpt = (u_char *)kvtop(raw_buf);
	custom.dsklen = len | DSKLEN_DMAEN;
	custom.dsklen = len | DSKLEN_DMAEN;
}

void
track_write(fdc, fd)
	fdc_p fdc;
	fd_p fd;
{
	int track;
	u_long len;
	u_short adk;

	amiga_write(fd);

	track = fd->buf_track;
	fd->write_cnt += 1;

	fdc->saved = fdc->state;
	fdc->state = WAIT_WRITE;
	timeout((timeout_t)fd_timeout, (caddr_t)fdc, 2 * hz);

	fd_seek(fd, track);

	len = fd->ft->write_size >> 1;

	if ((ciaa.pra & CIAA_PRA_WPRO) == 0)
		return;

	/* clear adkcon bits */
	custom.adkcon = ADKF_PRECOMP1 | ADKF_PRECOMP0 | ADKF_WORDSYNC |
	    ADKF_MSBSYNC;

	/* set appropriate adkcon bits */
	adk = ADKF_SETCLR | ADKF_FAST;
	if (track >= fd->ft->precomp2)
		adk |= ADKF_PRECOMP1;
	else if (track >= fd->ft->precomp1)
		adk |= ADKF_PRECOMP0;
	custom.adkcon = adk;

	custom.dsklen = DSKLEN_WRITE;
	delay(fd->ft->side_time);

	custom.dskpt = (u_char *)kvtop(raw_buf);	/* XXXX again raw */
	custom.dsklen = len | DSKLEN_DMAEN | DSKLEN_WRITE;
	custom.dsklen = len | DSKLEN_DMAEN | DSKLEN_WRITE;
}

/*
 * Floppy Device Code
 */
int
fdattach(ad)
	struct amiga_device *ad;
{
	int fdcu = 0;
	fdc_p fdc = fdc_data + fdcu;
	int i;
	unsigned long id;
	int type;

	fdc->fdcu = fdcu;
	fdc->state = FINDWORK;
	fdc->fd = NULL;
	fdc->motor_fdu = -1;

	for (i = 0; i < DRVS_PER_CTLR; i++) {
		fdc->fd_data[i].fdu = i;
		fdc->fd_data[i].flags = 0;

		fdc->fd_data[i].buf_track = -1;
		fdc->fd_data[i].buf_dirty = 0;
		fdc->fd_data[i].buf_data = 
		    malloc(MAX_SECTS * 512, M_DEVBUF, 0);
		fdc->fd_data[i].buf_labels =
		    malloc(MAX_SECTS * 16, M_DEVBUF, 0);

		if (fdc->fd_data[i].buf_data == NULL ||
		    fdc->fd_data[i].buf_labels == NULL) {
			printf("Cannot alloc buffer memory for fd device\n");
			return(0);
		}

		id = get_drive_id(i);
		type = get_drive_type(id);

		if (type != -1 && drive_types[type].tracks != 0) {
			printf("floppy drive %d: %s\n", i, 
			    drive_types[type].name);
		}
	}

	raw_buf = (char *)alloc_chipmem(30000);
	if (raw_buf == NULL) {
		printf("Cannot alloc chipmem for fd device\n");
		return 0;
	}

	/* enable disk DMA */
	custom.dmacon = DMAF_SETCLR | DMAF_DISK;

	/* enable interrupts for IRQ_DSKBLK */
	ciaa.icr = CIA_ICR_IR_SC | CIA_ICR_FLG;
	custom.intena = INTF_SETCLR | INTF_SOFTINT;

	/* enable disk block interrupts */
	custom.intena = INTF_SETCLR | INTF_DSKBLK;

	return(1);
}

int
Fdopen(dev, flags)
	dev_t dev;
	int flags;
{
	fdcu_t fdcu;
	fdc_p fdc;
	fdu_t fdu;
	fd_p fd;

	fdcu = 0;
	fdc = fdc_data + fdcu;
	fdu = UNIT(dev);
	fd = fdc->fd_data + fdu;

	/* check bounds */
	if (fdu >= DRVS_PER_CTLR) 
		return(ENXIO);

	fd_probe(fd);
	if (fd->ft == NULL || fd->ft->tracks == 0)
		return(ENXIO);

	fd->flags |= FDF_OPEN;
	fd->write_cnt = 0;

	return(0);
}

int
fdclose(dev, flags)
	dev_t dev;
	int flags;
{
	struct buf *dp,*bp;
	fdcu_t fdcu;
	fdc_p fdc;
	fdu_t fdu;
	fd_p fd;

	fdcu = 0;
	fdc = fdc_data + fdcu;
	fdu = UNIT(dev);
	fd = fdc->fd_data + fdu;


	/* wait until activity is done for this drive */
	/* XXXX ACK! sleep.. */
	do {
		dp = &(fd->head);
		bp = dp->b_actf;
	} while (bp);

	/* XXXX */
	printf("wrote %d tracks (%d)\n", fd->write_cnt, fd->buf_dirty);

	fd->buf_track = -1;
	fd->buf_dirty = 0;
	fd->flags &= ~FDF_OPEN;

	return(0);
}

int
fdioctl(dev, cmd, data, flag, p)
	dev_t dev;
	int cmd, flag;
	caddr_t data;
	struct proc *p;
{
	struct disklabel *fd_label;
	fdcu_t fdcu;
	fdc_p fdc;
	fdu_t fdu;
	fd_p fd;
	int error;

	fdcu = 0;
	fdc = fdc_data + fdcu;
	fdu = UNIT(dev);
	fd = fdc->fd_data + fdu;
	error = 0;

	if (cmd != DIOCGDINFO)
		return (EINVAL);

	fd_label = (struct disklabel *)data;

	bzero(fd_label, sizeof(fd_label));
	fd_label->d_magic = DISKMAGIC;
	fd_label->d_type = DTYPE_FLOPPY;
	strncpy(fd_label->d_typename, "fd", sizeof(fd_label->d_typename) - 1);
	strcpy(fd_label->d_packname, "some pack");

	fd_label->d_secsize = 512;
	fd_label->d_nsectors = 11;
	fd_label->d_ntracks = 2;
	fd_label->d_ncylinders = 80;
	fd_label->d_secpercyl = fd_label->d_nsectors * fd_label->d_ntracks;
	fd_label->d_secperunit= fd_label->d_ncylinders * fd_label->d_secpercyl;

	fd_label->d_magic2 = DISKMAGIC;
	fd_label->d_partitions[0].p_offset = 0;
	fd_label->d_partitions[0].p_size = fd_label->d_secperunit;
	fd_label->d_partitions[0].p_fstype = FS_UNUSED;
	fd_label->d_npartitions = 1;

	fd_label->d_checksum = 0;
	fd_label->d_checksum = dkcksum(fd_label);

	return(0);
}

int
fdsize(dev)
	dev_t dev;
{
	/* check UNIT? */
	return((fdc_data + 0)->fd_data[UNIT(dev)].size);
}

void
fdstrategy(bp)
	struct buf *bp;
{
	fdcu_t fdcu;
	fdc_p fdc;
	fdu_t fdu;
	fd_p fd;
	long nblocks, blknum;
	struct buf *dp;
	int s;
	
	fdcu = 0;
	fdc = fdc_data + fdcu;
	fdu = UNIT(bp->b_dev);
	fd = fdc->fd_data + fdu;

	if (bp->b_blkno < 0) {
		/* XXXX */
		printf("fdstrat error: fdu = %d, blkno = %d, bcount = %d\n",
		    fdu, bp->b_blkno, bp->b_bcount);
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}

	/*
	 * Set up block calculations.
	 */
	blknum = (unsigned long) bp->b_blkno * DEV_BSIZE / FDBLK;
	nblocks = fd->sects * fd->ft->tracks * fd->ft->heads;
	if (blknum + (bp->b_bcount / FDBLK) > nblocks) {
		/* XXXX */
		printf("at end of disk\n");
		bp->b_error = ENOSPC;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}

	bp->b_cylin = blknum;	/* set here for disksort */
	dp = &(fd->head);

	s = splbio();
	disksort(dp, bp);
	untimeout((timeout_t)fd_turnoff, (caddr_t)fdc); /* a good idea */
	fdstart(fdc);
	splx(s);
}

/*
 * We have just queued something.. if the controller is not busy
 * then simulate the case where it has just finished a command
 * So that it (the interrupt routine) looks on the queue for more
 * work to do and picks up what we just added.
 * If the controller is already busy, we need do nothing, as it
 * will pick up our work when the present work completes
 */
void
fdstart(fdc)
	fdc_p fdc;
{
	int s;

	s = splbio();
	if (fdc->state == FINDWORK)
		fdintr(fdc->fdcu);
	splx(s);
}

/* 
 * just ensure it has the right spl
 */
void
fd_pseudointr(fdc)
	fdc_p fdc;
{
	int s;

	s = splbio();
	fdintr(fdc->fdcu);
	splx(s);
}

void
fd_timeout(fdc)
	fdc_p fdc;
{
	struct buf *dp,*bp;
	fd_p fd;

	fd = fdc->fd;
	dp = &fd->head;
	bp = dp->b_actf;

	/* XXXX */
	printf("fd%d: Operation timeout\n", fd->fdu);
	if (bp) {
		retrier(fdc);
		fdc->state = DONE_IO;
		if (fdc->retry < 6)
			fdc->retry = 6;
	} else {
		fdc->fd = NULL;
		fdc->state = FINDWORK;
	}

	fd_pseudointr(fdc);
}

/*
 * keep calling the state machine until it returns a 0
 * ALWAYS called at SPLBIO
 */
void
fdintr(fdcu)
	fdcu_t fdcu;
{
	fdc_p fdc;

	fdc = fdc_data + fdcu;
	while (fdstate(fdc))
		;
}

/*
 * The controller state machine.
 * if it returns a non zero value, it should be called again immediatly
 */
int
fdstate(fdc)
	fdc_p fdc;
{
	struct buf *dp,*bp;
	int track, read, sec, i;
	u_long blknum;
	fd_p fd;

	fd = fdc->fd;

	if (fd == NULL) {
		/* search for a unit do work with */
		for (i = 0; i < DRVS_PER_CTLR; i++) {
			fd = fdc->fd_data + i;
			dp = &(fd->head);
			bp = dp->b_actf;
			if (bp) {
				fdc->fd = fd;
				break;
			}
		}

		if (fdc->fd)
			return(1);

		fdc->state = FINDWORK;
		TRACE1("[fdc%d IDLE]\n", fdc->fdcu);
		return(0);
	}

	dp = &(fd->head);
	bp = dp->b_actf;

	blknum = (u_long)bp->b_blkno * DEV_BSIZE / FDBLK + fd->skip / FDBLK;
	track = blknum / fd->sects;
	sec = blknum % fd->sects;

	read = bp->b_flags & B_READ;
	TRACE1("fd%d", fd->fdu);
	TRACE1("[%s]", fdstates[fdc->state]);
	TRACE1("(0x%x) ", fd->flags);
	TRACE1("%d\n", fd->buf_track);

	untimeout((timeout_t)fd_turnoff, (caddr_t)fdc);
	timeout((timeout_t)fd_turnoff, (caddr_t)fdc, 4 * hz);

	switch (fdc->state) {
	case FINDWORK:
		if (!bp) {
			if (fd->buf_dirty) {
				track_write(fdc, fd);
				return(0);
			}
			fdc->fd = NULL;
			return(1);
		}

		fdc->state = DOSEEK;
		fdc->retry = 0;
		fd->skip = 0;
		return(1);
	case DOSEEK:
		fd_turnon(fdc, fd->fdu);

		/*
		 * If not started, error starting it
		 */
		if (fdc->motor_fdu != fd->fdu) {
			/* XXXX */
			printf("motor not on!\n");
		}

		/*
		 * If track not in buffer, read it in
		 */
		if (fd->buf_track != track) {
			TRACE1("do track %d\n", track);

			if (fd->buf_dirty)
				track_write(fdc, fd);
			else
				track_read(fdc, fd, track);
			return(0);
		}

		fdc->state = DO_IO;
		return(1);
	case DO_IO:
		if (read) 
			bcopy(&fd->buf_data[sec * FDBLK], 
			    bp->b_un.b_addr + fd->skip, FDBLK);
		else {
			bcopy(bp->b_un.b_addr + fd->skip, 
			    &fd->buf_data[sec * FDBLK], FDBLK);
			fd->buf_dirty = 1;
			if (IMMED_WRITE) {
				fdc->state = DONE_IO;
				track_write(fdc, fd);
				return(0);
			}
		}
	case DONE_IO:
		fd->skip += FDBLK;
		if (fd->skip < bp->b_bcount)
			fdc->state = DOSEEK;
		else {
			fd->skip = 0;
			bp->b_resid = 0;
			dp->b_actf = bp->b_actf;
			biodone(bp);
			fdc->state = FINDWORK;
		}
		return(1);
	case WAIT_READ:
		untimeout((timeout_t)fd_timeout, (caddr_t)fdc);
		custom.dsklen = 0;
		amiga_read(fd);
		fdc->state = DO_IO;
		return(1);
	case WAIT_WRITE:
		untimeout((timeout_t)fd_timeout, (caddr_t)fdc);
		custom.dsklen = 0;
		fdc->state = fdc->saved;
		fd->buf_dirty = 0;
		return(1);
	default:
		/* XXXX */
		printf("Unexpected FD int->%d\n", fdc->state);
		return 0;
	}

	/* Come back immediatly to new state */
	return(1);
}

int
retrier(fdc)
	fdc_p fdc;
{
	struct buf *dp,*bp;
	fd_p fd;

	fd = fdc->fd;
	dp = &(fd->head);
	bp = dp->b_actf;

#if 0
	switch(fdc->retry) {
	case 0: 
	case 1:
	case 2:
		fdc->state = SEEKCOMPLETE;
		break;
	case 3:
	case 4:
	case 5:
		fdc->state = STARTRECAL;
		break;
	case 6:
		fdc->state = RESETCTLR;
		break;
	case 7:
		break;
	default:
#endif
	/* XXXX */
	printf("fd%d: hard error\n", fd->fdu);

	bp->b_flags |= B_ERROR;
	bp->b_error = EIO;
	bp->b_resid = bp->b_bcount - fd->skip;
	dp->b_actf = bp->b_actf;
	fd->skip = 0;
	biodone(bp);
	fdc->state = FINDWORK;
	return(1);
#if 0
	fdc->retry++;
	return(1);
#endif
}

#endif
