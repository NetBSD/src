/*	$NetBSD: locore.S,v 1.39 2006/05/13 08:56:08 skrll Exp $	*/

/*-
 * Copyright (c) 1998, 2000, 2004 The NetBSD Foundation, Inc.
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

#include "opt_compat_netbsd.h"
#include "opt_compat_oldboot.h"
#include "opt_cputype.h"
#include "opt_ddb.h"
#include "opt_ipkdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_realmem.h"
#include "opt_user_ldt.h"
#include "opt_vm86.h"

#include "npx.h"
#include "assym.h"
#include "apm.h"
#include "lapic.h"
#include "ioapic.h"
#include "ksyms.h"

#include <sys/errno.h>
#include <sys/syscall.h>

#include <machine/cputypes.h>
#include <machine/segments.h>
#include <machine/specialreg.h>
#include <machine/trap.h>

#if NLAPIC > 0
#include <machine/i82489reg.h>
#endif

#include <machine/multiboot.h>

/* LINTSTUB: include <sys/types.h> */
/* LINTSTUB: include <machine/cpu.h> */
/* LINTSTUB: include <sys/systm.h> */

#include <machine/asm.h>

#if defined(MULTIPROCESSOR)

#define SET_CURLWP(lwp,cpu)				\
	movl	CPUVAR(SELF),cpu		; 	\
	movl	lwp,CPUVAR(CURLWP)	;	\
	movl	cpu,L_CPU(lwp)

#else

#define SET_CURLWP(lwp,tcpu)		movl	lwp,CPUVAR(CURLWP)
#define GET_CURLWP(reg)			movl	CPUVAR(CURLWP),reg

#endif

#define SET_CURPCB(reg)			movl	reg,CPUVAR(CURPCB)

#define CLEAR_RESCHED(reg)		movl	reg,CPUVAR(RESCHED)

/* XXX temporary kluge; these should not be here */
/* Get definitions for IOM_BEGIN, IOM_END, and IOM_SIZE */
#include <dev/isa/isareg.h>


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

#include <machine/frameasm.h>


#ifdef MULTIPROCESSOR
#include <machine/i82489reg.h>
#endif

/*
 * Initialization
 */
	.data

	.globl	_C_LABEL(cpu)
	.globl	_C_LABEL(esym)
	.globl	_C_LABEL(atdevbase)
	.globl	_C_LABEL(proc0paddr),_C_LABEL(PDPpaddr)
	.globl	_C_LABEL(gdt)
#ifdef I586_CPU
	.globl	_C_LABEL(idt)
#endif
	.globl	_C_LABEL(lapic_tpr)

#if NLAPIC > 0
#ifdef __ELF__
	.align	PAGE_SIZE
#else
	.align	12
#endif
	.globl _C_LABEL(local_apic), _C_LABEL(lapic_id)
_C_LABEL(local_apic):
	.space	LAPIC_ID
_C_LABEL(lapic_id):
	.long	0x00000000
	.space  LAPIC_TPRI-(LAPIC_ID+4)
_C_LABEL(lapic_tpr):
	.space  LAPIC_PPRI-LAPIC_TPRI
_C_LABEL(lapic_ppr):
	.space	LAPIC_ISR-LAPIC_PPRI
_C_LABEL(lapic_isr):
	.space	PAGE_SIZE-LAPIC_ISR
#else
_C_LABEL(lapic_tpr):
	.long 0
#endif


_C_LABEL(cpu):		.long	0	# are we 386, 386sx, or 486,
					#   or Pentium, or..
_C_LABEL(atdevbase):	.long	0	# location of start of iomem in virtual
_C_LABEL(proc0paddr):	.long	0
_C_LABEL(PDPpaddr):	.long	0	# paddr of PDP, for libkvm

	.space 512
tmpstk:


#define	_RELOC(x)	((x) - KERNBASE)
#define	RELOC(x)	_RELOC(_C_LABEL(x))

	.text
	.globl	_C_LABEL(kernel_text)
	.set	_C_LABEL(kernel_text),KERNTEXTOFF

	.globl	start
start:	movw	$0x1234,0x472			# warm boot

#if defined(MULTIBOOT)
	jmp	1f

	.align	4
	.globl	Multiboot_Header
_C_LABEL(Multiboot_Header):
#define MULTIBOOT_HEADER_FLAGS	(MULTIBOOT_HEADER_WANT_MEMORY | \
				 MULTIBOOT_HEADER_HAS_ADDR)
	.long	MULTIBOOT_HEADER_MAGIC
	.long	MULTIBOOT_HEADER_FLAGS
	.long	-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
	.long	RELOC(Multiboot_Header)
	.long	RELOC(start)
	.long	RELOC(_edata)
	.long	RELOC(_end) + MULTIBOOT_SYMTAB_SPACE
	.long	RELOC(start)

1:
	/* Check if we are being executed by a Multiboot-compliant boot
	 * loader. */
	cmpl	$MULTIBOOT_INFO_MAGIC,%eax
	jne	1f

	/* Indeed, a multiboot-compliat boot loader executed us.  We copy
	 * the received Multiboot information structure into kernel's data
	 * space to process it later -- after we are relocated.  It will
	 * be safer to run complex C code than doing it at this point. */
	pushl	%ebx		# Address of Multiboot information
	call	_C_LABEL(multiboot_pre_reloc)
	addl	$4,%esp
	jmp	2f
#endif

1:
	/*
	 * At this point, we know that a NetBSD-specific boot loader
	 * booted this kernel.  The stack carries the following parameters:
	 * (boothowto, [bootdev], bootinfo, esym, biosbasemem, biosextmem),
	 * 4 bytes each.
	 */
	addl	$4,%esp		# Discard return address to boot loader
	call	_C_LABEL(native_loader)
	addl	$24,%esp

2:
	/* First, reset the PSL. */
	pushl	$PSL_MBO
	popfl

	/* Clear segment registers; always null in proc0. */
	xorl	%eax,%eax
	movw	%ax,%fs
	movw	%ax,%gs
	decl	%eax
	movl	%eax,RELOC(cpu_info_primary)+CPU_INFO_LEVEL

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
	movl	%eax,RELOC(cpu_info_primary)+CPU_INFO_LEVEL

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
#define	PROC0PDIR	((0)              * PAGE_SIZE)
#define	PROC0STACK	((1)              * PAGE_SIZE)
#define	SYSMAP		((1+UPAGES)       * PAGE_SIZE)
#define	TABLESIZE	((1+UPAGES) * PAGE_SIZE) /* + nkpde * PAGE_SIZE */

	/* Find end of kernel image. */
	movl	$RELOC(end),%edi
#if (NKSYMS || defined(DDB) || defined(LKM)) && !defined(SYMTAB_SPACE)
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
	 * how many entries it will have.  Adjust nkpde to the actual
	 * kernel size automatically.  Account for the bootstrap tables,
	 * round up, and add an extra 4MB.
	 */
	leal	TABLESIZE+NBPD+PDOFSET(%edi),%eax
	shrl	$PDSHIFT,%eax
	movl	RELOC(nkpde),%ecx		# get nkpde
	cmpl	%ecx,%eax
	jb	1f
	movl	%eax,%ecx
1:	cmpl	$NKPTP_MIN,%ecx			# larger than min?
	jge	1f
	movl	$NKPTP_MIN,%ecx			# set at min
	jmp	2f
1:	cmpl	$NKPTP_MAX,%ecx			# larger than max?
	jle	2f
	movl	$NKPTP_MAX,%ecx
2:	movl	%ecx,RELOC(nkpde)

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
	addl	$PAGE_SIZE,%eax	; /* increment physical address */ \
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

	/* Save phys. addr of PDP, for libkvm. */
	movl	%esi,RELOC(PDPpaddr)

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

#if defined(MULTIBOOT)
	/* It is now safe to parse the Multiboot information structure
	 * we saved before from C code.  Note that we cannot delay its
	 * parsing any more because initgdt (called below) needs to make
	 * use of this information. */
	call	_C_LABEL(multiboot_post_reloc)
#endif

	subl	$NGDT*8, %esp		# space for temporary gdt
	pushl	%esp
	call	_C_LABEL(initgdt)
	addl	$4,%esp

	movl	_C_LABEL(nkpde),%eax
	shll	$PGSHIFT,%eax
	addl	$TABLESIZE,%eax
	addl	%esi,%eax		# skip past stack and page tables

	pushl	%eax
	call	_C_LABEL(init386)	# wire 386 chip for unix operation
	addl	$4+NGDT*8,%esp		# pop temporary gdt

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
 * void proc_trampoline(void);
 * This is a trampoline function pushed onto the stack of a newly created
 * process in order to do some additional setup.  The trampoline is entered by
 * cpu_switch()ing to the process, so we abuse the callee-saved registers used
 * by cpu_switch() to store the information about the stub to call.
 * NOTE: This function does not have a normal calling sequence!
 */
/* LINTSTUB: Func: void proc_trampoline(void) */
NENTRY(proc_trampoline)
#ifdef MULTIPROCESSOR
	call	_C_LABEL(proc_trampoline_mp)
#endif
	movl	$IPL_NONE,CPUVAR(ILEVEL)
	pushl	%ebx
	call	*%esi
	addl	$4,%esp
	DO_DEFERRED_SWITCH(%eax)
	INTRFASTEXIT
	/* NOTREACHED */

/*****************************************************************************/
#ifdef COMPAT_16
/*
 * Signal trampoline; copied to top of user stack.
 */
/* LINTSTUB: Var: char sigcode[1], esigcode[1]; */
NENTRY(sigcode)
	/*
	 * Handler has returned here as if we called it.  The sigcontext
	 * is on the stack after the 3 args "we" pushed.
	 */
	leal	12(%esp),%eax		# get pointer to sigcontext
	movl	%eax,4(%esp)		# put it in the argument slot
					# fake return address already there
	movl	$SYS_compat_16___sigreturn14,%eax
	int	$0x80	 		# enter kernel with args on stack
	movl	$SYS_exit,%eax
	int	$0x80			# exit if sigreturn fails
	.globl	_C_LABEL(esigcode)
_C_LABEL(esigcode):
#endif

/*****************************************************************************/

/*
 * The following is i386-specific nonsense.
 */

/*
 * void lgdt(struct region_descriptor *rdp);
 * Load a new GDT pointer (and do any necessary cleanup).
 * XXX It's somewhat questionable whether reloading all the segment registers
 * is necessary, since the actual descriptor data is not changed except by
 * process creation and exit, both of which clean up via task switches.  OTOH,
 * this only happens at run time when the GDT is resized.
 */
/* LINTSTUB: Func: void lgdt(struct region_descriptor *rdp) */
NENTRY(lgdt)
	/* Reload the descriptor table. */
	movl	4(%esp),%eax
	lgdt	(%eax)
	/* Flush the prefetch queue. */
	jmp	1f
	nop
1:	/* Reload "stale" selectors. */
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%gs
	movw	%ax,%ss
	movl	$GSEL(GCPU_SEL, SEL_KPL),%eax
	movw	%ax,%fs
	/* Reload code selector by doing intersegment return. */
	popl	%eax
	pushl	$GSEL(GCODE_SEL, SEL_KPL)
	pushl	%eax
	lret

/*****************************************************************************/

/*
 * These functions are primarily used by DDB.
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

	.globl	_C_LABEL(sched_whichqs),_C_LABEL(sched_qs)
	.globl	_C_LABEL(uvmexp),_C_LABEL(panic)

#ifdef DIAGNOSTIC
NENTRY(switch_error)
	pushl	$1f
	call	_C_LABEL(panic)
	/* NOTREACHED */
1:	.asciz	"cpu_switch"
#endif /* DIAGNOSTIC */

/*
 * void cpu_switch(struct lwp *)
 * Find a runnable lwp and switch to it.  Wait if necessary.  If the new
 * lwp is the same as the old one, we short-circuit the context save and
 * restore.
 *
 * Note that the stack frame layout is known to "struct switchframe"
 * in <machine/frame.h> and to the code in cpu_fork() which initializes
 * it for a new lwp.
 */
ENTRY(cpu_switch)
	pushl	%ebx
	pushl	%esi
	pushl	%edi

#ifdef DEBUG
	cmpl	$IPL_SCHED,CPUVAR(ILEVEL)
	jae	1f
	pushl	$2f
	call	_C_LABEL(panic)
	/* NOTREACHED */
2:	.asciz	"not splsched() in cpu_switch!"
1:
#endif /* DEBUG */

	movl	16(%esp),%esi		# current

	/*
	 * Clear curlwp so that we don't accumulate system time while idle.
	 * This also insures that schedcpu() will move the old lwp to
	 * the correct queue if it happens to get called from the spllower()
	 * below and changes the priority.  (See corresponding comment in
	 * userret()).
	 */
	movl	$0,CPUVAR(CURLWP)
	/*
	 * First phase: find new lwp.
	 *
	 * Registers:
	 *   %eax - queue head, scratch, then zero
	 *   %ebx - queue number
	 *   %ecx - cached value of whichqs
	 *   %edx - next lwp in queue
	 *   %esi - old lwp
	 *   %edi - new lwp
	 */

	/* Look for new lwp. */
	cli				# splhigh doesn't do a cli
	movl	_C_LABEL(sched_whichqs),%ecx
	bsfl	%ecx,%ebx		# find a full q
	jnz	switch_dequeue

	/*
	 * idling:	save old context.
	 *
	 * Registers:
	 *   %eax, %ecx - scratch
	 *   %esi - old lwp, then old pcb
	 *   %edi - idle pcb
	 */

	pushl	%esi
	call	_C_LABEL(pmap_deactivate2)	# pmap_deactivate(oldproc)
	addl	$4,%esp

	movl	L_ADDR(%esi),%esi

	/* Save stack pointers. */
	movl	%esp,PCB_ESP(%esi)
	movl	%ebp,PCB_EBP(%esi)

	/* Find idle PCB for this CPU */
#ifndef MULTIPROCESSOR
	movl	$_C_LABEL(lwp0),%ebx
	movl	L_ADDR(%ebx),%edi
	movl	L_MD_TSS_SEL(%ebx),%edx
#else
	movl	CPUVAR(IDLE_PCB),%edi
	movl	CPUVAR(IDLE_TSS_SEL),%edx
#endif
	movl	$0,CPUVAR(CURLWP)		/* In case we fault... */

	/* Restore the idle context (avoid interrupts) */
	cli

	/* Restore stack pointers. */
	movl	PCB_ESP(%edi),%esp
	movl	PCB_EBP(%edi),%ebp

	/* Switch TSS. Reset "task busy" flag before loading. */
	movl	%cr3,%eax
	movl	%eax,PCB_CR3(%edi)
#ifdef MULTIPROCESSOR
	movl	CPUVAR(GDT),%eax
#else
	movl	_C_LABEL(gdt),%eax
#endif
	andl	$~0x0200,4-SEL_KPL(%eax,%edx,1)
	ltr	%dx

	/* We're always in the kernel, so we don't need the LDT. */

	/* Restore cr0 (including FPU state). */
	movl	PCB_CR0(%edi),%ecx
	movl	%ecx,%cr0

	/* Record new pcb. */
	SET_CURPCB(%edi)

	xorl	%esi,%esi
	sti
idle_unlock:
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	call	_C_LABEL(sched_unlock_idle)
#endif
	/* Interrupts are okay again. */
	pushl	$IPL_NONE		# spl0()
	call	_C_LABEL(Xspllower)	# process pending interrupts
	addl	$4,%esp
	jmp	idle_start
idle_zero:
	sti
	call	_C_LABEL(uvm_pageidlezero)
	cli
	cmpl	$0,_C_LABEL(sched_whichqs)
	jnz	idle_exit
idle_loop:
	/* Try to zero some pages. */
	movl	_C_LABEL(uvm)+UVM_PAGE_IDLE_ZERO,%ecx
	testl	%ecx,%ecx
	jnz	idle_zero
	sti
	hlt
NENTRY(mpidle)
idle_start:
	cli
	cmpl	$0,_C_LABEL(sched_whichqs)
	jz	idle_loop
idle_exit:
	movl	$IPL_HIGH,CPUVAR(ILEVEL)		# splhigh
	sti
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	call	_C_LABEL(sched_lock_idle)
#endif
	movl	_C_LABEL(sched_whichqs),%ecx
	bsfl	%ecx,%ebx
	jz	idle_unlock

switch_dequeue:
	/*
	 * we're running at splhigh(), but it's otherwise okay to take
	 * interrupts here.
	 */
	sti
	leal	_C_LABEL(sched_qs)(,%ebx,8),%eax # select q

	movl	L_FORW(%eax),%edi	# unlink from front of process q
#ifdef	DIAGNOSTIC
	cmpl	%edi,%eax		# linked to self (i.e. nothing queued)?
	je	_C_LABEL(switch_error)	# not possible
#endif /* DIAGNOSTIC */
	movl	L_FORW(%edi),%edx
	movl	%edx,L_FORW(%eax)
	movl	%eax,L_BACK(%edx)

	cmpl	%edx,%eax		# q empty?
	jne	3f

	btrl	%ebx,%ecx		# yes, clear to indicate empty
	movl	%ecx,_C_LABEL(sched_whichqs) # update q status

3:	/* We just did it. */
	xorl	%eax,%eax
	CLEAR_RESCHED(%eax)

switch_resume:
#ifdef	DIAGNOSTIC
	cmpl	%eax,L_WCHAN(%edi)	# Waiting for something?
	jne	_C_LABEL(switch_error)	# Yes; shouldn't be queued.
	cmpb	$LSRUN,L_STAT(%edi)	# In run state?
	jne	_C_LABEL(switch_error)	# No; shouldn't be queued.
#endif /* DIAGNOSTIC */

	/* Isolate lwp.  XXX Is this necessary? */
	movl	%eax,L_BACK(%edi)

	/* Record new lwp. */
	movb	$LSONPROC,L_STAT(%edi)	# l->l_stat = LSONPROC
	SET_CURLWP(%edi,%ecx)

	/* Skip context switch if same lwp. */
	xorl	%ebx,%ebx
	cmpl	%edi,%esi
	je	switch_return

	/* If old lwp exited, don't bother. */
	testl	%esi,%esi
	jz	switch_exited

	/*
	 * Second phase: save old context.
	 *
	 * Registers:
	 *   %eax, %ecx - scratch
	 *   %esi - old lwp, then old pcb
	 *   %edi - new lwp
	 */

	pushl	%esi
	call	_C_LABEL(pmap_deactivate2)	# pmap_deactivate(oldproc)
	addl	$4,%esp

	movl	L_ADDR(%esi),%esi

	/* Save stack pointers. */
	movl	%esp,PCB_ESP(%esi)
	movl	%ebp,PCB_EBP(%esi)

switch_exited:
	/*
	 * Third phase: restore saved context.
	 *
	 * Registers:
	 *   %eax, %ebx, %ecx, %edx - scratch
	 *   %esi - new pcb
	 *   %edi - new lwp
	 */

	/* No interrupts while loading new state. */
	cli
	movl	L_ADDR(%edi),%esi

	/* Restore stack pointers. */
	movl	PCB_ESP(%esi),%esp
	movl	PCB_EBP(%esi),%ebp

#if 0
	/* Don't bother with the rest if switching to a system process. */
	testl	$P_SYSTEM,L_FLAG(%edi);	XXX NJWLWP lwp's don't have P_SYSTEM!
	jnz	switch_restored
#endif

	/* Switch TSS. Reset "task busy" flag before loading. */
	movl	%cr3,%eax
	movl	%eax,PCB_CR3(%esi) /* XXX should be done by pmap_activate? */
#ifdef MULTIPROCESSOR
	movl	CPUVAR(GDT),%eax
#else
	/* Load TSS info. */
	movl	_C_LABEL(gdt),%eax
#endif
	movl	L_MD_TSS_SEL(%edi),%edx

	andl	$~0x0200,4(%eax,%edx, 1)
	ltr	%dx

	pushl	%edi
	call	_C_LABEL(pmap_activate)		# pmap_activate(p)
	addl	$4,%esp

#if 0
switch_restored:
#endif
	/* Restore cr0 (including FPU state). */
	movl	PCB_CR0(%esi),%ecx
#ifdef MULTIPROCESSOR
	/*
	 * If our floating point registers are on a different CPU,
	 * clear CR0_TS so we'll trap rather than reuse bogus state.
	 */
	movl	PCB_FPCPU(%esi),%ebx
	cmpl	CPUVAR(SELF),%ebx
	jz	1f
	orl	$CR0_TS,%ecx
1:
#endif
	movl	%ecx,%cr0

	/* Record new pcb. */
	SET_CURPCB(%esi)

	/* Interrupts are okay again. */
	sti

/*
 *  Check for restartable atomic sequences (RAS)
 */
	movl	CPUVAR(CURLWP),%edi
	movl	L_PROC(%edi),%esi
	cmpl	$0,P_RASLIST(%esi)
	jne	2f
1:
	movl	$1,%ebx

switch_return:
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	call    _C_LABEL(sched_unlock_idle)
#endif
	cmpl	$0,CPUVAR(IPENDING)
	jz	3f
	pushl	$IPL_NONE		# spl0()
	call	_C_LABEL(Xspllower)	# process pending interrupts
	addl	$4,%esp
3:
	movl	$IPL_HIGH,CPUVAR(ILEVEL)	# splhigh()

	movl	%ebx,%eax

	popl	%edi
	popl	%esi
	popl	%ebx
	ret

2:					# check RAS list
	movl	L_MD_REGS(%edi),%ebx
	movl	TF_EIP(%ebx),%eax
	pushl	%eax
	pushl	%esi
	call	_C_LABEL(ras_lookup)
	addl	$8,%esp
	cmpl	$-1,%eax
	je	1b
	movl	%eax,TF_EIP(%ebx)
	jmp	1b

/*
 * void cpu_switchto(struct lwp *current, struct lwp *next)
 * Switch to the specified next LWP.
 */
ENTRY(cpu_switchto)
	pushl	%ebx
	pushl	%esi
	pushl	%edi

#ifdef DEBUG
	cmpl	$IPL_SCHED,CPUVAR(ILEVEL)
	jae	1f
	pushl	$2f
	call	_C_LABEL(panic)
	/* NOTREACHED */
2:	.asciz	"not splsched() in cpu_switchto!"
1:
#endif /* DEBUG */

	movl	16(%esp),%esi		# current
	movl	20(%esp),%edi		# next

	/*
	 * Clear curlwp so that we don't accumulate system time while idle.
	 * This also insures that schedcpu() will move the old process to
	 * the correct queue if it happens to get called from the spllower()
	 * below and changes the priority.  (See corresponding comment in
	 * usrret()).
	 *
	 * XXX Is this necessary?  We know we won't go idle.
	 */
	movl	$0,CPUVAR(CURLWP)

	/*
	 * We're running at splhigh(), but it's otherwise okay to take
	 * interrupts here.
	 */
	sti

	/* Jump into the middle of cpu_switch */
	xorl	%eax,%eax
	jmp	switch_resume

/*
 * void cpu_exit(struct lwp *l)
 * Switch to the appropriate idle context (lwp0's if uniprocessor; the CPU's
 * if multiprocessor) and deallocate the address space and kernel stack for p.
 * Then jump into cpu_switch(), as if we were in the idle proc all along.
 */
#ifndef MULTIPROCESSOR
	.globl	_C_LABEL(lwp0)
#endif
/* LINTSTUB: Func: void cpu_exit(struct lwp *l) */
ENTRY(cpu_exit)
	movl	4(%esp),%edi		# old process
#ifndef MULTIPROCESSOR
	movl	$_C_LABEL(lwp0),%ebx
	movl	L_ADDR(%ebx),%esi
	movl	L_MD_TSS_SEL(%ebx),%edx
#else
	movl	CPUVAR(IDLE_PCB),%esi
	movl	CPUVAR(IDLE_TSS_SEL),%edx
#endif
	/* In case we fault... */
	movl	$0,CPUVAR(CURLWP)

	/* Restore the idle context. */
	cli

	/* Restore stack pointers. */
	movl	PCB_ESP(%esi),%esp
	movl	PCB_EBP(%esi),%ebp

	/* Switch TSS. Reset "task busy" flag before loading. */
	movl	%cr3,%eax
	movl	%eax,PCB_CR3(%esi)
#ifdef MULTIPROCESSOR
	movl	CPUVAR(GDT),%eax
#else
	/* Load TSS info. */
	movl	_C_LABEL(gdt),%eax
#endif

	andl	$~0x0200,4-SEL_KPL(%eax,%edx,1)
	ltr	%dx

	/* We're always in the kernel, so we don't need the LDT. */

	/* Restore cr0 (including FPU state). */
	movl	PCB_CR0(%esi),%ecx
	movl	%ecx,%cr0

	/* Record new pcb. */
	SET_CURPCB(%esi)

	/* Interrupts are okay again. */
	sti

	/*
	 * Schedule the dead LWP's stack to be freed.
	 */
	pushl	%edi
	call	_C_LABEL(lwp_exit2)
	addl	$4,%esp

	/* Jump into cpu_switch() with the right state. */
	xorl	%esi,%esi
	movl	%esi,CPUVAR(CURLWP)
	jmp	idle_start

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

/*
 * Old call gate entry for syscall
 */
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
/* LINTSTUB: Var: char Xsyscall[1]; */
IDTVEC(syscall)
	pushl	$2		# size of instruction for restart
syscall1:
	pushl	$T_ASTFLT	# trap # for doing ASTs
	INTRENTRY

#ifdef DIAGNOSTIC
	cmpl	$0, CPUVAR(WANT_PMAPLOAD)
	jz	1f
	pushl	$6f
	call	_C_LABEL(printf)
	addl	$4, %esp
1:
	movl	CPUVAR(ILEVEL),%ebx
	testl	%ebx,%ebx
	jz	1f
	pushl	$5f
	call	_C_LABEL(printf)
	addl	$4,%esp
#ifdef DDB
	int	$3
#endif
1:
#endif /* DIAGNOSTIC */
	movl	CPUVAR(CURLWP),%edx
	movl	%esp,L_MD_REGS(%edx)	# save pointer to frame
	movl	L_PROC(%edx),%edx
	pushl	%esp
	call	*P_MD_SYSCALL(%edx)	# get pointer to syscall() function
	addl	$4,%esp
.Lsyscall_checkast:
	/* Check for ASTs on exit to user mode. */
	cli
	CHECK_ASTPENDING(%eax)
	je	1f
	/* Always returning to user mode here. */
	CLEAR_ASTPENDING(%eax)
	sti
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	pushl	%esp
	call	_C_LABEL(trap)
	addl	$4,%esp
	jmp	.Lsyscall_checkast	/* re-check ASTs */
1:	CHECK_DEFERRED_SWITCH(%eax)
	jnz	9f
#ifndef DIAGNOSTIC
	INTRFASTEXIT
#else /* DIAGNOSTIC */
	cmpl	$IPL_NONE,CPUVAR(ILEVEL)
	jne	3f
	INTRFASTEXIT
3:	sti
	pushl	$4f
	call	_C_LABEL(printf)
	addl	$4,%esp
#ifdef DDB
	int	$3
#endif /* DDB */
	movl	$IPL_NONE,CPUVAR(ILEVEL)
	jmp	2b
4:	.asciz	"WARNING: SPL NOT LOWERED ON SYSCALL EXIT\n"
5:	.asciz	"WARNING: SPL NOT ZERO ON SYSCALL ENTRY\n"
6:	.asciz	"WARNING: WANT PMAPLOAD ON SYSCALL ENTRY\n"
#endif /* DIAGNOSTIC */
9:	sti
	call	_C_LABEL(pmap_load)
	jmp	.Lsyscall_checkast	/* re-check ASTs */

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
