/* $NetBSD: blkdev.c,v 1.5.2.1 2009/05/13 17:18:17 jym Exp $ */

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Van Jacobson of Lawrence Berkeley Laboratory and Ralph Campbell.
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
 *	@(#)rz.c	8.1 (Berkeley) 6/10/93
 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <sys/param.h>
#include <sys/disklabel.h>

#include "stand/common/cfe_api.h"

#include "stand/common/common.h"
#include "blkdev.h"

/*
 * If BOOTXX_FS_TYPE is defined, we'll try to find and use the first
 * partition with that type mentioned in the disklabel, else default to
 * using block 0.
 *
 * The old boot blocks used to look for a file system starting at block
 * 0.  It's not immediately obvious that change here is necessary or good,
 * so for now we don't bother looking for the specific type.
 */
#undef BOOTXX_FS_TYPE

int		blkdev_is_open;
u_int32_t	blkdev_part_offset;

/*
 * Since we have only one device, and want to squeeze space, we just
 * short-circuit devopen() to do the disk open as well.
 *
 * Devopen is supposed to decode the string 'fname', open the device,
 * and make 'file' point to the remaining file name.  Since we don't
 * do any device munging, we can just set *file to fname.
 */
int
devopen(struct open_file *f, const char *fname, char **file)
	/* file:	 out */
{
#if defined(BOOTXX_FS_TYPE)
	int i;
	size_t cnt;
	char *msg, buf[DEV_BSIZE];
	struct disklabel l;
#endif /* defined(BOOTXX_FS_TYPE) */

	if (blkdev_is_open) {
		return (EBUSY);
	    }

	*file = (char *)fname;

#if 0
	f->f_devdata = NULL;			/* no point */
#endif

	/* Try to read disk label and partition table information. */
	blkdev_part_offset = 0;
#if defined(BOOTXX_FS_TYPE)

	i = diskstrategy(NULL, F_READ,
	    (daddr_t)LABELSECTOR, DEV_BSIZE, buf, &cnt);
	if (i || cnt != DEV_BSIZE) {
		return (ENXIO);
	}
	msg = getdisklabel(buf, &l);
	if (msg == NULL) {
		/*
		 * there's a label.  find the first partition of the
		 * type we want and use its offset.  if none are
		 * found, we just use offset 0.
		 */
		for (i = 0; i < l.d_npartitions; i++) {
			if (l.d_partitions[i].p_fstype == BOOTXX_FS_TYPE) {
				blkdev_part_offset = l.d_partitions[i].p_offset;
				break;
			}
		}
	} else {
		/* just use offset 0; it's already set that way */
	}
#endif /* defined(BOOTXX_FS_TYPE) */

	blkdev_is_open = 1;
	return (0);
}

int
blkdevstrategy(void *devdata, int rw, daddr_t bn, size_t reqcnt, void *addrvoid, size_t *cnt)
	/* cnt:	 out: number of bytes transfered */
{
	unsigned char *addr = addrvoid;
	int res;

#if !defined(LIBSA_NO_TWIDDLE)
	twiddle();
#endif

	/* Partial-block transfers not handled. */
	if (reqcnt & (DEV_BSIZE - 1)) {
		*cnt = 0;
		return (EINVAL);
	}
	res = cfe_readblk(booted_dev_fd,(bn+blkdev_part_offset)*DEV_BSIZE,addr,reqcnt);
	if (res < 0) return EIO;

	*cnt = res;
	return (0);
}

#if !defined(LIBSA_NO_FS_CLOSE)
int
blkdevclose(struct open_file *f)
{

	blkdev_is_open = 0;
	return (0);
}
#endif /* !defined(LIBSA_NO_FS_CLOSE) */
