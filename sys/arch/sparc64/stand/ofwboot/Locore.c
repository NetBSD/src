/*	$NetBSD: Locore.c,v 1.1.1.1.2.1 1998/07/30 14:03:57 eeh Exp $	*/

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

vaddr_t OF_claim_virt __P((vaddr_t vaddr, int len));
vaddr_t OF_alloc_virt __P((int len, int align));
int OF_free_virt __P((vaddr_t vaddr, int len));
int OF_unmap_virt __P((vaddr_t vaddr, int len));
int OF_map_phys __P((u_int64_t paddr, off_t size, vaddr_t vaddr, int mode));
u_int64_t OF_alloc_phys __P((int len, int align));
u_int64_t OF_claim_phys __P((paddr_t phys, int len));
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
		int pad; char *name;
		int64_t nargs;
		int64_t nreturns;
	} args = {
		0, "exit",
		0,
		0
	};

	openfirmware(&args);
	while (1);			/* just in case */
}

int
OF_finddevice(name)
	char *name;
{
	static struct {
		int pad; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *device;
		int pad2; uint phandle;
	} args = {
		0, "finddevice",
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
		int pad; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; uint ihandle;
		int pad2; uint phandle;
	} args = {
		0, "instance-to-package",
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
		int pad; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; uint phandle;
		int pad2; char *prop;
		int pad3; void *buf;
		int64_t buflen;
		int64_t size;
	} args = {
		0, "getprop",
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
		int pad; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad; uint phandle;
		int pad1; char *prop;
		int pad2; void *buf;
		int64_t len;
		int64_t size;
	} args = {
		0, "setprop",
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
		int pad; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *dname;
		int pad2; uint handle;
	} args = {
		0, "open",
		1,
		1,
		0, NULL,
		0, 0
	};
	
	args.dname = dname;
	if (openfirmware(&args) == -1 ||
	    args.handle == 0)
		return -1;
	return args.handle;
}

void
OF_close(handle)
	int handle;
{
	static struct {
		int pad; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; uint handle;
	} args = {
		0, "close",
		1,
		0,
	};
	
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
		int pad; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; uint ihandle;
		int pad2; void *addr;
		int64_t len;
		int pad3; int actual;
	} args = {
		0, "write",
		3,
		1,
	};

	args.ihandle = handle;
	args.addr = addr;
	args.len = len;
	if (openfirmware(&args) == -1)
		return -1;
	return args.actual;
}

int
OF_read(handle, addr, len)
	int handle;
	void *addr;
	int len;
{
	static struct {
		int pad; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; uint ihandle;
		int pad2; void *addr;
		int64_t len;
		int pad3; int actual;
	} args = {
		0, "read",
		3,
		1,
	};

	args.ihandle = handle;
	args.addr = addr;
	args.len = len;
	if (openfirmware(&args) == -1)
		return -1;
	return args.actual;
}

int
OF_seek(handle, pos)
	int handle;
	u_quad_t pos;
{
	static struct {
		int pad; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; uint handle;
		int64_t poshi;
		int64_t poslo;
		int pad3; int status;
	} args = {
		0, "seek",
		3,
		1,
	};
	
	args.handle = handle;
	args.poshi = (int)(pos >> 32);
	args.poslo = (int)pos;
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
		int pad; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; void *virt;
		u_int64_t size;
	} args = {
		0, "release",
		2,
		0,
	};
	
	args.virt = virt;
	args.size = size;
	openfirmware(&args);
}

int
OF_milliseconds()
{
	static struct {
		int pad; char *name;
		int64_t nargs;
		int64_t nreturns;
		int64_t ms;
	} args = {
		0, "milliseconds",
		0,
		1,
	};
	
	openfirmware(&args);
	return args.ms;
}

#ifndef	__notyet__
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
		int pad; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; void *virt;
		u_int64_t size;
		int pad2; void (*entry)();
		int pad3; void *arg;
		int pad4; int len;
	} args = {
		0, "chain",
		5,
		0,
	};

	args.virt = virt;
	args.size = size;
	args.entry = entry;
	args.arg = arg;
	args.len = len;
	openfirmware(&args);
#ifdef DEBUG
	/* OK, firmware failed us.  Try calling prog directly */
	entry(0, arg, len, (int)romp, (int)romp);
	__asm("ta 2" : :);
#endif
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
	extern int64_t romp;

	/*
	 * This is a REALLY dirty hack till the firmware gets this going
	 */
/*	OF_release(virt, size); */
	entry(0, arg, len, (int)romp, (int)romp);
}
#endif

static int stdin;
static int stdout;
static int mmuh = -1;
static int memh = -1;

void
setup()
{
	int chosen;
	
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
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *method;
		int pad2; int ihandle;
		u_int64_t align;
		u_int64_t len;
		int pad3; vaddr_t vaddr;
		int64_t status;
		int64_t retaddr;
	} args = {
		0,"call-method",
		5,
		2,
		0,"claim",
		0, 0,
		0,
		0, 
		0, NULL,
		0,
		0
	};

#ifdef	__notyet
	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		OF_printf("OF_claim_virt: cannot get mmuh\r\n");
		return -1LL;
	}
#endif
	args.ihandle = mmuh;
	args.vaddr = vaddr;
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
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *method;
		int pad2; int ihandle;
		u_int64_t align;
		u_int64_t len;
		int64_t status;
		int pad4; void* retaddr;
	} args = {
		0,"call-method",
		4,
		2,
		0,"claim",
		0, 0,
		0,
		0, 
		0,
		0, &retaddr
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
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *method;
		int pad2; int ihandle;
		u_int64_t len;
		int pad3; vaddr_t vaddr;
	} args = {
		0,"call-method",
		4,
		0,
		0,"release",
		0, 0,
		0,
		0, NULL
	};

#ifdef	__notyet
	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		OF_printf("OF_claim_virt: cannot get mmuh\r\n");
		return -1;
	}
#endif
	args.ihandle = mmuh;
	args.vaddr = vaddr;
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
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *method;
		int pad2; int ihandle;
		u_int64_t len;
		int pad3; vaddr_t vaddr;
	} args = {
		0,"call-method",
		4,
		0,
		0,"unmap",
		0, 0,
		0,
		0, NULL
	};

#ifdef	__notyet
	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		OF_printf("OF_claim_virt: cannot get mmuh\r\n");
		return -1;
	}
#endif
	args.ihandle = mmuh;
	args.vaddr = vaddr;
	args.len = len;
	return openfirmware(&args);
}

/* 
 * Have prom map in some memory
 *
 * Only works while the prom is actively mapping us.
 */
int
OF_map_phys(paddr, size, vaddr, mode)
u_int64_t paddr;
off_t size;
vaddr_t vaddr;
int mode;
{
	int phys_hi, phys_lo;
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *method;
		int pad2; int ihandle;
		int64_t mode;
		u_int64_t size;
		int pad3; vaddr_t vaddr;
		paddr_t paddr_hi;
		paddr_t paddr_lo;
		int64_t status;
		int64_t retaddr;
	} args = {
		0,"call-method",
		7,
		1,
		0,"map",
		0, 0,
		0, 0,
		0, NULL,
		0, NULL,
		0, NULL
	};

#ifdef	__notyet
	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		OF_printf("OF_map_phys: cannot get mmuh\r\n");
		return 0LL;
	}
#endif
#ifdef INT_IS_64_BITS
	phys_hi = paddr>>32; 
#else
	phys_hi = 0; /* This is what Solaris does.  We gotta fix this for 64-bits */
#endif
	phys_lo = paddr;

	args.ihandle = mmuh;
	args.mode = mode;
	args.size = size;
	args.vaddr = vaddr;
	args.paddr_hi = phys_hi;
	args.paddr_lo = phys_lo;
	if (openfirmware(&args) == -1)
		return -1;
	if (args.status)
		return -1;
	return args.retaddr;
}


/* 
 * Request some RAM from the prom
 *
 * Only works while the prom is actively mapping us.
 */
u_int64_t
OF_alloc_phys(len, align)
int len;
int align;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *method;
		int pad2; int ihandle;
		u_int64_t align;
		u_int64_t len;
		int64_t status;
		u_int64_t phys_hi;
		u_int64_t phys_lo;
	} args = {
		0,"call-method",
		4,
		3,
		0,"claim",
		0, 0,
		0,
		0, 
		0,
	};

#ifdef	__notyet
	if (memh == -1 && ((memh = get_memory_handle()) == -1)) {
		OF_printf("OF_alloc_phys: cannot get memh\r\n");
		return -1LL;
	}
#endif
	args.ihandle = memh;
	args.align = align;
	args.len = len;
	if(openfirmware(&args) != 0)
		return -1LL;
	return args.phys_lo; /* Kluge till we go 64-bit */
}

/* 
 * Request some specific RAM from the prom
 *
 * Only works while the prom is actively mapping us.
 */
u_int64_t
OF_claim_phys(phys, len)
paddr_t phys;
int len;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *method;
		int pad2; int ihandle;
		u_int64_t align;
		u_int64_t len;
		int pad4; void* phys_hi;
		int pad5; void* phys_lo;
		int64_t status;
		int64_t res;
		u_int64_t rphys_hi;
		u_int64_t rphys_lo;
	} args = {
		0,"call-method",
		6,
		4,
		0,"claim",
		0, 0,
		0,
		0, 
		0, NULL,
		0, NULL,
		0
	};

#ifdef	__notyet
	if (memh == -1 && ((memh = get_memory_handle()) == -1)) {
		OF_printf("OF_alloc_phys: cannot get memh\r\n");
		return 0LL;
	}
#endif
	args.ihandle = memh;
	args.len = len;
	args.phys_lo = (void*)phys;
	if(openfirmware(&args) != 0)
		return 0LL;
	return args.rphys_lo; /* Kluge till we go 64-bit */
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
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *method;
		int pad2; int ihandle;
		u_int64_t len;
		int pad4; void* phys_hi;
		int pad5; void* phys_lo;
	} args = {
		0,"call-method",
		5,
		0,
		0,"release",
		0, 0,
		0, 
		0, NULL,
		0, NULL,
	};

#ifdef	__notyet
	if (memh == -1 && ((memh = get_memory_handle()) == -1)) {
		OF_printf("OF_free_phys: cannot get memh\r\n");
		return -1;
	}
#endif
	args.ihandle = memh;
	args.len = len;
	args.phys_lo = (void*)phys;
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
		int pad; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; void *virt;
		u_int64_t size;
		u_int64_t align;
		int pad2; void *baseaddr;
	} args = {
		0, "claim",
		3,
		1,
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

	u_int64_t paddr;
	void* newvirt = NULL;

	if (virt == NULL) {
		if ((virt = (void*)OF_alloc_virt(size, align)) == (void*)-1) {
			printf("OF_alloc_virt(%d,%d) failed w/%x\n", size, align, virt);
			return (void *)-1;
		}
	} else {
		if ((newvirt = (void*)OF_claim_virt((int)virt, size)) == (void*)-1) {
			printf("OF_claim_virt(%x,%d) failed w/%x\n", virt, size, newvirt);
			return (void *)-1;
		}
	}
	if ((paddr = OF_alloc_phys(size, align)) == -1) {
		printf("OF_alloc_phys(%d,%d) failed\n", size, align);
		OF_free_virt((int)virt, size);
		return (void *)-1;
	}
	if (OF_map_phys(paddr, size, (int)virt, -1) == -1) {
		printf("OF_map_phys(%x,%d,%x,%d) failed\n", paddr, size, virt, -1);
		OF_free_phys(paddr, size);
		OF_free_virt((int)virt, size);
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
