/*-
 * Copyright (c) 1993 Charles Hannum.
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
 *	from: @(#)locore.s	7.3 (Berkeley) 5/13/91
 *	$Id: locore.s,v 1.28.2.29 1993/11/13 06:25:46 mycroft Exp $
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
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/specialreg.h>
#include <machine/trap.h>

#include <i386/isa/debug.h>
#include <i386/isa/isa.h>

#define	KDSEL		0x10
#define	SEL_RPL_MASK	0x0003

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

#define	ENTRY(name)	.globl _/**/name; ALIGN_TEXT; _/**/name:
#define	ALTENTRY(name)	.globl _/**/name; _/**/name:

/*
 * Initialization
 */
	.data
        .globl	_esym
_esym:	.long	0		# ptr to end of syms

	.globl	_cpu,_cold,_boothowto,_bootdev,_cyloffset,_atdevbase,_proc0paddr,_curpcb
_cpu:	.long	0		# are we 386, 386sx, or 486
_cold:	.long	1		# cold till we are not
_atdevbase:	.long	0	# location of start of iomem in virtual
_atdevphys:	.long	0	# location of device mapping ptes (phys)
_cyloffset:	.long	0
_proc0paddr:	.long	0

	.globl	_IdlePTD,_KPTphys
_IdlePTD:	.long	0
_KPTphys:	.long	0

	.space 512
tmpstk:
	.text
	.globl	start
start:	jmp	1f
	.space	0x500

1:	movw	$0x1234,0x472	# warm boot

	/*
	 * pass parameters on stack (howto, bootdev, unit, cyloffset, esym)
	 * note: (%esp) is return address of boot
	 * ( if we want to hold onto /boot, it's physical %esp up to _end)
	 */
	movl	4(%esp),%eax
	movl	%eax,_boothowto-KERNBASE
	movl	8(%esp),%eax
	movl	%eax,_bootdev-KERNBASE
	movl	12(%esp),%eax
	movl	%eax,_cyloffset-KERNBASE
 	movl	16(%esp),%eax
 	addl	$(KERNBASE),%eax
 	movl	%eax,_esym-KERNBASE

	/* find out our CPU type. */
	/* first, clear the alignment check and identification flags */
	pushfl
	popl	%eax
	andl	$~(PSL_AC|PSL_ID),%eax
	pushl	%eax
	popfl
	
	/* try to frob alignment check flag; does not exist on 386 */
	pushfl
	popl	%eax
	movl	%eax,%ecx
	orl	$(PSL_AC),%eax
	pushl	%eax
	popfl
	pushfl
	popl	%eax
	xorl	%ecx,%eax
	andl	$(PSL_AC),%eax
	pushl	%ecx
	popfl
      
	testl	%eax,%eax
	jnz	1f
	movl    $(CPU_386),_cpu-KERNBASE
	jmp	2f
	
1:	/* try to frob identification flag; does not exist on 486 */
	pushfl
	popl	%eax
	movl	%eax,%ecx
	xorl	$(PSL_ID),%eax
	pushl	%eax
	popfl
	pushfl
	popl	%eax
	xorl	%ecx,%eax
	andl	$(PSL_ID),%eax
	pushl	%ecx
	popfl

	testl	%eax,%eax
	jnz	1f
	movl	$(CPU_486),_cpu-KERNBASE
	jmp	2f

1:	movl    $(CPU_586),_cpu-KERNBASE
2:

	/*
	 * Finished with old stack; load new %esp now instead of later so
	 * we can trace this code without having to worry about the trace
	 * trap clobbering the memory test or the zeroing of the bss+bootstrap
	 * page tables.
	 *
	 * XXX - wdboot clears the bss after testing that this is safe.
	 * This is too wasteful - memory below 640K is scarce.  The boot
	 * program should check:
	 *	text+data <= &stack_variable - more_space_for_stack
	 *	text+data+bss+pad+space_for_page_tables <= end_of_memory
	 * Oops, the gdt is in the carcass of the boot program so clearing
	 * the rest of memory is still not possible.
	 */
	movl	$(tmpstk-KERNBASE),%esp	# bootstrap stack end location

/*
 * Virtual address space of kernel:
 *
 *	text | data | bss | [syms] | page dir | usr stk map | proc0 kernel stack | Sysmap
 *			           0          1             2          3         4
 */
/* clear bss */
	movl	$(_edata-KERNBASE),%edi
	movl	$(((_end-_edata)+3)>>2),%ecx
	xorl	%eax,%eax
	cld
	rep
	stosl

/* find end of kernel image */
	movl	$(_end-KERNBASE),%ecx
#if	defined(DDB) && !defined(SYMTAB_SPACE)
/* save the symbols (if loaded) */
	movl	_esym-KERNBASE,%eax
	testl	%eax,%eax
	jz	1f
	movl	%eax,%ecx
	subl	$(KERNBASE),%ecx
1:
#endif
	movl	%ecx,%edi	# edi= end || esym
	addl	$(PGOFSET),%ecx	# page align up
	andl	$(~PGOFSET),%ecx
	movl	%ecx,%esi	# esi=start of tables
	subl	%edi,%ecx
	addl	$((NKPDE+UPAGES+2)*NBPG),%ecx	# size of tables
	
/* clear memory for bootstrap tables */
	xorl	%eax,%eax	# pattern
	cld
	rep
	stosb

/* physical address of Idle Address space */
	movl	%esi,_IdlePTD-KERNBASE

/*
 * fillkpt
 *	eax = pte (page frame | control | status)
 *	ebx = page table address
 *	ecx = number of pages to map
 */
#define	fillkpt		\
1:	movl	%eax,(%ebx)	; \
	addl	$(NBPG),%eax	; /* increment physical address */ \
	addl	$4,%ebx		; /* next pte */ \
	loop	1b		;

/*
 * Map Kernel
 * N.B. don't bother with making kernel text RO, as 386
 * ignores R/W AND U/S bits on kernel access (only v works) !
 *
 * First step - build page tables
 */
	movl	%esi,%ecx		# this much memory,
	shrl	$(PGSHIFT),%ecx		# for this many pte s
	addl	$(NKPDE+UPAGES+2),%ecx	# including our early context
	movl	$(PG_V|PG_KW),%eax	#  having these bits set,
	lea	((UPAGES+2)*NBPG)(%esi),%ebx	#   physical address of KPT in proc 0,
	movl	%ebx,_KPTphys-KERNBASE	#    in the kernel page table,
	fillkpt

/* map I/O memory */

	movl	$(IOM_SIZE>>PGSHIFT),%ecx	# for this many pte s,
	movl	$(IOM_BEGIN|PG_V|PG_UW|PG_N),%eax	# having these bits set
	movl	%ebx,_atdevphys-KERNBASE	# remember phys addr of ptes
	fillkpt

/* map proc 0's kernel stack into user page table page */

	movl	$(UPAGES),%ecx		# for this many pte s,
	lea	(2*NBPG)(%esi),%eax	# physical address in proc 0
	lea	(KERNBASE)(%eax),%edx
	movl	%edx,_proc0paddr-KERNBASE	# remember VA for 0th process init
	orl	$(PG_V|PG_KW),%eax	#  having these bits set,
	lea	(1*NBPG)(%esi),%ebx	# physical address of stack pt in proc 0
	addl	$((NPTEPD-UPAGES)*4),%ebx
	fillkpt

/*
 * Construct a page table directory
 * (of page directory elements - pde's)
 */
	/* install a pde for temporary double map of kernel text */
	movl	_KPTphys-KERNBASE,%eax	# physical address of kernel page tables
	orl     $(PG_V|PG_UW),%eax	# pde entry is valid
	movl	%eax,(%esi)		# which is where temp maps!

	/* kernel pde's */
	movl	$(NKPDE),%ecx		# for this many pde s,
	lea	(KPTDI*4)(%esi),%ebx	# offset of pde for kernel
	fillkpt

	/* install a pde recursively mapping page directory as a page table! */
	movl	%esi,%eax		# phys address of ptd in proc 0
	orl	$(PG_V|PG_UW),%eax	# pde entry is valid
	movl	%eax,(PTDPTDI*4)(%esi)	# which is where PTmap maps!

	/* install a pde to map kernel stack for proc 0 */
	lea	(1*NBPG)(%esi),%eax	# physical address of pt in proc 0
	orl	$(PG_V|PG_KW),%eax	# pde entry is valid
	movl	%eax,(UPTDI*4)(%esi)	# which is where kernel stack maps!

	/* copy and convert stuff from old gdt and idt for debugger */
#ifdef BDB
	cmpl	$0x0375c339,0x96104	# XXX - debugger signature
	jne	1f
	movb	$1,_bdb_exists-KERNBASE
1:
	pushal
	subl	$2*6,%esp

	sgdt	(%esp)
	movl	2(%esp),%esi		# base address of current gdt
	movl	$_gdt-KERNBASE,%edi
	movl	%edi,2(%esp)
	movl	$8*18/4,%ecx
	rep				# copy gdt
	movsl
	movl	$_gdt-KERNBASE,-8+2(%edi)	# adjust gdt self-ptr
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
#endif

	/* load base of page directory and enable mapping */
	movl	%esi,%eax		# phys address of ptd in proc 0
	movl	%eax,%cr3		# load ptd addr into mmu
	movl	%cr0,%eax		# get control word
	orl	$(CR0_PE|CR0_PG),%eax	# enable paging
	movl	%eax,%cr0		# and let's page NOW!

	pushl	$begin			# jump to high mem
	ret

begin: /* now running relocated at KERNBASE where the system is linked to run */

	/* this is ugly */
	movl	_atdevphys,%edx		# get pte PA
	subl	_KPTphys,%edx		# remove base of ptes; have phys offset
	shll	$(PGSHIFT-2),%edx	# corresponding to virt offset
	addl	$(KERNBASE),%edx	# add virtual base
	movl	%edx,_atdevbase

	/* set up bootstrap stack */
	movl	$(USRSTACK+UPAGES*NBPG-4*12),%esp # bootstrap stack end location
	xorl	%eax,%eax		# mark end of frames
	movl	%eax,%ebp
	movl	_proc0paddr,%eax
	movl	%esi,PCB_CR3(%eax)

#ifdef BDB
	/* relocate debugger gdt entries */
	movl	$(_gdt+8*9),%eax	# adjust slots 9-17
	movl	$9,%ecx
reloc_gdt:
	movb	$0xfe,7(%eax)		# top byte of base addresses, was 0,
	addl	$8,%eax			# now KERNBASE>>24
	loop	reloc_gdt

	cmpl	$0,_bdb_exists
	jz	1f
	int	$3
1:
#endif

	lea	((NKPDE+UPAGES+2)*NBPG)(%esi),%esi	# skip past stack and page tables
	pushl	%esi
	call	_init386		# wire 386 chip for unix operation
	
	movl	$0,_PTD
	call 	_main
	popl	%esi

#if defined(I486_CPU) || defined(I586_CPU)
	/*
	 * now we've run main() and determined what cpu-type we are, we can
	 * enable WP mode on i486 cpus and above.
	 */
	cmpl	$(CPUCLASS_386),_cpu_class
	je	1f			# 386s can't handle WP mode
	movl	%cr0,%eax		# get control word
	orl	$(CR0_WP),%eax		# enable ring 0 Write Protection
	movl	%eax,%cr0
1:
#endif

	.globl	__ucodesel,__udatasel
	movl	__ucodesel,%eax
	movl	__udatasel,%ecx
	# build outer stack frame
	pushl	%ecx		# user ss
	pushl	$(USRSTACK)	# user esp
	pushl	%eax		# user cs
	pushl	$0		# user ip
	movl	%cx,%ds
	movl	%cx,%es
	movl	%ax,%fs		# double map cs to fs
	movl	%cx,%gs		# and ds to gs
	lret			# goto user!

#ifdef DIAGNOSTIC
	pushl	$lretmsg1	/* "should never get here!" */
	call	_panic
lretmsg1:
	.asciz	"lret: toinit\n"
#endif


#define	LCALL(x,y)	.byte 0x9a ; .long y; .word x
/*
 * Icode is copied out to process 1 to exec /etc/init.
 * If the exec fails, process 1 exits.
 */
ENTRY(icode)
	pushl	$0		/* envp for execve() */

#	pushl	$argv-_icode	/* can't do this 'cos gas 1.38 is broken */
	movl	$(argv),%eax
	subl	$(_icode),%eax
	pushl	%eax		/* argp for execve() */

#	pushl	$init-_icode
	movl	$(init),%eax
	subl	$(_icode),%eax
	pushl	%eax		/* fname for execve() */
	pushl	%eax		/* dummy return address */
	movl	$(SYS_execve),%eax
	LCALL(7,0)
	/* exit if something botches up in the above exec */
	movl	$(SYS_exit),%eax
	LCALL(7,0)

init:
	.asciz	"/sbin/init"
	ALIGN_DATA
argv:
	.long	init+6-_icode		# argv[0] = "init" ("/sbin/init" + 6)
	.long	eicode-_icode		# argv[1] follows icode after copyout
	.long	0
eicode:

	.globl	_szicode
_szicode:
	.long	eicode-_icode

ENTRY(sigcode)
	call	SIGF_HANDLER(%esp)
	lea	SIGF_SC(%esp),%eax	# scp (the call may have clobbered the
					# copy at SIGF_SCP(%esp))
	pushl	%eax
	pushl	%eax			# junk to fake return address
	movl	$(SYS_sigreturn),%eax
	LCALL(7,0)			# enter kernel with args on stack
	movl	$(SYS_exit),%eax
	LCALL(7,0)			# exit if sigreturn fails

	.globl	_esigcode
_esigcode:


	/*
	 * fillw (pat,base,cnt)
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
	shrl	%ecx
	rep
	stosl
	movl	16(%esp),%ecx
	andl	$1,%ecx
	rep
	stosw
	popl	%edi
	ret

ENTRY(bcopyb)
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%esi
	movl	16(%esp),%edi
	movl	20(%esp),%ecx
	cmpl	%esi,%edi	/* potentially overlapping? */
	jnb	1f
	cld			/* nope, copy forwards */
	rep
	movsb
	popl	%edi
	popl	%esi
	ret

	ALIGN_TEXT
1:
	addl	%ecx,%edi	/* copy backwards. */
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

ENTRY(bcopyw)
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%esi
	movl	16(%esp),%edi
	movl	20(%esp),%ecx
	cmpl	%esi,%edi	/* potentially overlapping? */
	jnb	1f
	cld			/* nope, copy forwards */
	shrl	$1,%ecx		/* copy by 16-bit words */
	rep
	movsw
	adc	%ecx,%ecx	/* any bytes left? */
	rep
	movsb
	popl	%edi
	popl	%esi
	ret

	ALIGN_TEXT
1:
	addl	%ecx,%edi	/* copy backwards */
	addl	%ecx,%esi
	std
	andl	$1,%ecx		/* any fractional bytes? */
	decl	%edi
	decl	%esi
	rep
	movsb
	movl	20(%esp),%ecx	/* copy remainder by 16-bit words */
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
	 * (ov)bcopy (src,dst,cnt)
	 *  ws@tools.de     (Wolfgang Solfrank, TooLs GmbH) +49-228-985800
	 */
ENTRY(ovbcopy)
ALTENTRY(bcopy)
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%esi
	movl	16(%esp),%edi
	movl	20(%esp),%ecx
	cmpl	%esi,%edi	/* potentially overlapping? */
	jnb	1f
	cld			/* nope, copy forwards */
	shrl	$2,%ecx		/* copy by 32-bit words */
	rep
	movsl
	movl	20(%esp),%ecx
	andl	$3,%ecx		/* any bytes left? */
	rep
	movsb
	popl	%edi
	popl	%esi
	ret

	ALIGN_TEXT
1:
	addl	%ecx,%edi	/* copy backwards */
	addl	%ecx,%esi
	std
	andl	$3,%ecx		/* any fractional bytes? */
	decl	%edi
	decl	%esi
	rep
	movsb
	movl	20(%esp),%ecx	/* copy remainder by 32-bit words */
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
 * copyout() originally written by ws@tools.de, and optimised by
 *	by Christoph Robitschko <chmr@edvz.tu-graz.ac.at>.
 *	Minor modifications by Andrew Herbert <andrew@werple.apana.org.au>
 */
ENTRY(copyout)
	movl	_curpcb,%eax
	movl	$copyout_fault,PCB_ONFAULT(%eax)
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	movl	16(%esp),%esi
	movl	20(%esp),%edi
	movl	24(%esp),%ebx
	orl	%ebx,%ebx	/* anything to do? */
	jz	done_copyout

	pushl	%es

#if defined(I386_CPU)

#if defined(I486_CPU) || defined(I586_CPU)
	cmpl	$(CPUCLASS_386),_cpu_class
	jne	3f	/* fine, we're not a 386 so don't need this stuff */
#endif	/* I486_CPU || I586_CPU */

	/* we have to check each PTE for (write) permission */

			/* compute number of pages */
	movl	%edi,%ecx
	andl	$(PGOFSET),%ecx
	addl	%ebx,%ecx
	decl	%ecx
	shrl	$(PGSHIFT),%ecx
	incl	%ecx

			/* compute PTE offset for start address */
	movl	%edi,%edx
	shrl	$(PGSHIFT),%edx

1:			/* check PTE for each page */
	movb	_PTmap(,%edx,4),%al
	andb	$0x07,%al	/* Pages must be VALID + USERACC + WRITABLE */
	cmpb	$0x05,%al
	jne	2f
				
			/* simulate a trap */
	pushl	%edx
	pushl	%ecx
	shll	$(PGSHIFT),%edx
	pushl	%edx
	call	_trapwrite	/* trapwrite(addr) */
	popl	%edx
	popl	%ecx
	popl	%edx

	orl	%eax,%eax	/* if not ok, return EFAULT */
	jnz	copyout_fault

2:
	addl	$4,%edx
	decl	%ecx
	jnz	1b		/* check next page */

#endif /* I386_CPU */

	/* If WP bit in CR0 is set (n/a on 386), the hardware does the
	 * write check. We just have to load the right segment selector
	 * This is of some use on a 386 too, as it gives us bounds
	 * limiting.
	 */
3:
	movl	__udatasel,%eax
	movl	%ax,%es

			/* bcopy (%esi, %edi, %ebx) */
	cld
	movl	%ebx,%ecx
	shrl	$2,%ecx
	rep
	movsl
	movb	%bl,%cl
	andb	$3,%cl	    /* XXX can we trust the rest of %ecx on clones? */
	rep
	movsb

	popl	%es
done_copyout:
	popl	%ebx
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	movl	_curpcb,%edx
	movl	%eax,PCB_ONFAULT(%edx)
	ret

copyout_fault:
	popl	%es
	popl	%ebx
	popl	%edi
	popl	%esi
	movl	_curpcb,%edx
	movl	$0,PCB_ONFAULT(%edx)
	movl	$(EFAULT),%eax
	ret

ENTRY(copyin)
	movl	_curpcb,%eax
	movl	$copyin_fault,PCB_ONFAULT(%eax)
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	movl	16(%esp),%esi
	movl	20(%esp),%edi
	movl	24(%esp),%ecx
	movl	%ecx,%edx
	pushl	%ds
	movl	__udatasel,%eax	/* use the user segment descriptor */
	movl	%ax,%ds

	shrl	$2,%ecx
	cld
	rep
	movsl
	movb	%dl,%cl
	andb	$3,%cl
	rep
	movsb

	popl	%ds
	popl	%ebx
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	movl	_curpcb,%edx
	movl	%eax,PCB_ONFAULT(%edx)
	ret

copyin_fault:
	popl	%ds
	popl	%ebx
	popl	%edi
	popl	%esi
	movl	_curpcb,%edx
	movl	$0,PCB_ONFAULT(%edx)
	movl	$(EFAULT),%eax
	ret

/*
 * copyoutstr(from, to, maxlen, int *lencopied)
 *	copy a string from from to to, stop when a 0 character is reached.
 *	return ENAMETOOLONG if string is longer than maxlen, and
 *	EFAULT on protection violations. If lencopied is non-zero,
 *	return the actual length in *lencopied.
 *
 *	by Christoph Robitschko <chmr@edvz.tu-graz.ac.at>
 *	with modifications by Andrew Herbert <andrew@werple.apana.org.au>
 */
ENTRY(copyoutstr)
	pushl	%esi
	pushl	%edi
	movl	_curpcb,%ecx
	movl	$copystr_fault,PCB_ONFAULT(%ecx)

	movl	12(%esp),%esi			/* %esi = from */
	movl	16(%esp),%edi			/* %edi = to */
	movl	20(%esp),%edx			/* %edx = maxlen */
	movl	__udatasel,%eax
	movl	%ax,%gs

#if defined(I386_CPU)

#if defined(I486_CPU) || defined(I586_CPU)
	cmpl	$(CPUCLASS_386),_cpu_class
	jne	5f	/* fine, we're not a 386 so don't need this stuff */
#endif	/* I486_CPU || I586_CPU */

1:
	movl	%edi,%eax
	shrl	$(PGSHIFT),%eax
	movb	_PTmap(,%eax,4),%al
	andb	$7,%al
	cmpb	$5,%al
	jne	2f
			/* simulate trap */
	pushl	%edx
	pushl	%edi
	call	_trapwrite
	popl	%edi
	popl	%edx
	orl	%eax,%eax
	jnz	copystr_fault

2:			/* copy up to end of this page */
	movl	%edi,%eax
	andl	$(PGOFSET),%eax
	movl	$(NBPG),%ecx
	subl	%eax,%ecx	/* ecx = NBPG - (src % NBPG) */
	cmpl	%ecx,%edx
	jge	3f
	movl	%edx,%ecx	/* ecx = min (ecx, edx) */
3:
	orl	%ecx,%ecx
	jz	4f
	decl	%ecx
	decl	%edx
	lodsb
	gs
	stosb
	orb	%al,%al
	jnz	3b

			/* Success -- 0 byte reached */
	decl	%edx
	xorl	%eax,%eax
	jmp	7f

4:			/* next page */
	orl	%edx,%edx
	jnz	1b
			/* edx is zero -- return ENAMETOOLONG */
	movl	$(ENAMETOOLONG),%eax
	jmp	7f

#endif /* I386_CPU */

#if defined(I486_CPU) || defined(I586_CPU)
5:
	incl	%edx
1:			/* aren't numeric labels wonderful ;-) */
	decl	%edx
	jz	2f
	lodsb
	gs
	stosb
	orb	%al,%al
	jnz	1b
			/* Success -- 0 byte reached */
	decl	%edx
	xorl	%eax,%eax
	jmp	7f

2:			/* edx is zero -- return ENAMETOOLONG */
	movl	$(ENAMETOOLONG),%eax
	jmp	7f

#endif	/* I486_CPU || I586_CPU */

/*
 * copyinstr(from, to, maxlen, int *lencopied)
 *	copy a string from from to to, stop when a 0 character is reached.
 *	return ENAMETOOLONG if string is longer than maxlen, and
 *	EFAULT on protection violations. If lencopied is non-zero,
 *	return the actual length in *lencopied.
 *
 *	by Christoph Robitschko <chmr@edvz.tu-graz.ac.at>
 */
ENTRY(copyinstr)
	pushl	%esi
	pushl	%edi
	movl	_curpcb,%ecx
	movl	$copystr_fault,PCB_ONFAULT(%ecx)

	movl	12(%esp),%esi			# %esi = from
	movl	16(%esp),%edi			# %edi = to
	movl	20(%esp),%edx			# %edx = maxlen
	movl	__udatasel,%eax
	movl	%ax,%gs
	incl	%edx

1:
	decl	%edx
	jz	4f
	gs
	lodsb
	stosb
	orb	%al,%al
	jnz	1b
			/* Success -- 0 byte reached */
	decl	%edx
	xorl	%eax,%eax
	jmp	7f
4:
			/* edx is zero -- return ENAMETOOLONG */
	movl	$(ENAMETOOLONG),%eax
	jmp	7f

copystr_fault:
	movl	$(EFAULT),%eax
7:		/* set *lencopied and return %eax */
	movl	_curpcb,%ecx
	movl	$0,PCB_ONFAULT(%ecx)
	movl	20(%esp),%ecx
	subl	%edx,%ecx
	movl	24(%esp),%edx
	orl	%edx,%edx
	jz	8f
	movl	%ecx,(%edx)
8:
	popl	%edi
	popl	%esi
	ret


/*
 * copystr(from, to, maxlen, int *lencopied)
 *
 *	by Christoph Robitschko <chmr@edvz.tu-graz.ac.at>
 */
ENTRY(copystr)
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%esi			/* %esi = from */
	movl	16(%esp),%edi			/* %edi = to */
	movl	20(%esp),%edx			/* %edx = maxlen */
	incl	%edx

1:
	decl	%edx
	jz	4f
	lodsb
	stosb
	orb	%al,%al
	jnz	1b
			/* Success -- 0 byte reached */
	decl	%edx
	xorl	%eax,%eax
	jmp	6f
4:
			/* edx is zero -- return ENAMETOOLONG */
	movl	$(ENAMETOOLONG),%eax

6:			/* set *lencopied and return %eax */
	movl	20(%esp),%ecx
	subl	%edx,%ecx
	movl	24(%esp),%edx
	orl	%edx,%edx
	jz	7f
	movl	%ecx,(%edx)
7:
	popl	%edi
	popl	%esi
	ret

/*
 * {fu,su},{byte,word,sword,wintr}
 */
ENTRY(fuword)
ALTENTRY(fuiword)
	movl	__udatasel,%ax
	movl	%ax,%gs
	movl	_curpcb,%ecx
	movl	$fusufault,PCB_ONFAULT(%ecx)
	movl	4(%esp),%edx
	gs
	movl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret
	
ENTRY(fusword)
	movl	__udatasel,%ax
	movl	%ax,%gs
	movl	_curpcb,%ecx
	movl	$fusufault,PCB_ONFAULT(%ecx)
	movl	4(%esp),%edx
	gs
	movzwl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret
	
/* just like fusword, except it uses fusubail rather than fusufault */
ENTRY(fuswintr)
	movl	__udatasel,%ax
	movl	%ax,%gs
	movl	_curpcb,%ecx
	movl	$_fusubail,PCB_ONFAULT(%ecx)
	movl	4(%esp),%edx
	gs
	movzwl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret
	
ENTRY(fubyte)
ALTENTRY(fuibyte)
	movl	__udatasel,%ax
	movl	%ax,%gs
	movl	_curpcb,%ecx
	movl	$fusufault,PCB_ONFAULT(%ecx)
	movl	4(%esp),%edx
	gs
	movzbl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret
	
fusufault:
	movl	_curpcb,%ecx
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx)
	decl	%eax
	ret

	.globl	_fusubail
/* used by trap() */
_fusubail:
	movl	_curpcb,%ecx
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx)
	decl	%eax
	ret

ENTRY(suword)
ALTENTRY(suiword)
	movl	__udatasel,%ax
	movl	%ax,%gs
	movl	_curpcb,%ecx
	movl	$fusufault,PCB_ONFAULT(%ecx) #in case we page/protection violate
	movl	4(%esp),%edx

#if defined(I386_CPU)

#if defined(I486_CPU) || defined(I586_CPU)
	cmpl	$(CPUCLASS_386),_cpu_class
	jne	2f	/* fine, we're not a 386 so don't need this stuff */
#endif	/* I486_CPU || I586_CPU */

	movl	%edx,%eax
	shrl	$(PGSHIFT),%edx	/* fetch pte associated with address */
	movb	_PTmap(,%edx,4),%dl
	andb	$7,%dl		/* if we are the one case that won't trap... */
	cmpb	$5,%dl
	jne	1f
				/* ... then simulate the trap! */
	pushl	%eax
	call	_trapwrite	/* trapwrite(addr) */
	addl	$4,%esp	/* clear parameter from the stack */
	movl	_curpcb,%ecx	# restore trashed registers
	orl	%eax,%eax	/* if not ok, return */
	jne	fusufault
1:
		/* XXX also need to check the following 3 bytes for validity! */

	movl	4(%esp),%edx
#endif

2:
	movl	8(%esp),%eax
	gs
	movl	%eax,(%edx)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx) #in case we page/protection violate
	ret
	
ENTRY(susword)
	movl	__udatasel,%eax
	movl	%ax,%gs
	movl	_curpcb,%ecx
	movl	$fusufault,PCB_ONFAULT(%ecx) #in case we page/protection violate
	movl	4(%esp),%edx

#if defined(I386_CPU)

#if defined(I486_CPU) || defined(I586_CPU)
	cmpl	$(CPUCLASS_386),_cpu_class
	jne	2f	/* fine, we're not a 386 so don't need this stuff */
#endif	/* I486_CPU || I586_CPU */

	movl	%edx,%eax
	shrl	$(PGSHIFT),%edx	/* calculate pte address */
	movb	_PTmap(,%edx,4),%dl
	andb	$7,%dl		/* if we are the one case that won't trap... */
	cmpb	$5,%dl
	jne	1f
				/* ...then simulate the trap! */
	pushl	%eax
	call	_trapwrite	/* trapwrite(addr) */
	addl	$4,%esp	/* clear parameter from the stack */
	movl	_curpcb,%ecx	/* restore trashed registers */
	orl	%eax,%eax
	jne	fusufault
1:
		/* XXX also need to check the following byte for validity! */

	movl	4(%esp),%edx
#endif

2:
	movl	8(%esp),%eax
	gs
	movw	%ax,(%edx)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx) #in case we page/protection violate
	ret

/* same as susword, but uses fusubail rather than fusufault */
ENTRY(suswintr)
	movl	__udatasel,%eax
	movl	%ax,%gs
	movl	_curpcb,%ecx
	movl	$_fusubail,PCB_ONFAULT(%ecx) #in case we page/protection violate
	movl	4(%esp),%edx

#if defined(I386_CPU)

#if defined(I486_CPU) || defined(I586_CPU)
	cmpl	$(CPUCLASS_386),_cpu_class
	jne	2f	/* fine, we're not a 386 so don't need this stuff */
#endif	/* I486_CPU || I586_CPU */

	movl	%edx,%eax
	shrl	$(PGSHIFT),%edx	/* calculate pte address */
	movb	_PTmap(,%edx,4),%dl
	andb	$7,%dl		/* if we are the one case that won't trap... */
	cmpb	$5,%dl
	jne	1f
				/* ...then simulate the trap! */
	pushl	%eax
	call	_trapwrite	/* trapwrite(addr) */
	addl	$4,%esp	/* clear parameter from the stack */
	movl	_curpcb,%ecx	/* restore trashed registers */
	orl	%eax,%eax
	jne	_fusubail
1:
		/* XXX also need to check the following byte for validity! */

	movl	4(%esp),%edx
#endif

2:
	movl	8(%esp),%eax
	gs
	movw	%ax,(%edx)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx) #in case we page/protection violate
	ret

ENTRY(subyte)
ALTENTRY(suibyte)
	movl	__udatasel,%eax
	movl	%ax,%gs
	movl	_curpcb,%ecx
	movl	$fusufault,PCB_ONFAULT(%ecx)
	movl	4(%esp),%edx

#if defined(I386_CPU)

#if defined(I486_CPU) || defined(I586_CPU)
	cmpl	$(CPUCLASS_386),_cpu_class
	jne	2f	/* fine, we're not a 386 so don't need this stuff */
#endif	/* I486_CPU || I586_CPU */

	movl	%edx,%eax
	shrl	$(PGSHIFT),%edx	/* calculate pte address */
	movb	_PTmap(,%edx,4),%dl
	andb	$7,%dl		/* if we are the one case that won't trap... */
	cmpb	$5,%dl
	jne	1f
				/* ...then simulate the trap! */
	pushl	%eax
	call	_trapwrite	/* trapwrite(addr) */
	addl	$4,%esp	/* clear parameter from the stack */
	movl	_curpcb,%ecx	/* restore trashed registers */
	orl	%eax,%eax
	jne	fusufault
1:
	movl	4(%esp),%edx
#endif

2:
	movl	8(%esp),%eax
	gs
	movb	%eax,(%edx)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx) #in case we page/protection violate
	ret

	/*
	 * void lgdt(struct region_descriptor *rdp);
	 */
ENTRY(lgdt)
	/* reload the descriptor table */
	movl	4(%esp),%eax
	lgdt	(%eax)
	/* flush the prefetch q */
	jmp	1f
	nop
1:
	/* reload "stale" selectors */
	movl	$(KDSEL),%eax
	movl	%ax,%ds
	movl	%ax,%es
	movl	%ax,%ss

	/* reload code selector by turning return into intersegmental return */
	movl	(%esp),%eax
	pushl	%eax
	# movl	$(KCSEL),4(%esp)
	movl	$8,4(%esp)
	lret

	# ssdtosd(*ssdp,*sdp)
ENTRY(ssdtosd)
	pushl	%ebx
	movl	8(%esp),%ecx
	movl	8(%ecx),%ebx
	shll	$16,%ebx
	movl	(%ecx),%edx
	roll	$16,%edx
	movb	%dh,%bl
	movb	%dl,%bh
	rorl	$8,%ebx
	movl	4(%ecx),%eax
	movw	%ax,%dx
	andl	$0xf0000,%eax
	orl	%eax,%ebx
	movl	12(%esp),%ecx
	movl	%edx,(%ecx)
	movl	%ebx,4(%ecx)
	popl	%ebx
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
 * Setrq(p)
 *
 * Call should be made at spl6(), and p->p_stat should be SRUN
 */
ENTRY(setrq)
	movl	4(%esp),%eax
	cmpl	$0,P_RLINK(%eax)	# should not be on q already
	jne	1f
	movzbl	P_PRI(%eax),%edx
	shrl	$2,%edx
	btsl	%edx,_whichqs		# set q full bit
	shll	$3,%edx
	addl	$_qs,%edx		# locate q hdr
	movl	%edx,P_LINK(%eax)	# link process on tail of q
	movl	P_RLINK(%edx),%ecx
	movl	%ecx,P_RLINK(%eax)
	movl	%eax,P_RLINK(%edx)
	movl	%eax,P_LINK(%ecx)
	ret
1:	pushl	$2f
	call	_panic
	/*NOTREACHED*/
2:	.asciz	"setrq"

/*
 * Remrq(p)
 *
 * Call should be made at spl6().
 */
ENTRY(remrq)
	movl	4(%esp),%eax
	movzbl	P_PRI(%eax),%edx
	shrl	$2,%edx
	btrl	%edx,_whichqs		# clear full bit, panic if clear already
	jnb	1f
	pushl	%edx
	movl	P_LINK(%eax),%ecx	# unlink process
	movl	P_RLINK(%eax),%edx
	movl	%edx,P_RLINK(%ecx)
	movl	P_RLINK(%eax),%ecx
	movl	P_LINK(%eax),%edx
	movl	%edx,P_LINK(%ecx)
	popl	%edx
	movl	$_qs,%ecx
	shll	$3,%edx
	addl	%edx,%ecx
	cmpl	P_LINK(%ecx),%ecx	# q still has something?
	je	2f
	shrl	$3,%edx			# yes, set bit as still full
	btsl	%edx,_whichqs
2:	movl	$0,P_RLINK(%eax)	# zap reverse link to indicate off list
	ret
1:	pushl	$3f
	call	_panic
	/*NOTREACHED*/
3:	.asciz	"remrq"

/*
 * When no processes are on the runq, Swtch branches to idle
 * to wait for something to come ready.
 */
	.globl	Idle
	ALIGN_TEXT
Idle:
sti_for_idle:
	sti

	ALIGN_TEXT
idle:
	call	_splnone		# process pending interrupts
	cmpl	$0,_whichqs
	jne	sw1
	hlt				# wait for interrupt
	jmp	idle

	ALIGN_TEXT
badsw:
	pushl	$1f
	call	_panic
	/*NOTREACHED*/
1:	.asciz	"swtch"

/*
 * Swtch()
 */
	SUPERALIGN_TEXT	/* so profiling doesn't lump Idle with cpu_swtch */
ENTRY(cpu_swtch)

	/* switch to new process. first, save context as needed */
	movl	4(%esp),%ecx

	/* if no process to save, don't bother */
	testl	%ecx,%ecx
	je	sw1

	movl	P_ADDR(%ecx),%ecx

	movl	(%esp),%eax		# Hardware registers
	movl	%eax,PCB_EIP(%ecx)
	movl	%ebx,PCB_EBX(%ecx)
	movl	%esp,PCB_ESP(%ecx)
	movl	%ebp,PCB_EBP(%ecx)
	movl	%esi,PCB_ESI(%ecx)
	movl	%edi,PCB_EDI(%ecx)

#if NNPX > 0
	/* have we used fp, and need a save? */
	mov	_curproc,%eax
	cmp	%eax,_npxproc
	jne	1f
	pushl	%ecx			/* h/w bugs make saving complicated */
	leal	PCB_SAVEFPU(%ecx),%eax
	pushl	%eax
	call	_npxsave		/* do it in a big C function */
	popl	%eax
	popl	%ecx
1:
#endif

	movl	$0,_curproc		# out of process

	movl	_cpl,%eax		# splhigh()
	movl	$-1,_cpl
	movl	%eax,PCB_IML(%ecx)	# save ipl

	/* save is done, now choose a new process or idle */
sw1:
	cli				# splhigh doesn't do a cli
	movl	_whichqs,%edi
2:
	bsfl	%edi,%eax		# find a full q
	jz	sti_for_idle		# if none, idle
	# XXX update whichqs?
	btrl	%eax,%edi		# clear q full status
	jnb	2b			# if it was clear, look for another
	movl	%eax,%ebx		# save which one we are using

	shll	$3,%eax
	addl	$_qs,%eax		# select q
	movl	%eax,%esi

#ifdef	DIAGNOSTIC
	cmpl	P_LINK(%eax),%eax # linked to self? (e.g. not on list)
	je	badsw			# not possible
#endif

	movl	P_LINK(%eax),%ecx	# unlink from front of process q
	movl	P_LINK(%ecx),%edx
	movl	%edx,P_LINK(%eax)
	movl	P_RLINK(%ecx),%eax
	movl	%eax,P_RLINK(%edx)

	cmpl	P_LINK(%ecx),%esi	# q empty
	je	3f
	btsl	%ebx,%edi		# nope, set to indicate full
3:
	movl	%edi,_whichqs		# update q status

	movl	$0,%eax
	movl	%eax,_want_resched

#ifdef	DIAGNOSTIC
	cmpl	%eax,P_WCHAN(%ecx)
	jne	badsw
	cmpb	$(SRUN),P_STAT(%ecx)
	jne	badsw
#endif

	movl	%eax,P_RLINK(%ecx) /* isolate process to run */
	movl	P_ADDR(%ecx),%edx
	movl	PCB_CR3(%edx),%ebx

	/* switch address space */
	movl	%ebx,%cr3

	/* restore context */
	movl	PCB_EBX(%edx),%ebx
	movl	PCB_ESP(%edx),%esp
	movl	PCB_EBP(%edx),%ebp
	movl	PCB_ESI(%edx),%esi
	movl	PCB_EDI(%edx),%edi
	movl	PCB_EIP(%edx),%eax
	movl	%eax,(%esp)

	movl	%ecx,_curproc		# into next process
	movl	%edx,_curpcb

#ifdef	USER_LDT
	cmpl	$0, PCB_USERLDT(%edx)
	jnz	1f
	movl	__default_ldt,%eax
	cmpl	_currentldt,%eax
	je	2f
	lldt	__default_ldt
	movl	%eax,_currentldt
	jmp	2f
1:	pushl	%edx
	call	_set_user_ldt
	popl	%edx
2:
#endif
	sti				# splx() doesn't do an sti/cli

	pushl   PCB_IML(%edx)
	call    _splx			# restore the process's ipl
	addl	$4,%esp

	movl	%edx,%eax		# return (p);
	ret

ENTRY(mvesp)
	movl	%esp,%eax
	ret

/*
 * struct proc *swtch_to_inactive(p) ; struct proc *p;
 *
 * At exit of a process, move off the address space of the
 * process and onto a "safe" one. Then, on a temporary stack
 * return and run code that disposes of the old state.
 * Since this code requires a parameter from the "old" stack,
 * pass it back as a return value.
 */
ENTRY(swtch_to_inactive)
	popl	%edx			# old pc
	popl	%eax			# arg, our return value
	movl	_IdlePTD,%ecx
	movl	%ecx,%cr3		# good bye address space
 #write buffer?
	movl	$tmpstk-4,%esp		# temporary stack, compensated for call
	jmp	%edx			# return, execute remainder of cleanup

/*
 * savectx(pcb, altreturn)
 * Update pcb, saving current processor state and arranging
 * for alternate return ala longjmp in swtch if altreturn is true.
 */
ENTRY(savectx)
	movl	4(%esp),%ecx
	movl	_cpl,%eax
	movl	%eax,PCB_IML(%ecx)
	movl	(%esp),%eax	
	movl	%eax,PCB_EIP(%ecx)
	movl	%ebx,PCB_EBX(%ecx)
	movl	%esp,PCB_ESP(%ecx)
	movl	%ebp,PCB_EBP(%ecx)
	movl	%esi,PCB_ESI(%ecx)
	movl	%edi,PCB_EDI(%ecx)

#if NNPX > 0
	/*
	 * If npxproc == NULL, then the npx h/w state is irrelevant and the
	 * state had better already be in the pcb.  This is true for forks
	 * but not for dumps (the old book-keeping with FP flags in the pcb
	 * always lost for dumps because the dump pcb has 0 flags).
	 *
	 * If npxproc != NULL, then we have to save the npx h/w state to
	 * npxproc's pcb and copy it to the requested pcb, or save to the
	 * requested pcb and reload.  Copying is easier because we would
	 * have to handle h/w bugs for reloading.  We used to lose the
	 * parent's npx state for forks by forgetting to reload.
	 */
	movl	_npxproc,%eax
	testl	%eax,%eax
  	jz	1f

	pushl	%ecx
	movl	P_ADDR(%eax),%eax
	leal	PCB_SAVEFPU(%eax),%eax
	pushl	%eax
	pushl	%eax
	call	_npxsave
	addl	$4,%esp
	popl	%eax
	popl	%ecx

	pushl	%ecx
	pushl	$108+8*2	/* XXX h/w state size + padding */
	leal	PCB_SAVEFPU(%ecx),%ecx
	pushl	%ecx
	pushl	%eax
	call	_bcopy
	addl	$12,%esp
	popl	%ecx
1:
#endif

	cmpl	$0,8(%esp)
	je	1f
	movl	%esp,%edx		# relocate current sp relative to pcb
	subl	$(USRSTACK),%edx	#   (sp is relative to kstack):
	addl	%edx,%ecx		#   pcb += sp - kstack;
	movl	%eax,(%ecx)		# write return pc at (relocated) sp@
	# this mess deals with replicating register state gcc hides
	movl	12(%esp),%eax
	movl	%eax,12(%ecx)
	movl	16(%esp),%eax
	movl	%eax,16(%ecx)
	movl	20(%esp),%eax
	movl	%eax,20(%ecx)
	movl	24(%esp),%eax
	movl	%eax,24(%ecx)
1:
	xorl	%eax,%eax		# return 0
	ret
	

/*
 * Trap and fault vector routines
 *
 * XXX - debugger traps are now interrupt gates so at least bdb doesn't lose
 * control.  The sti's give the standard losing behaviour for ddb and kgdb.
 */ 
#define	IDTVEC(name)	ALIGN_TEXT; .globl _X/**/name; _X/**/name:
#define	INTRENTRY \
	pushl	%ds		; \
	pushl	%es		; /* now the stack frame is a trap frame */ \
	pushal			; \
	movl	$(KDSEL),%eax	; \
	movl	%ax,%ds		; \
	movl	%ax,%es
#define	INTREXIT \
	jmp	doreti
#define	INTRFASTEXIT \
	popal			; \
	popl	%es		; \
	popl	%ds		; \
	addl	$8,%esp		; \
	iret

#define	TRAP(a)		pushl $(a) ; jmp alltraps
#define	ZTRAP(a)	pushl $0 ; TRAP(a)
#ifdef KGDB
#define	BPTTRAP(a)	pushl $(a) ; jmp bpttraps
#else
#define	BPTTRAP(a)	TRAP(a)
#endif

	.text
IDTVEC(div)
	ZTRAP(T_DIVIDE)
IDTVEC(dbg)
	subl	$4,%esp
	pushl	%eax
#	movl	%dr6,%eax	/* XXX stupid assembler! */
	.byte	0x0f, 0x21, 0xf0
	movl	%eax,4(%esp)
	andb	$~15,%al
#	movl	%eax,%dr6	/* XXX stupid assembler! */
	.byte	0x0f, 0x23, 0xf0
	popl	%eax	
#ifdef BDB
	BDBTRAP(dbg)
#endif
	BPTTRAP(T_TRCTRAP)
IDTVEC(nmi)
	ZTRAP(T_NMI)
IDTVEC(bpt)
#ifdef BDBTRAP
	BDBTRAP(bpt)
#endif
	pushl	$0
	BPTTRAP(T_BPTFLT)
IDTVEC(ofl)
	ZTRAP(T_OFLOW)
IDTVEC(bnd)
	ZTRAP(T_BOUND)
IDTVEC(ill)
	ZTRAP(T_PRIVINFLT)
IDTVEC(dna)
	ZTRAP(T_DNA)
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
	pushl	$0		/* dummy error code */
	pushl	$(T_ASTFLT)
	INTRENTRY
	pushl	_cpl		/* now it's an intrframe */
	incl	_cnt+V_TRAP
	movl	%esp,%eax	/* pointer to frame */
	pushl	%eax
	call	_npxintr
	addl	$4,%esp
	INTREXIT
#else
	ZTRAP(T_ARITHTRAP)
#endif
IDTVEC(align)
	ZTRAP(T_ALIGNFLT)
	/* 18 - 31 reserved for future exp */
IDTVEC(rsvd1)
	ZTRAP(T_RESERVED)
IDTVEC(rsvd2)
	ZTRAP(T_RESERVED)
IDTVEC(rsvd3)
	ZTRAP(T_RESERVED)
IDTVEC(rsvd4)
	ZTRAP(T_RESERVED)
IDTVEC(rsvd5)
	ZTRAP(T_RESERVED)
IDTVEC(rsvd6)
	ZTRAP(T_RESERVED)
IDTVEC(rsvd7)
	ZTRAP(T_RESERVED)
IDTVEC(rsvd8)
	ZTRAP(T_RESERVED)
IDTVEC(rsvd9)
	ZTRAP(T_RESERVED)
IDTVEC(rsvd10)
	ZTRAP(T_RESERVED)
IDTVEC(rsvd11)
	ZTRAP(T_RESERVED)
IDTVEC(rsvd12)
	ZTRAP(T_RESERVED)
IDTVEC(rsvd13)
	ZTRAP(T_RESERVED)
IDTVEC(rsvd14)
	ZTRAP(T_RESERVED)

	SUPERALIGN_TEXT
alltraps:
	INTRENTRY
calltrap:
	call	_trap
	/*
	 * Return through doreti to handle ASTs.  Have to change trap frame
	 * to interrupt frame.
	 */
	movl	$(T_ASTFLT),TF_TRAPNO(%esp)	/* new trap type (err code not used) */
	pushl	_cpl
	INTREXIT

#ifdef KGDB
/*
 * This code checks for a kgdb trap, then falls through
 * to the regular trap code.
 */
	ALIGN_TEXT
bpttraps:
	INTRENTRY
	testb	$(SEL_RPL_MASK),TF_CS(%esp)
					# non-kernel mode?
	jne	calltrap		# yes
	call	_kgdb_trap_glue		
	jmp	calltrap
#endif

/*
 * Call gate entry for syscall
 */

	SUPERALIGN_TEXT
IDTVEC(syscall)
	pushl	$0			# Room for tf_err
	pushfl				# Room for tf_trapno
	INTRENTRY
	movl	TF_TRAPNO(%esp),%eax	# copy eflags from tf_trapno to tf_eflags
	movl	%eax,TF_EFLAGS(%esp)
	call	_syscall
	/*
	 * Return through doreti to handle ASTs.
	 */
	movl	$(T_ASTFLT),TF_TRAPNO(%esp)	# new trap type (err code not used)
	pushl	_cpl
	INTREXIT

#include "i386/isa/vector.s"
#include "i386/isa/icu.s"
