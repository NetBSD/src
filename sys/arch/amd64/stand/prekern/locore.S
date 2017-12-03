/*	$NetBSD: locore.S,v 1.6.2.2 2017/12/03 11:35:48 jdolecek Exp $	*/

/*
 * Copyright (c) 1998, 2000, 2007, 2008, 2016, 2017 The NetBSD Foundation, Inc.
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

#define _LOCORE

/* Override user-land alignment before including asm.h */
#define	ALIGN_DATA	.align	8
#define ALIGN_TEXT	.align 16,0x90
#define _ALIGN_TEXT	ALIGN_TEXT

#include <machine/asm.h>
#include <machine/param.h>
#include <machine/pte.h>
#include <machine/psl.h>
#include <machine/segments.h>
#include <machine/specialreg.h>
#include <machine/trap.h>

#define _KERNEL
#include <machine/bootinfo.h>
#undef _KERNEL

#include "pdir.h"
#include "redef.h"

/* 32bit version of PG_NX */
#define PG_NX32	0x80000000

#define TABLE_L2_ENTRIES (NKL2_KIMG_ENTRIES + 1)
#define TABLE_L3_ENTRIES NKL3_KIMG_ENTRIES

#define PROC0_PML4_OFF	0
#define PROC0_STK_OFF	(PROC0_PML4_OFF + 1 * PAGE_SIZE)
#define PROC0_PTP3_OFF	(PROC0_STK_OFF + UPAGES * PAGE_SIZE)
#define PROC0_PTP2_OFF	(PROC0_PTP3_OFF + NKL4_KIMG_ENTRIES * PAGE_SIZE)
#define PROC0_PTP1_OFF	(PROC0_PTP2_OFF + TABLE_L3_ENTRIES * PAGE_SIZE)
#define TABLESIZE \
  ((NKL4_KIMG_ENTRIES + TABLE_L3_ENTRIES + TABLE_L2_ENTRIES + 1 + UPAGES) \
    * PAGE_SIZE)

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
	movl	_C_LABEL(nox_flag),%ebp	; \
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
 * Initialization
 */
	.data

	.globl	_C_LABEL(tablesize)
	.globl	_C_LABEL(nox_flag)
	.globl	_C_LABEL(cpuid_level)
	.globl	_C_LABEL(esym)
	.globl	_C_LABEL(eblob)
	.globl	_C_LABEL(atdevbase)
	.globl	_C_LABEL(PDPpaddr)
	.globl	_C_LABEL(boothowto)
	.globl	_C_LABEL(bootinfo)
	.globl	_C_LABEL(biosbasemem)
	.globl	_C_LABEL(biosextmem)
	.globl	_C_LABEL(stkpa)
	.globl	_C_LABEL(stkva)
	.globl	_C_LABEL(kernpa_start)
	.globl	_C_LABEL(kernpa_end)

	.type	_C_LABEL(tablesize), @object
_C_LABEL(tablesize):	.long	TABLESIZE
END(tablesize)
	.type	_C_LABEL(nox_flag), @object
LABEL(nox_flag)		.long	0	/* 32bit NOX flag, set if supported */
END(nox_flag)
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
LABEL(biosbasemem)	.long	0	/* base memory reported by BIOS */
END(biosbasemem)
	.type	_C_LABEL(biosextmem), @object
LABEL(biosextmem)	.long	0	/* extended memory reported by BIOS */
END(biosextmem)
	.type	_C_LABEL(stkpa), @object
LABEL(stkpa)		.quad	0
END(stkpa)
	.type	_C_LABEL(stkva), @object
LABEL(stkva)		.quad	0
END(stkva)
	.type	_C_LABEL(kernpa_start), @object
LABEL(kernpa_start)	.quad	0
END(kernpa_start)
	.type	_C_LABEL(kernpa_end), @object
LABEL(kernpa_end)	.quad	0
END(kernpa_end)

	.globl	gdt64_lo
	.globl	gdt64_start

#define GDT64_LIMIT gdt64_end-gdt64_start-1
/* Temporary gdt64, with base address in low memory */
	.type	_C_LABEL(gdt64_lo), @object
LABEL(gdt64_lo)
	.word	GDT64_LIMIT
	.quad	gdt64_start
END(gdt64_lo)
.align 64
#undef GDT64_LIMIT

	.type	_C_LABEL(gdt64_start), @object
LABEL(gdt64_start)
	.quad 0x0000000000000000	/* always empty */
	.quad 0x00af9a000000ffff	/* kernel CS */
	.quad 0x00cf92000000ffff	/* kernel DS */
	.quad 0x0000000000000000	/* kernel TSS [1/2] */
	.quad 0x0000000000000000	/* kernel TSS [2/2] */
END(gdt64_start)
gdt64_end:

	.type	_C_LABEL(farjmp64), @object
_C_LABEL(farjmp64):
	.long	longmode
	.word	GSEL(GCODE_SEL, SEL_KPL)
END(farjmp64)

	/* Space for the temporary stack */
	.size	tmpstk, tmpstk - .
	.space	512
tmpstk:

	.text

ENTRY(start)
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
	movl	%eax,_C_LABEL(boothowto)

	/* Load 'bootinfo' */
	movl	12(%esp),%eax
	testl	%eax,%eax		/* bootinfo = NULL? */
	jz	bootinfo_finished

	movl	(%eax),%ebx		/* number of entries */
	movl	$_C_LABEL(bootinfo),%ebp
	movl	%ebp,%edx
	addl	$BOOTINFO_MAXSIZE,%ebp
	movl	%ebx,(%edx)
	addl	$4,%edx

bootinfo_entryloop:
	testl	%ebx,%ebx		/* no remaining entries? */
	jz	bootinfo_finished

	addl	$4,%eax
	movl	(%eax),%ecx		/* address of entry */
	pushl	%edi
	pushl	%esi
	pushl	%eax

	movl	(%ecx),%eax		/* btinfo_common::len (size of entry) */
	movl	%edx,%edi
	addl	(%ecx),%edx		/* update dest pointer */
	cmpl	%ebp,%edx		/* beyond bootinfo+BOOTINFO_MAXSIZE? */
	jg	bootinfo_overflow

	movl	%ecx,%esi
	movl	%eax,%ecx

	/* If any modules were loaded, record where they end. */
	cmpl	$BTINFO_MODULELIST,4(%esi) /* btinfo_common::type */
	jne	0f
	pushl	12(%esi)		/* btinfo_modulelist::endpa */
	popl	_C_LABEL(eblob)
0:

	/* Record the information about the kernel. */
	cmpl	$BTINFO_PREKERN,4(%esi) /* btinfo_common::type */
	jne	0f
	pushl	8(%esi)		/* btinfo_prekern::kernpa_start */
	popl	_C_LABEL(kernpa_start)
	pushl	12(%esi)	/* btinfo_prekern::kernpa_end */
	popl	_C_LABEL(kernpa_end)
0:

	rep
	movsb				/* copy esi -> edi */
	popl	%eax
	popl	%esi
	popl	%edi
	subl	$1,%ebx			/* decrement the # of entries */
	jmp	bootinfo_entryloop

bootinfo_overflow:
	/*
	 * Cleanup for overflow case. Pop the registers, and correct the number
	 * of entries.
	 */
	popl	%eax
	popl	%esi
	popl	%edi
	movl	$_C_LABEL(bootinfo),%ebp
	movl	%ebp,%edx
	subl	%ebx,(%edx)		/* correct the number of entries */

bootinfo_finished:
	/* Load 'esym' */
	movl	16(%esp),%eax
	movl	$_C_LABEL(esym),%ebp
	movl	%eax,(%ebp)

	/* Load 'biosextmem' */
	movl	20(%esp),%eax
	movl	$_C_LABEL(biosextmem),%ebp
	movl	%eax,(%ebp)

	/* Load 'biosbasemem' */
	movl	24(%esp),%eax
	movl	$_C_LABEL(biosbasemem),%ebp
	movl	%eax,(%ebp)

	/*
	 * Done with the parameters!
	 */

	/* First, reset the PSL. */
	pushl	$PSL_MBO
	popfl

	/* Switch to new stack now. */
	movl	$_C_LABEL(tmpstk),%esp

	xorl	%eax,%eax
	cpuid
	movl	%eax,_C_LABEL(cpuid_level)

	/*
	 * Retrieve the NX/XD flag. We use the 32bit version of PG_NX.
	 */
	movl	$0x80000001,%eax
	cpuid
	andl	$CPUID_NOX,%edx
	jz	no_NOX
	movl	$PG_NX32,_C_LABEL(nox_flag)
no_NOX:

/*
 * There are four levels of pages in amd64: PML4 -> PDP -> PD -> PT. They will
 * be referred to as: L4 -> L3 -> L2 -> L1.
 *
 * Physical address space:
 * +---------------+----------+--------------+--------+---------------------+-
 * | PREKERN IMAGE |**UNUSED**| KERNEL IMAGE | [SYMS] | [PRELOADED MODULES] |
 * +---------------+----------+--------------+--------+---------------------+-
 *                (1)                       (2)      (3)                   (4)
 * ------------------+
 *  BOOTSTRAP TABLES |
 * ------------------+
 *                  (5)
 *
 * Virtual address space of the prekern:
 * +---------------+----------+------------------+-------------+
 * | PREKERN IMAGE |**UNUSED**| BOOTSTRAP TABLES | ISA I/O MEM |
 * +---------------+----------+------------------+-------------+
 *
 * PROC0 STK is obviously not linked as a page level. It just happens to be
 * caught between L4 and L3.
 *
 * (PROC0 STK + L4 + L3 + L2 + L1) is later referred to as BOOTSTRAP TABLES.
 *
 * Important note: the kernel segments are properly 4k-aligned
 * (see kern.ldscript), so there's no need to enforce alignment.
 */

	/* Find end of the prekern image; brings us on (1). */
	movl	$_C_LABEL(__prekern_end),%edi

	/* Find end of the kernel image; brings us on (2). */
	movl	_C_LABEL(kernpa_end),%eax
	testl	%eax,%eax
	jz	1f
	movl	%eax,%edi
1:

	/* Find end of the kernel symbols; brings us on (3). */
	movl	_C_LABEL(esym),%eax
	testl	%eax,%eax
	jz	1f
	movl	%eax,%edi
1:

	/* Find end of the kernel preloaded modules; brings us on (4). */
	movl	_C_LABEL(eblob),%eax
	testl	%eax,%eax
	jz	1f
	movl	%eax,%edi
1:

	/* We are on (3). Align up for BOOTSTRAP TABLES. */
	movl	%edi,%esi
	addl	$PGOFSET,%esi
	andl	$~PGOFSET,%esi

	/* We are on the BOOTSTRAP TABLES. Save L4's physical address. */
	movl	$_C_LABEL(PDPpaddr),%ebp
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
 * together.
 */
	/*
	 * Build L1.
	 */
	leal	(PROC0_PTP1_OFF)(%esi),%ebx

	/* Skip the area below the prekern text. */
	movl	$(PREKERNTEXTOFF - PREKERNBASE),%ecx
	shrl	$PGSHIFT,%ecx
	fillkpt_blank

	/* Map the prekern text RX. */
	movl	$(PREKERNTEXTOFF - PREKERNBASE),%eax	/* start of TEXT */
	movl	$_C_LABEL(__rodata_start),%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KR),%eax
	fillkpt

	/* Map the prekern rodata R. */
	movl	$_C_LABEL(__rodata_start),%eax
	movl	$_C_LABEL(__data_start),%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KR),%eax
	fillkpt_nox

	/* Map the prekern data+bss RW. */
	movl	$_C_LABEL(__data_start),%eax
	movl	$_C_LABEL(__prekern_end),%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KW),%eax
	fillkpt_nox

	/* Map a RO view of the kernel. */
	movl	$_C_LABEL(__prekern_end),%eax
	movl	%esi,%ecx		/* start of BOOTSTRAP TABLES */
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KR),%eax
	fillkpt_nox

	/* Map the BOOTSTRAP TABLES RW. */
	movl	%esi,%eax		/* start of BOOTSTRAP TABLES */
	movl	$TABLESIZE,%ecx		/* length of BOOTSTRAP TABLES */
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KW),%eax
	fillkpt_nox

	/* Map the ISA I/O MEM RW. */
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

	/*
	 * Build L3. Linked to L2.
	 */
	leal	(PROC0_PTP3_OFF)(%esi),%ebx
	leal	(PROC0_PTP2_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW),%eax
	movl	$NKL3_KIMG_ENTRIES,%ecx
	fillkpt

	/*
	 * Build L4. Linked to L3.
	 */
	leal	(PROC0_PML4_OFF)(%esi),%ebx
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
	xorl	%eax,%eax
	orl	$(EFER_LME|EFER_SCE),%eax
	movl	_C_LABEL(nox_flag),%ebx
	cmpl	$0,%ebx
	je 	skip_NOX
	orl	$(EFER_NXE),%eax
skip_NOX:
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
	 *    a GDT with a long mode segment in it to do that.
	 */
	movl	$_C_LABEL(gdt64_lo),%eax
	lgdt	(%eax)
	movl	$_C_LABEL(farjmp64),%eax
	ljmp	*(%eax)

	.code64
longmode:

	/*
	 * We have arrived. Everything is identity-mapped.
	 */

	/* Store atdevbase. */
	movq	$TABLESIZE,%rdx
	addq	%rsi,%rdx
	movq	%rdx,_C_LABEL(atdevbase)(%rip)

	/* Set up bootstrap stack. */
	leaq	(PROC0_STK_OFF)(%rsi),%rax
	movq	%rax,_C_LABEL(stkpa)(%rip)
	leaq	(USPACE-FRAMESIZE)(%rax),%rsp
	xorq	%rbp,%rbp			/* mark end of frames */

	xorw	%ax,%ax
	movw	%ax,%gs
	movw	%ax,%fs

	/* The first physical page available. */
	leaq	(TABLESIZE)(%rsi),%rdi

	/*
	 * Continue execution in C.
	 */
	call	_C_LABEL(init_prekern)

	ret
END(start)

/* -------------------------------------------------------------------------- */

ENTRY(cpuid)
	movq	%rbx,%r8
	movq	%rdi,%rax
	movq	%rsi,%rcx
	movq	%rdx,%rsi
	cpuid
	movl	%eax,0(%rsi)
	movl	%ebx,4(%rsi)
	movl	%ecx,8(%rsi)
	movl	%edx,12(%rsi)
	movq	%r8,%rbx
	ret
END(cpuid)

ENTRY(lidt)
	lidt	(%rdi)
	ret
END(lidt)

ENTRY(rdtsc)
	xorq	%rax,%rax
	rdtsc
	shlq	$32,%rdx
	orq	%rdx,%rax
	ret
END(rdtsc)

ENTRY(rdseed)
	rdseed	%rax
	jc	.Lrdseed_success
	movq	$(-1),%rax
	ret
.Lrdseed_success:
	movq	%rax,(%rdi)
	xorq	%rax,%rax
	ret
END(rdseed)

ENTRY(rdrand)
	rdrand	%rax
	jc	.Lrdrand_success
	movq	$(-1),%rax
	ret
.Lrdrand_success:
	movq	%rax,(%rdi)
	xorq	%rax,%rax
	ret
END(rdrand)

ENTRY(jump_kernel)
	movq	_C_LABEL(stkva),%rsp
	xorq	%rbp,%rbp
	callq	exec_kernel
END(jump_kernel)
