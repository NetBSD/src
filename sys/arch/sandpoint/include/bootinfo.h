/*	$NetBSD: bootinfo.h,v 1.8.18.1 2017/12/03 11:36:40 jdolecek Exp $	*/

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

#ifndef	_MACHINE_BOOTINFO_H_
#define _MACHINE_BOOTINFO_H_

#define BOOTINFO_MAGIC	0xb007babe

struct btinfo_common {
	int next;		/* offset of next item, or zero */
	int type;
};

#define BTINFO_MAGIC		1
#define BTINFO_MEMORY		2
#define BTINFO_CONSOLE		3
#define BTINFO_CLOCK		4
#define BTINFO_BOOTPATH		5
#define BTINFO_ROOTDEVICE	6
#define BTINFO_NET		7
#define BTINFO_PRODFAMILY	8
#define BTINFO_MODULELIST	9
#define BTINFO_MODEL		10

struct btinfo_magic {
	struct btinfo_common common;
	unsigned magic;
};

struct btinfo_memory {
	struct btinfo_common common;
	int memsize;
};

struct btinfo_console {
	struct btinfo_common common;
	char devname[8];
	int addr;
	int speed;
};

struct btinfo_clock {
	struct btinfo_common common;
	int ticks_per_sec;
};

struct btinfo_bootpath {
	struct btinfo_common common;
	char bootpath[80];
};

struct btinfo_rootdevice {
	struct btinfo_common common;
	char devname[16];
	unsigned cookie;
};

struct btinfo_net {
	struct btinfo_common common;
	char devname[16];
	unsigned cookie;
	uint8_t mac_address[6];
};

struct btinfo_prodfamily {
	struct btinfo_common common;
	char name[24];
};

struct btinfo_model {
	struct btinfo_common common;
	char name[28];
	unsigned flags;			/* model specific flags */
/* Synology flags: */
#define BI_MODEL_CPLDVER_MASK	0x07
#define BI_MODEL_CPLD207	0x08
#define BI_MODEL_CPLD209	0x10
#define BI_MODEL_CPLD406	0x18
#define BI_MODEL_CPLD407	0x20
#define BI_MODEL_CPLD_MASK	0x38
#define BI_MODEL_THERMAL	0x40
};

struct btinfo_modulelist {
	struct btinfo_common common;
	int num;
	uint32_t endpa;
	/* bi_modulelist_entry follows as an array */
};

struct bi_modulelist_entry {
	char kmod[80];
	int type;
#define BI_MODULE_NONE		0x00
#define BI_MODULE_ELF		0x01
	int len;
	uint32_t base;
};

#define BOOTINFO_MAXSIZE 4096

#ifdef _KERNEL
void *lookup_bootinfo(int);
#endif

#endif	/* _MACHINE_BOOTINFO_H_ */
