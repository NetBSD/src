/*	$NetBSD: biostramp.S,v 1.9 2001/05/02 13:12:45 jdolecek Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John Kohl.
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
 * biostramp.S:		provide a means for NetBSD to call BIOS interrupts
 *			by switching to real mode, calling it, and switching
 *			back to protected & paging mode.
 */

/*
 * Micro$haft's book on i386/i486 programming says you should do the following
 * to return to real mode from protected mode:
 *
 * 1) disable paging, by jumping to code with identical virtual and physical
 * addresses, clearing PG in CR0, and zeroing CR3 (PDBR).
 *
 * 2) segment descriptors must be byte-granular with limit 64k-1, def32 = 0,
 * (i.e. 16-bit data accesses and/or 80286 instructions)
 * CS must be executable; DS,ES,FS,GS should be writable
 * 
 * 3) disable interrupts, load IDTR with original value (base 0, limit 1023)
 *
 * 4) clear PE in CR0, execute FAR jump to load CS.
 *
 * 5) load SP, and off you go
 *
 */

#include "assym.h"

#include <i386/include/param.h>
#include <i386/include/specialreg.h>
#include <i386/include/segments.h>
#include <i386/include/apmvar.h>
#include <i386/include/psl.h>
#include <i386/include/asm.h>

#define	addr32	.byte 0x67
#define	data32	.byte 0x66

	.set MYBASE,NBPG
	.set MYSCRATCH,NBPG+NBPG
	.set CR3_ADDR,(MYSCRATCH-4)
	.set IDTR_SAVE_ADDR,CR3_ADDR-6
	.set GDTR_SAVE_ADDR,IDTR_SAVE_ADDR-6
	.set GDTR_LOCAL_ADDR,GDTR_SAVE_ADDR-6
	.set STACK_PTR_ADDR,GDTR_LOCAL_ADDR-4
	.set BASE_PTR_ADDR,STACK_PTR_ADDR-4
	.set FUNCTION_ADDR,(BASE_PTR_ADDR-2)
	.set GDT_COPY_ADDR,(FUNCTION_ADDR-NGDT*8)
	.set EAX_REGADDR,(GDT_COPY_ADDR-4)
	.set EBX_REGADDR,(EAX_REGADDR-4)
	.set ECX_REGADDR,(EBX_REGADDR-4)
	.set EDX_REGADDR,(ECX_REGADDR-4)
	.set ESI_REGADDR,(EDX_REGADDR-4)
	.set EDI_REGADDR,(ESI_REGADDR-4)
	.set EFLAGS_REGADDR,(EDI_REGADDR-4)
	.set ES_REGADDR, (EFLAGS_REGADDR-4)
	.set ENDREGADDR,(ES_REGADDR-4)
	
	.set REALSTACK,ENDREGADDR-20		# leave a red zone?

#define COPY_FLAGS (PSL_C|PSL_PF|PSL_AF|PSL_Z|PSL_N|PSL_D|PSL_V)

/*
 * do_bios_call(int function, struct bioscall *regs)
 */

ENTRY(do_bios_call)
	pushl	%ebp
	movl	%esp,%ebp		/* set up frame ptr */
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	pushl	%ds		
	pushl	%es
	pushl	%fs
	pushl	%gs

	# copy data to where the real-mode hook can handle it
	movl 8(%ebp),%eax
	movw %ax,FUNCTION_ADDR
	movl 12(%ebp),%ebx
	movl BIOSCALLREG_EAX(%ebx),%eax
	movl %eax,EAX_REGADDR
	movl BIOSCALLREG_EBX(%ebx),%eax
	movl %eax,EBX_REGADDR
	movl BIOSCALLREG_ECX(%ebx),%eax
	movl %eax,ECX_REGADDR
	movl BIOSCALLREG_EDX(%ebx),%eax
	movl %eax,EDX_REGADDR
	movl BIOSCALLREG_ESI(%ebx),%eax
	movl %eax,ESI_REGADDR
	movl BIOSCALLREG_EDI(%ebx),%eax
	movl %eax,EDI_REGADDR
	# merge current flags with certain provided flags
	movl BIOSCALLREG_EFLAGS(%ebx),%ecx
	pushfl
	popl %eax
	andl $~(COPY_FLAGS|PSL_I),%eax
	andl $COPY_FLAGS,%ecx
	orl %ecx,%eax
	movl %eax,EFLAGS_REGADDR
	movl $0, ES_REGADDR 
	
	# save flags, disable interrupts, do real mode stuff
	pushfl
	
	# save GDT
	sgdt GDTR_SAVE_ADDR
	
	# copy the GDT to local area
	movl GDTR_SAVE_ADDR+2,%esi
	movl $GDT_COPY_ADDR,%edi
	movl $(NGDT*8),%ecx
	cld
	rep
	movsb
	movw $(NGDT*8)-1,GDTR_LOCAL_ADDR
	movl $GDT_COPY_ADDR,GDTR_LOCAL_ADDR+2
	
	# install GDT copy
	lgdt GDTR_LOCAL_ADDR
	
	cli

	# save IDT
	sidt IDTR_SAVE_ADDR
		
	# set up new stack: save old ones, create new segs
	movl %esp,STACK_PTR_ADDR
	movl %ebp,BASE_PTR_ADDR
	movl $REALSTACK,%esp
	movl $0,%ebp		# leave no trace, there is none.

	# save CR3
	movl %cr3,%eax
	movl %eax,CR3_ADDR
	
	# turn off paging
	movl %cr0,%eax
	andl $~(CR0_PG),%eax
	movl %eax,%cr0
	
	# flush TLB, drop PDBR
	xorl %eax,%eax
	movl %eax,%cr3
	
	## load 16-bit segment descriptors
	movw $GSEL(GBIOSDATA_SEL,SEL_KPL),%bx
	movw %bx,%ds
	movw %bx,%es
	movw %bx,%fs
	movw %bx,%gs

	ljmp $GSEL(GBIOSCODE_SEL,SEL_KPL),$x16+MYBASE
	
x16:	
	# turn off protected mode--yikes!
	mov	%cr0,%eax
	data32
	and	$~CR0_PE,%eax
	mov	%eax,%cr0
	
	# need inter-segment jump to reload real-mode CS
	data32
	ljmp $(MYBASE>>4),$xreal
		
xreal:	# really in real mode now
	# set up segment selectors.  Note: everything is now relative
	# to zero-base in this file, except %ss.
	# data items in our scratch area need to reflect MYADDR
	xorl %ax,%ax
	movw %ax,%ss

	movw %cs,%ax
	movw %ax,%es
	movw %ax,%fs
	movw %ax,%gs
	movw %ax,%ds
	
	## load IDT, now that we are here.
	addr32
	lidt IDT_bios

	# Don't forget that we're in real mode, with 16-bit default data.
	# all these movl's are really movw's, and movw's are movl's!
	addr32
	movw EDI_REGADDR-MYBASE,%edi
	addr32
	movw ESI_REGADDR-MYBASE,%esi
	addr32
	movw EDX_REGADDR-MYBASE,%edx
	addr32
	movw ECX_REGADDR-MYBASE,%ecx
	addr32
	movw EBX_REGADDR-MYBASE,%ebx
	addr32
	movb FUNCTION_ADDR-MYBASE,%al
	addr32
	movb %al,intaddr+1	# self modifying code, yuck. no indirect interrupt instruction!
	# long jump to flush processor cache to reflect code modification
	data32
	ljmp $(MYBASE>>4),$flushit
flushit:
	addr32
	movw EFLAGS_REGADDR-MYBASE,%eax
	pushl %eax
	popfl
	addr32
	movw EAX_REGADDR-MYBASE,%eax
	
intaddr: 
	int $0xff

	# save results
	pushf
	addr32
	movw %eax,EAX_REGADDR-MYBASE
	addr32
	movw %ebx,EBX_REGADDR-MYBASE
	addr32
	movw %ecx,ECX_REGADDR-MYBASE
	addr32
	movw %edx,EDX_REGADDR-MYBASE
	addr32
	movw %esi,ESI_REGADDR-MYBASE
	addr32
	movw %edi,EDI_REGADDR-MYBASE
	pop %eax
	addr32
	movw %eax,EFLAGS_REGADDR-MYBASE
	addr32
	movw %es,ES_REGADDR-MYBASE

	# and return to protected mode
	cli	# just to be sure

	mov %cr0,%eax
	data32
	or $CR0_PE,%eax
	mov %eax,%cr0
	
	# long jump to 32-bit code segment
	data32
	ljmp $GSEL(GCODE_SEL,SEL_KPL),$x32+MYBASE
x32:	
	#back in 32-bit mode/protected mode (but not paging yet).
	# Reload the segment registers & IDT

	movw $GSEL(GDATA_SEL,SEL_KPL),%bx
	movw %bx,%ds
	movw %bx,%ss
	movw %bx,%es
	
	# reload PDBR
	movl CR3_ADDR,%eax
	movl %eax,%cr3
	movl %cr0,%eax
	orl $CR0_PG,%eax
	movl %eax,%cr0
	
	# reload system copy of GDT
	lgdt GDTR_SAVE_ADDR

	# restore protected-mode stack
	movl STACK_PTR_ADDR,%esp
	movl BASE_PTR_ADDR,%ebp
	
	#restore protected-mode IDT
	lidt IDTR_SAVE_ADDR
	
	# copy back arguments from holding pen

	movl 12(%ebp),%ebx
	movl EAX_REGADDR,%eax
	movl %eax,BIOSCALLREG_EAX(%ebx)
	movl EBX_REGADDR,%eax
	movl %eax,BIOSCALLREG_EBX(%ebx)
	movl ECX_REGADDR,%eax
	movl %eax,BIOSCALLREG_ECX(%ebx)
	movl EDX_REGADDR,%eax
	movl %eax,BIOSCALLREG_EDX(%ebx)
	movl ESI_REGADDR,%eax
	movl %eax,BIOSCALLREG_ESI(%ebx)
	movl EDI_REGADDR,%eax
	movl %eax,BIOSCALLREG_EDI(%ebx)
	movl EFLAGS_REGADDR,%eax
	movl %eax,BIOSCALLREG_EFLAGS(%ebx)
	movl ES_REGADDR, %eax
	movl %eax,BIOSCALLREG_ES(%ebx)
	
	# finish up, restore registers, and return
	popfl
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds		# see above
	popl	%ebx
	popl	%edi
	popl	%esi
	leave
	ret

#ifdef __ELF__
	.align 16
#else
	.align 4
#endif
IDT_bios:			# BIOS IDT descriptor (real-mode)
	.word 1023
	.long 0
