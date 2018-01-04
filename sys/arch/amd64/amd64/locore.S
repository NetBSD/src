/*	$NetBSD: locore.S,v 1.144 2018/01/04 13:36:30 maxv Exp $	*/

/*
 * Copyright-o-rama!
 */

/*
 * Copyright (c) 1998, 2000, 2007, 2008, 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Maxime Villard.
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

/* Override user-land alignment before including asm.h */
#define	ALIGN_DATA	.align	8
#define ALIGN_TEXT	.align 16,0x90
#define _ALIGN_TEXT	ALIGN_TEXT

#include <machine/asm.h>

#include "opt_copy_symtab.h"
#include "opt_ddb.h"
#include "opt_ddbparam.h"
#include "opt_modular.h"
#include "opt_realmem.h"

#include "opt_compat_netbsd.h"
#include "opt_compat_netbsd32.h"
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

/* Get definitions for IOM_BEGIN, IOM_END, and IOM_SIZE */
#include <dev/isa/isareg.h>

#define	_RELOC(x)	((x) - KERNBASE)
#define	RELOC(x)	_RELOC(_C_LABEL(x))

/* 32bit version of PG_NX */
#define PG_NX32	0x80000000

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
#define PROC0_STK_OFF	(PROC0_PML4_OFF + 1 * PAGE_SIZE)
#define PROC0_PTP3_OFF	(PROC0_STK_OFF + UPAGES * PAGE_SIZE)
#define PROC0_PTP2_OFF	(PROC0_PTP3_OFF + NKL4_KIMG_ENTRIES * PAGE_SIZE)
#define PROC0_PTP1_OFF	(PROC0_PTP2_OFF + TABLE_L3_ENTRIES * PAGE_SIZE)
#define TABLESIZE \
  ((NKL4_KIMG_ENTRIES + TABLE_L3_ENTRIES + TABLE_L2_ENTRIES + 1 + UPAGES) \
    * PAGE_SIZE)

/* Amount of VA used to map the kernel, the syms and the preloaded modules */
#define BOOTMAP_VA_SIZE \
	(NKL2_KIMG_ENTRIES * (1 << L2_SHIFT) - TABLESIZE - IOM_SIZE)

/*
 * fillkpt - Fill in a kernel page table
 *	eax = pte (page frame | control | status)
 *	ebx = page table address
 *	ecx = number of pages to map
 *
 * Each entry is 8 (PDE_SIZE) bytes long: we must set the 4 upper bytes to 0.
 */
#define fillkpt	\
	cmpl	$0,%ecx			;	/* zero-sized? */	\
	je 	2f			; \
1:	movl	$0,(PDE_SIZE-4)(%ebx)	;	/* upper 32 bits: 0 */	\
	movl	%eax,(%ebx)		;	/* store phys addr */	\
	addl	$PDE_SIZE,%ebx		;	/* next PTE/PDE */	\
	addl	$PAGE_SIZE,%eax		;	/* next phys page */	\
	loop	1b			; \
2:					;

/*
 * fillkpt_nox - Same as fillkpt, but sets the NX/XD bit.
 */
#define fillkpt_nox \
	cmpl	$0,%ecx			;	/* zero-sized? */	\
	je 	2f			; \
	pushl	%ebp			; \
	movl	RELOC(nox_flag),%ebp	; \
1:	movl	%ebp,(PDE_SIZE-4)(%ebx)	;	/* upper 32 bits: NX */ \
	movl	%eax,(%ebx)		;	/* store phys addr */	\
	addl	$PDE_SIZE,%ebx		;	/* next PTE/PDE */	\
	addl	$PAGE_SIZE,%eax		;	/* next phys page */	\
	loop	1b			; \
	popl	%ebp			; \
2:					;

/*
 * fillkpt_blank - Fill in a kernel page table with blank entries
 *	ebx = page table address
 *	ecx = number of pages to map
 */
#define fillkpt_blank	\
	cmpl	$0,%ecx			;	/* zero-sized? */	\
	je 	2f			; \
1:	movl	$0,(PDE_SIZE-4)(%ebx)	;	/* upper 32 bits: 0 */	\
	movl	$0,(%ebx)		;	/* lower 32 bits: 0 */	\
	addl	$PDE_SIZE,%ebx		;	/* next PTE/PDE */	\
	loop	1b			; \
2:					;

/*
 * killkpt - Destroy a kernel page table (long mode)
 *	rbx = page table address
 *	rcx = number of pages to destroy
 */
#define killkpt \
1:	movq	$0,(%rbx)	; \
	addq	$PDE_SIZE,%rbx	; \
	loop	1b		;


#ifdef XEN
#define __ASSEMBLY__
#include <xen/xen-public/elfnote.h>
#include <xen/xen-public/xen.h>
#define ELFNOTE(name, type, desctype, descdata...) \
.pushsection .note.name			;	\
  .align 4				;	\
  .long 2f - 1f		/* namesz */	;	\
  .long 4f - 3f		/* descsz */	;	\
  .long type				;	\
1:.asciz #name				;	\
2:.align 4				;	\
3:desctype descdata			;	\
4:.align 4				;	\
.popsection

/*
 * Xen guest identifier and loader selection
 */
.section __xen_guest
	ELFNOTE(Xen, XEN_ELFNOTE_GUEST_OS,       .asciz, "NetBSD")
	ELFNOTE(Xen, XEN_ELFNOTE_GUEST_VERSION,  .asciz, "4.99")
	ELFNOTE(Xen, XEN_ELFNOTE_XEN_VERSION,    .asciz, "xen-3.0")
	ELFNOTE(Xen, XEN_ELFNOTE_VIRT_BASE,      .quad,  KERNBASE)
	ELFNOTE(Xen, XEN_ELFNOTE_PADDR_OFFSET,   .quad,  KERNBASE)
	ELFNOTE(Xen, XEN_ELFNOTE_ENTRY,          .quad,  start)
	ELFNOTE(Xen, XEN_ELFNOTE_HYPERCALL_PAGE, .quad,  hypercall_page)
	ELFNOTE(Xen, XEN_ELFNOTE_HV_START_LOW,   .quad,  HYPERVISOR_VIRT_START)
	ELFNOTE(Xen, XEN_ELFNOTE_FEATURES,       .asciz, "")
	ELFNOTE(Xen, XEN_ELFNOTE_PAE_MODE,       .asciz, "yes")
	ELFNOTE(Xen, XEN_ELFNOTE_L1_MFN_VALID,   .long,  PG_V, PG_V)\
	ELFNOTE(Xen, XEN_ELFNOTE_LOADER,         .asciz, "generic")     
	ELFNOTE(Xen, XEN_ELFNOTE_SUSPEND_CANCEL, .long,  0)
#if NKSYMS > 0 || defined(DDB) || defined(MODULAR)
	ELFNOTE(Xen, XEN_ELFNOTE_BSD_SYMTAB,     .asciz, "yes")
#endif
#endif	/* XEN */

/*
 * Initialization
 */
	.data

	.globl	_C_LABEL(tablesize)
	.globl	_C_LABEL(nox_flag)
	.globl	_C_LABEL(cputype)
	.globl	_C_LABEL(cpuid_level)
	.globl	_C_LABEL(esym)
	.globl	_C_LABEL(eblob)
	.globl	_C_LABEL(atdevbase)
	.globl	_C_LABEL(PDPpaddr)
	.globl	_C_LABEL(boothowto)
	.globl	_C_LABEL(bootinfo)
	.globl	_C_LABEL(biosbasemem)
	.globl	_C_LABEL(biosextmem)
	.globl	_C_LABEL(lwp0uarea)
	.globl	do_sysret
	.globl	do_mov_es
	.globl	do_mov_ds
	.globl	do_mov_fs
	.globl	do_mov_gs
	.globl	do_iret

	.type	_C_LABEL(tablesize), @object
_C_LABEL(tablesize):	.long	TABLESIZE
END(tablesize)
	.type	_C_LABEL(nox_flag), @object
LABEL(nox_flag)		.long	0	/* 32bit NOX flag, set if supported */
END(nox_flag)
	.type	_C_LABEL(cputype), @object
LABEL(cputype)		.long	0	/* are we 80486, Pentium, or.. */
END(cputype)
	.type	_C_LABEL(cpuid_level), @object
LABEL(cpuid_level)	.long	-1	/* max. level accepted by cpuid instr */
END(cpuid_level)
	.type	_C_LABEL(esym), @object
LABEL(esym)		.quad	0	/* ptr to end of syms */
END(esym)
	.type	_C_LABEL(eblob), @object
LABEL(eblob)		.quad	0	/* ptr to end of modules */
END(eblob)
	.type	_C_LABEL(atdevbase), @object
LABEL(atdevbase)	.quad	0	/* location of start of iomem in virt */
END(atdevbase)
	.type	_C_LABEL(PDPpaddr), @object
LABEL(PDPpaddr)		.quad	0	/* paddr of PTD, for libkvm */
END(PDPpaddr)
	.type	_C_LABEL(biosbasemem), @object
#ifndef REALBASEMEM
LABEL(biosbasemem)	.long	0	/* base memory reported by BIOS */
#else
LABEL(biosbasemem)	.long	REALBASEMEM
#endif
END(biosbasemem)
	.type	_C_LABEL(biosextmem), @object
#ifndef REALEXTMEM
LABEL(biosextmem)	.long	0	/* extended memory reported by BIOS */
#else
LABEL(biosextmem)	.long	REALEXTMEM
#endif
END(biosextmem)
	.type	_C_LABEL(lwp0uarea), @object
LABEL(lwp0uarea)	.quad	0
END(lwp0uarea)

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

	/* Space for the temporary stack */
	.size	tmpstk, tmpstk - .
	.space	512
tmpstk:

/*
 * Some hackage to deal with 64bit symbols in 32 bit mode.
 * This may not be needed if things are cleaned up a little.
 */

	.text
	.globl	_C_LABEL(kernel_text)
	.set	_C_LABEL(kernel_text),KERNTEXTOFF

ENTRY(start)
#ifndef XEN
	.code32

	/* Warm boot */
	movw	$0x1234,0x472

	/*
	 * Load parameters from the stack (32 bits):
	 *     boothowto, [bootdev], bootinfo, esym, biosextmem, biosbasemem
	 * We are not interested in 'bootdev'.
	 */

	/* Load 'boothowto' */
	movl	4(%esp),%eax
	movl	%eax,RELOC(boothowto)

	/* Load 'bootinfo' */
	movl	12(%esp),%eax
	testl	%eax,%eax		/* bootinfo = NULL? */
	jz	.Lbootinfo_finished

	movl	(%eax),%ebx		/* bootinfo::bi_nentries */
	movl	$RELOC(bootinfo),%ebp
	movl	%ebp,%edx
	addl	$BOOTINFO_MAXSIZE,%ebp
	movl	%ebx,(%edx)
	addl	$4,%edx

.Lbootinfo_entryloop:
	testl	%ebx,%ebx		/* no remaining entries? */
	jz	.Lbootinfo_finished

	addl	$4,%eax
	movl	(%eax),%ecx		/* address of entry */
	pushl	%edi
	pushl	%esi
	pushl	%eax

	movl	(%ecx),%eax		/* btinfo_common::len (size of entry) */
	movl	%edx,%edi
	addl	%eax,%edx		/* update dest pointer */
	cmpl	%ebp,%edx		/* beyond bootinfo+BOOTINFO_MAXSIZE? */
	jg	.Lbootinfo_overflow

	movl	%ecx,%esi
	movl	%eax,%ecx

	/*
	 * If any modules were loaded, record where they end. 'eblob' is used
	 * later to compute the initial bootstrap tables.
	 */
	cmpl	$BTINFO_MODULELIST,4(%esi) /* btinfo_common::type */
	jne	.Lbootinfo_copy

	/* Skip the modules if we won't have enough VA to map them */
	movl	12(%esi),%eax		/* btinfo_modulelist::endpa */
	addl	$PGOFSET,%eax		/* roundup to a page */
	andl	$~PGOFSET,%eax
	cmpl	$BOOTMAP_VA_SIZE,%eax
	jg	.Lbootinfo_skip
	movl	%eax,RELOC(eblob)
	addl	$KERNBASE_LO,RELOC(eblob)
	adcl	$KERNBASE_HI,RELOC(eblob)+4

.Lbootinfo_copy:
	rep
	movsb				/* copy esi -> edi */
	jmp	.Lbootinfo_next

.Lbootinfo_skip:
	subl	%ecx,%edx		/* revert dest pointer */

.Lbootinfo_next:
	popl	%eax
	popl	%esi
	popl	%edi
	subl	$1,%ebx			/* decrement the # of entries */
	jmp	.Lbootinfo_entryloop

.Lbootinfo_overflow:
	/*
	 * Cleanup for overflow case. Pop the registers, and correct the number
	 * of entries.
	 */
	popl	%eax
	popl	%esi
	popl	%edi
	movl	$RELOC(bootinfo),%ebp
	movl	%ebp,%edx
	subl	%ebx,(%edx)		/* correct the number of entries */
.Lbootinfo_finished:

	/* Load 'esym' */
	movl	16(%esp),%eax
	testl	%eax,%eax		/* esym = NULL? */
	jz	1f

	addl	$KERNBASE_LO,%eax

1:
	movl	$RELOC(esym),%ebp
	movl	%eax,(%ebp)
	movl	$KERNBASE_HI,4(%ebp)

	/* Load 'biosextmem' */
	movl	$RELOC(biosextmem),%ebp
	movl	(%ebp),%eax
	testl	%eax,%eax		/* already set? */
	jnz	.Lbiosextmem_finished

	movl	20(%esp),%eax
	movl	%eax,(%ebp)

.Lbiosextmem_finished:
	/* Load 'biosbasemem' */
	movl	$RELOC(biosbasemem),%ebp
	movl	(%ebp),%eax
	testl	%eax,%eax		/* already set? */
	jnz	.Lbiosbasemem_finished

	movl	24(%esp),%eax
	movl	%eax,(%ebp)

.Lbiosbasemem_finished:
	/*
	 * Done with the parameters!
	 */

	/* First, reset the PSL. */
	pushl	$PSL_MBO
	popfl

	xorl	%eax,%eax
	cpuid
	movl	%eax,RELOC(cpuid_level)

	/*
	 * Finished with old stack; load new %esp now instead of later so we
	 * can trace this code without having to worry about the trace trap
	 * clobbering the memory test or the zeroing of the bss+bootstrap page
	 * tables.
	 *
	 * The boot program should check:
	 *	text+data <= &stack_variable - more_space_for_stack
	 *	text+data+bss+pad+space_for_page_tables <= end_of_memory
	 *
	 * XXX: the gdt is in the carcass of the boot program so clearing
	 * the rest of memory is still not possible.
	 */
	movl	$RELOC(tmpstk),%esp

	/*
	 * Retrieve the NX/XD flag. We use the 32bit version of PG_NX.
	 */
	movl	$0x80000001,%eax
	cpuid
	andl	$CPUID_NOX,%edx
	jz	.Lno_NOX
	movl	$PG_NX32,RELOC(nox_flag)
.Lno_NOX:

/*
 * There are four levels of pages in amd64: PML4 -> PDP -> PD -> PT. They will
 * be referred to as: L4 -> L3 -> L2 -> L1.
 *
 * Virtual address space of the kernel:
 * +------+--------+------+-----+--------+---------------------+----------
 * | TEXT | RODATA | DATA | BSS | [SYMS] | [PRELOADED MODULES] | L4 ->
 * +------+--------+------+-----+--------+---------------------+----------
 *                             (1)      (2)                   (3)
 *
 * --------------+-----+-----+----+-------------+
 * -> PROC0 STK -> L3 -> L2 -> L1 | ISA I/O MEM |
 * --------------+-----+-----+----+-------------+
 *                               (4)
 *
 * PROC0 STK is obviously not linked as a page level. It just happens to be
 * caught between L4 and L3.
 *
 * (PROC0 STK + L4 + L3 + L2 + L1) is later referred to as BOOTSTRAP TABLES.
 *
 * ISA I/O MEM has no physical page allocated here, just virtual addresses.
 *
 * Important note: the kernel segments are properly 4k-aligned
 * (see kern.ldscript), so there's no need to enforce alignment.
 */

	/* Find end of kernel image; brings us on (1). */
	movl	$RELOC(__kernel_end),%edi

#if (NKSYMS || defined(DDB) || defined(MODULAR)) && !defined(makeoptions_COPY_SYMTAB)
	/* Save the symbols (if loaded); brinds us on (2). */
	movl	RELOC(esym),%eax
	testl	%eax,%eax
	jz	1f
	subl	$KERNBASE_LO,%eax	/* XXX */
	movl	%eax,%edi
1:
#endif
	/* Skip over any modules/blobs; brings us on (3). */
	movl	RELOC(eblob),%eax
	testl	%eax,%eax
	jz	1f
	subl	$KERNBASE_LO,%eax	/* XXX */
	movl	%eax,%edi
1:

	/* We are on (3). Align up for BOOTSTRAP TABLES. */
	movl	%edi,%esi
	addl	$PGOFSET,%esi
	andl	$~PGOFSET,%esi

	/* We are on the BOOTSTRAP TABLES. Save L4's physical address. */
	movl	$RELOC(PDPpaddr),%ebp
	movl	%esi,(%ebp)
	movl	$0,4(%ebp)

	/* Now, zero out the BOOTSTRAP TABLES (before filling them in). */
	movl	%esi,%edi
	xorl	%eax,%eax
	cld
	movl	$TABLESIZE,%ecx
	shrl	$2,%ecx
	rep
	stosl				/* copy eax -> edi */

/*
 * Build the page tables and levels. We go from L1 to L4, and link the levels
 * together. Note: RELOC computes &addr - KERNBASE in 32 bits; the value can't
 * be > 4G, or we can't deal with it anyway, since we are in 32bit mode.
 */
	/*
	 * Build L1.
	 */
	leal	(PROC0_PTP1_OFF)(%esi),%ebx

	/* Skip the area below the kernel text. */
	movl	$(KERNTEXTOFF_LO - KERNBASE_LO),%ecx
	shrl	$PGSHIFT,%ecx
	fillkpt_blank

	/* Map the kernel text RX. */
	movl	$(KERNTEXTOFF_LO - KERNBASE_LO),%eax	/* start of TEXT */
	movl	$RELOC(__rodata_start),%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KR),%eax
	fillkpt

	/* Map the kernel rodata R. */
	movl	$RELOC(__rodata_start),%eax
	movl	$RELOC(__data_start),%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KR),%eax
	fillkpt_nox

	/* Map the kernel data+bss RW. */
	movl	$RELOC(__data_start),%eax
	movl	$RELOC(__kernel_end),%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KW),%eax
	fillkpt_nox

	/* Map [SYMS]+[PRELOADED MODULES] RW. */
	movl	$RELOC(__kernel_end),%eax
	movl	%esi,%ecx		/* start of BOOTSTRAP TABLES */
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KW),%eax
	fillkpt_nox

	/* Map the BOOTSTRAP TABLES RW. */
	movl	%esi,%eax		/* start of BOOTSTRAP TABLES */
	movl	$TABLESIZE,%ecx		/* length of BOOTSTRAP TABLES */
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KW),%eax
	fillkpt_nox

	/* We are on (4). Map ISA I/O MEM RW. */
	movl	$IOM_BEGIN,%eax
	movl	$IOM_SIZE,%ecx	/* size of ISA I/O MEM */
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KW/*|PG_N*/),%eax
	fillkpt_nox

	/*
	 * Build L2. Linked to L1.
	 */
	leal	(PROC0_PTP2_OFF)(%esi),%ebx
	leal	(PROC0_PTP1_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW),%eax
	movl	$(NKL2_KIMG_ENTRIES+1),%ecx
	fillkpt

#if L2_SLOT_KERNBASE > 0
	/* If needed, set up level 2 entries for actual kernel mapping */
	leal	(PROC0_PTP2_OFF + L2_SLOT_KERNBASE * PDE_SIZE)(%esi),%ebx
	leal	(PROC0_PTP1_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW),%eax
	movl	$(NKL2_KIMG_ENTRIES+1),%ecx
	fillkpt
#endif

	/*
	 * Build L3. Linked to L2.
	 */
	leal	(PROC0_PTP3_OFF)(%esi),%ebx
	leal	(PROC0_PTP2_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW),%eax
	movl	$NKL3_KIMG_ENTRIES,%ecx
	fillkpt

#if L3_SLOT_KERNBASE > 0
	/* If needed, set up level 3 entries for actual kernel mapping */
	leal	(PROC0_PTP3_OFF + L3_SLOT_KERNBASE * PDE_SIZE)(%esi),%ebx
	leal	(PROC0_PTP2_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW),%eax
	movl	$NKL3_KIMG_ENTRIES,%ecx
	fillkpt
#endif

	/*
	 * Build L4 for identity mapping. Linked to L3.
	 */
	leal	(PROC0_PML4_OFF)(%esi),%ebx
	leal	(PROC0_PTP3_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW),%eax
	movl	$NKL4_KIMG_ENTRIES,%ecx
	fillkpt

	/* Set up L4 entries for actual kernel mapping */
	leal	(PROC0_PML4_OFF + L4_SLOT_KERNBASE * PDE_SIZE)(%esi),%ebx
	leal	(PROC0_PTP3_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW),%eax
	movl	$NKL4_KIMG_ENTRIES,%ecx
	fillkpt

	/* Install recursive top level PDE (one entry) */
	leal	(PROC0_PML4_OFF + PDIR_SLOT_PTE * PDE_SIZE)(%esi),%ebx
	leal	(PROC0_PML4_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW),%eax
	movl	$1,%ecx
	fillkpt_nox

	/*
	 * Startup checklist:
	 * 1. Enable PAE (and SSE while here).
	 */
	movl	%cr4,%eax
	orl	$(CR4_PAE|CR4_OSFXSR|CR4_OSXMMEXCPT),%eax
	movl	%eax,%cr4

	/*
	 * 2. Set Long Mode Enable in EFER. Also enable the syscall extensions,
	 *    and NOX if available.
	 */
	movl	$MSR_EFER,%ecx
	rdmsr
	xorl	%eax,%eax	/* XXX */
	orl	$(EFER_LME|EFER_SCE),%eax
	movl	RELOC(nox_flag),%ebx
	cmpl	$0,%ebx
	je 	.Lskip_NOX
	orl	$(EFER_NXE),%eax
.Lskip_NOX:
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
	 * 5. Not quite done yet, we're now in a compatibility segment, in
	 *    legacy mode. We must jump to a long mode segment. Need to set up
	 *    a temporary GDT with a long mode segment in it to do that.
	 */
	movl	$RELOC(gdt64_lo),%eax
	lgdt	(%eax)
	movl	$RELOC(farjmp64),%eax
	ljmp	*(%eax)

	.code64
longmode:
	/*
	 * 6. Finally, we're in long mode. However, we're still in the identity
	 *    mapped area (could not jump out of that earlier because it would
	 *    have been a > 32bit jump). We can do that now, so here we go.
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
	 * We have arrived. There's no need anymore for the identity mapping in
	 * low memory, remove it.
	 */
	movq	$KERNBASE,%r8

#if L2_SLOT_KERNBASE > 0
	movq	$(NKL2_KIMG_ENTRIES+1),%rcx
	leaq	(PROC0_PTP2_OFF)(%rsi),%rbx	/* old, phys address */
	addq	%r8,%rbx			/* new, virt address */
	killkpt
#endif

#if L3_SLOT_KERNBASE > 0
	movq	$NKL3_KIMG_ENTRIES,%rcx
	leaq	(PROC0_PTP3_OFF)(%rsi),%rbx	/* old, phys address */
	addq	%r8,%rbx			/* new, virt address */
	killkpt
#endif

	movq	$NKL4_KIMG_ENTRIES,%rcx
	leaq	(PROC0_PML4_OFF)(%rsi),%rbx	/* old, phys address of PML4 */
	addq	%r8,%rbx			/* new, virt address of PML4 */
	killkpt

	/* Relocate atdevbase. */
	movq	$(TABLESIZE+KERNBASE),%rdx
	addq	%rsi,%rdx
	movq	%rdx,_C_LABEL(atdevbase)(%rip)

	/* Set up bootstrap stack. */
	leaq	(PROC0_STK_OFF)(%rsi),%rax
	addq	%r8,%rax
	movq	%rax,_C_LABEL(lwp0uarea)(%rip)
	leaq	(USPACE-FRAMESIZE)(%rax),%rsp
	xorq	%rbp,%rbp			/* mark end of frames */

	xorw	%ax,%ax
	movw	%ax,%gs
	movw	%ax,%fs

	/* The first physical page available. */
	leaq	(TABLESIZE)(%rsi),%rdi

#else	/* XEN */
	/* First, reset the PSL. */
	pushq	$2
	popfq

	cld

	/*
	 * Xen info:
	 * - %rsi -> start_info struct
	 * - %rsp -> stack, *theoretically* the last used page by Xen bootstrap
	 */
	movq	%rsi,%rbx

	/* Clear BSS. */
	xorq	%rax,%rax
	movq	$_C_LABEL(__bss_start),%rdi
	movq	$_C_LABEL(_end),%rcx
	subq	%rdi,%rcx
	rep
	stosb

	/* Copy start_info to a safe place. */
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
	 * - guaranteed 512kB padding
	 *
	 * As we want to rebuild our page tables and place our stack
	 * in proc0 struct, all data starting from after console can be
	 * discarded after we've done a little setup.
	 */

	/*
	 * We want our own page tables, and will rebuild them. We will reclaim
	 * the Xen space later, INCLUDING the stack. So we need to switch to a
	 * temporary one now.
	 */
	movq	$tmpstk,%rax
	subq	$8,%rax
	movq	%rax,%rsp

	xorl	%eax,%eax
	cpuid
	movl	%eax,_C_LABEL(cpuid_level)

	movq	$cpu_info_primary,%rdi
	movq	%rdi,CPU_INFO_SELF(%rdi) /* ci->ci_self = ci */
	movq	$1,%rsi
	call	cpu_init_msrs	/* cpu_init_msrs(ci, true); */

	call	xen_locore

	/*
	 * The first VA available is returned by xen_locore in %rax. We
	 * use it as the UAREA, and set up the stack here.
	 */
	movq	%rax,%rsi
	movq	%rsi,_C_LABEL(lwp0uarea)(%rip)
	leaq	(USPACE-FRAMESIZE)(%rsi),%rsp
	xorq	%rbp,%rbp

	/* Clear segment registers. */
	xorw	%ax,%ax
	movw	%ax,%gs
	movw	%ax,%fs

	/* Set first_avail after the DUMMY PAGE (see xen_locore). */
	movq	%rsi,%rdi
	addq	$(USPACE+PAGE_SIZE),%rdi
	subq	$KERNBASE,%rdi	/* init_x86_64 wants a physical address */
#endif	/* XEN */

	pushq	%rdi
	call	_C_LABEL(init_bootspace)
	popq	%rdi
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
	/* Build a fake switch frame. */
	pushq	%rbx
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15

	/* Save a context. */
	movq	$dumppcb, %rax
	movq	%rsp, PCB_RSP(%rax)
	movq	%rbp, PCB_RBP(%rax)

	call	_C_LABEL(dodumpsys)

	addq	$(5*8), %rsp	/* sizeof(switchframe) - sizeof(%rip) */
	ret
END(dumpsys)

/*
 * struct lwp *cpu_switchto(struct lwp *oldlwp, struct lwp *newlwp,
 *     bool returning)
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

	movq	%rdi,%r13	/* oldlwp */
	movq	%rsi,%r12	/* newlwp */

	testq	%r13,%r13	/* oldlwp = NULL ? */
	jz	.Lskip_save

	/* Save old context. */
	movq	L_PCB(%r13),%rax
	movq	%rsp,PCB_RSP(%rax)
	movq	%rbp,PCB_RBP(%rax)
.Lskip_save:

	/* Switch to newlwp's stack. */
	movq	L_PCB(%r12),%r14
	movq	PCB_RSP(%r14),%rsp
	movq	PCB_RBP(%r14),%rbp

	/*
	 * Set curlwp.  This must be globally visible in order to permit
	 * non-interlocked mutex release.
	 */
	movq	%r12,%rcx
	xchgq	%rcx,CPUVAR(CURLWP)

	/* Skip the rest if returning to a pinned LWP. */
	testb	%dl,%dl		/* returning = true ? */
	jnz	.Lswitch_return

	/* Switch ring0 stack */
#ifndef XEN
	movq	PCB_RSP0(%r14),%rax
	movq	CPUVAR(TSS),%rdi
	movq	%rax,TSS_RSP0(%rdi)
#else
	movq	%r14,%rdi
	callq	_C_LABEL(x86_64_switch_context);
#endif

	/* Don't bother with the rest if switching to a system process. */
	testl	$LW_SYSTEM,L_FLAG(%r12)
	jnz	.Lswitch_return

	/* Is this process using RAS (restartable atomic sequences)? */
	movq	L_PROC(%r12),%rdi
	cmpq	$0,P_RASLIST(%rdi)
	je	.Lno_RAS

	/* Handle restartable atomic sequences (RAS). */
	movq	L_MD_REGS(%r12),%rbx
	movq	TF_RIP(%rbx),%rsi
	call	_C_LABEL(ras_lookup)
	cmpq	$-1,%rax
	je	.Lno_RAS
	movq	%rax,TF_RIP(%rbx)
.Lno_RAS:

	/*
	 * Restore cr0 including FPU state (may have CR0_TS set).  Note that
	 * IPL_SCHED prevents from FPU interrupt altering the LWP's saved cr0.
	 */
#ifndef XEN
	movl	$IPL_HIGH,CPUVAR(ILEVEL)
	movl	PCB_CR0(%r14),%ecx	/* has CR0_TS clear */
	movq	%cr0,%rdx

	/*
	 * If our floating point registers are on a different CPU,
	 * set CR0_TS so we'll trap rather than reuse bogus state.
	 */
	cmpq	CPUVAR(FPCURLWP),%r12
	je	.Lskip_TS
	orq	$CR0_TS,%rcx
.Lskip_TS:

	/* Reloading CR0 is very expensive - avoid if possible. */
	cmpq	%rdx,%rcx
	je	.Lskip_CR0
	movq	%rcx,%cr0
.Lskip_CR0:

	/* The 32bit LWPs are handled differently. */
	testl	$PCB_COMPAT32,PCB_FLAGS(%r14)
	jnz	.Llwp_32bit

.Llwp_64bit:
	/* Set default 64bit values in %ds, %es, %fs and %gs. */
	movq	$GSEL(GUDATA_SEL, SEL_UPL),%rax
	movw	%ax,%ds
	movw	%ax,%es
	xorq	%rax,%rax
	movw	%ax,%fs
	CLI(cx)
	SWAPGS
	movw	%ax,%gs
	SWAPGS
	STI(cx)

	/* Zero out GDT descriptors. */
	movq	CPUVAR(GDT),%rcx
	movq	%rax,(GUFS_SEL*8)(%rcx)
	movq	%rax,(GUGS_SEL*8)(%rcx)

	/* Reload 64-bit %fs/%gs MSRs. */
	movl	$MSR_FSBASE,%ecx
	movl	PCB_FS(%r14),%eax
	movl	4+PCB_FS(%r14),%edx
	wrmsr
	movl	$MSR_KERNELGSBASE,%ecx
	movl	PCB_GS(%r14),%eax
	movl	4+PCB_GS(%r14),%edx
	wrmsr

	jmp	.Lswitch_return

.Llwp_32bit:
	/* Reload %fs/%gs GDT descriptors. */
	movq	CPUVAR(GDT),%rcx
	movq	PCB_FS(%r14),%rax
	movq	%rax,(GUFS_SEL*8)(%rcx)
	movq	PCB_GS(%r14),%rax
	movq	%rax,(GUGS_SEL*8)(%rcx)

	/* Set default 32bit values in %ds, %es, %fs and %gs. */
	movq	L_MD_REGS(%r12),%rbx
	movq	$GSEL(GUDATA32_SEL, SEL_UPL),%rax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	CLI(ax)
	SWAPGS
	movw	%ax,%gs
	SWAPGS
	STI(ax)
#else
	movq	%r12,%rdi
	callq	_C_LABEL(x86_64_tls_switch)
#endif

.Lswitch_return:
	/* Return to the new LWP, returning 'oldlwp' in %rax. */
	movq	%r13,%rax
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%rbx
	ret
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
 */
IDTVEC(syscall)
#ifndef XEN
	/*
	 * The user %rip is in %rcx and the user %rflags in %r11. The kernel %cs
	 * and %ss are loaded, but nothing else is.
	 *
	 * The 'swapgs' instruction gives us access to cpu-specific memory where
	 * we can save a user register and then read the LWP's kernel stack
	 * pointer.
	 *
	 * This code doesn't seem to set %ds, this may not matter since it is
	 * ignored in 64bit mode, OTOH the syscall instruction sets %ss and that
	 * is ignored as well.
	 */
	swapgs
	movq	%r15,CPUVAR(SCRATCH)
	movq	CPUVAR(CURLWP),%r15
	movq	L_PCB(%r15),%r15
	movq	PCB_RSP0(%r15),%r15	/* LWP's kernel stack pointer */

	/* Make stack look like an 'int nn' frame */
#define SP(x)	(x)-(TF_SS+8)(%r15)
	movq	$(LSEL(LUDATA_SEL, SEL_UPL)),SP(TF_SS)	/* user %ss */
	movq	%rsp,SP(TF_RSP)				/* user %rsp */
	movq	%r11,SP(TF_RFLAGS)			/* user %rflags */
	movq	$(LSEL(LUCODE_SEL, SEL_UPL)),SP(TF_CS)	/* user %cs */
	movq	%rcx,SP(TF_RIP)				/* user %rip */

	leaq	SP(0),%rsp		/* %rsp now valid after frame */
	movq	CPUVAR(SCRATCH),%r15
#undef SP

	movq	$2,TF_ERR(%rsp)		/* syscall instruction size */
	movq	$T_ASTFLT,TF_TRAPNO(%rsp)
#else
	/* Xen already switched to kernel stack */
	addq	$0x10,%rsp	/* gap to match cs:rip */
	pushq	$2		/* error code */
	pushq	$T_ASTFLT
	subq	$TF_REGSIZE,%rsp
	cld
#endif
	INTR_SAVE_GPRS
	movw	$GSEL(GUDATA_SEL, SEL_UPL),TF_DS(%rsp)
	movw	$GSEL(GUDATA_SEL, SEL_UPL),TF_ES(%rsp)
	movw	$0,TF_FS(%rsp)
	movw	$0,TF_GS(%rsp)
	STI(si)

.Ldo_syscall:
	movq	CPUVAR(CURLWP),%r14
	incq	CPUVAR(NSYSCALL)	/* count it atomically */
	movq	%rsp,L_MD_REGS(%r14)	/* save pointer to frame */
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
	movl	L_MD_ASTPENDING(%r14),%eax
	orl	CPUVAR(WANT_PMAPLOAD),%eax
	jnz	9f

#ifdef DIAGNOSTIC
	cmpl	$IPL_NONE,CPUVAR(ILEVEL)
	jne	.Lspl_error
#endif

	/*
	 * Decide if we need to take a slow path. That's the case when we
	 * want to reload %cs and %ss on a 64bit LWP (MDL_IRET set), or when
	 * we're returning to a 32bit LWP (MDL_COMPAT32 set).
	 *
	 * In either case, we jump into intrfastexit and return to userland
	 * with the iret instruction.
	 */
	testl	$(MDL_IRET|MDL_COMPAT32),L_MD_FLAGS(%r14)
	jnz	intrfastexit

	INTR_RESTORE_GPRS
	SWAPGS
#ifndef XEN
	movq	TF_RIP(%rsp),%rcx	/* %rip for sysret */
	movq	TF_RFLAGS(%rsp),%r11	/* %flags for sysret */
	movq	TF_RSP(%rsp),%rsp
do_sysret:
	sysretq
#else
	addq	$TF_RIP,%rsp
	pushq	$256	/* VGCF_IN_SYSCALL */
	jmp	HYPERVISOR_iret
#endif

#ifdef DIAGNOSTIC
	/* Report SPL error */
.Lspl_error:
	movabsq	$4f,%rdi
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
	cmpl	$0,CPUVAR(WANT_PMAPLOAD)
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
	pushq	$2		/* size of instruction for restart */
	pushq	$T_ASTFLT	/* trap # for doing ASTs */
	INTRENTRY
	STI(si)
	jmp	.Ldo_syscall
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

ENTRY(intrfastexit)
	INTR_RESTORE_GPRS
	testw	$SEL_UPL,TF_CS(%rsp)	/* interrupted %cs */
	jz	.Lkexit
	cmpw	$LSEL(LUCODE_SEL, SEL_UPL),TF_CS(%rsp)
	je	.Luexit64
	cmpw	$GSEL(GUCODE_SEL, SEL_UPL),TF_CS(%rsp)
	je	.Luexit64
#ifdef XEN
	cmpw	$FLAT_RING3_CS64,TF_CS(%rsp)
	je	.Luexit64
#endif

.Luexit32:
	NOT_XEN(cli;)
do_mov_es:
	movw	TF_ES(%rsp),%es
do_mov_ds:
	movw	TF_DS(%rsp),%ds
do_mov_fs:
	movw	TF_FS(%rsp),%fs
	SWAPGS
#ifndef XEN
do_mov_gs:
	movw	TF_GS(%rsp),%gs
#endif
	jmp	.Lkexit

.Luexit64:
	NOT_XEN(cli;)
	SWAPGS

.Lkexit:
	addq	$TF_REGSIZE+16,%rsp	/* + T_xxx and error code */
do_iret:
	iretq
END(intrfastexit)
