/*	$NetBSD: unixdev.c,v 1.1.6.2 2009/05/13 17:18:55 jym Exp $	*/
/*	$OpenBSD: unixdev.c,v 1.6 2007/06/16 00:26:33 deraadt Exp $	*/

/*
 * Copyright (c) 1996-1998 Michael Shalayeff
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
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

#include <sys/param.h>
#include <sys/reboot.h>

#include <machine/stdarg.h>

#include "boot.h"
#include "bootinfo.h"
#include "disk.h"
#include "unixdev.h"
#include "compat_linux.h"

static struct btinfo_bootdisk bi_disk;

int
unixstrategy(void *devdata, int rw, daddr_t blk, size_t size, void *buf,
    size_t *rsize)
{
	off_t off;
	int fd = (int)devdata;
	int rc = 0;

#ifdef UNIX_DEBUG
	printf("unixstrategy: %s %d bytes @ %d\n",
	    ((rw == F_READ) ? "reading" : "writing"), (int)size, (int)blk);
#endif

	off = (off_t)blk * DEV_BSIZE;
	if ((rc = ulseek(fd, off, SEEK_SET)) >= 0)
		rc = (rw == F_READ) ? uread(fd, buf, size) :
		    uwrite(fd, buf, size);

	if (rc >= 0) {
		*rsize = (size_t)rc;
		rc = 0;
	} else
		rc = errno;
	return rc;
}

int
unixopen(struct open_file *f, ...)
{
	va_list	ap;
	char path[PATH_MAX];
	struct diskinfo *dip;
	char *devname;
	const char *fname;
	int dev;
	u_int unit, partition;
	int dospart;

	va_start(ap, f);
	dev = va_arg(ap, int);
	devname = va_arg(ap, char *);
	unit = va_arg(ap, u_int);
	partition = va_arg(ap, u_int);
	fname = va_arg(ap, char *);
	va_end(ap);

	f->f_devdata = NULL;

	/* Find device. */
	dip = dkdevice(devname, unit);
	if (dip == NULL)
		return ENOENT;

	/* Try for disklabel again (might be removable media). */
	if (dip->bios_info.flags & BDI_BADLABEL) {
		const char *st = bios_getdisklabel(&dip->bios_info,
		    &dip->disklabel);
#ifdef UNIX_DEBUG
		if (debug && st)
			printf("%s\n", st);
#endif
		if (!st) {
			dip->bios_info.flags &= ~BDI_BADLABEL;
			dip->bios_info.flags |= BDI_GOODLABEL;
		} else
			return ERDLAB;
	}

	dospart = bios_getdospart(&dip->bios_info);
	bios_devpath(dip->bios_info.bios_number, dospart, path);
	f->f_devdata = (void *)uopen(path, LINUX_O_RDONLY);
	if ((int)f->f_devdata == -1)
		return errno;

	bi_disk.biosdev = dip->bios_info.bios_number;
	bi_disk.partition = partition;
	bi_disk.labelsector =
	    dip->disklabel.d_partitions[partition].p_offset + LABELSECTOR;
	bi_disk.label.type = dip->disklabel.d_type;
	memcpy(bi_disk.label.packname, dip->disklabel.d_packname, 16);
	bi_disk.label.checksum = dip->disklabel.d_checksum;
	BI_ADD(&bi_disk, BTINFO_BOOTDISK, sizeof(bi_disk));

	return 0;
}

int
unixclose(struct open_file *f)
{

	return uclose((int)f->f_devdata);
}

int
unixioctl(struct open_file *f, u_long cmd, void *data)
{

	return uioctl((int)f->f_devdata, cmd, data);
}

off_t
ulseek(int fd, off_t off, int wh)
{
	extern long ulseek32(int, long, int);
	off_t r;

	/* XXX only SEEK_SET is used, so anything else can fail for now. */

	if (wh == SEEK_SET) {
		if (ulseek32(fd, 0, SEEK_SET) != 0)
			return -1;
		while (off > OFFT_OFFSET_MAX) {
			off -= OFFT_OFFSET_MAX;
			if (ulseek32(fd, OFFT_OFFSET_MAX, SEEK_CUR) < 0 &&
			    errno != LINUX_EOVERFLOW)
				return -1;
		}
		r = ulseek32(fd, (long)off, SEEK_CUR);
		if (r == -1 && errno == LINUX_EOVERFLOW)
			r = off;
	} else
		r = ulseek32(fd, (long)off, wh);

	return r;
}
