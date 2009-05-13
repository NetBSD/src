/*      $NetBSD: bootinfo.h,v 1.2.68.1 2009/05/13 17:18:51 jym Exp $	*/

/*-
 * Copyright (c) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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

#ifndef	_ZAURUS_BOOTINFO_H_
#define	_ZAURUS_BOOTINFO_H_

#define BOOTARGS_BUFSIZ	256
#define	BOOTARGS_MAGIC	0x4f425344

#define BOOTINFO_MAXSIZE (BOOTARGS_BUFSIZ - sizeof(uint32_t)) /* uint32_t for magic */

#define BTINFO_BOOTDISK		0
#define BTINFO_HOWTO		1
#define BTINFO_CONSDEV		2
#define BTINFO_ROOTDEVICE	3
#define BTINFO_MAX		4

#ifndef	_LOCORE

struct btinfo_common {
	int len;
	int type;
};

struct btinfo_bootdisk {
	struct btinfo_common common;
	int labelsector; /* label valid if != -1 */
	struct {
		uint16_t type, checksum;
		char packname[16];
	} label;
	int biosdev;
	int partition;
};

struct btinfo_howto {
	struct btinfo_common common;
	u_int howto;
};

struct btinfo_console {
	struct btinfo_common common;
	char devname[16];
	int addr;
	int speed;
};

struct btinfo_rootdevice {
	struct btinfo_common common;
	char devname[16];
};

struct bootinfo {
        int nentries;
	char info[BOOTINFO_MAXSIZE - sizeof(int)];
};

#endif /* _LOCORE */

#ifdef _KERNEL
void *lookup_bootinfo(int type);
#endif

#endif	/* _ZAURUS_BOOTINFO_H_ */
