/*
 * Copyright (c) 1993 Christopher G. Demetriou
 * Copyright (c) 1982, 1986 Regents of the University of California.
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
 * 	from: @(#)subr_log.c	7.11 (Berkeley) 3/17/91
 *	$Id: subr_acct.c,v 1.4 1993/06/30 13:43:03 mycroft Exp $
 */

/*
 * acct buffer for accounting information
 */

#include "param.h"
#include "systm.h"
#include "proc.h"
#include "vnode.h"
#include "ioctl.h"
#include "file.h"
#include "malloc.h"
#include "acct.h"
#include "acctbuf.h"
#include "select.h"

#define ACCT_RDPRI	(PZERO + 1)

#define ACCT_ASYNC	0x04
#define ACCT_RDWAIT	0x08

struct acctsoftc {
	int	sc_state;		/* see above for possibilities */
	struct selinfo sc_selp;		/* process waiting on select call */
	int	sc_pgid;		/* process/group for async I/O */
} acctsoftc;

int		acct_open;		/* also used in acct() */
struct acctbuf	*acctbufp;		/* the accounting buffer pointer */

/*ARGSUSED*/
int
#ifdef __STDC__
acctopen(dev_t dev, int flags, int mode, struct proc *p)
/* gcc 1.x doesn't like non-ansi def for this */
#else
acctopen()
	dev_t dev;
	int flags, mode;
	struct proc *p;
#endif
{
	if (acct_open)
		return (EBUSY);
	acct_open = 1;
	acctsoftc.sc_pgid = p->p_pid;		/* signal process only */

	if (acctbufp == NULL)
		acctbuf_init();
#ifdef ACCT_DEBUG
	acctbuf_checkbuf("acctopen");
#endif

	return (0);
}

/*ARGSUSED*/
int
#ifdef __STDC__
acctclose(dev_t dev, int flag)
/* gcc 1.x doesn't like non-ansi def for this */
#else
acctclose()
	dev_t dev;
	int flag;
#endif
{
	acct_open = 0;
	acctsoftc.sc_state = 0;
	bzero(&acctsoftc.sc_selp,sizeof(acctsoftc.sc_selp));

	return 0;
}

/*ARGSUSED*/
int
#ifdef __STDC__
acctread(dev_t dev, struct uio *uio, int flag)
/* gcc 1.x doesn't like non-ansi def for this */
#else
acctread()
	dev_t dev;
	struct uio *uio;
	int flag;
#endif
{
	register struct acctbuf *abp = acctbufp;
	register long l;
	int error = 0;

#ifdef ACCT_DEBUG
	acctbuf_checkbuf("acctread 1");
#endif
	while (abp->ab_rind == abp->ab_wind) {
		if (flag & IO_NDELAY) {
			return (EWOULDBLOCK);
		}
		acctsoftc.sc_state |= ACCT_RDWAIT;
		if (error = tsleep((caddr_t)abp, ACCT_RDPRI | PCATCH,
		    "acct", 0)) {
			return (error);
		}
	}
	acctsoftc.sc_state &= ~ACCT_RDWAIT;

	while (uio->uio_resid > 0) {
		l = abp->ab_wind - abp->ab_rind;
		if (l < 0)
			l = ACCT_NBRECS - abp->ab_rind;
		l = MIN(l, uio->uio_resid / sizeof(struct acct));
		if (l == 0)
			break;
		error = uiomove((caddr_t)&abp->recs[abp->ab_rind],
			(int)(l * sizeof(struct acct)), uio);
		if (error)
			break;
		abp->ab_rind += l;
		if (abp->ab_rind < 0 || abp->ab_rind >= ACCT_NBRECS)
			abp->ab_rind = 0;
	}
#ifdef ACCT_DEBUG
	acctbuf_checkbuf("acctread 2");
#endif
	return (error);
}

/*ARGSUSED*/
int
#ifdef __STDC__
acctselect(dev_t dev, int rw, struct proc *p)
/* gcc 1.x doesn't like non-ansi def for this */
#else
acctselect()
	dev_t dev;
	int rw;
	struct proc *p;
#endif
{
	switch (rw) {

	case FREAD:
#ifdef ACCT_DEBUG
		acctbuf_checkbuf("acctselect");
#endif
		if (acctbufp->ab_rind != acctbufp->ab_wind) {
			return (1);
		}
		selrecord(p, &acctsoftc.sc_selp);
		break;
	}
	return (0);
}

void
acctwakeup()
{
	struct proc *p;

	if (!acct_open)
		return;
	selwakeup(&acctsoftc.sc_selp);
	if (acctsoftc.sc_state & ACCT_ASYNC) {
		if (acctsoftc.sc_pgid < 0)
			gsignal(-acctsoftc.sc_pgid, SIGIO); 
		else if (p = pfind(acctsoftc.sc_pgid))
			psignal(p, SIGIO);
	}
	if (acctsoftc.sc_state & ACCT_RDWAIT) {
		wakeup((caddr_t)acctbufp);
		acctsoftc.sc_state &= ~ACCT_RDWAIT;
	}
}

/*ARGSUSED*/
int
#ifdef __STDC__
acctioctl(dev_t dev, int com, caddr_t data, int flag)
/* gcc 1.x doesn't like non-ansi def for this */
#else
acctioctl()
	dev_t dev;
	int com;
	caddr_t data;
	int flag;
#endif
{
	long l;

	switch (com) {

	/* return number of characters immediately available */
	case FIONREAD:
#ifdef ACCT_DEBUG
		acctbuf_checkbuf("acctioctl");
#endif
		l = acctbufp->ab_wind - acctbufp->ab_rind;
		if (l < 0)
			l += ACCT_NBRECS;
		*(off_t *)data = l * sizeof(struct acct);
		break;

	case FIONBIO:
		break;

	case FIOASYNC:
		if (*(int *)data)
			acctsoftc.sc_state |= ACCT_ASYNC;
		else
			acctsoftc.sc_state &= ~ACCT_ASYNC;
		break;

	case TIOCSPGRP:
		acctsoftc.sc_pgid = *(int *)data;
		break;

	case TIOCGPGRP:
		*(int *)data = acctsoftc.sc_pgid;
		break;

	default:
		return (-1);
	}
	return (0);
}

void
acctbuf_init()
{
	if (acctbufp != NULL)
		panic("acctbuf reinit\n");

	acctbufp = malloc(ACCT_BSIZE, M_DEVBUF, M_WAITOK);
	if (acctbufp == NULL)
		panic("acctbuf_init: couldn't allocate accounting buffer\n");

	bzero(acctbufp, ACCT_BSIZE);
	acctbufp->ab_magic = ACCT_MAGIC;
}

#ifdef ACCT_DEBUG
void
acctbuf_checkbuf(char *from)
{
	char *str;
	int dopanic = 0;

	if (acctbufp == NULL) {
		str = "accounting buffer pointer null";
		dopanic = 1;
	}
	if (acctbufp->ab_magic != ACCT_MAGIC) {
		str = "accounting buffer magic bad";
		dopanic = 1;
	}
	if (acctbufp->ab_rind < 0 || acctbufp->ab_rind >= ACCT_NBRECS) {
		str = "accounting buffer read index out of range";
		dopanic = 1;
	}
	if (acctbufp->ab_wind < 0 || acctbufp->ab_wind >= ACCT_NBRECS) {
		str = "accounting buffer write index out of range";
		dopanic = 1;
	}
	if (dopanic) {
		printf("acctbuf_checkbuf: from %s\n", from);
		panic(str);
	}
}
#endif
