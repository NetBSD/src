/* $NetBSD: dev_lc.c,v 1.1 2003/05/01 07:02:01 igy Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Naoto Shimazaki of YOKOGAWA Electric Corporation.
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
 * Pseudo device and filesystem operation for L-Card+ boot loader
 */

#include <lib/libsa/stand.h>

#include "extern.h"

struct dev_lc {
	u_int8_t	*lc_addr;
} lc_ca;

int
devopen(struct open_file *f, const char *fname, char **file)
{
	*file = (char *) fname;
	return 0;
}

int
lc_open (char *path, struct open_file *f)
{
	static int cnt = 0;	

	if (cnt) {
		printf("lc_open called multitime\n");
		panic("nnnnnnnnnn");
	}
	cnt++;

	f->f_devdata = (void *) KERN_ROMBASE;
	return 0;
}

int
lc_close(struct open_file *f)
{
	return 0;
}

ssize_t
lc_read (struct open_file *f, void *buf, size_t size, size_t *resid)
{
#define READ_CHUNK	0x10000
	u_int8_t	*addr = f->f_devdata;

	while (size >= READ_CHUNK) {
		twiddle();
		bcopy(addr, buf, READ_CHUNK);
		addr += READ_CHUNK;
		buf = READ_CHUNK + (u_int8_t *) buf;
		size -= READ_CHUNK;
	}
	twiddle();
	bcopy(addr, buf, size);
	f->f_devdata = addr + size;
	*resid = 0;
        return 0;
}

off_t
lc_seek (struct open_file *f, off_t offset, int where)
{
	switch (where) {
	case SEEK_SET:
		f->f_devdata = offset + (u_int8_t *) KERN_ROMBASE;
		return 0;

	case SEEK_CUR:
		f->f_devdata = offset + (u_int8_t *) f->f_devdata;
		return 0;

	case SEEK_END:
	default:
	}
        errno = EIO;
        return -1;
}

int
lcdevstrategy(devdata, rw, blk, size, buf, rsize)
	void *devdata;
	int rw;
	daddr_t blk;
	size_t size;
	void *buf;
	size_t *rsize;
{
	return EIO;
}

int
lcdevclose(struct open_file *f)
{
	return 0;
}

void
_rtt(void)
{
	for (;;)
		;
}
