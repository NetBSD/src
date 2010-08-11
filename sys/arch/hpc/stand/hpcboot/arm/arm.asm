;	$NetBSD: arm.asm,v 1.7.4.2 2010/08/11 22:52:03 yamt Exp $	
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
	EXPORT	|dcachesize|	[DATA]
	EXPORT	|dcachebuf|	[DATA]
	AREA	|.data|, DATA
|dcachesize|
	DCD	8192	; for SA1100
|dcachebuf|
	%	65536	; max D-cache size

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
	ldr	r0, [pc, #24]	; dcachebuf
	ldr	r1, [pc, #24]
	ldr	r1, [r1]	; dcache-size
	add	r1, r1, r0
|wbdc1|
	ldr	r2, [r0], #32	; line-size is 32byte.
	teq	r1, r0
	bne	|wbdc1|
	mov	pc, lr
	DCD	|dcachebuf|
	DCD	|dcachesize|
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
	ldr	r0, [pc, #28]	; dcachebuf
	ldr	r1, [pc, #28]
	ldr	r1, [r1]	; dcache-size
	add	r1, r1, r0
|wbidc1|
	ldr	r2, [r0], #32
	teq	r1, r0
	bne	|wbidc1|
	mcr	p15, 0, r0, c7, c6, 0
	mov	pc, lr
	DCD	|dcachebuf|
	DCD	|dcachesize|
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

	END
