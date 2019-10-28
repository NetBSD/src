/*	$NetBSD: bootxx.c,v 1.20 2019/10/28 18:13:40 joerg Exp $	*/

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
#include <powerpc/oea/bat.h>
#include <powerpc/oea/spr.h>

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
#define	DEFAULT_ENTRY_POINT	0xE00000
#endif

void (*entry_point)(int, int, void *) = (void *)DEFAULT_ENTRY_POINT;


__asm(
"	.text			\n"
"	.align 2		\n"
"	.globl	_start		\n"
"_start:			\n"

"	lis	%r8,(_start)@ha	\n"
"	addi	%r8,8,(_start)@l\n"
"	li	%r9,0x40	\n"	/* loop 64 times (for 2048 bytes of bootxx) */
"	mtctr	%r9		\n"
"				\n"
"1:	dcbf	%r0,%r8		\n"
"	icbi	%r0,%r8		\n"
"	addi	%r8,%r8,0x20	\n"
"	bdnz	1b		\n"
"	sync			\n"

"	li	%r0,0		\n"
"				\n"	/* test for 601 cpu */
"	mfspr	%r9,287		\n"	/* mfpvbr %r9 PVR = 287 */
"	srwi	%r9,%r9,0x10	\n"
"	cmplwi	%r9,0x02	\n"	/* 601 cpu == 0x0001 */
"	blt	2f		\n"	/* skip over non-601 BAT setup */
"				\n"
"	mtdbatu 3,%r0		\n"	/* non-601 BAT */
"	mtibatu	3,%r0		\n"
"	isync			\n"
"	li	%r8,0x1ffe	\n"	/* map the lowest 256MB */
"	li	%r9,0x22	\n"	/* BAT_I */
"	mtdbatl	3,%r9		\n"
"	mtdbatu	3,%r8		\n"
"	mtibatl	3,%r9		\n"
"	mtibatu	3,%r8		\n"
"	isync			\n"
"	b	3f		\n"
"				\n"
"2:	mfmsr	%r8		\n"	/* 601 BAT */
"	mtmsr	%r0		\n"
"	isync			\n"
"				\n"
"	mtibatu 0,%r0		\n"
"	mtibatu 1,%r0		\n"
"	mtibatu 2,%r0		\n"
"	mtibatu 3,%r0		\n"
"				\n"
"	li	%r9,0x7f	\n"
"	mtibatl 0,%r9		\n"
"	li	%r9,0x1a	\n"
"	mtibatu 0,%r9		\n"
"				\n"
"	lis %r9,0x80		\n"
"	addi %r9,%r9,0x7f	\n"
"	mtibatl 1,%r9		\n"
"	lis %r9,0x80		\n"
"	addi %r9,%r9,0x1a	\n"
"	mtibatu 1,%r9		\n"
"				\n"
"	lis %r9,0x100		\n"
"	addi %r9,%r9,0x7f	\n"
"	mtibatl 2,%r9		\n"
"	lis %r9,0x100		\n"
"	addi %r9,%r9,0x1a	\n"
"	mtibatu 2,%r9		\n"
"				\n"
"	lis %r9,0x180		\n"
"	addi %r9,%r9,0x7f	\n"
"	mtibatl 3,%r9		\n"
"	lis %r9,0x180		\n"
"	addi %r9,%r9,0x1a	\n"
"	mtibatu 3,%r9		\n"
"				\n"
"	isync			\n"
"				\n"
"	mtmsr	%r8		\n"
"	isync			\n"
"				\n"
	/*
	 * setup 32 KB of stack with 32 bytes overpad (see above)
	 */
"3:	lis	%r1,(stack+32768)@ha\n"
"	addi	%r1,%r1,(stack+32768)@l\n"
	/*
	 * terminate the frame link chain,
	 * clear by bytes to avoid ppc601 alignment exceptions
	 */
"	stb	%r0,0(%r1)	\n"
"	stb	%r0,1(%r1)	\n"
"	stb	%r0,2(%r1)	\n"
"	stb	%r0,3(%r1)	\n"

"	b	startup		\n"
);


static inline int
OF_finddevice(char *name)
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

static inline int
OF_getprop(int handle, char *prop, void *buf, int buflen)
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

static inline int
OF_open(char *dname)
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

static inline int
OF_read(int handle, void *addr, int len)
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

static inline int
OF_seek(int handle, u_quad_t pos)
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

static inline int
OF_write(int handle, const void *addr, int len)
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		int ihandle;
		const void *addr;
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

static void
putstrn(const char *s, size_t n)
{
	OF_write(stdout, s, n);
}

#define putstr(x)	putstrn((x),sizeof(x)-1)
#define putc(x)		do { char __x = (x) ; putstrn(&__x, 1); } while (0)


void
startup(int arg1, int arg2, void *openfirm)
{
	int fd, blk, chosen, options, j;
	uint32_t cpuvers;
	size_t i;
	char *addr;
	char bootpath[128];

	__asm volatile ("mfpvr %0" : "=r"(cpuvers));
	cpuvers >>= 16;

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
	for (j = 0; j < bbinfo.bbi_block_count; j++) {
		if ((blk = bbinfo.bbi_block_table[j]) == 0)
			break;
		putc('0' + j % 10);
		OF_seek(fd, (u_quad_t)blk * 512);
		OF_read(fd, addr, bbinfo.bbi_block_size);
		addr += bbinfo.bbi_block_size;
	}
	putstr(". done!\r\nstarting stage 2...\r\n");

	if (cpuvers != MPC601) {
		/*
		 * enable D/I cache
		 */
		__asm(
			"mtdbatu	3,%0\n\t"
			"mtdbatl	3,%1\n\t"
			"mtibatu	3,%0\n\t"
			"mtibatl	3,%1\n\t"
			"isync"
		::	"r"(BATU(0, BAT_BL_256M, BAT_Vs)),
			"r"(BATL(0, 0, BAT_PP_RW)));
	}

	entry_point(0, 0, openfirm);
	for (;;);			/* just in case */
}
