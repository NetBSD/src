/*	$NetBSD: locore.S,v 1.2 2004/03/26 19:05:33 drochner Exp $	*/
/*	NetBSD: locore.S,v 1.24 2004/02/20 17:35:01 yamt Exp 	*/

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
#include "opt_xen.h"

#include "npx.h"
#include "assym.h"
#include "apm.h"
#include "lapic.h"
#include "ioapic.h"
#include "ksyms.h"

#include <sys/errno.h>
#include <sys/syscall.h>

#include <machine/cputypes.h>
#include <machine/param.h>
#include <machine/pte.h>
#include <machine/segments.h>
#include <machine/specialreg.h>
#include <machine/trap.h>
#include <machine/bootinfo.h>

#if NLAPIC > 0
#include <machine/i82489reg.h>
#endif

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

#define GET_CURPCB(reg)			movl	CPUVAR(CURPCB),reg	
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
 * PTmap is recursive pagemap at top of virtual address space.
 * Within PTmap, the page directory can be found (third indirection).
 *
 * XXX 4 == sizeof pde
 */
	.set	_C_LABEL(PTmap),(PDSLOT_PTE << PDSHIFT)
	.set	_C_LABEL(PTD),(_C_LABEL(PTmap) + PDSLOT_PTE * PAGE_SIZE)
	.set	_C_LABEL(PTDpde),(_C_LABEL(PTD) + PDSLOT_PTE * 4)

/*
 * APTmap, APTD is the alternate recursive pagemap.
 * It's used when modifying another process's page tables.
 *
 * XXX 4 == sizeof pde
 */
	.set	_C_LABEL(APTmap),(PDSLOT_APTE << PDSHIFT)
	.set	_C_LABEL(APTD),(_C_LABEL(APTmap) + PDSLOT_APTE * PAGE_SIZE)
	.set	_C_LABEL(APTDpde),(_C_LABEL(PTD) + PDSLOT_APTE * 4)


/*
 * Initialization
 */
	.data

	.globl	_C_LABEL(cpu)
	.globl	_C_LABEL(esym),_C_LABEL(boothowto)
	.globl	_C_LABEL(bootinfo),_C_LABEL(atdevbase)
#ifdef COMPAT_OLDBOOT
	.globl	_C_LABEL(bootdev)
#endif
	.globl	_C_LABEL(proc0paddr),_C_LABEL(PTDpaddr)
	.globl	_C_LABEL(biosbasemem),_C_LABEL(biosextmem)
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

#include <machine/xen.h>
#define __HYPERVISOR_yield		   8

	.space 512
tmpstk:
	.long tmpstk, __KERNEL_DS


#define	_RELOC(x)	((x))
#define	RELOC(x)	_RELOC(_C_LABEL(x))

	.text
	.globl	_C_LABEL(kernel_text)
	.set	_C_LABEL(kernel_text),KERNTEXTOFF

	.globl	start
start:
	cld

	lss	tmpstk,%esp		# bootstrap stack end location

	movl	%esi,%ebx		# save start_info pointer

#if (NKSYMS || defined(DDB) || defined(LKM)) && !defined(SYMTAB_SPACE)
	/* Save the symbol locations. */
/* XXX assym.h */
#define MOD_START   20
#define MOD_LEN     24
	movl	MOD_START(%ebx),%esi
	addl	MOD_LEN(%ebx),%esi
	movl	%esi,RELOC(esym)
#endif

        /* Clear BSS first so that there are no surprises... */
	xorl	%eax,%eax
	movl	$RELOC(__bss_start),%edi
	movl	$RELOC(_end),%ecx
	subl	%edi,%ecx
	rep stosb

	/* Copy the necessary stuff from start_info structure. */
        /* We need to copy shared_info early, so that sti/cli work */
	movl	%ebx,%esi
	movl	$RELOC(start_info_union),%edi
	movl	$128,%ecx
	rep movsl

    	/* (howto, [bootdev], bootinfo, basemem, extmem). */
	xorl	%eax,%eax
	movl	%eax,RELOC(boothowto)
#ifdef COMPAT_OLDBOOT
	movl	%eax,RELOC(bootdev)
#endif
	movl	$0x20000,%eax
	movl	%eax,RELOC(boothowto)

	/* First, reset the PSL. */
	pushl	$PSL_MBO
	popfl

	/* Clear segment registers; always null in proc0. */
	xorl	%eax,%eax
	movw	%ax,%fs
	movw	%ax,%gs
	decl	%eax
	movl	%eax,RELOC(cpu_info_primary)+CPU_INFO_LEVEL

	xorl	%eax,%eax
	cpuid
	movl	%eax,RELOC(cpu_info_primary)+CPU_INFO_LEVEL

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
	movl	%ecx,RELOC(gdt)
	addl	$PAGE_SIZE,%ecx
	movl	%ecx,RELOC(xpmap_phys_to_machine_mapping)
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
	movl	$KERNTEXTOFF,%eax
	movl	%eax,%ecx
	subl	$KERNBASE_LOCORE,%ecx
	shrl	$PGSHIFT,%ecx
	leal	(SYSMAP)(%esi,%ecx,4),%ebx

	/* Map the kernel text read-only. */
	movl	%edx,%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KR),%eax
	fillkpt

/* XXX assym.h */
#define NR_PAGES 0
	/* Map the data, BSS, and bootstrap tables read-write. */
	movl	$RELOC(start_info_union),%eax
	movl	NR_PAGES(%eax),%eax
	shll	$2,%eax
	movl	RELOC(xpmap_phys_to_machine_mapping),%ecx
	addl	%eax,%ecx
	addl	$PGOFSET,%ecx			# page align up
	andl	$~PGOFSET,%ecx
						    # end of tables
	subl	%edx,%ecx				# subtract end of text
	shrl	$PGSHIFT,%ecx
	leal	(PG_V|PG_KW)(%edx),%eax
	fillkpt

	movl	$0xffffffff,(%ebx)
	addl	$4,%ebx

/*
 * Construct a page table directory.
 */
	/* Map kernel PDEs. */
	movl	RELOC(nkpde),%ecx			# for this many pde s,
	leal	(PROC0PDIR+PDSLOT_KERN*4)(%esi),%ebx	# kernel pde offset
	leal	(SYSMAP+PG_V|PG_KW)(%esi),%eax		# pte for KPT in proc 0,
	fillkpt

	/* Install a PDE recursively mapping page directory as a page table! */
	leal	(PROC0PDIR+PG_V/*|PG_KW*/)(%esi),%eax	# pte for ptd
	movl	%eax,(PROC0PDIR+PDSLOT_PTE*4)(%esi)	# recursive PD slot

	/* Save phys. addr of PTD, for libkvm. */
	movl	%esi,RELOC(PTDpaddr)

    	call	xpmap_init

	movl	$__HYPERVISOR_fpu_taskswitch,%eax
	TRAP_INSTR

	/* cr0 is 0x8005003b */

	/* Relocate atdevbase. */
	movl	$_C_LABEL(start_info_union),%edx
	movl	NR_PAGES(%edx),%edx
	shll	$2,%edx
	movl	_C_LABEL(xpmap_phys_to_machine_mapping),%eax
	addl	%eax,%edx
	addl	$PGOFSET,%edx			# page align up
	andl	$~PGOFSET,%edx
	movl	%edx,_C_LABEL(HYPERVISOR_shared_info)
	addl	$PAGE_SIZE,%edx			# shared_inf
	movl	%edx,_C_LABEL(atdevbase)

	/* Set up bootstrap stack. */
	leal	(PROC0STACK)(%esi),%eax
	movl	%eax,_C_LABEL(proc0paddr)
	leal	(USPACE-FRAMESIZE)(%eax),%esp
	subl	$KERNTEXTOFF,%esi
	movl	%esi,PCB_CR3(%eax)	# pcb->pcb_cr3
	xorl	%ebp,%ebp               # mark end of frames

	movl	_C_LABEL(atdevbase),%eax
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
 * int kcopy(const void *from, void *to, size_t len);
 * Copy len bytes, abort on fault.
 */
/* LINTSTUB: Func: int kcopy(const void *from, void *to, size_t len) */
ENTRY(kcopy)
	pushl	%esi
	pushl	%edi
	GET_CURPCB(%eax)		# load curpcb into eax and set on-fault
	pushl	PCB_ONFAULT(%eax)
	movl	$_C_LABEL(kcopy_fault), PCB_ONFAULT(%eax)

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

	GET_CURPCB(%edx)		# XXX save curpcb?
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

	GET_CURPCB(%edx)
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
 * Default to the lowest-common-denominator.  We will improve it
 * later.
 */
#if defined(I386_CPU)
#define	DEFAULT_COPYOUT		_C_LABEL(i386_copyout)
#define	DEFAULT_COPYIN		_C_LABEL(i386_copyin)
#elif defined(I486_CPU)
#define	DEFAULT_COPYOUT		_C_LABEL(i486_copyout)
#define	DEFAULT_COPYIN		_C_LABEL(i386_copyin)
#elif defined(I586_CPU)
#define	DEFAULT_COPYOUT		_C_LABEL(i486_copyout)	/* XXX */
#define	DEFAULT_COPYIN		_C_LABEL(i386_copyin)	/* XXX */
#elif defined(I686_CPU)
#define	DEFAULT_COPYOUT		_C_LABEL(i486_copyout)	/* XXX */
#define	DEFAULT_COPYIN		_C_LABEL(i386_copyin)	/* XXX */
#endif

	.data

	.globl	_C_LABEL(copyout_func)
_C_LABEL(copyout_func):
	.long	DEFAULT_COPYOUT

	.globl	_C_LABEL(copyin_func)
_C_LABEL(copyin_func):
	.long	DEFAULT_COPYIN

	.text

/*
 * int copyout(const void *from, void *to, size_t len);
 * Copy len bytes into the user's address space.
 * see copyout(9)
 */
/* LINTSTUB: Func: int copyout(const void *kaddr, void *uaddr, size_t len) */
ENTRY(copyout)
	DO_DEFERRED_SWITCH(%eax)
	jmp	*_C_LABEL(copyout_func)

#if defined(I386_CPU)
/* LINTSTUB: Func: int i386_copyout(const void *kaddr, void *uaddr, size_t len) */
ENTRY(i386_copyout)
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

	GET_CURPCB(%edx)
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

3:	GET_CURPCB(%edx)
	movl	$_C_LABEL(copy_fault),PCB_ONFAULT(%edx)

	/* bcopy(%esi, %edi, %eax); */
	cld
	movl	%eax,%ecx
	shrl	$2,%ecx
	rep
	movsl
	movl	%eax,%ecx
	andl	$3,%ecx
	rep
	movsb

	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	ret
#endif /* I386_CPU */

#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
/* LINTSTUB: Func: int i486_copyout(const void *kaddr, void *uaddr, size_t len) */
ENTRY(i486_copyout)
	pushl	%esi
	pushl	%edi
	pushl	$0
	
	movl	16(%esp),%esi
	movl	20(%esp),%edi
	movl	24(%esp),%eax

	/*
	 * We check that the end of the destination buffer is not past the end
	 * of the user's address space.
	 */
	movl	%edi,%edx
	addl	%eax,%edx
	jc	_C_LABEL(copy_efault)
	cmpl	$VM_MAXUSER_ADDRESS,%edx
	ja	_C_LABEL(copy_efault)

	GET_CURPCB(%edx)
	movl	$_C_LABEL(copy_fault),PCB_ONFAULT(%edx)

	/* bcopy(%esi, %edi, %eax); */
	cld
	movl	%eax,%ecx
	shrl	$2,%ecx
	rep
	movsl
	movl	%eax,%ecx
	andl	$3,%ecx
	rep
	movsb

	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	ret
#endif /* I486_CPU || I586_CPU || I686_CPU */

/*
 * int copyin(const void *from, void *to, size_t len);
 * Copy len bytes from the user's address space.
 * see copyin(9)
 */
/* LINTSTUB: Func: int copyin(const void *uaddr, void *kaddr, size_t len) */
ENTRY(copyin)
	DO_DEFERRED_SWITCH(%eax)
	jmp	*_C_LABEL(copyin_func)

#if defined(I386_CPU) || defined(I486_CPU) || defined(I586_CPU) || \
    defined(I686_CPU)
/* LINTSTUB: Func: int i386_copyin(const void *uaddr, void *kaddr, size_t len) */
ENTRY(i386_copyin)
	pushl	%esi
	pushl	%edi
	GET_CURPCB(%eax)
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

	/* bcopy(%esi, %edi, %eax); */
	cld
	movl	%eax,%ecx
	shrl	$2,%ecx
	rep
	movsl
	movl	%eax,%ecx
	andl	$3,%ecx
	rep
	movsb

	GET_CURPCB(%edx)
	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	xorl	%eax,%eax
	ret
#endif /* I386_CPU || I486_CPU || I586_CPU || I686_CPU */

/* LINTSTUB: Ignore */
NENTRY(copy_efault)
	movl	$EFAULT,%eax

/*
 * kcopy_fault is used by kcopy and copy_fault is used by copyin/out.
 *
 * they're distinguished for lazy pmap switching.  see trap().
 */
/* LINTSTUB: Ignore */
NENTRY(kcopy_fault)
	GET_CURPCB(%edx)
	popl	PCB_ONFAULT(%edx)
	popl	%edi
	popl	%esi
	ret

/* LINTSTUB: Ignore */
NENTRY(copy_fault)
	GET_CURPCB(%edx)
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

	DO_DEFERRED_SWITCH(%eax)

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
	movl	$PAGE_SIZE,%ecx
	subl	%eax,%ecx		# ecx = PAGE_SIZE - (src % PAGE_SIZE)

	GET_CURPCB(%eax)
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
	movl	$PAGE_SIZE,%ecx
	testl	%edx,%edx
	jnz	1b

	/* edx is zero -- return ENAMETOOLONG. */
	movl	$ENAMETOOLONG,%eax
	jmp	copystr_return
#endif /* I386_CPU */

#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
5:	GET_CURPCB(%eax)
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

	DO_DEFERRED_SWITCH(%eax)

	GET_CURPCB(%ecx)
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
NENTRY(copystr_efault)
	movl	$EFAULT,%eax

/* LINTSTUB: Ignore */
NENTRY(copystr_fault)
copystr_return:
	/* Set *lencopied and return %eax. */
	GET_CURPCB(%ecx)
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
	DO_DEFERRED_SWITCH(%eax)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-4,%edx
	ja	_C_LABEL(fusuaddrfault)
	GET_CURPCB(%ecx)
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
	DO_DEFERRED_SWITCH(%eax)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-2,%edx
	ja	_C_LABEL(fusuaddrfault)
	GET_CURPCB(%ecx)
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
	cmpl	$TLBSTATE_VALID, CPUVAR(TLBSTATE)
	jnz	_C_LABEL(fusuaddrfault)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-2,%edx
	ja	_C_LABEL(fusuaddrfault)
	movl	CPUVAR(CURLWP),%ecx
	movl	L_ADDR(%ecx),%ecx
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
	DO_DEFERRED_SWITCH(%eax)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-1,%edx
	ja	_C_LABEL(fusuaddrfault)
	GET_CURPCB(%ecx)
	movl	$_C_LABEL(fusufault),PCB_ONFAULT(%ecx)
	movzbl	(%edx),%eax
	movl	$0,PCB_ONFAULT(%ecx)
	ret

/*
 * Handle faults from [fs]u*().  Clean up and return -1.
 */
/* LINTSTUB: Ignore */
NENTRY(fusufault)
	movl	$0,PCB_ONFAULT(%ecx)
	movl	$-1,%eax
	ret

/*
 * Handle faults from [fs]u*().  Clean up and return -1.  This differs from
 * fusufault() in that trap() will recognize it and return immediately rather
 * than trying to page fault.
 */
/* LINTSTUB: Ignore */
NENTRY(fusubail)
	movl	$0,PCB_ONFAULT(%ecx)
	movl	$-1,%eax
	ret

/*
 * Handle earlier faults from [fs]u*(), due to our of range addresses.
 */
/* LINTSTUB: Ignore */
NENTRY(fusuaddrfault)
	movl	$-1,%eax
	ret

/*
 * int suword(void *uaddr, long x);
 * Store an int in the user's address space.
 * see suword(9)
 */
/* LINTSTUB: Func: int suword(void *base, long c) */
ENTRY(suword)
	DO_DEFERRED_SWITCH(%eax)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-4,%edx
	ja	_C_LABEL(fusuaddrfault)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class)
	jne	2f
#endif /* I486_CPU || I586_CPU || I686_CPU */

	GET_CURPCB(%eax)
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
	GET_CURPCB(%ecx)
	testl	%eax,%eax
	jnz	_C_LABEL(fusufault)

1:	/* XXX also need to check the following 3 bytes for validity! */
#endif

2:	GET_CURPCB(%ecx)
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
	DO_DEFERRED_SWITCH(%eax)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-2,%edx
	ja	_C_LABEL(fusuaddrfault)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class)
	jne	2f
#endif /* I486_CPU || I586_CPU || I686_CPU */

	GET_CURPCB(%eax)
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
	GET_CURPCB(%ecx)
	testl	%eax,%eax
	jnz	_C_LABEL(fusufault)

1:	/* XXX also need to check the following byte for validity! */
#endif

2:	GET_CURPCB(%ecx)
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
	cmpl	$TLBSTATE_VALID, CPUVAR(TLBSTATE)
	jnz	_C_LABEL(fusuaddrfault)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-2,%edx
	ja	_C_LABEL(fusuaddrfault)
	movl	CPUVAR(CURLWP),%ecx
	movl	L_ADDR(%ecx),%ecx
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
	DO_DEFERRED_SWITCH(%eax)
	movl	4(%esp),%edx
	cmpl	$VM_MAXUSER_ADDRESS-1,%edx
	ja	_C_LABEL(fusuaddrfault)

#if defined(I386_CPU)
#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
	cmpl	$CPUCLASS_386,_C_LABEL(cpu_class)
	jne	2f
#endif /* I486_CPU || I586_CPU || I686_CPU */

	GET_CURPCB(%eax)	
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
	GET_CURPCB(%ecx)
	testl	%eax,%eax
	jnz	_C_LABEL(fusufault)

1:
#endif

2:	GET_CURPCB(%ecx)
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
 * void lgdt_finish(void);
 * Finish load a new GDT pointer (do any necessary cleanup).
 * XXX It's somewhat questionable whether reloading all the segment registers
 * is necessary, since the actual descriptor data is not changed except by
 * process creation and exit, both of which clean up via task switches.  OTOH,
 * this only happens at run time when the GDT is resized.
 */
/* LINTSTUB: Func: void lgdt_finish(void) */
NENTRY(lgdt_finish)
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
3:	call	_C_LABEL(panic)
	/* NOTREACHED */
1:	.asciz	"cpu_switch"
#endif /* DIAGNOSTIC */

/*
 * void cpu_switch(struct lwp *)
 * Find a runnable process and switch to it.  Wait if necessary.  If the new
 * process is the same as the old one, we short-circuit the context save and
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
	CLI(%ecx)			# splhigh doesn't do a cli
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
	CLI(%ecx)

	/* Restore stack pointers. */
	movl	PCB_ESP(%edi),%esp
	movl	PCB_EBP(%edi),%ebp

#ifdef XENDEBUG_LOW
	movl	_C_LABEL(xen_once),%ecx
	testl	%ecx,%ecx
	jz	1f
	movl	$2,%ecx
	movl	%ecx,_C_LABEL(xen_once)
1:
	call	_C_LABEL(xen_dbg2)
#endif

#ifdef XENDEBUG_LOW
	#call	_C_LABEL(xen_dbg2)
	movl	%ds,%ecx
	movl	%cs,%ebx
	pushl	%ecx
	pushl	%ebx
	call	_C_LABEL(xen_dbg3)
	addl	$8,%esp
#endif

	/* Switch TSS. */
	movl	PCB_TSS_ESP0(%edi),%ecx
	movl	PCB_TSS_SS0(%edi),%ebx
	movl	$__HYPERVISOR_stack_switch,%eax
	TRAP_INSTR

#ifdef XENDEBUG_LOW
	pushl	PCB_TSS_SS0(%edi)
	pushl	PCB_TSS_ESP0(%edi)
	call	_C_LABEL(xen_dbg1)
	addl	$8,%esp
#endif

	/* Reset FPU state if necessary. */
	movl	PCB_CR0(%edi),%ecx
	testl	$CR0_TS,%ecx
	jz	1f
	andl	$~CR0_TS,%ecx
	movl	%ecx,PCB_CR0(%edi)
	movl	$__HYPERVISOR_fpu_taskswitch,%eax
	TRAP_INSTR
1:

	/* Record new pcb. */
	SET_CURPCB(%edi)

	xorl	%esi,%esi
	STI(%eax)
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
	STIC(%eax)
    	jz	4f
	call	_C_LABEL(stipending)
	testl	%eax,%eax
	jz	4f
	pushl	$IPL_NONE
	call	_C_LABEL(Xspllower)
	addl	$4,%esp
4:
	call	_C_LABEL(uvm_pageidlezero)
	CLI(%eax)
	cmpl	$0,_C_LABEL(sched_whichqs)
	jnz	idle_exit
idle_loop:
	/* Try to zero some pages. */
	movl	_C_LABEL(uvm)+UVM_PAGE_IDLE_ZERO,%ecx
	testl	%ecx,%ecx
	jnz	idle_zero
	STIC(%eax)
    	jz	4f
	call	_C_LABEL(stipending)
	testl	%eax,%eax
	jz	4f
	pushl	$IPL_NONE
	call	_C_LABEL(Xspllower)
	addl	$4,%esp
	jmp	idle_start
4:
	movl	$__HYPERVISOR_yield,%eax
	TRAP_INSTR
NENTRY(mpidle)
idle_start:	
	CLI(%eax)
	cmpl	$0,_C_LABEL(sched_whichqs)
	jz	idle_loop
idle_exit:	
	movl	$IPL_HIGH,CPUVAR(ILEVEL)		# splhigh
	STI(%eax)
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)	
	call	_C_LABEL(sched_lock_idle)
#endif
	movl	_C_LABEL(sched_whichqs),%ecx
	bsfl	%ecx,%ebx
	jz	idle_unlock

#ifdef XENDEBUG_LOW
	pushl	%ecx
	call	_C_LABEL(xen_dbg1)
	xorl	%ecx,%ecx
	movl	%ecx,_C_LABEL(xen_once)
	popl	%ecx
#endif
switch_dequeue:		
	/* 
	 * we're running at splhigh(), but it's otherwise okay to take
	 * interrupts here. 
	 */
	STI(%edi)
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
	CLI(%eax)
	movl	L_ADDR(%edi),%esi

	/* Restore stack pointers. */
	movl	PCB_ESP(%esi),%esp
	movl	PCB_EBP(%esi),%ebp

#if 0
	/* Don't bother with the rest if switching to a system process. */
	testl	$P_SYSTEM,L_FLAG(%edi);	XXX NJWLWP lwp's don't have P_SYSTEM!
	jnz	switch_restored
#endif

	/* Switch TSS. */
	movl	PCB_TSS_ESP0(%esi),%ecx
	movl	PCB_TSS_SS0(%esi),%ebx
	movl	$__HYPERVISOR_stack_switch,%eax
	TRAP_INSTR

	pushl	%edi
	call	_C_LABEL(pmap_activate)		# pmap_activate(p)
	addl	$4,%esp

#if 0
switch_restored:
#endif
	/* Reset FPU state if necessary. */
	movl	PCB_CR0(%esi),%ecx
	testl	$CR0_TS,%ecx
	jz	1f
	andl	$~CR0_TS,%ecx
	movl	%ecx,PCB_CR0(%esi)
	movl	$__HYPERVISOR_fpu_taskswitch,%eax
	TRAP_INSTR
1:
	/* Record new pcb. */
	SET_CURPCB(%esi)

	/* Interrupts are okay again. */
	STI(%edi)

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
	pushl	$IPL_NONE		# spl0()
	call	_C_LABEL(Xspllower)	# process pending interrupts
	addl	$4,%esp
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
	STI(%eax)

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
	.globl  _C_LABEL(uvmspace_free),_C_LABEL(kernel_map)
	.globl	_C_LABEL(uvm_km_free),_C_LABEL(tss_free)
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
	CLI(%eax)

	/* Restore stack pointers. */
	movl	PCB_ESP(%esi),%esp
	movl	PCB_EBP(%esi),%ebp

	/* Switch TSS. */
    	movl	PCB_TSS_ESP0(%esi),%ecx
	movl	PCB_TSS_SS0(%esi),%ebx
	movl	$__HYPERVISOR_stack_switch,%eax
	TRAP_INSTR

	/* Reset FPU state if necessary. */
	movl	PCB_CR0(%esi),%ecx
	testl	$CR0_TS,%ecx
	jz	1f
	andl	$~CR0_TS,%ecx
	movl	%ecx,PCB_CR0(%esi)
	movl	$__HYPERVISOR_fpu_taskswitch,%eax
	TRAP_INSTR
1:

	/* Record new pcb. */
	SET_CURPCB(%esi)

	/* Interrupts are okay again. */
	STI(%eax)

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
	cmpl    $0, CPUVAR(WANT_PMAPLOAD)
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
syscall_checkast:
	/* Check for ASTs on exit to user mode. */
	CLI(%eax)
	CHECK_ASTPENDING(%eax)
	je	1f
	/* Always returning to user mode here. */
	CLEAR_ASTPENDING(%eax)
	STI(%eax)
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	pushl	%esp
	call	_C_LABEL(trap)
	addl	$4,%esp
	jmp	syscall_checkast
1:	STI(%eax)
	CHECK_DEFERRED_SWITCH(%eax)
	jnz	9f
#ifndef DIAGNOSTIC
	INTRFASTEXIT
#else /* DIAGNOSTIC */
	cmpl	$IPL_NONE,CPUVAR(ILEVEL)
	jne	3f
	INTRFASTEXIT
3:	pushl	$4f
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
9:	call    _C_LABEL(pmap_load)
	jmp     syscall_checkast        /* re-check ASTs */

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
