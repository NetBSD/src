/* $NetBSD: dev_flash.c,v 1.3.92.1 2009/05/13 17:17:46 jym Exp $ */

/*
 * Copyright (c) 2003 Naoto Shimazaki.
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
 * THIS SOFTWARE IS PROVIDED BY NAOTO SHIMAZAKI AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE NAOTO OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dev_flash.c,v 1.3.92.1 2009/05/13 17:17:46 jym Exp $");

#include <sys/param.h>
#include <lib/libsa/stand.h>
#include <machine/stdarg.h>

#include "extern.h"

#define READ_CHUNK	0x10000

int
flash_strategy(void *devdata, int rw, daddr_t blk,
	       size_t size, void *buf , size_t *rsize)
{
	u_int8_t	*src;
	size_t		count;

	if (rw != F_READ)
		return EIO;

	src = (u_int8_t *) KERN_ROMBASE + dbtob(blk);
	count = size < READ_CHUNK ? size : READ_CHUNK;
	memcpy( buf, src, count);
	*rsize = count;
        return 0;
}

int
flash_open(struct open_file *f, ...)
{
	char	*fname;
	char	**file;
	va_list	ap;

	va_start(ap, f);
	fname = va_arg(ap, char *);
	file = va_arg(ap, char **);
	va_end(ap);

	*file = NULL;

	return 0;
}

int
flash_close(struct open_file *f)
{
	return 0;
}

int
flash_ioctl(struct open_file *f, u_long cmd, void *data)
{
	return EIO;
}
