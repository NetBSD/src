/*	$NetBSD: smdk2800_start.S,v 1.1.2.2 2002/12/11 05:58:33 thorpej Exp $ */

/*
 * Copyright (c) 2002 Fujitsu Component Limited
 * Copyright (c) 2002 Genetec Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/asm.h>
#include <arm/armreg.h>
#include <arm/arm32/pte.h>

/*
 * Kernel start routine for Samsung SMDK2800.
 * This code is excuted at the very first after the kernel is loaded
 * by boot program on ROM or gzboot.
 */
	.text

	.global	_C_LABEL(smdk2800_start)
_C_LABEL(smdk2800_start):
	/* Are we running on ROM ? */
	cmp	pc, #0x08200000
	bhs	smdk2800_start_ram
	
	/* move me to RAM
	 * XXX: we can use memcpy if it is PIC
	 */
	ldr r1, Lcopy_size
	adr r0, _C_LABEL(smdk2800_start)
	add r1, r1, #3
	mov r1, r1, LSR #2
	mov r2, #0x08200000
	mov r4, r2

	cmp r0, r2
	bhs 5f

	/* src < dest. copy from top */
	add r0,r0,r1,LSL #2
	add r2,r2,r1,LSL #2

3:	ldr r3,[r0,#-4]!
	str r3,[r2,#-4]!
	subs r1,r1,#1
	bhi 3b
	b 7f

	/* src >= dest. copy from bottom */
5:	ldr r3,[r0],#4
	str r3,[r2],#4
	subs r1,r1,#1
	bhi 5b

7:	
	/* Jump to RAM */
	ldr r0, Lstart_off
	add pc, r4, r0

Lcopy_size:	.word _edata-_C_LABEL(smdk2800_start)
Lstart_off:	.word smdk2800_start_ram-_C_LABEL(smdk2800_start)

smdk2800_start_ram:	
	/*
	 *  Kernel is loaded in SDRAM (0x08200000..), and is expected to run
	 *  in VA 0xc0200000..  
	 */

	/* get ttb prepared by boot program */
	mrc p15, 0, r0, c2, c0, 0
	/* Disable MMU for a while */
	mrc	p15, 0, r2, c1, c0, 0
	bic	r2, r2, #CPU_CONTROL_MMU_ENABLE
	mcr	p15, 0, r2, c1, c0, 0

	nop
	nop
	nop
	
	/*
	 * Map VA 0xc0000000..0xc3ffffff to PA 0x08000000..0x09ffffff
	 */
	mov	r1, #(L1_S_AP(AP_KRW))
	orr	r1, r1, #(L1_TYPE_S)
	orr	r1, r1, #0x08000000
	mov	r2, #(0xc00 * 4)
1:
	str	r1, [r0, r2]
	add	r2, r2, #4
	add	r1, r1, #L1_S_SIZE
	cmp	r2, #(0xc20 * 4)
	blo	1b


	mcr	p15, 0, r0, c2, c0, 0	/* Set TTB */
	mcr	p15, 0, r0, c8, c7, 0	/* Flush TLB */

	/* Set the Domain Access register.  Very important! */
	mov	r0, #1
	mcr	p15, 0, r0, c3, c0, 0

	/* Enable MMU */
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #CPU_CONTROL_MMU_ENABLE
	mcr	p15, 0, r0, c1, c0, 0
	
	nop
	nop
	nop

	/* Jump to kernel code in TRUE VA */
	adr	r0, Lstart
	ldr	pc, [r0]

Lstart:
	.word	start
