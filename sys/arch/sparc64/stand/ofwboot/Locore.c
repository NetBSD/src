/*	$NetBSD: Locore.c,v 1.4 1998/08/23 02:48:28 eeh Exp $	*/

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
#include <sparc64/stand/ofwboot/openfirm.h>

#include <machine/cpu.h>

/* All cells are 8 byte slots */
typedef u_int64_t cell_t;
/* 
 * Zero extend -- We force zero extension in case someone else
 *		  sign extended these values elsewhere.
 */
#define ZEX(x)	(cell_t)(u_int)(int)(x)

vaddr_t OF_claim_virt __P((vaddr_t vaddr, int len));
vaddr_t OF_alloc_virt __P((int len, int align));
int OF_free_virt __P((vaddr_t vaddr, int len));
int OF_unmap_virt __P((vaddr_t vaddr, int len));
vaddr_t OF_map_phys __P((paddr_t paddr, off_t size, vaddr_t vaddr, int mode));
paddr_t OF_alloc_phys __P((int len, int align));
paddr_t OF_claim_phys __P((paddr_t phys, int len));
int OF_free_phys __P((paddr_t paddr, int len));

extern int openfirmware(void *);

void setup __P((void));

#if 0
#ifdef XCOFF_GLUE
asm (".text; .globl _entry; _entry: .long _start,0,0");
#endif

__dead void
_start(vpd, res, openfirm, arg, argl)
	void *vpd;
	int res;
	int (*openfirm)(void *);
	char *arg;
	int argl;
{
	extern char etext[];

#ifdef	FIRMWORKSBUGS
	syncicache((void *)RELOC, etext - (char *)RELOC);
#endif
	openfirmware = openfirm;	/* Save entry to Open Firmware */
	setup();
	main(arg, argl);
	exit();
}
#endif

__dead void
_rtt()
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
	} args = {
		(cell_t)&"exit",
		(cell_t)0,
		(cell_t)0
	};

	openfirmware(&args);
	while (1);			/* just in case */
}

u_int
OF_finddevice(name)
	char *name;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t device;
		cell_t phandle;
	} args = {
		(cell_t)&"finddevice",
		(cell_t)1,
		(cell_t)1,
	};	
	
	args.device = (cell_t)name;
	if (openfirmware(&args) == -1)
		return -1;
	return args.phandle;
}

u_int
OF_instance_to_package(ihandle)
	u_int ihandle;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t ihandle;
		cell_t phandle;
	} args = {
		(cell_t)&"instance-to-package",
		(cell_t)1,
		(cell_t)1,
	};
	
	args.ihandle = ZEX(ihandle);
	if (openfirmware(&args) == -1)
		return -1;
	return args.phandle;
}

u_int
OF_getprop(handle, prop, buf, buflen)
	u_int handle;
	char *prop;
	void *buf;
	int buflen;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t phandle;
		cell_t prop;
		cell_t buf;
		cell_t buflen;
		cell_t size;
	} args = {
		(cell_t)&"getprop",
		(cell_t)4,
		(cell_t)1,
	};
	
	args.phandle = ZEX(handle);
	args.prop = (cell_t)prop;
	args.buf = (cell_t)buf;
	args.buflen = buflen;
	if (openfirmware(&args) == -1)
		return -1;
	return args.size;
}

#ifdef	__notyet__	/* Has a bug on FirePower */
int
OF_setprop(handle, prop, buf, len)
	u_int handle;
	char *prop;
	void *buf;
	int len;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t phandle;
		cell_t prop;
		cell_t buf;
		cell_t len;
		cell_t size;
	} args = {
		(cell_t)&"setprop",
		(cell_t)4,
		(cell_t)1,
	};
	
	args.phandle = ZEX(handle);
	args.prop = (cell_t)prop;
	args.buf = (cell_t)buf;
	args.len = len;
	if (openfirmware(&args) == -1)
		return -1;
	return args.size;
}
#endif

u_int
OF_open(dname)
	char *dname;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t dname;
		cell_t handle;
	} args = {
		(cell_t)&"open",
		(cell_t)1,
		(cell_t)1,
		(cell_t)NULL,
		(cell_t)0
	};
	
	args.dname = (cell_t)dname;
	if (openfirmware(&args) == -1 ||
	    args.handle == 0)
		return -1;
	return args.handle;
}

void
OF_close(handle)
	u_int handle;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t handle;
	} args = {
		(cell_t)&"close",
		1,
		0,
	};
	
	args.handle = ZEX(handle);
	openfirmware(&args);
}

int
OF_write(handle, addr, len)
	u_int handle;
	void *addr;
	int len;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t ihandle;
		cell_t addr;
		cell_t len;
		cell_t actual;
	} args = {
		(cell_t)&"write",
		(cell_t)3,
		(cell_t)1,
	};

	args.ihandle = ZEX(handle);
	args.addr = (cell_t)addr;
	args.len = len;
	if (openfirmware(&args) == -1)
		return -1;
	return args.actual;
}

int
OF_read(handle, addr, len)
	u_int handle;
	void *addr;
	int len;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t ihandle;
		cell_t addr;
		cell_t len;
		cell_t actual;
	} args = {
		(cell_t)&"read",
		(cell_t)3,
		(cell_t)1,
	};

	args.ihandle = ZEX(handle);
	args.addr = (cell_t)addr;
	args.len = len;
	if (openfirmware(&args) == -1)
		return -1;
	return args.actual;
}

int
OF_seek(handle, pos)
	u_int handle;
	u_quad_t pos;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t handle;
		cell_t poshi;
		cell_t poslo;
		cell_t status;
	} args = {
		(cell_t)&"seek",
		(cell_t)3,
		(cell_t)1,
	};
	
	args.handle = ZEX(handle);
	args.poshi = ZEX(pos >> 32);
	args.poslo = ZEX(pos);
	if (openfirmware(&args) == -1)
		return -1;
	return args.status;
}

void
OF_release(virt, size)
	void *virt;
	u_int size;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t virt;
		cell_t size;
	} args = {
		(cell_t)&"release",
		(cell_t)2,
		(cell_t)0,
	};
	
	args.virt = (cell_t)virt;
	args.size = size;
	openfirmware(&args);
}

int
OF_milliseconds()
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t ms;
	} args = {
		(cell_t)&"milliseconds",
		(cell_t)0,
		(cell_t)1,
	};
	
	openfirmware(&args);
	return args.ms;
}

void
OF_chain(virt, size, entry, arg, len)
	void *virt;
	u_int size;
	void (*entry)();
	void *arg;
	u_int len;
{
	extern int64_t romp;
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t virt;
		cell_t size;
		cell_t entry;
		cell_t arg;
		cell_t len;
	} args = {
		(cell_t)&"chain",
		(cell_t)5,
		(cell_t)0,
	};

	args.virt = (cell_t)virt;
	args.size = size;
	args.entry = (cell_t)entry;
	args.arg = (cell_t)arg;
	args.len = len;
	openfirmware(&args);
	printf("OF_chain: prom returned!\n");

	/* OK, firmware failed us.  Try calling prog directly */
	entry(0, arg, len, (unsigned long)romp, (unsigned long)romp);
	panic("OF_chain: kernel returned!\n");
	__asm("ta 2" : :);
}

static u_int stdin;
static u_int stdout;
static u_int mmuh = -1;
static u_int memh = -1;

void
setup()
{
	u_int chosen;
	
	if ((chosen = OF_finddevice("/chosen")) == -1)
		_rtt();
	if (OF_getprop(chosen, "stdin", &stdin, sizeof(stdin)) != sizeof(stdin)
	    || OF_getprop(chosen, "stdout", &stdout, sizeof(stdout)) != sizeof(stdout)
	    || OF_getprop(chosen, "mmu", &mmuh, sizeof(mmuh)) != sizeof(mmuh)
	    || OF_getprop(chosen, "memory", &memh, sizeof(memh)) != sizeof(memh))
		_rtt();
}

/*
 * The following need either the handle to memory or the handle to the MMU.
 */

/* 
 * Grab some address space from the prom
 *
 * Only works while the prom is actively mapping us.
 */
vaddr_t
OF_claim_virt(vaddr, len)
vaddr_t vaddr;
int len;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t align;
		cell_t len;
		cell_t vaddr;
		cell_t status;
		cell_t retaddr;
	} args = {
		(cell_t)&"call-method",
		(cell_t)5,
		(cell_t)2,
		(cell_t)&"claim",
		(cell_t)0,
		(cell_t)0,
		(cell_t)0, 
		(cell_t)NULL,
		(cell_t)0,
		(cell_t)0
	};

#ifdef	__notyet
	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		OF_printf("OF_claim_virt: cannot get mmuh\r\n");
		return -1LL;
	}
#endif
	args.ihandle = mmuh;
	args.vaddr = (cell_t)vaddr;
	args.len = len;
	if(openfirmware(&args) != 0)
		return -1LL;
	return args.retaddr; /* Kluge till we go 64-bit */
}

/* 
 * Request some address space from the prom
 *
 * Only works while the prom is actively mapping us.
 */
vaddr_t
OF_alloc_virt(len, align)
int len;
int align;
{
	static int retaddr=-1;
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t align;
		cell_t len;
		cell_t status;
		cell_t retaddr;
	} args = {
		(cell_t)&"call-method",
		(cell_t)4,
		(cell_t)2,
		(cell_t)&"claim",
		(cell_t)0,
		(cell_t)0,
		(cell_t)0, 
		(cell_t)0,
		(cell_t)&retaddr
	};

#ifdef	__notyet
	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		OF_printf("OF_alloc_virt: cannot get mmuh\r\n");
		return -1LL;
	}
#endif
	args.ihandle = mmuh;
	args.align = align;
	args.len = len;
	if(openfirmware(&args) != 0)
		return -1LL;
	return (vaddr_t)args.retaddr; /* Kluge till we go 64-bit */
}

/* 
 * Release some address space to the prom
 *
 * Only works while the prom is actively mapping us.
 */
int
OF_free_virt(vaddr, len)
vaddr_t vaddr;
int len;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t len;
		cell_t vaddr;
	} args = {
		(cell_t)&"call-method",
		4,
		0,
		(cell_t)&"release",
		0,
		0,
		NULL
	};

#ifdef	__notyet
	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		OF_printf("OF_claim_virt: cannot get mmuh\r\n");
		return -1;
	}
#endif
	args.ihandle = mmuh;
	args.vaddr = (cell_t)vaddr;
	args.len = len;
	return openfirmware(&args);
}


/* 
 * Unmap some address space
 *
 * Only works while the prom is actively mapping us.
 */
int
OF_unmap_virt(vaddr, len)
vaddr_t vaddr;
int len;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t len;
		cell_t vaddr;
	} args = {
		(cell_t)&"call-method",
		(cell_t)4,
		(cell_t)0,
		(cell_t)&"unmap",
		(cell_t)0,
		(cell_t)0,
		(cell_t)NULL
	};

#ifdef	__notyet
	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		OF_printf("OF_claim_virt: cannot get mmuh\r\n");
		return -1;
	}
#endif
	args.ihandle = mmuh;
	args.vaddr = (cell_t)vaddr;
	args.len = len;
	return openfirmware(&args);
}

/* 
 * Have prom map in some memory
 *
 * Only works while the prom is actively mapping us.
 */
vaddr_t
OF_map_phys(paddr, size, vaddr, mode)
paddr_t paddr;
off_t size;
vaddr_t vaddr;
int mode;
{
	u_int phys_hi, phys_lo;
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t mode;
		cell_t size;
		cell_t vaddr;
		cell_t paddr_hi;
		cell_t paddr_lo;
		cell_t status;
		cell_t retaddr;
	} args = {
		(cell_t)&"call-method",
		(cell_t)7,
		(cell_t)1,
		(cell_t)&"map",
		(cell_t)0,
		(cell_t)0,
		(cell_t)NULL,
		(cell_t)NULL,
		(cell_t)NULL
	};

#ifdef	__notyet
	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		OF_printf("OF_map_phys: cannot get mmuh\r\n");
		return 0LL;
	}
#endif
	phys_hi = (cell_t)(paddr>>32);
	phys_lo = (cell_t)(int)paddr;

	args.ihandle = ZEX(mmuh);
	args.mode = mode;
	args.size = size;
	args.vaddr = (cell_t)vaddr;
	args.paddr_hi = phys_hi;
	args.paddr_lo = phys_lo;
	if (openfirmware(&args) == -1)
		return -1;
	if (args.status)
		return -1;
	return (vaddr_t)args.retaddr;
}


/* 
 * Request some RAM from the prom
 *
 * Only works while the prom is actively mapping us.
 */
paddr_t
OF_alloc_phys(len, align)
int len;
int align;
{
	paddr_t paddr;
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t align;
		cell_t len;
		cell_t status;
		cell_t phys_hi;
		cell_t phys_lo;
	} args = {
		(cell_t)&"call-method",
		(cell_t)4,
		(cell_t)3,
		(cell_t)&"claim",
		(cell_t)0,
		(cell_t)0,
		(cell_t)0, 
		(cell_t)0,
	};

#ifdef	__notyet
	if (memh == -1 && ((memh = get_memory_handle()) == -1)) {
		OF_printf("OF_alloc_phys: cannot get memh\r\n");
		return -1LL;
	}
#endif
	args.ihandle = ZEX(memh);
	args.align = align;
	args.len = len;
	if(openfirmware(&args) != 0)
		return -1LL;
	paddr = (paddr_t)(args.phys_hi<<32)|((unsigned int)(args.phys_lo));
	return paddr; /* Kluge till we go 64-bit */
}

/* 
 * Request some specific RAM from the prom
 *
 * Only works while the prom is actively mapping us.
 */
paddr_t
OF_claim_phys(phys, len)
paddr_t phys;
int len;
{
	paddr_t paddr;
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t align;
		cell_t len;
		cell_t phys_hi;
		cell_t phys_lo;
		cell_t status;
		cell_t res;
		cell_t rphys_hi;
		cell_t rphys_lo;
	} args = {
		(cell_t)&"call-method",
		(cell_t)6,
		(cell_t)4,
		(cell_t)&"claim",
		(cell_t)0,
		(cell_t)0,
		(cell_t)0, 
		(cell_t)NULL,
		(cell_t)NULL,
		(cell_t)0
	};

#ifdef	__notyet
	if (memh == -1 && ((memh = get_memory_handle()) == -1)) {
		OF_printf("OF_alloc_phys: cannot get memh\r\n");
		return 0LL;
	}
#endif
	args.ihandle = ZEX(memh);
	args.len = len;
	args.phys_hi = ZEX(phys>>32);
	args.phys_lo = ZEX(phys);
	if(openfirmware(&args) != 0)
		return 0LL;
	paddr = (paddr_t)(args.phys_hi<<32)|((unsigned int)(args.phys_lo));
	return paddr;
}

/* 
 * Free some RAM to prom
 *
 * Only works while the prom is actively mapping us.
 */
int
OF_free_phys(phys, len)
paddr_t phys;
int len;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t len;
		cell_t phys_hi;
		cell_t phys_lo;
	} args = {
		(cell_t)&"call-method",
		(cell_t)5,
		(cell_t)0,
		(cell_t)&"release",
		(cell_t)0,
		(cell_t)0, 
		(cell_t)NULL,
		(cell_t)NULL,
	};

#ifdef	__notyet
	if (memh == -1 && ((memh = get_memory_handle()) == -1)) {
		OF_printf("OF_free_phys: cannot get memh\r\n");
		return -1;
	}
#endif
	args.ihandle = ZEX(memh);
	args.len = len;
	args.phys_hi = ZEX(phys>>32);
	args.phys_lo = ZEX(phys);
	return openfirmware(&args);
}


/*
 * Claim virtual memory -- does not map it in.
 */

void *
OF_claim(virt, size, align)
	void *virt;
	u_int size;
	u_int align;
{
#define SUNVMOF
#ifndef SUNVMOF
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t virt;
		cell_t size;
		cell_t align;
		cell_t baseaddr;
	} args = {
		(cell_t)&"claim",
		(cell_t)3,
		(cell_t)1,
	};


	args.virt = virt;
	args.size = size;
	args.align = align;
	if (openfirmware(&args) == -1)
		return (void *)-1;
	return args.baseaddr;
#else
/*
 * Sun Ultra machines run the firmware with VM enabled,
 * so you need to handle allocating and mapping both
 * virtual and physical memory.  Ugh.
 */

	paddr_t paddr;
	void* newvirt = NULL;

	if (virt == NULL) {
		if ((virt = (void*)OF_alloc_virt(size, align)) == (void*)-1) {
			printf("OF_alloc_virt(%d,%d) failed w/%x\n", size, align, virt);
			return (void *)-1;
		}
	} else {
		if ((newvirt = (void*)OF_claim_virt((vaddr_t)virt, size)) == (void*)-1) {
			printf("OF_claim_virt(%x,%d) failed w/%x\n", virt, size, newvirt);
			return (void *)-1;
		}
	}
	if ((paddr = OF_alloc_phys(size, align)) == -1) {
		printf("OF_alloc_phys(%d,%d) failed\n", size, align);
		OF_free_virt((vaddr_t)virt, size);
		return (void *)-1;
	}
	if (OF_map_phys(paddr, size, (vaddr_t)virt, -1) == -1) {
		printf("OF_map_phys(%x,%d,%x,%d) failed\n", paddr, size, virt, -1);
		OF_free_phys((paddr_t)paddr, size);
		OF_free_virt((vaddr_t)virt, size);
		return (void *)-1;
	}
	return (void *)virt;
#endif
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
