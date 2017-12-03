/*	$NetBSD: cfl.c,v 1.20.18.2 2017/12/03 11:36:48 jdolecek Exp $	*/
/*-
 * Copyright (c) 1982, 1986 The Regents of the University of California.
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
 *	@(#)crl.c	7.5 (Berkeley) 5/9/91
 */

/*-
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
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
 *	@(#)crl.c	7.5 (Berkeley) 5/9/91
 */

/*
 * Console floppy driver for 11/780.
 *	XXX - Does not work. (Not completed)
 *	Included here if someone wants to finish it.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cfl.c,v 1.20.18.2 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#include <sys/buf.h>

#include <machine/sid.h>
#include <machine/scb.h>

#include <vax/vax/gencons.h>

#define	CFL_TRACKS	77
#define	CFL_SECTORS	26
#define	CFL_BYTESPERSEC	128
#define	CFL_MAXSEC	(CFL_TRACKS * CFL_SECTORS)

#define	FLOP_READSECT	0x900
#define	FLOP_WRITSECT	0x901
#define	FLOP_DATA	0x100
#define	FLOP_COMPLETE	0x200

struct {
	short	cfl_state;		/* open and busy flags */
	short	cfl_active;		/* driver state flag */
	struct	buf *cfl_buf;		/* buffer we're using */
	unsigned char *cfl_xaddr;		/* transfer address */
	short	cfl_errcnt;
} cfltab;

#define	IDLE	0
#define	OPEN	1
#define	BUSY	2

#define	CFL_IDLE	0
#define	CFL_START	1
#define	CFL_SECTOR	2
#define	CFL_DATA	3
#define	CFL_TRACK	4
#define	CFL_NEXT	5
#define	CFL_FINISH	6
#define	CFL_GETIN	7

static void cflstart(void);

static dev_type_open(cflopen);
static dev_type_close(cflclose);
static dev_type_read(cflrw);

const struct cdevsw cfl_cdevsw = {
	.d_open = cflopen,
	.d_close = cflclose,
	.d_read = cflrw,
	.d_write = cflrw,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

/*ARGSUSED*/
int
cflopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	if (vax_cputype != VAX_780)
		return (ENXIO);
	if (cfltab.cfl_state != IDLE)
		return (EALREADY);
	cfltab.cfl_state = OPEN;
	cfltab.cfl_buf = geteblk(512);
	return (0);
}

/*ARGSUSED*/
int
cflclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	int s;
	s = splbio();
	brelse(cfltab.cfl_buf, 0);
	splx(s);
	cfltab.cfl_state = IDLE;
	return 0;
}

/*ARGSUSED*/
int
cflrw(dev_t dev, struct uio *uio, int flag)
{
	struct buf *bp;
	int i;
	int s;
	int error;

	if (uio->uio_resid == 0) 
		return (0);
	s = splconsmedia();
	while (cfltab.cfl_state == BUSY)
		(void) tsleep(&cfltab, PRIBIO, "cflbusy", 0);
	cfltab.cfl_state = BUSY;
	splx(s);

	bp = cfltab.cfl_buf;
	error = 0;
	while ((i = imin(CFL_BYTESPERSEC, uio->uio_resid)) > 0) {
		bp->b_blkno = uio->uio_offset>>7;
		if (bp->b_blkno >= CFL_MAXSEC ||
		    (uio->uio_offset & 0x7F) != 0) {
			error = EIO;
			break;
		}
		if (uio->uio_rw == UIO_WRITE) {
			error = uiomove(bp->b_data, i, uio);
			if (error)
				break;
		}
		if (uio->uio_rw == UIO_WRITE) {
			bp->b_oflags &= ~(BO_DONE);
			bp->b_flags &= ~(B_READ);
			bp->b_flags |= B_WRITE;
		} else {
			bp->b_oflags &= ~(BO_DONE);
			bp->b_flags &= ~(B_WRITE);
			bp->b_flags |= B_READ;
		}
		s = splconsmedia();
		cflstart();
		biowait(bp);
		splx(s);
		if (bp->b_error != 0) {
			error = bp->b_error;
			break;
		}
		if (uio->uio_rw == UIO_READ) {
			error = uiomove(bp->b_data, i, uio);
			if (error)
				break;
		}
	}
	cfltab.cfl_state = OPEN;
	wakeup((void *)&cfltab);
	return (error);
}

void
cflstart(void)
{
	struct buf *bp;

	bp = cfltab.cfl_buf;
	cfltab.cfl_errcnt = 0;
	cfltab.cfl_xaddr = (unsigned char *) bp->b_data;
	cfltab.cfl_active = CFL_START;
	bp->b_resid = 0;

	if ((mfpr(PR_TXCS) & GC_RDY) == 0)
		/* not ready to receive order */
		return;

	cfltab.cfl_active = CFL_SECTOR;
	mtpr(bp->b_flags & B_READ ? FLOP_READSECT : FLOP_WRITSECT, PR_TXDB);

#ifdef lint
	cflintr();
#endif
}

void cfltint(int);

void
cfltint(int arg)
{
	struct buf *bp = cfltab.cfl_buf;

	switch (cfltab.cfl_active) {
	case CFL_START:/* do a read */
		mtpr(bp->b_flags & B_READ ? FLOP_READSECT : FLOP_WRITSECT,
		    PR_TXDB);
		cfltab.cfl_active = CFL_SECTOR;
		break;

	case CFL_SECTOR:/* send sector */
		mtpr(FLOP_DATA | (int)bp->b_blkno % (CFL_SECTORS + 1), PR_TXDB);
		cfltab.cfl_active = CFL_TRACK;
		break;

	case CFL_TRACK:
		mtpr(FLOP_DATA | (int)bp->b_blkno / CFL_SECTORS, PR_TXDB);
		cfltab.cfl_active = CFL_NEXT;
		break;

	case CFL_NEXT:
		mtpr(FLOP_DATA | *cfltab.cfl_xaddr++, PR_TXDB);
		if (--bp->b_bcount == 0)
			cfltab.cfl_active = CFL_FINISH;
		break;

	}
}

void cflrint(int);

void
cflrint(int ch)
{
	struct buf *bp = cfltab.cfl_buf;

	switch (cfltab.cfl_active) {
	case CFL_NEXT:
		if ((bp->b_flags & B_READ) == B_READ)
			cfltab.cfl_active = CFL_GETIN;
		else {
			cfltab.cfl_active = CFL_IDLE;
			mutex_enter(bp->b_objlock);
			bp->b_oflags |= BO_DONE;
			cv_broadcast(&bp->b_done);
			mutex_exit(bp->b_objlock);
		}
		break;

	case CFL_GETIN:
		*cfltab.cfl_xaddr++ = ch & 0377;
		if (--bp->b_bcount==0) {
			cfltab.cfl_active = CFL_IDLE;
			mutex_enter(bp->b_objlock);
			bp->b_oflags |= BO_DONE;
			cv_broadcast(&bp->b_done);
			mutex_exit(bp->b_objlock);
		}
		break;
	}
}
