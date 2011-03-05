/*	$NetBSD: bootinfo.h,v 1.2.98.1 2011/03/05 15:09:53 bouyer Exp $	*/

/*
 * Copyright (c) 1997, 2000-2004
 *	Matthias Drochner.  All rights reserved.
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
 *
 */

#ifndef _MMEYE_BOOTINFO_H_
#define _MMEYE_BOOTINFO_H_

#define BOOTINFO_MAGIC	0xb007babe
#define BOOTINFO_SIZE	1024

struct btinfo_common {
	int32_t next;		/* offset of next item, or zero */
	int32_t type;
};

#define BTINFO_MAGIC	1
#define BTINFO_BOOTDEV	2
#define BTINFO_BOOTPATH	3
#define BTINFO_HOWTO	4

struct btinfo_magic {
	struct btinfo_common common;
	int32_t magic;
};

#define BTINFO_BOOTDEV_LEN	8
struct btinfo_bootdev {
	struct btinfo_common common;
	char bootdev[BTINFO_BOOTDEV_LEN];
};

#define BTINFO_BOOTPATH_LEN	80
struct btinfo_bootpath {
	struct btinfo_common common;
	char bootpath[BTINFO_BOOTPATH_LEN];
};

struct btinfo_howto {
	struct btinfo_common common;

	uint32_t bi_howto;
};

#ifdef _KERNEL
void	*lookup_bootinfo(unsigned int);
#endif

#endif	/* !_MMEYE_BOOTINFO_H_ */
