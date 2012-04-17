/*	$NetBSD: xx.c,v 1.1.2.2 2012/04/17 00:07:03 yamt Exp $	*/

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

struct open_file;
extern void RAW_READ __P((void *buf, u_int32_t blkpos, size_t bytelen));
int xxopen(struct open_file *);
int xxclose(struct open_file *);
int xxstrategy(void *, int, daddr_t, size_t, void *, size_t *);

int
xxopen(struct open_file *f)
{
	return 0;
}

int
xxclose(struct open_file *f)
{
	return 0;
}

extern unsigned int SCSI_BLKLEN;
int
xxstrategy(void *arg, int rw, daddr_t dblk, size_t size,
           void *buf, size_t *rsize)
{
	RAW_READ(buf, (u_int32_t)dblk, size);
	if (rsize)
		*rsize = size;
	return 0;
}
