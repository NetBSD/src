;	$NetBSD: milli_tiny.S,v 1.1.10.2 2014/08/20 00:03:05 tls Exp $

; Copyright (c) 2003 ITOH Yasufumi.
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary forms are unlimited.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS''
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
; THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
; PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS
; BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
; THE POSSIBILITY OF SUCH DAMAGE.

; millicode library, optimized for size

	.level	1.0
	.code
	.align	4

; $$divU	unsigned division, return quotient
;
; inputs:
;	%r26	dividend
;	%r25	divisor
;	%r31	return address
; outputs:
;	%r29	quotient
;	%r1, %r25, %r26	undefined
	.export		$$divU,millicode
$$divU:
	.proc
	.callinfo	millicode,no_unwind
	.entry
	comb,<,n	%r25,0,bigdivisor_divU	; special case (>=0x80000000)
	bl		sub_divU,%r29
	subt,=		%r0,%r25,%r1		; trap divide by 0, negate

	bv		%r0(%r31)		; return millicode
	.exit
	addc		%r26,%r26,%r29		; fix quotient
bigdivisor_divU:
	comclr,<<	%r26,%r25,%r29		; if dividend >= divisor
	ldi		1,%r29			;   quotient is 1
	bv,n		%r0(%r31)		; return millicode
	.procend

; Note this is not a normal subroutine
; r29: return address
sub_divU:
	stwm		%r19,64(%sp)
	ldi		31,%r19

	ds		%r0,%r1,%r0
	addc		%r26,%r26,%r26
	ds		%r0,%r25,%r1
loop_sub_divU:					; addc/ds 31 times
	addc		%r26,%r26,%r26
	addib,<>	-1,%r19,loop_sub_divU
	ds		%r1,%r25,%r1

	bv		%r0(%r29)
	ldwm		-64(%sp),%r19

; $$remU	unsigned division, return remainder
;
; inputs:
;	%r26	dividend
;	%r25	divisor
;	%r31	return address
; outputs:
;	%r29	remainder
;	%r1, %r25, %r26	undefined
	.export		$$remU,millicode
$$remU:
	.proc
	.callinfo	millicode,no_unwind
	.entry
	comb,<,n	%r25,0,bigdivisor_remU	; special case (>=0x80000000)
	bl		sub_divU,%r29
	subt,=		%r0,%r25,%r1		; trap divide by 0, negate

	comclr,>=	%r1,%r0,%r0
	addl		%r1,%r25,%r1		; fix remainder
	bv		%r0(%r31)		; return millicode
	.exit
	copy		%r1,%r29		; the return value is remainder
bigdivisor_remU:
	sub,>>=		%r26,%r25,%r29		; if dividend < divisor
	copy		%r26,%r29		;   the remainder is dividend
	bv,n		%r0(%r31)		; return millicode
	.procend

; $$mulU	unsigned multiplication
;
; inputs:
;	%r26	multiplicand
;	%r25	multiplier
;	%r31	return address
; outputs:
;	%r29	product
;	%r1, %r25, %r26	undefined
	.export		$$mulU,millicode
	.export		$$mulI,millicode
$$mulU:
$$mulI:	; XXX actually wrong (not signed) but works for small positive numbers
	.proc
	.callinfo	frame=0,no_calls,millicode
	.entry
	copy		%r0,%r29
	ldi		32,%r1			; loop counter

	add,nuv		%r25,%r25,%r25		; shift left, skip next if not C
loop_mul:
	sh1add,tr	%r29,%r26,%r29		; shift left and add, skip next
	sh1add		%r29,%r0,%r29		; shift left only
	addib,<>,n	-1,%r1,loop_mul		; check loop condition
	add,nuv		%r25,%r25,%r25		; shift left, skip next if not C
	.exit
	bv,n		%r0(%r31)		; return millicode
	.procend
