/*	$NetBSD: bootxx.S,v 1.2 2003/10/08 04:25:45 lukem Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Laight.
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

#include <machine/asm.h>
#include <sys/bootblock.h>

/*
 * Code linked to 0xa00 and copied to sectors 2+ of the netbsd boot
 * partition by MI /usr/sbin/installboot.
 * Read into memory by code in pbr.S
 *
 * On entry:
 * 	%dl			BIOS drive number
 *	%esi			Sector number of netbsd partition
 *	%cs, %ds, %es, %ss	All zero
 *	%sp			near 0xfffc
 */
	.text
	.code16
ENTRY(bootxx)
	jmp	1f
	.align	4
ENTRY(bootxx_magic)
	.long	X86_BOOT_MAGIC_1	/* checked by installboot & pbr code */
bootparams:				/* space for patchable variables */
	.long	1f - bootparams		/* length of this data area */
#include "boot_params.S"
	.space	4 * 4			/* some spare */

1:	call	gdt_fixup

	calll	real_to_prot
	.code32

	movl	$_end, %ecx		/* zero bss */
	movl	$__bss_start, %edi
	subl	%edi, %ecx
	shr	$2, %ecx		/* _end and __bss_start are aligned */
	xor	%eax, %eax
	rep
	stosl

	and	$0xff, %edx
	push	%esi			/* save args for secondary bootstrap */
	push	%edx
	push	%esi			/* args for boot1 */
	push	%edx
	call	_C_LABEL(boot1)		/* C code to load /boot */
	add	$8, %esp
	call	prot_to_real
	.code16

	test	%ax, %ax
	jnz	boot_fail

	pop	%edx			/* bios disk number */
	pop	%ebx			/* expected partition start sector */
	movl	$bootparams, %esi
	lcall	$SECONDARY_LOAD_ADDRESS/16, $0

boot_fail:
	push	%ax
	movw	$1f, %si
	call	message
	pop	%si
	call	message
	jmp	loopstop
1:	.asciz	"Boot failed: "

ENTRY(_rtt)
	.code32
	call	prot_to_real
	.code16
loopstop:
	movb	0x86, %ah		/* delay for about a second */
	movw	$16, %cx
	int	$0x15
	int	$0x18			/* might be a boot fail entry */
1:	sti
	hlt
	jmp	1b

	/*
	 * Vector the fs calls through here so we can support multiple
	 * file system types with one copy of the library code and
	 * multiple copies of this file.
	 */
	.global	xxfs_open, xxfs_close, xxfs_read, xxfs_stat
	.code32
xxfs_open:	jmp	XXfs_open
xxfs_close:	jmp	XXfs_close
xxfs_read:	jmp	XXfs_read
xxfs_stat:	jmp	XXfs_stat
