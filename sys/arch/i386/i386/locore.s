/*	$NetBSD: locore.s,v 1.110 1995/02/05 01:41:33 mycroft Exp $	*/

#undef DIAGNOSTIC
#define DIAGNOSTIC
/*-
 * Copyright (c) 1993, 1994, 1995 Charles Hannum.
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

/*
 * locore.s:	4BSD machine support for the Intel 386
 *		Preliminary version
 *		Written by William F. Jolitz, 386BSD Project
 */

#include "npx.h"
#include "assym.s"

#include <sys/errno.h>
#include <sys/syscall.h>

#include <machine/cputypes.h>
#include <machine/param.h>
#include <machine/pte.h>
#include <machine/specialreg.h>
#include <machine/trap.h>

#include <i386/isa/debug.h>

#define	KCSEL		0x08
#define	KDSEL		0x10
#define	SEL_RPL_MASK	0x0003

/* XXX temporary kluge; these should not be here */
#define	IOM_BEGIN	0x0a0000	/* start of I/O memory "hole" */
#define	IOM_END		0x100000	/* end of I/O memory "hole" */
#define	IOM_SIZE	(IOM_END - IOM_BEGIN)


#define	ALIGN_DATA	.align	2
#define	ALIGN_TEXT	.align	2,0x90	/* 4-byte boundaries, NOP-filled */
#define	SUPERALIGN_TEXT	.align	4,0x90	/* 16-byte boundaries better for 486 */

/* NB: NOP now preserves registers so NOPs can be inserted anywhere */
/* XXX: NOP and FASTER_NOP are misleadingly named */
#ifdef DUMMY_NOPS	/* this will break some older machines */
#define	FASTER_NOP
#define	NOP
#else
#define	FASTER_NOP	pushl %eax ; inb $0x84,%al ; popl %eax
#define	NOP	pushl %eax ; inb $0x84,%al ; inb $0x84,%al ; popl %eax
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
	movl	$KDSEL,%eax	; \
	movl	%ax,%ds		; \
	movl	%ax,%es
#define	INTREXIT \
	jmp	_Xdoreti
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
 */
	.globl	_PTmap,_PTD,_PTDpde,_Sysmap
	.set	_PTmap,(PTDPTDI << PDSHIFT)
	.set	_PTD,(_PTmap + PTDPTDI * NBPG)
	.set	_PTDpde,(_PTD + PTDPTDI * 4)		# XXX 4 == sizeof pde
	.set	_Sysmap,(_PTmap + KPTDI * NBPG)

/*
 * APTmap, APTD is the alternate recursive pagemap.
 * It's used when modifying another process's page tables.
 */
	.globl	_APTmap,_APTD,_APTDpde
	.set	_APTmap,(APTDPTDI << PDSHIFT)
	.set	_APTD,(_APTmap + APTDPTDI * NBPG)
	.set	_APTDpde,(_PTD + APTDPTDI * 4)		# XXX 4 == sizeof pde

/*
 * Access to each processes kernel stack is via a region of per-process address
 * space (at the beginning), immediatly above the user process stack.
 */
	.set	_kstack,USRSTACK
	.globl	_kstack

#define	ENTRY(name)	.globl _/**/name; ALIGN_TEXT; _/**/name:
#define	ALTENTRY(name)	.globl _/**/name; _/**/name:

/*
 * Initialization
 */
	.data

	.globl	_cpu,_cpu_vendor,_cold,_esym,_boothowto,_bootdev,_atdevbase
	.globl	_cyloffset,_proc0paddr,_curpcb,_IdlePTD,_KPTphys
_cpu:		.long	0	# are we 386, 386sx, or 486
_cpu_vendor:	.space	16	# vendor string returned by `cpuid' instruction
_cold:		.long	1	# cold till we are not
_esym:		.long	0	# ptr to end of syms
_atdevbase:	.long	0	# location of start of iomem in virtual
_atdevphys:	.long	0	# location of device mapping ptes (phys)
_cyloffset:	.long	0
_proc0paddr:	.long	0
_IdlePTD:	.long	0
_KPTphys:	.long	0

	.space 512
tmpstk:

	.text
	.globl	start
start:	movw	$0x1234,0x472			# warm boot

	/*
	 * Load parameters from stack (howto, bootdev, unit, cyloffset, esym).
	 * note: (%esp) is return address of boot
	 * (If we want to hold onto /boot, it's physical %esp up to _end.)
	 */
	movl	4(%esp),%eax
	movl	%eax,_boothowto-KERNBASE
	movl	8(%esp),%eax
	movl	%eax,_bootdev-KERNBASE
	movl	12(%esp),%eax
	movl	%eax,_cyloffset-KERNBASE
 	movl	16(%esp),%eax
	testl	%eax,%eax
	jz	1f
 	addl	$(KERNBASE),%eax
1: 	movl	%eax,_esym-KERNBASE

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
	movl	$CPU_386,_cpu-KERNBASE
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
is486:	movl	$CPU_486,_cpu-KERNBASE

	/*
	 * Check for Cyrix CPU by seeing if the flags change during a divide.
	 * This is documented in the Cx486SLC/e SMM Programmer's Guide.
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
	jne	2f			# yes; must not be Cyrix CPU

	movl	$CPU_486DLC,_cpu-KERNBASE 		# set CPU type
	movl	$0x69727943,_cpu_vendor-KERNBASE	# store vendor string
	movw	$0x0078,_cpu_vendor-KERNBASE+4

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
	.byte	0x0f,0xa2		# cpuid 0
	movl	%ebx,_cpu_vendor-KERNBASE	# store vendor string
	movl	%edx,_cpu_vendor-KERNBASE+4
	movl	%ecx,_cpu_vendor-KERNBASE+8
	movb	$0,_cpu_vendor-KERNBASE+12

	movl	$1,%eax
	.byte	0x0f,0xa2		# cpuid 1
	rorl	$8,%eax			# extract family type
	andl	$15,%eax
	cmpl	$5,%eax
	jb	is486			# less than a Pentium
	movl	$CPU_586,_cpu-KERNBASE

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
	movl	$(tmpstk-KERNBASE),%esp	# bootstrap stack end location

/*
 * Virtual address space of kernel:
 *
 * text | data | bss | [syms] | page dir | usr stk map | proc0 kstack | Sysmap
 *			      0          1             2       3      4
 */
#define	PROC0PDIR	((0)              * NBPG)
#define	PROC0STACKMAP	((1)              * NBPG)
#define	PROC0STACK	((2)              * NBPG)
#define	SYSMAP		((2+UPAGES)       * NBPG)
#define	TABLESIZE	((2+UPAGES+NKPDE) * NBPG)

	/* Clear the BSS. */
	movl	$(_edata-KERNBASE),%edi
	movl	$(((_end-_edata)+3)>>2),%ecx
	xorl	%eax,%eax
	cld
	rep
	stosl

	/* Find end of kernel image. */
	movl	$(_end-KERNBASE),%edi
#if defined(DDB) && !defined(SYMTAB_SPACE)
	/* Save the symbols (if loaded). */
	movl	_esym-KERNBASE,%eax
	testl	%eax,%eax
	jz	1f
	subl	$KERNBASE,%eax
	movl	%eax,%edi
1:
#endif

	/* Calculate where to start the bootstrap tables. */
	movl	%edi,%esi			# edi = esym ?: end
	addl	$PGOFSET,%esi			# page align up
	andl	$~PGOFSET,%esi

	/* Clear memory for bootstrap tables. */
	leal	(TABLESIZE)(%esi),%ecx		# end of tables
	subl	%edi,%ecx			# size of tables
	shrl	$2,%ecx
	xorl	%eax,%eax
	cld
	rep
	stosl

	movl	%esi,_IdlePTD-KERNBASE		# physaddr of proc0 stack

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
	leal	(SYSMAP)(%esi),%ebx			#   physical address of KPT in proc 0,
	movl	%ebx,_KPTphys-KERNBASE			#    in the kernel page table,

	/* XXX Map the first MB of memory read-write. */
	movl	$(0+PG_V|PG_KW),%eax			#  having these bits set,
	movl	$(KERNTEXTOFF-KERNBASE),%ecx
	shrl	$PGSHIFT,%ecx
	fillkpt

	/* Calculate end of text segment, rounded to a page. */
	leal	(_etext-KERNBASE+PGOFSET),%edx
	andl	$~PGOFSET,%edx
	
	/* Map the kernel text read-only. */
	leal	(KERNBASE-KERNTEXTOFF)(%edx),%ecx
	shrl	$PGSHIFT,%ecx
	andl	$PG_FRAME,%eax
#ifdef DDB
	orl	$(PG_V|PG_KW),%eax
#else
	orl	$(PG_V|PG_KR),%eax
#endif
	fillkpt

	/* Map the data, BSS, and bootstrap tables read-write. */
	leal	(TABLESIZE)(%esi),%ecx		# end of tables
	subl	%edx,%ecx			# subtract end of text
	shrl	$PGSHIFT,%ecx
	andl	$PG_FRAME,%eax
	orl	$(PG_V|PG_KW),%eax
	fillkpt

	/* Map ISA I/O memory. */
	movl	$(IOM_SIZE>>PGSHIFT),%ecx		# for this many pte s,
	movl	$(IOM_BEGIN|PG_V|PG_KW/*|PG_N*/),%eax	# having these bits set
	movl	%ebx,_atdevphys-KERNBASE		# remember phys addr of ptes
	fillkpt

	/* Map proc 0's kernel stack into user page table page. */
	movl	$UPAGES,%ecx				# for this many pte s,
	leal	(PROC0STACK+KERNBASE)(%esi),%edx
	movl	%edx,_proc0paddr-KERNBASE		# remember VA for 0th process init
	leal	(PROC0STACK+PG_V|PG_KW)(%esi),%eax	# physical address in proc 0
	leal	(PROC0STACKMAP+NBPG-UPAGES*4)(%esi),%ebx # physical address of stack pt in proc 0
	fillkpt

/*
 * Construct a page table directory.
 */
	/* Install a PDE for temporary double map of kernel text. */
	leal	(SYSMAP+PG_V|PG_KW)(%esi),%eax		#   pte for KPT in proc 0,
	movl	%eax,(%esi)				# which is where temp maps!

	/* Map kernel PDEs. */
	movl	$NKPDE,%ecx				# for this many pde s,
	leal	(KPTDI*4)(%esi),%ebx			# offset of pde for kernel
	fillkpt

	/* Install a PDE recursively mapping page directory as a page table! */
	leal	(PROC0PDIR+PG_V|PG_KW)(%esi),%eax	# pte for ptd
	movl	%eax,(PTDPTDI*4)(%esi)			# which is where PTmap maps!

	/* Install a PDE to map kernel stack for proc 0. */
	leal	(PROC0STACKMAP+PG_V|PG_KW)(%esi),%eax	# pte for pt in proc 0
	movl	%eax,(UPTDI*4)(%esi)			# which is where kernel stack maps!

#ifdef BDB
	/* Copy and convert stuff from old GDT and IDT for debugger. */
	cmpl	$0x0375c339,0x96104	# XXX - debugger signature
	jne	1f
	movb	$1,_bdb_exists-KERNBASE

	pushal
	subl	$2*6,%esp

	sgdt	(%esp)
	movl	2(%esp),%esi		# base address of current gdt
	movl	$(_gdt-KERNBASE),%edi
	movl	%edi,2(%esp)
	movl	$8*18/4,%ecx
	rep				# copy gdt
	movsl
	movl	$(_gdt-KERNBASE),-8+2(%edi)	# adjust gdt self-ptr
	movb	$0x92,-8+5(%edi)

	sidt	6(%esp)
	movl	6+2(%esp),%esi		# base address of current idt
	movl	8+4(%esi),%eax		# convert dbg descriptor to ...
	movw	8(%esi),%ax
	movl	%eax,bdb_dbg_ljmp+1-KERNBASE	# ... immediate offset ...
	movl	8+2(%esi),%eax
	movw	%ax,bdb_dbg_ljmp+5-KERNBASE	# ... and selector for ljmp
	movl	24+4(%esi),%eax		# same for bpt descriptor
	movw	24(%esi),%ax
	movl	%eax,bdb_bpt_ljmp+1-KERNBASE
	movl	24+2(%esi),%eax
	movw	%ax,bdb_bpt_ljmp+5-KERNBASE

	movl	$(_idt-KERNBASE),%edi
	movl	%edi,6+2(%esp)
	movl	$8*4/4,%ecx
	rep				# copy idt
	movsl

	lgdt	(%esp)
	lidt	6(%esp)

	addl	$2*6,%esp
	popal

1:
#endif /* BDB */

	/* Load base of page directory and enable mapping. */
	movl	%esi,%eax		# phys address of ptd in proc 0
	movl	%eax,%cr3		# load ptd addr into mmu
	movl	%cr0,%eax		# get control word
	orl	$(CR0_PE|CR0_PG),%eax	# enable paging
	movl	%eax,%cr0		# and let's page NOW!

	pushl	$begin			# jump to high mem
	ret

begin:	/* Now running relocated at KERNBASE. */

	/* Relocate atdevbase. */
	.globl	_Crtat
	movl	_Crtat,%eax
	subl	$(KERNBASE|IOM_BEGIN),%eax
	movl	_atdevphys,%edx		# get pte PA
	subl	_KPTphys,%edx		# remove base of ptes; have phys offset
	shll	$(PGSHIFT-2),%edx	# corresponding to virt offset
	addl	$(KERNBASE),%edx	# add virtual base
	movl	%edx,_atdevbase
	addl	%eax,%edx
	movl	%edx,_Crtat

	/* Set up bootstrap stack. */
	movl	$(_kstack+UPAGES*NBPG-4*12),%esp # bootstrap stack end location
	xorl	%eax,%eax		# mark end of frames
	movl	%eax,%ebp
	movl	_proc0paddr,%eax
	movl	%esi,PCB_CR3(%eax)

#ifdef BDB
	cmpl	$0,_bdb_exists
	jz	1f

	/* relocate debugger gdt entries */
	movl	$(_gdt+8*9),%eax	# adjust slots 9-17
	movl	$9,%ecx
reloc_gdt:
	movb	$(KERNBASE>>24),7(%eax)	# top byte of base addresses, was 0,
	addl	$8,%eax			# now KERNBASE>>24
	loop	reloc_gdt

	int	$3
1:
#endif /* BDB */

	leal	(TABLESIZE)(%esi),%esi	# skip past stack and page tables
	pushl	%esi
	call	_init386		# wire 386 chip for unix operation
	addl	$4,%esp

	movl	$0,_PTD

	/*
	 * Set up the initial stack frame for execve() to munge.
	 */
	.globl	__ucodesel,__udatasel
	movl	__ucodesel,%eax
	movl	__udatasel,%ecx
	pushl	%ecx			# user ss
	pushl	$0xdeadbeef		# user esp (set by execve)
	pushl	$PSL_USERSET		# user eflags
	pushl	%eax			# user cs
	pushl	$0xdeadbeef		# user eip (set by execve)
	subl	$40,%esp		# error code, trap number, registers
	pushl	%ecx			# user ds
	pushl	%ecx			# user es
	movl	%cx,%fs			# user fs (not used)
	movl	%cx,%gs			# user gs (not used)

	movl	%esp,%eax		# push pointer to frame
	pushl	%eax
	call 	_main
	addl	$4,%esp

#if defined(I486_CPU) || defined(I586_CPU)
	/*
	 * Now we've run main() and determined what CPU type we are, so we can
	 * enable ring 0 write protection and outer ring alignment checking on
	 * i486 CPUs and above.
	 */
	cmpl	$CPUCLASS_386,_cpu_class
	je	1f
	movl	%cr0,%eax
	orl	$CR0_WP|CR0_AM,%eax
	movl	%eax,%cr0
1:
#endif

	INTRFASTEXIT
	/* NOTREACHED */

/*****************************************************************************/

/*
 * Signal trampoline; copied to top of user stack.
 */
ENTRY(sigcode)
	call	SIGF_HANDLER(%esp)
	leal	SIGF_SC(%esp),%eax	# scp (the call may have clobbered the
					# copy at SIGF_SCP(%esp))
	pushl	%eax
	pushl	%eax			# junk to fake return address
	movl	$SYS_sigreturn,%eax
	int	$0x80	 		# enter kernel with args on stack
	movl	$SYS_exit,%eax
	int	$0x80			# exit if sigreturn fails
	.globl	_esigcode
_esigcode:

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
 */
ENTRY(bcopy)
ALTENTRY(ovbcopy)
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%esi
	movl	16(%esp),%edi
	movl	20(%esp),%ecx
	cmpl	%esi,%edi		# potentially overlapping? */
	jnb	1f
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
	movl	_curpcb,%eax
	movl	$_copy_fault,PCB_ONFAULT(%eax)
	
	movl	12(%esp),%esi
	movl	16(%esp),%edi
	movl	20(%esp),%eax

	/*
	 * We check that the end of the destination buffer is not past the end
	 * of the user's address space.  If it's not, then we only need to
	 * check that each page is writable.  The 486 will do this for us; the
	 * 386 will not.  (We assume that pages in user space that are not
	 * writable by the user are not writable by the kernel either.)
	 */
	movl	%edi,%edx
	addl	%eax,%edx
	jc	_copy_fault
	cmpl	$VM_MAXUSER_ADDRESS,%edx
	ja	_copy_fault

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU)
	cmpl	$CPUCLASS_386,_cpu_class
	jne	3f
#endif /* I486_CPU || I586_CPU */

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

1:	/* Check PTE for each page. */
	testb	$PG_RW,_PTmap(,%edi,4)
	jz	2f
	
4:	incl	%edi
	decl	%ecx
	jns	1b

	movl	16(%esp),%edi
	jmp	3f
	
2:	/* Simulate a trap. */
	pushl	%eax
	pushl	%ecx
	movl	%edi,%eax
	shll	$PGSHIFT,%eax
	pushl	%eax
	call	_trapwrite		# trapwrite(addr)
	addl	$4,%esp			# pop argument
	popl	%ecx
	testl	%eax,%eax		# if not ok, return EFAULT
	popl	%eax
	jz	4b
	jmp	_copy_fault
#endif /* I386_CPU */

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
	xorl	%eax,%eax

	popl	%edi
	popl	%esi
	movl	_curpcb,%edx
	movl	%eax,PCB_ONFAULT(%edx)
	ret

/*
 * copyin(caddr_t from, caddr_t to, size_t len);
 * Copy len bytes from the user's address space.
 */
ENTRY(copyin)
	pushl	%esi
	pushl	%edi
	movl	_curpcb,%eax
	movl	$_copy_fault,PCB_ONFAULT(%eax)
	
	movl	12(%esp),%esi
	movl	16(%esp),%edi
	movl	20(%esp),%eax

	/*
	 * We check that the end of the destination buffer is not past the end
	 * of the user's address space.  If it's not, then we only need to
	 * check that each page is readable, and the CPU will do that for us.
	 */
	movl	%esi,%edx
	addl	%eax,%edx
	jc	_copy_fault
	cmpl	$VM_MAXUSER_ADDRESS,%edx
	ja	_copy_fault

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
	xorl	%eax,%eax

	popl	%edi
	popl	%esi
	movl	_curpcb,%edx
	movl	%eax,PCB_ONFAULT(%edx)
	ret

ENTRY(copy_fault)
	popl	%edi
	popl	%esi
	movl	_curpcb,%edx
	movl	$0,PCB_ONFAULT(%edx)
	movl	$EFAULT,%eax
	ret

/*
 * copyoutstr(caddr_t from, caddr_t to, size_t maxlen, size_t int *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long, into the
 * user's address space.  Return the number of characters copied (including the
 * NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG; else
 * return 0 or EFAULT.
 */
ENTRY(copyoutstr)
	pushl	%esi
	pushl	%edi
	movl	_curpcb,%ecx
	movl	$_copystr_fault,PCB_ONFAULT(%ecx)

	movl	12(%esp),%esi		# esi = from
	movl	16(%esp),%edi		# edi = to
	movl	20(%esp),%edx		# edx = maxlen

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU)
	cmpl	$CPUCLASS_386,_cpu_class
	jne	5f
#endif /* I486_CPU || I586_CPU */

	/* Compute number of bytes in first page. */
	movl	%edi,%eax
	andl	$PGOFSET,%eax
	movl	$NBPG,%ecx
	subl	%eax,%ecx		# ecx = NBPG - (src % NBPG)

	/* Compute PTE offset for start address. */
	movl	%edi,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address

1:	/*
	 * Once per page, check that we are still within the bounds of user
	 * space, and check for a write fault.
	 */
	cmpl	$VM_MAXUSER_ADDRESS,%edi
	jae	_copystr_fault

	testb	$PG_RW,_PTmap(,%eax,4)
	jnz	2f

	/* Simulate a trap. */
	pushl	%eax
	pushl	%edx
	pushl	%edi
	call	_trapwrite		# trapwrite(addr)
	addl	$4,%esp			# clear argument from stack
	popl	%edx
	testl	%eax,%eax
	popl	%eax
	jnz	_copystr_fault

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
	incl	%eax
	testl	%edx,%edx
	jnz	1b

	/* edx is zero -- return ENAMETOOLONG. */
	movl	$ENAMETOOLONG,%eax
	jmp	copystr_return
#endif /* I386_CPU */

#if defined(I486_CPU) || defined(I586_CPU)
5:	/*
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
	jae	_copystr_fault
	movl	$ENAMETOOLONG,%eax
	jmp	copystr_return
#endif /* I486_CPU || I586_CPU */

/*
 * copyinstr(caddr_t from, caddr_t to, size_t maxlen, size_t int *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long, from the
 * user's address space.  Return the number of characters copied (including the
 * NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG; else
 * return 0 or EFAULT.
 */
ENTRY(copyinstr)
	pushl	%esi
	pushl	%edi
	movl	_curpcb,%ecx
	movl	$_copystr_fault,PCB_ONFAULT(%ecx)

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
	jae	_copystr_fault
	movl	$ENAMETOOLONG,%eax
	jmp	copystr_return

ENTRY(copystr_fault)
	movl	$EFAULT,%eax

copystr_return:	
	/* Set *lencopied and return %eax. */
	movl	_curpcb,%ecx
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
 * copystr(caddr_t from, caddr_t to, size_t maxlen, size_t int *lencopied);
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
ALTENTRY(fuiword)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-4,%edx
	ja	_fusufault
	movl	_curpcb,%ecx
	movl	$_fusufault,PCB_ONFAULT(%ecx)
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
	ja	_fusufault
	movl	_curpcb,%ecx
	movl	$_fusufault,PCB_ONFAULT(%ecx)
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
	ja	_fusubail
	movl	_curpcb,%ecx
	movl	$_fusubail,PCB_ONFAULT(%ecx)
	movzwl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret
	
/*
 * fubyte(caddr_t uaddr);
 * Fetch a byte from the user's address space.
 */
ENTRY(fubyte)
ALTENTRY(fuibyte)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-1,%edx
	ja	_fusufault
	movl	_curpcb,%ecx
	movl	$_fusufault,PCB_ONFAULT(%ecx)
	movzbl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret

/*
 * Handle faults from [fs]u*().  Clean up and return -1.
 */
ENTRY(fusufault)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx)
	decl	%eax
	ret

/*
 * Handle faults from [fs]u*().  Clean up and return -1.  This differs from
 * fusufault() in that trap() will recognize it and return immediately rather
 * than trying to page fault.
 */
ENTRY(fusubail)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx)
	decl	%eax
	ret

/*
 * suword(caddr_t uaddr, int x);
 * Store an int in the user's address space.
 */
ENTRY(suword)
ALTENTRY(suiword)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-4,%edx
	ja	_fusufault
	movl	_curpcb,%ecx
	movl	$_fusufault,PCB_ONFAULT(%ecx)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU)
	cmpl	$CPUCLASS_386,_cpu_class
	jne	2f
#endif /* I486_CPU || I586_CPU */

	movl	%edx,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address
	testb	$PG_RW,_PTmap(,%eax,4)
	jnz	1f

	/* Simulate a trap. */
	pushl	%edx
	pushl	%edx
	call	_trapwrite		# trapwrite(addr)
	addl	$4,%esp			# clear parameter from the stack
	popl	%edx
	movl	_curpcb,%ecx
	testl	%eax,%eax
	jnz	_fusufault

1:	/* XXX also need to check the following 3 bytes for validity! */
#endif

2:	movl	8(%esp),%eax
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
	ja	_fusufault
	movl	_curpcb,%ecx
	movl	$_fusufault,PCB_ONFAULT(%ecx)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU)
	cmpl	$CPUCLASS_386,_cpu_class
	jne	2f
#endif /* I486_CPU || I586_CPU */

	movl	%edx,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address
	testb	$PG_RW,_PTmap(,%eax,4)
	jnz	1f

	/* Simulate a trap. */
	pushl	%edx
	pushl	%edx
	call	_trapwrite		# trapwrite(addr)
	addl	$4,%esp			# clear parameter from the stack
	popl	%edx
	movl	_curpcb,%ecx
	testl	%eax,%eax
	jnz	_fusufault

1:	/* XXX also need to check the following byte for validity! */
#endif

2:	movl	8(%esp),%eax
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
	ja	_fusubail
	movl	_curpcb,%ecx
	movl	$_fusubail,PCB_ONFAULT(%ecx)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU)
	cmpl	$CPUCLASS_386,_cpu_class
	jne	2f
#endif /* I486_CPU || I586_CPU */

	movl	%edx,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address
	testb	$PG_RW,_PTmap(,%eax,4)
	jnz	1f

	/* Simulate a trap. */
	jmp	_fusubail

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
ALTENTRY(suibyte)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-1,%edx
	ja	_fusufault
	movl	_curpcb,%ecx
	movl	$_fusufault,PCB_ONFAULT(%ecx)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU)
	cmpl	$CPUCLASS_386,_cpu_class
	jne	2f
#endif /* I486_CPU || I586_CPU */

	movl	%edx,%eax
	shrl	$PGSHIFT,%eax		# calculate pte address
	testb	$PG_RW,_PTmap(,%eax,4)
	jnz	1f

	/* Simulate a trap. */
	pushl	%edx
	pushl	%edx
	call	_trapwrite		# trapwrite(addr)
	addl	$4,%esp			# clear parameter from the stack
	popl	%edx
	movl	_curpcb,%ecx
	testl	%eax,%eax
	jnz	_fusufault

1:
#endif

2:	movb	8(%esp),%al
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
ENTRY(lgdt)
	/* Reload the descriptor table. */
	movl	4(%esp),%eax
	lgdt	(%eax)
	/* Flush the prefetch q. */
	jmp	1f
	nop
1:	/* Reload "stale" selectors. */
	movl	$KDSEL,%eax
	movl	%ax,%ds
	movl	%ax,%es
	movl	%ax,%ss
	/* Reload code selector by doing intersegment return. */
	popl	%eax
	pushl	$KCSEL
	pushl	%eax
	lret

ENTRY(rtcin)
	movb	4(%esp),%al
	outb	%al,$0x70
	xorl	%eax,%eax		# clear eax
	inb	$0x71,%al
	ret

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
	.globl	_whichqs,_qs,_cnt,_panic

/*
 * setrunqueue(struct proc *p);
 * Insert a process on the appropriate queue.  Should be called at splclock().
 */
ENTRY(setrunqueue)
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
	btsl	%edx,_whichqs		# set q full bit
	leal	_qs(,%edx,8),%edx	# locate q hdr
	movl	P_BACK(%edx),%ecx
	movl	%edx,P_FORW(%eax)	# link process on tail of q
	movl	%eax,P_BACK(%edx)
	movl	%eax,P_FORW(%ecx)
	movl	%ecx,P_BACK(%eax)
	ret
#ifdef DIAGNOSTIC
1:	pushl	$2f
	call	_panic
	/*NOTREACHED*/
2:	.asciz	"setrunqueue"
#endif /* DIAGNOSTIC */

/*
 * remrq(struct proc *p);
 * Remove a process from its queue.  Should be called at splclock().
 */
ENTRY(remrq)
	movl	4(%esp),%ecx
	movzbl	P_PRIORITY(%ecx),%eax
#ifdef DIAGNOSTIC
	shrl	$2,%eax
	btl	%eax,_whichqs
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
	btrl	%eax,_whichqs		# no; clear bit
2:	ret
#ifdef DIAGNOSTIC
1:	pushl	$3f
	call	_panic
	/*NOTREACHED*/
3:	.asciz	"remrq"
#endif /* DIAGNOSTIC */

/*
 * When no processes are on the runq, cpu_switch() branches to here to wait for
 * something to come ready.
 */
ENTRY(idle)
	cli
	movl	_whichqs,%ecx
	testl	%ecx,%ecx
	jnz	sw1
	sti
	hlt
	jmp	_idle

#ifdef DIAGNOSTIC
ENTRY(switch_error)
	pushl	$1f
	call	_panic
	/*NOTREACHED*/
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
	pushl	_cpl

	movl	_curproc,%esi

	/*
	 * Clear curproc so that we don't accumulate system time while idle.
	 * This also insures that schedcpu() will move the old process to
	 * the correct queue if it happens to get called from the spllower()
	 * below and changes the priority.  (See corresponding comment in
	 * userret()).
	 */
	movl	$0,_curproc

	movl	$0,_cpl			# spl0()
	call	_spllower		# process pending interrupts

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
	movl	_whichqs,%ecx

sw1:	bsfl	%ecx,%ebx		# find a full q
	jz	_idle			# if none, idle

	leal	_qs(,%ebx,8),%eax	# select q

	movl	P_FORW(%eax),%edi	# unlink from front of process q
#ifdef	DIAGNOSTIC
	cmpl	%edi,%eax		# linked to self (i.e. nothing queued)?
	je	_switch_error		# not possible
#endif /* DIAGNOSTIC */
	movl	P_FORW(%edi),%edx
	movl	%edx,P_FORW(%eax)
	movl	%eax,P_BACK(%edx)

	cmpl	%edx,%eax		# q empty?
	jne	3f

	btrl	%ebx,%ecx		# yes, clear to indicate empty
	movl	%ecx,_whichqs		# update q status

3:	/* We just did it. */
	xorl	%eax,%eax
	movl	%eax,_want_resched

#ifdef	DIAGNOSTIC
	cmpl	%eax,P_WCHAN(%edi)	# Waiting for something?
	jne	_switch_error		# Yes; shouldn't be queued.
	cmpb	$SRUN,P_STAT(%edi)	# In run state?
	jne	_switch_error		# No; shouldn't be queued.
#endif /* DIAGNOSTIC */

	/* Isolate process.  XXX Is this necessary? */
	movl	%eax,P_BACK(%edi)

	/* Record new process. */
	movl	%edi,_curproc

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
	 *   %eax - 0
	 *   %ecx - scratch
	 *   %esi - old process, then old pcb
	 *   %edi - new process
	 */

#if NNPX > 0
	/* Have we used fp, and need a save? */
	cmpl	%esi,_npxproc
	jne	1f

	/* Just turn off FPU for now; npxdna() will save the state later. */
	smsw	%cx
	orb	$CR0_TS,%cl
	lmsw	%cx
1:
#endif

	movl	P_ADDR(%esi),%esi

	movl	%esp,PCB_ESP(%esi)
	movl	%ebp,PCB_EBP(%esi)
	movl	%fs,%cx
	movl	%ecx,PCB_FS(%esi)
	movl	%gs,%cx
	movl	%ecx,PCB_GS(%esi)

switch_exited:
	/*
	 * Third phase: restore saved context.
	 *
	 * Registers:
	 *   %eax - 0
	 *   %ecx - scratch
	 *   %esi - new pcb
	 *   %edi - new process
	 */

	/* No interrupts while loading new state. */
	cli
	movl	P_ADDR(%edi),%esi

	/* Switch address space. */
	movl	PCB_CR3(%esi),%ecx
	movl	%ecx,%cr3

	/* Restore stack. */
	movl	PCB_ESP(%esi),%esp
	movl	PCB_EBP(%esi),%ebp

#ifdef USER_LDT
	cmpl	%eax,PCB_USERLDT(%esi)
	jnz	1f
	movl	__default_ldt,%ecx
	cmpl	_currentldt,%ecx
	je	2f
	lldt	%ecx
	movl	%ecx,_currentldt
	jmp	2f
1:	pushl	%esi
	call	_set_user_ldt
	addl	$4,%esp
2:
#endif /* USER_LDT */

	/* Restore segments. */
	movl	PCB_FS(%esi),%ecx
	movl	%cx,%fs
	movl	PCB_GS(%esi),%ecx
	movl	%cx,%gs

	/* Record new pcb. */
	movl	%esi,_curpcb

	/* Interrupts are okay again. */
	sti

switch_return:
	/*
	 * Restore old cpl from stack.  Note that this is always an increase,
	 * due to the spl0() on entry.
	 */
	popl	_cpl

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
	.globl	_proc0,_vmspace_free,_kernel_map,_kmem_free
ENTRY(switch_exit)
	movl	4(%esp),%edi		# old process
	movl	$_proc0,%ebx

	/* In case we fault... */
	movl	$0,_curproc

#if NNPX > 0
	/* Have we used fp, and need a save? */
	cmpl	%edi,_npxproc
	jne	1f

	/* Just turn off FPU and forget npxproc. */
	smsw	%cx
	orb	$CR0_TS,%cl
	lmsw	%cx
	movl	$0,_npxproc
1:
#endif

	/* Restore proc0's context. */
	cli
	movl	P_ADDR(%ebx),%esi

	/* Switch address space. */
	movl	PCB_CR3(%esi),%ecx
	movl	%ecx,%cr3

	/* Can't have a user-set ldt. */

	/* Restore stack and segments. */
	movl	PCB_ESP(%esi),%esp
	movl	PCB_EBP(%esi),%ebp
	xorl	%ecx,%ecx		# always null in proc0
	movl	%cx,%fs
	movl	%cx,%gs

	/* Record new pcb. */
	movl	%esi,_curpcb

	/* Interrupts are okay again. */
	sti

	/* Thoroughly nuke the old process's resources. */
	pushl	P_VMSPACE(%edi)
	call	_vmspace_free
	pushl	$(UPAGES << PGSHIFT)
	pushl	P_ADDR(%edi)
	pushl	_kernel_map
	call	_kmem_free
	addl	$16,%esp

	/* Jump into cpu_switch() with the right state. */
	movl	%ebx,%esi
	movl	$0,_curproc
	jmp	switch_search

/*
 * savectx(struct pcb *pcb, int altreturn);
 * Update pcb, saving current processor state and arranging for alternate
 * return in cpu_switch() if altreturn is true.
 */
ENTRY(savectx)
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	_cpl

	/* Save the context. */
	movl	20(%esp),%esi		# esi = p2->p_addr

	movl	%esp,PCB_ESP(%esi)
	movl	%ebp,PCB_EBP(%esi)
	movl	%fs,%cx
	movl	%ecx,PCB_FS(%esi)
	movl	%gs,%cx
	movl	%ecx,PCB_GS(%esi)

	/* Copy the stack if requested. */
	cmpl	$0,24(%esp)
	je	1f
	movl	%esp,%eax		# eax = stack pointer
	movl	%eax,%edx		# edx = stack offset from bottom
	subl	$_kstack,%edx
	movl	$(UPAGES << PGSHIFT),%ecx	# ecx = ctob(UPAGES) - offset
	subl	%edx,%ecx
	pushl	%ecx
	addl	%edx,%esi		# esi = stack in p2
	pushl	%esi
	pushl	%eax
	call	_bcopy
	addl	$12,%esp
	
1:	/* This is the parent.  The child will return from cpu_switch(). */
	xorl	%eax,%eax		# return 0

	addl	$4,%esp			# drop saved _cpl on the floor
	popl	%edi
	popl	%esi
	popl	%ebx
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
#define	IDTVEC(name)	ALIGN_TEXT; .globl _X/**/name; _X/**/name:

#define	TRAP(a)		pushl $(a) ; jmp _alltraps
#define	ZTRAP(a)	pushl $0 ; TRAP(a)
#ifdef KGDB
#define	BPTTRAP(a)	testb $(PSL_I>>8),13(%esp) ; jz 1f ; sti ; 1: ; \
			pushl $(a) ; jmp _bpttraps
#else
#define	BPTTRAP(a)	testb $(PSL_I>>8),13(%esp) ; jz 1f ; sti ; 1: ; \
			TRAP(a)
#endif

	.text
IDTVEC(div)
	ZTRAP(T_DIVIDE)
IDTVEC(dbg)
#ifdef BDB
	BDBTRAP(dbg)
#endif /* BDB */
	subl	$4,%esp
	pushl	%eax
/*	movl	%dr6,%eax		# XXX stupid assembler! */
	.byte	0x0f, 0x21, 0xf0
	movl	%eax,4(%esp)
	andb	$~15,%al
/*	movl	%eax,%dr6		# XXX stupid assembler! */
	.byte	0x0f, 0x23, 0xf0
	popl	%eax
	BPTTRAP(T_TRCTRAP)
IDTVEC(nmi)
	ZTRAP(T_NMI)
IDTVEC(bpt)
#ifdef BDB
	BDBTRAP(bpt)
#endif /* BDB */
	pushl	$0
	BPTTRAP(T_BPTFLT)
IDTVEC(ofl)
	ZTRAP(T_OFLOW)
IDTVEC(bnd)
	ZTRAP(T_BOUND)
IDTVEC(ill)
	ZTRAP(T_PRIVINFLT)
IDTVEC(dna)
#if NNPX > 0
	pushl	$0			# dummy error code
	pushl	$T_DNA
	INTRENTRY
	call	_npxdna
	testl	%eax,%eax
	jz	calltrap
	INTRFASTEXIT
#else
	ZTRAP(T_DNA)
#endif
IDTVEC(dble)
	TRAP(T_DOUBLEFLT)
IDTVEC(fpusegm)
	ZTRAP(T_FPOPFLT)
IDTVEC(tss)
	TRAP(T_TSSFLT)
IDTVEC(missing)
	TRAP(T_SEGNPFLT)
IDTVEC(stk)
	TRAP(T_STKFLT)
IDTVEC(prot)
	TRAP(T_PROTFLT)
IDTVEC(page)
	TRAP(T_PAGEFLT)
IDTVEC(rsvd)
	ZTRAP(T_RESERVED)
IDTVEC(fpu)
#if NNPX > 0
	/*
	 * Handle like an interrupt so that we can call npxintr to clear the
	 * error.  It would be better to handle npx interrupts as traps but
	 * this is difficult for nested interrupts.
	 */
	pushl	$0			# dummy error code
	pushl	$T_ASTFLT
	INTRENTRY
	pushl	_cpl
	pushl	%esp
	incl	_cnt+V_TRAP
	call	_npxintr
	addl	$4,%esp
	INTREXIT
#else
	ZTRAP(T_ARITHTRAP)
#endif
IDTVEC(align)
	ZTRAP(T_ALIGNFLT)
	/* 18 - 31 reserved for future exp */

/*
 * If an error is detected during trap, syscall, or interrupt exit, trap() will
 * change %eip to point to one of these labels.  We clean up the stack, if
 * necessary, and resume as if we were handling a general protection fault.
 * This will cause the process to get a SIGBUS.
 */
ENTRY(resume_iret)
	ZTRAP(T_PROTFLT)
ENTRY(resume_pop_ds)
	movl	$KDSEL,%eax
	movl	%ax,%es
ENTRY(resume_pop_es)
	movl	$T_PROTFLT,TF_TRAPNO(%esp)
	jmp	calltrap

ENTRY(alltraps)
	INTRENTRY
calltrap:
#ifdef DIAGNOSTIC
	movl	_cpl,%ebx
#endif /* DIAGNOSTIC */
	call	_trap
2:	/* Check for ASTs on exit to user mode. */
	cli
	cmpb	$0,_astpending
	je	1f
	testb	$SEL_RPL_MASK,TF_CS(%esp)
	jz	1f
	movb	$0,_astpending
	sti
	movl	$T_ASTFLT,TF_TRAPNO(%esp)
	call	_trap
#ifndef DIAGNOSTIC
1:	INTRFASTEXIT
#else /* DIAGNOSTIC */
1:	cmpl	_cpl,%ebx
	jne	3f
	INTRFASTEXIT
3:	sti
	pushl	$4f
	call	_printf
	addl	$4,%esp
#ifdef DDB
	int	$3
#endif /* DDB */
	movl	%ebx,_cpl
	jmp	2b
4:	.asciz	"WARNING: SPL NOT LOWERED ON TRAP EXIT\n"
#endif /* DIAGNOSTIC */

#ifdef KGDB
/*
 * This code checks for a kgdb trap, then falls through
 * to the regular trap code.
 */
ENTRY(bpttraps)
	INTRENTRY
	testb	$SEL_RPL_MASK,TF_CS(%esp)
	jne	calltrap
	call	_kgdb_trap_glue		
	jmp	calltrap
#endif /* KGDB */

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
	movl	_cpl,%ebx
#endif /* DIAGNOSTIC */
	call	_syscall
2:	/* Check for ASTs on exit to user mode. */
	cli
	cmpb	$0,_astpending
	je	1f
	/* Always returning to user mode here. */
	movb	$0,_astpending
	sti
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	call	_trap
#ifndef DIAGNOSTIC
1:	INTRFASTEXIT
#else /* DIAGNOSTIC */
1:	cmpl	_cpl,%ebx
	jne	3f
	INTRFASTEXIT
3:	sti
	pushl	$4f
	call	_printf
	addl	$4,%esp
#ifdef DDB
	int	$3
#endif /* DDB */
	movl	%ebx,_cpl
	jmp	2b
4:	.asciz	"WARNING: SPL NOT LOWERED ON SYSCALL EXIT\n"
#endif /* DIAGNOSTIC */

#include <i386/isa/vector.s>
#include <i386/isa/icu.s>

/*
 * bzero (void *b, size_t len)
 *	write len zero bytes to the string b.
 */

ENTRY(bzero)
	pushl	%edi
	pushl	%ebx
	movl	12(%esp),%edi
	movl	16(%esp),%ecx

	cld				/* set fill direction forward */
	xorl	%eax,%eax		/* set fill data to 0 */

	/*
	 * if the string is too short, it's really not worth the overhead
	 * of aligning to word boundries, etc.  So we jump to a plain
	 * unaligned set.
	 */
	cmpl	$0x0f,%ecx
	jle	9f

	movl	%edi,%edx		/* compute misalignment */
	negl	%edx
	andl	$3,%edx
	movl	%ecx,%ebx
	subl	%edx,%ebx

	movl	%edx,%ecx		/* zero until word aligned */
	rep
	stosb

#if defined(I486_CPU)
#if defined(I386_CPU) || defined(I586_CPU)
	cmpl	$CPUCLASS_486,_cpu_class
	jne	8f
#endif

	movl	%ebx,%ecx
	shrl	$6,%ecx
	jz	8f
	andl	$63,%ebx
1:	movl	%eax,(%edi)
	movl	%eax,4(%edi)
	movl	%eax,8(%edi)
	movl	%eax,12(%edi)
	movl	%eax,16(%edi)
	movl	%eax,20(%edi)
	movl	%eax,24(%edi)
	movl	%eax,28(%edi)
	movl	%eax,32(%edi)
	movl	%eax,36(%edi)
	movl	%eax,40(%edi)
	movl	%eax,44(%edi)
	movl	%eax,48(%edi)
	movl	%eax,52(%edi)
	movl	%eax,56(%edi)
	movl	%eax,60(%edi)
	addl	$64,%edi
	decl	%ecx
	jnz	1b
#endif

8:	movl	%ebx,%ecx		/* zero by words */
	shrl	$2,%ecx
	andl	$3,%ebx
	rep
	stosl

7:	movl	%ebx,%ecx		/* zero remainder bytes */
9:	rep
	stosb

	popl	%ebx
	popl	%edi
	ret
