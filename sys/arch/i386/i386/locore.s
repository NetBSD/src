/*	$NetBSD: locore.s,v 1.243 2001/07/08 19:33:44 perry Exp $	*/

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

#include "opt_cputype.h"
#include "opt_ddb.h"
#include "opt_ipkdb.h"
#include "opt_vm86.h"
#include "opt_user_ldt.h"
#include "opt_dummy_nops.h"
#include "opt_compat_oldboot.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_realmem.h"

#include "npx.h"
#include "assym.h"
#include "apm.h"

#include <sys/errno.h>
#include <sys/syscall.h>

#include <machine/cputypes.h>
#include <machine/param.h>
#include <machine/pte.h>
#include <machine/segments.h>
#include <machine/specialreg.h>
#include <machine/trap.h>
#include <machine/bootinfo.h>

/* LINTSTUB: include <sys/systm.h> */
/* LINTSTUB: include <machine/cpu.h> */

/*
 * override user-land alignment before including asm.h
 */
#ifdef __ELF__
#define	ALIGN_DATA	.align	4
#define	ALIGN_TEXT	.align	4,0x90	/* 4-byte boundaries, NOP-filled */
#define	SUPERALIGN_TEXT	.align	16,0x90	/* 16-byte boundaries better for 486 */
#else
#define	ALIGN_DATA	.align	2
#define	ALIGN_TEXT	.align	2,0x90	/* 4-byte boundaries, NOP-filled */
#define	SUPERALIGN_TEXT	.align	4,0x90	/* 16-byte boundaries better for 486 */
#endif
#define _ALIGN_TEXT	ALIGN_TEXT
#include <machine/asm.h>


/* XXX temporary kluge; these should not be here */
/* Get definitions for IOM_BEGIN, IOM_END, and IOM_SIZE */
#include <dev/isa/isareg.h>


/* NB: NOP now preserves registers so NOPs can be inserted anywhere */
/* XXX: NOP and FASTER_NOP are misleadingly named */
#ifdef DUMMY_NOPS	/* this will break some older machines */
#define	FASTER_NOP
#define	NOP
#else
#define	FASTER_NOP	pushl %eax ; inb $0x84,%al ; popl %eax
#define	NOP	pushl %eax ; inb $0x84,%al ; inb $0x84,%al ; popl %eax
#endif

/* Disallow old names for REALBASEMEM */
#ifdef BIOSBASEMEM
#error BIOSBASEMEM option deprecated; use REALBASEMEM only if memory size reported by latest boot block is incorrect
#endif

/* Disallow old names for REALEXTMEM */
#ifdef EXTMEM_SIZE
#error EXTMEM_SIZE option deprecated; use REALEXTMEM only if memory size reported by latest boot block is incorrect
#endif
#ifdef BIOSEXTMEM
#error BIOSEXTMEM option deprecated; use REALEXTMEM only if memory size reported by latest boot block is incorrect
#endif

/*
 * These are used on interrupt or trap entry or exit.
 */
#define	INTRENTRY \
	pushl	%eax		; \
	pushl	%ecx		; \
	pushl	%edx		; \
	pushl	%ebx		; \
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax	; \
	pushl	%ebp		; \
	pushl	%esi		; \
	pushl	%edi		; \
	pushl	%ds		; \
	pushl	%es		; \
	movl	%ax,%ds		; \
	movl	%ax,%es		; \
	pushl	%fs		; \
	pushl	%gs		; \
	movl	%ax,%fs		; \
	movl	%ax,%gs		; \

#define	INTRFASTEXIT \
	popl	%gs		; \
	popl	%fs		; \
	popl	%es		; \
	popl	%ds		; \
	popl	%edi		; \
	popl	%esi		; \
	popl	%ebp		; \
	popl	%ebx		; \
	popl	%edx		; \
	popl	%ecx		; \
	popl	%eax		; \
	addl	$8,%esp		; \
	iret


/*
 * PTmap is recursive pagemap at top of virtual address space.
 * Within PTmap, the page directory can be found (third indirection).
 *
 * XXX 4 == sizeof pde
 */
	.set	_C_LABEL(PTmap),(PDSLOT_PTE << PDSHIFT)
	.set	_C_LABEL(PTD),(_C_LABEL(PTmap) + PDSLOT_PTE * NBPG)
	.set	_C_LABEL(PTDpde),(_C_LABEL(PTD) + PDSLOT_PTE * 4)

/*
 * APTmap, APTD is the alternate recursive pagemap.
 * It's used when modifying another process's page tables.
 *
 * XXX 4 == sizeof pde
 */
	.set	_C_LABEL(APTmap),(PDSLOT_APTE << PDSHIFT)
	.set	_C_LABEL(APTD),(_C_LABEL(APTmap) + PDSLOT_APTE * NBPG)
	.set	_C_LABEL(APTDpde),(_C_LABEL(PTD) + PDSLOT_APTE * 4)


/*
 * Initialization
 */
	.data

	.globl	_C_LABEL(cpu),_C_LABEL(cpu_id),_C_LABEL(cpu_vendor)
	.globl	_C_LABEL(cpuid_level),_C_LABEL(cpu_feature)
	.globl	_C_LABEL(cpu_brand_id)
	.globl	_C_LABEL(esym),_C_LABEL(boothowto)
	.globl	_C_LABEL(bootinfo),_C_LABEL(atdevbase)
#ifdef COMPAT_OLDBOOT
	.globl	_C_LABEL(bootdev)
#endif
	.globl	_C_LABEL(proc0paddr),_C_LABEL(curpcb),_C_LABEL(PTDpaddr)
	.globl	_C_LABEL(biosbasemem),_C_LABEL(biosextmem)
	.globl	_C_LABEL(gdt)
#ifdef I586_CPU
	.globl	_C_LABEL(idt)
#endif
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
_C_LABEL(esym):		.long	0	# ptr to end of syms
_C_LABEL(atdevbase):	.long	0	# location of start of iomem in virtual
_C_LABEL(proc0paddr):	.long	0
_C_LABEL(PTDpaddr):	.long	0	# paddr of PTD, for libkvm
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
	
	.space 512
tmpstk:


#define	_RELOC(x)	((x) - KERNBASE)
#define	RELOC(x)	_RELOC(_C_LABEL(x))

	.text
	.globl	_C_LABEL(kernel_text)
	.set	_C_LABEL(kernel_text),KERNTEXTOFF

	.globl	start
start:	movw	$0x1234,0x472			# warm boot

	/*
	 * Load parameters from stack
	 * (howto, [bootdev], bootinfo, esym, basemem, extmem).
	 */
	movl	4(%esp),%eax
	movl	%eax,RELOC(boothowto)
#ifdef COMPAT_OLDBOOT
	movl	8(%esp),%eax
	movl	%eax,RELOC(bootdev)
#endif
	movl	12(%esp),%eax

	testl	%eax, %eax
	jz	1f
	movl	(%eax), %ebx		/* number of entries */
	movl	$RELOC(bootinfo), %edx
	movl	%ebx, (%edx)
	addl	$4, %edx
2:
	testl	%ebx, %ebx
	jz	1f
	addl	$4, %eax
	movl	(%eax), %ecx		/* address of entry */
	pushl	%eax
	pushl	(%ecx)			/* len */
	pushl	%ecx
	pushl	%edx
	addl	(%ecx), %edx		/* update dest pointer */
	cmpl	$_RELOC(_C_LABEL(bootinfo) + BOOTINFO_MAXSIZE), %edx
	jg	2f
	call	_C_LABEL(memcpy)
	addl	$12, %esp
	popl	%eax
	subl	$1, %ebx
	jmp	2b
2:	/* cleanup for overflow case */
	addl	$16, %esp
	movl	$RELOC(bootinfo), %edx
	subl	%ebx, (%edx)		/* correct number of entries */
1:

 	movl	16(%esp),%eax
	testl	%eax,%eax
	jz	1f
	addl	$KERNBASE,%eax
1: 	movl	%eax,RELOC(esym)

	movl	RELOC(biosextmem),%eax
	testl	%eax,%eax
	jnz	1f
	movl	20(%esp),%eax
	movl	%eax,RELOC(biosextmem)
1:
	movl	RELOC(biosbasemem),%eax
	testl	%eax,%eax
	jnz	1f
	movl	24(%esp),%eax
	movl	%eax,RELOC(biosbasemem)
1:

	/* First, reset the PSL. */
	pushl	$PSL_MBO
	popfl

	/* Clear segment registers; always null in proc0. */
	xorl	%eax,%eax
	movl	%ax,%fs
	movl	%ax,%gs
	/* Find out our CPU type. */

try386:	/* Try to toggle alignment check flag; does not exist on 386. */
	pushfl
	popl	%eax
	movl	%eax,%ecx
	orl	$PSL_AC,%eax
	pushl	%eax
	popfl
	pushfl
	popl	%eax
	xorl	%ecx,%eax
	andl	$PSL_AC,%eax
	pushl	%ecx
	popfl

	testl	%eax,%eax
	jnz	try486

	/*
	 * Try the test of a NexGen CPU -- ZF will not change on a DIV
	 * instruction on a NexGen, it will on an i386.  Documented in
	 * Nx586 Processor Recognition Application Note, NexGen, Inc.
	 */
	movl	$0x5555,%eax
	xorl	%edx,%edx
	movl	$2,%ecx
	divl	%ecx
	jnz	is386

isnx586:
	/*
	 * Don't try cpuid, as Nx586s reportedly don't support the
	 * PSL_ID bit.
	 */
	movl	$CPU_NX586,RELOC(cpu)
	jmp	2f

is386:
	movl	$CPU_386,RELOC(cpu)
	jmp	2f

try486:	/* Try to toggle identification flag; does not exist on early 486s. */
	pushfl
	popl	%eax
	movl	%eax,%ecx
	xorl	$PSL_ID,%eax
	pushl	%eax
	popfl
	pushfl
	popl	%eax
	xorl	%ecx,%eax
	andl	$PSL_ID,%eax
	pushl	%ecx
	popfl

	testl	%eax,%eax
	jnz	try586
is486:	movl	$CPU_486,RELOC(cpu)
	/*
	 * Check Cyrix CPU
	 * Cyrix CPUs do not change the undefined flags following
	 * execution of the divide instruction which divides 5 by 2.
	 *
	 * Note: CPUID is enabled on M2, so it passes another way.
	 */
	pushfl
	movl	$0x5555, %eax
	xorl	%edx, %edx
	movl	$2, %ecx
	clc
	divl	%ecx
	jnc	trycyrix486
	popfl
	jmp 2f
trycyrix486:
	movl	$CPU_6x86,RELOC(cpu) 	# set CPU type
	/*
	 * Check for Cyrix 486 CPU by seeing if the flags change during a
	 * divide. This is documented in the Cx486SLC/e SMM Programmer's
	 * Guide.
	 */
	xorl	%edx,%edx
	cmpl	%edx,%edx		# set flags to known state
	pushfl
	popl	%ecx			# store flags in ecx
	movl	$-1,%eax
	movl	$4,%ebx
	divl	%ebx			# do a long division
	pushfl
	popl	%eax
	xorl	%ecx,%eax		# are the flags different?
	testl	$0x8d5,%eax		# only check C|PF|AF|Z|N|V
	jne	2f			# yes; must be Cyrix 6x86 CPU
	movl	$CPU_486DLC,RELOC(cpu) 	# set CPU type

#ifndef CYRIX_CACHE_WORKS
	/* Disable caching of the ISA hole only. */
	invd
	movb	$CCR0,%al		# Configuration Register index (CCR0)
	outb	%al,$0x22
	inb	$0x23,%al
	orb	$(CCR0_NC1|CCR0_BARB),%al
	movb	%al,%ah
	movb	$CCR0,%al
	outb	%al,$0x22
	movb	%ah,%al
	outb	%al,$0x23
	invd
#else /* CYRIX_CACHE_WORKS */
	/* Set cache parameters */
	invd				# Start with guaranteed clean cache
	movb	$CCR0,%al		# Configuration Register index (CCR0)
	outb	%al,$0x22
	inb	$0x23,%al
	andb	$~CCR0_NC0,%al
#ifndef CYRIX_CACHE_REALLY_WORKS
	orb	$(CCR0_NC1|CCR0_BARB),%al
#else
	orb	$CCR0_NC1,%al
#endif
	movb	%al,%ah
	movb	$CCR0,%al
	outb	%al,$0x22
	movb	%ah,%al
	outb	%al,$0x23
	/* clear non-cacheable region 1	*/
	movb	$(NCR1+2),%al
	outb	%al,$0x22
	movb	$NCR_SIZE_0K,%al
	outb	%al,$0x23
	/* clear non-cacheable region 2	*/
	movb	$(NCR2+2),%al
	outb	%al,$0x22
	movb	$NCR_SIZE_0K,%al
	outb	%al,$0x23
	/* clear non-cacheable region 3	*/
	movb	$(NCR3+2),%al
	outb	%al,$0x22
	movb	$NCR_SIZE_0K,%al
	outb	%al,$0x23
	/* clear non-cacheable region 4	*/
	movb	$(NCR4+2),%al
	outb	%al,$0x22
	movb	$NCR_SIZE_0K,%al
	outb	%al,$0x23
	/* enable caching in CR0 */
	movl	%cr0,%eax
	andl	$~(CR0_CD|CR0_NW),%eax
	movl	%eax,%cr0
	invd
#endif /* CYRIX_CACHE_WORKS */

	jmp	2f

try586:	/* Use the `cpuid' instruction. */
	xorl	%eax,%eax
	cpuid
	movl	%eax,RELOC(cpuid_level)
	movl	%ebx,RELOC(cpu_vendor)	# store vendor string
	movl	%edx,RELOC(cpu_vendor)+4
	movl	%ecx,RELOC(cpu_vendor)+8
	movl	$0,  RELOC(cpu_vendor)+12

	movl	$1,%eax
	cpuid
	movl	%eax,RELOC(cpu_id)	# store cpu_id and features
	movl	%edx,RELOC(cpu_feature)

	/* Brand ID is bits 0-7 of %ebx */
	andl	$255,%ebx
	movl	%ebx,RELOC(cpu_brand_id)

2:
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
	movl	$_RELOC(tmpstk),%esp	# bootstrap stack end location

/*
 * Virtual address space of kernel:
 *
 * text | data | bss | [syms] | page dir | proc0 kstack 
 *			      0          1       2      3
 */
#define	PROC0PDIR	((0)              * NBPG)
#define	PROC0STACK	((1)              * NBPG)
#define	SYSMAP		((1+UPAGES)       * NBPG)
#define	TABLESIZE	((1+UPAGES) * NBPG) /* + nkpde * NBPG */

	/* Find end of kernel image. */
	movl	$RELOC(end),%edi
#if defined(DDB) && !defined(SYMTAB_SPACE)
	/* Save the symbols (if loaded). */
	movl	RELOC(esym),%eax
	testl	%eax,%eax
	jz	1f
	subl	$KERNBASE,%eax
	movl	%eax,%edi
1:
#endif

	/* Calculate where to start the bootstrap tables. */
	movl	%edi,%esi			# edi = esym ? esym : end
	addl	$PGOFSET,%esi			# page align up
	andl	$~PGOFSET,%esi

	/*
	 * Calculate the size of the kernel page table directory, and
	 * how many entries it will have.
	 */
	movl	RELOC(nkpde),%ecx		# get nkpde
	cmpl	$NKPTP_MIN,%ecx			# larger than min?
	jge	1f
	movl	$NKPTP_MIN,%ecx			# set at min
	jmp	2f
1:	cmpl	$NKPTP_MAX,%ecx			# larger than max?
	jle	2f
	movl	$NKPTP_MAX,%ecx
2:

	/* Clear memory for bootstrap tables. */
	shll	$PGSHIFT,%ecx
	addl	$TABLESIZE,%ecx
	addl	%esi,%ecx			# end of tables
	subl	%edi,%ecx			# size of tables
	shrl	$2,%ecx
	xorl	%eax,%eax
	cld
	rep
	stosl

/*
 * fillkpt
 *	eax = pte (page frame | control | status)
 *	ebx = page table address
 *	ecx = number of pages to map
 */
#define	fillkpt		\
1:	movl	%eax,(%ebx)	; \
	addl	$NBPG,%eax	; /* increment physical address */ \
	addl	$4,%ebx		; /* next pte */ \
	loop	1b		;

/*
 * Build initial page tables.
 */
	/* Calculate end of text segment, rounded to a page. */
	leal	(RELOC(etext)+PGOFSET),%edx
	andl	$~PGOFSET,%edx
	
	/* Skip over the first 1MB. */
	movl	$_RELOC(KERNTEXTOFF),%eax
	movl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	leal	(SYSMAP)(%esi,%ecx,4),%ebx

	/* Map the kernel text read-only. */
	movl	%edx,%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KR),%eax
	fillkpt

	/* Map the data, BSS, and bootstrap tables read-write. */
	leal	(PG_V|PG_KW)(%edx),%eax
	movl	RELOC(nkpde),%ecx
	shll	$PGSHIFT,%ecx
	addl	$TABLESIZE,%ecx
	addl	%esi,%ecx				# end of tables
	subl	%edx,%ecx				# subtract end of text
	shrl	$PGSHIFT,%ecx
	fillkpt

	/* Map ISA I/O memory. */
	movl	$(IOM_BEGIN|PG_V|PG_KW/*|PG_N*/),%eax	# having these bits set
	movl	$(IOM_SIZE>>PGSHIFT),%ecx		# for this many pte s,
	fillkpt

/*
 * Construct a page table directory.
 */
	/* Install PDEs for temporary double map of kernel. */
	movl	RELOC(nkpde),%ecx			# for this many pde s,
	leal	(PROC0PDIR+0*4)(%esi),%ebx		# which is where temp maps!
	leal	(SYSMAP+PG_V|PG_KW)(%esi),%eax		# pte for KPT in proc 0,
	fillkpt

	/* Map kernel PDEs. */
	movl	RELOC(nkpde),%ecx			# for this many pde s,
	leal	(PROC0PDIR+PDSLOT_KERN*4)(%esi),%ebx	# kernel pde offset
	leal	(SYSMAP+PG_V|PG_KW)(%esi),%eax		# pte for KPT in proc 0,
	fillkpt

	/* Install a PDE recursively mapping page directory as a page table! */
	leal	(PROC0PDIR+PG_V|PG_KW)(%esi),%eax	# pte for ptd
	movl	%eax,(PROC0PDIR+PDSLOT_PTE*4)(%esi)	# recursive PD slot

	/* Save phys. addr of PTD, for libkvm. */
	movl	%esi,RELOC(PTDpaddr)

	/* Load base of page directory and enable mapping. */
	movl	%esi,%eax		# phys address of ptd in proc 0
	movl	%eax,%cr3		# load ptd addr into mmu
	movl	%cr0,%eax		# get control word
					# enable paging & NPX emulation
	orl	$(CR0_PE|CR0_PG|CR0_NE|CR0_TS|CR0_EM|CR0_MP),%eax
	movl	%eax,%cr0		# and let's page NOW!

	pushl	$begin			# jump to high mem
	ret

begin:
	/* Now running relocated at KERNBASE.  Remove double mapping. */
	movl	_C_LABEL(nkpde),%ecx		# for this many pde s,
	leal	(PROC0PDIR+0*4)(%esi),%ebx	# which is where temp maps!
	addl	$(KERNBASE), %ebx	# now use relocated address
1:	movl	$0,(%ebx)
	addl	$4,%ebx	# next pde
	loop	1b

	/* Relocate atdevbase. */
	movl	_C_LABEL(nkpde),%edx
	shll	$PGSHIFT,%edx
	addl	$(TABLESIZE+KERNBASE),%edx
	addl	%esi,%edx
	movl	%edx,_C_LABEL(atdevbase)

	/* Set up bootstrap stack. */
	leal	(PROC0STACK+KERNBASE)(%esi),%eax
	movl	%eax,_C_LABEL(proc0paddr)
	leal	(USPACE-FRAMESIZE)(%eax),%esp
	movl	%esi,PCB_CR3(%eax)	# pcb->pcb_cr3
	xorl	%ebp,%ebp               # mark end of frames

	movl	_C_LABEL(nkpde),%eax
	shll	$PGSHIFT,%eax
	addl	$TABLESIZE,%eax
	addl	%esi,%eax		# skip past stack and page tables
	pushl	%eax
	call	_C_LABEL(init386)	# wire 386 chip for unix operation
	addl	$4,%esp

#ifdef SAFARI_FIFO_HACK
	movb	$5,%al
	movw	$0x37b,%dx
	outb	%al,%dx
	movw	$0x37f,%dx
	inb	%dx,%al
	movb	%al,%cl

	orb	$1,%cl

	movb	$5,%al
	movw	$0x37b,%dx
	outb	%al,%dx
	movw	$0x37f,%dx
	movb	%cl,%al
	outb	%al,%dx
#endif /* SAFARI_FIFO_HACK */

	call 	_C_LABEL(main)

/*
 * XXX We need a comment here (lightly) explaining this. Probably a
 * detailed section 9 man page, too, explaining the proc_trampoline.
 * Almost every port has a proc_trampoline, so it needs documentation, IMHO.
 * -- Perry Metzger, May 7, 2001
 */
/* LINTSTUB: Func: void proc_trampoline(void) */
NENTRY(proc_trampoline)
	pushl	%ebx
	call	%esi
	addl	$4,%esp
	INTRFASTEXIT
	/* NOTREACHED */

/*****************************************************************************/

/*
 * XXX No section 9 man page for sigcode or esigcode. IMHO,
 * Since it is part of the MI/MD interface, it needs documentation.
 * -- Perry Metzger, May 7, 2001
 */
/*
 * Signal trampoline; copied to top of user stack.
 */
/* LINTSTUB: Var: char sigcode[1], esigcode[1]; */
NENTRY(sigcode)
	call	SIGF_HANDLER(%esp)
	leal	SIGF_SC(%esp),%eax	# scp (the call may have clobbered the
					# copy at SIGF_SCP(%esp))
	pushl	%eax
	pushl	%eax			# junk to fake return address
	movl	$SYS___sigreturn14,%eax
	int	$0x80	 		# enter kernel with args on stack
	movl	$SYS_exit,%eax
	int	$0x80			# exit if sigreturn fails
	.globl	_C_LABEL(esigcode)
_C_LABEL(esigcode):

/*****************************************************************************/

/*
 * The following primitives are used to fill and copy regions of memory.
 */

/*
 * XXX No section 9 man page for fillw.
 * fillw seems to be very sparsely used (only in pccons it seems.)
 * One wonders if it couldn't be done without.
 * -- Perry Metzger, May 7, 2001
 */
/*
 * void fillw(short pattern, void *addr, size_t len);
 * Write len copies of pattern at addr.
 */
/* LINTSTUB: Func: void fillw(short pattern, void *addr, size_t len) */
ENTRY(fillw)
	pushl	%edi
	movl	8(%esp),%eax
	movl	12(%esp),%edi
	movw	%ax,%cx
	rorl	$16,%eax
	movw	%cx,%ax
	cld
	movl	16(%esp),%ecx
	shrl	%ecx			# do longwords
	rep
	stosl
	movl	16(%esp),%ecx
	andl	$1,%ecx			# do remainder
	rep
	stosw
	popl	%edi
	ret

/*
 * XXX No section 9 man page for kcopy. IMHO,
 * Since it is part of the MI/MD interface, it needs documentation.
 * so far as I can tell it is used only in one function in the MI kernel,
 * but it still counts.
 * -- Perry Metzger, May 7, 2001
 */
/*
 * int kcopy(const void *from, void *to, size_t len);
 * Copy len bytes, abort on fault.
 */
/* LINTSTUB: Func: int kcopy(const void *from, void *to, size_t len) */
ENTRY(kcopy)
	pushl	%esi
	pushl	%edi
	movl	_C_LABEL(curpcb),%eax	# load curpcb into eax and set on-fault
	pushl	PCB_ONFAULT(%eax)
	movl	$_C_LABEL(copy_fault), PCB_ONFAULT(%eax)

	movl	16(%esp),%esi
	movl	20(%esp),%edi
	movl	24(%esp),%ecx
	movl	%edi,%eax
	subl	%esi,%eax
	cmpl	%ecx,%eax		# overlapping?
	jb	1f
	cld				# nope, copy forward
	shrl	$2,%ecx			# copy by 32-bit words
	rep
	movsl
	movl	24(%esp),%ecx
	andl	$3,%ecx			# any bytes left?
	rep
	movsb

	movl	_C_LABEL(curpcb),%edx
	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	ret

	ALIGN_TEXT
1:	addl	%ecx,%edi		# copy backward
	addl	%ecx,%esi
	std
	andl	$3,%ecx			# any fractional bytes?
	decl	%edi
	decl	%esi
	rep
	movsb
	movl	24(%esp),%ecx		# copy remainder by 32-bit words
	shrl	$2,%ecx
	subl	$3,%esi
	subl	$3,%edi
	rep
	movsl
	cld

	movl	_C_LABEL(curpcb),%edx
	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	ret

/*****************************************************************************/

/*
 * The following primitives are used to copy data in and out of the user's
 * address space.
 */

/*
 * int copyout(const void *from, void *to, size_t len);
 * Copy len bytes into the user's address space.
 * see copyout(9)
 */
/* LINTSTUB: Func: int copyout(const void *kaddr, void *uaddr, size_t len) */
ENTRY(copyout)
	pushl	%esi
	pushl	%edi
	pushl	$0
	
	movl	16(%esp),%esi
	movl	20(%esp),%edi
	movl	24(%esp),%eax

	/*
	 * We check that the end of the destination buffer is not past the end
	 * of the user's address space.  If it's not, then we only need to
	 * check that each page is writable.  The 486 will do this for us; the
	 * 386 will not.  (We assume that pages in user space that are not
	 * writable by the user are not writable by the kernel either.)
	 */
	movl	%edi,%edx
	addl	%eax,%edx
	jc	_C_LABEL(copy_efault)
	cmpl	$VM_MAXUSER_ADDRESS,%edx
	ja	_C_LABEL(copy_efault)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class)
	jne	3f
#endif /* I486_CPU || I586_CPU || I686_CPU */

	testl	%eax,%eax		# anything to do?
	jz	3f

	/*
	 * We have to check each PTE for (write) permission, since the CPU
	 * doesn't do it for us.
	 */

	/* Compute number of pages. */
	movl	%edi,%ecx
	andl	$PGOFSET,%ecx
	addl	%eax,%ecx
	decl	%ecx
	shrl	$PGSHIFT,%ecx

	/* Compute PTE offset for start address. */
	shrl	$PGSHIFT,%edi

	movl	_C_LABEL(curpcb),%edx
	movl	$2f,PCB_ONFAULT(%edx)

1:	/* Check PTE for each page. */
	testb	$PG_RW,_C_LABEL(PTmap)(,%edi,4)
	jz	2f
	
4:	incl	%edi
	decl	%ecx
	jns	1b

	movl	20(%esp),%edi
	movl	24(%esp),%eax
	jmp	3f
	
2:	/* Simulate a trap. */
	pushl	%ecx
	movl	%edi,%eax
	shll	$PGSHIFT,%eax
	pushl	%eax
	call	_C_LABEL(trapwrite)	# trapwrite(addr)
	addl	$4,%esp			# pop argument
	popl	%ecx
	testl	%eax,%eax		# if not ok, return EFAULT
	jz	4b
	jmp	_C_LABEL(copy_efault)
#endif /* I386_CPU */

3:	movl	_C_LABEL(curpcb),%edx
	movl	$_C_LABEL(copy_fault),PCB_ONFAULT(%edx)

	/* bcopy(%esi, %edi, %eax); */
	cld
	movl	%eax,%ecx
	shrl	$2,%ecx
	rep
	movsl
	movb	%al,%cl
	andb	$3,%cl
	rep
	movsb

	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	ret

/*
 * int copyin(const void *from, void *to, size_t len);
 * Copy len bytes from the user's address space.
 * see copyin(9)
 */
/* LINTSTUB: Func: int copyin(const void *uaddr, void *kaddr, size_t len) */
ENTRY(copyin)
	pushl	%esi
	pushl	%edi
	movl	_C_LABEL(curpcb),%eax
	pushl	$0
	movl	$_C_LABEL(copy_fault),PCB_ONFAULT(%eax)
	
	movl	16(%esp),%esi
	movl	20(%esp),%edi
	movl	24(%esp),%eax

	/*
	 * We check that the end of the destination buffer is not past the end
	 * of the user's address space.  If it's not, then we only need to
	 * check that each page is readable, and the CPU will do that for us.
	 */
	movl	%esi,%edx
	addl	%eax,%edx
	jc	_C_LABEL(copy_efault)
	cmpl	$VM_MAXUSER_ADDRESS,%edx
	ja	_C_LABEL(copy_efault)

3:	/* bcopy(%esi, %edi, %eax); */
	cld
	movl	%eax,%ecx
	shrl	$2,%ecx
	rep
	movsl
	movb	%al,%cl
	andb	$3,%cl
	rep
	movsb

	movl	_C_LABEL(curpcb),%edx
	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	ret

/* LINTSTUB: Ignore */
ENTRY(copy_efault)
	movl	$EFAULT,%eax

/* LINTSTUB: Ignore */
ENTRY(copy_fault)
	movl	_C_LABEL(curpcb),%edx
	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	ret

/*
 * int copyoutstr(const void *from, void *to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long, into the
 * user's address space.  Return the number of characters copied (including the
 * NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG; else
 * return 0 or EFAULT.
 * see copyoutstr(9)
 */
/* LINTSTUB: Func: int copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done) */
ENTRY(copyoutstr)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%esi		# esi = from
	movl	16(%esp),%edi		# edi = to
	movl	20(%esp),%edx		# edx = maxlen

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class)
	jne	5f
#endif /* I486_CPU || I586_CPU || I686_CPU */

	/* Compute number of bytes in first page. */
	movl	%edi,%eax
	andl	$PGOFSET,%eax
	movl	$NBPG,%ecx
	subl	%eax,%ecx		# ecx = NBPG - (src % NBPG)

	movl	_C_LABEL(curpcb),%eax
	movl	$6f,PCB_ONFAULT(%eax)

1:	/*
	 * Once per page, check that we are still within the bounds of user
	 * space, and check for a write fault.
	 */
	cmpl	$VM_MAXUSER_ADDRESS,%edi
	jae	_C_LABEL(copystr_efault)

	/* Compute PTE offset. */
	movl	%edi,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address

	testb	$PG_RW,_C_LABEL(PTmap)(,%eax,4)
	jnz	2f

6:	/* Simulate a trap. */
	pushl	%edx
	pushl	%edi
	call	_C_LABEL(trapwrite)	# trapwrite(addr)
	addl	$4,%esp			# clear argument from stack
	popl	%edx
	testl	%eax,%eax
	jnz	_C_LABEL(copystr_efault)

2:	/* Copy up to end of this page. */
	subl	%ecx,%edx		# predecrement total count
	jnc	3f
	addl	%edx,%ecx		# ecx += (edx - ecx) = edx
	xorl	%edx,%edx

3:	decl	%ecx
	js	4f
	lodsb
	stosb
	testb	%al,%al
	jnz	3b

	/* Success -- 0 byte reached. */
	addl	%ecx,%edx		# add back residual for this page
	xorl	%eax,%eax
	jmp	copystr_return

4:	/* Go to next page, if any. */
	movl	$NBPG,%ecx
	testl	%edx,%edx
	jnz	1b

	/* edx is zero -- return ENAMETOOLONG. */
	movl	$ENAMETOOLONG,%eax
	jmp	copystr_return
#endif /* I386_CPU */

#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
5:	movl	_C_LABEL(curpcb),%eax
	movl	$_C_LABEL(copystr_fault),PCB_ONFAULT(%eax)
	/*
	 * Get min(%edx, VM_MAXUSER_ADDRESS-%edi).
	 */
	movl	$VM_MAXUSER_ADDRESS,%eax
	subl	%edi,%eax
	cmpl	%edx,%eax
	jae	1f
	movl	%eax,%edx
	movl	%eax,20(%esp)

1:	incl	%edx
	cld

1:	decl	%edx
	jz	2f
	lodsb
	stosb
	testb	%al,%al
	jnz	1b

	/* Success -- 0 byte reached. */
	decl	%edx
	xorl	%eax,%eax
	jmp	copystr_return

2:	/* edx is zero -- return EFAULT or ENAMETOOLONG. */
	cmpl	$VM_MAXUSER_ADDRESS,%edi
	jae	_C_LABEL(copystr_efault)
	movl	$ENAMETOOLONG,%eax
	jmp	copystr_return
#endif /* I486_CPU || I586_CPU || I686_CPU */

/*
 * int copyinstr(const void *from, void *to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long, from the
 * user's address space.  Return the number of characters copied (including the
 * NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG; else
 * return 0 or EFAULT.
 * see copyinstr(9)
 */
/* LINTSTUB: Func: int copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done) */
ENTRY(copyinstr)
	pushl	%esi
	pushl	%edi
	movl	_C_LABEL(curpcb),%ecx
	movl	$_C_LABEL(copystr_fault),PCB_ONFAULT(%ecx)

	movl	12(%esp),%esi		# %esi = from
	movl	16(%esp),%edi		# %edi = to
	movl	20(%esp),%edx		# %edx = maxlen

	/*
	 * Get min(%edx, VM_MAXUSER_ADDRESS-%esi).
	 */
	movl	$VM_MAXUSER_ADDRESS,%eax
	subl	%esi,%eax
	cmpl	%edx,%eax
	jae	1f
	movl	%eax,%edx
	movl	%eax,20(%esp)

1:	incl	%edx
	cld

1:	decl	%edx
	jz	2f
	lodsb
	stosb
	testb	%al,%al
	jnz	1b

	/* Success -- 0 byte reached. */
	decl	%edx
	xorl	%eax,%eax
	jmp	copystr_return

2:	/* edx is zero -- return EFAULT or ENAMETOOLONG. */
	cmpl	$VM_MAXUSER_ADDRESS,%esi
	jae	_C_LABEL(copystr_efault)
	movl	$ENAMETOOLONG,%eax
	jmp	copystr_return

/* LINTSTUB: Ignore */
ENTRY(copystr_efault)
	movl	$EFAULT,%eax

/* LINTSTUB: Ignore */
ENTRY(copystr_fault)
copystr_return:
	/* Set *lencopied and return %eax. */
	movl	_C_LABEL(curpcb),%ecx
	movl	$0,PCB_ONFAULT(%ecx)
	movl	20(%esp),%ecx
	subl	%edx,%ecx
	movl	24(%esp),%edx
	testl	%edx,%edx
	jz	8f
	movl	%ecx,(%edx)

8:	popl	%edi
	popl	%esi
	ret

/*
 * int copystr(const void *from, void *to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long.  Return the
 * number of characters copied (including the NUL) in *lencopied.  If the
 * string is too long, return ENAMETOOLONG; else return 0.
 * see copystr(9)
 */
/* LINTSTUB: Func: int copystr(const void *kfaddr, void *kdaddr, size_t len, size_t *done) */
ENTRY(copystr)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%esi		# esi = from
	movl	16(%esp),%edi		# edi = to
	movl	20(%esp),%edx		# edx = maxlen
	incl	%edx
	cld

1:	decl	%edx
	jz	4f
	lodsb
	stosb
	testb	%al,%al
	jnz	1b

	/* Success -- 0 byte reached. */
	decl	%edx
	xorl	%eax,%eax
	jmp	6f

4:	/* edx is zero -- return ENAMETOOLONG. */
	movl	$ENAMETOOLONG,%eax

6:	/* Set *lencopied and return %eax. */
	movl	20(%esp),%ecx
	subl	%edx,%ecx
	movl	24(%esp),%edx
	testl	%edx,%edx
	jz	7f
	movl	%ecx,(%edx)

7:	popl	%edi
	popl	%esi
	ret

/*
 * long fuword(const void *uaddr);
 * Fetch an int from the user's address space.
 * see fuword(9)
 */
/* LINTSTUB: Func: long fuword(const void *base) */
ENTRY(fuword)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-4,%edx
	ja	_C_LABEL(fusuaddrfault)
	movl	_C_LABEL(curpcb),%ecx
	movl	$_C_LABEL(fusufault),PCB_ONFAULT(%ecx)
	movl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret
	
/*
 * int fusword(const void *uaddr);
 * Fetch a short from the user's address space.
 * see fusword(9)
 */
/* LINTSTUB: Func: int fusword(const void *base) */
ENTRY(fusword)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-2,%edx
	ja	_C_LABEL(fusuaddrfault)
	movl	_C_LABEL(curpcb),%ecx
	movl	$_C_LABEL(fusufault),PCB_ONFAULT(%ecx)
	movzwl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret
	
/*
 * int fuswintr(const void *uaddr);
 * Fetch a short from the user's address space.  Can be called during an
 * interrupt.
 * see fuswintr(9)
 */
/* LINTSTUB: Func: int fuswintr(const void *base) */
ENTRY(fuswintr)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-2,%edx
	ja	_C_LABEL(fusuaddrfault)
	movl	_C_LABEL(curpcb),%ecx
	movl	$_C_LABEL(fusubail),PCB_ONFAULT(%ecx)
	movzwl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret
	
/*
 * int fubyte(const void *uaddr);
 * Fetch a byte from the user's address space.
 * see fubyte(9)
 */
/* LINTSTUB: Func: int fubyte(const void *base) */
ENTRY(fubyte)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-1,%edx
	ja	_C_LABEL(fusuaddrfault)
	movl	_C_LABEL(curpcb),%ecx
	movl	$_C_LABEL(fusufault),PCB_ONFAULT(%ecx)
	movzbl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret

/*
 * Handle faults from [fs]u*().  Clean up and return -1.
 */
/* LINTSTUB: Ignore */
ENTRY(fusufault)
	movl	$0,PCB_ONFAULT(%ecx)
	movl	$-1,%eax
	ret

/*
 * Handle faults from [fs]u*().  Clean up and return -1.  This differs from
 * fusufault() in that trap() will recognize it and return immediately rather
 * than trying to page fault.
 */
/* LINTSTUB: Ignore */
ENTRY(fusubail)
	movl	$0,PCB_ONFAULT(%ecx)
	movl	$-1,%eax
	ret

/*
 * Handle earlier faults from [fs]u*(), due to our of range addresses.
 */
/* LINTSTUB: Ignore */
ENTRY(fusuaddrfault)
	movl	$-1,%eax
	ret

/*
 * int suword(void *uaddr, long x);
 * Store an int in the user's address space.
 * see suword(9)
 */
/* LINTSTUB: Func: int suword(void *base, long c) */
ENTRY(suword)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-4,%edx
	ja	_C_LABEL(fusuaddrfault)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class)
	jne	2f
#endif /* I486_CPU || I586_CPU || I686_CPU */

	movl	_C_LABEL(curpcb),%eax
	movl	$3f,PCB_ONFAULT(%eax)

	movl	%edx,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address
	testb	$PG_RW,_C_LABEL(PTmap)(,%eax,4)
	jnz	1f

3:	/* Simulate a trap. */
	pushl	%edx
	pushl	%edx
	call	_C_LABEL(trapwrite)	# trapwrite(addr)
	addl	$4,%esp			# clear parameter from the stack
	popl	%edx
	movl	_C_LABEL(curpcb),%ecx
	testl	%eax,%eax
	jnz	_C_LABEL(fusufault)

1:	/* XXX also need to check the following 3 bytes for validity! */
#endif

2:	movl	_C_LABEL(curpcb),%ecx
	movl	$_C_LABEL(fusufault),PCB_ONFAULT(%ecx)

	movl	8(%esp),%eax
	movl	%eax,(%edx)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx)
	ret
	
/*
 * int susword(void *uaddr, short x);
 * Store a short in the user's address space.
 * see susword(9)
 */
/* LINTSTUB: Func: int susword(void *base, short c) */
ENTRY(susword)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-2,%edx
	ja	_C_LABEL(fusuaddrfault)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class)
	jne	2f
#endif /* I486_CPU || I586_CPU || I686_CPU */

	movl	_C_LABEL(curpcb),%eax
	movl	$3f,PCB_ONFAULT(%eax)

	movl	%edx,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address
	testb	$PG_RW,_C_LABEL(PTmap)(,%eax,4)
	jnz	1f

3:	/* Simulate a trap. */
	pushl	%edx
	pushl	%edx
	call	_C_LABEL(trapwrite)	# trapwrite(addr)
	addl	$4,%esp			# clear parameter from the stack
	popl	%edx
	movl	_C_LABEL(curpcb),%ecx
	testl	%eax,%eax
	jnz	_C_LABEL(fusufault)

1:	/* XXX also need to check the following byte for validity! */
#endif

2:	movl	_C_LABEL(curpcb),%ecx
	movl	$_C_LABEL(fusufault),PCB_ONFAULT(%ecx)

	movl	8(%esp),%eax
	movw	%ax,(%edx)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx)
	ret

/*
 * int suswintr(void *uaddr, short x);
 * Store a short in the user's address space.  Can be called during an
 * interrupt.
 * see suswintr(9)
 */
/* LINTSTUB: Func: int suswintr(void *base, short c) */
ENTRY(suswintr)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-2,%edx
	ja	_C_LABEL(fusuaddrfault)
	movl	_C_LABEL(curpcb),%ecx
	movl	$_C_LABEL(fusubail),PCB_ONFAULT(%ecx)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class)
	jne	2f
#endif /* I486_CPU || I586_CPU || I686_CPU */

	movl	%edx,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address
	testb	$PG_RW,_C_LABEL(PTmap)(,%eax,4)
	jnz	1f

	/* Simulate a trap. */
	jmp	_C_LABEL(fusubail)

1:	/* XXX also need to check the following byte for validity! */
#endif

2:	movl	8(%esp),%eax
	movw	%ax,(%edx)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx)
	ret

/*
 * int subyte(void *uaddr, char x);
 * Store a byte in the user's address space.
 * see subyte(9)
 */
/* LINTSTUB: Func: int subyte(void *base, int c) */
ENTRY(subyte)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-1,%edx
	ja	_C_LABEL(fusuaddrfault)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class)
	jne	2f
#endif /* I486_CPU || I586_CPU || I686_CPU */

	movl	_C_LABEL(curpcb),%eax
	movl	$3f,PCB_ONFAULT(%eax)

	movl	%edx,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address
	testb	$PG_RW,_C_LABEL(PTmap)(,%eax,4)
	jnz	1f

3:	/* Simulate a trap. */
	pushl	%edx
	pushl	%edx
	call	_C_LABEL(trapwrite)	# trapwrite(addr)
	addl	$4,%esp			# clear parameter from the stack
	popl	%edx
	movl	_C_LABEL(curpcb),%ecx
	testl	%eax,%eax
	jnz	_C_LABEL(fusufault)

1:
#endif

2:	movl	_C_LABEL(curpcb),%ecx
	movl	$_C_LABEL(fusufault),PCB_ONFAULT(%ecx)

	movb	8(%esp),%al
	movb	%al,(%edx)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx)
	ret

/*****************************************************************************/

/*
 * The following is i386-specific nonsense.
 */

/*
 * void lgdt(struct region_descriptor *rdp);
 * Change the global descriptor table.
 * XXX should there be an MD section 9 man page for this?
 *     or even just a better comment? --Perry, May 7, 2001
 */
/* LINTSTUB: Func: void lgdt(struct region_descriptor *rdp) */
NENTRY(lgdt)
	/* Reload the descriptor table. */
	movl	4(%esp),%eax
	lgdt	(%eax)
	/* Flush the prefetch q. */
	jmp	1f
	nop
1:	/* Reload "stale" selectors. */
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movl	%ax,%ds
	movl	%ax,%es
	movl	%ax,%ss
	/* Reload code selector by doing intersegment return. */
	popl	%eax
	pushl	$GSEL(GCODE_SEL, SEL_KPL)
	pushl	%eax
	lret


/*
 * XXX We need a comment here (lightly) explaining this. Probably a
 * short section 9 man page, too, explaining how kernel setjmp differs
 * from userland.
 * Since it is part of the MI/MD interface, it needs documentation, IMHO.
 * -- Perry Metzger, May 7, 2001
 */
/* LINTSTUB: Func: int setjmp (label_t *l) */
ENTRY(setjmp)
	movl	4(%esp),%eax
	movl	%ebx,(%eax)		# save ebx
	movl	%esp,4(%eax)		# save esp
	movl	%ebp,8(%eax)		# save ebp
	movl	%esi,12(%eax)		# save esi
	movl	%edi,16(%eax)		# save edi
	movl	(%esp),%edx		# get rta
	movl	%edx,20(%eax)		# save eip
	xorl	%eax,%eax		# return (0);
	ret

/*
 * XXX We need a comment here (lightly) explaining this. Probably a
 * short section 9 man page, too, explaining how kernel longjmp differs
 * from userland.
 * Since it is part of the MI/MD interface, it needs documentation, IMHO.
 * -- Perry Metzger, May 7, 2001
 */
/* LINTSTUB: Func: void longjmp (label_t *l) */
ENTRY(longjmp)
	movl	4(%esp),%eax
	movl	(%eax),%ebx		# restore ebx
	movl	4(%eax),%esp		# restore esp
	movl	8(%eax),%ebp		# restore ebp
	movl	12(%eax),%esi		# restore esi
	movl	16(%eax),%edi		# restore edi
	movl	20(%eax),%edx		# get rta
	movl	%edx,(%esp)		# put in return frame
	xorl	%eax,%eax		# return (1);
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
 * void setrunqueue(struct proc *p);
 * Insert a process on the appropriate queue.  Should be called at splclock().
 * See setrunqueue(9) for more details.
 */
/* LINTSTUB: Func: void setrunqueue(struct proc *p) */
NENTRY(setrunqueue)
	movl	4(%esp),%eax
#ifdef DIAGNOSTIC
	cmpl	$0,P_BACK(%eax)	# should not be on q already
	jne	1f
	cmpl	$0,P_WCHAN(%eax)
	jne	1f
	cmpb	$SRUN,P_STAT(%eax)
	jne	1f
#endif /* DIAGNOSTIC */
	movzbl	P_PRIORITY(%eax),%edx
	shrl	$2,%edx
	btsl	%edx,_C_LABEL(sched_whichqs)	# set q full bit
	leal	_C_LABEL(sched_qs)(,%edx,8),%edx # locate q hdr
	movl	P_BACK(%edx),%ecx
	movl	%edx,P_FORW(%eax)	# link process on tail of q
	movl	%eax,P_BACK(%edx)
	movl	%eax,P_FORW(%ecx)
	movl	%ecx,P_BACK(%eax)
	ret
#ifdef DIAGNOSTIC
1:	pushl	$2f
	call	_C_LABEL(panic)
	/* NOTREACHED */
2:	.asciz	"setrunqueue"
#endif /* DIAGNOSTIC */

/*
 * void remrunqueue(struct proc *p);
 * Remove a process from its queue.  Should be called at splclock().
 * See remrunqueue(9) for more details.
 */
/* LINTSTUB: Func: void remrunqueue(struct proc *p) */
NENTRY(remrunqueue)
	movl	4(%esp),%ecx
	movzbl	P_PRIORITY(%ecx),%eax
#ifdef DIAGNOSTIC
	shrl	$2,%eax
	btl	%eax,_C_LABEL(sched_whichqs)
	jnc	1f
#endif /* DIAGNOSTIC */
	movl	P_BACK(%ecx),%edx	# unlink process
	movl	$0,P_BACK(%ecx)		# zap reverse link to indicate off list
	movl	P_FORW(%ecx),%ecx
	movl	%ecx,P_FORW(%edx)
	movl	%edx,P_BACK(%ecx)
	cmpl	%ecx,%edx		# q still has something?
	jne	2f
#ifndef DIAGNOSTIC
	shrl	$2,%eax
#endif
	btrl	%eax,_C_LABEL(sched_whichqs)	# no; clear bit
2:	ret
#ifdef DIAGNOSTIC
1:	pushl	$3f
	call	_C_LABEL(panic)
	/* NOTREACHED */
3:	.asciz	"remrunqueue"
#endif /* DIAGNOSTIC */

#if NAPM > 0
/* LINTSTUB: Func: void apm_cpu_busy (void) */
/* LINTSTUB: Func: void apm_cpu_idle (void) */
	.globl _C_LABEL(apm_cpu_idle),_C_LABEL(apm_cpu_busy)
#endif
/*
 * When no processes are on the runq, cpu_switch() branches to here to wait for
 * something to come ready.
 */
/* LINTSTUB: Ignore */
ENTRY(idle)
	/*
	 * When we get here, interrupts are off (via cli) and
	 * sched_lock is held.
	 */
	movl	_C_LABEL(sched_whichqs),%ecx
	testl	%ecx,%ecx
	jnz	sw1
#if defined(LOCKDEBUG)
	call	_C_LABEL(sched_unlock_idle)
#endif
	sti

	/* Try to zero some pages. */
	movl	_C_LABEL(uvm)+UVM_PAGE_IDLE_ZERO,%ecx
	testl	%ecx,%ecx
	jz	1f
	call	_C_LABEL(uvm_pageidlezero)
1:
#if NAPM > 0
	call	_C_LABEL(apm_cpu_idle)
#endif
	hlt
#if NAPM > 0
	call	_C_LABEL(apm_cpu_busy)
#endif
	cli
#if defined(LOCKDEBUG)
	call	_C_LABEL(sched_lock_idle)
#endif
	jmp	_C_LABEL(idle)

#ifdef DIAGNOSTIC
/* LINTSTUB: Ignore */
NENTRY(switch_error)
	pushl	$1f
	call	_C_LABEL(panic)
	/* NOTREACHED */
1:	.asciz	"cpu_switch"
#endif /* DIAGNOSTIC */

/*
 * void cpu_switch(struct proc *)
 * Find a runnable process and switch to it.  Wait if necessary.  If the new
 * process is the same as the old one, we short-circuit the context save and
 * restore.
 * see cpu_switch(9)
 */
/* LINTSTUB: Func: void cpu_switch(struct proc *p) */
ENTRY(cpu_switch)
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	_C_LABEL(cpl)

	movl	_C_LABEL(curproc),%esi

	/*
	 * Clear curproc so that we don't accumulate system time while idle.
	 * This also insures that schedcpu() will move the old process to
	 * the correct queue if it happens to get called from the spllower()
	 * below and changes the priority.  (See corresponding comment in
	 * userret()).
	 */
	movl	$0,_C_LABEL(curproc)

#if defined(LOCKDEBUG)
	/* Release the sched_lock before processing interrupts. */
	call	_C_LABEL(sched_unlock_idle)
#endif

	movl	$0,_C_LABEL(cpl)	# spl0()
	call	_C_LABEL(Xspllower)	# process pending interrupts

switch_search:
	/*
	 * First phase: find new process.
	 *
	 * Registers:
	 *   %eax - queue head, scratch, then zero
	 *   %ebx - queue number
	 *   %ecx - cached value of whichqs
	 *   %edx - next process in queue
	 *   %esi - old process
	 *   %edi - new process
	 */

	/* Lock the scheduler. */
	cli				# splhigh doesn't do a cli
#if defined(LOCKDEBUG)
	call	_C_LABEL(sched_lock_idle)
#endif

	/* Wait for new process. */
	movl	_C_LABEL(sched_whichqs),%ecx

sw1:	bsfl	%ecx,%ebx		# find a full q
	jz	_C_LABEL(idle)		# if none, idle

	leal	_C_LABEL(sched_qs)(,%ebx,8),%eax # select q

	movl	P_FORW(%eax),%edi	# unlink from front of process q
#ifdef	DIAGNOSTIC
	cmpl	%edi,%eax		# linked to self (i.e. nothing queued)?
	je	_C_LABEL(switch_error)	# not possible
#endif /* DIAGNOSTIC */
	movl	P_FORW(%edi),%edx
	movl	%edx,P_FORW(%eax)
	movl	%eax,P_BACK(%edx)

	cmpl	%edx,%eax		# q empty?
	jne	3f

	btrl	%ebx,%ecx		# yes, clear to indicate empty
	movl	%ecx,_C_LABEL(sched_whichqs) # update q status

3:	/* We just did it. */
	xorl	%eax,%eax
	movl	%eax,_C_LABEL(want_resched)

#ifdef	DIAGNOSTIC
	cmpl	%eax,P_WCHAN(%edi)	# Waiting for something?
	jne	_C_LABEL(switch_error)	# Yes; shouldn't be queued.
	cmpb	$SRUN,P_STAT(%edi)	# In run state?
	jne	_C_LABEL(switch_error)	# No; shouldn't be queued.
#endif /* DIAGNOSTIC */

	/* Isolate process.  XXX Is this necessary? */
	movl	%eax,P_BACK(%edi)

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
	movb	$SONPROC,P_STAT(%edi)	# p->p_stat = SONPROC
	movl	%edi,_C_LABEL(curproc)

	/* It's okay to take interrupts here. */
	sti

	/* Skip context switch if same process. */
	cmpl	%edi,%esi
	je	switch_return

	/* If old process exited, don't bother. */
	testl	%esi,%esi
	jz	switch_exited

	/*
	 * Second phase: save old context.
	 *
	 * Registers:
	 *   %eax, %ecx - scratch
	 *   %esi - old process, then old pcb
	 *   %edi - new process
	 */

	movl	P_ADDR(%esi),%esi

	/* Save stack pointers. */
	movl	%esp,PCB_ESP(%esi)
	movl	%ebp,PCB_EBP(%esi)

switch_exited:
	/*
	 * Third phase: restore saved context.
	 *
	 * Registers:
	 *   %eax, %ecx, %edx - scratch
	 *   %esi - new pcb
	 *   %edi - new process
	 */

	/* No interrupts while loading new state. */
	cli
	movl	P_ADDR(%edi),%esi

	/* Restore stack pointers. */
	movl	PCB_ESP(%esi),%esp
	movl	PCB_EBP(%esi),%ebp

#if 0
	/* Don't bother with the rest if switching to a system process. */
	testl	$P_SYSTEM,P_FLAG(%edi)
	jnz	switch_restored
#endif

	/*
	 * Activate the address space.  We're curproc, so %cr3 will
	 * be reloaded, but we're not yet curpcb, so the LDT won't
	 * be reloaded, although the PCB copy of the selector will
	 * be refreshed from the pmap.
	 */
	pushl	%edi
	call	_C_LABEL(pmap_activate)
	addl	$4,%esp

	/* Load TSS info. */
	movl	_C_LABEL(gdt),%eax
	movl	P_MD_TSS_SEL(%edi),%edx

	/* Switch TSS. Reset "task busy" flag before */
	andl	$~0x0200,4(%eax,%edx, 1)
	ltr	%dx

#ifdef USER_LDT
	/*
	 * Switch LDT.
	 *
	 * XXX
	 * Always do this, because the LDT could have been swapped into a
	 * different selector after a process exited.  (See gdt_compact().)
	 */
	movl	PCB_LDT_SEL(%esi),%edx
	lldt	%dx
#endif /* USER_LDT */

	/* Restore segment registers. */
switch_restored:
	/* Restore cr0 (including FPU state). */
	movl	PCB_CR0(%esi),%ecx
	movl	%ecx,%cr0

	/* Record new pcb. */
	movl	%esi,_C_LABEL(curpcb)

	/* Interrupts are okay again. */
	sti

switch_return:
	/*
	 * Restore old cpl from stack.  Note that this is always an increase,
	 * due to the spl0() on entry.
	 */
	popl	_C_LABEL(cpl)

	movl	%edi,%eax		# return (p);
	popl	%edi
	popl	%esi
	popl	%ebx
	ret

/*
 * void switch_exit(struct proc *p);
 * Switch to proc0's saved context and deallocate the address space and kernel
 * stack for p.  Then jump into cpu_switch(), as if we were in proc0 all along.
 */
	.globl	_C_LABEL(proc0),_C_LABEL(uvmspace_free),_C_LABEL(kernel_map)
	.globl	_C_LABEL(uvm_km_free),_C_LABEL(tss_free)
/* LINTSTUB: Func: void switch_exit(struct proc *p) */
ENTRY(switch_exit)
	movl	4(%esp),%edi		# old process
	movl	$_C_LABEL(proc0),%ebx

	/* In case we fault... */
	movl	$0,_C_LABEL(curproc)

	/* Restore proc0's context. */
	cli
	movl	P_ADDR(%ebx),%esi

	/* Restore stack pointers. */
	movl	PCB_ESP(%esi),%esp
	movl	PCB_EBP(%esi),%ebp

	/* Load TSS info. */
	movl	_C_LABEL(gdt),%eax
	movl	P_MD_TSS_SEL(%ebx),%edx

	/* Switch address space. */
	movl	PCB_CR3(%esi),%ecx
	movl	%ecx,%cr3

	/* Switch TSS. */
	andl	$~0x0200,4-SEL_KPL(%eax,%edx,1)
	ltr	%dx

	/* We're always in the kernel, so we don't need the LDT. */

	/* Clear segment registers; always null in proc0. */
	xorl	%ecx,%ecx
	movl	%cx,%fs
	movl	%cx,%gs

	/* Restore cr0 (including FPU state). */
	movl	PCB_CR0(%esi),%ecx
	movl	%ecx,%cr0

	/* Record new pcb. */
	movl	%esi,_C_LABEL(curpcb)

	/* Interrupts are okay again. */
	sti

	/*
	 * Schedule the dead process's vmspace and stack to be freed.
	 */
	pushl	%edi			/* exit2(p) */
	call	_C_LABEL(exit2)
	addl	$4,%esp

	/* Jump into cpu_switch() with the right state. */
	movl	%ebx,%esi
	movl	$0,_C_LABEL(curproc)
	jmp	switch_search

/*
 * void savectx(struct pcb *pcb);
 * Update pcb, saving current processor state.
 */
/* LINTSTUB: Func: void savectx(struct pcb *pcb) */
ENTRY(savectx)
	movl	4(%esp),%edx		# edx = p->p_addr
  
	/* Save stack pointers. */
	movl	%esp,PCB_ESP(%edx)
	movl	%ebp,PCB_EBP(%edx)

	ret

/*****************************************************************************/

/*
 * Trap and fault vector routines
 *
 * On exit from the kernel to user mode, we always need to check for ASTs.  In
 * addition, we need to do this atomically; otherwise an interrupt may occur
 * which causes an AST, but it won't get processed until the next kernel entry
 * (possibly the next clock tick).  Thus, we disable interrupt before checking,
 * and only enable them again on the final `iret' or before calling the AST
 * handler.
 *
 * XXX - debugger traps are now interrupt gates so at least bdb doesn't lose
 * control.  The sti's give the standard losing behaviour for ddb and kgdb.
 */ 

/*
 * XXX traditional CPP's evaluation semantics make this necessary.
 * XXX (__CONCAT() would be evaluated incorrectly)
 */
#ifdef __ELF__
#define	IDTVEC(name)	ALIGN_TEXT; .globl X/**/name; X/**/name:
#else
#define	IDTVEC(name)	ALIGN_TEXT; .globl _X/**/name; _X/**/name:
#endif

#define	TRAP(a)		pushl $(a) ; jmp _C_LABEL(alltraps)
#define	ZTRAP(a)	pushl $0 ; TRAP(a)

#ifdef IPKDB
#define	BPTTRAP(a)	pushl $0; pushl $(a); jmp _C_LABEL(bpttraps)
#else
#define	BPTTRAP(a)	ZTRAP(a)
#endif

	.text
IDTVEC(trap00)
	ZTRAP(T_DIVIDE)
IDTVEC(trap01)
	BPTTRAP(T_TRCTRAP)
IDTVEC(trap02)
	ZTRAP(T_NMI)
IDTVEC(trap03)
	BPTTRAP(T_BPTFLT)
IDTVEC(trap04)
	ZTRAP(T_OFLOW)
IDTVEC(trap05)
	ZTRAP(T_BOUND)
IDTVEC(trap06)
	ZTRAP(T_PRIVINFLT)
IDTVEC(trap07)
#if NNPX > 0
	pushl	$0			# dummy error code
	pushl	$T_DNA
	INTRENTRY
	pushl	_C_LABEL(curproc)
	call	_C_LABEL(npxdna)
	addl	$4,%esp
	testl	%eax,%eax
	jz	calltrap
	INTRFASTEXIT
#else
	ZTRAP(T_DNA)
#endif
IDTVEC(trap08)
	TRAP(T_DOUBLEFLT)
IDTVEC(trap09)
	ZTRAP(T_FPOPFLT)
IDTVEC(trap0a)
	TRAP(T_TSSFLT)
IDTVEC(trap0b)
	TRAP(T_SEGNPFLT)
IDTVEC(trap0c)
	TRAP(T_STKFLT)
IDTVEC(trap0d)
	TRAP(T_PROTFLT)
IDTVEC(trap0e)
#ifndef I586_CPU
	TRAP(T_PAGEFLT)
#else
	pushl	$T_PAGEFLT
	INTRENTRY
	testb	$PGEX_U,TF_ERR(%esp)
	jnz	calltrap
	movl	%cr2,%eax
	subl	_C_LABEL(pentium_idt),%eax
	cmpl	$(6*8),%eax
	jne	calltrap
	movb	$T_PRIVINFLT,TF_TRAPNO(%esp)
	jmp	calltrap
#endif
IDTVEC(trap0f)
	/*
	 * The Pentium Pro local APIC may erroneously call this vector for a
	 * default IR7.  Just ignore it.
	 */
	iret
IDTVEC(trap10)
#if NNPX > 0
	/*
	 * Handle like an interrupt so that we can call npxintr to clear the
	 * error.  It would be better to handle npx interrupts as traps but
	 * this is difficult for nested interrupts.
	 */
	pushl	$0			# dummy error code
	pushl	$T_ASTFLT
	INTRENTRY
	pushl	_C_LABEL(cpl)
	pushl	%esp
	incl	_C_LABEL(uvmexp)+V_TRAP
	call	_C_LABEL(npxintr)
	addl	$8,%esp
	INTRFASTEXIT
#else
	ZTRAP(T_ARITHTRAP)
#endif
IDTVEC(trap11)
	ZTRAP(T_ALIGNFLT)
IDTVEC(trap12)
IDTVEC(trap13)
IDTVEC(trap14)
IDTVEC(trap15)
IDTVEC(trap16)
IDTVEC(trap17)
IDTVEC(trap18)
IDTVEC(trap19)
IDTVEC(trap1a)
IDTVEC(trap1b)
IDTVEC(trap1c)
IDTVEC(trap1d)
IDTVEC(trap1e)
IDTVEC(trap1f)
	/* 18 - 31 reserved for future exp */
	ZTRAP(T_RESERVED)

IDTVEC(exceptions)
	.long	_C_LABEL(Xtrap00), _C_LABEL(Xtrap01)
	.long	_C_LABEL(Xtrap02), _C_LABEL(Xtrap03)
	.long	_C_LABEL(Xtrap04), _C_LABEL(Xtrap05)
	.long	_C_LABEL(Xtrap06), _C_LABEL(Xtrap07)
	.long	_C_LABEL(Xtrap08), _C_LABEL(Xtrap09)
	.long	_C_LABEL(Xtrap0a), _C_LABEL(Xtrap0b)
	.long	_C_LABEL(Xtrap0c), _C_LABEL(Xtrap0d)
	.long	_C_LABEL(Xtrap0e), _C_LABEL(Xtrap0f)
	.long	_C_LABEL(Xtrap10), _C_LABEL(Xtrap11)
	.long	_C_LABEL(Xtrap12), _C_LABEL(Xtrap13)
	.long	_C_LABEL(Xtrap14), _C_LABEL(Xtrap15)
	.long	_C_LABEL(Xtrap16), _C_LABEL(Xtrap17)
	.long	_C_LABEL(Xtrap18), _C_LABEL(Xtrap19)
	.long	_C_LABEL(Xtrap1a), _C_LABEL(Xtrap1b)
	.long	_C_LABEL(Xtrap1c), _C_LABEL(Xtrap1d)
	.long	_C_LABEL(Xtrap1e), _C_LABEL(Xtrap1f)

/*
 * If an error is detected during trap, syscall, or interrupt exit, trap() will
 * change %eip to point to one of these labels.  We clean up the stack, if
 * necessary, and resume as if we were handling a general protection fault.
 * This will cause the process to get a SIGBUS.
 */
/* LINTSTUB: Var: char resume_iret[1]; */
NENTRY(resume_iret)
	ZTRAP(T_PROTFLT)
/* LINTSTUB: Var: char resume_pop_ds[1]; */
NENTRY(resume_pop_ds)
	pushl	%es
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movl	%ax,%es
/* LINTSTUB: Var: char resume_pop_es[1]; */
NENTRY(resume_pop_es)
	pushl	%fs
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movl	%ax,%fs
/* LINTSTUB: Var: char resume_pop_fs[1]; */
NENTRY(resume_pop_fs)
	pushl	%gs	
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movl	%ax,%gs
/* LINTSTUB: Var: char resume_pop_gs[1]; */
NENTRY(resume_pop_gs)
	movl	$T_PROTFLT,TF_TRAPNO(%esp)
	jmp	calltrap

/* LINTSTUB: Ignore */
NENTRY(alltraps)
	INTRENTRY
calltrap:
#ifdef DIAGNOSTIC
	movl	_C_LABEL(cpl),%ebx
#endif /* DIAGNOSTIC */
	call	_C_LABEL(trap)
2:	/* Check for ASTs on exit to user mode. */
	cli
	cmpb	$0,_C_LABEL(astpending)
	je	1f
	testb	$SEL_RPL,TF_CS(%esp)
#ifdef VM86
	jnz	5f
	testl	$PSL_VM,TF_EFLAGS(%esp)
#endif
	jz	1f
5:	movb	$0,_C_LABEL(astpending)
	sti
	movl	$T_ASTFLT,TF_TRAPNO(%esp)
	call	_C_LABEL(trap)
	jmp	2b
#ifndef DIAGNOSTIC
1:	INTRFASTEXIT
#else /* DIAGNOSTIC */
1:	cmpl	_C_LABEL(cpl),%ebx
	jne	3f
	INTRFASTEXIT
3:	sti
	pushl	$4f
	call	_C_LABEL(printf)
	addl	$4,%esp
#ifdef DDB
	int	$3
#endif /* DDB */
	movl	%ebx,_C_LABEL(cpl)
	jmp	2b
4:	.asciz	"WARNING: SPL NOT LOWERED ON TRAP EXIT\n"
#endif /* DIAGNOSTIC */

#ifdef IPKDB
/* LINTSTUB: Ignore */
NENTRY(bpttraps)
	INTRENTRY
	call	_C_LABEL(ipkdb_trap_glue)
	testl	%eax,%eax
	jz	calltrap
	INTRFASTEXIT

ipkdbsetup:
	popl	%ecx

	/* Disable write protection: */
	movl	%cr0,%eax
	pushl	%eax
	andl	$~CR0_WP,%eax
	movl	%eax,%cr0

	/* Substitute Protection & Page Fault handlers: */
	movl	_C_LABEL(idt),%edx
	pushl	13*8(%edx)
	pushl	13*8+4(%edx)
	pushl	14*8(%edx)
	pushl	14*8+4(%edx)
	movl	$fault,%eax
	movw	%ax,13*8(%edx)
	movw	%ax,14*8(%edx)
	shrl	$16,%eax
	movw	%ax,13*8+6(%edx)
	movw	%ax,14*8+6(%edx)

	pushl	%ecx
	ret

ipkdbrestore:
	popl	%ecx

	/* Restore Protection & Page Fault handlers: */
	movl	_C_LABEL(idt),%edx
	popl	14*8+4(%edx)
	popl	14*8(%edx)
	popl	13*8+4(%edx)
	popl	13*8(%edx)

	/* Restore write protection: */
	popl	%edx
	movl	%edx,%cr0

	pushl	%ecx
	ret

/* XXX: Documentation! grrr! --Perry Metzger, May 7, 2001 */
/* LINTSTUB: Func: int ipkdbfbyte(u_char *c) */
NENTRY(ipkdbfbyte)
	pushl	%ebp
	movl	%esp,%ebp
	call	ipkdbsetup
	movl	8(%ebp),%edx
	movzbl	(%edx),%eax
faultexit:
	call	ipkdbrestore
	popl	%ebp
	ret

/* XXX: Documentation! grrr! --Perry Metzger, May 7, 2001 */
/* LINTSTUB: Func: int ipkdbsbyte(u_char *c, int i) */
NENTRY(ipkdbsbyte)
	pushl	%ebp
	movl	%esp,%ebp
	call	ipkdbsetup
	movl	8(%ebp),%edx
	movl	12(%ebp),%eax
	movb	%al,(%edx)
	call	ipkdbrestore
	popl	%ebp
	ret

fault:
	popl	%eax		/* error code */
	movl	$faultexit,%eax
	movl	%eax,(%esp)
	movl	$-1,%eax
	iret
#endif	/* IPKDB */

/*
 * Old call gate entry for syscall
 */
/* XXX Manually doing Xblah is wrong. Yuck. --Perry */
/* LINTSTUB: Var: char Xosyscall[1]; */
IDTVEC(osyscall)
	/* Set eflags in trap frame. */
	pushfl
	popl	8(%esp)
	pushl	$7		# size of instruction for restart
	jmp	syscall1

/*
 * Trap gate entry for syscall
 */
/* XXX Manually doing Xblah is wrong. Yuck. --Perry */
/* LINTSTUB: Var: char Xsyscall[1]; */
IDTVEC(syscall)
	pushl	$2		# size of instruction for restart
syscall1:
	pushl	$T_ASTFLT	# trap # for doing ASTs
	INTRENTRY
	movl	_C_LABEL(curproc),%edx	# get pointer to curproc
#ifdef DIAGNOSTIC
	movl	_C_LABEL(cpl),%ebx
#endif /* DIAGNOSTIC */
	movl	%esp,P_MD_REGS(%edx)	# save pointer to frame
	call	P_MD_SYSCALL(%edx)	# get pointer to syscall() function
2:	/* Check for ASTs on exit to user mode. */
	cli
	cmpb	$0,_C_LABEL(astpending)
	je	1f
	/* Always returning to user mode here. */
	movb	$0,_C_LABEL(astpending)
	sti
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	call	_C_LABEL(trap)
	jmp	2b
#ifndef DIAGNOSTIC
1:	INTRFASTEXIT
#else /* DIAGNOSTIC */
1:	cmpl	_C_LABEL(cpl),%ebx
	jne	3f
	INTRFASTEXIT
3:	sti
	pushl	$4f
	call	_C_LABEL(printf)
	addl	$4,%esp
#ifdef DDB
	int	$3
#endif /* DDB */
	movl	%ebx,_C_LABEL(cpl)
	jmp	2b
4:	.asciz	"WARNING: SPL NOT LOWERED ON SYSCALL EXIT\n"
#endif /* DIAGNOSTIC */

#if NNPX > 0
/*
 * Special interrupt handlers.  Someday intr0-intr15 will be used to count
 * interrupts.  We'll still need a special exception 16 handler.  The busy
 * latch stuff in probintr() can be moved to npxprobe().
 */

/* LINTSTUB: Func: void probeintr(void) */
NENTRY(probeintr)
	ss
	incl	_C_LABEL(npx_intrs_while_probing)
	pushl	%eax
	movb	$0x20,%al	# EOI (asm in strings loses cpp features)
	outb	%al,$0xa0	# IO_ICU2
	outb	%al,$0x20	# IO_ICU1
	movb	$0,%al
	outb	%al,$0xf0	# clear BUSY# latch
	popl	%eax
	iret

/* LINTSTUB: Func: void probetrap(void) */
NENTRY(probetrap)
	ss
	incl	_C_LABEL(npx_traps_while_probing)
	fnclex
	iret

/* LINTSTUB: Func: int npx586bug1(int a, int b) */
NENTRY(npx586bug1)
	fildl	4(%esp)		# x
	fildl	8(%esp)		# y
	fld	%st(1)
	fdiv	%st(1),%st	# x/y
	fmulp	%st,%st(1)	# (x/y)*y
	fsubrp	%st,%st(1)	# x-(x/y)*y
	pushl	$0
	fistpl	(%esp)
	popl	%eax
	ret
#endif /* NNPX > 0 */

#include <i386/isa/vector.s>
#include <i386/isa/icu.s>
