/*	$NetBSD: bootxx.c,v 1.9 2002/05/18 04:24:22 lukem Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <machine/bat.h>

#include <sys/bootblock.h>

int (*openfirmware)(void *);

				/*
				 * 32 KB of stack with 32 bytes overpad
				 * (see below)
				 */
int32_t __attribute__((aligned(16))) stack[8192 + 8];

struct shared_bbinfo bbinfo = {
	{ MACPPC_BBINFO_MAGIC },
	0,
	SHARED_BBINFO_MAXBLOCKS,
	{ 0 }
};

#ifndef DEFAULT_ENTRY_POINT
#define	DEFAULT_ENTRY_POINT	0x600000
#endif

void (*entry_point)(int, int, void *) = (void *)DEFAULT_ENTRY_POINT;


asm("
	.text
	.align 2
	.globl	_start
_start:

	lis	8,(_start)@ha
	addi	8,8,(_start)@l
	li	9,0x40		/* loop 64 times (for 2048 bytes of bootxx) */
	mtctr	9
1:
	dcbf	0,8
	icbi	0,8
	addi	8,8,0x20
	bdnz	1b
	sync

	li	0,0
	mtdbatu	3,0
	mtibatu	3,0
	isync
	li	8,0x1ffe	/* map the lowest 256MB */
	li	9,0x22		/* BAT_I */
	mtdbatl	3,9
	mtdbatu	3,8
	mtibatl	3,9
	mtibatu	3,8
	isync

				/*
				 * setup 32 KB of stack with 32 bytes overpad
				 * (see above)
				 */
	lis	1,(stack  + 4 * 8192)@ha
	addi	1,1,(stack+ 4 * 8192)@l
	stw	0,0(1)		/* terminate the frame link chain */

	b	startup
");


static __inline int
OF_finddevice(name)
	char *name;
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		char *device;
		int phandle;
	} args = {
		"finddevice",
		1,
		1,
	};	
	
	args.device = name;
	openfirmware(&args);

	return args.phandle;
}

static __inline int
OF_getprop(handle, prop, buf, buflen)
	int handle;
	char *prop;
	void *buf;
	int buflen;
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		int phandle;
		char *prop;
		void *buf;
		int buflen;
		int size;
	} args = {
		"getprop",
		4,
		1,
	};
	
	args.phandle = handle;
	args.prop = prop;
	args.buf = buf;
	args.buflen = buflen;
	openfirmware(&args);

	return args.size;
}

static __inline int
OF_open(dname)
	char *dname;
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		char *dname;
		int handle;
	} args = {
		"open",
		1,
		1,
	};
	
	args.dname = dname;
	openfirmware(&args);

	return args.handle;
}

static __inline int
OF_read(handle, addr, len)
	int handle;
	void *addr;
	int len;
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		int ihandle;
		void *addr;
		int len;
		int actual;
	} args = {
		"read",
		3,
		1,
	};

	args.ihandle = handle;
	args.addr = addr;
	args.len = len;
	openfirmware(&args);

	return args.actual;
}

static __inline int
OF_seek(handle, pos)
	int handle;
	u_quad_t pos;
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		int handle;
		int poshi;
		int poslo;
		int status;
	} args = {
		"seek",
		3,
		1,
	};
	
	args.handle = handle;
	args.poshi = (int)(pos >> 32);
	args.poslo = (int)pos;
	openfirmware(&args);

	return args.status;
}

static __inline int
OF_write(handle, addr, len)
	int handle;
	void *addr;
	int len;
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		int ihandle;
		void *addr;
		int len;
		int actual;
	} args = {
		"write",
		3,
		1,
	};

	args.ihandle = handle;
	args.addr = addr;
	args.len = len;
	openfirmware(&args);

	return args.actual;
}

int stdout;

void
putstrn(const char *s, size_t n)
{
	OF_write(stdout, s, n);
}

#define putstr(x)	putstrn((x),sizeof(x)-1)
#define putc(x)		do { char __x = (x) ; putstrn(&__x, 1); } while (0)


void
startup(arg1, arg2, openfirm)
	int arg1, arg2;
	void *openfirm;
{
	int fd, blk, chosen, options, i;
	char *addr;
	char bootpath[128];

	openfirmware = openfirm;

	chosen = OF_finddevice("/chosen");
	if (OF_getprop(chosen, "bootpath", bootpath, sizeof(bootpath)) == 1) {
		/*
		 * buggy firmware doesn't set bootpath...
		 */
		options = OF_finddevice("/options");
		OF_getprop(options, "boot-device", bootpath, sizeof(bootpath));
	}
	if (OF_getprop(chosen, "stdout", &stdout, sizeof(stdout))
	    != sizeof(stdout))
		stdout = -1;

	/*
	 * "scsi/sd@0:0" --> "scsi/sd@0"
	 */
	for (i = 0; i < sizeof(bootpath); i++) {
		if (bootpath[i] == ':')
			bootpath[i] = 0;
		if (bootpath[i] == 0)
			break;
	}

	putstr("\r\nOF_open bootpath=");
	putstrn(bootpath, i);
	fd = OF_open(bootpath);

	addr = (char *)entry_point;
	putstr("\r\nread stage 2 blocks: ");
	for (i = 0; i < bbinfo.bbi_block_count; i++) {
		if ((blk = bbinfo.bbi_block_table[i]) == 0)
			break;
		putc('0' + i % 10);
		OF_seek(fd, (u_quad_t)blk * 512);
		OF_read(fd, addr, bbinfo.bbi_block_size);
		addr += bbinfo.bbi_block_size;
	}
	putstr(". done!\r\nstarting stage 2...\r\n");

	/*
	 * enable D/I cache
	 */
	asm("
		mtdbatu	3,%0
		mtdbatl	3,%1
		mtibatu	3,%0
		mtibatl	3,%1
		isync
	" :: "r"(BATU(0, BAT_BL_256M, BAT_Vs)),
	     "r"(BATL(0, 0, BAT_PP_RW)));

	entry_point(0, 0, openfirm);
	for (;;);			/* just in case */
}
