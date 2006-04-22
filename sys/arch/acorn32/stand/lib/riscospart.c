/*	$NetBSD: riscospart.c,v 1.1.6.2 2006/04/22 11:37:10 simonb Exp $	*/

/*-
 * Copyright (c) 2006 Ben Harris
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1995 Mark Brinicombe
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
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/disklabel_acorn.h>

#include <lib/libsa/stand.h>

#include "riscospart.h"

/*
 * This function should be shared between here,
 * sys/arch/arm/arm/disksubr_acorn.c, and
 * sys/fs/filecorefs/filecore_utils.c, rather than being copied.
 */
/*
 * static int filecore_checksum(u_char *bootblock)
 *
 * Calculates the filecore boot block checksum. This is used to validate
 * a filecore boot block on the disk.  If a boot block is validated then
 * it is used to locate the partition table. If the boot block is not
 * validated, it is assumed that the whole disk is NetBSD.
 *
 * The basic algorithm is:
 *
 *	for (each byte in block, excluding checksum) {
 *		sum += byte;
 *		if (sum > 255)
 *			sum -= 255;
 *	}
 *
 * That's equivalent to summing all of the bytes in the block
 * (excluding the checksum byte, of course), then calculating the
 * checksum as "cksum = sum - ((sum - 1) / 255) * 255)".  That
 * expression may or may not yield a faster checksum function,
 * but it's easier to reason about.
 *
 * Note that if you have a block filled with bytes of a single
 * value "X" (regardless of that value!) and calculate the cksum
 * of the block (excluding the checksum byte), you will _always_
 * end up with a checksum of X.  (Do the math; that can be derived
 * from the checksum calculation function!)  That means that
 * blocks which contain bytes which all have the same value will
 * always checksum properly.  That's a _very_ unlikely occurence
 * (probably impossible, actually) for a valid filecore boot block,
 * so we treat such blocks as invalid.
 */
static int
filecore_checksum(u_char *bootblock)
{  
	u_char byte0, accum_diff;
	u_int sum;
	int i;
 
	sum = 0;
	accum_diff = 0;
	byte0 = bootblock[0];
 
	/*
	 * Sum the contents of the block, keeping track of whether
	 * or not all bytes are the same.  If 'accum_diff' ends up
	 * being zero, all of the bytes are, in fact, the same.
	 */
	for (i = 0; i < 511; ++i) {
		sum += bootblock[i];
		accum_diff |= bootblock[i] ^ byte0;
	}

	/*
	 * Check to see if the checksum byte is the same as the
	 * rest of the bytes, too.  (Note that if all of the bytes
	 * are the same except the checksum, a checksum compare
	 * won't succeed, but that's not our problem.)
	 */
	accum_diff |= bootblock[i] ^ byte0;

	/* All bytes in block are the same; call it invalid. */
	if (accum_diff == 0)
		return (-1);

	return (sum - ((sum - 1) / 255) * 255);
}


int
getdisklabel_acorn(struct open_file *f, struct disklabel *lp)
{
	size_t rsize;
	int err;
	unsigned char *buf;
	struct filecore_bootblock *bb;
	daddr_t labelsect;
	char *msg;

	buf = alloc(DEV_BSIZE);
	err = DEV_STRATEGY(f->f_dev)(f->f_devdata, F_READ,
	    FILECORE_BOOT_SECTOR, DEV_BSIZE, buf, &rsize);
	if (err != 0) goto out;
	bb = (struct filecore_bootblock *) buf;
	if (bb->checksum == filecore_checksum((u_char *)bb)) {
		if (bb->partition_type == PARTITION_FORMAT_RISCBSD)
			labelsect = (daddr_t)bb->partition_cyl_low *
			    bb->heads * bb->secspertrack + LABELSECTOR;
		else {
			err = EUNLAB;
			goto out;
		}
	} else
		labelsect = LABELSECTOR;
	err = DEV_STRATEGY(f->f_dev)(f->f_devdata, F_READ,
	    labelsect, DEV_BSIZE, buf, &rsize);
	if (err != 0) goto out;
	msg = getdisklabel(buf, lp);
	if (msg) {
		printf("%s\n", msg);
		err = ERDLAB;
	}
out:
	dealloc(buf, DEV_BSIZE);
	return err;
}
