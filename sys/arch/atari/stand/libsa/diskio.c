/*	$NetBSD: diskio.c,v 1.3 2005/06/28 21:03:02 junyoung Exp $	*/

/*
 * Copyright (c) 1995 Waldi Ravens.
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
 *        This product includes software developed by Waldi Ravens.
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

#include <lib/libsa/stand.h>
#include "atari_stand.h"
#include <sys/disklabel.h>

typedef int (*rdsec_f)__P((void *buffer, u_int offset, u_int count));
typedef	struct { rdsec_f rds; u_int rst; u_int rend; } bdevd_t;

static int rootstrategy __P((void *, int, daddr_t, size_t, void *, size_t *));
static int rootopen __P((struct open_file *, ...));
static int rootclose __P((struct open_file *));
static int rootioctl __P((struct open_file *, u_long, void *));

static struct devsw devsw[] = {
	{ "root", rootstrategy, rootopen, rootclose, rootioctl }
};
static bdevd_t	bootdev;

/*
 * Initialise boot device info.
 */
int
init_dskio (func, label, root)
	void	*func, *label;
	int	root;
{
	struct disklabel *dl = label;
	struct partition *pd = &dl->d_partitions[root];

	if (dl->d_magic != DISKMAGIC || dl->d_magic2 != DISKMAGIC
	    || dl->d_npartitions > MAXPARTITIONS || dkcksum(dl) != 0) {
		printf("Invalid disk label.\n");
		return(-1);
	}
	if (root >= 0) {
		if (root >= dl->d_npartitions
		    || pd->p_fstype != FS_BSDFFS || pd->p_size == 0) {
			printf("No suitable root.\n");
			return(-1);
		}
		bootdev.rds  = func;
		bootdev.rst  = pd->p_offset;
		bootdev.rend = pd->p_offset + pd->p_size - 1;
	}
	return(0);
}

/*
 * No choice, the kernel is loaded from the
 * same device as the bootstrap.
 */
int
devopen (f, fname, file)
	struct open_file *f;
	const char *fname;
	char **file;
{
	f->f_devdata = &bootdev;
	f->f_dev = &devsw[0];
	*file = (char *)fname;
	return(0);
}

static int
rootstrategy (devd, flag, dblk, size, buf, rsize)
	void	*devd;
	int	flag;
	daddr_t	dblk;
	size_t	size;
	void	*buf;
	size_t	*rsize;
{
	bdevd_t	*dd = devd;
	daddr_t stb = dd->rst + dblk;
	size_t	nb  = size >> 9;

	if ((flag == F_READ) && !(size & 511) && (stb + nb <= dd->rend)) {
		if (!dd->rds(buf, stb, nb)) {
			*rsize = size;
			return(0);
		}
	}
	*rsize = 0;
	return(EIO);
}

static int
rootopen (f)
	struct open_file *f;
{
	return(0);
}

static int
rootclose (f)
	struct open_file *f;
{
	return(EIO);
}

static int
rootioctl (f, cmd, data)
	struct open_file *f;
	u_long	cmd;
	void	*data;
{
	return(EIO);
}
