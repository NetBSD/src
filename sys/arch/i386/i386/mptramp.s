/*	$NetBSD: mptramp.s,v 1.1.2.3 2001/01/03 17:02:06 thorpej Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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
 * the stuff from cpu_spinup_trampoline to mp_startup 
 * is copied into the first 640 KB
 *
 * We startup the processors now when the kthreads become ready.
 * The steps are:
 *        1)   Get the processors running kernel-code from a special
 *                  page-table and stack page, do chip identification.
 *        2)   halt the processors waiting for them to be enabled
 *              by a idle-thread 
 */
	
#include "opt_mpbios.h"		/* for MPDEBUG */
		
#include "assym.h"
#include <machine/param.h>
#include <machine/asm.h>
#include <machine/specialreg.h>
#include <machine/segments.h>
#include <machine/mpbiosvar.h>
#include <machine/i82489reg.h>

#define GDTE(a,b)               .byte   0xff,0xff,0x0,0x0,0x0,a,b,0x0
#define _RELOC(x)       ((x) - KERNBASE)
#define RELOC(x)        _RELOC(_C_LABEL(x))

#define _TRMP_LABEL(a)  a = . - _C_LABEL(cpu_spinup_trampoline) + MP_TRAMPOLINE

/*
 * Debug code to stop aux. processors in various stages based on the
 * value in cpu_trace.
 *
 * %edi points at cpu_trace;  cpu_trace[0] is the "hold point";
 * cpu_trace[1] is the point which the cpu has reached.
 * cpu_trace[2] is the last value stored by HALTT.
 */
	
	
#ifdef MPDEBUG
#define HALT(x)	1: movl (%edi),%ebx;cmpl $ x,%ebx ; jle 1b ; movl $x,4(%edi)
#define HALTT(x,y)	movl y,8(%edi); HALT(x)
#else
#define HALT(x)	/**/
#define HALTT(x,y) /**/
#endif

        .globl  _C_LABEL(cpu),_C_LABEL(cpu_id),_C_LABEL(cpu_vendor)
	.globl  _C_LABEL(cpuid_level),_C_LABEL(cpu_feature)
	.globl	_C_LABEL(mpidle)

	.global _C_LABEL(cpu_spinup_trampoline)
	.global _C_LABEL(cpu_spinup_trampoline_end)
	.global _C_LABEL(cpu_hatch)
	.global _C_LABEL(mp_pdirpa)
	.global _C_LABEL(gdt), _C_LABEL(local_apic)

	.text
	.align 4,0x0
_C_LABEL(cpu_spinup_trampoline):
	cli
	xorl    %eax,%eax
	movl    %ax, %ds
	movl    %ax, %es
	movl    %ax, %ss
	aword
	word
	lgdt    (gdt_desc)      # load flat descriptor table
	movl    %cr0,%eax       # get cr0
	word
	orl     $0x1, %eax      # enable protected mode
	movl    %eax, %cr0      # doit
	movl    $0x10, %eax     # data segment
	movl    %ax, %ds
	movl    %ax, %ss
	movl    %ax, %es
	movl    %ax, %fs
	movl    %ax, %gs
	word
	ljmp    $0x8, $mp_startup

_TRMP_LABEL(mp_startup)
	movl    $ (MP_TRAMPOLINE+NBPG-4),%esp       # bootstrap stack end location
	
#ifdef MPDEBUG
	leal    RELOC(cpu_trace),%edi       
#endif

	HALT(0x1)
#if 0
	/* 
	 * use cpuid
	 */
	xorl    %eax,%eax
	cpuid
	movl    %eax,RELOC(cpuid_level)
	movl    %ebx,RELOC(cpu_vendor)  # store vendor string
	movl    %edx,RELOC(cpu_vendor)+4
	movl    %ecx,RELOC(cpu_vendor)+8
	movl    $0,  RELOC(cpu_vendor)+12
	movl    $1,%eax
	cpuid
	movl    %eax,RELOC(cpu_id)      # store cpu_id and features
	movl    %edx,RELOC(cpu_feature)
	HALT(0x2)
#endif
	/* First, reset the PSL. */
	pushl   $PSL_MBO
	popfl
	
	movl	RELOC(mp_pdirpa),%ecx
	HALTT(0x5,%ecx)
	
        /* Load base of page directory and enable mapping. */
        movl    %ecx,%cr3               # load ptd addr into mmu
        movl    %cr0,%eax               # get control word
                                        # enable paging & NPX emulation
        orl     $(CR0_PE|CR0_PG|CR0_NE|CR0_TS|CR0_EM|CR0_MP|CR0_WP),%eax
        movl    %eax,%cr0               # and let's page NOW!
#ifdef MPDEBUG
	leal    _C_LABEL(cpu_trace),%edi       # bootstrap stack end location
#endif
	HALT(0x7)
	movw    $((NGDT*8) - 1), ngdt_table	# prepare segment descriptor
	movl    _C_LABEL(gdt), %eax		# for real gdt
	movl    %eax, ngdt_table+2
	lgdt	ngdt_table
	HALT(0x8)	
	jmp	1f
	nop
1:	
	HALT(0x12)
	movl    $GSEL(GDATA_SEL, SEL_KPL),%eax 	#switch to new segment
	movl    %ax,%ds
	movl    %ax,%es
	movl    %ax,%ss
	HALT(0x13)
	pushl   $GSEL(GCODE_SEL, SEL_KPL)
	pushl	$mp_cont
	HALT(0x14)
	lret
	.align 4,0x0
_TRMP_LABEL(gdt_table)   
	.word   0x0,0x0,0x0,0x0  # null GDTE
	 GDTE(0x9f,0xcf)         # Kernel text
	 GDTE(0x93,0xcf)         # Kernel data
_TRMP_LABEL(gdt_desc)	
	.word   0x17             # limit 3 entries
	.long   gdt_table              # where is is gdt
_TRMP_LABEL(ngdt_table)   
	.long  0		# filled in after paging in enabled
	.long  0
	.align 4,0x0
_C_LABEL(cpu_spinup_trampoline_end):	#end of code copied to MP_TRAMPOLINE
mp_cont:
	HALT(0x15)

# ok, we're now running with paging enabled and sharing page tables with cpu0.
# figure out which processor we really are, what stack we should be on, etc.

	movzbl	_C_LABEL(local_apic)+LAPIC_ID+3,%ecx
	leal	0(,%ecx,4),%ecx
	movl	_C_LABEL(cpu_info)(%ecx),%ecx
	
	HALTT(0x18, %ecx)

# %ecx points at our cpu_info structure..

	movl	CPU_INFO_IDLE_PCB(%ecx),%esi
#	movl	P_ADDR(%edx),%esi
	
	HALTT(0x19, %esi)
# %ecx points at our CPU_INFO.	
# %esi now points at our PCB.
	
	movl	PCB_ESP(%esi),%esp
	movl	PCB_EBP(%esi),%ebp
	
	/* Load TSS info. */
	movl	_C_LABEL(gdt),%eax
#	movl	PCB_TSS_SEL(%esi),%edx
	HALT(0x20)	
	/* Switch address space. */
	movl	PCB_CR3(%esi),%eax
	HALTT(0x22, %eax)		
	movl	%eax,%cr3
	HALT(0x24)
	
#	/* Switch TSS. */
#	andl	$~0x0200,4-SEL_KPL(%eax,%edx,1)
#	ltr	%dx
	
	HALT(0x25)
	/* Restore segment registers. */
	movl	PCB_FS(%esi),%eax
	HALTT(0x26,%eax)
	movl	%ax,%fs
	movl	PCB_GS(%esi),%eax
	HALTT(0x27,%eax)	
	movl	%ax,%gs
	movl    PCB_CR0(%esi),%eax
	HALTT(0x28,%eax)		
	movl    %eax,%cr0
	HALTT(0x30,%ecx)	
	pushl	%ecx
	call	_C_LABEL(cpu_hatch)
	HALT(0x33)
	xorl	%esi,%esi
	jmp	_C_LABEL(mpidle)
mps:
	hlt
	jmp mps
	
	.data
_C_LABEL(mp_pdirpa):
	.long	0
#ifdef MPDEBUG
	.global _C_LABEL(cpu_trace)
_C_LABEL(cpu_trace):
	.long  0x40
	.long  0xff
	.long  0xff		
#endif
