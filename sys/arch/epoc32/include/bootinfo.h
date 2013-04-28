/*	$NetBSD: bootinfo.h,v 1.1 2013/04/28 12:11:25 kiyohara Exp $	*/
/*
 * Copyright (c) 2012 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _EPOC32_BOOTINFO_H_
#define _EPOC32_BOOTINFO_H_


#ifndef _LOCORE
struct btinfo_common {
	int len;
	int type;
};
#endif

#define BTINFO_NONE	0
#define BTINFO_MODEL	1
#define BTINFO_MEMORY	2
#define BTINFO_VIDEO	3

#define BTINFO_MAX_SIZE	512

#ifndef _LOCORE
struct btinfo_model {
	struct btinfo_common common;
	char model[16];
};

struct btinfo_memory {
	struct btinfo_common common;
	int address;
	int size;	/* Kbytes */
};

struct btinfo_video {
	struct btinfo_common common;
	int width;
	int height;
};
#endif	/* _LOCORE */

#endif	/* _EPOC32_BOOTINFO_H_ */
