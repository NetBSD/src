/*	$NetBSD: promdev.c,v 1.1.1.1 1995/02/14 22:56:37 gwr Exp $ */

/*
 * Copyright (c) 1993 Paul Kranenburg
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
 *      This product includes software developed by Paul Kranenburg.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/types.h>
#include <machine/mon.h>
#include <machine/saio.h>

#include "stand.h"

int	promopen __P((struct open_file *, ...));
int	promclose __P((struct open_file *));
int	promioctl __P((struct open_file *, u_long, void *));
int	promstrategy __P((void *, int, daddr_t, u_int, char *, u_int *));

struct devsw devsw[] = {
	{ "prom", promstrategy, promopen, promclose, promioctl },
};

int	ndevs = (sizeof(devsw)/sizeof(devsw[0]));

static char pathbuf[100];

int
devopen(f, fname, file)
	struct open_file *f;
	char *fname;
	char **file;
{
	struct bootparam *bp;
	struct boottab *ops;
	struct devinfo *dip;
	struct saioreq *si;
	char *cp, *path, *dev;
	int	error;

	bp = *romp->bootParam;
	ops = bp->bootDevice;
	dip = ops->b_devinfo;

	si = (struct saioreq *) alloc(sizeof(*si));
	bzero((caddr_t)si, sizeof(*si));
	si->si_boottab = ops;
	si->si_ctlr = bp->ctlrNum;
	si->si_unit = bp->unitNum;
	si->si_boff = bp->partNum;

	f->f_devdata = (void *) si;
	f->f_dev = devsw;

#ifdef DEBUG
	printf("Boot device type: %s\n", ops->b_desc);
#endif

	path = pathbuf;
	cp = bp->argPtr[0];
	while (*cp) {
		*path++ = *cp;
		if (*cp++ == ')')
			break;
	}
	*path = '\0';
	dev = path = pathbuf;
	error = (*ops->b_open)(si);
	if (error != 0) {
		printf("Can't open device `%s'\n", dev);
		return ENXIO;
	}

	*file = fname;
	return 0;
}

int
promstrategy(devdata, flag, dblk, size, buf, rsize)
	void	*devdata;
	int	flag;
	daddr_t	dblk;
	u_int	size;
	char	*buf;
	u_int	*rsize;
{
	struct saioreq *si;
	struct boottab *ops;
	int	error = 0;
	int	si_flag, xcnt;

	si = (struct saioreq *) devdata;
	ops = si->si_boottab;

#ifdef DEBUG
	printf("promstrategy: size=%d dblk=%d\n", size, dblk);
#endif
	twiddle();

	si->si_bn = dblk;
	si->si_ma = buf;
	si->si_cc =	xcnt = 512;	/* XXX */

	si_flag = (flag == F_READ) ? SAIO_F_READ : SAIO_F_WRITE;
	error = (*ops->b_strategy)(si, si_flag);
	if (error)
		return (EIO);

	*rsize = xcnt;

#ifdef DEBUG
	printf("rsize = %x\n", *rsize);
#endif
	return (0);
}

int
promopen(f)
	struct open_file *f;
{
#ifdef DEBUG
	printf("promopen:\n");
#endif
	return 0;
}

int
promclose(f)
	struct open_file *f;
{
	return EIO;
}

int
promioctl(f, cmd, data)
	struct open_file *f;
	u_long cmd;
	void *data;
{
	return EIO;
}

