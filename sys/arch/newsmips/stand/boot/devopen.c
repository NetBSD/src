/*	$NetBSD: devopen.c,v 1.2 1999/12/22 05:54:41 tsubai Exp $	*/

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/ufs.h>
#include <netinet/in.h>
#include <lib/libsa/nfs.h>

#include <machine/apcall.h>
#include <machine/romcall.h>
#include <promdev.h>

#ifdef BOOT_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

int dkopen __P((struct open_file *, ...));
int dkclose __P((struct open_file *));
int dkstrategy __P((void *, int, daddr_t, size_t, void *, size_t *));

struct devsw devsw[] = {
	{ "dk", dkstrategy, dkopen, dkclose, noioctl }
};
int ndevs = sizeof(devsw) / sizeof(devsw[0]);

struct fs_ops file_system_ufs[] = {
	{ ufs_open, ufs_close, ufs_read, ufs_write, ufs_seek, ufs_stat }
};
struct fs_ops file_system_nfs[] = {
	{ nfs_open, nfs_close, nfs_read, nfs_write, nfs_seek, nfs_stat },
};
struct fs_ops file_system[1];
int nfsys = sizeof(file_system) / sizeof(file_system[0]);

struct romdev romdev;

extern int apbus;

int
devopen(f, fname, file)
	struct open_file *f;
	const char *fname;
	char **file;	/* out */
{
	int fd;
	char devname[32];
	char *cp;
	int error = 0;

	DPRINTF("devopen: %s\n", fname);

	strcpy(devname, fname);
	cp = strchr(devname, ')') + 1;
	*cp = 0;
	if (apbus)
		fd = apcall_open(devname, 2);
	else
		fd = rom_open(devname, 2);

	DPRINTF("devname = %s, fd = %d\n", devname, fd);
	if (fd == -1)
		return -1;

	romdev.fd = fd;
	if (strncmp(devname, "sonic", 5) == 0)
		romdev.devtype = DT_NET;
	else
		romdev.devtype = DT_BLOCK;

	f->f_dev = devsw;
	f->f_devdata = &romdev;
	*file = strchr(fname, ')') + 1;

	if (romdev.devtype == DT_BLOCK)
		bcopy(file_system_ufs, file_system, sizeof(file_system));
	else {	/* DT_NET */
		bcopy(file_system_nfs, file_system, sizeof(file_system));

		if ((error = net_open(&romdev)) != 0) {
			printf("Can't open NFS network connection on `%s'\n",
			       devname);
			return error;
		}
	}

	return 0;
}

int
dkopen(struct open_file *f, ...)
{
	DPRINTF("dkopen\n");
	return 0;
}

int
dkclose(f)
	struct open_file *f;
{
	struct romdev *dev = f->f_devdata;

	DPRINTF("dkclose\n");
	if (apbus)
		apcall_close(dev->fd);
	else
		rom_close(dev->fd);

	return 0;
}

int
dkstrategy(devdata, rw, blk, size, buf, rsize)
	void *devdata;
	int rw;
	daddr_t blk;
	size_t size;
	void *buf;
	size_t *rsize;	/* out: number of bytes transfered */
{
	struct romdev *dev = devdata;

	/* XXX should use partition offset */

	if (apbus) {
		apcall_lseek(dev->fd, blk * 512, 0);
		apcall_read(dev->fd, buf, size);
	} else {
		rom_lseek(dev->fd, blk * 512, 0);
		rom_read(dev->fd, buf, size);
	}
	*rsize = size;		/* XXX */
	return 0;
}
