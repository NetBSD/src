;	$NetBSD: arm.asm,v 1.2.2.1 2001/08/03 04:11:31 lukem Exp $	
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
	
;
;armasm.exe $(InputPath)
;arm.obj
;
	; dummy buffer for WritebackDCache
	EXPORT	|dcachebuf|	[DATA]
	AREA	|.data|, DATA
|dcachebuf|	
	%	8192	; D-cache size

	AREA	|.text|, CODE, PIC
	
	;
	; Operation mode ops.
	;
	EXPORT	|SetSVCMode|
|SetSVCMode| PROC
	mrs	r0, cpsr
	bic	r0, r0, #0x1f
	orr	r0, r0, #0x13
	msr	cpsr, r0
	mov	pc, lr
	ENDP  ; |SetSVCMode|
	EXPORT	|SetSystemMode|
|SetSystemMode| PROC
	mrs	r0, cpsr
	orr	r0, r0, #0x1f
	msr	cpsr, r0
	mov	pc, lr
	ENDP  ; |SetSystemMode|

	;
	; Interrupt ops.
	;
	EXPORT	|DI|
|DI| PROC
	mrs	r0, cpsr
	orr	r0, r0, #0xc0
	msr	cpsr, r0
	mov	pc, lr
	ENDP  ; |DI|
	EXPORT	|EI|
|EI| PROC
	mrs	r0, cpsr
	bic	r0, r0, #0xc0
	msr	cpsr, r0
	mov	pc, lr
	ENDP  ; |EI|
	
	;
	; Cache ops.
	;
	EXPORT	|InvalidateICache|
|InvalidateICache| PROC
	; c7	(CRn) Cache Control Register
	; c5, 0	(CRm, opcode_2) Flush I
	; r0	(Rd) ignored
	mcr	p15, 0, r0, c7, c5, 0
	mov	pc, lr
	ENDP  ; |InvalidateICache|
	
	EXPORT	|WritebackDCache|
|WritebackDCache| PROC
	ldr	r0, [pc, #16]	; dcachebuf
	add	r1, r0, #8192	; cache-size is 8Kbyte.
|wbdc1|
	ldr	r2, [r0], #32	; line-size is 32byte.
	teq	r1, r0
	bne	|wbdc1|
	mov	pc, lr
	DCD	|dcachebuf|
	ENDP  ; |WritebackDCache|
	
	EXPORT	|InvalidateDCache|
|InvalidateDCache| PROC
	; c7	(CRn) Cache Control Register
	; c6, 0	(CRm, opcode_2) Flush D
	; r0	(Rd) ignored
	mcr	p15, 0, r0, c7, c6, 0	
	mov	pc, lr
	ENDP  ; |InvalidateDCache|

	EXPORT	|WritebackInvalidateDCache|
|WritebackInvalidateDCache| PROC
	ldr	r0, [pc, #20]	; dcachebuf
	add	r1, r0, #8192
|wbidc1|
	ldr	r2, [r0], #32
	teq	r1, r0
	bne	|wbidc1|
	mcr	p15, 0, r0, c7, c6, 0
	mov	pc, lr
	DCD	|dcachebuf|
	ENDP  ; |WritebackInvalidateDCache|

	;
	; WriteBuffer ops
	;
	EXPORT	|WritebufferFlush|
|WritebufferFlush| PROC
	; c7	(CRn) Cache Control Register
	; c10, 4(CRm, opcode_2) Flush D
	; r0	(Rd) ignored
	mcr	p15, 0, r0, c7, c10, 4
	mov	pc, lr
	ENDP  ; |WritebufferFlush|

	;
	;	TLB ops.
	;
	EXPORT	|FlushIDTLB|
|FlushIDTLB| PROC
	mcr	p15, 0, r0, c8, c7, 0
	mov	pc, lr
	ENDP  ; |FlushIDTLB|

	EXPORT	|FlushITLB|
|FlushITLB| PROC
	mcr	p15, 0, r0, c8, c5, 0
	mov	pc, lr
	ENDP  ; |FlushITLB|
	
	EXPORT	|FlushDTLB|
|FlushDTLB| PROC
	mcr	p15, 0, r0, c8, c6, 0
	mov	pc, lr
	ENDP  ; |FlushITLB|

	EXPORT	|FlushDTLBS|
|FlushDTLBS| PROC
	mcr	p15, 0, r0, c8, c6, 1
	mov	pc, lr
	ENDP  ; |FlushITLBS|

	;
	;	CurrentProgramStatusRegister access.
	;
	EXPORT	|GetCPSR|
|GetCPSR| PROC
	mrs	r0, cpsr
	mov	pc, lr
	ENDP  ; |GetCPSR|

	EXPORT	|SetCPSR|	
|SetCPSR| PROC
	msr	cpsr, r0
	mov	pc, lr
	ENDP  ; |SetCPSR|

	;
	;	SA-1100 Coprocessor15 access.
	;
; Reg0	ID (R)
	EXPORT	|GetCop15Reg0|
|GetCop15Reg0| PROC
	mrc	p15, 0, r0, c0, c0, 0
	; 0x4401a119 (44|01 = version 4|A11 = SA1100|9 = E stepping)
	mov	pc, lr
	ENDP  ; |GetCop15Reg0|

; Reg1	Control (R/W)
	EXPORT	|GetCop15Reg1|
|GetCop15Reg1| PROC
	mrc	p15, 0, r0, c1, c0, 0
	; 0xc007327f (||...........|||..||..|..|||||||)
	;	0 (1)MMU enabled
	;	1 (1)Address fault enabled
	;	2 (1)D-cache enabled
	;	3 (1)Write-buffer enabled
	;	7 (0)little-endian
	;	8 (0)MMU protection (System)
	;	9 (1)MMU protection (ROM)
	;	12 (1)I-cache enabled
	;	13 (1)Base address of interrupt vector is 0xffff0000
	mov	pc, lr
	ENDP  ; |GetCop15Reg1|
	EXPORT	|SetCop15Reg1|	
|SetCop15Reg1| PROC
	mcr	p15, 0, r0, c1, c0, 0
	nop
	nop
	nop
	mov	pc, lr
	ENDP  ; |SetCop15Reg1|
	
; Reg2	Translation table base (R/W)	
	EXPORT	|GetCop15Reg2|	
|GetCop15Reg2| PROC
	mrc	p15, 0, r0, c2, c0, 0
	mov	pc, lr
	ENDP  ; |GetCop15Reg2|
	EXPORT	|SetCop15Reg2|	
|SetCop15Reg2| PROC
	mcr	p15, 0, r0, c2, c0, 0
	mov	pc, lr
	ENDP  ; |SetCop15Reg2|

; Reg3	Domain access control (R/W)
	EXPORT	|GetCop15Reg3|
|GetCop15Reg3| PROC
	mrc	p15, 0, r0, c3, c0, 0
	mov	pc, lr
	ENDP  ; |GetCop15Reg3|
	EXPORT	|SetCop15Reg3|	
|SetCop15Reg3| PROC
	mcr	p15, 0, r0, c3, c0, 0
	mov	pc, lr
	ENDP  ; |SetCop15Reg3|
	
; Reg5	Fault status (R/W)
	EXPORT	|GetCop15Reg5|	
|GetCop15Reg5| PROC
	mrc	p15, 0, r0, c5, c0, 0
	mov	pc, lr
	ENDP  ; |GetCop15Reg5|
	
; Reg6	Fault address (R/W)	
	EXPORT	|GetCop15Reg6|	
|GetCop15Reg6| PROC
	mrc	p15, 0, r0, c6, c0, 0
	mov	pc, lr
	ENDP  ; |GetCop15Reg6|

; Reg7	Cache operations (W)
	; -> Cache ops
; Reg8	TLB operations (Flush) (W)
	; -> TLB ops
; Reg9	Read buffer operations (W)	
; Reg13	Process ID (R/W)	
	EXPORT	|GetCop15Reg13|	
|GetCop15Reg13| PROC
	mrc	p15, 0, r0, c13, c0, 0
	mov	pc, lr
	ENDP  ; |GetCop15Reg13|
	EXPORT	|SetCop15Reg13|	
|SetCop15Reg13| PROC
	mcr	p15, 0, r0, c13, c0, 0
	mov	pc, lr
	ENDP  ; |SetCop15Reg13|
	
; Reg14	Breakpoint (R/W)
	EXPORT	|GetCop15Reg14|	
|GetCop15Reg14| PROC
	mrc	p15, 0, r0, c14, c0, 0
	mov	pc, lr
	ENDP  ; |GetCop15Reg14|
; Reg15	Test, clock, and idle (W)
	
	; FlatJump (kaddr_t bootinfo, kaddr_t pvec, kaddr_t stack
	;		kaddr_t jump)
	;	bootinfo	boot information block address.
	;	pvec		page vector of kernel.
	;	stack		physical address of stack
	;	jump		physical address of boot function 
	; *** MMU and pipeline behavier are SA-1100 specific. ***
	EXPORT	|FlatJump|
|FlatJump| PROC
	; disable interrupt
	mrs	r4, cpsr
	orr	r4, r4, #0xc0
	msr	cpsr, r4
	; disable MMU, I/D-Cache, Writebuffer.
	; interrupt vector address is 0xffff0000
	; 32bit exception handler/address range.
	ldr	r4, [pc, #24]
	; Disable WB/Cache/MMU
	mcr	p15, 0, r4, c1, c0, 0
	; Invalidate I/D-cache.
	mcr	p15, 0, r4, c7, c7, 0	; Fetch translated	fetch
	; Invalidate TLB entries.
	mcr	p15, 0, r4, c8, c7, 0	; Fetch translated	decode
	; jump to kernel entry physical address.
	mov	pc, r3			; Fetch translated	execute
	; NOTREACHED
	nop				; Fetch nontranslated	cache access
	nop				; Fetch nontranslated	writeback
	mov	pc, lr			; Fetch nontranslated
	DCD	0x00002030
	ENDP  ; |FlatJump|
;
;	UART test
;	
	; boot_func (u_int32_t mapaddr, u_int32_t bootinfo, u_int32_t flags)
	;
	EXPORT	|boot_func|
|boot_func| PROC
	nop				; Cop15 hazard
	nop				; Cop15 hazard
	nop				; Cop15 hazard
	mov	sp, r2			; set bootloader stack
;	mov	r4, r0
;	mov	r5, r1
;	bl	colorbar
;	mov	r0, r4
;	mov	r1, r5
	bl	boot
	nop	; NOTREACHED
	nop
	ENDP  ; |boot_func|	
	
	EXPORT |colorbar|
|colorbar| PROC	
	stmea	sp!, {r4-r7, lr}	
	adr	r4, |$FBADDR|	
	ldr	r4, [r4]
	
	mov	r7, #8
	add	r0, r0, r7
|color_loop|
	mov	r6, r0
	and	r6, r6, #7
	orr	r6, r6, r6, LSL #8	
	orr	r6, r6, r6, LSL #16
	add	r5, r4, #0x9600
|fb_loop|
	str	r6, [r4], #4
	cmp	r4, r5
	blt	|fb_loop|
	
	subs	r7, r7, #1
	bne	|color_loop|	
	
	ldmea	sp!, {r4-r7, pc}
|$FBADDR|
	DCD	0xc0003000	; use WindowsCE default.
	ENDP  ; |colorbar|	
	
	EXPORT	|boot|
|boot| PROC
;
;	UART test code
;	
;	; print boot_info address (r0) and page_vector start address (r1).
;	mov	r4, r0
;	mov	r5, r1
;	mov	r0, #'I'
;	bl	btputc
;	mov	r0, r4
;	bl	hexdump
;	mov	r0, #'P'
;	bl	btputc
;	mov	r0, r5
;	bl	hexdump
;	mov	r7, r4
;	mov	r2, r5		; start	
	
	mov	r7, r0		; if enabled above debug print, remove this.
	mov	r2, r1		; if enabled above debug print, remove this.
|page_loop|	
	mvn	r0, #0		; ~0
	cmp	r2, r0
	beq	|page_end|	; if (next == ~0) goto page_end
	
	mov	r1, r2		; p = next
	ldr	r2, [r1]	; next
	ldr	r3, [r1, #4]	; src
	ldr	r4, [r1, #8]	; dst
	ldr	r5, [r1, #12]	; sz
	
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
;	ldr	r0, [r7]
;	ldr	r0, [r0]
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
	and	r2, r2, #1
	cmp	r2, #1
	beq	|btputc_busy|
	adr	r1, |$UARTTXADR|
	ldr	r1, [r1]
	str	r0, [r1]
	mov	pc, lr
	ENDP	;|btputc|

|hexdump| PROC
	stmea	sp!, {r4-r5, lr}
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
	and	r4, r4, #1
	cmp	r4, #1
	beq	|hex_busyloop|
	str	r3, [r1]
	mov	r0, r0, LSL #4
	subs	r5, r5, #1
	bne	|hex_loop|
	mov	r0, #0x0d
	bl	btputc
	mov	r0, #0x0a
	bl	btputc
	ldmea	sp!, {r4-r5, pc}
	ENDP	;|hexdump|
	
|$UARTTXADR|
	DCD	0x80050014
|$UARTTXBSY|	
	DCD	0x80050020	
	
	EXPORT	|boot_func_end| [ DATA ]
|boot_func_end|	DCD	0x0	

	END
