/*	$NetBSD: bootinfo.h,v 1.3.8.1 2006/05/24 10:56:39 yamt Exp $	*/

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

#ifndef _COBALT_BOOTINFO_H_
#define _COBALT_BOOTINFO_H_

#define BOOTINFO_MAGIC	0xb007babe
#define BOOTINFO_SIZE	1024

struct btinfo_common {
	int next;		/* offset of next item, or zero */
	int type;
};

#define BTINFO_MAGIC	1
#define BTINFO_BOOTPATH 2
#define BTINFO_SYMTAB	3
#define BTINFO_FLAGS	4

struct btinfo_magic {
	struct btinfo_common common;
	int magic;
};

#define BTINFO_BOOTPATH_LEN	80
struct btinfo_bootpath {
	struct btinfo_common common;
	char bootpath[BTINFO_BOOTPATH_LEN];
};

struct btinfo_symtab {
	struct btinfo_common common;
	int nsym;
	int ssym;
	int esym;
};

struct btinfo_flags {
	struct btinfo_common common;

#define	BI_SERIAL_CONSOLE	0x1
	uint32_t bi_flags;
};

#ifdef _KERNEL
void	*lookup_bootinfo __P((int));
#endif

#endif	/* !_COBALT_BOOTINFO_H_ */
