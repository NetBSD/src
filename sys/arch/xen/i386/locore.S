/*	$NetBSD: locore.S,v 1.24.6.1 2007/07/11 20:03:30 mjf Exp $	*/
/*	NetBSD: locore.S,v 1.31 2004/08/26 10:12:33 junyoung Exp	*/

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 *	This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <machine/xen.h>

/*
 * Xen guest identifier and loader selection
 */
.section __xen_guest
#ifdef XEN3
	.ascii	"GUEST_OS=netbsd,GUEST_VER=3.0,XEN_VER=xen-3.0"
#if defined(DOM0OPS) || !defined(XEN_COMPAT_030001)
	.ascii  ",VIRT_BASE=0xc0000000" /* KERNBASE */
	.ascii  ",ELF_PADDR_OFFSET=0xc0000000" /* KERNBASE */
#else
	.ascii  ",VIRT_BASE=0xc0100000" /* KERNTEXTOFF */
	.ascii  ",ELF_PADDR_OFFSET=0xc0100000" /* KERNTEXTOFF */
#endif
	.ascii  ",VIRT_ENTRY=0xc0100000" /* KERNTEXTOFF */
#if !defined(XEN_COMPAT_030001)
	.ascii  ",HYPERCALL_PAGE=0x00000101"
		/* (???+HYPERCALL_PAGE_OFFSET)/PAGE_SIZE) */
#endif
#else /* !XEN3 */
	.ascii	"GUEST_OS=netbsd,GUEST_VER=2.0,XEN_VER=2.0"
#endif /* XEN3 */
	.ascii	",LOADER=generic"
#if (NKSYMS || defined(DDB) || defined(LKM)) && !defined(SYMTAB_SPACE)
	.ascii	",BSD_SYMTAB=yes"
#endif
	.byte	0


/*
 * Initialization
 */
	.data

	.globl	_C_LABEL(cpu)
	.globl	_C_LABEL(boothowto)
	.globl	_C_LABEL(bootinfo),_C_LABEL(atdevbase)
#ifdef COMPAT_OLDBOOT
	.globl	_C_LABEL(bootdev)
#endif
	.globl	_C_LABEL(proc0uarea),_C_LABEL(PDPpaddr)
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
_C_LABEL(atdevbase):	.long	0	# location of start of iomem in virtual
_C_LABEL(proc0uarea):	.long	0
_C_LABEL(PDPpaddr):	.long	0	# paddr of PDP, for libkvm
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
	.long tmpstk, __KERNEL_DS


#define	_RELOC(x)	((x))
#define	RELOC(x)	_RELOC(_C_LABEL(x))

	.text
	.globl	_C_LABEL(kernel_text)
	.set	_C_LABEL(kernel_text),KERNTEXTOFF

	.globl	start
start:
	cld
#ifdef XEN3
	movl	%esp, %ebx		# save start of available space
#else
	movl    %esi,%ebx               # save start_info pointer
#endif
	lss	tmpstk,%esp		# bootstrap stack end location

        /* Clear BSS first so that there are no surprises... */
	xorl	%eax,%eax
	movl	$RELOC(__bss_start),%edi
	movl	$RELOC(_end),%ecx
	subl	%edi,%ecx
	rep stosb

	movl	%ebx,RELOC(avail_start) # now init avail_start in bbs

	/* Copy the necessary stuff from start_info structure. */
        /* We need to copy shared_info early, so that sti/cli work */
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
	movl	RELOC(avail_start),%edi
	/* Calculate where to start the bootstrap tables. */
	movl	%edi,%esi

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
	movl	%ecx,RELOC(avail_start)
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
	subl	$KERNBASE,%ecx
	shrl	$PGSHIFT,%ecx
	leal	(SYSMAP)(%esi,%ecx,4),%ebx

	/* Map the kernel text read-only. */
	movl	%edx,%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PG_V|PG_KR),%eax
	fillkpt

	/* Map the data, BSS, and bootstrap tables read-write. */
	movl	RELOC(avail_start),%ecx
						    # end of tables
	subl	%edx,%ecx				# subtract end of text
	shrl	$PGSHIFT,%ecx
	leal	(PG_V|PG_KW)(%edx),%eax
	fillkpt

	/* map the shared_info page read/write */
	movl	RELOC(start_info_union)+START_INFO_SHARED_INFO,%eax
	orl	$(PG_V|PG_RW), %eax;
	movl	%eax, (%ebx)
	addl	$4,%ebx
#ifdef XEN3
	/* map the console page if not domain0 */
	movl	RELOC(start_info_union)+START_INFO_FLAGS,%eax
	testl	$SIF_INITDOMAIN, %eax;
	jnz	1f
	movl	RELOC(start_info_union)+START_INFO_CONSOLE_MFN,%eax
	shll	$PGSHIFT,%eax
	orl	$(PG_V|PG_RW), %eax;
	movl	%eax, (%ebx)
	addl	$4,%ebx
	movl	RELOC(start_info_union)+START_INFO_STORE_MFN,%eax
	shll	$PGSHIFT,%eax
	orl	$(PG_V|PG_RW), %eax;
	movl	%eax, (%ebx)
	addl	$4,%ebx
	jmp	2f
1:
#endif
#if defined(XEN3) || defined (DOM0OPS)
	/* map ISA hole */
	movl    $(IOM_BEGIN|PG_V|PG_KW/*|PG_N*/),%eax   # having these bits set
	movl    $(IOM_SIZE>>PGSHIFT),%ecx               # for this many pte s,
	fillkpt
#endif
#ifdef XEN3
2:
#endif

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

	/* Save phys. addr of PDP, for libkvm. */
	movl	%esi,RELOC(PDPpaddr)

    	call	xpmap_init

	/* cr0 is 0x8005003b */

	/* Relocate atdevbase. */
	movl	_C_LABEL(avail_start),%edx
	movl	%edx,_C_LABEL(HYPERVISOR_shared_info)
	addl	$PAGE_SIZE,%edx			# shared_inf
#ifdef XEN3
	/* set console and xenstore pages virtual address if not domain0 */
	movl	RELOC(start_info_union)+START_INFO_FLAGS,%eax
	testl	$SIF_INITDOMAIN, %eax;
	jnz	1f
	movl	%edx,_C_LABEL(xencons_interface)
	addl	$PAGE_SIZE,%edx			# xencons_interface
	movl	%edx,_C_LABEL(xenstore_interface)
	addl	$PAGE_SIZE,%edx			# xenstore_interface
1:
#endif
	movl	%edx,_C_LABEL(atdevbase)

	/* Set up bootstrap stack. */
	leal	(PROC0STACK)(%esi),%eax
	movl	%eax,_C_LABEL(proc0uarea)
	leal	(KSTACK_SIZE-FRAMESIZE)(%eax),%esp
	subl	$KERNBASE,%esi
	movl	%esi,(KSTACK_SIZE+PCB_CR3)(%eax)	# pcb->pcb_cr3
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

#if defined(XEN3) && !defined(XEN_COMPAT_030001)
/* space for the hypercall call page */
#define HYPERCALL_PAGE_OFFSET 0x1000
.org HYPERCALL_PAGE_OFFSET
ENTRY(hypercall_page)
.skip 0x1000
#endif

/*
 * void lwp_trampoline(void);
 * This is a trampoline function pushed onto the stack of a newly created
 * process in order to do some additional setup.  The trampoline is entered by
 * cpu_switch()ing to the process, so we abuse the callee-saved registers used
 * by cpu_switch() to store the information about the stub to call.
 * NOTE: This function does not have a normal calling sequence!
 */
/* LINTSTUB: Func: void lwp_trampoline(void) */
NENTRY(lwp_trampoline)
	pushl	%ebp
	xorl	%ebp,%ebp
	pushl	%eax
	call	_C_LABEL(lwp_startup)
	addl	$8,%esp
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

	.globl	_C_LABEL(uvmexp),_C_LABEL(panic)

#ifdef DIAGNOSTIC
NENTRY(switch_error)
	pushl	$1f
3:	call	_C_LABEL(panic)
	/* NOTREACHED */
1:	.asciz	"cpu_switch"
#endif /* DIAGNOSTIC */

/*
 * struct lwp *cpu_switchto(struct lwp *oldlwp, struct newlwp)
 *
 *	1. if (oldlwp != NULL), save its context.
 *	2. then, restore context of newlwp.
 *
 * Note that the stack frame layout is known to "struct switchframe"
 * in <machine/frame.h> and to the code in cpu_lwp_fork() which initializes
 * it for a new lwp.
 */
ENTRY(cpu_switchto)
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	16(%esp),%esi		# oldlwp
	movl	20(%esp),%edi		# newlwp

	testl	%esi,%esi
	jz	switch_skipsave

	/*
	 * Save old context.
	 */

	movl	L_ADDR(%esi),%eax
	movl	%esp,PCB_ESP(%eax)
	movl	%ebp,PCB_EBP(%eax)

switch_skipsave:

	/*
	 * Switch to newlwp's stack.
	 */

	CLI(%ebx)
	movl	L_ADDR(%edi),%ebx
	movl	PCB_EBP(%ebx),%ebp
	movl	PCB_ESP(%ebx),%esp

	/*
	 * Restore the rest of newlwp's context.
	 *
	 * Registers:
	 *   %ebx - new pcb
	 *   %edi - new lwp
	 */

	pushl	%ebx
	call	_C_LABEL(i386_switch_context)
	addl	$4,%esp

	movl	$0,CPUVAR(RESCHED)

	/*
	 *  Check for restartable atomic sequences (RAS)
	 */

	movl	L_PROC(%edi),%ebx
	cmpl	$0,P_RASLIST(%ebx)
	jne	check_ras

switch_return:
	STIC(%ebx)
	jz 1f
	call	_C_LABEL(stipending)
	testl	%eax,%eax
	jz 1f
	pushl  CPUVAR(ILEVEL)
	call	_C_LABEL(Xspllower) # process pending interrupts
1:
	movl	%esi,%eax	# return 'oldlwp'
	popl	%edi
	popl	%esi
	popl	%ebx
	ret

check_ras:
	movl	L_MD_REGS(%edi),%ebx
	pushl	TF_EIP(%ebx)
	pushl	L_PROC(%edi)
	call	_C_LABEL(ras_lookup)
	addl	$8,%esp
	cmpl	$-1,%eax
	je	switch_return
	movl	%eax,TF_EIP(%ebx)
	jmp	switch_return

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
.Lsyscall_checkast:
	/* Check for ASTs on exit to user mode. */
	CLI(%eax)
	movl	CPUVAR(CURLWP),%eax
	CHECK_ASTPENDING(%eax)
	je	1f
	/* Always returning to user mode here. */
	CLEAR_ASTPENDING(%eax)
	STI(%eax)
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	pushl	%esp
	call	_C_LABEL(trap)
	addl	$4,%esp
	jmp	.Lsyscall_checkast
1:	CHECK_DEFERRED_SWITCH(%eax)
	jnz	9f
	STIC(%eax)
	jz	14f
	call	_C_LABEL(stipending)
	testl	%eax,%eax
	jz	14f
	/* process pending interrupts */
	CLI(%eax)
	movl	CPUVAR(ILEVEL), %ebx
	movl	$.Lsyscall_resume, %esi # address to resume loop at
.Lsyscall_resume:
	movl	%ebx,%eax		# get cpl
	movl	CPUVAR(IUNMASK)(,%eax,4),%eax
	andl	CPUVAR(IPENDING),%eax	# any non-masked bits left?
	jz	17f
	bsrl	%eax,%eax
	btrl	%eax,CPUVAR(IPENDING)
	movl	CPUVAR(ISOURCES)(,%eax,4),%eax
	jmp	*IS_RESUME(%eax)
17:	movl	%ebx, CPUVAR(ILEVEL)	#restore cpl
	jmp	.Lsyscall_checkast
14:
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
9:	STI(%eax)
	call    _C_LABEL(pmap_load)
	jmp     .Lsyscall_checkast        /* re-check ASTs */

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
