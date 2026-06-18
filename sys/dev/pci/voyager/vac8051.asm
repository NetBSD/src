; $NetBSD: vac8051.asm,v 1.1 2026/06/18 00:44:21 rkujawa Exp $
;
; Copyright (c) 2026 The NetBSD Foundation, Inc.
; All rights reserved.
;
; This code is derived from software contributed to The NetBSD Foundation
; by Radoslaw Kujawa.
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
; SM502 8051 firmware for the NetBSD "vac" AC97 audio driver
;
; Assemble:  naken_asm -type bin -o vac8051.bin vac8051.asm
;            then  xxd -i < vac8051.bin 
;            into vac8051_fw[] in vac.c
;
; --- dual-port SRAM header (8051 xdata 0x3000 = host MMIO 0x0C3000) ---
;   H_CMD   0x3000  u8  host: command sequence (bumped per queued buffer)
;   H_DONE  0x3001  u8  8051: last fully-played command (host polls)
;   H_ALIVE 0x3003  u8  8051: 0x51 once the feeder is running
;   H_FC0   0x3004  u16 host: frame count for buf0 (even cmd), little-endian
;   H_FC1   0x3006  u16 host: frame count for buf1 (odd cmd), little-endian
;   BUF0    0x3010      504 frames * 4 bytes
;   BUF1    0x37F0
;
; --- AC97 link registers (8051 xdata view) ---
;   AC_TAG  0x9100  tag slot; 0x9800 = FRAME_VALID|S3_VALID|S4_VALID
;   AC_TXL  0x910C  slot 3 (PCM left),  20-bit MSB-justified (sample<<4)
;   AC_TXR  0x9110  slot 4 (PCM right)
;   AC_RXA  0x9144  RX slot1; R3 on-demand flag = bit 11 (0 = send next)

.8051

H_CMD   equ 0x3000
H_DONE  equ 0x3001
H_ALIVE equ 0x3003
H_FC0L  equ 0x3004
H_FC0H  equ 0x3005
H_FC1L  equ 0x3006
H_FC1H  equ 0x3007
BUF0    equ 0x3010
BUF1    equ 0x37F0	; BUF0 + 504*4 (0x7E0)

AC_TAG  equ 0x9100
AC_TXL  equ 0x910C
AC_TXR  equ 0x9110
AC_RXA  equ 0x9144

; idata scratch
LO      equ 0x20	; left sample low byte
HI      equ 0x21	; left sample high byte
SL      equ 0x22	; swap(LO)
SH      equ 0x23	; swap(HI)
RLO     equ 0x24	; right sample low byte
RHI     equ 0x25	; right sample high byte
TMP     equ 0x26

DPL     equ 0x82	; SFR: data pointer low
DPH     equ 0x83	; SFR: data pointer high

.org 0x0000
start:
	; announce we are alive
	mov dptr, #H_ALIVE
	mov a, #0x51
	movx @dptr, a
	; done_seq = 0; last-processed command (r7) = 0
	mov dptr, #H_DONE
	clr a
	movx @dptr, a
	mov r7, #0x00

mainloop:
	mov dptr, #H_CMD
	movx a, @dptr		; A = cmd_seq
	mov r6, a		; r6 = cmd_seq
	xrl a, r7		; A ^= last-processed
	jz mainloop		; nothing new -> idle

	; new command: buffer = cmd_seq & 1
	mov a, r6
	anl a, #0x01
	jz use0
	; --- buf1 (odd command) ---
	mov dptr, #H_FC1L
	movx a, @dptr
	mov r4, a
	mov dptr, #H_FC1H
	movx a, @dptr
	mov r5, a
	mov r2, #(BUF1 & 0xff)
	mov r3, #(BUF1 >> 8)
	sjmp haveframes
use0:
	; --- buf0 (even command) ---
	mov dptr, #H_FC0L
	movx a, @dptr
	mov r4, a
	mov dptr, #H_FC0H
	movx a, @dptr
	mov r5, a
	mov r2, #(BUF0 & 0xff)
	mov r3, #(BUF0 >> 8)
haveframes:

frameloop:
	; ---- pace: wait until R3 == 0 (link ready for slot 3), bounded ----
	mov r0, #0
	mov r1, #0
waitr3:
	mov dptr, #(AC_RXA + 1)
	movx a, @dptr
	anl a, #0x08
	jz r3ok
	djnz r1, waitr3
	djnz r0, waitr3
r3ok:

	; ---- fetch one stereo frame (Llo Lhi Rlo Rhi) from the buffer ----
	mov DPL, r2
	mov DPH, r3
	movx a, @dptr
	mov LO, a
	inc dptr
	movx a, @dptr
	mov HI, a
	inc dptr
	movx a, @dptr
	mov RLO, a
	inc dptr
	movx a, @dptr
	mov RHI, a
	inc dptr
	mov r2, DPL		; save advanced buffer pointer
	mov r3, DPH

	; ---- tag: FRAME_VALID | S3_VALID | S4_VALID = 0x9800 ----
	mov dptr, #AC_TAG
	clr a
	movx @dptr, a		; tag[7:0]
	inc dptr
	mov a, #0x98
	movx @dptr, a		; tag[15:8]

	; ---- left = (HI:LO) << 4, written as 3 bytes + zero ----
	mov a, LO
	swap a
	mov SL, a
	mov a, HI
	swap a
	mov SH, a
	mov dptr, #AC_TXL
	mov a, SL
	anl a, #0xf0
	movx @dptr, a		; byte0 = (LO.lo)<<4
	inc dptr
	mov a, SL
	anl a, #0x0f
	mov TMP, a
	mov a, SH
	anl a, #0xf0
	orl a, TMP
	movx @dptr, a		; byte1 = LO.hi | (HI.lo)<<4
	inc dptr
	mov a, SH
	anl a, #0x0f
	movx @dptr, a		; byte2 = HI.hi
	inc dptr
	clr a
	movx @dptr, a		; byte3 = 0

	; ---- right = (RHI:RLO) << 4 ----
	mov a, RLO
	swap a
	mov SL, a
	mov a, RHI
	swap a
	mov SH, a
	mov dptr, #AC_TXR
	mov a, SL
	anl a, #0xf0
	movx @dptr, a
	inc dptr
	mov a, SL
	anl a, #0x0f
	mov TMP, a
	mov a, SH
	anl a, #0xf0
	orl a, TMP
	movx @dptr, a
	inc dptr
	mov a, SH
	anl a, #0x0f
	movx @dptr, a
	inc dptr
	clr a
	movx @dptr, a

	; ---- decrement 16-bit frame count (r5:r4); loop while nonzero ----
	mov a, r4
	jnz nob
	dec r5
nob:
	dec r4
	mov a, r4
	orl a, r5
	jz drained
	ljmp frameloop		; (ljmp: frameloop is out of rel8 range)
drained:

	; ---- buffer drained: publish done_seq = cmd, mark processed ----
	mov dptr, #H_DONE
	mov a, r6
	movx @dptr, a
	mov r7, a		; a still holds r6
	ljmp mainloop
