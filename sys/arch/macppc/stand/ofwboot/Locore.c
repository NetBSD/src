/*	$NetBSD: Locore.c,v 1.32 2018/08/17 16:04:39 macallan Exp $	*/

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

#include <sys/param.h>
#include <lib/libsa/stand.h>

#include <machine/cpu.h>
#include <powerpc/oea/spr.h>

#include "openfirm.h"

static int (*openfirmware)(void *);

static void startup(void *, int, int (*)(void *), char *, int)
		__attribute__((__used__));
static void setup(void);

#ifdef HEAP_VARIABLE
#ifndef HEAP_SIZE
#define HEAP_SIZE 0x20000
#endif
char *heapspace;
#endif

static int stack[8192/4 + 4] __attribute__((__used__));

#ifdef XCOFF_GLUE
__asm(
"	.text			\n"
"	.globl	_entry		\n"
"_entry:			\n"
"	.long	_start,0,0	\n"
);
#endif /* XCOFF_GLUE */

__asm(
"	.text			\n"
"	.globl	_start		\n"
"_start:			\n"
"	sync			\n"
"	isync			\n"
"	lis	%r1,stack@ha	\n"
"	addi	%r1,%r1,stack@l	\n"
"	addi	%r1,%r1,8192	\n"
"				\n"
"	mfmsr	%r8		\n"
"	li	%r0,0		\n"
"	mtmsr	%r0		\n"
"	isync			\n"
"				\n"
"				\n" /* test for 601 */
"	mfspr	%r0,287		\n" /* mfpvbr %r0 PVR = 287 */
"	srwi	%r0,%r0,0x10	\n"
"	cmpi	0,1,%r0,0x02	\n" /* 601 CPU = 0x0001 */
"	blt	2f		\n" /* skip over non-601 BAT setup */
"	cmpi	0,1,%r0,0x39	\n" /* PPC970 */
"	blt	0f		\n"
"	cmpi	0,1,%r0,0x45	\n" /* PPC970GX */
"	ble	1f		\n"
	/* non PPC 601 BATs */
"0:	li	%r0,0		\n"
"	mtibatu	0,%r0		\n"
"	mtibatu	1,%r0		\n"
"	mtibatu	2,%r0		\n"
"	mtibatu	3,%r0		\n"
"	mtdbatu	0,%r0		\n"
"	mtdbatu	1,%r0		\n"
"	mtdbatu	2,%r0		\n"
"	mtdbatu	3,%r0		\n"
"				\n"
"	li	%r9,0x12	\n"	/* BATL(0, BAT_M, BAT_PP_RW) */
"	mtibatl	0,%r9		\n"
"	mtdbatl	0,%r9		\n"
"	li	%r9,0x1ffe	\n"	/* BATU(0, BAT_BL_256M, BAT_Vs) */
"	mtibatu	0,%r9		\n"
"	mtdbatu	0,%r9		\n"
"	b	3f		\n"
	/* 970 initialization stuff */
"1:				\n"
	/* make sure we're in bridge mode */
"	clrldi	%r8,%r8,3	\n"
"	mtmsrd	%r8		\n"
"	isync			\n"
	 /* clear HID5 DCBZ bits (56/57), need to do this early */
"	mfspr	%r9,0x3f6	\n"
"	rldimi	%r9,0,6,56	\n"
"	sync			\n"
"	mtspr	0x3f6,%r9	\n"
"	isync			\n"
"	sync			\n"
	/* Setup HID1 features, prefetch + i-cacheability controlled by PTE */
"	mfspr	%r9,0x3f1	\n"
"	li	%r11,0x1200	\n"
"	sldi	%r11,%r11,44	\n"
"	or	%r9,%r9,%r11	\n"
"	mtspr	0x3f1,%r9	\n"
"	isync			\n"
"	sync			\n"
"	b	3f		\n"	
	/* PPC 601 BATs */
"2:	li	%r0,0		\n"
"	mtibatu	0,%r0		\n"
"	mtibatu	1,%r0		\n"
"	mtibatu	2,%r0		\n"
"	mtibatu	3,%r0		\n"
"				\n"
"	li	%r9,0x7f	\n"
"	mtibatl	0,%r9		\n"
"	li	%r9,0x1a	\n"
"	mtibatu	0,%r9		\n"
"				\n"
"	lis	%r9,0x80	\n"
"	addi	%r9,%r9,0x7f	\n"
"	mtibatl	1,%r9		\n"
"	lis	%r9,0x80	\n"
"	addi	%r9,%r9,0x1a	\n"
"	mtibatu	1,%r9		\n"
"				\n"
"	lis	%r9,0x100	\n"
"	addi	%r9,%r9,0x7f	\n"
"	mtibatl	2,%r9		\n"
"	lis	%r9,0x100	\n"
"	addi	%r9,%r9,0x1a	\n"
"	mtibatu	2,%r9		\n"
"				\n"
"	lis	%r9,0x180	\n"
"	addi	%r9,%r9,0x7f	\n"
"	mtibatl	3,%r9		\n"
"	lis	%r9,0x180	\n"
"	addi	%r9,%r9,0x1a	\n"
"	mtibatu	3,%r9		\n"
"				\n"
"3:	isync			\n"
"				\n"
"	mtmsr	%r8		\n"
"	isync			\n"
"				\n"
	/*
	 * Make sure that .bss is zeroed
	 */
"				\n"
"	li	%r0,0		\n"
"	lis	%r8,_edata@ha	\n"
"	addi	%r8,%r8,_edata@l\n"
"	lis	%r9,_end@ha	\n"
"	addi	%r9,%r9,_end@l	\n"
"				\n"
"5:	cmpw	0,%r8,%r9	\n"
"	bge	6f		\n"
"	stw	%r0,0(%r8)	\n"
"	addi	%r8,%r8,4	\n"
"	b	5b		\n"
"				\n"
"6:	b	startup		\n"
);

#if 0
static int
openfirmware(void *arg)
{

	__asm volatile ("sync; isync");
	openfirmware_entry(arg);
	__asm volatile ("sync; isync");
}
#endif

static void
startup(void *vpd, int res, int (*openfirm)(void *), char *arg, int argl)
{

	openfirmware = openfirm;
	setup();
	main();
	OF_exit();
}

#if 0
void
OF_enter(void)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
	} args = {
		"enter",
		0,
		0
	};

	openfirmware(&args);
}
#endif	/* OF_enter */

__dead void
OF_exit(void)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
	} args = {
		"exit",
		0,
		0
	};

	openfirmware(&args);
	for (;;);			/* just in case */
}

int
OF_finddevice(const char *name)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		const char *device;
		int phandle;
	} args = {
		"finddevice",
		1,
		1,
	};

	args.device = name;
	if (openfirmware(&args) == -1)
		return -1;
	return args.phandle;
}

int
OF_instance_to_package(int ihandle)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int ihandle;
		int phandle;
	} args = {
		"instance-to-package",
		1,
		1,
	};

	args.ihandle = ihandle;
	if (openfirmware(&args) == -1)
		return -1;
	return args.phandle;
}

int
OF_getprop(int handle, const char *prop, void *buf, int buflen)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		const char *prop;
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
	if (openfirmware(&args) == -1)
		return -1;
	return args.size;
}

#ifdef	__notyet__	/* Has a bug on FirePower */
int
OF_setprop(int handle, const char *prop, void *buf, int len)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		const char *prop;
		void *buf;
		int len;
		int size;
	} args = {
		"setprop",
		4,
		1,
	};

	args.phandle = handle;
	args.prop = prop;
	args.buf = buf;
	args.len = len;
	if (openfirmware(&args) == -1)
		return -1;
	return args.size;
}
#endif

int
OF_open(const char *dname)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		const char *dname;
		int handle;
	} args = {
		"open",
		1,
		1,
	};

#ifdef OFW_DEBUG
	printf("OF_open(%s) -> ", dname);
#endif
	args.dname = dname;
	if (openfirmware(&args) == -1 ||
	    args.handle == 0) {
#ifdef OFW_DEBUG
		printf("lose\n");
#endif
		return -1;
	}
#ifdef OFW_DEBUG
	printf("%d\n", args.handle);
#endif
	return args.handle;
}

void
OF_close(int handle)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int handle;
	} args = {
		"close",
		1,
		0,
	};

#ifdef OFW_DEBUG
	printf("OF_close(%d)\n", handle);
#endif
	args.handle = handle;
	openfirmware(&args);
}

int
OF_write(int handle, void *addr, int len)
{
	static struct {
		const char *name;
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

#ifdef OFW_DEBUG
	if (len != 1)
		printf("OF_write(%d, %p, %x) -> ", handle, addr, len);
#endif
	args.ihandle = handle;
	args.addr = addr;
	args.len = len;
	if (openfirmware(&args) == -1) {
#ifdef OFW_DEBUG
		printf("lose\n");
#endif
		return -1;
	}
#ifdef OFW_DEBUG
	if (len != 1)
		printf("%x\n", args.actual);
#endif
	return args.actual;
}

int
OF_read(int handle, void *addr, int len)
{
	static struct {
		const char *name;
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

#ifdef OFW_DEBUG
	if (len != 1)
		printf("OF_read(%d, %p, %x) -> ", handle, addr, len);
#endif
	args.ihandle = handle;
	args.addr = addr;
	args.len = len;
	if (openfirmware(&args) == -1) {
#ifdef OFW_DEBUG
		printf("lose\n");
#endif
		return -1;
	}
#ifdef OFW_DEBUG
	if (len != 1)
		printf("%x\n", args.actual);
#endif
	return args.actual;
}

int
OF_seek(int handle, u_quad_t pos)
{
	static struct {
		const char *name;
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

#ifdef OFW_DEBUG
	printf("OF_seek(%d, %x, %x) -> ", handle, (int)(pos >> 32), (int)pos);
#endif
	args.handle = handle;
	args.poshi = (int)(pos >> 32);
	args.poslo = (int)pos;
	if (openfirmware(&args) == -1) {
#ifdef OFW_DEBUG
		printf("lose\n");
#endif
		return -1;
	}
#ifdef OFW_DEBUG
	printf("%d\n", args.status);
#endif
	return args.status;
}

void *
OF_claim(void *virt, u_int size, u_int align)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		void *virt;
		u_int size;
		u_int align;
		void *baseaddr;
	} args = {
		"claim",
		3,
		1,
	};

#ifdef OFW_DEBUG
	printf("OF_claim(%p, %x, %x) -> ", virt, size, align);
#endif
	args.virt = virt;
	args.size = size;
	args.align = align;
	if (openfirmware(&args) == -1) {
#ifdef OFW_DEBUG
		printf("lose\n");
#endif
		return (void *)-1;
	}
#ifdef OFW_DEBUG
	printf("%p\n", args.baseaddr);
#endif
	return args.baseaddr;
}

void
OF_release(void *virt, u_int size)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		void *virt;
		u_int size;
	} args = {
		"release",
		2,
		0,
	};

#ifdef OFW_DEBUG
	printf("OF_release(%p, %x)\n", virt, size);
#endif
	args.virt = virt;
	args.size = size;
	openfirmware(&args);
}

int
OF_milliseconds(void)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int ms;
	} args = {
		"milliseconds",
		0,
		1,
	};

	openfirmware(&args);
	return args.ms;
}

#ifdef	__notyet__
void
OF_chain(void *virt, u_int size, void (*entry)(), void *arg, u_int len)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		void *virt;
		u_int size;
		void (*entry)();
		void *arg;
		u_int len;
	} args = {
		"chain",
		5,
		0,
	};

	args.virt = virt;
	args.size = size;
	args.entry = entry;
	args.arg = arg;
	args.len = len;
	openfirmware(&args);
}
#else
void
OF_chain(void *virt, u_int size, boot_entry_t entry, void *arg, u_int len)
{
	/*
	 * This is a REALLY dirty hack till the firmware gets this going
	 */
#if 0
	OF_release(virt, size);
#endif
	entry(0, 0, openfirmware, arg, len);
}
#endif

int
OF_call_method(const char *method, int ihandle, int nargs, int nreturns, ...)
{
	va_list ap;
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		const char *method;
		int ihandle;
		int args_n_results[12];
	} args = {
		"call-method",
		2,
		1,
	};
	int *ip, n;

	if (nargs > 6)
		return -1;
	args.nargs = nargs + 2;
	args.nreturns = nreturns + 1;
	args.method = method;
	args.ihandle = ihandle;
	va_start(ap, nreturns);
	for (ip = args.args_n_results + (n = nargs); --n >= 0;)
		*--ip = va_arg(ap, int);

	if (openfirmware(&args) == -1) {
		va_end(ap);
		return -1;
	}
	if (args.args_n_results[nargs]) {
		va_end(ap);
		return args.args_n_results[nargs];
	}
	for (ip = args.args_n_results + nargs + (n = args.nreturns); --n > 0;)
		*va_arg(ap, int *) = *--ip;
	va_end(ap);
	return 0;
}

static int stdin;
static int stdout;

static void
setup(void)
{
	int chosen;

	if ((chosen = OF_finddevice("/chosen")) == -1)
		OF_exit();
	if (OF_getprop(chosen, "stdin", &stdin, sizeof(stdin)) !=
	    sizeof(stdin) ||
	    OF_getprop(chosen, "stdout", &stdout, sizeof(stdout)) !=
	    sizeof(stdout))
		OF_exit();

#ifdef HEAP_VARIABLE
	uint32_t pvr, vers, hsize = HEAP_SIZE;

	__asm volatile ("mfpvr %0" : "=r"(pvr));
	vers = pvr >> 16;
	if (vers >= IBM970 && vers <= IBM970GX) hsize = 0x800000;

	heapspace = OF_claim(0, hsize, NBPG);
	if (heapspace == (char *)-1) {
		panic("Failed to allocate heap");
	}

	setheap(heapspace, heapspace + HEAP_SIZE);
#endif	/* HEAP_VARIABLE */
}

void
putchar(int c)
{
	char ch = c;

	if (c == '\n')
		putchar('\r');
	OF_write(stdout, &ch, 1);
}

int
getchar(void)
{
	unsigned char ch = '\0';
	int l;

	while ((l = OF_read(stdin, &ch, 1)) != 1)
		if (l != -2 && l != 0)
			return -1;
	return ch;
}
