/*	$NetBSD: bootxx.c,v 1.3 2002/04/30 01:14:40 tsutsui Exp $	*/

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include <machine/apcall.h>
#include <machine/romcall.h>

#define MAXBLOCKNUM 64

void (*entry_point)() = (void *)0;
int block_size = 8192;
int block_count = MAXBLOCKNUM;
int block_table[MAXBLOCKNUM] = { 0 };

#ifdef BOOTXX_DEBUG
# define DPRINTF(x) printf x
#else
# define DPRINTF(x)
#endif

char *devs[] = { "sd", "fh", "fd", NULL, NULL, "rd", "st" };
struct apbus_sysinfo *_sip;
int apbus = 0;

void
bootxx(a0, a1, a2, a3, a4, a5)
	u_int a0, a1, a2, a3, a4, a5;
{
	int fd, blk, bs;
	int ctlr, unit, part, type;
	int i;
	int bootdev = a1;
	char *addr;
	char devname[32];

	/*
	 * XXX a3 contains:
	 *     maxmem (nws-3xxx)
	 *     argv   (apbus-based machine)
	 */
	if (a3 & 0x80000000)
		apbus = 1;
	else
		apbus = 0;

	printf("NetBSD/newsmips Primary Boot\n");

	DPRINTF(("\n"));
	DPRINTF(("a0 %x\n", a0));
	DPRINTF(("a1 %x\n", a1));
	DPRINTF(("a2 %x (%s)\n", a2, (char *)a2));
	DPRINTF(("a3 %x\n", a3));
	DPRINTF(("a4 %x\n", a4));
	DPRINTF(("a5 %x\n", a5));

	DPRINTF(("block_size  = %d\n", block_size));
	DPRINTF(("block_count = %d\n", block_count));
	DPRINTF(("entry_point = %x\n", (int)entry_point));

	if (apbus) {
		strcpy(devname, (char *)a1);
		fd = apcall_open(devname, 0);
	} else {
		/* sd(ctlr, lun, part, bus?, host) */

		ctlr = BOOTDEV_CTLR(bootdev);
		unit = BOOTDEV_UNIT(bootdev);
		part = BOOTDEV_PART(bootdev);
		type = BOOTDEV_TYPE(bootdev);

		if (devs[type] == NULL) {
			printf("unknown bootdev (0x%x)\n", bootdev);
			return;
		}
		sprintf(devname, "%s(%d,%d,%d)", devs[type], ctlr, unit, part);

		fd = rom_open(devname, 0);
	}
	if (fd == -1) {
		printf("cannot open %s\n", devname);
		return;
	}

	addr = (char *)entry_point;
	bs = block_size;
	DPRINTF(("reading block:"));
	for (i = 0; i < block_count; i++) {
		blk = block_table[i];

		DPRINTF((" %d", blk));

		if (apbus) {
			apcall_lseek(fd, blk * 512, 0);
			apcall_read(fd, addr, bs);
		} else {
			rom_lseek(fd, blk * 512, 0);
			rom_read(fd, addr, bs);
		}
		addr += bs;
	}
	DPRINTF((" done\n"));
	if (apbus)
		apcall_close(fd);
	else
		rom_close(fd);

	(*entry_point)(a0, a1, a2, a3, _sip);
}

void
putchar(x)
	int x;
{
	char c = x;

	if (apbus)
		apcall_write(1, &c, 1);
	else
		rom_write(1, &c, 1);
}
