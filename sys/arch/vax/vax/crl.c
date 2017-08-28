/*	$NetBSD: crl.c,v 1.32.4.2 2017/08/28 17:51:55 skrll Exp $	*/
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

/*
 * Bugfix by Johnny Billquist 2010
 */

/*
 * TO DO (tef  7/18/85):
 *	1) change printf's to log() instead???
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: crl.c,v 1.32.4.2 2017/08/28 17:51:55 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/buf.h>

#include <machine/sid.h>
#include <machine/scb.h>

#include <vax/vax/crl.h>

struct {
	short	crl_state;		/* open and busy flags */
	short	crl_active;		/* driver state flag */
	struct	buf *crl_buf;		/* buffer we're using */
	ushort *crl_xaddr;		/* transfer address */
	short	crl_errcnt;
} crltab;

struct {
	int	crl_cs;		/* saved controller status */
	int	crl_ds;		/* saved drive status */
} crlstat;

void	crlintr(void *);
void	crlattach(void);

static void crlstart(void);
static dev_type_open(crlopen);
static dev_type_close(crlclose);
static dev_type_read(crlrw);

const struct cdevsw crl_cdevsw = {
	.d_open = crlopen,
	.d_close = crlclose,
	.d_read = crlrw,
	.d_write = crlrw,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

struct evcnt crl_ev = EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "crl", "intr");
EVCNT_ATTACH_STATIC(crl_ev);

void
crlattach(void)
{
	scb_vecalloc(0xF0, crlintr, NULL, SCB_ISTACK, &crl_ev);
}	

/*ARGSUSED*/
int
crlopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	if (vax_cputype != VAX_8600)
		return (ENXIO);
	if (crltab.crl_state != CRL_IDLE)
		return (EALREADY);
	crltab.crl_state = CRL_OPEN;
	crltab.crl_buf = geteblk(512);
	return (0);
}

/*ARGSUSED*/
int
crlclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	brelse(crltab.crl_buf, 0);
	crltab.crl_state = CRL_IDLE;
	return 0;
}

/*ARGSUSED*/
int
crlrw(dev_t dev, struct uio *uio, int flag)
{
	struct buf *bp;
	int i;
	int s;
	int error;

	if (uio->uio_resid == 0) 
		return (0);
	s = splconsmedia();
	while (crltab.crl_state & CRL_BUSY)
		(void) tsleep(&crltab, PRIBIO, "crlbusy", 0);
	crltab.crl_state |= CRL_BUSY;
	splx(s);

	bp = crltab.crl_buf;
	error = 0;
	while ((i = imin(CRLBYSEC, uio->uio_resid)) > 0) {
		bp->b_blkno = uio->uio_offset>>9;
		if (bp->b_blkno >= MAXSEC || (uio->uio_offset & 0x1FF) != 0) {
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
		crlstart();
                while ((bp->b_oflags & BO_DONE) == 0)
                  (void) tsleep(bp, PRIBIO, "crlxfer", 0);
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
	crltab.crl_state &= ~CRL_BUSY;
	wakeup((void *)&crltab);
	return (error);
}

void
crlstart(void)
{
	struct buf *bp;

	bp = crltab.crl_buf;
	crltab.crl_errcnt = 0;
	crltab.crl_xaddr = (ushort *) bp->b_data;
	bp->b_resid = 0;

	if ((mfpr(PR_STXCS) & STXCS_RDY) == 0)
		/* not ready to receive order */
		return;
	if ((bp->b_flags&(B_READ|B_WRITE)) == B_READ) {
		crltab.crl_active = CRL_F_READ;
		mtpr(bp->b_blkno<<8 | STXCS_IE | CRL_F_READ, PR_STXCS);
	} else {
		crltab.crl_active = CRL_F_WRITE;
		mtpr(bp->b_blkno<<8 | STXCS_IE | CRL_F_WRITE, PR_STXCS);
	}
#ifdef lint
	crlintr(NULL);
#endif
}

void
crlintr(void *arg)
{
	struct buf *bp;
	int i;

	bp = crltab.crl_buf;
	i = mfpr(PR_STXCS);
	switch ((i>>24) & 0xFF) {

	case CRL_S_XCMPLT:
		switch (crltab.crl_active) {

		case CRL_F_RETSTS:
			{
				char sbuf[256], sbuf2[256];

				crlstat.crl_ds = mfpr(PR_STXDB);

				snprintb(sbuf, sizeof(sbuf), CRLCS_BITS,
				    crlstat.crl_cs);
				snprintb(sbuf2, sizeof(sbuf2), CRLDS_BITS,
				    crlstat.crl_ds);
				printf("crlcs=%s, crlds=%s\n", sbuf, sbuf2);
				break;
			}

		case CRL_F_READ:
		case CRL_F_WRITE:
			bp->b_oflags |= BO_DONE;
		}
		crltab.crl_active = 0;
		wakeup(bp);
		break;

	case CRL_S_XCONT:
		switch (crltab.crl_active) {

		case CRL_F_WRITE:
			mtpr(*crltab.crl_xaddr++, PR_STXDB);
			mtpr(bp->b_blkno<<8 | STXCS_IE | CRL_F_WRITE, PR_STXCS);
			break;

		case CRL_F_READ:
			*crltab.crl_xaddr++ = mfpr(PR_STXDB);
			mtpr(bp->b_blkno<<8 | STXCS_IE | CRL_F_READ, PR_STXCS);
		}
		break;

	case CRL_S_ABORT:
		crltab.crl_active = CRL_F_RETSTS;
		mtpr(STXCS_IE | CRL_F_RETSTS, PR_STXCS);
		bp->b_oflags |= BO_DONE;
		bp->b_error = EIO;
		break;

	case CRL_S_RETSTS:
		crlstat.crl_cs = mfpr(PR_STXDB);
		mtpr(STXCS_IE | CRL_S_RETSTS, PR_STXCS);
		break;

	case CRL_S_HNDSHK:
		printf("crl: hndshk error\n");	/* dump out some status too? */
		crltab.crl_active = 0;
		bp->b_oflags |= BO_DONE;
		bp->b_error = EIO;
		cv_broadcast(&bp->b_done);
		break;

	case CRL_S_HWERR:
		printf("crl: hard error sn%" PRId64 "\n", bp->b_blkno);
		crltab.crl_active = CRL_F_ABORT;
		mtpr(STXCS_IE | CRL_F_ABORT, PR_STXCS);
		break;
	}
}
