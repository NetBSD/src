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

	.file	"bios.s"

#include "asm.h"
	.text

/*
# biosread(dev, cyl, head, sec, nsec, offset)
#	Read "nsec" sectors from disk to offset "offset" in boot segment
# BIOS call "INT 0x13 Function 0x2" to read sectors from disk into memory
#	Call with	%ah = 0x2
#			%al = number of sectors
#			%ch = cylinder
#			%cl = sector
#			%dh = head
#			%dl = drive (0x80 for hard disk, 0x0 for floppy disk)
#			%es:%bx = segment:offset of buffer
#	Return:		
#			%al = 0x0 on success; err code on failure
*/

ENTRY(biosread)
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%es

	movb	0x10(%ebp), %dh
	movw	0x0c(%ebp), %cx
	xchgb	%ch, %cl	# cylinder; the highest 2 bits of cyl is in %cl
	rorb	$2, %cl
	movb	0x14(%ebp), %al
	orb	%al, %cl
	incb	%cl		# sector; sec starts from 1, not 0
	movb	0x8(%ebp), %dl	# device
	movl	0x1c(%ebp), %ebx	# offset
				# prot_to_real will set %es to BOOTSEG

	call	EXT(prot_to_real)	# enter real mode
	movb	$0x2, %ah	# subfunction
	addr32
	movb	0x18(%ebp), %al	# number of sectors

	sti
	int	$0x13
	cli

	movl	%eax, %ebx	# save return value (actually movw %ax, %bx)

	data32
	call	EXT(real_to_prot) # back to protected mode

	xorl	%eax, %eax
	movb	%bh, %al	# return value in %ax

	popl	%es
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp

	ret


/*
# putc(ch)
# BIOS call "INT 10H Function 0Eh" to write character to console
#	Call with	%ah = 0x0e
#			%al = character
#			%bh = page
#			%bl = foreground color ( graphics modes)
*/


ENTRY(putc)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%ecx

	movb	0x8(%ebp), %cl

	call	EXT(prot_to_real)

	data32
	movl	$0x1, %ebx	# %bh=0, %bl=1 (blue)
	movb	$0xe, %ah
	movb	%cl, %al
	sti
	int	$0x10		# display a byte
	cli

	data32
	call	EXT(real_to_prot)

	popl	%ecx
	popl	%ebx
	popl	%ebp
	ret


/*
# getc()
# BIOS call "INT 16H Function 00H" to read character from keyboard
#	Call with	%ah = 0x0
#	Return:		%ah = keyboard scan code
#			%al = ASCII character
*/

ENTRY(getc)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx		# save %ebx

	call	EXT(prot_to_real)

	movb	$0x0, %ah
	sti
	int	$0x16
	cli

	movb	%al, %bl	# real_to_prot uses %eax

	data32
	call	EXT(real_to_prot)

	xorl	%eax, %eax
	movb	%bl, %al

	popl	%ebx
	popl	%ebp
	ret
/*
# ischar()
#       if there is a character pending, return it; otherwise return 0
# BIOS call "INT 16H Function 01H" to check whether a character is pending
#	Call with	%ah = 0x1
#	Return:
#		If key waiting to be input:
#			%ah = keyboard scan code
#			%al = ASCII character
#			Zero flag = clear
#		else
#			Zero flag = set
*/
ENTRY(ischar)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx

	call	EXT(prot_to_real)		# enter real mode

	xorl	%ebx, %ebx
	movb	$0x1, %ah
	sti
	int	$0x16
	cli
	data32
	jz	nochar
	movb	%al, %bl

nochar:
	data32
	call	EXT(real_to_prot)

	xorl	%eax, %eax
	movb	%bl, %al

	popl	%ebx
	popl	%ebp
	ret

/*
#
# get_diskinfo():  return a word that represents the
#	max number of sectors and  heads and drives for this device
#
*/

ENTRY(get_diskinfo)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%es
	pushl	%ebx
	pushl	%ecx
	pushl	%edx

	movb	0x8(%ebp), %dl		# diskinfo(drive #)
	call	EXT(prot_to_real)	# enter real mode

	movb	$0x8, %ah		# ask for disk info

	sti
	int	$0x13
	cli

	jnc	ok
	/*
	 * Urk.  Call failed.  It is not supported for floppies by old BIOS's.
	 * Guess it's a 15-sector floppy.  Initialize all the registers for
	 * documentation, although we only need head and sector counts.
	 */
	subb	%ah, %ah		# %ax = 0
	movb	%ah, %bh		# %bh = 0
	movb	$2, %bl			# %bl bits 0-3 = drive type, 2 = 1.2M
	movb	$79, %ch		# max track
	movb	$15, %cl		# max sector
	movb	$1, %dh			# max head
	movb	$1, %dl			# # floppy drives installed
	# es:di = parameter table
	# carry = 0
ok:

	data32
	call	EXT(real_to_prot)	# back to protected mode

	xor	%eax, %eax

	/*form a longword representing all this gunk*/
	movb	%dh, %ah		# max head
	andb	$0x3f, %cl		# mask of cylinder gunk
	movb	%cl, %al		# max sector (and # sectors)

	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%es
	popl	%ebp
	ret

/*
#
# memsize(i) :  return the memory size in KB. i == 0 for conventional memory,
#		i == 1 for extended memory
#	BIOS call "INT 12H" to get conventional memory size
#	BIOS call "INT 15H, AH=88H" to get extended memory size
#		Both have the return value in AX.
#
*/

ENTRY(memsize)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx

	movl	8(%ebp), %ebx

	call	EXT(prot_to_real)		# enter real mode

	testb	%bl, %bl
	data32
	jnz	xext
	
	sti
	int	$0x12
	cli
	data32
	jmp	xdone

xext:	movb	$0x88, %ah
	sti
	int	$0x15
	cli

xdone:
	movl	%eax, %ebx

	data32
	call	EXT(real_to_prot)

	movl	%ebx, %eax
	popl	%ebx
	popl	%ebp
	ret
