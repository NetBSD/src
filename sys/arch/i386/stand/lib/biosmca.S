/*	$NetBSD: biosmca.S,v 1.1 2001/05/02 13:41:07 jdolecek Exp $	*/

/*
 * Ported to boot 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 *
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
  Copyright 1988, 1989, 1990, 1991, 1992 
   by Intel Corporation, Santa Clara, California.

                All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/* extracted from netbsd:sys/arch/i386/stand/bios_disk.S */

#include <machine/asm.h>

#define	addr32	.byte 0x67
#define	data32	.byte 0x66

/*
# BIOS call "INT 0x15 Function 0xc0" to read extended sys config info on PS/2
#	Return:		
#			%ah = 0x0 on success; err code on failure
#			%es:%bx = segment:offset of ROM address of sys config
# Called like
#	biosmca(&model, &features)
*/
ENTRY(biosmca)
	pushl	%ebp
	movl    %esp, %ebp
	pushl	%ebx
	push	%ecx
	push	%edx
	push	%esi
	push	%edi

	call	_C_LABEL(prot_to_real)	# enter real mode

	data32
	movl    $0xa0, %ax	# set table length to 0
	movb	$0xc0, %ah	# subfunction
	int	$0x15
	jc	err

	addr32
	movl	%es, %cx
	addr32
	movl	%bx, %dx
	data32
	movl	%ax, %bx
	jmp	back

err:
	data32
	movb	$0, %bx

back:
	data32
	call	_C_LABEL(real_to_prot) # back to protected mode

	xorl	%eax, %eax
	movw	%bx, %ax	# return value in %ax

	# save model
	movl	28(%esp), %ebx
	movl	%ecx, 0(%ebx)
	# save bios rev & features
	movl	32(%esp), %ebx
	movl	%edx, 0(%ebx)

	pop	%edi
	pop	%esi
	pop	%edx
	pop	%ecx
	popl	%ebx
	popl	%ebp
	ret
