/*	$NetBSD: Locore.c,v 1.4 1999/02/02 16:29:25 tsubai Exp $	*/

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

#include <lib/libsa/stand.h>

#include <machine/cpu.h>

#include "openfirm.h"

static int (*openfirmware) __P((void *));

static void startup __P((void *, int, int (*)(void *), char *, int));
static void setup __P((void));

static int stack[4096/4];

#ifdef XCOFF_GLUE
asm("
	.text
	.globl	_entry
_entry:
	.long	_start,0,0
");
#endif

asm("
	.text
	.globl	_start
_start:
	li	8,0
	li	9,0x100
	mtctr	9
1:
	dcbf	0,8
	icbi	0,8
	addi	8,8,0x20
	bdnz	1b
	sync
	isync

	lis	1,stack@ha
	addi	1,1,stack@l
	addi	1,1,4096

	mfmsr	8
	li	0,0
	mtmsr	0
	isync

	mtibatu	0,0
	mtibatu	1,0
	mtibatu	2,0
	mtibatu	3,0
	mtdbatu	0,0
	mtdbatu	1,0
	mtdbatu	2,0
	mtdbatu	3,0

	li	9,0x12		/* BATL(0, BAT_M) */
	mtibatl	0,9
	mtdbatl	0,9
	li	9,0x1ffe	/* BATU(0) */
	mtibatu	0,9
	mtdbatu	0,9
	isync

	mtmsr	8
	isync

	b	startup
");

#if 0
static int
openfirmware(arg)
	void *arg;
{

	asm volatile ("sync; isync");
	openfirmware_entry(arg);
	asm volatile ("sync; isync");
}
#endif

static void
startup(vpd, res, openfirm, arg, argl)
	void *vpd;
	int res;
	int (*openfirm)(void *);
	char *arg;
	int argl;
{
	extern char etext[], _end[], _edata[];

	bzero(_edata, (_end - _edata));
	openfirmware = openfirm;
	setup();
	main();
	OF_exit();
}

__dead void
OF_exit()
{
	static struct {
		char *name;
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
	if (openfirmware(&args) == -1)
		return -1;
	return args.phandle;
}

int
OF_instance_to_package(ihandle)
	int ihandle;
{
	static struct {
		char *name;
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
	if (openfirmware(&args) == -1)
		return -1;
	return args.size;
}

#ifdef	__notyet__	/* Has a bug on FirePower */
int
OF_setprop(handle, prop, buf, len)
	int handle;
	char *prop;
	void *buf;
	int len;
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		int phandle;
		char *prop;
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
OF_close(handle)
	int handle;
{
	static struct {
		char *name;
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

#ifdef OFW_DEBUG
	if (len != 1)
		printf("OF_write(%d, %x, %x) -> ", handle, addr, len);
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

#ifdef OFW_DEBUG
	if (len != 1)
		printf("OF_read(%d, %x, %x) -> ", handle, addr, len);
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
OF_claim(virt, size, align)
	void *virt;
	u_int size;
	u_int align;
{
	static struct {
		char *name;
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
	printf("OF_claim(%x, %x, %x) -> ", virt, size, align);
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
	printf("%x\n", args.baseaddr);
#endif
	return args.baseaddr;
}

void
OF_release(virt, size)
	void *virt;
	u_int size;
{
	static struct {
		char *name;
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
	printf("OF_release(%x, %x)\n", virt, size);
#endif
	args.virt = virt;
	args.size = size;
	openfirmware(&args);
}

int
OF_milliseconds()
{
	static struct {
		char *name;
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
OF_chain(virt, size, entry, arg, len)
	void *virt;
	u_int size;
	void (*entry)();
	void *arg;
	u_int len;
{
	static struct {
		char *name;
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
OF_chain(virt, size, entry, arg, len)
	void *virt;
	u_int size;
	void (*entry)();
	void *arg;
	u_int len;
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

static int stdin;
static int stdout;

static void
setup()
{
	int chosen;

	if ((chosen = OF_finddevice("/chosen")) == -1)
		OF_exit();
	if (OF_getprop(chosen, "stdin", &stdin, sizeof(stdin)) !=
	    sizeof(stdin) ||
	    OF_getprop(chosen, "stdout", &stdout, sizeof(stdout)) !=
	    sizeof(stdout))
		OF_exit();
}

void
putchar(c)
	int c;
{
	char ch = c;

	if (c == '\n')
		putchar('\r');
	OF_write(stdout, &ch, 1);
}

int
getchar()
{
	unsigned char ch = '\0';
	int l;

	while ((l = OF_read(stdin, &ch, 1)) != 1)
		if (l != -2 && l != 0)
			return -1;
	return ch;
}
