/*	$NetBSD: bootinfo.h,v 1.4 1999/12/14 22:39:14 thorpej Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 * 4. The name of the author may not be used to endorse or promote products
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

#define BOOTINFO_MAGIC1	0x04191973
#define	BOOTINFO_MAGIC2	0x02241973
#define BOOTINFO_SIZE	1024

struct btinfo_common {
	int next;		/* offset of next item, or zero */
	int type;
};

#define BTINFO_MAGIC	1
#define BTINFO_BOOTPATH 2
#define BTINFO_SYMTAB	3
#define	BTINFO_CONSOLE	4

struct btinfo_magic {
	struct btinfo_common common;
	int magic1;
	int magic2;
};

#define BTINFO_FILENAME_LEN		64
#define	BTINFO_BOOTPATH_TYPE_SCSI	0
#define	BTINFO_BOOTPATH_TYPE_HPIB	1
#define	BTINFO_BOOTPATH_TYPE_NET	2
struct btinfo_bootpath {
	struct btinfo_common common;
	int type;
	int scode;
	int target;
	int lun;
	char filename[BTINFO_FILENAME_LEN];
};

struct btinfo_symtab {
	struct btinfo_common common;
	int ssym;
	int esym;
};

#define	BTINFO_CONSOLE_INTIO_FB		-1
#define	BTINFO_CONSOLE_APCI		-2
#define	BTINFO_CONSOLE_SGC_FB		-3
#define	BTINFO_CONSOLE_HIL_KBD		0
#define	BTINFO_CONSOLE_DOMAIN_KBD	1
struct btinfo_console {
	int scode;
	int keyboard;
};

#ifdef _KERNEL
void *lookup_bootinfo __P((int));
#endif
