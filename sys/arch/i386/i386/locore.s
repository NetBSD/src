/*	$NetBSD: locore.s,v 1.213.4.1 1999/11/15 00:38:00 fvdl Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
#include "opt_vm86.h"
#include "opt_user_ldt.h"
#include "opt_dummy_nops.h"
#include "opt_compat_freebsd.h"
#include "opt_compat_linux.h"
#include "opt_compat_ibcs2.h"
#include "opt_compat_svr4.h"
#include "opt_compat_oldboot.h"

#include "npx.h"
#include "assym.h"
#include "apm.h"

#include <sys/errno.h>
#include <sys/syscall.h>
#ifdef COMPAT_SVR4
#include <compat/svr4/svr4_syscall.h>
#endif
#ifdef COMPAT_LINUX
#include <compat/linux/linux_syscall.h>
#endif
#ifdef COMPAT_FREEBSD
#include <compat/freebsd/freebsd_syscall.h>
#endif
#ifdef COMPAT_IBCS2
#include <compat/ibcs2/ibcs2_syscall.h>
#endif

#include <machine/cputypes.h>
#include <machine/param.h>
#include <machine/pte.h>
#include <machine/segments.h>
#include <machine/specialreg.h>
#include <machine/trap.h>
#include <machine/bootinfo.h>

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
#define	IOM_BEGIN	0x0a0000	/* start of I/O memory "hole" */
#define	IOM_END		0x100000	/* end of I/O memory "hole" */
#define	IOM_SIZE	(IOM_END - IOM_BEGIN)


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
	pushl	%ebp		; \
	pushl	%esi		; \
	pushl	%edi		; \
	pushl	%ds		; \
	pushl	%es		; \
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax	; \
	movl	%ax,%ds		; \
	movl	%ax,%es
#define	INTRFASTEXIT \
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
	pushl	(%ecx)		/* len */
	pushl	%edx
	addl	(%ecx), %edx		/* update dest pointer */
	cmpl	$_RELOC(_C_LABEL(bootinfo) + BOOTINFO_MAXSIZE), %edx
	jg	2f
	pushl	%ecx
	call	_C_LABEL(bcopy)
	addl	$12, %esp
	popl	%eax
	subl	$1, %ebx
	jmp	2b
2:	/* cleanup for overflow case */
	addl	$12, %esp
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
#ifdef DDB
	orl	$(PG_V|PG_KW),%eax
#else
	orl	$(PG_V|PG_KR),%eax
#endif
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

	/* Clear segment registers; always null in proc0. */
	xorl	%ecx,%ecx
	movl	%cx,%fs
	movl	%cx,%gs

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

NENTRY(proc_trampoline)
	pushl	%ebx
	call	%esi
	addl	$4,%esp
	INTRFASTEXIT
	/* NOTREACHED */

/*****************************************************************************/

/*
 * Signal trampoline; copied to top of user stack.
 */
NENTRY(sigcode)
	call	SIGF_HANDLER(%esp)
	leal	SIGF_SC(%esp),%eax	# scp (the call may have clobbered the
					# copy at SIGF_SCP(%esp))
#ifdef VM86
	testl	$PSL_VM,SC_EFLAGS(%eax)
	jnz	1f
#endif
	movl	SC_FS(%eax),%ecx
	movl	SC_GS(%eax),%edx
	movl	%cx,%fs
	movl	%dx,%gs
1:	pushl	%eax
	pushl	%eax			# junk to fake return address
	movl	$SYS___sigreturn14,%eax
	int	$0x80	 		# enter kernel with args on stack
	movl	$SYS_exit,%eax
	int	$0x80			# exit if sigreturn fails
	.globl	_C_LABEL(esigcode)
_C_LABEL(esigcode):

/*****************************************************************************/

#ifdef COMPAT_SVR4
NENTRY(svr4_sigcode)
	call	SVR4_SIGF_HANDLER(%esp)
	leal	SVR4_SIGF_UC(%esp),%eax	# ucp (the call may have clobbered the
					# copy at SIGF_UCP(%esp))
#ifdef VM86
	testl	$PSL_VM,SVR4_UC_EFLAGS(%eax)
	jnz	1f
#endif
	movl	SVR4_UC_FS(%eax),%ecx
	movl	SVR4_UC_GS(%eax),%edx
	movl	%cx,%fs
	movl	%dx,%gs
1:	pushl	%eax
	pushl	$1			# setcontext(p) == syscontext(1, p) 
	pushl	%eax			# junk to fake return address
	movl	$SVR4_SYS_context,%eax
	int	$0x80	 		# enter kernel with args on stack
	movl	$SVR4_SYS_exit,%eax
	int	$0x80			# exit if sigreturn fails
	.globl	_C_LABEL(svr4_esigcode)
_C_LABEL(svr4_esigcode):
#endif

/*****************************************************************************/

#ifdef COMPAT_LINUX
/*
 * Signal trampoline; copied to top of user stack.
 */
NENTRY(linux_sigcode)
	call	LINUX_SIGF_HANDLER(%esp)
	leal	LINUX_SIGF_SC(%esp),%ebx # scp (the call may have clobbered the
					# copy at SIGF_SCP(%esp))
#ifdef VM86
	testl	$PSL_VM,LINUX_SC_EFLAGS(%ebx)
	jnz	1f
#endif
	movl	LINUX_SC_FS(%ebx),%ecx
	movl	LINUX_SC_GS(%ebx),%edx
	movl	%cx,%fs
	movl	%dx,%gs
1:	pushl	%eax			# junk to fake return address
	movl	$LINUX_SYS_sigreturn,%eax
	int	$0x80	 		# enter kernel with args on stack
	movl	$LINUX_SYS_exit,%eax
	int	$0x80			# exit if sigreturn fails
	.globl	_C_LABEL(linux_esigcode)
_C_LABEL(linux_esigcode):

NENTRY(linux_rt_sigcode)
	call	LINUX_SIGF_HANDLER(%esp)
	leal	LINUX_SIGF_SC(%esp),%ebx # scp (the call may have clobbered the
					# copy at SIGF_SCP(%esp))
#ifdef VM86
	testl	$PSL_VM,LINUX_SC_EFLAGS(%ebx)
	jnz	1f
#endif
	movl	LINUX_SC_FS(%ebx),%ecx
	movl	LINUX_SC_GS(%ebx),%edx
	movl	%cx,%fs
	movl	%dx,%gs
1:	pushl	%eax			# junk to fake return address
	movl	$LINUX_SYS_rt_sigreturn,%eax
	int	$0x80	 		# enter kernel with args on stack
	movl	$LINUX_SYS_exit,%eax
	int	$0x80			# exit if sigreturn fails
	.globl	_C_LABEL(linux_rt_esigcode)
_C_LABEL(linux_rt_esigcode):
#endif

/*****************************************************************************/

#ifdef COMPAT_FREEBSD
/*
 * Signal trampoline; copied to top of user stack.
 */
NENTRY(freebsd_sigcode)
	call	FREEBSD_SIGF_HANDLER(%esp)
	leal	FREEBSD_SIGF_SC(%esp),%eax # scp (the call may have clobbered
					# the copy at SIGF_SCP(%esp))
	pushl	%eax
	pushl	%eax			# junk to fake return address
	movl	$FREEBSD_SYS_sigreturn,%eax
	int	$0x80	 		# enter kernel with args on stack
	movl	$FREEBSD_SYS_exit,%eax
	int	$0x80			# exit if sigreturn fails
	.globl	_C_LABEL(freebsd_esigcode)
_C_LABEL(freebsd_esigcode):
#endif

/*****************************************************************************/

#ifdef COMPAT_IBCS2
NENTRY(ibcs2_sigcode)
	call    SIGF_HANDLER(%esp)
	leal    SIGF_SC(%esp),%eax      # scp (the call may have clobbered the
					# copy at SIGF_SCP(%esp))
	movl    SC_FS(%eax),%ecx
	movl    SC_GS(%eax),%edx
	movl    %cx,%fs
	movl    %dx,%gs
	pushl   %eax
	pushl   %eax                    # junk to fake return address
	movl    $IBCS2_SYS_sigreturn,%eax
	int     $0x80                   # enter kernel with args on stack
	movl    $IBCS2_SYS_exit,%eax
	int     $0x80                   # exit if sigreturn fails
	.globl  _C_LABEL(ibcs2_esigcode)
_C_LABEL(ibcs2_esigcode):
#endif
	
/*****************************************************************************/

/*
 * The following primitives are used to fill and copy regions of memory.
 */

/*
 * fillw(short pattern, caddr_t addr, size_t len);
 * Write len copies of pattern at addr.
 */
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
 * bcopyb(caddr_t from, caddr_t to, size_t len);
 * Copy len bytes, one byte at a time.
 */
ENTRY(bcopyb)
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%esi
	movl	16(%esp),%edi
	movl	20(%esp),%ecx
	cmpl	%esi,%edi		# potentially overlapping?
	jnb	1f
	cld				# no; copy forward
	rep
	movsb
	popl	%edi
	popl	%esi
	ret

	ALIGN_TEXT
1:	addl	%ecx,%edi		# copy backward
	addl	%ecx,%esi
	std
	decl	%edi
	decl	%esi
	rep
	movsb
	popl	%edi
	popl	%esi
	cld
	ret

/*
 * bcopyw(caddr_t from, caddr_t to, size_t len);
 * Copy len bytes, two bytes at a time.
 */
ENTRY(bcopyw)
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%esi
	movl	16(%esp),%edi
	movl	20(%esp),%ecx
	cmpl	%esi,%edi		# potentially overlapping?
	jnb	1f
	cld				# no; copy forward
	shrl	$1,%ecx			# copy by 16-bit words
	rep
	movsw
	adc	%ecx,%ecx		# any bytes left?
	rep
	movsb
	popl	%edi
	popl	%esi
	ret

	ALIGN_TEXT
1:	addl	%ecx,%edi		# copy backward
	addl	%ecx,%esi
	std
	andl	$1,%ecx			# any fractional bytes?
	decl	%edi
	decl	%esi
	rep
	movsb
	movl	20(%esp),%ecx		# copy remainder by 16-bit words
	shrl	$1,%ecx
	decl	%esi
	decl	%edi
	rep
	movsw
	popl	%edi
	popl	%esi
	cld
	ret

/*
 * bcopy(caddr_t from, caddr_t to, size_t len);
 * Copy len bytes.
 * This routine handles overlapping regions, although bcopy
 * is not specified to do so (and should not be counted on to do so).
 */
ENTRY(bcopy)
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%esi
	movl	16(%esp),%edi
	movl	20(%esp),%ecx
	movl	%edi,%eax
	subl	%esi,%eax
	cmpl	%ecx,%eax		# overlapping?
	jb	1f
	cld				# nope, copy forward
	shrl	$2,%ecx			# copy by 32-bit words
	rep
	movsl
	movl	20(%esp),%ecx
	andl	$3,%ecx			# any bytes left?
	rep
	movsb
	popl	%edi
	popl	%esi
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
	movl	20(%esp),%ecx		# copy remainder by 32-bit words
	shrl	$2,%ecx
	subl	$3,%esi
	subl	$3,%edi
	rep
	movsl
	popl	%edi
	popl	%esi
	cld
	ret


/*
 * kcopy(caddr_t from, caddr_t to, size_t len);
 * Copy len bytes, abort on fault.
 */
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
 * copyout(caddr_t from, caddr_t to, size_t len);
 * Copy len bytes into the user's address space.
 */
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
	jc	_C_LABEL(copy_fault)
	cmpl	$VM_MAXUSER_ADDRESS,%edx
	ja	_C_LABEL(copy_fault)

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
	jmp	3f
	
2:	/* Simulate a trap. */
	pushl	%eax
	pushl	%ecx
	movl	%edi,%eax
	shll	$PGSHIFT,%eax
	pushl	%eax
	call	_C_LABEL(trapwrite)	# trapwrite(addr)
	addl	$4,%esp			# pop argument
	popl	%ecx
	testl	%eax,%eax		# if not ok, return EFAULT
	popl	%eax
	jz	4b
	jmp	_C_LABEL(copy_fault)
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
 * copyin(caddr_t from, caddr_t to, size_t len);
 * Copy len bytes from the user's address space.
 */
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
	jc	_C_LABEL(copy_fault)
	cmpl	$VM_MAXUSER_ADDRESS,%edx
	ja	_C_LABEL(copy_fault)

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

ENTRY(copy_fault)
	movl	_C_LABEL(curpcb),%edx
	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	movl	$EFAULT,%eax
	ret

/*
 * copyoutstr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long, into the
 * user's address space.  Return the number of characters copied (including the
 * NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG; else
 * return 0 or EFAULT.
 */
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
	jae	_C_LABEL(copystr_fault)

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
	jnz	_C_LABEL(copystr_fault)

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
	jae	_C_LABEL(copystr_fault)
	movl	$ENAMETOOLONG,%eax
	jmp	copystr_return
#endif /* I486_CPU || I586_CPU || I686_CPU */

/*
 * copyinstr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long, from the
 * user's address space.  Return the number of characters copied (including the
 * NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG; else
 * return 0 or EFAULT.
 */
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
	jae	_C_LABEL(copystr_fault)
	movl	$ENAMETOOLONG,%eax
	jmp	copystr_return

ENTRY(copystr_fault)
	movl	$EFAULT,%eax

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
 * copystr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long.  Return the
 * number of characters copied (including the NUL) in *lencopied.  If the
 * string is too long, return ENAMETOOLONG; else return 0.
 */
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
 * fuword(caddr_t uaddr);
 * Fetch an int from the user's address space.
 */
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
 * fusword(caddr_t uaddr);
 * Fetch a short from the user's address space.
 */
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
 * fuswintr(caddr_t uaddr);
 * Fetch a short from the user's address space.  Can be called during an
 * interrupt.
 */
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
 * fubyte(caddr_t uaddr);
 * Fetch a byte from the user's address space.
 */
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
ENTRY(fusufault)
	movl	$0,PCB_ONFAULT(%ecx)
	movl	$-1,%eax
	ret

/*
 * Handle faults from [fs]u*().  Clean up and return -1.  This differs from
 * fusufault() in that trap() will recognize it and return immediately rather
 * than trying to page fault.
 */
ENTRY(fusubail)
	movl	$0,PCB_ONFAULT(%ecx)
	movl	$-1,%eax
	ret

/*
 * Handle earlier faults from [fs]u*(), due to our of range addresses.
 */
ENTRY(fusuaddrfault)
	movl	$-1,%eax
	ret

/*
 * suword(caddr_t uaddr, int x);
 * Store an int in the user's address space.
 */
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
 * susword(caddr_t uaddr, short x);
 * Store a short in the user's address space.
 */
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
 * suswintr(caddr_t uaddr, short x);
 * Store a short in the user's address space.  Can be called during an
 * interrupt.
 */
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
 * subyte(caddr_t uaddr, char x);
 * Store a byte in the user's address space.
 */
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
 */
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
	.globl	_C_LABEL(whichqs),_C_LABEL(qs),_C_LABEL(uvmexp),_C_LABEL(panic)

/*
 * setrunqueue(struct proc *p);
 * Insert a process on the appropriate queue.  Should be called at splclock().
 */
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
	btsl	%edx,_C_LABEL(whichqs)		# set q full bit
	leal	_C_LABEL(qs)(,%edx,8),%edx	# locate q hdr
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
 * remrunqueue(struct proc *p);
 * Remove a process from its queue.  Should be called at splclock().
 */
NENTRY(remrunqueue)
	movl	4(%esp),%ecx
	movzbl	P_PRIORITY(%ecx),%eax
#ifdef DIAGNOSTIC
	shrl	$2,%eax
	btl	%eax,_C_LABEL(whichqs)
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
	btrl	%eax,_C_LABEL(whichqs)	# no; clear bit
2:	ret
#ifdef DIAGNOSTIC
1:	pushl	$3f
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
	cli
	movl	_C_LABEL(whichqs),%ecx
	testl	%ecx,%ecx
	jnz	sw1
	sti
#if NAPM > 0
	call	_C_LABEL(apm_cpu_idle)
#endif
	hlt
#if NAPM > 0
	call	_C_LABEL(apm_cpu_busy)
#endif
	jmp	_C_LABEL(idle)

#ifdef DIAGNOSTIC
NENTRY(switch_error)
	pushl	$1f
	call	_C_LABEL(panic)
	/* NOTREACHED */
1:	.asciz	"cpu_switch"
#endif /* DIAGNOSTIC */

/*
 * cpu_switch(void);
 * Find a runnable process and switch to it.  Wait if necessary.  If the new
 * process is the same as the old one, we short-circuit the context save and
 * restore.
 */
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

	/* Wait for new process. */
	cli				# splhigh doesn't do a cli
	movl	_C_LABEL(whichqs),%ecx

sw1:	bsfl	%ecx,%ebx		# find a full q
	jz	_C_LABEL(idle)		# if none, idle

	leal	_C_LABEL(qs)(,%ebx,8),%eax	# select q

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
	movl	%ecx,_C_LABEL(whichqs)	# update q status

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

	/* Record new process. */
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

	/* Save segment registers. */
	movl	%fs,%ax
	movl	%gs,%cx
	movl	%eax,PCB_FS(%esi)
	movl	%ecx,PCB_GS(%esi)

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

	/* Load TSS info. */
	movl	_C_LABEL(gdt),%eax
	movl	PCB_TSS_SEL(%esi),%edx

	/* Switch address space. */
	movl	PCB_CR3(%esi),%ecx
	movl	%ecx,%cr3

	/* Switch TSS. */
	andl	$~0x0200,4-SEL_KPL(%eax,%edx,1)
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
	movl	PCB_FS(%esi),%eax
	movl	PCB_GS(%esi),%ecx
	movl	%ax,%fs
	movl	%cx,%gs

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
 * switch_exit(struct proc *p);
 * Switch to proc0's saved context and deallocate the address space and kernel
 * stack for p.  Then jump into cpu_switch(), as if we were in proc0 all along.
 */
	.globl	_C_LABEL(proc0),_C_LABEL(uvmspace_free),_C_LABEL(kernel_map)
	.globl	_C_LABEL(uvm_km_free),_C_LABEL(tss_free)
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
	movl	PCB_TSS_SEL(%esi),%edx

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
 * savectx(struct pcb *pcb);
 * Update pcb, saving current processor state.
 */
ENTRY(savectx)
	movl	4(%esp),%edx		# edx = p->p_addr
  
	/* Save segment registers. */
	movl	%fs,%ax
	movl	%gs,%cx
	movl	%eax,PCB_FS(%edx)
	movl	%ecx,PCB_GS(%edx)

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

	.text
IDTVEC(trap00)
	ZTRAP(T_DIVIDE)
IDTVEC(trap01)
	ZTRAP(T_TRCTRAP)
IDTVEC(trap02)
	ZTRAP(T_NMI)
IDTVEC(trap03)
	ZTRAP(T_BPTFLT)
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
NENTRY(resume_iret)
	ZTRAP(T_PROTFLT)
NENTRY(resume_pop_ds)
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movl	%ax,%es
NENTRY(resume_pop_es)
	movl	$T_PROTFLT,TF_TRAPNO(%esp)
	jmp	calltrap

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

/*
 * Old call gate entry for syscall
 */
IDTVEC(osyscall)
	/* Set eflags in trap frame. */
	pushfl
	popl	8(%esp)
	/* Turn off trace flag and nested task. */
	pushfl
	andb	$~((PSL_T|PSL_NT)>>8),1(%esp)
	popfl
	pushl	$7		# size of instruction for restart
	jmp	syscall1

/*
 * Trap gate entry for syscall
 */
IDTVEC(syscall)
	pushl	$2		# size of instruction for restart
syscall1:
	pushl	$T_ASTFLT	# trap # for doing ASTs
	INTRENTRY
#ifdef DIAGNOSTIC
	movl	_C_LABEL(cpl),%ebx
#endif /* DIAGNOSTIC */
	call	_C_LABEL(syscall)
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

#ifdef COMPAT_SVR4
IDTVEC(svr4_fasttrap)
	pushl	$2		# size of instruction for restart
	pushl	$T_ASTFLT	# trap # for doing ASTs
	INTRENTRY
	call	_C_LABEL(svr4_fasttrap)
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
1:	INTRFASTEXIT
#endif /* COMPAT_SVR4 */

#if NNPX > 0
/*
 * Special interrupt handlers.  Someday intr0-intr15 will be used to count
 * interrupts.  We'll still need a special exception 16 handler.  The busy
 * latch stuff in probintr() can be moved to npxprobe().
 */

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

NENTRY(probetrap)
	ss
	incl	_C_LABEL(npx_traps_while_probing)
	fnclex
	iret

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
