/*	$NetBSD: winblk.c,v 1.7 2006/01/25 18:28:26 christos Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura.
 * All rights reserved.
 *
 * This software is part of the PocketBSD.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
#define STANDALONE_WINDOWS_SIDE
#include <stand.h>
#include <winblk.h>
#include <winioctl.h>
#include <sys/disklabel.h>
#include "diskio.h"

/*
 * BOOL
 * DeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode,
 * 			LPVOID lpInBuffer, DWORD nInBufferSize,
 * 			LPVOID lpOutBuffer, DWORD nOutBufferSize,
 * 			LPDWORD lpBytesReturned,
 *			LPOVERLAPPED lpOverlapped);
 */

#ifdef DEBUG
#define DEBUG_PRINTF(a) win_printf a
#else
#define DEBUG_PRINTF(a)
#endif

#define islower(c)	('a' <= (c) && (c) <= 'z')
#define toupper(c)	(islower(c) ? ((c) - 'a' + 'A') : (c))

#define BLKSZ	512

struct winblk {
	HANDLE	hDevice;
	DISK_INFO di;
	struct mbr_partition mbr[MBR_PART_COUNT];
	struct disklabel dl;
	char buf[BLKSZ];
	int start;
};

static int rawread(struct winblk *ctx, int start, int nsecs, char *buf);

int 
winblkstrategy(devdata, flag, dblk, size, buf, rsize)
	void           *devdata;
	int             flag;
	daddr_t         dblk;
	size_t          size;
	void           *buf;
	size_t         *rsize;
{
	struct winblk *ctx = (struct winblk*)devdata;
	int error;
	size_t nblks;

	if (flag != F_READ)
		return (EROFS);

	dblk += ctx->start;
	nblks = (size / BLKSZ);

	if (error = rawread(ctx, dblk, nblks, buf)) {
		return (error);
	}
	if (nblks * BLKSZ < size) {
		if (error = rawread(ctx, dblk + nblks, 1, ctx->buf)) {
			return (error);
		}
		memcpy((BYTE*)buf + nblks * BLKSZ, ctx->buf,
		       size - nblks * BLKSZ);
	}

	if (rsize)
		*rsize = size;
	return (0);
}


int 
winblkopen(struct open_file *f, ...)
/* file, devname, unit, partition */
{
	va_list ap;
	struct winblk *ctx = NULL;
	char *devname;
	int unit;
	int partition;
	TCHAR wdevname[6];
	DWORD wres;
	int i;
	int start_386bsd;

	int error = 0;

	ctx = (struct winblk *)alloc(sizeof(*ctx));
	if (!ctx) {
		error = ENOMEM;
		goto end;
	}
	f->f_devdata = ctx;

	va_start(ap, f);
	devname = va_arg(ap, char*);
	unit = va_arg(ap, int);
	partition = va_arg(ap, int);
        va_end(ap);

	/*
	 *  Windows' device name must be 3 uppper letters and 1 digit
	 *  following a semicolon like "DSK1:".
	 */
	if (strlen(devname) != 3 || unit < 0 || 9 < unit) {
		error = ENODEV;
		goto end;
	}
	wsprintf(wdevname, TEXT("%C%C%C%d:"),
		toupper(devname[0]),
		toupper(devname[1]),
		toupper(devname[2]),
		unit);
	DEBUG_PRINTF((TEXT("winblk.open: block device name is '%s'\n"),
		      wdevname));

	ctx->hDevice = CreateFile(wdevname, GENERIC_READ, 0, NULL,    
				  OPEN_EXISTING, 0, NULL);
	if (ctx->hDevice == INVALID_HANDLE_VALUE) {
		win_printf(TEXT("can't open %s.\n"), wdevname);
		error = ENODEV; /* XXX, We shuld check GetLastError(). */
		goto end;
	}

	/*
	 *  get DISK_INFO
	 *  CHS, sector size and device flags.
	 */
	if (!DeviceIoControl(ctx->hDevice, DISK_IOCTL_GETINFO,
			     &ctx->di, sizeof(ctx->di),
			     NULL, 0, &wres, NULL)) {
		win_printf(TEXT("DeviceIoControl() failed.error=%d\n"),
			   GetLastError());

		error = EIO; /* XXX, We shuld check GetLastError(). */
		goto end;
	}

#ifdef DEBUG
	win_printf(TEXT("DISK_INFO: CHS=%d:%d:%d  block size=%d  flag="),
		   ctx->di.di_cylinders,
		   ctx->di.di_heads,
		   ctx->di.di_sectors,
		   ctx->di.di_bytes_per_sect);
	if (ctx->di.di_flags & DISK_INFO_FLAG_MBR) {
		win_printf(TEXT("MBR "));
	}
	if (ctx->di.di_flags & DISK_INFO_FLAG_CHS_UNCERTAIN) {
		win_printf(TEXT("CHS_UNCERTAIN "));
	}
	if (ctx->di.di_flags & DISK_INFO_FLAG_UNFORMATTED) {
		win_printf(TEXT("UNFORMATTED "));
	}
	if (ctx->di.di_flags & DISK_INFO_FLAG_PAGEABLE) {
		win_printf(TEXT("PAGEABLE "));
	}
	win_printf(TEXT("\n"));
#endif /* DEBUG */

	if (!(ctx->di.di_flags & DISK_INFO_FLAG_MBR) ||
	     (ctx->di.di_flags & DISK_INFO_FLAG_CHS_UNCERTAIN) ||
	     (ctx->di.di_flags & DISK_INFO_FLAG_UNFORMATTED) ||
	     (ctx->di.di_bytes_per_sect != BLKSZ)) {
		win_printf(TEXT("invalid flags\n"));
		error = EINVAL;
		goto end;
	}

	/*
	 *  read MBR
	 */
	if (error = rawread(ctx, MBR_BBSECTOR, 1, ctx->buf)) {
		goto end;
	}
	memcpy(&ctx->mbr, &ctx->buf[MBR_PART_OFFSET], sizeof(ctx->mbr));

	for (i = 0; i < MBR_PART_COUNT; i++) {
	        DEBUG_PRINTF((TEXT("%d: type=%d %d(%d) (%d:%d:%d - %d:%d:%d)")
			      TEXT(" flag=0x%02x\n"),
			      i,
			      ctx->mbr[i].mbrp_type,
			      ctx->mbr[i].mbrp_start,
			      ctx->mbr[i].mbrp_size,
			      ctx->mbr[i].mbrp_scyl,
			      ctx->mbr[i].mbrp_shd,
			      ctx->mbr[i].mbrp_ssect,
			      ctx->mbr[i].mbrp_ecyl,
			      ctx->mbr[i].mbrp_ehd,
			      ctx->mbr[i].mbrp_esect,
			      ctx->mbr[i].mbrp_flag));
	}

	/*
	 *  find BSD partition
	 */
	ctx->start = -1;
	start_386bsd = -1;
	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (ctx->mbr[i].mbrp_type == MBR_PTYPE_NETBSD) {
			ctx->start = ctx->mbr[i].mbrp_start;
			break;
		}
		if (ctx->mbr[i].mbrp_type == MBR_PTYPE_386BSD) {
			start_386bsd = ctx->mbr[i].mbrp_start;
		}
	}
	if (ctx->start == -1) {
		ctx->start = start_386bsd;
	}

	if (ctx->start == -1) {
		/*
		 *  BSD partition is not found.
		 *  Try to use entire disk.
		 */
		ctx->start = 0;
		win_printf(TEXT("no BSD partition, start sector=0x%x\n"),
			   ctx->start);
		goto end;
	}

	/*
	 *  read disklabel
	 */
	if (error = rawread(ctx, ctx->start + LABELSECTOR, 1, ctx->buf)) {
		goto end;
	}
	memcpy(&ctx->dl, &ctx->buf[LABELOFFSET], sizeof(ctx->dl));

	if (ctx->dl.d_magic != DISKMAGIC ||
	    ctx->dl.d_magic2 != DISKMAGIC ||
	    dkcksum(&ctx->dl) != 0) {
		win_printf(TEXT("invalid disklabel, start sector=0x%x\n"),
			   ctx->start);
		/*
		 *  Disklabel is not found.
		 *  Try to use entire partition.
		 */
		goto end;
	}

	if (partition < 0 || ctx->dl.d_npartitions <= partition) {
		error = EINVAL;
		goto end;
	}

	ctx->start = ctx->dl.d_partitions[partition].p_offset;
	win_printf(TEXT("start sector=0x%x\n"), ctx->start);

      end:
	if (error && ctx) {
		dealloc(ctx, sizeof(*ctx));
		f->f_devdata = NULL;
	}
	return (error);
}

int 
winblkclose(f)
	struct open_file *f;
{
	struct winblk *ctx = f->f_devdata;

	dealloc(ctx, sizeof(*ctx));

	f->f_devdata = NULL;
	return (0);
}

int 
winblkioctl(f, cmd, arg)
	struct open_file *f;
	u_long          cmd;
	void           *arg;
{
	return EIO;
}

static int
rawread(ctx, start, nsecs, buf)
	struct winblk *ctx;
	int start, nsecs;
	char *buf;
{
	SG_REQ req;
	DWORD res;

	req.sr_start = start;
	req.sr_num_sec = nsecs;
	req.sr_num_sg = 1;
	req.sr_callback = NULL;
	req.sr_sglist[0].sb_buf = buf;
	req.sr_sglist[0].sb_len = nsecs * BLKSZ;

	DEBUG_PRINTF((TEXT("rawread(0x%x, %d)"), start, nsecs));
	if (!DeviceIoControl(ctx->hDevice, DISK_IOCTL_READ,
			     &req, sizeof(req),
			     NULL, 0, &res, NULL)) {
		win_printf(TEXT("DeviceIoControl() failed.error=%d\n"),
			   GetLastError());

		return (EIO); /* XXX, We shuld check GetLastError(). */
	}
	DEBUG_PRINTF((TEXT("=%d\n"), req.sr_status));

	if (req.sr_status != ERROR_SUCCESS) {
		win_printf(TEXT("DeviceIoControl(READ): status=%d\n"),
			   req.sr_status);
		return (EIO); /* XXX, We shuld check error code. */
	}

	return (0);
}
