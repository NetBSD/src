/*	$NetBSD: ofw_machdep.c,v 1.9.4.1 2000/07/18 16:23:31 mrg Exp $	*/

/*
 * Copyright (C) 1996 Wolfgang Solfrank.
 * Copyright (C) 1996 TooLs GmbH.
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
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/stat.h>
#include <sys/systm.h>

#include <machine/openfirm.h>

#if defined(FFS) && defined(CD9660)
#include <ufs/ffs/fs.h>
#endif

/*
 * Note that stdarg.h and the ANSI style va_start macro is used for both
 * ANSI and traditional C compilers.
 */
#include <machine/stdarg.h>

#include <machine/sparc64.h>

int vsprintf __P((char *, const char *, va_list));

void dk_cleanup __P((void));
#if defined(FFS) && defined(CD9660)
static int dk_match_ffs __P((void));
#endif

static u_int mmuh = -1, memh = -1;

static u_int get_mmu_handle __P((void));
static u_int get_memory_handle __P((void));

static u_int 
get_mmu_handle()
{
	u_int chosen;

	if ((chosen = OF_finddevice("/chosen")) == -1) {
		prom_printf("get_mmu_handle: cannot get /chosen\r\n");
		return -1;
	}
	if (OF_getprop(chosen, "mmu", &mmuh, sizeof(mmuh)) == -1) {
		prom_printf("get_mmu_handle: cannot get mmuh\r\n");
		return -1;
	}
	return mmuh;
}

static u_int 
get_memory_handle()
{
	u_int chosen;

	if ((chosen = OF_finddevice("/chosen")) == -1) {
		prom_printf("get_mmu_handle: cannot get /chosen\r\n");
		return -1;
	}
	if (OF_getprop(chosen, "memory", &memh, sizeof(memh)) == -1) {
		prom_printf("get_memory_handle: cannot get memh\r\n");
		return -1;
	}
	return memh;
}


/* 
 * Point prom to our trap table.  This stops the prom from mapping us.
 */
int
prom_set_trap_table(tba)
	vaddr_t tba;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t tba;
	} args;

	args.name = ADR2CELL(&"SUNW,set-trap-table");
	args.nargs = 1;
	args.nreturns = 0;
	args.tba = ADR2CELL(tba);
	return openfirmware(&args);
}

/* 
 * Have the prom convert from virtual to physical addresses.
 *
 * Only works while the prom is actively mapping us.
 */
paddr_t
prom_vtop(vaddr)
	vaddr_t vaddr;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t vaddr;
		cell_t status;
		cell_t retaddr;
		cell_t mode;
		cell_t phys_hi;
		cell_t phys_lo;
	} args;

	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		prom_printf("prom_vtop: cannot get mmuh\r\n");
		return 0;
	}
	args.name = ADR2CELL(&"call-method");
	args.nargs = 3;
	args.nreturns = 5;
	args.method = ADR2CELL(&"translate");
	args.ihandle = HDL2CELL(mmuh);
	args.vaddr = ADR2CELL(vaddr);
	if(openfirmware(&args) != 0)
		return 0;
#if 0
	prom_printf("Called \"translate\", mmuh=%x, vaddr=%x, status=%x %x,\r\n retaddr=%x %x, mode=%x %x, phys_hi=%x %x, phys_lo=%x %x\r\n",
		    mmuh, vaddr, (int)(args.status>>32), (int)args.status, (int)(args.retaddr>>32), (int)args.retaddr, 
		    (int)(args.mode>>32), (int)args.mode, (int)(args.phys_hi>>32), (int)args.phys_hi,
		    (int)(args.phys_lo>>32), (int)args.phys_lo);
#endif
	return (paddr_t)((((paddr_t)args.phys_hi)<<32)|(int)args.phys_lo); 
}

/* 
 * Grab some address space from the prom
 *
 * Only works while the prom is actively mapping us.
 */
vaddr_t
prom_claim_virt(vaddr, len)
	vaddr_t vaddr;
	int len;
{
	struct {
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
	} args;

	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		prom_printf("prom_claim_virt: cannot get mmuh\r\n");
		return 0;
	}
	args.name = ADR2CELL(&"call-method");
	args.nargs = 5;
	args.nreturns = 2;
	args.method = ADR2CELL(&"claim");
	args.ihandle = HDL2CELL(mmuh);
	args.align = 0;
	args.len = len;
	args.vaddr = ADR2CELL(vaddr);
	if (openfirmware(&args) != 0)
		return 0;
	return (paddr_t)args.retaddr;
}

/* 
 * Request some address space from the prom
 *
 * Only works while the prom is actively mapping us.
 */
vaddr_t
prom_alloc_virt(len, align)
	int len;
	int align;
{
	static int retaddr;
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t align;
		cell_t len;
		cell_t status;
		cell_t retaddr;
	} args;

	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		prom_printf("prom_alloc_virt: cannot get mmuh\r\n");
		return -1LL;
	}
	args.name = ADR2CELL(&"call-method");
	args.nargs = 4;
	args.nreturns = 2;
	args.method = ADR2CELL(&"claim");
	args.ihandle = HDL2CELL(mmuh);
	args.align = align;
	args.len = len;
	args.retaddr = ADR2CELL(&retaddr);
	if (openfirmware(&args) != 0)
		return -1;
	return retaddr; /* Kluge till we go 64-bit */
}

/* 
 * Release some address space to the prom
 *
 * Only works while the prom is actively mapping us.
 */
int
prom_free_virt(vaddr, len)
	vaddr_t vaddr;
	int len;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t len;
		cell_t vaddr;
	} args;

	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		prom_printf("prom_claim_virt: cannot get mmuh\r\n");
		return -1;
	}
	args.name = ADR2CELL(&"call-method");
	args.nargs = 4;
	args.nreturns = 0;
	args.method = ADR2CELL(&"release");
	args.ihandle = HDL2CELL(mmuh);
	args.vaddr = ADR2CELL(vaddr);
	args.len = len;
	return openfirmware(&args);
}


/* 
 * Unmap some address space
 *
 * Only works while the prom is actively mapping us.
 */
int
prom_unmap_virt(vaddr, len)
	vaddr_t vaddr;
	int len;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t len;
		cell_t vaddr;
	} args;

	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		prom_printf("prom_claim_virt: cannot get mmuh\r\n");
		return -1;
	}
	args.name = ADR2CELL(&"call-method");
	args.nargs = 4;
	args.nreturns = 0;
	args.method = ADR2CELL(&"unmap");
	args.ihandle = HDL2CELL(mmuh);
	args.vaddr = ADR2CELL(vaddr);
	args.len = len;
	return openfirmware(&args);
}

/* 
 * Have prom map in some memory
 *
 * Only works while the prom is actively mapping us.
 */
int
prom_map_phys(paddr, size, vaddr, mode)
	paddr_t paddr;
	off_t size;
	vaddr_t vaddr;
	int mode;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t mode;
		cell_t size;
		cell_t vaddr;
		cell_t phys_hi;
		cell_t phys_lo;
		cell_t status;
		cell_t retaddr;
	} args;

	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		prom_printf("prom_map_phys: cannot get mmuh\r\n");
		return 0;
	}
	args.name = ADR2CELL(&"call-method");
	args.nargs = 7;
	args.nreturns = 1;
	args.method = ADR2CELL(&"map");
	args.ihandle = HDL2CELL(mmuh);
	args.mode = mode;
	args.size = size;
	args.vaddr = ADR2CELL(vaddr);
	args.phys_hi = HDL2CELL(paddr>>32); 
	args.phys_lo = HDL2CELL(paddr);

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
paddr_t
prom_alloc_phys(len, align)
	int len;
	int align;
{
	struct {
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
	} args;

	if (memh == -1 && ((memh = get_memory_handle()) == -1)) {
		prom_printf("prom_alloc_phys: cannot get memh\r\n");
		return 0;
	}
	args.name = ADR2CELL(&"call-method");
	args.nargs = 4;
	args.nreturns = 3;
	args.method = ADR2CELL(&"claim");
	args.ihandle = HDL2CELL(memh);
	args.align = align;
	args.len = len;
	if (openfirmware(&args) != 0)
		return 0;
	return (paddr_t)((((paddr_t)args.phys_hi)<<32)|(int)args.phys_lo);
}

/* 
 * Request some specific RAM from the prom
 *
 * Only works while the prom is actively mapping us.
 */
paddr_t
prom_claim_phys(phys, len)
	paddr_t phys;
	int len;
{
	struct {
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
		cell_t rphys_hi;
		cell_t rphys_lo;
	} args;

	if (memh == -1 && ((memh = get_memory_handle()) == -1)) {
		prom_printf("prom_alloc_phys: cannot get memh\r\n");
		return 0;
	}
	args.name = ADR2CELL(&"call-method");
	args.nargs = 6;
	args.nreturns = 3;
	args.method = ADR2CELL(&"claim");
	args.ihandle = HDL2CELL(memh);
	args.align = 0;
	args.len = len;
	args.phys_hi = HDL2CELL(phys>>32);
	args.phys_lo = HDL2CELL(phys);
	if (openfirmware(&args) != 0)
		return -1;
	return (paddr_t)((((paddr_t)args.rphys_hi)<<32)|(int)args.rphys_lo);
}

/* 
 * Free some RAM to prom
 *
 * Only works while the prom is actively mapping us.
 */
int
prom_free_phys(phys, len)
	paddr_t phys;
	int len;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t len;
		cell_t phys_hi;
		cell_t phys_lo;
	} args;

	if (memh == -1 && ((memh = get_memory_handle()) == -1)) {
		prom_printf("prom_free_phys: cannot get memh\r\n");
		return -1;
	}
	args.name = ADR2CELL(&"call-method");
	args.nargs = 5;
	args.nreturns = 0;
	args.method = ADR2CELL(&"release");
	args.ihandle = HDL2CELL(memh);
	args.len = len;
	args.phys_hi = HDL2CELL(phys>>32);
	args.phys_lo = HDL2CELL(phys);
	return openfirmware(&args);
}

/* 
 * Get the msgbuf from the prom.  Only works once.
 *
 * Only works while the prom is actively mapping us.
 */
paddr_t
prom_get_msgbuf(len, align)
	int len;
	int align;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t align;
		cell_t len;
		cell_t id;
		cell_t status;
		cell_t phys_hi;
		cell_t phys_lo;
	} args;
	paddr_t addr;

	if (memh == -1 && ((memh = get_memory_handle()) == -1)) {
		prom_printf("prom_get_msgbuf: cannot get memh\r\n");
		return -1;
	}
	if (OF_test("test-method") == 0) {
		if (OF_test_method(memh, "SUNW,retain") != 0) {
			args.name = ADR2CELL(&"call-method");
			args.nargs = 5;
			args.nreturns = 3;
			args.method = ADR2CELL(&"SUNW,retain");
			args.id = ADR2CELL(&"msgbuf");
			args.ihandle = HDL2CELL(memh);
			args.len = len;
			args.align = align;
			args.status = -1;
			if (openfirmware(&args) == 0 && args.status == 0) {
				return (((paddr_t)args.phys_hi<<32)|args.phys_lo);
			} else prom_printf("prom_get_msgbuf: SUNW,retain failed\r\n");
		} else prom_printf("prom_get_msgbuf: test-method failed\r\n");
	} else prom_printf("prom_get_msgbuf: test failed\r\n");
	/* Allocate random memory -- page zero avail?*/
	addr = prom_claim_phys(0x000, len);
	prom_printf("prom_get_msgbuf: allocated new buf at %08x\r\n", (int)addr); 
	if (addr == -1) {
		prom_printf("prom_get_msgbuf: cannot get allocate physmem\r\n");
		return -1;
	}
	prom_printf("prom_get_msgbuf: claiming new buf at %08x\r\n", (int)addr);
	{ int i; for (i=0; i<200000000; i++); }
	return addr; /* Kluge till we go 64-bit */
}

/* 
 * Low-level prom I/O routines.
 */

static u_int stdin = NULL;
static u_int stdout = NULL;

int 
OF_stdin() 
{
	u_int chosen;

	if (stdin != NULL) 
		return stdin;
		
	chosen = OF_finddevice("/chosen");
	OF_getprop(chosen, "stdin", &stdin, sizeof(stdin));
	return stdin;
}

int
OF_stdout()
{
	u_int chosen;

	if (stdout != NULL) 
		return stdout;
		
	chosen = OF_finddevice("/chosen");
	OF_getprop(chosen, "stdout", &stdout, sizeof(stdout));
	return stdout;
}


/*
 * print debug info to prom. 
 * This is not safe, but then what do you expect?
 */
void
#ifdef __STDC__
prom_printf(const char *fmt, ...)
#else
prom_printf(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	int len;
	static char buf[256];
	va_list ap;

	va_start(ap, fmt);
	len = vsprintf(buf, fmt, ap);
	va_end(ap);

	OF_write(OF_stdout(), buf, len);
}
