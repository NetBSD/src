/*	$NetBSD: bootinfo.h,v 1.1 1997/09/17 17:39:30 drochner Exp $	*/

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
 *
 */

#ifdef notyet
#include <machine/bootinfo.h>
#else
struct btinfo_common {
	int len;
	int type;
};

#define BTINFO_BOOTPATH 0
#define BTINFO_BOOTDISK 3
#define BTINFO_NETIF 4
#define BTINFO_CONSOLE 6
#define BTINFO_BIOSGEOM 7

struct btinfo_bootpath {
	struct btinfo_common common;
	char bootpath[80];
};

struct btinfo_bootdisk {
	struct btinfo_common common;
	int labelsector; /* label valid if != -1 */
	struct {
		u_int16_t type, checksum;
		char packname[16];
	} label;
	int biosdev;
	int partition;
};

struct btinfo_netif {
	struct btinfo_common common;
	char ifname[16];
	int bus, addr;
#define BI_BUS_ISA 0
#define BI_BUS_PCI 1
	char hw_addr[6];
	char dummy[2]; /* align */
};

struct btinfo_console {
	struct btinfo_common common;
	char devname[16];
	int addr;
	int speed;
};

#include <machine/disklabel.h>

struct bi_biosgeom_entry {
	int spc, spt;
	struct dos_partition dosparts[NDOSPART];
};

struct btinfo_biosgeom {
	struct btinfo_common common;
	int num;
	struct bi_biosgeom_entry disk[1]; /* var len */
};
#endif /* notyet */

struct bootinfo {
	int nentries;
	physaddr_t entry[1];
};

extern struct bootinfo *bootinfo;

#define BI_ALLOC(max) (bootinfo = alloc(sizeof(struct bootinfo) \
                                        + ((max) - 1) * sizeof(physaddr_t))) \
                      ->nentries = 0

#define BI_FREE() free(bootinfo, 0)

#define BI_ADD(x, type, size) bi_add((struct btinfo_common*)(x), type, size)

void bi_add __P((struct btinfo_common*, int, int));
void bi_getbiosgeom __P((void));
