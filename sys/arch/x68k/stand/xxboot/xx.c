/*	$NetBSD: xx.c,v 1.5 2022/04/25 15:12:07 mlelstv Exp $	*/

/*
 * Copyright (c) 2010 MINOURA Makoto.
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

#include <sys/param.h>
#include <lib/libsa/stand.h>

#include "xxboot.h"
#include "iocs.h"

int
xxopen(struct open_file *f, ...)
{

	return 0;
}

int
xxclose(struct open_file *f)
{

	return 0;
}

int
xxstrategy(void *arg, int rw, daddr_t dblk, size_t size,
           void *buf, size_t *rsize)
{

	/*
	 * dblk is (always?) in 512 bytes unit, even if CD (2048 byte/sect).
	 * size is in byte.
	 *
	 * On SCSI HD, the position specified in raw_read() (1st argument)
	 * counts from the beginning of the disk, not the beginning of the
	 * partition.  On SCSI CD and floppy, SCSI_PARTTOP is zero.
	 */
#if defined(XXBOOT_DEBUG)
	IOCS_B_PRINT("xxstrategy ");
	print_hex(dblk, 8);
	IOCS_B_PRINT(" len=");
	print_hex(size, 8);
	IOCS_B_PRINT("\r\n");
#endif
	if (size != 0) {
		raw_read((uint32_t)(SCSI_PARTTOP + dblk), (uint32_t)size, buf);
	}
	if (rsize)
		*rsize = size;
	return 0;
}

int
xxioctl(struct open_file *f, u_long cmd, void *data)
{

	return ENOTTY;
}

