/*	$NetBSD: mptramp.S,v 1.24.2.2 2017/12/03 11:36:17 jdolecek Exp $	*/

/*
 * Copyright (c) 2000, 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc. (Author: Bill Sommerfeld), and Maxime Villard.
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
 * Copyright (c) 1999 Stefan Grefen
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR AND CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * MP startup ...
 * the stuff from cpu_spinup_trampoline to mp_startup is copied into the
 * first 640 KB.
 *
 * We startup the processors now when the kthreads become ready.
 * The steps are:
 *     1) Get the processors running kernel-code from a special
 *        page-table and stack page, do chip identification.
 *     2) halt the processors waiting for them to be enabled
 *        by a idle-thread
 */

#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: mptramp.S,v 1.24.2.2 2017/12/03 11:36:17 jdolecek Exp $");

#include "assym.h"
#include <machine/specialreg.h>
#include <machine/segments.h>
#include <machine/mpbiosvar.h>
#include <machine/i82489reg.h>
#include <machine/gdt.h>

#define GDTE(a,b)	.byte   0xff,0xff,0x0,0x0,0x0,a,b,0x0

#define _TRMP_LABEL(a)	a = . - _C_LABEL(cpu_spinup_trampoline) + MP_TRAMPOLINE

/*
 * A smp_data structure is packed at the end of the trampoline page. The stack
 * is right below this structure.
 */
#define SMP_DATA	(MP_TRAMPOLINE + PAGE_SIZE - 3 * 4)
#define SMP_DATA_STACK	(SMP_DATA + 0 * 4)
#define SMP_DATA_LARGE	(SMP_DATA + 0 * 4)
#define SMP_DATA_NOX	(SMP_DATA + 1 * 4)
#define SMP_DATA_PDIR	(SMP_DATA + 2 * 4)

	.global _C_LABEL(cpu_spinup_trampoline)
	.global _C_LABEL(cpu_spinup_trampoline_end)

	.text
	.align 4,0x0
	.code16
/* XXX ENTRY() */
LABEL(cpu_spinup_trampoline)
	cli
	xorw	%ax,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%ss

	/* load flat descriptor table */
#ifdef __clang__
	lgdt (gdt_desc)
#else
	data32 addr32 lgdt (gdt_desc)
#endif

	/* enable protected mode */
	movl	%cr0,%eax
	orl	$CR0_PE,%eax
	movl	%eax,%cr0
	ljmpl	$0x8,$mp_startup

_TRMP_LABEL(mp_startup)
	.code32

	movl	$0x10,%eax	/* data segment */
	movw	%ax,%ds
	movw	%ax,%ss
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs

	/* bootstrap stack end */
	movl	$SMP_DATA_STACK,%esp

	/* First, reset the PSL. */
	pushl	$PSL_MBO
	popfl

	/* Enable PSE if available */
	movl	$SMP_DATA_LARGE,%eax
	movl	(%eax),%eax
	orl	%eax,%eax
	jz	no_PSE
	movl	%cr4,%eax
	orl	$CR4_PSE,%eax
	movl	%eax,%cr4
no_PSE:

#ifdef PAE
	/* Enable PAE */
	movl	%cr4,%eax
	or	$CR4_PAE,%eax
	movl	%eax,%cr4
#endif

	/*
	 * Set NOX in EFER, if available.
	 */
	movl	$SMP_DATA_NOX,%ebx
	movl	(%ebx),%ebx
	cmpl	$0,%ebx
	je 	no_NOX
	movl	$MSR_EFER,%ecx
	rdmsr
	xorl	%eax,%eax
	orl	$(EFER_NXE),%eax
	wrmsr
no_NOX:

	/* Load %cr3. */
	movl	$SMP_DATA_PDIR,%ecx
	movl	(%ecx),%ecx
	movl	%ecx,%cr3		/* load PTD addr into MMU */

	/* Enable paging and the rest of it. */
	movl	%cr0,%eax
	orl	$(CR0_PE|CR0_PG|CR0_NE|CR0_TS|CR0_MP|CR0_WP|CR0_AM),%eax
	movl	%eax,%cr0

	/* Wait until BP has done init sequence. */
1:
	movl	_C_LABEL(cpu_starting),%ecx
	pause
	testl	%ecx,%ecx
	jz	1b

	movw	$(MAXGDTSIZ-1),6(%esp)	/* prepare segment descriptor */
	movl	CPU_INFO_GDT(%ecx),%eax	/* for our real gdt */
	movl	%eax,8(%esp)
	lgdt	6(%esp)
	jmp	1f
	nop
1:
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax 	/* switch to new segment */
	movl	%eax,%ds
	movl	%eax,%es
	movl	%eax,%ss
	pushl	$GSEL(GCODE_SEL, SEL_KPL)
	pushl	$mp_cont
	lret
	.align	4,0x0

_TRMP_LABEL(gdt_table)
	.word	0x0,0x0,0x0,0x0	/* null gdte */
	GDTE(0x9f,0xcf)		/* Kernel text */
	GDTE(0x93,0xcf)		/* Kernel data */
_TRMP_LABEL(gdt_desc)
	.word	0x17		/* limit 3 entries */
	.long	gdt_table	/* base of gdt */

_C_LABEL(cpu_spinup_trampoline_end):	/* end of code copied to MP_TRAMPOLINE */

mp_cont:
	movl	CPU_INFO_IDLELWP(%ecx),%esi
	movl	L_PCB(%esi),%esi

	movl	PCB_ESP(%esi),%esp
	movl	PCB_EBP(%esi),%ebp

	/* Switch address space. */
	movl	PCB_CR3(%esi),%eax
	movl	%eax,%cr3

	/* load segment registers. */
	movl	$GSEL(GCPU_SEL, SEL_KPL),%eax
	movl	%eax,%fs
	xorl	%eax,%eax
	movl	%eax,%gs

	movl	PCB_CR0(%esi),%eax
	movl	%eax,%cr0
	pushl	%ecx

	call	_C_LABEL(cpu_hatch)
END(cpu_spinup_trampoline)

