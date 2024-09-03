/*	$NetBSD: biosmemx.S,v 1.11.8.1 2024/09/03 15:23:40 martin Exp $	*/

/*
 * Copyright (c) 1997, 1999
 *	Matthias Drochner.  All rights reserved.
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

#include <machine/asm.h>

	.text

/*
 * int getextmem2(int buffer[2])
 *
 * return: 0=OK, -1=error
 * buffer[0]: extmem kBytes below 16M (max 15M/1024)
 * buffer[1]: extmem above 16M, in 64k units
 */
ENTRY(getextmem2)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	push	%esi
	push	%edi

	call	_C_LABEL(prot_to_real)
	.code16

	xorl	%ebx,%ebx
	movl	$0xe801,%eax
	int	$0x15
	pushf

	movw	%si,%ax
	orw	%si,%bx
	jz	1f		/* if zero use configured values */
	movw	%cx,%ax		/* k below 16M (max 0x3c00 = 15MB) */
	movw	%dx,%bx		/* 64k above 16M */
1:
	popf
	setc	%bl

	calll	_C_LABEL(real_to_prot)
	.code32

	movl	8(%ebp),%edi
	xorl	%eax,%eax
	movw	%cx,%ax
	stosl
	movw	%dx,%ax
	stosl
	movb	%bl,%al
	cbw

	pop	%edi
	pop	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp
	ret

/*
 * int getmementry(int *iterator, int buffer[6])
 *
 * return: 0=ok, else error
 * buffer[0]: start of memory chunk
 * buffer[2]: length (bytes)
 * buffer[4]: type
 * buffer[5]: ACPI 3.0 Extended Attributes bitfield (unused)
 *
 * Some buggy BIOSes may write to 24 bytes even if only 20 were requested.
 * Therefore, the buffer is defined for 6 elements to avoid stack buffer
 * overruns.  See PR install/49470.
 *
 * More details can be found in the:
 *
 *	Advanced Configuration and Power Interface (ACPI)
 *	Specification, Release 6.5, 2022-08-29, UEFI Forum, Inc.,
 *	Sec. 15.1 `INT 15H E820H - Query System Address Map',
 *	pp. 756-757
 *	https://uefi.org/sites/default/files/resources/ACPI_Spec_6_5_Aug29.pdf#page=824
 *	https://uefi.org/specs/ACPI/6.5/15_System_Address_Map_Interfaces.html#int-15h-e820h-query-system-address-map
 *
 * as well as this OSDev.org wiki page:
 *
 *	https://wiki.osdev.org/Detecting_Memory_(x86)#BIOS_Function:_INT_0x15,_EAX_=_0xE820
 */
ENTRY(getmementry)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	push	%esi
	push	%edi

	movl	8(%ebp),%eax
	movl	0(%eax),%ebx		/* index */
	movl	$20,%ecx		/* Buffer size */
	movl	$0x534d4150,%edx	/* "SMAP" */
	movl	12(%ebp),%edi		/* buffer address */

	call	_C_LABEL(prot_to_real)
	.code16

	push	%di
	shrl	$4,%edi
	mov	%ds,%ax
	add	%di,%ax
	mov	%ax,%es
	pop	%di
	and	$0xf,%di		/* buffer address now in ES:DI */

	movl	$0xe820,%eax		/* Some BIOS check EAX value */
	int	$0x15

	setc	%cl

	calll	_C_LABEL(real_to_prot)
	.code32

	movl	8(%ebp),%eax
	movl	%ebx,0(%eax)		/* updated index */
	xorl	%eax,%eax
	movb	%cl,%al

	pop	%edi
	pop	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp
	ret

/*
 * int biosA20(void)
 *
 * return: 0=ok, else error
 */
ENTRY(biosA20)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	push	%esi
	push	%edi

	call	_C_LABEL(prot_to_real)
	.code16

	movl	$0x2401,%eax
	int	$0x15
	setc	%cl

	calll	_C_LABEL(real_to_prot)
	.code32

	movzbl	%cl,%eax

	pop	%edi
	pop	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp
	ret
