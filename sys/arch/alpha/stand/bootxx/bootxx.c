/* $NetBSD: bootxx.c,v 1.15 1999/05/26 06:22:03 cgd Exp $ */

/*
 * Copyright (C) 1998 by Ross Harvey
 *
 * This work was supported by Ross Harvey, Avalon Computer Systems, and The
 * NetBSD Foundation.
 *
 * This work may be distributed under the terms of the original license
 * below, provided that this notice is not changed.  ROSS HARVEY,
 * AVALON COMPUTER SYSTEMS, INC., AND THE NETBSD FOUNDATION DISCLAIM
 * ALL LIABILITY OF ANY KIND FOR THE USE OF THIS SOFTWARE.
 */
/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/param.h>

#include <machine/prom.h>
#include <lib/libkern/libkern.h>
#include "stand/common/common.h"
#include "stand/common/bbinfo.h"

extern int _end, start;

struct bbinfoloc desc = {
	0xbabefacedeadbeef,
	(u_int64_t)&start,
	(u_int64_t)&_end,
	{ 0, },
	0xdeadbeeffacebabe
};

#ifdef PBDEBUG
static void
errorstatus(const char *msg, u_int64_t val)
{
	int	i, c;

	putstr(msg);
	for(i=60; i >= 0; i -= 4) {
		c = val >> i & 0xf;
		if (c >= 10)
			c = c - 10 + 'a';
		else	c += '0';
		putchar(c);
	}
}
#endif

static int
load_file(fd, bbinfop, loadaddr)
	int fd;
	struct bbinfo *bbinfop;
	char *loadaddr;
{
	char *cp;
	int twiddle;
	prom_return_t ret;
	int32_t cksum, *int32p;
	int i, j, n, rv, nextblk, wantblk, blksize;

	if (bbinfop->nblocks <= 0) {
		putstr("invalid number of blocks in boot program description\n");
		return 0;
	}
	if (bbinfop->bsize < DEV_BSIZE || bbinfop->bsize > MAXBSIZE) {
		putstr("invalid block size in boot program description\n");
		return 0;
	}

	int32p = (int32_t *)&_end;
	n = bbinfop->nblocks +
	    (sizeof(*bbinfop) / sizeof(bbinfop->blocks[0])) - 1;
	if ((long)&_end - (long)&start + sizeof(bbinfop->blocks[0]) * n >
	    15 * 512) {
		putstr("way too many blocks\n");
		return 0;
	}

	for (i = 0, cksum = 0; i < n; i++)
		cksum += *int32p++;
	if (cksum != 0) {
		putstr("invalid checksum in boot program description\n");
		return 0;
	}

	cp = loadaddr;
	rv = 1;
	nextblk = 16;
	for (i = twiddle = 0; i < bbinfop->nblocks; i++) {
		wantblk = bbinfop->blocks[i];
		blksize = bbinfop->bsize;
		for(j = 1; i + 1 < bbinfop->nblocks
		   && bbinfop->blocks[i + 1] == wantblk + j
		   && blksize < 50 * 1024; ++i, ++j)
			blksize += bbinfop->bsize;
		for(; nextblk < wantblk && wantblk - nextblk <= 4; ++nextblk) {
			putchar('+');
			prom_read(fd, 512, cp, nextblk);
		}
		putchar(".oOo"[twiddle++ & 3]);
		ret.bits = prom_read(fd, blksize, cp, wantblk);
		putchar('\b');
		nextblk += blksize / 512;
		cp      += blksize;
		if (ret.u.status) {
			rv = 0;
			putstr("\nBLOCK READ ERROR!\n");
			break;
		}
	}
	putstr(".\n");

	return (rv);
}

static void printdec(int n)
{
	if (n)
		printdec(n/10);
	putchar(n % 10 | '0');
}

void
main()
{
	char *loadaddr;
	struct bbinfo *bbinfop;
	void (*entry) __P((int));

	/* Init prom callback vector. */
	init_prom_calls();

	putstr("\nNetBSD/alpha " NETBSD_VERS " Primary Boot +\n");

	bbinfop = (struct bbinfo *)&_end;
	loadaddr = (char *)SECONDARY_LOAD_ADDRESS;

	if (!booted_dev_open()) {
		putstr("Can't open boot device\n");
		return;
	}
	if (!load_file(booted_dev_fd, bbinfop, loadaddr)) {
		putstr("\nLOAD FAILED!\n\n");
		return;
	}

	putstr("Jumping to entry point...\n");
	entry = (void (*)(int))loadaddr;
	(*entry)(booted_dev_fd);
	booted_dev_close();
	putstr("SECONDARY BOOT RETURNED!\n");
}
