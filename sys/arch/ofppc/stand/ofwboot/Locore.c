/*	$NetBSD: Locore.c,v 1.23 2014/09/20 23:10:46 phx Exp $	*/

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

#include "openfirm.h"
#include <sys/param.h>
#include <lib/libsa/stand.h>

#include <machine/cpu.h>

static int (*openfirmware_entry)(void *);
static int openfirmware(void *);

void startup(void *, int, int (*)(void *), char *, int)
	__attribute__((__used__));
static void setup(void);

/* this pad gets the rodata laignment right, don't EVER fiddle it */
char *pad __attribute__((__aligned__ (8))) = "pad";
int stack[0x20000/4 + 4] __attribute__((__aligned__ (4), __used__));
char *heapspace __attribute__((__aligned__ (4)));
char altheap[0x20000] __attribute__((__aligned__ (4)));

static int
openfirmware(void *arg)
{
	int r;

	__asm volatile ("sync; isync");
	r = openfirmware_entry(arg);
	__asm volatile ("sync; isync");

	return r;
}

void
startup(void *vpd, int res, int (*openfirm)(void *), char *arg, int argl)
{

	openfirmware_entry = openfirm;
	setup();
	main();
	OF_exit();
}

__dead void
OF_exit(void)
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

__dead void
OF_boot(char *bootspec)
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		char *bootspec;
	} args = {
		"boot",
		1,
		0,
	};

	args.bootspec = bootspec;
	openfirmware(&args);
	for (;;);			/* just in case */
}

int
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
	if (openfirmware(&args) == -1)
		return -1;
	return args.phandle;
}

int
OF_instance_to_package(int ihandle)
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
	if (openfirmware(&args) == -1)
		return -1;
	return args.size;
}

#ifdef	__notyet__	/* Has a bug on FirePower */
int
OF_setprop(int handle, char *prop, void *buf, int len)
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
OF_write(int handle, void *addr, int len)
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
OF_alloc_mem(u_int size)
{
	static struct {
		char *name;
		int nargs;
		int nreturns;
		u_int size;
		void *baseaddr;
	} args = {
		"alloc-mem",
		1,
		1,
	};
#ifdef OFW_DEBUG
	printf("alloc-mem %x -> ", size);
#endif
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

void *
OF_claim(void *virt, u_int size, u_int align)
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
OF_chain(void *virt, u_int size, void (*entry)(), void *arg, u_int len)
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
OF_chain(void *virt, u_int size, boot_entry_t entry, void *arg, u_int len)
{
	/*
	 * This is a REALLY dirty hack till the firmware gets this going
	 */
#if 0
	OF_release(virt, size);
#endif
	entry(0, 0, openfirmware_entry, arg, len);
}
#endif

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

	//printf("Allocating 0x20000 bytes of ram for boot\n");
	heapspace = OF_claim(0, 0x20000, NBPG);
	if (heapspace == (char *)-1) {
		printf("WARNING: Failed to alloc ram, using bss\n");
		setheap(&altheap, &altheap[0x20000]);
	} else
		setheap(heapspace, heapspace+0x20000);
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

#ifdef OFWDUMP

static int
OF_peer(int phandle)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		int sibling;
	} args = {
		"peer",
			1,
			1,
	};
	
	args.phandle = phandle;
	if (openfirmware(&args) == -1)
		return 0;
	return args.sibling;
}

static int
OF_child(int phandle)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		int child;
	} args = {
		"child",
			1,
			1,
	};

	args.phandle = phandle;
	if (openfirmware(&args) == -1)
		return 0;
	return args.child;
}

int
OF_nextprop(int handle, const char *prop, void *nextprop)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		const char *prop;
		char *buf;
		int flag;
	} args = {
		"nextprop",
			3,
			1,
	};

	args.phandle = handle;
	args.prop = prop;
	args.buf = nextprop;
	if (openfirmware(&args) == -1)
		return -1;
	return args.flag;
}

static int
OF_package_to_path(int phandle, char *buf, int buflen)
{
	static struct {
		const char *name;
		int nargs;
		int nreturns;
		int phandle;
		char *buf;
		int buflen;
		int length;
	} args = {
		"package-to-path",
			3,
			1,
	};

	if (buflen > 4096)
		return -1;
	args.phandle = phandle;
	args.buf = buf;
	args.buflen = buflen;
	if (openfirmware(&args) < 0)
		return -1;
	if (args.length > buflen)
		args.length = buflen;
	return args.length;
}

void
dump_ofwtree(int node)
{
	int peer, child, namelen, dlen, i;
	char namebuf[33], newnamebuf[33];
	char path[256], data[256];

	for (peer = node; peer; peer = OF_peer(peer)) {
		printf("\nnode: 0x%x ", peer);
		if (OF_package_to_path(peer, path, 512) >= 0)
			printf("path=%s", path);
		printf("\n");
		namebuf[0] = '\0';
		namelen = OF_nextprop(peer, namebuf, &newnamebuf);
		while (namelen >= 0) {
			/*printf("namelen == %d namebuf=%s new=%s\n", namelen,
			  namebuf, newnamebuf);*/
			//newnamebuf[namelen] = '\0';
			strcpy(namebuf, newnamebuf);
			printf("  %s :", newnamebuf);
			dlen = OF_getprop(peer, newnamebuf, data, 256);
			if (dlen > 0) {
				if (data[0] < 0177)
					printf(" %s\n", data);
				else
					printf("\n");
				printf("    ");
				for (i=0; i < dlen && i < 256; i++) {
					if (data[i] < 0x10)
						printf("0");
					printf("%x", data[i]);
					if ((i+1)%4 == 0)
						printf(" ");
					if ((i+1)%32 == 0)
						printf("\n    ");
				}
			}
			printf("\n");
			namelen = OF_nextprop(peer, namebuf, &newnamebuf);
			if (newnamebuf[0] == '\0' ||
			    strcmp(namebuf, newnamebuf) == 0)
				break;
		}
		child = OF_child(peer);
		if (child > 0)
			dump_ofwtree(child);
	}
}

#endif /* OFWDUMP */
