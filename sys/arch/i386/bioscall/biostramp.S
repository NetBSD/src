/*	$NetBSD: biostramp.S,v 1.1 1996/09/08 15:36:53 jtk Exp $	*/
/*-
 * Copyright (c) 1996 John T. Kohl.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR `AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
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

#define ASM
#define _LOCORE

#define addr32	.byte 0x67
#define data32	.byte 0x66

/* these from locore.s: */
#define	ALIGN_TEXT	.align	2,0x90	/* 4-byte boundaries, NOP-filled */
#define	ENTRY(name)	.globl _ ## name; ALIGN_TEXT; _ ## name:

#include "assym.h"
#include <machine/param.h>
#include <machine/specialreg.h>
#include <machine/segments.h>
#include <machine/apmvar.h>
#include <machine/psl.h>

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
	.set AX_REGADDR,(GDT_COPY_ADDR-2)
	.set BX_REGADDR,(AX_REGADDR-2)
	.set CX_REGADDR,(BX_REGADDR-2)
	.set DX_REGADDR,(CX_REGADDR-2)
	.set SI_REGADDR,(DX_REGADDR-2)
	.set DI_REGADDR,(SI_REGADDR-2)
	.set FLAGS_REGADDR,(DI_REGADDR-2)
	.set ENDREGADDR,(FLAGS_REGADDR-2)
	
	.set REALSTACK,ENDREGADDR-16		# leave a red zone?

#define COPY_FLAGS (PSL_C|PSL_PF|PSL_AF|PSL_Z|PSL_N|PSL_D|PSL_V)

/*
 * do_bios_call(int function, struct apmregs *regs)
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
	movw APMREG_AX(%ebx),%ax
	movw %ax,AX_REGADDR
	movw APMREG_BX(%ebx),%ax
	movw %ax,BX_REGADDR
	movw APMREG_CX(%ebx),%ax
	movw %ax,CX_REGADDR
	movw APMREG_DX(%ebx),%ax
	movw %ax,DX_REGADDR
	movw APMREG_SI(%ebx),%ax
	movw %ax,SI_REGADDR
	movw APMREG_DI(%ebx),%ax
	movw %ax,DI_REGADDR
	# merge current flags with certain provided flags
	movw APMREG_FLAGS(%ebx),%cx
	pushfl
	popl %eax
	andl $~(COPY_FLAGS|PSL_I),%eax
	andl $COPY_FLAGS,%ecx
	orl %ecx,%eax
	movw %ax,FLAGS_REGADDR
	
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
	# all these movl's are really movw's !
	addr32
	movl DI_REGADDR-MYBASE,%edi
	addr32
	movl SI_REGADDR-MYBASE,%esi
	addr32
	movl DX_REGADDR-MYBASE,%edx
	addr32
	movl CX_REGADDR-MYBASE,%ecx
	addr32
	movl BX_REGADDR-MYBASE,%ebx
	addr32
	movb FUNCTION_ADDR-MYBASE,%al
	addr32
	movb %al,intaddr+1	# self modifying code, yuck. no indirect interrupt instruction!
	# long jump to flush processor cache to reflect code modification
	data32
	ljmp $(MYBASE>>4),$flushit
flushit:
	addr32
	movl FLAGS_REGADDR-MYBASE,%eax
	pushl %eax
	popfl
	addr32
	movl AX_REGADDR-MYBASE,%eax
	
intaddr: 
	int $0xff

	# save results
	pushf
	addr32
	movl %eax,AX_REGADDR-MYBASE
	addr32
	movl %ebx,BX_REGADDR-MYBASE
	addr32
	movl %ecx,CX_REGADDR-MYBASE
	addr32
	movl %edx,DX_REGADDR-MYBASE
	addr32
	movl %esi,SI_REGADDR-MYBASE
	addr32
	movl %edi,DI_REGADDR-MYBASE
	pop %eax
	addr32
	movl %eax,FLAGS_REGADDR-MYBASE

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
	movw AX_REGADDR,%ax
	movw %ax,APMREG_AX(%ebx)
	movw BX_REGADDR,%ax
	movw %ax,APMREG_BX(%ebx)
	movw CX_REGADDR,%ax
	movw %ax,APMREG_CX(%ebx)
	movw DX_REGADDR,%ax
	movw %ax,APMREG_DX(%ebx)
	movw SI_REGADDR,%ax
	movw %ax,APMREG_SI(%ebx)
	movw DI_REGADDR,%ax
	movw %ax,APMREG_DI(%ebx)
	movw FLAGS_REGADDR,%ax
	movw %ax,APMREG_FLAGS(%ebx)
	
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

	.align 4
IDT_bios:			# BIOS IDT descriptor (real-mode)
	.word 1023
	.long 0
