/*	$NetBSD: locore.S,v 1.75.2.1 2014/08/10 06:53:49 tls Exp $	*/

/*
 * Copyright-o-rama!
 */

/*
 * Copyright (c) 2007 Manuel Bouyer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Copyright (c) 2006 Mathieu Ropert <mro@adviseo.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/*-
 * Copyright (c) 1998, 2000, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)locore.s	7.3 (Berkeley) 5/13/91
 */

/*
 * override user-land alignment before including asm.h
 */
#define	ALIGN_DATA	.align	8
#define ALIGN_TEXT	.align 16,0x90
#define _ALIGN_TEXT	ALIGN_TEXT

#include <machine/asm.h>

#include "opt_ddb.h"
#include "opt_ddbparam.h"
#include "opt_modular.h"
#include "opt_realmem.h"

#include "opt_compat_netbsd.h"
#include "opt_compat_netbsd32.h"
#include "opt_compat_ibcs2.h"
#include "opt_xen.h"

#include "assym.h"
#include "lapic.h"
#include "ioapic.h"
#include "ksyms.h"

#include <sys/errno.h>
#include <sys/syscall.h>

#include <machine/pte.h>
#include <machine/segments.h>
#include <machine/specialreg.h>
#include <machine/trap.h>
#include <machine/bootinfo.h>
#include <machine/frameasm.h>
#include <machine/cputypes.h>

#if NLAPIC > 0
#include <machine/i82489reg.h>
#endif

/* XXX temporary kluge; these should not be here */
/* Get definitions for IOM_BEGIN, IOM_END, and IOM_SIZE */
#include <dev/isa/isareg.h>

#define	_RELOC(x)	((x) - KERNBASE)
#define	RELOC(x)	_RELOC(_C_LABEL(x))

#ifdef XEN

/*
 * Xen guest identifier and loader selection
 */

.section __xen_guest

	.ascii	"GUEST_OS=NetBSD,GUEST_VER=4.99"
	.ascii	",XEN_VER=xen-3.0"
	.ascii	",LOADER=generic"
	.ascii	",VIRT_BASE=0xffffffff80000000"
	.ascii	",ELF_PADDR_OFFSET=0xffffffff80000000"
	.ascii	",VIRT_ENTRY=0xffffffff80100000"
	.ascii	",HYPERCALL_PAGE=0x00000101" /*  (???+HYPERCALL_PAGE_OFFSET)/PAGE_SIZE) */
#if NKSYMS > 0 || defined(DDB) || defined(MODULAR)
	.ascii	",BSD_SYMTAB=yes"
#endif
	.byte	0

#endif	/* XEN */

/*
 * Initialization
 */
	.data

#if NLAPIC > 0
	.align	PAGE_SIZE
	.globl _C_LABEL(local_apic)
	.globl _C_LABEL(lapic_id)
	.globl _C_LABEL(lapic_tpr)

	.type	_C_LABEL(local_apic), @object
LABEL(local_apic)
	.space	LAPIC_ID
END(local_apic)
	.type	_C_LABEL(lapic_id), @object
LABEL(lapic_id)
	.long	0x00000000
	.space	LAPIC_TPRI-(LAPIC_ID+4)
END(lapic_id)
	.type	_C_LABEL(lapic_tpr), @object
LABEL(lapic_tpr)
	.space	LAPIC_PPRI-LAPIC_TPRI
END(lapic_tpr)
	.type	_C_LABEL(lapic_ppr), @object
_C_LABEL(lapic_ppr):
	.space	LAPIC_ISR-LAPIC_PPRI
END(lapic_ppr)
	.type	_C_LABEL(lapic_isr), @object
_C_LABEL(lapic_isr):
	.space	PAGE_SIZE-LAPIC_ISR
END(lapic_isr)
#endif

	.globl	_C_LABEL(cpu_id)
	.globl	_C_LABEL(cpu_vendorname)
	.globl	_C_LABEL(cpu_brand_id)
	.globl	_C_LABEL(cpuid_level)
	.globl	_C_LABEL(esym)
	.globl	_C_LABEL(eblob)
	.globl	_C_LABEL(boothowto)
	.globl	_C_LABEL(bootinfo)
	.globl	_C_LABEL(atdevbase)
	.globl	_C_LABEL(PDPpaddr)
	.globl	_C_LABEL(biosbasemem)
	.globl	_C_LABEL(biosextmem)
	.globl	_C_LABEL(gdtstore)
	.globl	_C_LABEL(cputype)

	.type	_C_LABEL(cputype), @object
LABEL(cputype)		.long	0	# are we 80486, Pentium, or..
END(cputype)
	.type	_C_LABEL(cpu_id), @object
LABEL(cpu_id)		.long	0	# saved from `cpuid' instruction
END(cpu_id)
	.type	_C_LABEL(cpuid_level), @object
LABEL(cpuid_level)	.long	-1	# max. level accepted by 'cpuid'
					#   instruction
END(cpuid_level)
	.type	_C_LABEL(cpu_vendorname), @object
LABEL(cpu_vendorname)	.space	16	# vendor string returned by `cpuid'
					#   instruction
END(cpu_vendorname)
	.type	_C_LABEL(cpu_brand_id), @object
LABEL(cpu_brand_id)	.long	0	# brand ID from 'cpuid' instruction
END(cpu_brand_id)
	.type	_C_LABEL(esym), @object
LABEL(esym)		.quad	0	# ptr to end of syms
END(esym)
	.type	_C_LABEL(eblob), @object
LABEL(eblob)		.quad	0	# ptr to end of modules
END(eblob)
	.type	_C_LABEL(atdevbase), @object
LABEL(atdevbase)	.quad	0	# location of start of iomem in virtual
END(atdevbase)
	.type	_C_LABEL(PDPpaddr), @object
LABEL(PDPpaddr)		.quad	0	# paddr of PTD, for libkvm
END(PDPpaddr)
	.type	_C_LABEL(biosbasemem), @object
#ifndef REALBASEMEM
LABEL(biosbasemem)	.long	0	# base memory reported by BIOS
#else
LABEL(biosbasemem)	.long	REALBASEMEM
#endif
END(biosbasemem)
	.type	_C_LABEL(biosextmem), @object
#ifndef REALEXTMEM
LABEL(biosextmem)	.long	0	# extended memory reported by BIOS
#else
LABEL(biosextmem)	.long	REALEXTMEM
#endif
END(biosextmem)

#ifndef XEN
	.globl	gdt64_lo
	.globl	gdt64_hi

#define GDT64_LIMIT gdt64_end-gdt64_start-1

/* Temporary gdt64, with base address in low memory */
	.type	_C_LABEL(gdt64_lo), @object
LABEL(gdt64_lo)
	.word	GDT64_LIMIT
	.quad	_RELOC(gdt64_start)
END(gdt64_lo)
.align 64

/* Temporary gdt64, with base address in high memory */
	.type	_C_LABEL(gdt64_hi), @object
LABEL(gdt64_hi)
	.word	GDT64_LIMIT
	.quad	gdt64_start
END(gdt64_hi)
.align 64

#undef GDT64_LIMIT

	.type	_C_LABEL(gdt64_start), @object
_C_LABEL(gdt64_start):
	.quad 0x0000000000000000	/* always empty */
	.quad 0x00af9a000000ffff	/* kernel CS */
	.quad 0x00cf92000000ffff	/* kernel DS */
END(gdt64_start)
gdt64_end:

	.type	_C_LABEL(farjmp64), @object
_C_LABEL(farjmp64):
	.long	_RELOC(longmode)
	.word	GSEL(GCODE_SEL, SEL_KPL)
END(farjmp64)
	
#endif	/* !XEN */

	.size	tmpstk, tmpstk - .
	.space 512
tmpstk:

	.globl _C_LABEL(cpu_private)
	.comm _C_LABEL(cpu_private),PAGE_SIZE,PAGE_SIZE

/*
 * Some hackage to deal with 64bit symbols in 32 bit mode.
 * This may not be needed it things are cleaned up a little.
 */


	.text
	.globl	_C_LABEL(kernel_text)
	.set	_C_LABEL(kernel_text),KERNTEXTOFF

ENTRY(start)
#ifndef XEN
	.code32
	movw	$0x1234,0x472			# warm boot
	/*
	 * Load parameters from stack
	 * (howto, [bootdev], bootinfo, esym, basemem, extmem).
	 */
	movl	4(%esp),%eax
	movl	%eax,RELOC(boothowto)
	movl	12(%esp),%eax
	testl	%eax, %eax
	jz	1f
	movl	(%eax), %ebx		/* number of entries */
	movl	$RELOC(bootinfo),%ebp
	movl	%ebp, %edx
	addl	$BOOTINFO_MAXSIZE,%ebp
	movl	%ebx, (%edx)
	addl	$4, %edx
2:
	testl	%ebx, %ebx
	jz	1f
	addl	$4, %eax
	movl	(%eax), %ecx		/* address of entry */
	pushl	%edi
	pushl	%esi
	pushl	%eax

	movl	(%ecx),%eax	/* len */
	movl	%edx,%edi
	addl	(%ecx), %edx		/* update dest pointer */
	cmpl	%ebp, %edx
	jg	2f
	movl	%ecx,%esi
	movl	%eax,%ecx
	/*
	 * If any modules were loaded, record where they
	 * end.  We'll need to skip over them.
	 */
	cmpl	$BTINFO_MODULELIST, 4(%esi)
	jne	0f
	pushl	12(%esi)		/* endpa */
	popl	RELOC(eblob)
	addl	$KERNBASE_LO, RELOC(eblob)
	adcl	$KERNBASE_HI, RELOC(eblob)+4
0:
	rep
	movsb
	popl	%eax
	popl	%esi
	popl	%edi
	subl	$1, %ebx
	jmp	2b
2:	/* cleanup for overflow case */
	popl	%eax
	popl	%esi
	popl	%edi
	movl	$RELOC(bootinfo),%ebp
	movl	%ebp, %edx
	subl	%ebx, (%edx)		/* correct number of entries */
1:

	movl	16(%esp),%eax
	testl	%eax,%eax
	jz	1f
	addl	$KERNBASE_LO,%eax
1: 	movl	$RELOC(esym),%ebp
	movl	%eax,(%ebp)
	movl	$KERNBASE_HI,4(%ebp)

	movl	$RELOC(biosextmem),%ebp
	movl	(%ebp),%eax
	testl	%eax,%eax
	jnz	1f
	movl	20(%esp),%eax
	movl	%eax,(%ebp)
1:
	movl	$RELOC(biosbasemem),%ebp
	movl	(%ebp),%eax
	testl	%eax,%eax
	jnz	1f
	movl	24(%esp),%eax
	movl	%eax,(%ebp)
1:

	/* First, reset the PSL. */
	pushl	$PSL_MBO
	popfl

	xorl	%eax,%eax
	cpuid
	movl	%eax,RELOC(cpuid_level)
	movl	$RELOC(cpu_vendorname),%ebp
	movl	%ebx,(%ebp)
	movl	%edx,4(%ebp)
	movl	%ecx,8(%ebp)
	movl	$0, 12(%ebp)

	movl	$1,%eax
	cpuid
	movl	%eax,RELOC(cpu_id)

	/* Brand ID is bits 0-7 of %ebx */
	andl	$255,%ebx
	movl	%ebx,RELOC(cpu_brand_id)

	/*
	 * Finished with old stack; load new %esp now instead of later so we
	 * can trace this code without having to worry about the trace trap
	 * clobbering the memory test or the zeroing of the bss+bootstrap page
	 * tables.
	 *
	 * The boot program should check:
	 *	text+data <= &stack_variable - more_space_for_stack
	 *	text+data+bss+pad+space_for_page_tables <= end_of_memory
	 * Oops, the gdt is in the carcass of the boot program so clearing
	 * the rest of memory is still not possible.
	 */
	movl	$RELOC(tmpstk),%esp

/*
 * Virtual address space of kernel:
 *
 * text | data | bss | [syms] | page dir | proc0 kstack | L1 ptp | L2 ptp | L3
 *			      0          1       2      3
 */

#if L2_SLOT_KERNBASE > 0
#define TABLE_L2_ENTRIES (2 * (NKL2_KIMG_ENTRIES + 1))
#else
#define TABLE_L2_ENTRIES (NKL2_KIMG_ENTRIES + 1)
#endif

#if L3_SLOT_KERNBASE > 0
#define TABLE_L3_ENTRIES (2 * NKL3_KIMG_ENTRIES)
#else
#define TABLE_L3_ENTRIES NKL3_KIMG_ENTRIES
#endif


#define PROC0_PML4_OFF	0
#define PROC0_STK_OFF	(PROC0_PML4_OFF + PAGE_SIZE)
#define PROC0_PTP3_OFF	(PROC0_STK_OFF + UPAGES * PAGE_SIZE)
#define PROC0_PTP2_OFF	(PROC0_PTP3_OFF + NKL4_KIMG_ENTRIES * PAGE_SIZE)
#define PROC0_PTP1_OFF	(PROC0_PTP2_OFF + TABLE_L3_ENTRIES * PAGE_SIZE)
#define TABLESIZE \
  ((NKL4_KIMG_ENTRIES + TABLE_L3_ENTRIES + TABLE_L2_ENTRIES + 1 + UPAGES) \
    * PAGE_SIZE)

#define fillkpt	\
1:	movl	%eax,(%ebx)		; 	/* store phys addr */	\
	movl	$0,4(%ebx)		; 	/* upper 32 bits 0 */	\
	addl	$8,%ebx			; 	/* next pte/pde */	\
	addl	$PAGE_SIZE,%eax		; 	/* next phys page */	\
	loop	1b			;

	/* Find end of kernel image. */
	movl	$RELOC(end),%edi
#if (NKSYMS || defined(DDB) || defined(MODULAR)) && !defined(SYMTAB_SPACE)
	/* Save the symbols (if loaded). */
	movl	RELOC(esym),%eax
	testl	%eax,%eax
	jz	1f
	subl	$KERNBASE_LO,%eax	/* XXX */
	movl	%eax,%edi
1:
#endif
	/* Skip over any modules/blobs. */
	movl	RELOC(eblob),%eax
	testl	%eax,%eax
	jz	1f
	subl	$KERNBASE_LO,%eax	/* XXX */
	movl	%eax,%edi
1:
	/* Compute sizes */
	movl	%edi,%esi
	addl	$PGOFSET,%esi			# page align up
	andl	$~PGOFSET,%esi

	movl	%esi,%edi
	xorl	%eax,%eax
	cld
	movl	$TABLESIZE,%ecx
	shrl	$2,%ecx
	rep
	stosl

	leal	(PROC0_PTP1_OFF)(%esi), %ebx

/*
 * Build initial page tables.
 */
	/*
	 * Compute &__data_start - KERNBASE. This can't be > 4G,
	 * or we can't deal with it anyway, since we can't load it in
	 * 32 bit mode. So use the bottom 32 bits.
	 */
	movl	$RELOC(__data_start),%edx
	andl	$~PGOFSET,%edx

	/*
	 * Skip the first MB.
	 */
	movl	$(KERNTEXTOFF_LO - KERNBASE_LO),%eax
	movl	%eax,%ecx
	shrl	$(PGSHIFT-3),%ecx	/* ((n >> PGSHIFT) << 3) for # pdes */
	addl	%ecx,%ebx

	/* Map the kernel text read-only. */
	movl	%edx,%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KR),%eax
	fillkpt

	/* Map the data, BSS, and bootstrap tables read-write. */
	leal	(PG_V|PG_KW)(%edx),%eax
	movl	$TABLESIZE,%ecx
	addl	%esi,%ecx				# end of tables
	subl	%edx,%ecx				# subtract end of text
	shrl	$PGSHIFT,%ecx
	fillkpt

	/* Map ISA I/O mem (later atdevbase) */
	movl	$(IOM_BEGIN|PG_V|PG_KW/*|PG_N*/),%eax	# having these bits set
	movl	$(IOM_SIZE>>PGSHIFT),%ecx		# for this many pte s,
	fillkpt

/*
 * Construct a page table directory.
 */
	/* Set up level 2 pages */
	leal	(PROC0_PTP2_OFF)(%esi),%ebx
	leal	(PROC0_PTP1_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW), %eax
	movl	$(NKL2_KIMG_ENTRIES+1),%ecx
	fillkpt

#if L2_SLOT_KERNBASE > 0
	/* If needed, set up level 2 entries for actual kernel mapping */
	leal	(PROC0_PTP2_OFF+ L2_SLOT_KERNBASE*8)(%esi),%ebx
	leal	(PROC0_PTP1_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW), %eax
	movl	$(NKL2_KIMG_ENTRIES+1),%ecx
	fillkpt
#endif

	/* Set up level 3 pages */
	leal	(PROC0_PTP3_OFF)(%esi),%ebx
	leal	(PROC0_PTP2_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW), %eax
	movl	$NKL3_KIMG_ENTRIES,%ecx
	fillkpt

#if L3_SLOT_KERNBASE > 0
	/* If needed, set up level 3 entries for actual kernel mapping */
	leal	(PROC0_PTP3_OFF+ L3_SLOT_KERNBASE*8)(%esi),%ebx
	leal	(PROC0_PTP2_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW), %eax
	movl	$NKL3_KIMG_ENTRIES,%ecx
	fillkpt
#endif

	/* Set up top level entries for identity mapping */
	leal	(PROC0_PML4_OFF)(%esi),%ebx
	leal	(PROC0_PTP3_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW), %eax
	movl	$NKL4_KIMG_ENTRIES,%ecx
	fillkpt

	/* Set up top level entries for actual kernel mapping */
	leal	(PROC0_PML4_OFF + L4_SLOT_KERNBASE*8)(%esi),%ebx
	leal	(PROC0_PTP3_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW), %eax
	movl	$NKL4_KIMG_ENTRIES,%ecx
	fillkpt

	/* Install recursive top level PDE */
	leal	(PROC0_PML4_OFF + PDIR_SLOT_PTE*8)(%esi),%ebx
	leal	(PROC0_PML4_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW),%eax
	movl	%eax,(%ebx)
	movl	$0, 4(%ebx)


	/* Save phys. addr of PTD, for libkvm. */
	movl	$RELOC(PDPpaddr),%ebp
	movl	%esi,(%ebp)
	movl	$0,4(%ebp)

	/*
	 * Startup checklist:
	 * 1. Enable PAE (and SSE while here).
	 */
	movl	%cr4,%eax
	orl	$(CR4_PAE|CR4_OSFXSR|CR4_OSXMMEXCPT),%eax
	movl	%eax,%cr4

	/*
	 * 2. Set Long Mode Enable in EFER. Also enable the
	 *    syscall extensions.
	 */
	movl	$MSR_EFER,%ecx
	rdmsr
	xorl	%eax,%eax	/* XXX */
	orl	$(EFER_LME|EFER_SCE),%eax
	wrmsr

	/*
	 * 3. Load %cr3 with pointer to PML4.
	 */
	movl	%esi,%eax
	movl	%eax,%cr3

	/*
	 * 4. Enable paging and the rest of it.
	 */
	movl	%cr0,%eax
	orl	$(CR0_PE|CR0_PG|CR0_NE|CR0_TS|CR0_MP|CR0_WP|CR0_AM),%eax
	movl	%eax,%cr0
	jmp	compat
compat:

	/*
	 * 5.
	 * Not quite done yet, we're now in a compatibility segment,
	 * in legacy mode. We must jump to a long mode segment.
	 * Need to set up a temporary GDT with a long mode segment
	 * in it to do that.
	 */

	movl	$RELOC(gdt64_lo),%eax
	lgdt	(%eax)
	movl	$RELOC(farjmp64),%eax
	ljmp	*(%eax)

.code64
longmode:
	/*
	 * 6.
	 * Finally, we're in long mode. However, we're still
	 * in the identity mapped area (could not jump out
	 * of that earlier because it would have been a > 32bit
	 * jump). We can do that now, so here we go.
	 */
	movabsq	$longmode_hi,%rax
	jmp	*%rax

longmode_hi:

	/*
	 * We left the identity mapped area. Base address of
	 * the temporary gdt64 should now be in high memory.
	 */
	movq	$RELOC(gdt64_hi),%rax
	lgdt	(%rax)

	/*
	 * We have arrived.
	 * There's no need anymore for the identity mapping in low
	 * memory, remove it.
	 */
	movq	$KERNBASE,%r8

#if L2_SLOT_KERNBASE > 0
	movq	$(NKL2_KIMG_ENTRIES+1),%rcx
	leaq	(PROC0_PTP2_OFF)(%rsi),%rbx
	addq	%r8, %rbx
1:	movq	$0,(%rbx)
	addq	$8,%rbx
	loop	1b
#endif

#if L3_SLOT_KERNBASE > 0
	movq	$NKL3_KIMG_ENTRIES,%rcx
	leaq	(PROC0_PTP3_OFF)(%rsi),%rbx
	addq	%r8, %rbx
1:	movq	$0,(%rbx)
	addq	$8,%rbx
	loop	1b
#endif

	movq	$NKL4_KIMG_ENTRIES,%rcx
	leaq	(PROC0_PML4_OFF)(%rsi),%rbx	# old, phys  address of PML4
	addq	%r8, %rbx			# new, virtual address of PML4
1:	movq	$0,(%rbx)
	addq	$8,%rbx
	loop	1b

	/* Relocate atdevbase. */
	movq	$(TABLESIZE+KERNBASE),%rdx
	addq	%rsi,%rdx
	movq	%rdx,_C_LABEL(atdevbase)(%rip)

	/* Set up bootstrap stack. */
	leaq	(PROC0_STK_OFF)(%rsi),%rax
	addq	%r8,%rax
	movq	%rax,(_C_LABEL(lwp0)+L_PCB)(%rip) /* XXX L_PCB != uarea */
	leaq	(USPACE-FRAMESIZE)(%rax),%rsp
	movq	%rsi,PCB_CR3(%rax)	# pcb->pcb_cr3
	xorq	%rbp,%rbp		# mark end of frames

	xorw	%ax,%ax
	movw	%ax,%gs
	movw	%ax,%fs

	/* XXX merge these */
	leaq	(TABLESIZE+IOM_SIZE)(%rsi),%rdi

#else	/* XEN */
	/* First, reset the PSL. */
	pushq	$2
	popfq

	cld

	/*
	 * Xen info:
	 * - %rsi -> start_info struct
	 * - %rsp -> stack, *theorically* the last used page
	 *	by Xen bootstrap
	 */
	movq	%rsi, %rbx

	/* Clear BSS. */
	xorq	%rax,%rax
	movq	$_C_LABEL(__bss_start),%rdi
	movq	$_C_LABEL(_end),%rcx
	subq	%rdi,%rcx
	rep
	stosb

	/* Copy start_info to a safe place */
	movq	%rbx,%rsi
	movq	$_C_LABEL(start_info_union),%rdi
	movq	$64,%rcx
	rep
	movsq

	/*
	 * Memory layout at start of the day:
	 * - Kernel image
	 * - Page frames list
	 * - start_info struct. we copied it, so it can be recycled.
	 * - xenstore
	 * - console
	 * - Xen bootstrap page tables
	 * - kernel stack. provided by Xen
	 * - guaranted 512kB padding
	 *
	 * As we want to rebuild our page tables and place our stack
	 * in proc0 struct, all data starting from after console can be
	 * discarded after we've done a little setup.
	 */

	/*
	 * We want our own page tables, let's rebuild them
	 * We will reclaim xen space afterward INCLUDING stack
	 * so let's change it to a temporary one
	 */

	movq	$tmpstk, %rax
	subq	$8, %rax
	movq	%rax, %rsp

	xorl	%eax,%eax
	cpuid
	movl	%eax,_C_LABEL(cpuid_level)

	movq	$cpu_info_primary, %rdi
	movq	%rdi, CPU_INFO_SELF(%rdi) /* ci->ci_self = ci */
	movq	$1, %rsi
	call	cpu_init_msrs	/* cpu_init_msrs(ci, true); */

	call	xen_pmap_bootstrap

	/*
	 * First avail returned by xen_pmap_bootstrap in %rax
	 */
	movq	%rax, %rsi
	movq	%rsi,(_C_LABEL(lwp0)+L_PCB)	/* XXX L_PCB != uarea */
	
	/*
	 * Set new stack and clear segments
	 */

	leaq	(USPACE-FRAMESIZE)(%rsi),%rsp
	xorq	%rbp,%rbp

	xorw	%ax,%ax
	movw	%ax,%gs
	movw	%ax,%fs
	
	/*
	 * Set first_avail after proc0
	 */

	movq	%rsi,%rdi
	addq	$USPACE,%rdi
	subq	$KERNBASE,%rdi		# init_x86_64 want a physical address

#endif	/* XEN */
	call	_C_LABEL(init_x86_64)

	call 	_C_LABEL(main)
END(start)

#if defined(XEN)
/* space for the hypercall call page */
#define HYPERCALL_PAGE_OFFSET 0x1000
.org HYPERCALL_PAGE_OFFSET
ENTRY(hypercall_page)
.skip 0x1000
END(hypercall_page)
#endif /* XEN */

/*
 * int setjmp(label_t *)
 *
 * Used primarily by DDB.
 */
ENTRY(setjmp)
	/*
	 * Only save registers that must be preserved across function
	 * calls according to the ABI (%rbx, %rsp, %rbp, %r12-%r15)
	 * and %rip.
	 */
	movq	%rdi,%rax
	movq	%rbx,(%rax)
	movq	%rsp,8(%rax)
	movq	%rbp,16(%rax)
	movq	%r12,24(%rax)
	movq	%r13,32(%rax)
	movq	%r14,40(%rax)
	movq	%r15,48(%rax)
	movq	(%rsp),%rdx
	movq	%rdx,56(%rax)
	xorl	%eax,%eax
	ret
END(setjmp)

/*
 * int longjmp(label_t *)
 *
 * Used primarily by DDB.
 */
ENTRY(longjmp)
	movq	%rdi,%rax
	movq	(%rax),%rbx
	movq	8(%rax),%rsp
	movq	16(%rax),%rbp
	movq	24(%rax),%r12
	movq	32(%rax),%r13
	movq	40(%rax),%r14
	movq	48(%rax),%r15
	movq	56(%rax),%rdx
	movq	%rdx,(%rsp)
	movl	$1,%eax
	ret
END(longjmp)

/*
 * void dumpsys(void)
 *
 * Mimic cpu_switchto() for postmortem debugging.
 */
ENTRY(dumpsys)
	# build a fake switch frame.
	pushq	%rbx
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	
	# save a context.
	movq	$dumppcb, %rax
	movq	%rsp, PCB_RSP(%rax)
	movq	%rbp, PCB_RBP(%rax)

	call	_C_LABEL(dodumpsys)

	addq	$(5*8), %rsp	# sizeof(switchframe) - sizeof(%rip)
	ret
END(dumpsys)

/*
 * struct lwp *cpu_switchto(struct lwp *oldlwp, struct lwp *newlwp,
 *			    bool returning)
 *
 *	1. if (oldlwp != NULL), save its context.
 *	2. then, restore context of newlwp.
 *
 * Note that the stack frame layout is known to "struct switchframe" in
 * <machine/frame.h> and to the code in cpu_lwp_fork() which initializes
 * it for a new lwp.
 */
ENTRY(cpu_switchto)
	pushq	%rbx
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15

	movq	%rdi,%r13	# oldlwp
	movq	%rsi,%r12	# newlwp

	testq	%r13,%r13
	jz	1f

	/* Save old context. */
	movq	L_PCB(%r13),%rax
	movq	%rsp,PCB_RSP(%rax)
	movq	%rbp,PCB_RBP(%rax)

	/* Switch to newlwp's stack. */
1:	movq	L_PCB(%r12),%r14
#ifdef XEN /* XXX debug code */
	cmpq	$0, PCB_RSP(%r14)
	jne 999f
	callq _C_LABEL(cpu_Debugger);
999:
#endif
	movq	PCB_RSP(%r14),%rsp
	movq	PCB_RBP(%r14),%rbp

	/*
	 * Set curlwp.  This must be globally visible in order to permit
	 * non-interlocked mutex release.
	 */
	movq	%r12,%rcx
	xchgq	%rcx,CPUVAR(CURLWP)

	/* Skip the rest if returning to a pinned LWP. */
	testb	%dl,%dl
	jnz	4f

	/* Switch ring0 stack */
#ifndef XEN
	movq	PCB_RSP0(%r14),%rax
	movq	%rax,CPUVAR(RSP0)
#else
	movq	%r14, %rdi
	callq	_C_LABEL(x86_64_switch_context);
#endif

	/* Don't bother with the rest if switching to a system process. */
	testl	$LW_SYSTEM,L_FLAG(%r12)
	jnz	4f

	/* Is this process using RAS (restartable atomic sequences)? */
	movq	L_PROC(%r12),%rdi
	cmpq	$0,P_RASLIST(%rdi)
	jne	5f

	/*
	 * Restore cr0 including FPU state (may have CR0_TS set).  Note that
	 * IPL_SCHED prevents from FPU interrupt altering the LWP's saved cr0.
	 */
2:
#ifndef XEN
	movl	$IPL_HIGH,CPUVAR(ILEVEL)
	movl	PCB_CR0(%r14),%ecx	/* has CR0_TS clear */
	movq	%cr0,%rdx

	/*
	 * If our floating point registers are on a different CPU,
	 * set CR0_TS so we'll trap rather than reuse bogus state.
	 */
	cmpq	CPUVAR(FPCURLWP),%r12
	je	3f
	orq	$CR0_TS,%rcx

	/* Reloading CR0 is very expensive - avoid if possible. */
3:	cmpq	%rdx,%rcx
	je	6f
	movq	%rcx,%cr0

6:	testl	$PCB_COMPAT32, PCB_FLAGS(%r14)
	jne	32f

	/* Zero out %fs/%gs registers and GDT descriptors. */
	xorq	%rax, %rax
	movw	%ax, %fs
	CLI(cx)
	SWAPGS
	movw	%ax, %gs
	SWAPGS
	STI(cx)

	movq	CPUVAR(GDT),%rcx
	movq	%rax, (GUFS_SEL*8)(%rcx)
	movq	%rax, (GUGS_SEL*8)(%rcx)

	/* Reload 64-bit %fs/%gs MSRs. */
	movl	$MSR_FSBASE, %ecx
	movl	PCB_FS(%r14), %eax
	movl	4+PCB_FS(%r14), %edx
	wrmsr
	movl	$MSR_KERNELGSBASE, %ecx
	movl	PCB_GS(%r14), %eax
	movl	4+PCB_GS(%r14), %edx
	wrmsr
	jmp	4f

32:
	/* Reload %fs/%gs GDT descriptors. */
	movq	CPUVAR(GDT),%rcx
	movq	PCB_FS(%r14), %rax
	movq	%rax, (GUFS_SEL*8)(%rcx)
	movq	PCB_GS(%r14), %rax
	movq	%rax, (GUGS_SEL*8)(%rcx)

	/* Reload %fs and %gs */
	movq	L_MD_REGS(%r12), %rbx
	movw	TF_FS(%rbx), %fs
	CLI(ax)
	SWAPGS
	movw	TF_GS(%rbx), %gs
	SWAPGS
	STI(ax)

#else
	movq	%r12,%rdi
	callq	_C_LABEL(x86_64_tls_switch)
#endif
	
	/* Return to the new LWP, returning 'oldlwp' in %rax. */
4:	movq	%r13,%rax
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%rbx
	ret

	/* Check for restartable atomic sequences (RAS). */
5:	movq	L_MD_REGS(%r12),%rbx
	movq	TF_RIP(%rbx),%rsi
	call	_C_LABEL(ras_lookup)
	cmpq	$-1,%rax
	je	2b
	movq	%rax,TF_RIP(%rbx)
	jmp	2b
END(cpu_switchto)

/*
 * void savectx(struct pcb *pcb);
 *
 * Update pcb, saving current processor state.
 */
ENTRY(savectx)
	/* Save stack pointers. */
	movq	%rsp,PCB_RSP(%rdi)
	movq	%rbp,PCB_RBP(%rdi)
	ret
END(savectx)

IDTVEC(syscall32)
	sysret		/* go away please */
IDTVEC_END(syscall32)

/*
 * syscall()
 *
 * syscall insn entry.
 * This currently isn't much faster, but it can be made faster in the future.
 * (Actually we've already saved a few 100 clocks by not loading the trap gate)
 */
IDTVEC(syscall)
#ifndef XEN
	/*
	 * The user %rip is in %rcx and the user %flags in %r11.
	 * The kernel %cs and %ss are loaded, but nothing else is.
	 * The 'swapgs' gives us access to cpu-specific memory where
	 * we can save a user register and then read the lwps
	 * kernel stack pointer,
	 * This code doesn't seem to set %ds, this may not matter since it
	 * is ignored in 64bit mode, OTOH the syscall instruction sets %ss
	 * and that is ignored as well.
	 */
	swapgs
	movq	%r15,CPUVAR(SCRATCH)
	movq	CPUVAR(CURLWP),%r15
	movq	L_PCB(%r15),%r15
	movq	PCB_RSP0(%r15),%r15	/* LWP's kernel stack pointer */

	/* Make stack look like an 'int nn' frame */
#define SP(x)	(x)-(TF_SS+8)(%r15)
	movq	$(LSEL(LUDATA_SEL, SEL_UPL)), SP(TF_SS) /* user %ss */
	movq	%rsp, SP(TF_RSP) 	/* User space rsp */

	movq	%r11, SP(TF_RFLAGS)	/* old rflags from syscall insn */
	movq	$(LSEL(LUCODE_SEL, SEL_UPL)), SP(TF_CS)
	movq	%rcx, SP(TF_RIP)	/* syscall saves rip in rcx */

	leaq	SP(0),%rsp		/* %rsp now valid after frame */
	movq	CPUVAR(SCRATCH),%r15
#undef SP

	movq	$2,TF_ERR(%rsp)		/* syscall instruction size */
	movq	$T_ASTFLT, TF_TRAPNO(%rsp)

	movw	%es,TF_ES(%rsp)
	sti
	INTR_SAVE_GPRS
	movw	%fs,TF_FS(%rsp)
	movw	%gs,TF_GS(%rsp)
	movw	$(LSEL(LUDATA_SEL, SEL_UPL)),TF_DS(%rsp)
#else
	/* Xen already switched to kernel stack */
	pushq	%rsi
	STI(si)
	popq	%rsi
	addq	$0x10,%rsp	/* gap to match cs:rip */
	pushq	$2		/* error code */
	pushq	$T_ASTFLT
	subq	$TF_REGSIZE,%rsp
	INTR_SAVE_GPRS
	movw	%fs,TF_FS(%rsp)
	movw	%gs,TF_GS(%rsp)
	movw	%es,TF_ES(%rsp)
	movw	$(LSEL(LUDATA_SEL, SEL_UPL)),TF_DS(%rsp)
#endif

do_syscall:
	movq	CPUVAR(CURLWP),%r14
	incq	CPUVAR(NSYSCALL)	# count it atomically
	movq	%rsp,L_MD_REGS(%r14)	# save pointer to frame
	movq	L_PROC(%r14),%r15
	andl	$~MDL_IRET,L_MD_FLAGS(%r14)   /* Allow sysret return */
	movq	%rsp,%rdi		/* Pass frame as arg0 */
	call	*P_MD_SYSCALL(%r15)
.Lsyscall_checkast:
	/*
	 * Disable interrupts to avoid new ASTs (etc) being added and
	 * to ensure we don't take an interrupt with some of the user
	 * registers loaded.
	 */
	CLI(si)
	/* Check for ASTs on exit to user mode. */
	movl	L_MD_ASTPENDING(%r14), %eax
	orl	CPUVAR(WANT_PMAPLOAD), %eax
	jnz	9f
#ifdef DIAGNOSTIC
	cmpl	$IPL_NONE,CPUVAR(ILEVEL)
	jne	3f
#endif
	testl	$(MDL_IRET | MDL_COMPAT32), L_MD_FLAGS(%r14)
	INTR_RESTORE_GPRS
	movw	TF_ES(%rsp),%es
	SWAPGS
	jnz	2f
#ifndef XEN
	movq	TF_RIP(%rsp), %rcx	/* %rip for sysret */
	movq	TF_RFLAGS(%rsp), %r11	/* %flags for sysret */
	movw	TF_DS(%rsp), %ds
	movq	TF_RSP(%rsp), %rsp
	sysretq
#else
	movw	TF_DS(%rsp), %ds
	addq	$TF_RIP, %rsp
	pushq	$256	/* VGCF_IN_SYSCALL */
	jmp	HYPERVISOR_iret
#endif

/*
 * If the syscall might have modified some registers, or we are a 32bit
 * process we must return to user with an 'iret' instruction.
 * If the iret faults in kernel (assumed due to illegal register values)
 * then a SIGSEGV will be signalled.
 */
2:
	movw	TF_DS(%rsp), %ds
	addq	$TF_RIP, %rsp
	iretq

/* Report SPL error */
#ifdef DIAGNOSTIC
3:	movabsq	$4f, %rdi
	movl	TF_RAX(%rsp),%esi
	movl	TF_RDI(%rsp),%edx
	movl	%ebx,%ecx
	movl	CPUVAR(ILEVEL),%r8d
	xorq	%rax,%rax
	call	_C_LABEL(printf)
	movl	$IPL_NONE,%edi
	call	_C_LABEL(spllower)
	jmp	.Lsyscall_checkast
4:	.asciz	"WARNING: SPL NOT LOWERED ON SYSCALL %d %d EXIT %x %x\n"
#endif

/* AST pending or pmap load needed */
9:
	cmpl	$0, CPUVAR(WANT_PMAPLOAD)
	jz	10f
	STI(si)
	call	_C_LABEL(do_pmap_load)
	jmp	.Lsyscall_checkast	/* re-check ASTs */
10:
	CLEAR_ASTPENDING(%r14)
	STI(si)
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	movq	%rsp,%rdi
	call	_C_LABEL(trap)
	jmp	.Lsyscall_checkast	/* re-check ASTs */
IDTVEC_END(syscall)

/*
 * void lwp_trampoline(void);
 *
 * This is a trampoline function pushed run by newly created LWPs
 * in order to do additional setup in their context.
 */
NENTRY(lwp_trampoline)
	movq	%rbp,%rsi
	movq	%rbp,%r14	/* for .Lsyscall_checkast */
	movq	%rax,%rdi
	xorq	%rbp,%rbp
	call	_C_LABEL(lwp_startup)
	movq	%r13,%rdi
	call	*%r12
	jmp	.Lsyscall_checkast
END(lwp_trampoline)

/*
 * oosyscall()
 *
 * Old call gate entry for syscall. only needed if we're
 * going to support running old i386 NetBSD 1.0 or ibcs2 binaries, etc,
 * on NetBSD/amd64.
 * The 64bit call gate can't request that arguments be copied from the
 * user stack (which the i386 code uses to get a gap for the flags).
 * push/pop are <read>:<modify_sp>:<write> cycles.
 */
IDTVEC(oosyscall)
	/* Set rflags in trap frame. */
	pushq	(%rsp)		# move user's %eip
	pushq	16(%rsp)	# and %cs
	popq	8(%rsp)
	pushfq
	popq	16(%rsp)
	pushq	$7		# size of instruction for restart
	jmp	osyscall1
IDTVEC_END(oosyscall)

/*
 * osyscall()
 *
 * Trap gate entry for int $80 syscall, also used by sigreturn.
 */
IDTVEC(osyscall)
#ifdef XEN
	movq (%rsp),%rcx
	movq 8(%rsp),%r11
	addq $0x10,%rsp
#endif
	pushq	$2		# size of instruction for restart
osyscall1:
	pushq	$T_ASTFLT	# trap # for doing ASTs
	INTRENTRY
	STI(si)
	jmp	do_syscall
IDTVEC_END(osyscall)

/*
 * bool sse2_idlezero_page(void *pg)
 *
 * Zero a page without polluting the cache.  Preemption must be
 * disabled by the caller. Abort if a preemption is pending.
 * Returns true if the page is zeroed, false if not.
 */
ENTRY(sse2_idlezero_page)
	pushq	%rbp
	movq	%rsp,%rbp
	movl	$(PAGE_SIZE/64), %ecx
	xorq	%rax, %rax
	.align	16
1:
	testl	$RESCHED_KPREEMPT, CPUVAR(RESCHED)
	jnz	2f
	movnti	%rax, 0(%rdi)
	movnti	%rax, 8(%rdi)
	movnti	%rax, 16(%rdi)
	movnti	%rax, 24(%rdi)
	movnti	%rax, 32(%rdi)
	movnti	%rax, 40(%rdi)
	movnti	%rax, 48(%rdi)
	movnti	%rax, 56(%rdi)
	addq	$64, %rdi
	decl	%ecx
	jnz	1b
	sfence
	incl	%eax
	popq	%rbp
	ret
2:
	sfence
	popq	%rbp
	ret
END(sse2_idlezero_page)

/*
 * void pagezero(vaddr_t va)
 *
 * Zero a page without polluting the cache.
 */

ENTRY(pagezero)
	movq	$-PAGE_SIZE,%rdx
	subq	%rdx,%rdi
	xorq	%rax,%rax
1:
	movnti	%rax,(%rdi,%rdx)
	movnti	%rax,8(%rdi,%rdx)
	movnti	%rax,16(%rdi,%rdx)
	movnti	%rax,24(%rdi,%rdx)
	movnti	%rax,32(%rdi,%rdx)
	movnti	%rax,40(%rdi,%rdx)
	movnti	%rax,48(%rdi,%rdx)
	movnti	%rax,56(%rdi,%rdx)
	addq	$64,%rdx
	jne	1b
	sfence
	ret
END(pagezero)
