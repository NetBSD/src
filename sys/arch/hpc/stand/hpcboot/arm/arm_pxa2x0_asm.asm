;	$NetBSD: arm_pxa2x0_asm.asm,v 1.1.4.2 2010/05/30 05:16:50 rmind Exp $	
;
; Copyright (c) 2001 The NetBSD Foundation, Inc.
; All rights reserved.
;
; This code is derived from software contributed to The NetBSD Foundation
; by UCHIYAMA Yasushi.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
; 3. All advertising materials mentioning features or use of this software
;    must display the following acknowledgement:
;        This product includes software developed by the NetBSD
;        Foundation, Inc. and its contributors.
; 4. Neither the name of The NetBSD Foundation nor the names of its
;    contributors may be used to endorse or promote products derived
;    from this software without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
; ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
; TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
; PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
; BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.
;

	AREA	|.text|, CODE, PIC

;
;armasm.exe $(InputPath)
;arm.obj
;
	; FlatJump_pxa2x0 (kaddr_t bootinfo, kaddr_t pvec, kaddr_t stack
	;		kaddr_t jump)
	;	bootinfo	boot information block address.
	;	pvec		page vector of kernel.
	;	stack		physical address of stack
	;	jump		physical address of boot function
	EXPORT	|FlatJump_pxa2x0|
|FlatJump_pxa2x0| PROC
	; disable interrupt
	mrs	r4, cpsr
	orr	r4, r4, #0xc0
	msr	cpsr, r4
	; Invalidate I/D-cache.
	mcr	p15, 0, r4, c7, c7, 0
	mov	r4, r4
	sub	pc, pc, #4
	; disable MMU, I/D-Cache, Writebuffer.
	; interrupt vector address is 0xffff0000
	; 32bit exception handler/address range.
	ldr	r4, [pc, #20]
	; Disable WB/Cache/MMU
	mcr	p15, 0, r4, c1, c0, 0
	; Invalidate TLB entries.
	mcr	p15, 0, r4, c8, c7, 0
	mov	r4, r4			; wait for it to complete
	sub	pc, pc, #4		; branch to next insn
	mov	pc, r3
	; NOTREACHED
	mov	pc, lr
	DCD	0x00002030
	ENDP  ; |FlatJump_pxa2x0|
;
;	UART test
;
	; boot_func (uint32_t mapaddr, uint32_t bootinfo, uint32_t flags)
	;
	EXPORT	|boot_func_pxa2x0|
|boot_func_pxa2x0| PROC
	nop				; cop15 hazard
	nop				; cop15 hazard
	nop				; cop15 hazard
	mov	sp, r2			; set bootloader stack
	bl	boot_pxa2x0
	nop	; NOTREACHED
	nop
	ENDP  ; |boot_func_pxa2x0|

	EXPORT	|boot_pxa2x0|
|boot_pxa2x0| PROC
	mov	r4, r0
	mov	r5, r1

;
;	UART test code
;
;	; print boot_info address (r0) and page_vector start address (r1).
;	mov	r0, #'I'
;	bl	btputc
;	mov	r0, r4
;	bl	hexdump
;	mov	r0, #'P'
;	bl	btputc
;	mov	r0, r5
;	bl	hexdump

	mov	r7, r4
	mov	r2, r5		; start
|page_loop|
	mvn	r0, #0		; ~0
	cmp	r2, r0
	beq	|page_end|	; if (next == ~0) goto page_end

	mov	r1, r2		; p = next
	ldr	r2, [r1]	; next
	ldr	r3, [r1, #4]	; src
	ldr	r4, [r1, #8]	; dst
	ldr	r5, [r1, #12]	; sz

	bic	r4, r4, #0xff000000
	orr	r4, r4, #0xa0000000

	cmp	r3, r0
	add	r6, r4, r5	; end address
	bne	|page_memcpy4|	; if (src != ~0) goto page_memcpy4

	mov	r0, #0
|page_memset|			; memset (dst, 0, sz) uncached.
	str	r0, [r4], #4
	cmp	r4, r6
	blt	|page_memset|
	b	|page_loop|

|page_memcpy4|			; memcpy (dst, src, sz) uncached.
	ldr	r0, [r3], #4
	ldr	r5, [r3], #4
	str	r0, [r4], #4
	cmp	r4, r6
	strlt	r5, [r4], #4
	cmplt	r4, r6
	blt	|page_memcpy4|

	b	|page_loop|
|page_end|
	;
	; jump to kernel
	;
;	mov	r0, #'E'
;	bl	btputc
;	ldr	r0, [r7]
;	bl	hexdump

	; set stack pointer
	mov	r5, #4096
	add	r6, r6, #8192
	sub	r5, r5, #1
	bic	sp, r6, r5

	; set bootargs
	ldr	r4, [r7]
	ldr	r0, [r7, #4]
	ldr	r1, [r7, #8]
	ldr	r2, [r7, #12]
	bic	r4, r4, #0xff000000
	orr	r4, r4, #0xa0000000
	mov	pc, r4
	; NOTREACHED

|infinite_loop|
	nop
	nop
	nop
	nop
	nop
	b	|infinite_loop|
	ENDP  ; |boot|

|btputc| PROC
	adr	r1, |$UARTTXBSY|
	ldr	r1, [r1]
|btputc_busy|
	ldr	r2, [r1]
	ands	r2, r2, #0x20
	beq	|btputc_busy|
	adr	r1, |$UARTTXADR|
	ldr	r1, [r1]
	str	r0, [r1]
	adr	r1, |$UARTINTR|
	ldr	r1, [r1]
	mov	pc, lr
	ENDP	;|btputc|

|hexdump| PROC
	stmfd	sp!, {r4-r5, lr}
	mov	r4, r0
	mov	r0, #0x30
	bl	btputc
	mov	r0, #0x78
	bl	btputc
	mov	r0, r4
	;	Transmit register address
	adr	r1, |$UARTTXADR|
	ldr	r1, [r1]
	;	Transmit busy register address
	adr	r2, |$UARTTXBSY|
	ldr	r2, [r2]
	mov	r5, #8
|hex_loop|
	mov	r3, r0, LSR #28
	cmp	r3, #9
	addgt	r3, r3, #0x41 - 10
	addle	r3, r3, #0x30
|hex_busyloop|
	ldr	r4, [r2]
	ands	r4, r4, #0x20
	beq	|hex_busyloop|
	str	r3, [r1]
	adr	r4, |$UARTINTR|
	ldr	r4, [r4]
	mov	r0, r0, LSL #4
	subs	r5, r5, #1
	bne	|hex_loop|
	mov	r0, #0x0d
	bl	btputc
	mov	r0, #0x0a
	bl	btputc
	ldmfd	sp!, {r4-r5, pc}
	ENDP	;|hexdump|

	; FFUART
|$UARTTXADR|
	DCD	0x40100000
|$UARTTXBSY|
	DCD	0x40100014
|$UARTINTR|
	DCD	0x40100008

	EXPORT	|boot_func_end_pxa2x0| [ DATA ]
|boot_func_end_pxa2x0|	DCD	0x0

	END
