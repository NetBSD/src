/*	$NetBSD: ofw_machdep.c,v 1.1.1.1.2.1 1998/07/30 14:03:56 eeh Exp $	*/

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

static int mmuh = -1, memh = -1;

static int get_mmu_handle __P((void));
static int get_memory_handle __P((void));

static int 
get_mmu_handle() {
	int chosen;
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

static int 
get_memory_handle() {
	int chosen;
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
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		u_int64_t tba;
	} args = {
		0,"SUNW,set-trap-table",
		1,
		0,
		NULL
	};

	args.tba = tba;
	return openfirmware(&args);
}

/* 
 * Have the prom convert from virtual to physical addresses.
 *
 * Only works while the prom is actively mapping us.
 */
u_int64_t
prom_vtop(vaddr)
vaddr_t vaddr;
{
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *method;
		int pad2; int ihandle;
		u_int64_t vaddr;
		int64_t status;
		int64_t retaddr;
		int64_t mode;
		u_int64_t paddr_hi;
		u_int64_t paddr_lo;
	} args = {
		0,"call-method",
		3,
		5,
		0,"translate",
		0, 0,
		0, NULL,
		0
	};

	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		prom_printf("prom_vtop: cannot get mmuh\r\n");
		return 0LL;
	}
	args.ihandle = mmuh;
	args.vaddr = vaddr;
	if(openfirmware(&args) != 0)
		return 0LL;
#if 0
	prom_printf("Called \"translate\", mmuh=%x, vaddr=%x, status=%x %x,\r\n retaddr=%x %x, mode=%x %x, phys_hi=%x %x, phys_lo=%x %x\r\n",
		    mmuh, vaddr, (int)(args.status>>32), (int)args.status, (int)(args.retaddr>>32), (int)args.retaddr, 
		    (int)(args.mode>>32), (int)args.mode, (int)(args.paddr_hi>>32), (int)args.paddr_hi,
		    (int)(args.paddr_lo>>32), (int)args.paddr_lo);
#endif
#ifdef INT_IS_64_BITS
	return (u_int64_t)((((u_int64_t)args.paddr_hi)<<32)|(u_int64_t)args.paddr_lo); 
#else
	return args.paddr_lo; /* Kluge till we go 64-bit */
#endif
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
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *method;
		int pad2; int ihandle;
		u_int64_t align;
		u_int64_t len;
		u_int64_t vaddr;
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
		NULL,
		0,
		0
	};

	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		prom_printf("prom_claim_virt: cannot get mmuh\r\n");
		return 0LL;
	}
	args.ihandle = mmuh;
	args.vaddr = vaddr;
	args.len = len;
	if(openfirmware(&args) != 0)
		return 0LL;
	return args.retaddr; /* Kluge till we go 64-bit */
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

	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		prom_printf("prom_alloc_virt: cannot get mmuh\r\n");
		return -1LL;
	}
	args.ihandle = mmuh;
	args.align = align;
	args.len = len;
	if(openfirmware(&args) != 0)
		return -1LL;
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
	static struct {
		int pad0; char *name;
		int64_t nargs;
		int64_t nreturns;
		int pad1; char *method;
		int pad2; int ihandle;
		u_int64_t len;
		u_int64_t vaddr;
	} args = {
		0,"call-method",
		4,
		0,
		0,"release",
		0, 0,
		0,
		NULL
	};

	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		prom_printf("prom_claim_virt: cannot get mmuh\r\n");
		return -1;
	}
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
prom_unmap_virt(vaddr, len)
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
		u_int64_t vaddr;
	} args = {
		0,"call-method",
		4,
		0,
		0,"unmap",
		0, 0,
		0,
		NULL
	};

	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		prom_printf("prom_claim_virt: cannot get mmuh\r\n");
		return -1;
	}
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
prom_map_phys(paddr, size, vaddr, mode)
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
		u_int64_t vaddr;
		int pad4; int paddr_hi;
		int pad6; int paddr_lo;
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

	if (mmuh == -1 && ((mmuh = get_mmu_handle()) == -1)) {
		prom_printf("prom_map_phys: cannot get mmuh\r\n");
		return 0LL;
	}
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
prom_alloc_phys(len, align)
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

	if (memh == -1 && ((memh = get_memory_handle()) == -1)) {
		prom_printf("prom_alloc_phys: cannot get memh\r\n");
		return 0LL;
	}
	args.ihandle = memh;
	args.align = align;
	args.len = len;
	if(openfirmware(&args) != 0)
		return 0LL;
	return args.phys_lo; /* Kluge till we go 64-bit */
}

/* 
 * Request some specific RAM from the prom
 *
 * Only works while the prom is actively mapping us.
 */
u_int64_t
prom_claim_phys(phys, len)
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
		int pad4; u_int32_t phys_hi;
		int pad5; u_int32_t phys_lo;
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

	if (memh == -1 && ((memh = get_memory_handle()) == -1)) {
		prom_printf("prom_alloc_phys: cannot get memh\r\n");
		return 0LL;
	}
	args.ihandle = memh;
	args.len = len;
	args.phys_lo = phys;
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
prom_free_phys(phys, len)
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
		int pad4; u_int32_t phys_hi;
		int pad5; u_int32_t phys_lo;
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

	if (memh == -1 && ((memh = get_memory_handle()) == -1)) {
		prom_printf("prom_free_phys: cannot get memh\r\n");
		return -1;
	}
	args.ihandle = memh;
	args.len = len;
	args.phys_lo = phys;
	return openfirmware(&args);
}

/* 
 * Get the msgbuf from the prom.  Only works once.
 *
 * Only works while the prom is actively mapping us.
 */
u_int64_t
prom_get_msgbuf(len, align)
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
		int pad3; char *id;
		int64_t status;
		int pad4; u_int32_t phys_hi;
		int pad5; u_int32_t phys_lo;
	} args = {
		0,"call-method",
		5,
		3,
		0,"SUNW,retain",
		0, 0,
		0,
		0,
		0, "msgbuf",
		-1,
		0, 0,
		0, 0
	};
	u_int64_t addr;

	if (memh == -1 && ((memh = get_memory_handle()) == -1)) {
		prom_printf("prom_get_msgbuf: cannot get memh\r\n");
		return -1LL;
	}
	args.ihandle = memh;
	args.len = len;
	args.align = align;
	if (OF_test("test-method") == 0) {
		if (OF_test_method(memh, "SUNW,retain") != 0) {
			if (openfirmware(&args) == 0 && args.status == 0) {
				return (((u_int64_t)args.phys_hi<<32)|args.phys_lo);
			} else prom_printf("prom_get_msgbuf: SUNW,retain failed\r\n");
		} else prom_printf("prom_get_msgbuf: test-method failed\r\n");
	} else prom_printf("prom_get_msgbuf: test failed\r\n");
	/* Allocate random memory */
	addr = prom_claim_phys(0x2000, len);
	prom_printf("prom_get_msgbuf: allocated new buf at %08x\r\n", (int)addr); 
	if( !addr ) {
		prom_printf("prom_get_msgbuf: cannot get allocate physmem\r\n");
		return -1LL;
	}
	prom_printf("prom_get_msgbuf: claiming new buf at %08x\r\n", (int)addr); 	
	return addr; /* Kluge till we go 64-bit */
}

/* 
 * Low-level prom I/O routines.
 */

static int stdin = NULL;
static int stdout = NULL;

int 
OF_stdin() 
{
	if( stdin == NULL ) {
		int chosen;
		
		chosen = OF_finddevice("/chosen");
		OF_getprop(chosen, "stdout", &stdin, sizeof(stdin));
	}
	return stdin;
}

int
OF_stdout()
{
	if( stdout == NULL ) {
		int chosen;
		
		chosen = OF_finddevice("/chosen");
		OF_getprop(chosen, "stdout", &stdout, sizeof(stdout));
	}
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

static char *ksprintn __P((u_long, int, int *));

/*
 * Scaled down version of sprintf(3) modified to deal w/va_args.
 */
int
vsprintf(buf, cfmt, ap)
	char *buf;
	const char *cfmt;
	va_list ap;
{
	register const char *fmt = cfmt;
	register char *p, *bp;
	register int ch, base;
	u_long ul;
	int lflag, tmp, width;
	char padc;

	for (bp = buf; ; ) {
		padc = ' ';
		width = 0;
		while ((ch = *(u_char *)fmt++) != '%')
			if ((*bp++ = ch) == '\0')
				return ((bp - buf) - 1);

		lflag = 0;
reswitch:	switch (ch = *(u_char *)fmt++) {
		case '0':
			padc = '0';
			goto reswitch;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			for (width = 0;; ++fmt) {
				width = width * 10 + ch - '0';
				ch = *fmt;
				if (ch < '0' || ch > '9')
					break;
			}
			goto reswitch;
		case 'l':
			lflag = 1;
			goto reswitch;
		/* case 'b': ... break; XXX */
		case 'c':
			*bp++ = va_arg(ap, int);
			break;
		/* case 'r': ... break; XXX */
		case 's':
			p = va_arg(ap, char *);
			while ((*bp++ = *p++) != 0)
				continue;
			--bp;
			break;
		case 'd':
			ul = lflag ? va_arg(ap, long) : va_arg(ap, int);
			if ((long)ul < 0) {
				*bp++ = '-';
				ul = -(long)ul;
			}
			base = 10;
			goto number;
			break;
		case 'o':
			ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
			base = 8;
			goto number;
			break;
		case 'u':
			ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
			base = 10;
			goto number;
			break;
		case 'p':
			*bp++ = '0';
			*bp++ = 'x';
			ul = (u_long)va_arg(ap, void *);
			base = 16;
			goto number;
		case 'x':
			ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
			base = 16;
number:			p = ksprintn(ul, base, &tmp);
			if (width && (width -= tmp) > 0)
				while (width--)
					*bp++ = padc;
			while ((ch = *p--) != 0)
				*bp++ = ch;
			break;
		default:
			*bp++ = '%';
			if (lflag)
				*bp++ = 'l';
			/* FALLTHROUGH */
		case '%':
			*bp++ = ch;
		}
	}
}

/*
 * Put a number (base <= 16) in a buffer in reverse order; return an
 * optional length and a pointer to the NULL terminated (preceded?)
 * buffer.
 */
static char *
ksprintn(ul, base, lenp)
	register u_long ul;
	register int base, *lenp;
{					/* A long in base 8, plus NULL. */
	static char buf[sizeof(long) * NBBY / 3 + 2];
	register char *p;

	p = buf;
	do {
		*++p = "0123456789abcdef"[ul % base];
	} while (ul /= base);
	if (lenp)
		*lenp = p - buf;
	return (p);
}
