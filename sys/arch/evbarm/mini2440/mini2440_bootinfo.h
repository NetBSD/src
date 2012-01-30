/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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
/* arch/sandpoint/include/bootinfo.h has served as base for this file */
#ifndef _MINI2440_BOOTINFO_H
#define _MINI2440_BOOTINFO_H

#include <machine/bootconfig.h>

#define BOOTINFO_MAGIC 0xb007babe
#define BOOTINFO_MAXSIZE 4096

struct btinfo_common {
	int next;
	int type;
};

#define BTINFO_MAGIC		1
#define BTINFO_SYMTAB		2
#define BTINFO_BOOTSTRING	3
#define BTINFO_BOOTPATH		4
#define BTINFO_ROOTDEVICE	5
#define BTINFO_NET		6

struct btinfo_magic {
	struct btinfo_common	common;
	unsigned int		magic;
};

struct btinfo_symtab {
	struct btinfo_common	common;
	int			nsym;
	void			*ssym;
	void			*esym;
};

struct btinfo_bootstring {
	struct btinfo_common	common;
	char			bootstring[MAX_BOOT_STRING];
};

struct btinfo_bootpath {
	struct btinfo_common	common;
	char			bootpath[80];
};

struct btinfo_rootdevice {
	struct btinfo_common	common;
	char			devname[16];
	unsigned int		cookie;
	unsigned int		partition;
};

struct btinfo_net {
	struct btinfo_common	common;
	char			devname[16];
	unsigned		cookie;
	uint8_t			mac_address[6];
};

#endif
