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
 *
 *	$Id: promdev.c,v 1.3 1994/07/20 20:47:22 pk Exp $
 */

#include <sys/param.h>

#include "defs.h"

struct promvec	*promvec;

int promopen __P((struct open_file *, ...));
int promclose __P((struct open_file *));
int promioctl __P((struct open_file *, int, void *));
int promstrategy __P((void *, int, daddr_t, u_int, char *, u_int *));

struct devsw devsw[] = {
	{ "prom", promstrategy, promopen, promclose, promioctl },
};

int	ndevs = (sizeof(devsw)/sizeof(devsw[0]));

int
devopen(f, fname, file)
	struct open_file *f;
	char *fname;
	char **file;
{
	int	error, fd;
	char	*dev, *cp;

	if (promvec->pv_romvec_vers >= 2) {
		dev = *promvec->pv_v2bootargs.v2_bootpath;
		fd = (*promvec->pv_v2devops.v2_open)(dev);
	} else {
		char *cp;
		static char bdev[100];

		dev = bdev;
		cp = (*promvec->pv_v0bootargs)->ba_argv[0];
		while (*cp) {
			*dev++ = *cp;
			if (*cp++ == ')')
				break;
		}
		*dev = '\0';
		dev = bdev;
		fd = (*promvec->pv_v0devops.v0_open)(dev);
	}

	if (fd == 0) {
		printf("Can't open device `%s'\n", dev);
		return ENXIO;
	}

	f->f_dev = devsw;
	f->f_devdata = (void *)fd;
	*file = fname;
	return 0;
}

int
promstrategy(devdata, flag, dblk, size, buf, rsize)
	void *devdata;
	int flag;
	daddr_t dblk;
	u_int size;
	char *buf;
	u_int *rsize;
{
	int	error = 0;

#ifdef DEBUG
	printf("promstrategy: size=%d dblk=%d\n", size, dblk);
#endif
	twiddle();

	if (promvec->pv_romvec_vers >= 2) {
		(*promvec->pv_v2devops.v2_seek)((int)devdata, 0, dbtob(dblk));

		*rsize = (*((flag == F_READ)
				? (u_int (*)())promvec->pv_v2devops.v2_read
				: (u_int (*)())promvec->pv_v2devops.v2_write
			 ))((int)devdata, buf, size);
	} else {
		int n = (*((flag == F_READ)
				? (u_int (*)())promvec->pv_v0devops.v0_rbdev
				: (u_int (*)())promvec->pv_v0devops.v0_wbdev
			))((int)devdata, btodb(size), dblk, buf);
		*rsize = dbtob(n);
	}

#ifdef DEBUG
	printf("rsize = %x\n", *rsize);
#endif
	return error;
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
	int cmd;
	void *data;
{
	return EIO;
}

getchar()
{
	char c;
	int n;
 
	if (promvec->pv_romvec_vers > 2)
		while ((n = (*promvec->pv_v2devops.v2_read)
			(*promvec->pv_v2bootargs.v2_fd0, (caddr_t)&c, 1)) != 1);
	else
		c = (*promvec->pv_getchar)();

	if (c == '\r')
		c = '\n';
	return (c);
}
 
peekchar()
{
	register int c;
 
	c = (*promvec->pv_nbgetchar)();
	if (c == '\r')
		c = '\n';
	return (c);
}

static void
pv_putchar(c)
	int c;
{
	char c0 = c;
	if (promvec->pv_romvec_vers > 2)
		(*promvec->pv_v2devops.v2_write)
			(*promvec->pv_v2bootargs.v2_fd1, &c0, 1);
	else
		(*promvec->pv_putchar)(c);
}

putchar(c)
	int c;
{
 
	if (c == '\n')
		pv_putchar('\r');
	pv_putchar(c);

#if 0
	if (c == '\n')
		(*promvec->pv_putchar)('\r');
	(*promvec->pv_putchar)(c);
#endif
}

