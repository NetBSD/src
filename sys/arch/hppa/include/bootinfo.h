/*	$NetBSD: bootinfo.h,v 1.1.10.2 2014/08/20 00:03:05 tls Exp $	*/

/*
 * Copyright (c) 1997
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

#define BOOTINFO_MAGIC	0xb007babe

#ifndef _LOCORE

struct btinfo_common {
	int len;
	int type;
};

#define BTINFO_MAGIC		0
#define BTINFO_KERNELFILE	1
#define BTINFO_SYMTAB		2

struct btinfo_magic {
	struct btinfo_common common;
	int magic;
};

struct btinfo_kernelfile {
	struct btinfo_common common;
	char name[1];	/* variable length */
};

struct btinfo_symtab {
	struct btinfo_common common;
	int nsym;
	int ssym;
	int esym;
};

#endif /* _LOCORE */

#ifdef _KERNEL

#define BOOTINFO_MAXSIZE 1024
#define BOOTINFO_DATASIZE (BOOTINFO_MAXSIZE - 2 * sizeof(uint32_t))

#ifndef _LOCORE
/*
 * Structure that holds the information passed by the boot loader.
 */
struct bootinfo {
	/* Number of bootinfo_* entries in bi_data. */
	uint32_t	bi_nentries;
	uint32_t	bi_offset;

	/* Raw data of bootinfo entries.  The first one (if any) is
	 * found at bi_data[0] and can be casted to (bootinfo_common *).
	 * Once this is done, the following entry is found at 'len'
	 * offset as specified by the previous entry. */
	uint8_t		bi_data[BOOTINFO_DATASIZE];
};

void *lookup_bootinfo(int);
#endif /* _LOCORE */

#endif /* _KERNEL */
