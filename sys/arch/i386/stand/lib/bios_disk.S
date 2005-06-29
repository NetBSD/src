/*	$NetBSD: bios_disk.S,v 1.16 2005/06/29 18:02:52 junyoung Exp $	*/

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

/* extracted from netbsd:sys/arch/i386/boot/bios.S */

#include <machine/asm.h>

/*
 * BIOS call "INT 0x13 Function 0x0" to reset the disk subsystem
 *	Call with	%ah = 0x0
 *			%dl = drive (0x80 for hard disk, 0x0 for floppy disk)
 *	Return:
 *			%al = 0x0 on success; err code on failure
 */
ENTRY(biosdiskreset)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	push	%edx
	push	%edi

	movb	8(%ebp), %dl	# device

	call	_C_LABEL(prot_to_real)	# enter real mode
	.code16

	movb	$0x0, %ah	# subfunction
	int	$0x13
	setc	%bl
	movb	%ah, %bh	# save error code

	calll	_C_LABEL(real_to_prot) # back to protected mode
	.code32

	xorl	%eax, %eax
	movw	%bx, %ax	# return value in %ax

	pop	%edi
	pop	%edx
	popl	%ebx
	popl	%ebp
	ret

/*
 * BIOS call "INT 0x13 Function 0x2" to read sectors from disk into memory
 *	Call with	%ah = 0x2
 *			%al = number of sectors
 *			%ch = cylinder
 *			%cl = sector
 *			%dh = head
 *			%dl = drive (0x80 for hard disk, 0x0 for floppy disk)
 *			%es:%bx = segment:offset of buffer
 *	Return:
 *			%al = 0x0 on success; err code on failure
 *
 * biosread( dev, cyl, head, sect, count, buff_addr );
 *
 *  Note: On failure, you must reset the disk with biosdiskreset() before
 *        sending another command.
 */
ENTRY(biosread)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	push	%ecx
	push	%edx
	push	%esi
	push	%edi

	movb	16(%ebp), %dh
	movw	12(%ebp), %cx
	xchgb	%ch, %cl	# cylinder; the highest 2 bits of cyl is in %cl
	rorb	$2, %cl
	movb	20(%ebp), %al
	orb	%al, %cl
	incb	%cl		# sector; sec starts from 1, not 0
	movb	8(%ebp), %dl	# device
	movl	28(%ebp), %ebx	# buffer address (may be >64k)
	movb	24(%ebp), %al	# number of sectors

	call	_C_LABEL(prot_to_real)	# enter real mode
	.code16

	push	%bx
	shrl	$4, %ebx	# max segment
	mov	%ds, %si
	add	%si, %bx
	mov	%bx, %es	# %es:%bx now valid buffer address
	pop	%bx
	and	$0xf, %bx	# and min offset - to avoid overrun

	movb	$0x2, %ah	# subfunction
	int	$0x13
	setc	%al		# error code is in %ah

	calll	_C_LABEL(real_to_prot) # back to protected mode
	.code32

	andl	$0xffff, %eax

	pop	%edi
	pop	%esi
	pop	%edx
	pop	%ecx
	popl	%ebx
	popl	%ebp
	ret

/*
 * get_diskinfo():  return a word that represents the
 *	max number of sectors, heads and cylinders for this device
 */
ENTRY(get_diskinfo)
	pushl	%ebp
	movl	%esp, %ebp
	push	%es
	pushl	%ebx
	push	%ecx
	push	%edx
	push	%esi
	push	%edi

	movb	8(%ebp), %dl		# diskinfo(drive #)

	call	_C_LABEL(prot_to_real)	# enter real mode
	.code16

	push	%dx			# save drive #
	movb	$0x08, %ah		# ask for disk info
	int	$0x13
	pop	%bx			# restore drive #
	jnc	ok

	testb	$0x80, %bl		# is it a hard disk?
	jnz	ok

	/*
	 * Urk.  Call failed.  It is not supported for floppies by old BIOS's.
	 * Guess it's a 15-sector floppy.  Initialize all the registers for
	 * documentation, although we only need head and sector counts.
	 */
	xorw	%ax, %ax		# set status to success
#	movb	%ah, %bh		# %bh = 0
#	movb	$2, %bl			# %bl bits 0-3 = drive type, 2 = 1.2M
	movb	$79, %ch		# max track
	movb	$15, %cl		# max sector
	movb	$1, %dh			# max head
#	movb	$1, %dl			# # floppy drives installed
	# es:di = parameter table
	# carry = 0

ok:
	calll	_C_LABEL(real_to_prot)	# back to protected mode
	.code32

	/* form a longword representing all this gunk */
	shrl	$8, %eax		# clear unnecessary bits
	shll	$24, %eax
	shll	$16, %ecx		# do the same for %ecx
	shrl	$8, %ecx
	movb	%dh, %cl		# max head
	orl	%ecx, %eax		# return value in %eax

	pop	%edi
	pop	%esi
	pop	%edx
	pop	%ecx
	popl	%ebx
	pop	%es
	popl	%ebp
	ret

/*
 * int13_extension: check for availibility of int13 extensions.
 */
ENTRY(int13_extension)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi
	pushl	%edi

	movb	8(%ebp), %dl		# drive #
	movw	$0x55aa, %bx

	call	_C_LABEL(prot_to_real)	# enter real mode
	.code16

	movb	$0x41, %ah		# ask for disk info
	int	$0x13
	setnc	%dl

	calll	_C_LABEL(real_to_prot)	# switch back
	.code32

	xorl	%eax, %eax
	movb	%dl, %al	# return value in %ax

	cmpw	$0xaa55, %bx
	sete	%dl
	andb	%dl, %al

	andb	%cl, %al

	popl	%edi
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp
	ret

/*
 * BIOS call "INT 0x13 Function 0x42" to read sectors from disk into memory
 *	Call with	%ah = 0x42
 *			%ds:%si = parameter block (data buffer address
 *				must be a real mode physical address).
 *			%dl = drive (0x80 for hard disk, 0x0 for floppy disk)
 *	Return:
 *			%al = 0x0 on success; err code on failure
 */
ENTRY(biosextread)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	push	%ecx
	push	%edx
	push	%esi
	push	%edi

	movb	8(%ebp), %dl	# device
	movl	12(%ebp), %esi	# parameter block

	call	_C_LABEL(prot_to_real)	# enter real mode
	.code16

	movl	%esi, %eax
	shrl	$4, %eax
	movw	%ds, %bx
	addw	%bx, %ax
	movw	%ax, %ds
	andw	$0xf, %si

	movb	$0x42, %ah	# subfunction
	int	$0x13
	setc	%bl
	movb	%ah, %bh	# save error code

	calll	_C_LABEL(real_to_prot) # back to protected mode
	.code32

	xorl	%eax, %eax
	movw	%bx, %ax	# return value in %ax

	pop	%edi
	pop	%esi
	pop	%edx
	pop	%ecx
	popl	%ebx
	popl	%ebp
	ret

ENTRY(int13_getextinfo)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	push	%ecx
	push	%edx
	push	%esi
	push	%edi

	movb	8(%ebp), %dl	# device
	movl	12(%ebp), %esi	# parameter block

	call	_C_LABEL(prot_to_real)	# enter real mode
	.code16

	movl	%esi, %eax
	shrl	$4, %eax
	andw	$0xf, %si
	movw	%ds, %bx
	addw	%bx, %ax
	movw	%ax, %ds

	movb	$0x48, %ah	# subfunction
	int	$0x13
	setc	%bl

	calll	_C_LABEL(real_to_prot) # back to protected mode
	.code32

	xorl	%eax, %eax
	movb	%bl, %al	# return value in %ax

	pop	%edi
	pop	%esi
	pop	%edx
	pop	%ecx
	popl	%ebx
	popl	%ebp
	ret
