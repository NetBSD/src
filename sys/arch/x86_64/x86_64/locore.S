/*	$NetBSD: locore.S,v 1.7 2002/06/25 01:24:50 thorpej Exp $	*/

/*
 * Copyright-o-rama!
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
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#include "opt_ddb.h"
#include "opt_user_ldt.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"

#include "assym.h"

#include <sys/errno.h>
#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/pte.h>
#include <machine/pmap.h>
#include <machine/segments.h>
#include <machine/specialreg.h>
#include <machine/trap.h>
#include <machine/bootinfo.h>
#include <machine/frameasm.h>

/*
 * override user-land alignment before including asm.h
 */
#define	ALIGN_DATA	.align	4
#define	ALIGN_TEXT	.align	4,0x90	/* 4-byte boundaries, NOP-filled */
#define	SUPERALIGN_TEXT	.align	16,0x90	/* 16-byte boundaries better for 486 */
#define _ALIGN_TEXT	ALIGN_TEXT

#include <machine/asm.h>


/* XXX temporary kluge; these should not be here */
/* Get definitions for IOM_BEGIN, IOM_END, and IOM_SIZE */
#include <dev/isa/isareg.h>


/*
 * Initialization
 */
	.data

	.globl	_C_LABEL(cpu_id),_C_LABEL(cpu_vendor), _C_LABEL(cpu_brand_id)
	.globl	_C_LABEL(cpuid_level),_C_LABEL(cpu_feature)
	.globl	_C_LABEL(esym),_C_LABEL(boothowto)
	.globl	_C_LABEL(bootinfo),_C_LABEL(atdevbase)
	.globl	_C_LABEL(proc0paddr),_C_LABEL(curpcb),_C_LABEL(PTDpaddr)
	.globl	_C_LABEL(biosbasemem),_C_LABEL(biosextmem)
	.globl	_C_LABEL(gdtstore)
_C_LABEL(cpu):		.long	0	# are we 386, 386sx, or 486,
					#   or Pentium, or..
_C_LABEL(cpu_id):	.long	0	# saved from `cpuid' instruction
_C_LABEL(cpu_feature):	.long	0	# feature flags from 'cpuid'
					#   instruction
_C_LABEL(cpuid_level):	.long	-1	# max. level accepted by 'cpuid'
					#   instruction
_C_LABEL(cpu_vendor):	.space	16	# vendor string returned by `cpuid'
					#   instruction
_C_LABEL(cpu_brand_id):	.long	0	# brand ID from 'cpuid' instruction
_C_LABEL(esym):		.quad	0	# ptr to end of syms
_C_LABEL(atdevbase):	.quad	0	# location of start of iomem in virtual
_C_LABEL(proc0paddr):	.quad	0
_C_LABEL(PTDpaddr):	.quad	0	# paddr of PTD, for libkvm
#ifndef REALBASEMEM
_C_LABEL(biosbasemem):	.long	0	# base memory reported by BIOS
#else
_C_LABEL(biosbasemem):	.long	REALBASEMEM
#endif
#ifndef REALEXTMEM
_C_LABEL(biosextmem):	.long	0	# extended memory reported by BIOS
#else
_C_LABEL(biosextmem):	.long	REALEXTMEM
#endif

#define	_RELOC(x)	((x) - KERNBASE)
#define	RELOC(x)	_RELOC(_C_LABEL(x))

gdt64:
	.word	gdt64_end-gdt64_start
	.quad	_RELOC(gdt64_start)
.align 64

gdt64_start:
	.quad 0x0000000000000000	/* always empty */
	.quad 0x00af9a000000ffff	/* kernel CS */
	.quad 0x00cf92000000ffff	/* kernel DS */
gdt64_end:

farjmp64:
	.long	longmode-KERNBASE
	.word	GSEL(GCODE_SEL, SEL_KPL)
	
	.space 512
tmpstk:

	.globl _C_LABEL(cpu_private)
	.comm _C_LABEL(cpu_private),NBPG,NBPG

/*
 * Some hackage to deal with 64bit symbols in 32 bit mode.
 * This may not be needed it things are cleaned up a little.
 */


	.text
	.globl	_C_LABEL(kernel_text)
	.set	_C_LABEL(kernel_text),KERNTEXTOFF+VAR32_SIZE
	.globl	_C_LABEL(var32_frob)
_C_LABEL(var32_frob):
_reloc_boothowto:	.long	_RELOC(boothowto)
_reloc_bootinfo:	.long	_RELOC(bootinfo)
_reloc_biosextmem:	.long	_RELOC(biosextmem)
_reloc_biosbasemem:	.long	_RELOC(biosbasemem)
_reloc_cpuid_level:	.long	_RELOC(cpuid_level)
_reloc_cpu_vendor:	.long	_RELOC(cpu_vendor)
_reloc_cpu_id:		.long	_RELOC(cpu_id)
_reloc_cpu_feature:	.long	_RELOC(cpu_feature)
_reloc_cpu_brand_id:	.long	_RELOC(cpu_brand_id)
_reloc_tmpstk:		.long	_RELOC(tmpstk)
_reloc_end:		.long	_RELOC(end)
_reloc_etext:		.long	_RELOC(etext)
_reloc_esym:		.long	_RELOC(esym)
_reloc_PTDpaddr:	.long	_RELOC(PTDpaddr)
_reloc_gdt64:		.long	_RELOC(gdt64)
_reloc_farjmp64:	.long	_RELOC(farjmp64)

#define RELOC_boothowto		0
#define RELOC_bootinfo		4
#define RELOC_biosextmem	8
#define RELOC_biosbasemem	12
#define RELOC_cpuid_level	16
#define RELOC_cpu_vendor	20
#define RELOC_cpu_id		24
#define RELOC_cpu_feature	28
#define RELOC_cpu_brand_id	32
#define RELOC_tmpstk		36
#define RELOC_end		40
#define RELOC_etext		44
#define RELOC_esym		48
#define RELOC_PTDpaddr		52
#define RELOC_gdt64		56
#define RELOC_farjmp64		60

	.space VAR32_SIZE - (. - _C_LABEL(var32_frob))

#define GET_RELOC_ADDR(var) \
    movl $(KERNTEXTOFF-KERNBASE),%ebp ; movl RELOC_/**/var(%ebp),%ebp

.code32

	.globl	start
start:	movw	$0x1234,0x472			# warm boot

	/*
	 * Load parameters from stack
	 * (howto, [bootdev], bootinfo, esym, basemem, extmem).
	 */
	movl	4(%esp),%eax
	GET_RELOC_ADDR(boothowto)
	movl	%eax, (%ebp)
	movl	12(%esp),%eax

	testl	%eax, %eax
	jz	1f
	movl	(%eax), %ebx		/* number of entries */
	GET_RELOC_ADDR(bootinfo)
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
	GET_RELOC_ADDR(bootinfo)
	movl	%ebp, %edx
	subl	%ebx, (%edx)		/* correct number of entries */
1:

 	movl	16(%esp),%eax
	testl	%eax,%eax
	jz	1f
	addl	$KERNBASE_LO,%eax
1: 	GET_RELOC_ADDR(esym)
	movl	%eax,(%ebp)
	movl	$KERNBASE_HI,4(%ebp)

	GET_RELOC_ADDR(biosextmem)
	movl	(%ebp),%eax
	testl	%eax,%eax
	jnz	1f
	movl	20(%esp),%eax
	movl	%eax,(%ebp)
1:
	GET_RELOC_ADDR(biosbasemem)
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
	GET_RELOC_ADDR(cpuid_level)
	movl	%eax,(%ebp)
	GET_RELOC_ADDR(cpu_vendor)
	movl	%ebx,(%ebp)
	movl	%edx,4(%ebp)
	movl	%ecx,8(%ebp)
	movl	$0,  12(%ebp)

	movl	$1,%eax
	cpuid
	GET_RELOC_ADDR(cpu_id)
	movl	%eax,(%ebp)		# store cpu_id and features
	GET_RELOC_ADDR(cpu_feature)
	movl	%edx,(%ebp)

	/* Brand ID is bits 0-7 of %ebx */
	andl	$255,%ebx
	GET_RELOC_ADDR(cpu_brand_id)
	movl	%ebx,(%ebp)

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
	GET_RELOC_ADDR(tmpstk)
	movl	%ebp,%esp		# bootstrap stack end location

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
#define PROC0_STK_OFF	(PROC0_PML4_OFF + NBPG)
#define PROC0_PTP3_OFF	(PROC0_STK_OFF + UPAGES * NBPG)
#define PROC0_PTP2_OFF	(PROC0_PTP3_OFF + NKL4_KIMG_ENTRIES * NBPG)
#define PROC0_PTP1_OFF	(PROC0_PTP2_OFF + TABLE_L3_ENTRIES * NBPG)
#define TABLESIZE \
  ((NKL4_KIMG_ENTRIES + TABLE_L3_ENTRIES + TABLE_L2_ENTRIES + 1 + UPAGES) \
    * NBPG)

	/* Find end of kernel image. */
	GET_RELOC_ADDR(end)
	movl	%ebp,%edi
#if defined(DDB) && !defined(SYMTAB_SPACE)
	/* Save the symbols (if loaded). */
	GET_RELOC_ADDR(esym)
	movl	(%ebp),%eax
	testl	%eax,%eax
	jz	1f
	subl	$KERNBASE_LO,%eax	/* XXX */
	movl	%eax,%edi
1:
#endif

#define fillkpt	\
1:	movl	%eax,(%ebx)	; 	/* store phys addr */ \
	movl	$0,4(%ebx)	; 	/* upper 32 bits 0 */ \
	addl	$8,%ebx		; 	/* next pte/pde */ \
	addl	$NBPG,%eax	; 	/* next phys page */ \
	loop	1b		;  \

	/* Clear tables */
	GET_RELOC_ADDR(end)
	movl	%ebp,%esi
	addl	$PGOFSET,%esi
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
	 * Compute etext - KERNBASE. This can't be > 4G, or we can't deal
	 * with it anyway, since we can't load it in 32 bit mode. So use
	 * the bottom 32 bits.
	 */
	GET_RELOC_ADDR(etext)
	movl	%ebp,%edx		/* should be $RELOC */
	addl	$PGOFSET,%edx
	andl	$~PGOFSET,%edx

	/*
	 * Skip the first MB.
	 */
	movl	$(KERNTEXTOFF_LO - KERNBASE_LO),%eax
	movl	%eax,%ecx
	shrl	$(PGSHIFT-3),%ecx	/* ((n >> PGSHIFT) << 3) for # pdes */
	addl	%ecx,%ebx

	/* Map kernel text read-only */
	movl	%edx,%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl     $(PG_V|PG_KR),%eax
	fillkpt

	/* Map the data, BSS, and bootstrap tables read-write. */
	leal	(PG_V|PG_KW)(%edx),%eax
	movl	$TABLESIZE,%ecx
	addl	%esi,%ecx		/* %ecx = &end[TABLESIZE] */
	subl	%edx,%ecx		/* %ecx = %ecx - etext */
	shrl	$PGSHIFT,%ecx
	fillkpt

	/* Map ISA I/O mem (later atdevbase) */
	movl	$(IOM_BEGIN|PG_V|PG_KW/*|PG_N*/),%eax
	movl	$(IOM_SIZE>>PGSHIFT),%ecx
	fillkpt

	/* Set up level 2 pages */
	leal    (PROC0_PTP2_OFF)(%esi),%ebx
	leal	(PROC0_PTP1_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW), %eax
	movl	$(NKL2_KIMG_ENTRIES+1),%ecx
	fillkpt

#if L2_SLOT_KERNBASE > 0
	/* If needed, set up level 2 entries for actual kernel mapping */
	leal	(PROC0_PTP2_OFF+ L2_SLOT_KERNBASE*8)(%esi),%ebx
	leal    (PROC0_PTP1_OFF)(%esi),%eax
	orl     $(PG_V|PG_KW), %eax
	movl    $(NKL2_KIMG_ENTRIES+1),%ecx
	fillkpt
#endif

	/* Set up level 3 pages */
	leal    (PROC0_PTP3_OFF)(%esi),%ebx
	leal	(PROC0_PTP2_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW), %eax
	movl	$NKL3_KIMG_ENTRIES,%ecx
	fillkpt

#if L3_SLOT_KERNBASE > 0
	/* If needed, set up level 3 entries for actual kernel mapping */
	leal	(PROC0_PTP3_OFF+ L3_SLOT_KERNBASE*8)(%esi),%ebx
	leal    (PROC0_PTP2_OFF)(%esi),%eax
	orl     $(PG_V|PG_KW), %eax
	movl    $NKL3_KIMG_ENTRIES,%ecx
	fillkpt
#endif

	/* Set up top level entries for identity mapping */
	leal    (PROC0_PML4_OFF)(%esi),%ebx
	leal	(PROC0_PTP3_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW), %eax
	movl	$NKL4_KIMG_ENTRIES,%ecx
	fillkpt

	/* Set up top level entries for actual kernel mapping */
	leal    (PROC0_PML4_OFF + L4_SLOT_KERNBASE*8)(%esi),%ebx
	leal	(PROC0_PTP3_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW), %eax
	movl	$NKL4_KIMG_ENTRIES,%ecx
	fillkpt

	/* Install recursive top level PDE */
	leal    (PROC0_PML4_OFF + PDIR_SLOT_PTE*8)(%esi),%ebx
	leal    (PROC0_PML4_OFF)(%esi),%eax
	orl	$(PG_V|PG_KW),%eax
	movl	%eax,(%ebx)
	movl	$0, 4(%ebx)


	/* Save phys. addr of PTD, for libkvm. */
	GET_RELOC_ADDR(PTDpaddr)
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
	movl    $MSR_EFER,%ecx
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
	orl	$(CR0_PE|CR0_PG|CR0_NE|CR0_TS|CR0_MP),%eax
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

	GET_RELOC_ADDR(gdt64)
	movl	%ebp,%eax
	lgdt	(%eax)
	GET_RELOC_ADDR(farjmp64)
	movl	%ebp,%eax
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
	addq	%r8, %rbx			# new, virtual adress of PML4
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
	movq	%rax,_C_LABEL(proc0paddr)(%rip)
	leaq	(USPACE-FRAMESIZE)(%rax),%rsp
	movq	%rsi,PCB_CR3(%rax)	# pcb->pcb_cr3
	xorq	%rbp,%rbp               # mark end of frames

	xorw	%ax,%ax
	movw	%ax,%gs
	movw	%ax,%fs

	/* XXX merge these */
	movq	$VM_MIN_KERNEL_ADDRESS,%rdi
	call	_C_LABEL(init_x86_64)

	call 	_C_LABEL(main)

/*****************************************************************************/

/*
 * Signal trampoline; copied to top of user stack.
 */
NENTRY(sigcode)
	/*
	 * Handler has returned here as if we called it.  The sigcontext
	 * is now the first thing on the stack.
	 */
	movl	SC_FS(%rsp),%ecx	/* XXXfvdl these are wrong */
	movl	SC_GS(%rsp),%edx
	movl	%ecx,%fs
	movl	%edx,%gs
	movq	%rsp,%rdi
	pushq	%rdi			/* fake return address */
	movq	$SYS___sigreturn14,%rax
	syscall
	movq	$SYS_exit,%rax
	syscall
	.globl	_C_LABEL(esigcode)
_C_LABEL(esigcode):

/*
 * void lgdt(struct region_descriptor *rdp);
 * Change the global descriptor table.
 */
NENTRY(lgdt)
	/* Reload the descriptor table. */
	movq	%rdi,%rax
	lgdt	(%rax)
	/* Flush the prefetch q. */
	jmp	1f
	nop
1:	/* Reload "stale" selectors. */
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movl	%eax,%ds
	movl	%eax,%es
	movl	%eax,%ss
	/* Reload code selector by doing intersegment return. */
	popq	%rax
	pushq	$GSEL(GCODE_SEL, SEL_KPL)
	pushq	%rax
	lretq

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
	xorl	%eax,%eax
	incl	%eax
	ret

/*****************************************************************************/

/*
 * The following primitives manipulate the run queues.
 * _whichqs tells which of the 32 queues _qs
 * have processes in them.  Setrq puts processes into queues, Remrq
 * removes them from queues.  The running process is on no queue,
 * other processes are on a queue related to p->p_pri, divided by 4
 * actually to shrink the 0-127 range of priorities into the 32 available
 * queues.
 */
	.globl	_C_LABEL(sched_whichqs),_C_LABEL(sched_qs)
	.globl	_C_LABEL(uvmexp),_C_LABEL(panic)

/*
 * setrunqueue(struct proc *p);
 * Insert a process on the appropriate queue.  Should be called at splclock().
 */
NENTRY(setrunqueue)
#ifdef DIAGNOSTIC
	cmpq	$0,P_BACK(%rdi)	# should not be on q already
	jne	1f
	cmpq	$0,P_WCHAN(%rdi)
	jne	1f
	cmpb	$SRUN,P_STAT(%rdi)
	jne	1f
#endif /* DIAGNOSTIC */
	movzbl	P_PRIORITY(%rdi),%edx
	shrl	$2,%edx
	btsl	%edx,_C_LABEL(sched_whichqs)(%rip)	# set q full bit

	shlq	$4,%rdx
	leaq	_C_LABEL(sched_qs)(%rip),%rax	# sched_qs[0]
	addq	%rax,%rdx			# sched_qs[queue]

	movq	P_BACK(%rdx),%rcx
	movq	%rdx,P_FORW(%rdi)	# link process on tail of q
	movq	%rdi,P_BACK(%rdx)
	movq	%rdi,P_FORW(%rcx)
	movq	%rcx,P_BACK(%rdi)
	ret
#ifdef DIAGNOSTIC
1:	movabsq	$2f,%rdi
	call	_C_LABEL(panic)
	/* NOTREACHED */
2:	.asciz	"setrunqueue"
#endif /* DIAGNOSTIC */

/*
 * remrunqueue(struct proc *p);
 * Remove a process from its queue.  Should be called at splclock().
 */
NENTRY(remrunqueue)
	movzbl	P_PRIORITY(%rdi),%eax
#ifdef DIAGNOSTIC
	shrl	$2,%eax
	btl	%eax,_C_LABEL(sched_whichqs)(%rip)
	jnc	1f
#endif /* DIAGNOSTIC */
	movq	P_BACK(%rdi),%rdx	# unlink process
	movq	$0,P_BACK(%rdi)		# zap reverse link to indicate off list
	movq	P_FORW(%rdi),%rdi
	movq	%rdi,P_FORW(%rdx)
	movq	%rdx,P_BACK(%rdi)
	cmpq	%rdi,%rdx		# q still has something?
	jne	2f
#ifndef DIAGNOSTIC
	shrl	$2,%eax
#endif
	btrl	%eax,_C_LABEL(sched_whichqs)(%rip)	# no; clear bit
2:	ret
#ifdef DIAGNOSTIC
1:	movabsq	$3f,%rdi
	call	_C_LABEL(panic)
	/* NOTREACHED */
3:	.asciz	"remrunqueue"
#endif /* DIAGNOSTIC */

#if NAPM > 0
	.globl _C_LABEL(apm_cpu_idle),_C_LABEL(apm_cpu_busy)
#endif
/*
 * When no processes are on the runq, cpu_switch() branches to here to wait for
 * something to come ready.
 */
ENTRY(idle)
	/*
	 * When we get here, interrupts are off (via cli) and
	 * sched_lock is held.
	 */
	movl	_C_LABEL(sched_whichqs)(%rip),%ecx
	testl	%ecx,%ecx
	jnz	sw1
#if defined(LOCKDEBUG)
	call	_C_LABEL(sched_unlock_idle)
#endif
	sti

#if 0
	/* Try to zero some pages. */
	leaq	_C_LABEL(uvm)(%rip),%rax
	movl	UVM_PAGE_IDLE_ZERO(%rax),%ecx
	testl	%ecx,%ecx
	jz	1f
	call	_C_LABEL(uvm_pageidlezero)
1:
#endif
	hlt
	cli
#if defined(LOCKDEBUG)
	call	_C_LABEL(sched_lock_idle)
#endif
	jmp	_C_LABEL(idle)

#ifdef DIAGNOSTIC
NENTRY(switch_error1)
	movabsq	$1f,%rdi
	call	_C_LABEL(panic)
	/* NOTREACHED */
1:	.asciz	"cpu_switch 1"
NENTRY(switch_error2)
	movabsq	$1f,%rdi
	call	_C_LABEL(panic)
	/* NOTREACHED */
1:	.asciz	"cpu_switch 2"
NENTRY(switch_error3)
	movabsq	$1f,%rdi
	call	_C_LABEL(panic)
	/* NOTREACHED */
1:	.asciz	"cpu_switch 3"
#endif /* DIAGNOSTIC */

/*
 * void cpu_switch(struct proc *)
 * Find a runnable process and switch to it.  Wait if necessary.  If the new
 * process is the same as the old one, we short-circuit the context save and
 * restore.
 */
ENTRY(cpu_switch)
	pushq	%rbx
	pushq	%rbp
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	movl	_C_LABEL(cpl)(%rip),%r11d
	pushq	%r11

	movq	_C_LABEL(curproc)(%rip),%r13

	/*
	 * Clear curproc so that we don't accumulate system time while idle.
	 * This also insures that schedcpu() will move the old process to
	 * the correct queue if it happens to get called from the spllower()
	 * below and changes the priority.  (See corresponding comment in
	 * userret()).
	 */
	movq	$0,_C_LABEL(curproc)(%rip)


#if defined(LOCKDEBUG)
	/* Release the sched_lock before processing interrupts. */
	call	_C_LABEL(sched_unlock_idle)
#endif

	movl	$0,_C_LABEL(cpl)(%rip)	# spl0()
	call	_C_LABEL(Xspllower)	# process pending interrupts

switch_search:
	/*
	 * First phase: find new process.
	 *
	 * Registers:
	 *   %rax - queue head, scratch, then zero
	 *   %r8 - queue number
	 *   %ecx - cached value of whichqs
	 *   %rdx - next process in queue
	 *   %r13 - old process
	 *   %r12 - new process
	 */

	/* Lock the scheduler. */
	cli				# splhigh doesn't do a cli
#if defined(LOCKDEBUG)
	call	_C_LABEL(sched_lock_idle)
#endif

	/* Wait for new process. */
	movl	_C_LABEL(sched_whichqs)(%rip),%ecx

sw1:	bsfl	%ecx,%r8d		# find a full q
	jz	_C_LABEL(idle)		# if none, idle
	movq	%r8,%r9

	shlq	$4, %r9
	leaq	_C_LABEL(sched_qs)(%rip),%rax
	addq	%r9,%rax
	/* movq	(%rax),%rax */

	movq	P_FORW(%rax),%r12	# unlink from front of process q
#ifdef	DIAGNOSTIC
	cmpq	%r12,%rax		# linked to self (i.e. nothing queued)?
	je	_C_LABEL(switch_error1)	# not possible
#endif /* DIAGNOSTIC */
	movq	P_FORW(%r12),%rdx
	movq	%rdx,P_FORW(%rax)
	movq	%rax,P_BACK(%rdx)

	cmpq	%rdx,%rax		# q empty?
	jne	3f

	btrl	%r8d,%ecx		# yes, clear to indicate empty
	movl	%ecx,_C_LABEL(sched_whichqs)(%rip) # update q status

3:	/* We just did it. */
	xorq	%rax,%rax
	movl	%eax,_C_LABEL(want_resched)(%rip)

#ifdef	DIAGNOSTIC
	cmpq	%rax,P_WCHAN(%r12)	# Waiting for something?
	je	1f
	xchgw	%bx,%bx
#if 0
	jne	_C_LABEL(switch_error2)	# Yes; shouldn't be queued.
#endif
1:	cmpb	$SRUN,P_STAT(%r12)	# In run state?
	jne	_C_LABEL(switch_error3)	# No; shouldn't be queued.
#endif /* DIAGNOSTIC */

	/* Isolate process.  XXX Is this necessary? */
	movq	%rax,P_BACK(%r12)

#if defined(LOCKDEBUG)
	/*
	 * Unlock the sched_lock, but leave interrupts off, for now.
	 */
	call	_C_LABEL(sched_unlock_idle)
#endif

#if defined(MULTIPROCESSOR)
	/*
	 * p->p_cpu = curcpu()
	 * XXXSMP
	 */
#endif

	/* Record new process. */
	movb	$SONPROC,P_STAT(%r12)	# p->p_stat = SONPROC
	movq	%r12,_C_LABEL(curproc)(%rip)

	/* It's okay to take interrupts here. */
	sti

	/* Skip context switch if same process. */
	cmpq	%r12,%r13
	je	switch_return

	/* If old process exited, don't bother. */
	testq	%r13,%r13
	jz	switch_exited

	/*
	 * Second phase: save old context.
	 *
	 * Registers:
	 *   %rax, %rcx - scratch
	 *   %r13 - old process, then old pcb
	 *   %r12 - new process
	 */

	movq	P_ADDR(%r13),%r13

	/* Save stack pointers. */
	movq	%rsp,PCB_RSP(%r13)
	movq	%rbp,PCB_RBP(%r13)

switch_exited:
	/*
	 * Third phase: restore saved context.
	 *
	 * Registers:
	 *   %rax, %rcx, %rdx - scratch
	 *   %r13 - new pcb
	 *   %r12 - new process
	 */

	/* No interrupts while loading new state. */
	cli
	movq	P_ADDR(%r12),%r13

	/* Restore stack pointers. */
	movq	PCB_RSP(%r13),%rsp
	movq	PCB_RBP(%r13),%rbp

#if 0
	/* Don't bother with the rest if switching to a system process. */
	testl	$P_SYSTEM,P_FLAG(%r12)
	jnz	switch_restored
#endif

	/*
	 * Activate the address space.  We're curproc, so %cr3 will
	 * be reloaded, but we're not yet curpcb, so the LDT won't
	 * be reloaded, although the PCB copy of the selector will
	 * be refreshed from the pmap.
	 */
	movq	%r12,%rdi
	call	_C_LABEL(pmap_activate)

	/* Load TSS info. */
	movq	_C_LABEL(gdtstore)(%rip),%rax
	movl	P_MD_TSS_SEL(%r12),%edx

	/* Switch TSS. Reset "task busy" flag before */
	andl	$~0x0200,4(%rax,%rdx, 1)
	ltr	%dx

switch_restored:
	/* Restore cr0 (including FPU state). */
	movl	PCB_CR0(%r13),%ecx
	movq	%rcx,%cr0

	/* Record new pcb. */
	movq	%r13,_C_LABEL(curpcb)(%rip)

	/* Interrupts are okay again. */
	sti

switch_return:
	/*
	 * Restore old cpl from stack.  Note that this is always an increase,
	 * due to the spl0() on entry.
	 */
	popq	%r11
	movl	%r11d,_C_LABEL(cpl)(%rip)

	movq	%r12,%rax		# return (p);
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%rbp
	popq	%rbx
	ret

/*
 * switch_exit(struct proc *p);
 * Switch to proc0's saved context and deallocate the address space and kernel
 * stack for p.  Then jump into cpu_switch(), as if we were in proc0 all along.
 */
	.globl	_C_LABEL(proc0),_C_LABEL(uvmspace_free),_C_LABEL(kernel_map)
	.globl	_C_LABEL(uvm_km_free),_C_LABEL(tss_free)
ENTRY(switch_exit)
	leaq	_C_LABEL(proc0)(%rip),%rbx

	/* In case we fault... */
	movq	$0,_C_LABEL(curproc)(%rip)

	/* Restore proc0's context. */
	cli
	movq	P_ADDR(%rbx),%rsi

	/* Restore stack pointers. */
	movq	PCB_RSP(%rsi),%rsp
	movq	PCB_RBP(%rsi),%rbp

	/* Load TSS info. */
	movq	_C_LABEL(gdtstore)(%rip),%rax
	movl	P_MD_TSS_SEL(%rbx),%edx

	/* Switch address space. */
	movq	PCB_CR3(%rsi),%rcx
	movq	%rcx,%cr3

	/* Switch TSS. */
	andl	$~0x0200,4-SEL_KPL(%rax,%rdx,1)
	ltr	%dx

	/* We're always in the kernel, so we don't need the LDT. */

#if 0
	movl	$GSEL(GUDATA_SEL,SEL_KPL),%ecx
	movl	%ecx,%fs
	movl	%ecx,%gs
#endif

	/* Restore cr0 (including FPU state). */
	movl	PCB_CR0(%rsi),%ecx
	movq	%rcx,%cr0

	/* Record new pcb. */
	movq	%rsi,_C_LABEL(curpcb)(%rip)

	/* Interrupts are okay again. */
	sti

	/*
	 * Schedule the dead process's vmspace and stack to be freed.
	 */
	call	_C_LABEL(exit2)

	/* Jump into cpu_switch() with the right state. */
	movq	%rbx,%r13
	movq	$0,_C_LABEL(curproc)(%rip)
	jmp	switch_search

/*
 * savectx(struct pcb *pcb);
 * Update pcb, saving current processor state.
 */
ENTRY(savectx)
	/* Save stack pointers. */
	movq	%rsp,PCB_RSP(%rdi)
	movq	%rbp,PCB_RBP(%rdi)

	ret
