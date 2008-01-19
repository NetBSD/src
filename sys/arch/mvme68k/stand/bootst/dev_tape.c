/*	$NetBSD: dev_tape.c,v 1.8.32.1 2008/01/19 12:14:31 bouyer Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This module implements a "raw device" interface suitable for
 * use by the stand-alone I/O library UFS file-system code, and
 * possibly for direct access (i.e. boot from tape).
 */

#include <sys/types.h>
#include <machine/prom.h>
#include <machine/stdarg.h>

#include <lib/libkern/libkern.h>

#include <lib/libsa/stand.h>
#include "libsa.h"
#include "dev_tape.h"

extern int debug;

struct mvmeprom_dskio tape_ioreq;

static int hackprom_diskrd(struct mvmeprom_dskio *);

/*
 * This is a special version of devopen() for tape boot.
 * In this version, the file name is a numeric string of
 * one digit, which is passed to the device open so it
 * can open the appropriate tape segment.
 */
int
devopen(struct open_file *f, const char *fname, char **file)
{
	struct devsw *dp;

	*file = (char *)fname;
	dp = &devsw[0];
	f->f_dev = dp;

	/* The following will call tape_open() */
	return dp->dv_open(f, fname);
}

int
tape_open(struct open_file *f, ...)
{
	char *fname;		/* partition number, i.e. "1" */
	int part;
	struct mvmeprom_dskio *ti;
	va_list ap;

	va_start(ap, f);
	fname = va_arg(ap, char *);
	va_end(ap);

	/*
	 * Set the tape segment number to the one indicated
	 * by the single digit fname passed in above.
	 */
	if ((fname[0] < '0') && (fname[0] > '9')) {
		return ENOENT;
	}
	part = fname[0] - '0';

	/*
	 * Setup our part of the saioreq.
	 * (determines what gets opened)
	 */
	ti = &tape_ioreq;
	memset((void *)ti, 0, sizeof(*ti));

	ti->ctrl_lun = bugargs.ctrl_lun;
	ti->dev_lun = bugargs.dev_lun;
	ti->status = 0;
	ti->pbuffer = NULL;
	ti->blk_num = part;
	ti->blk_cnt = 0;
	ti->flag = 0;
	ti->addr_mod = 0;

	f->f_devdata = ti;

	return 0;
}

int
tape_close(struct open_file *f)
{
	struct mvmeprom_dskio *ti;

	ti = f->f_devdata;
	f->f_devdata = NULL;
	return 0;
}

#define MVMEPROM_SCALE (512/MVMEPROM_BLOCK_SIZE)

int
tape_strategy(void *devdata, int flag, daddr_t dblk, u_int size, void *buf,
    u_int *rsize)
{
	struct mvmeprom_dskio *ti;
	int ret;

	ti = devdata;

	if (flag != F_READ)
		return EROFS;

	ti->status = 0;
	ti->pbuffer = buf;
	/* don't change block #, set in open */
	ti->blk_cnt = size / (512 / MVMEPROM_SCALE);

	/* work around for stupid '147 prom bug */
	if (bugargs.cputyp == 0x147)
		ret = hackprom_diskrd(ti);
	else
		ret = mvmeprom_diskrd(ti);

	if (ret != 0)
		return EIO;

	*rsize = (ti->blk_cnt / MVMEPROM_SCALE) * 512;
	ti->flag |= IGNORE_FILENUM; /* ignore next time */

	return 0;
}

int
tape_ioctl(struct open_file *f, u_long cmd, void *data)
{

	return EIO;
}

static int
hackprom_diskrd(struct mvmeprom_dskio *ti)
{
	static int blkoffset = 0;

#define	hackload_addr	((char *)0x080000)	/* Load tape segment here */
#define hackload_blocks 0x2000			/* 2Mb worth */

	if ((ti->flag & IGNORE_FILENUM) == 0) {
		/*
		 * First time through. Load the whole tape segment...
		 */
		struct mvmeprom_dskio nti;
		int ret;

		nti = *ti;

		nti.pbuffer = hackload_addr;
		nti.blk_cnt = hackload_blocks;
		nti.flag |= END_OF_FILE;

		ret = mvmeprom_diskrd(&nti);

		/*
		 * PROM returns 1 on end-of-file. This isn't an
		 * error in this instance, just in case you're wondering! ;-)
		 */
		if (ret < 0 || ret > 1)
			return ret;

		blkoffset = 0;
	}

	/*
	 * Grab the required number of block(s)
	 */
	memcpy(ti->pbuffer, &(hackload_addr[blkoffset]),
	    ti->blk_cnt * MVMEPROM_BLOCK_SIZE);

	blkoffset += (ti->blk_cnt * MVMEPROM_BLOCK_SIZE);

	return 0;
}
