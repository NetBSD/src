/*	$NetBSD: locore.S,v 1.189 2022/06/12 11:36:42 bouyer Exp $	*/

/*
 * Copyright-o-rama!
 */

/*
 * Copyright (c) 1998, 2000, 2004, 2006, 2007, 2009, 2016
 * The NetBSD Foundation, Inc., All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, by Andrew Doran and by Maxime Villard.
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

#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: locore.S,v 1.189 2022/06/12 11:36:42 bouyer Exp $");

#include "opt_copy_symtab.h"
#include "opt_ddb.h"
#include "opt_modular.h"
#include "opt_multiboot.h"
#include "opt_realmem.h"
#include "opt_xen.h"

#include "assym.h"
#include "lapic.h"
#include "ioapic.h"
#include "ksyms.h"

#include <sys/errno.h>
#include <sys/syscall.h>

#include <machine/segments.h>
#include <machine/specialreg.h>
#include <machine/trap.h>
#include <machine/i82489reg.h>
#include <machine/frameasm.h>
#include <machine/i82489reg.h>
#include <machine/cputypes.h>

#ifndef XENPV
#include <machine/multiboot.h>
#endif

/* Get definitions for IOM_BEGIN, IOM_END, and IOM_SIZE */
#include <dev/isa/isareg.h>

#ifndef XENPV
#define	_RELOC(x)	((x) - KERNBASE)
#else
#define	_RELOC(x)	((x))
#endif /* XENPV */
#define	RELOC(x)	_RELOC(_C_LABEL(x))

/* 32bit version of PTE_NX */
#define PTE_NX32	0x80000000

#ifndef PAE
#define	PROC0_PDIR_OFF	0
#else
#define PROC0_L3_OFF	0
#define PROC0_PDIR_OFF	1 * PAGE_SIZE
#endif

#define	PROC0_STK_OFF	(PROC0_PDIR_OFF + PDP_SIZE * PAGE_SIZE)
#define	PROC0_PTP1_OFF	(PROC0_STK_OFF + UPAGES * PAGE_SIZE)

/*
 * fillkpt - Fill in a kernel page table
 *	eax = pte (page frame | control | status)
 *	ebx = page table address
 *	ecx = number of pages to map
 *
 * For PAE, each entry is 8 bytes long: we must set the 4 upper bytes to 0.
 * This is done by the first instruction of fillkpt. In the non-PAE case, this
 * instruction just clears the page table entry.
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
 * killkpt - Destroy a kernel page table
 *	ebx = page table address
 *	ecx = number of pages to destroy
 */
#define killkpt \
1:	movl	$0,(PDE_SIZE-4)(%ebx)	;	/* upper bits (for PAE) */ \
	movl	$0,(%ebx)		; \
	addl	$PDE_SIZE,%ebx		; \
	loop	1b			;


#ifdef XEN
#define __ASSEMBLY__
#include <xen/include/public/elfnote.h>
#include <xen/include/public/xen.h>

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
	ELFNOTE(Xen, XEN_ELFNOTE_VIRT_BASE,      .long,  KERNBASE)
#ifdef XENPV
	ELFNOTE(Xen, XEN_ELFNOTE_PADDR_OFFSET,   .long,  KERNBASE)
	ELFNOTE(Xen, XEN_ELFNOTE_ENTRY,          .long,  start)
#else
	ELFNOTE(Xen, XEN_ELFNOTE_PADDR_OFFSET,   .long,  0)
	ELFNOTE(Xen, XEN_ELFNOTE_PHYS32_ENTRY,   .long,  RELOC(start_xenpvh))
#endif /* XENPV */
	ELFNOTE(Xen, XEN_ELFNOTE_HYPERCALL_PAGE, .long,  hypercall_page)
	ELFNOTE(Xen, XEN_ELFNOTE_HV_START_LOW,   .long,  HYPERVISOR_VIRT_START)
	ELFNOTE(Xen, XEN_ELFNOTE_FEATURES,       .asciz, "writable_descriptor_tables|auto_translated_physmap|supervisor_mode_kernel|hvm_callback_vector")
	ELFNOTE(Xen, XEN_ELFNOTE_PAE_MODE,       .asciz, "yes")
	ELFNOTE(Xen, XEN_ELFNOTE_L1_MFN_VALID,   .quad,  PTE_P, PTE_P)\
	ELFNOTE(Xen, XEN_ELFNOTE_LOADER,         .asciz, "generic")     
	ELFNOTE(Xen, XEN_ELFNOTE_SUSPEND_CANCEL, .long,  0)
#if NKSYMS > 0 || defined(DDB) || defined(MODULAR)
	ELFNOTE(Xen, XEN_ELFNOTE_BSD_SYMTAB,     .asciz, "yes")
#endif
#endif  /* XEN */

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
	.globl	_C_LABEL(lwp0uarea)
	.globl	_C_LABEL(gdt)
	.globl	_C_LABEL(idt)

	.type	_C_LABEL(tablesize), @object
_C_LABEL(tablesize):	.long	0
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
	.type	_C_LABEL(atdevbase), @object
LABEL(atdevbase)	.long	0	/* location of start of iomem in virt */
END(atdevbase)
	.type	_C_LABEL(lwp0uarea), @object
LABEL(lwp0uarea)	.long	0
END(lwp0uarea)
	.type	_C_LABEL(PDPpaddr), @object
LABEL(PDPpaddr)		.long	0	/* paddr of PDP, for libkvm */
END(PDPpaddr)

	/* Space for the temporary stack */
	.globl	_C_LABEL(tmpstk)
	.size	tmpstk, tmpstk - .
	.space	512
tmpstk:
#ifdef XENPV
	.align 		PAGE_SIZE, 0x0	/* Align on page boundary */
LABEL(tmpgdt)
	.space 		PAGE_SIZE	/* Xen expects a page */
END(tmpgdt)
#endif /* XENPV */

	.text
	.globl	_C_LABEL(kernel_text)
	.set	_C_LABEL(kernel_text),KERNTEXTOFF

ENTRY(start)
#ifndef XENPV

	/* Warm boot */
	movw	$0x1234,0x472

#if defined(MULTIBOOT)
	jmp	1f

	.align	4
	.globl	Multiboot_Header
_C_LABEL(Multiboot_Header):
#define MULTIBOOT_HEADER_FLAGS	(MULTIBOOT_HEADER_WANT_MEMORY)
	.long	MULTIBOOT_HEADER_MAGIC
	.long	MULTIBOOT_HEADER_FLAGS
	.long	-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

	.align	8
	.globl	Multiboot2_Header
_C_LABEL(Multiboot2_Header):
	.long	MULTIBOOT2_HEADER_MAGIC
	.long	MULTIBOOT2_ARCHITECTURE_I386
	.long	Multiboot2_Header_end - Multiboot2_Header
	.long	-(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_ARCHITECTURE_I386 \
		+ (Multiboot2_Header_end - Multiboot2_Header))

	.long	1	/* MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST */
	.long	12	/* sizeof(multiboot_header_tag_information_request) */
			/* + sizeof(uint32_t) * requests */
	.long	4	/* MULTIBOOT_TAG_TYPE_BASIC_MEMINFO */
	.long	0	/* pad for 8 bytes alignment */

	.long	8	/* MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS_EFI32 */
	.long	12	/* sizeof(struct multiboot_tag_efi32) */
	.long	efi_multiboot2_loader - KERNBASE
	.long   0	/* pad for 8 bytes alignment */

#if notyet 
	/* 
	 * Could be used to get an early console for debug,
	 * but this is broken.
	 */
	.long	7	/* MULTIBOOT_HEADER_TAG_EFI_BS */
	.long	8	/* sizeof(struct multiboot_tag) */
#endif

	.long	0	/* MULTIBOOT_HEADER_TAG_END */
	.long	8	/* sizeof(struct multiboot_tag) */
	.globl	Multiboot2_Header_end
_C_LABEL(Multiboot2_Header_end):

1:
	/* Check if we are being executed by a Multiboot-compliant boot
	 * loader. */
	cmpl	$MULTIBOOT_INFO_MAGIC,%eax
	je	multiboot1_loader

	cmpl	$MULTIBOOT2_BOOTLOADER_MAGIC,%eax
	je	multiboot2_loader

	jmp	1f

multiboot1_loader:
	/*
	 * Indeed, a multiboot-compliant boot loader executed us. We switch
	 * to the temporary stack, and copy the received Multiboot information
	 * structure into kernel's data space to process it later -- after we
	 * are relocated. It will be safer to run complex C code than doing it
	 * at this point.
	 */
	movl	$_RELOC(tmpstk),%esp
	pushl	%ebx		/* Address of Multiboot information */
	call	_C_LABEL(multiboot1_pre_reloc)
	addl	$4,%esp
	jmp	2f

efi_multiboot2_loader:
	/*
	 * EFI32 multiboot2 entry point. We are left here without
	 * stack and with no idea of where we were loaded in memory.
	 * The only inputs are 
	 * %eax MULTIBOOT2_BOOTLOADER_MAGIC
	 * %ebx pointer to multiboot_info
	 *
	 * Here we will copy the kernel to 0x100000 (KERNTEXTOFF - KERNBASE)
	 * as almost all the code in locore.S assume it is there. Once done, 
	 * we join the main start code .This is derived from
	 * src/sys/arch/i386/stand/efiboot/bootia32/startprog32.S
	 */

	cli

	/*
	 * Discover our load address and store it in %edx
	 */
	movl	$_RELOC(tmpstk),%esp
	call	next
next:	popl	%edx
	subl	$(next - efi_multiboot2_loader), %edx

	/*
	 * Save multiboot_info for later. We cannot use
	 * temporary stack for that since we are going to
	 * overwrite it.
	 */
	movl	%ebx, (multiboot2_info_ptr - efi_multiboot2_loader)(%edx)
 
	/*
	 * Get relocated multiboot2_loader entry point in %ebx
	 */
	movl	$(KERNTEXTOFF - KERNBASE), %ebx
	addl	$(multiboot2_loader - start), %ebx

        /* Copy kernel */
        movl    $(KERNTEXTOFF - KERNBASE), %edi		/* dest */ 
        movl    %edx, %esi
	subl	$(efi_multiboot2_loader - start), %esi	/* src */
	movl	$(__kernel_end - kernel_text), %ecx	/* size */
#if defined(NO_OVERLAP)
        movl    %ecx, %eax
#else 
        movl    %edi, %eax
        subl    %esi, %eax
        cmpl    %ecx, %eax      /* overlapping? */
        movl    %ecx, %eax
        jb      .Lbackwards
#endif
        /* nope, copy forwards. */
        shrl    $2, %ecx        /* copy by words */
        rep
        movsl
        and     $3, %eax        /* any bytes left? */
        jnz     .Ltrailing
        jmp     .Lcopy_done

.Ltrailing:
        cmp     $2, %eax
        jb      11f
        movw    (%esi), %ax
        movw    %ax, (%edi)
        je      .Lcopy_done
        movb    2(%esi), %al
        movb    %al, 2(%edi)
        jmp     .Lcopy_done
11:     movb    (%esi), %al
        movb    %al, (%edi)
        jmp     .Lcopy_done

#if !defined(NO_OVERLAP)
.Lbackwards:
        addl    %ecx, %edi      /* copy backwards. */
        addl    %ecx, %esi
        and     $3, %eax        /* any fractional bytes? */
        jnz     .Lback_align
.Lback_aligned:
        shrl    $2, %ecx
        subl    $4, %esi
        subl    $4, %edi
        std
        rep
        movsl
        cld
        jmp     .Lcopy_done

.Lback_align:
        sub     %eax, %esi
        sub     %eax, %edi
        cmp     $2, %eax
        jb      11f
        je      12f
        movb    2(%esi), %al
        movb    %al, 2(%edi)
12:     movw    (%esi), %ax
        movw    %ax, (%edi)
        jmp     .Lback_aligned
11:     movb    (%esi), %al
        movb    %al, (%edi)
        jmp     .Lback_aligned
#endif
        /* End of copy kernel */
.Lcopy_done:
	cld			/* LynxOS depends on it */

	/* Prepare jump address */
	lea	(efi_multiboot2_loader32a - efi_multiboot2_loader)(%edx), %eax
	movl	%eax, (efi_multiboot2_loader32r - efi_multiboot2_loader)(%edx)

	/* Setup GDT */
	lea	(gdt - efi_multiboot2_loader)(%edx), %eax
	movl	%eax, (gdtrr - efi_multiboot2_loader)(%edx)
	lgdt	(gdtr - efi_multiboot2_loader)(%edx)

	/* Jump to set %cs */
	ljmp	*(efi_multiboot2_loader32r - efi_multiboot2_loader)(%edx)

	.align	4
efi_multiboot2_loader32a:
	movl	$0x10, %eax	/* #define DATA_SEGMENT	0x10 */
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* Already set new stack pointer */
	movl	%esp, %ebp

	/* Disable Paging in CR0 */
	movl	%cr0, %eax
	andl	$(~CR0_PG), %eax
	movl	%eax, %cr0

	/* Disable PAE in CR4 */
	movl	%cr4, %eax
	andl	$(~CR4_PAE), %eax
	movl	%eax, %cr4

	jmp	efi_multiboot2_loader32b

	.align	4
efi_multiboot2_loader32b:
	xor	%eax, %eax
	movl	%ebx, (efi_multiboot2_loader32r - efi_multiboot2_loader)(%edx)
	/*
	 * Reload multiboot info from target location
	 */
	movl	_RELOC(multiboot2_info_ptr), %ebx
	ljmp	*(efi_multiboot2_loader32r - efi_multiboot2_loader)(%edx)

	.align	16
efi_multiboot2_loader32r:
	.long	0
	.long	0x08	/* #define	CODE_SEGMENT	0x08 */
	.align	16
gdt:
	.long	0, 0
	.byte	0xff, 0xff, 0x00, 0x00, 0x00, 0x9f, 0xcf, 0x00
	.byte	0xff, 0xff, 0x00, 0x00, 0x00, 0x93, 0xcf, 0x00
gdtr:
	.word	gdtr - gdt
gdtrr:
	.quad  	0
multiboot2_info_ptr:
	.long	0

	.align 16
multiboot2_loader:
	movl    $_RELOC(tmpstk),%esp
	pushl	%ebx		/* Address of Multiboot information */
	call	_C_LABEL(multiboot2_pre_reloc)
	addl	$4,%esp
	jmp	.Lstart_common
#endif /* MULTIBOOT */

1:
	/*
	 * At this point, we know that a NetBSD-specific boot loader
	 * booted this kernel.
	 *
	 * Load parameters from the stack (32 bits):
	 *     boothowto, [bootdev], bootinfo, esym, biosextmem, biosbasemem
	 * We are not interested in 'bootdev'.
	 */

	addl	$4,%esp		/* Discard return address to boot loader */
	call	_C_LABEL(native_loader)
	addl	$24,%esp

.Lstart_common:
	/* First, reset the PSL. */
	pushl	$PSL_MBO
	popfl

	/* Clear segment registers; always null in proc0. */
	xorl	%eax,%eax
	movw	%ax,%fs
	movw	%ax,%gs

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
	movl	$CPU_NX586,RELOC(cputype)
	jmp	2f

is386:
	movl	$CPU_386,RELOC(cputype)
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
is486:	movl	$CPU_486,RELOC(cputype)
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
	movl	$CPU_6x86,RELOC(cputype)	/* set CPU type */
	/*
	 * Check for Cyrix 486 CPU by seeing if the flags change during a
	 * divide. This is documented in the Cx486SLC/e SMM Programmer's
	 * Guide.
	 */
	xorl	%edx,%edx
	cmpl	%edx,%edx		/* set flags to known state */
	pushfl
	popl	%ecx			/* store flags in ecx */
	movl	$-1,%eax
	movl	$4,%ebx
	divl	%ebx			/* do a long division */
	pushfl
	popl	%eax
	xorl	%ecx,%eax		/* are the flags different? */
	testl	$0x8d5,%eax		/* only check C|PF|AF|Z|N|V */
	jne	2f			/* yes; must be Cyrix 6x86 CPU */
	movl	$CPU_486DLC,RELOC(cputype) 	/* set CPU type */

#ifndef CYRIX_CACHE_WORKS
	/* Disable caching of the ISA hole only. */
	invd
	movb	$CCR0,%al	/* Configuration Register index (CCR0) */
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
	invd			/* Start with guaranteed clean cache */
	movb	$CCR0,%al	/* Configuration Register index (CCR0) */
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

	/*
	 * Retrieve the NX/XD flag. We use the 32bit version of PTE_NX.
	 */
	movl	$0x80000001,%eax
	cpuid
	andl	$CPUID_NOX,%edx
	jz	no_NOX
	movl	$PTE_NX32,RELOC(nox_flag)
no_NOX:

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
	 *
	 * XXX: the gdt is in the carcass of the boot program so clearing
	 * the rest of memory is still not possible.
	 */
	movl	$_RELOC(tmpstk),%esp

/*
 * There are two different layouts possible, depending on whether PAE is
 * enabled or not.
 *
 * If PAE is not enabled, there are two levels of pages: PD -> PT. They will
 * be referred to as: L2 -> L1. L2 is 1 page long. The BOOTSTRAP TABLES have
 * the following layout:
 * 	+-----+------------+----+
 * 	| L2 -> PROC0 STK -> L1 |
 * 	+-----+------------+----+
 *
 * If PAE is enabled, there are three levels of pages: PDP -> PD -> PT. They
 * will be referred to as: L3 -> L2 -> L1. L3 is 1 page long, L2 is 4 page
 * long. The BOOTSTRAP TABLES have the following layout:
 * 	+-----+-----+------------+----+
 * 	| L3 -> L2 -> PROC0 STK -> L1 |
 * 	+-----+-----+------------+----+
 *
 * Virtual address space of the kernel in both cases:
 * +------+--------+------+-----+--------+---------------------+-----------
 * | TEXT | RODATA | DATA | BSS | [SYMS] | [PRELOADED MODULES] | BOOTSTRAP
 * +------+--------+------+-----+--------+---------------------+-----------
 *                             (1)      (2)                   (3)
 *
 * -------+-------------+
 * TABLES | ISA I/O MEM |
 * -------+-------------+
 *       (4)
 *
 * PROC0 STK is obviously not linked as a page level. It just happens to be
 * caught between L2 and L1.
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
	subl	$KERNBASE,%eax
	movl	%eax,%edi
1:
#endif

	/* Skip over any modules/blobs; brings us on (3). */
	movl	RELOC(eblob),%eax
	testl	%eax,%eax
	jz	1f
	subl	$KERNBASE,%eax
	movl	%eax,%edi
1:

	/* We are on (3). Align up for BOOTSTRAP TABLES. */
	movl	%edi,%esi
	addl	$PGOFSET,%esi
	andl	$~PGOFSET,%esi

	/* nkptp[1] = (esi + ~L2_FRAME) >> L2_SHIFT + 1; */
	movl	%esi,%eax
	addl	$~L2_FRAME,%eax
	shrl	$L2_SHIFT,%eax
	incl	%eax		/* one more PTP for VAs stolen by bootstrap */
1:	movl	%eax,RELOC(nkptp)+1*4

	/* tablesize = (PDP_SIZE + UPAGES + nkptp[1]) << PGSHIFT; */
	addl	$(PDP_SIZE+UPAGES),%eax
#ifdef PAE
	incl	%eax 		/* one more page for L3 */
	shll	$PGSHIFT+1,%eax	/* PTP tables are twice larger with PAE */
#else
	shll	$PGSHIFT,%eax
#endif
	movl	%eax,RELOC(tablesize)

	/* Ensure that nkptp[1] covers BOOTSTRAP TABLES, ie:
	 * (esi + tablesize) >> L2_SHIFT + 1 < nkptp[1] */
	addl	%esi,%eax
	addl	$~L2_FRAME,%eax
	shrl	$L2_SHIFT,%eax
	incl	%eax
	cmpl	%eax,RELOC(nkptp)+1*4
	jnz	1b

	/* Now, zero out the BOOTSTRAP TABLES (before filling them in). */
	movl	%esi,%edi
	xorl	%eax,%eax
	cld
	movl	RELOC(tablesize),%ecx
	shrl	$2,%ecx
	rep
	stosl				/* copy eax -> edi */

/*
 * Build the page tables and levels. We go from L1 to L2/L3, and link the levels
 * together. Note: RELOC computes &addr - KERNBASE in 32 bits; the value can't
 * be > 4G, or we can't deal with it anyway, since we are in 32bit mode.
 */
	/*
	 * Build L1.
	 */
	leal	(PROC0_PTP1_OFF)(%esi),%ebx

	/* Skip the area below the kernel text. */
	movl	$(KERNTEXTOFF - KERNBASE),%ecx
	shrl	$PGSHIFT,%ecx
	fillkpt_blank

	/* Map the kernel text RX. */
	movl	$(KERNTEXTOFF - KERNBASE),%eax	/* start of TEXT */
	movl	$RELOC(__rodata_start),%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PTE_P),%eax
	fillkpt

	/* Map the kernel rodata R. */
	movl	$RELOC(__rodata_start),%eax
	movl	$RELOC(__data_start),%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PTE_P),%eax
	fillkpt_nox

	/* Map the kernel data+bss RW. */
	movl	$RELOC(__data_start),%eax
	movl	$RELOC(__kernel_end),%ecx
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PTE_P|PTE_W),%eax
	fillkpt_nox

	/* Map [SYMS]+[PRELOADED MODULES] RW. */
	movl	$RELOC(__kernel_end),%eax
	movl	%esi,%ecx		/* start of BOOTSTRAP TABLES */
	subl	%eax,%ecx
	shrl	$PGSHIFT,%ecx
	orl	$(PTE_P|PTE_W),%eax
	fillkpt_nox

	/* Map the BOOTSTRAP TABLES RW. */
	movl	%esi,%eax		/* start of BOOTSTRAP TABLES */
	movl	RELOC(tablesize),%ecx	/* length of BOOTSTRAP TABLES */
	shrl	$PGSHIFT,%ecx
	orl	$(PTE_P|PTE_W),%eax
	fillkpt_nox

	/* We are on (4). Map ISA I/O MEM RW. */
	movl	$IOM_BEGIN,%eax
	movl	$IOM_SIZE,%ecx	/* size of ISA I/O MEM */
	shrl	$PGSHIFT,%ecx
	orl	$(PTE_P|PTE_W/*|PTE_PCD*/),%eax
	fillkpt_nox

	/*
	 * Build L2 for identity mapping. Linked to L1.
	 */
	leal	(PROC0_PDIR_OFF)(%esi),%ebx
	leal	(PROC0_PTP1_OFF)(%esi),%eax
	orl	$(PTE_P|PTE_W),%eax
	movl	RELOC(nkptp)+1*4,%ecx
	fillkpt

	/* Set up L2 entries for actual kernel mapping */
	leal	(PROC0_PDIR_OFF + L2_SLOT_KERNBASE * PDE_SIZE)(%esi),%ebx
	leal	(PROC0_PTP1_OFF)(%esi),%eax
	orl	$(PTE_P|PTE_W),%eax
	movl	RELOC(nkptp)+1*4,%ecx
	fillkpt

	/* Install recursive top level PDE */
	leal	(PROC0_PDIR_OFF + PDIR_SLOT_PTE * PDE_SIZE)(%esi),%ebx
	leal	(PROC0_PDIR_OFF)(%esi),%eax
	orl	$(PTE_P|PTE_W),%eax
	movl	$PDP_SIZE,%ecx
	fillkpt_nox

#ifdef PAE
	/*
	 * Build L3. Linked to L2.
	 */
	leal	(PROC0_L3_OFF)(%esi),%ebx
	leal	(PROC0_PDIR_OFF)(%esi),%eax
	orl	$(PTE_P),%eax
	movl	$PDP_SIZE,%ecx
	fillkpt

	/* Enable PAE mode */
	movl	%cr4,%eax
	orl	$CR4_PAE,%eax
	movl	%eax,%cr4
#endif

	/* Save physical address of L2. */
	leal	(PROC0_PDIR_OFF)(%esi),%eax
	movl	%eax,RELOC(PDPpaddr)

	/*
	 * Startup checklist:
	 * 1. Load %cr3 with pointer to L2 (or L3 for PAE).
	 */
	movl	%esi,%eax
	movl	%eax,%cr3

	/*
	 * 2. Set NOX in EFER, if available.
	 */
	movl	RELOC(nox_flag),%ebx
	cmpl	$0,%ebx
	je 	skip_NOX
	movl	$MSR_EFER,%ecx
	rdmsr
	xorl	%eax,%eax
	orl	$(EFER_NXE),%eax
	wrmsr
skip_NOX:

	/*
	 * 3. Enable paging and the rest of it.
	 */
	movl	%cr0,%eax
	orl	$(CR0_PE|CR0_PG|CR0_NE|CR0_TS|CR0_MP|CR0_WP|CR0_AM),%eax
	movl	%eax,%cr0

	pushl	$begin			/* jump to high mem */
	ret

begin:
	/*
	 * We have arrived. There's no need anymore for the identity mapping in
	 * low memory, remove it.
	 */
	movl	_C_LABEL(nkptp)+1*4,%ecx
	leal	(PROC0_PDIR_OFF)(%esi),%ebx	/* old, phys address of PDIR */
	addl	$(KERNBASE), %ebx		/* new, virt address of PDIR */
	killkpt

	/* Relocate atdevbase. */
	movl	$KERNBASE,%edx
	addl	_C_LABEL(tablesize),%edx
	addl	%esi,%edx
	movl	%edx,_C_LABEL(atdevbase)

	/* Set up bootstrap stack. */
	leal	(PROC0_STK_OFF+KERNBASE)(%esi),%eax
	movl	%eax,_C_LABEL(lwp0uarea)
	leal	(USPACE-FRAMESIZE)(%eax),%esp
	movl	%esi,PCB_CR3(%eax)	/* pcb->pcb_cr3 */
	xorl	%ebp,%ebp		/* mark end of frames */

#if defined(MULTIBOOT)
	/* It is now safe to parse the Multiboot information structure
	 * we saved before from C code.  Note that we cannot delay its
	 * parsing any more because initgdt (called below) needs to make
	 * use of this information.
	 * We call both multiboot 1 and 2 flavors, they now if they 
	 * have something to do on their own.
	 */
	call	_C_LABEL(multiboot1_post_reloc)
	call 	_C_LABEL(multiboot2_post_reloc)
#endif

	subl	$NGDT*8, %esp		/* space for temporary gdt */
	pushl	%esp
	call	_C_LABEL(initgdt)
	addl	$4,%esp

	movl	_C_LABEL(tablesize),%eax
	addl	%esi,%eax		/* skip past stack and page tables */

#ifdef PAE
	pushl	$0	/* init386() expects a 64 bits paddr_t with PAE */
#endif
	pushl	%eax
#if defined(XEN) && !defined(XENPV)
        call    _C_LABEL(init_xen_early)
#endif
	call	_C_LABEL(init_bootspace)
	call	_C_LABEL(init386)
	addl	$PDE_SIZE,%esp		/* pop paddr_t */
	addl	$NGDT*8,%esp		/* pop temporary gdt */

	call 	_C_LABEL(main)
#else /* XENPV */
	/* First, reset the PSL. */
	pushl	$PSL_MBO
	popfl

	cld

	/*
	 * Xen info:
	 * - %esp -> stack, *theoretically* the last used page by Xen bootstrap
	 */
	movl	%esp,%ebx
	movl	$_RELOC(tmpstk),%esp

	/* Clear BSS. */
	xorl	%eax,%eax
	movl	$RELOC(__bss_start),%edi
	movl	$RELOC(_end),%ecx
	subl	%edi,%ecx
	rep
	stosb

	/* Copy the necessary stuff from start_info structure. */
	/* We need to copy shared_info early, so that sti/cli work */
	movl	$RELOC(start_info_union),%edi
	movl	$128,%ecx
	rep
	movsl

	/* Clear segment registers. */
	xorl	%eax,%eax
	movw	%ax,%fs
	movw	%ax,%gs

	xorl	%eax,%eax
	cpuid
	movl	%eax,RELOC(cpuid_level)

	movl    $VM_GUEST_XENPV, RELOC(vm_guest)

	/*
	 * Use a temporary GDT page. We'll re-add it to uvm(9) once we're done
	 * using it.
	 */
	movl	$RELOC(tmpgdt),%eax
	pushl	%eax		/* start of temporary gdt */
	call	_C_LABEL(initgdt)
	addl	$4,%esp

	call	xen_locore

	/*
	 * The first VA available is returned by xen_locore in %eax. We
	 * use it as the UAREA, and set up the stack here.
	 */
	movl	%eax,%esi
	movl	%esi,_C_LABEL(lwp0uarea)
	leal	(USPACE-FRAMESIZE)(%eax),%esp
	xorl	%ebp,%ebp		/* mark end of frames */

	/* Set first_avail after the DUMMY PAGE (see xen_locore). */
	addl	$(USPACE+PAGE_SIZE),%esi
	subl	$KERNBASE,%esi		/* init386 wants a physical address */

	pushl	$0	/* init386() expects a 64 bits paddr_t with PAE */
	pushl	%esi
	call	_C_LABEL(init_bootspace)
	call	_C_LABEL(init386)
	addl	$PDE_SIZE,%esp		/* pop paddr_t */
	call 	_C_LABEL(main)
#endif /* XENPV */
END(start)

#if defined(XEN)
#ifndef XENPV
/* entry point for Xen PVH */
ENTRY(start_xenpvh)
	/* Xen doesn't start us with a valid gdt */
	movl    $RELOC(gdtdesc_xenpvh), %eax
	lgdt    (%eax)
	jmp     $GSEL(GCODE_SEL, SEL_KPL), $RELOC(.Lreload_cs)

.Lreload_cs:
	movw    $GSEL(GDATA_SEL, SEL_KPL), %ax          
	movw    %ax, %ds
	movw    %ax, %es
	movw    %ax, %ss   

	/* we need a valid stack */
	movl	$RELOC(tmpstk),%esp

	/* clear BSS */
        xorl    %eax,%eax
	movl    $RELOC(__bss_start),%edi
	movl    $RELOC(_end),%ecx
	subl    %edi,%ecx
	rep
	stosb

	/*
	 * save addr of the hvm_start_info structure. This is also the end
	 * of the symbol table
	 */
	movl	%ebx, RELOC(hvm_start_paddr)
	movl	%ebx, %eax
	addl	$KERNBASE,%eax
	movl	$RELOC(esym),%ebp
	movl	%eax,(%ebp)
	/* get a page for HYPERVISOR_shared_info */
	addl	$PAGE_SIZE, %ebx
	addl	$PGOFSET,%ebx
	andl	$~PGOFSET,%ebx
	movl	$RELOC(HYPERVISOR_shared_info_pa),%ebp
	movl	%ebx,(%ebp)
	/* XXX assume hvm_start_info+dependant structure fits in a single page */
	addl	$PAGE_SIZE, %ebx
	addl	$PGOFSET,%ebx
	andl	$~PGOFSET,%ebx
	addl	$KERNBASE,%ebx
	movl	$RELOC(eblob),%ebp
	movl	%ebx,(%ebp)
	/* announce ourself */
	movl	$VM_GUEST_XENPVH, RELOC(vm_guest)
	jmp	.Lstart_common
END(start_xenpvh)
	.align 8
gdtdesc_xenpvh:
	.word	gdt_xenpvhend - gdt_xenpvh
	.long	RELOC(gdt_xenpvh)
	.word	0
gdt_xenpvh:
	.long   0			# null descriptor
	.long   0
	.long   0x0000ffff		# %cs
	.long   0x00cf9a00
	.long   0x0000ffff		# %ds, %es, %ss
	.long   0x00cf9200
gdt_xenpvhend:
	.align 4
#endif /* !XENPV */


/* space for the hypercall call page */
#define HYPERCALL_PAGE_OFFSET 0x1000
.align HYPERCALL_PAGE_OFFSET
ENTRY(hypercall_page) /* Returns -1, on HYPERVISOR_xen_version() */
.skip	(__HYPERVISOR_xen_version*32), 0x90
	movl	$-1, %eax
	retl
.align HYPERCALL_PAGE_OFFSET, 0x90
END(hypercall_page)

#ifdef XENPV
/*
 * void lgdt_finish(void);
 * Finish load a new GDT pointer (do any necessary cleanup).
 * XXX It's somewhat questionable whether reloading all the segment registers
 * is necessary, since the actual descriptor data is not changed except by
 * process creation and exit, both of which clean up via task switches.  OTOH,
 * this only happens at run time when the GDT is resized.
 */
/* LINTSTUB: Func: void lgdt_finish(void) */
ENTRY(lgdt_finish)
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
END(lgdt_finish)

#endif /* XENPV */
#endif /* XEN */

/*
 * void lwp_trampoline(void);
 *
 * This is a trampoline function pushed onto the stack of a newly created
 * process in order to do some additional setup.  The trampoline is entered by
 * cpu_switchto()ing to the process, so we abuse the callee-saved
 * registers used by cpu_switchto() to store the information about the
 * stub to call.
 * NOTE: This function does not have a normal calling sequence!
 */
ENTRY(lwp_trampoline)
	movl	%ebp,%edi	/* for .Lsyscall_checkast */
	xorl	%ebp,%ebp
	pushl	%edi
	pushl	%eax
	call	_C_LABEL(lwp_startup)
	addl	$8,%esp
	pushl	%ebx
	call	*%esi
	addl	$4,%esp
	jmp	.Lsyscall_checkast
	/* NOTREACHED */
END(lwp_trampoline)

/*
 * sigcode()
 *
 * Signal trampoline; copied to top of user stack.  Used only for
 * compatibility with old releases of NetBSD.
 */
ENTRY(sigcode)
	/*
	 * Handler has returned here as if we called it.  The sigcontext
	 * is on the stack after the 3 args "we" pushed.
	 */
	leal	12(%esp),%eax		/* get pointer to sigcontext */
	movl	%eax,4(%esp)		/* put it in the argument slot */
					/* fake return address already there */
	movl	$SYS_compat_16___sigreturn14,%eax
	int	$0x80	 		/* enter kernel with args on stack */
	movl	$SYS_exit,%eax
	int	$0x80			/* exit if sigreturn fails */
	.globl	_C_LABEL(esigcode)
_C_LABEL(esigcode):
END(sigcode)

/*
 * int setjmp(label_t *)
 *
 * Used primarily by DDB.
 */
ENTRY(setjmp)
	movl	4(%esp),%eax
	movl	%ebx,(%eax)		/* save ebx */
	movl	%esp,4(%eax)		/* save esp */
	movl	%ebp,8(%eax)		/* save ebp */
	movl	%esi,12(%eax)		/* save esi */
	movl	%edi,16(%eax)		/* save edi */
	movl	(%esp),%edx		/* get rta */
	movl	%edx,20(%eax)		/* save eip */
	xorl	%eax,%eax		/* return 0 */
	ret
END(setjmp)

/*
 * int longjmp(label_t *)
 *
 * Used primarily by DDB.
 */
ENTRY(longjmp)
	movl	4(%esp),%eax
	movl	(%eax),%ebx		/* restore ebx */
	movl	4(%eax),%esp		/* restore esp */
	movl	8(%eax),%ebp		/* restore ebp */
	movl	12(%eax),%esi		/* restore esi */
	movl	16(%eax),%edi		/* restore edi */
	movl	20(%eax),%edx		/* get rta */
	movl	%edx,(%esp)		/* put in return frame */
	movl	$1,%eax			/* return 1 */
	ret
END(longjmp)

/*
 * void dumpsys(void)
 *
 * Mimic cpu_switchto() for postmortem debugging.
 */
ENTRY(dumpsys)
	pushl	%ebx			/* set up fake switchframe */
	pushl	%esi			/* and save context */
	pushl	%edi
	movl	%esp,_C_LABEL(dumppcb)+PCB_ESP
	movl	%ebp,_C_LABEL(dumppcb)+PCB_EBP
	call	_C_LABEL(dodumpsys)	/* dump! */
	addl	$(3*4), %esp		/* unwind switchframe */
	ret
END(dumpsys)

/*
 * struct lwp *cpu_switchto(struct lwp *oldlwp, struct lwp *newlwp,
 *     bool returning)
 *
 *	1. save context of oldlwp.
 *	2. restore context of newlwp.
 *
 * Note that the stack frame layout is known to "struct switchframe" in
 * <machine/frame.h> and to the code in cpu_lwp_fork() which initializes
 * it for a new lwp.
 */
ENTRY(cpu_switchto)
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	16(%esp),%esi		/* oldlwp */
	movl	20(%esp),%edi		/* newlwp */
	movl	24(%esp),%edx		/* returning */

	/* Save old context. */
	movl	L_PCB(%esi),%eax
	movl	%esp,PCB_ESP(%eax)
	movl	%ebp,PCB_EBP(%eax)

	/* Switch to newlwp's stack. */
	movl	L_PCB(%edi),%ebx
	movl	PCB_EBP(%ebx),%ebp
	movl	PCB_ESP(%ebx),%esp

	/*
	 * Set curlwp.  This must be globally visible in order to permit
	 * non-interlocked mutex release.
	 */
	movl	%edi,%ecx
	xchgl	%ecx,CPUVAR(CURLWP)

#ifdef XENPV
	/* if we are there, we're obviously not in user context.
	 * reset ci_xen_clockf_* in case the splx() at the end of mi_switch()
	 * triggers a deffered call do xen_timer_handler()
	 */
	movb	$0, CPUVAR(XEN_CLOCKF_USERMODE)
	movl	$_C_LABEL(cpu_switchto), CPUVAR(XEN_CLOCKF_PC)
#endif

	/* Skip the rest if returning to a pinned LWP. */
	testl	%edx,%edx
	jnz	switch_return

	/* Switch ring0 stack */
#ifdef XENPV
	pushl	%edi
	call	_C_LABEL(i386_switch_context)
	addl	$4,%esp
#else
	movl	PCB_ESP0(%ebx),%eax
	movl	CPUVAR(TSS),%ecx
	movl	%eax,TSS_ESP0(%ecx)
#endif

	/* Switch the dbregs. */
	pushl	%edi
	pushl	%esi
	call	_C_LABEL(x86_dbregs_switch)
	addl	$8,%esp

	/* Switch the FPU. */
	pushl	%edx
	pushl	%edi
	pushl	%esi
	call	_C_LABEL(fpu_switch)
	addl	$8,%esp
	popl	%edx

	/* Don't bother with the rest if switching to a system process. */
	testl	$LW_SYSTEM,L_FLAG(%edi)
	jnz	switch_return

#ifndef XENPV
	/* Restore thread-private %fs/%gs descriptors. */
	movl	CPUVAR(GDT),%ecx
	movl	PCB_FSD(%ebx),%eax
	movl	PCB_FSD+4(%ebx),%edx
	movl	%eax,(GUFS_SEL*8)(%ecx)
	movl	%edx,(GUFS_SEL*8+4)(%ecx)
	movl	PCB_GSD(%ebx),%eax
	movl	PCB_GSD+4(%ebx),%edx
	movl	%eax,(GUGS_SEL*8)(%ecx)
	movl	%edx,(GUGS_SEL*8+4)(%ecx)
#endif /* !XENPV */

	/* Switch I/O bitmap */
	movl	PCB_IOMAP(%ebx),%eax
	orl	%eax,%eax
	jnz	.Lcopy_iobitmap
	movl	CPUVAR(TSS),%eax
	movl	$(IOMAP_INVALOFF << 16),TSS_IOBASE(%eax)
.Liobitmap_done:

	/* Is this process using RAS (restartable atomic sequences)? */
	movl	L_PROC(%edi),%eax
	cmpl	$0,P_RASLIST(%eax)
	je	no_RAS

	/* Handle restartable atomic sequences (RAS). */
	movl	L_MD_REGS(%edi),%ecx
	pushl	TF_EIP(%ecx)
	pushl	%eax
	call	_C_LABEL(ras_lookup)
	addl	$8,%esp
	cmpl	$-1,%eax
	je	no_RAS
	movl	L_MD_REGS(%edi),%ecx
	movl	%eax,TF_EIP(%ecx)
no_RAS:

#ifdef XENPV
	pushl	%edi
	call	_C_LABEL(i386_tls_switch)
	addl	$4,%esp
#endif

switch_return:
	/* Return to the new LWP, returning 'oldlwp' in %eax. */
	movl	%esi,%eax
	popl	%edi
	popl	%esi
	popl	%ebx
	ret

.Lcopy_iobitmap:
	/* Copy I/O bitmap. */
	incl	_C_LABEL(pmap_iobmp_evcnt)+EV_COUNT
	movl	$(IOMAPSIZE/4),%ecx
	pushl	%esi
	pushl	%edi
	movl	%eax,%esi		/* pcb_iomap */
	movl	CPUVAR(TSS),%edi
	leal	TSS_IOMAP(%edi),%edi
	rep
	movsl
	popl	%edi
	popl	%esi
	movl	CPUVAR(TSS),%eax
	movl	$(IOMAP_VALIDOFF << 16),TSS_IOBASE(%eax)
	jmp	.Liobitmap_done
END(cpu_switchto)

/*
 * void savectx(struct pcb *pcb);
 *
 * Update pcb, saving current processor state.
 */
ENTRY(savectx)
	movl	4(%esp),%edx		/* edx = pcb */
	movl	%esp,PCB_ESP(%edx)
	movl	%ebp,PCB_EBP(%edx)
	ret
END(savectx)

/*
 * syscall()
 *
 * Trap gate entry for syscall
 */
IDTVEC(syscall)
	pushl	$2		/* size of instruction for restart */
	pushl	$T_ASTFLT	/* trap # for doing ASTs */
	INTRENTRY
	STI(%eax)

#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
	testl	%ebx,%ebx
	jz	1f
	pushl	$5f
	call	_C_LABEL(panic)
	addl	$4,%esp
#ifdef DDB
	int	$3
#endif
1:
#endif /* DIAGNOSTIC */

	addl	$1,CPUVAR(NSYSCALL)	/* count it atomically */
	adcl	$0,CPUVAR(NSYSCALL)+4	/* count it atomically */
	movl	CPUVAR(CURLWP),%edi
	movl	L_PROC(%edi),%edx
	movl	%esp,L_MD_REGS(%edi)	/* save pointer to frame */
	pushl	%esp
	call	*P_MD_SYSCALL(%edx)	/* get pointer to syscall() function */
	addl	$4,%esp
.Lsyscall_checkast:
	/* Check for ASTs on exit to user mode. */
	CLI(%eax)
	movl	L_MD_ASTPENDING(%edi), %eax
	orl	CPUVAR(WANT_PMAPLOAD), %eax
	jnz	9f

	HANDLE_DEFERRED_FPU

#ifdef XENPV
	STIC(%eax)
	jz	14f
	call	_C_LABEL(stipending)
	testl	%eax,%eax
	jz	14f
	/* process pending interrupts */
	CLI(%eax)
	movl	CPUVAR(ILEVEL), %ebx
	movl	$.Lsyscall_resume, %esi /* address to resume loop at */
.Lsyscall_resume:
	movl	%ebx,%eax		/* get cpl */
	movl	CPUVAR(IUNMASK)(,%eax,4),%eax
	andl	CPUVAR(IPENDING),%eax	/* any non-masked bits left? */
	jz	17f
	bsrl	%eax,%eax
	btrl	%eax,CPUVAR(IPENDING)
	movl	CPUVAR(ISOURCES)(,%eax,4),%eax
	jmp	*IS_RESUME(%eax)
17:	movl	%ebx, CPUVAR(ILEVEL)	/* restore cpl  */
	jmp	.Lsyscall_checkast
14:
#endif /* XENPV */

#ifdef DIAGNOSTIC
	cmpl	$IPL_NONE,CPUVAR(ILEVEL)
	jne	3f
#endif

	INTRFASTEXIT

#ifdef DIAGNOSTIC
3:	STI(%eax)
	pushl	$4f
	call	_C_LABEL(panic)
	addl	$4,%esp
	pushl	$IPL_NONE
	call	_C_LABEL(spllower)
	addl	$4,%esp
	jmp	.Lsyscall_checkast
4:	.asciz	"SPL NOT LOWERED ON SYSCALL EXIT\n"
5:	.asciz	"SPL NOT ZERO ON SYSCALL ENTRY\n"
#endif

9:
	cmpl	$0, CPUVAR(WANT_PMAPLOAD)
	jz	10f
	STI(%eax)
	call	_C_LABEL(pmap_load)
	jmp	.Lsyscall_checkast	/* re-check ASTs */
10:
	/* Always returning to user mode here. */
	movl	$0, L_MD_ASTPENDING(%edi)
	STI(%eax)
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	pushl	%esp
	call	_C_LABEL(trap)
	addl	$4,%esp
	jmp	.Lsyscall_checkast	/* re-check ASTs */
IDTVEC_END(syscall)

/*
 * int npx586bug1(int a, int b)
 * Used when checking for the FDIV bug on first generations pentiums.
 * Anything 120MHz or above is fine.
 */
ENTRY(npx586bug1)
	fildl	4(%esp)		/* x */
	fildl	8(%esp)		/* y */
	fld	%st(1)
	fdiv	%st(1),%st	/* x/y */
	fmulp	%st,%st(1)	/* (x/y)*y */
	fsubrp	%st,%st(1)	/* x-(x/y)*y */
	pushl	$0
	fistpl	(%esp)
	popl	%eax
	ret
END(npx586bug1)

ENTRY(intrfastexit)
	movw	TF_GS(%esp),%gs
	movw	TF_FS(%esp),%fs
	movw	TF_ES(%esp),%es
	movw	TF_DS(%esp),%ds
	movl	TF_EDI(%esp),%edi
	movl	TF_ESI(%esp),%esi
	movl	TF_EBP(%esp),%ebp
	movl	TF_EBX(%esp),%ebx
	movl	TF_EDX(%esp),%edx
	movl	TF_ECX(%esp),%ecx
	movl	TF_EAX(%esp),%eax
	addl	$(TF_PUSHSIZE+8),%esp
	iret
END(intrfastexit)

	.section .rodata

	/*
	 * Hotpatch templates.
	 */

LABEL(hp_nolock)
	nop
LABEL(hp_nolock_end)

LABEL(hp_retfence)
	lfence
LABEL(hp_retfence_end)

LABEL(hp_clac)
	clac
LABEL(hp_clac_end)

LABEL(hp_stac)
	stac
LABEL(hp_stac_end)
