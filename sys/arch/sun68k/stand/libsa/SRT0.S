| $NetBSD: SRT0.S,v 1.1 2001/06/14 12:57:13 fredette Exp $

| Copyright (c) 1998 The NetBSD Foundation, Inc.
| All rights reserved.
|
| This code is derived from software contributed to The NetBSD Foundation
| by Gordon W. Ross.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions
| are met:
| 1. Redistributions of source code must retain the above copyright
|    notice, this list of conditions and the following disclaimer.
| 2. Redistributions in binary form must reproduce the above copyright
|    notice, this list of conditions and the following disclaimer in the
|    documentation and/or other materials provided with the distribution.
| 3. All advertising materials mentioning features or use of this software
|    must display the following acknowledgement:
|        This product includes software developed by the NetBSD
|        Foundation, Inc. and its contributors.
| 4. Neither the name of The NetBSD Foundation nor the names of its
|    contributors may be used to endorse or promote products derived
|    from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
| ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
| TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
| PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
| BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
| CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
| SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
| INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
| CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
| ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
| POSSIBILITY OF SUCH DAMAGE.

#include <machine/asm.h>

|	SRT0.S - Stand-alone Run-Time startup code, part 0
	.file	"SRT0.S"
	.data

| Flush the CPU cache using MC68020 values just to be safe.
| This will cause the MC68030 to run with the data cache
| disabled, but that is OK for boot programs.
	.set	IC_CLEAR,0x9
	.set	PSL_HIGHIPL,0x2700

	.text

ASENTRY_NOPROFILE(start)
| Disable interrupts (just in case...)
	movw	#PSL_HIGHIPL,%sr

| Check to see if the code is located correctly.
| Get current location via PC-relative load, then...
	lea	%pc@(start:w),%a0	| current location (0x4000)
| ...force a long (not PC-relative) load to a1 and compare.
	lea	start:l,%a1		| desired location (LINKADDR)
	cmpl	%a0,%a1
	beqs	restart

| Relocate the code and data to where they belong.
	movl	#_edata,%d0		| Desired end of program
	subl	%a1,%d0			| Calculate length, round up.
	lsrl	#2,%d0
Lcp:
	movl	%a0@+,%a1@+
	dbra	%d0,Lcp

| If we are on a sun2, we don't want to clear the I-cache
| because we don't have one.  We are on a sun2 if the PROM
| has pointed the vector base register to zero.  This is 
| similar to the test that SRT1.c's _start does.
	movc	%vbr, %d0
	tstl	%d0
	beqs	Ljmpreloc
| Clear the I-cache in case the copied code was cached.
	movl	#IC_CLEAR,%d0
	movc	%d0,%cacr
Ljmpreloc:
| Force a long jump to the relocated code (not pc-relative)
	lea	restart:l,%a0
	jmp	%a0@

| Define the location of our stack (just before relocated text).
| Leave room the exit jmpbuf at the end of our stack.
	.set	estack,start-60

restart:
| Now in the relocated code, using the monitor stack.
| Save this context so we can return with it.
	pea	estack
	jsr	_C_LABEL(setjmp)
	addqw	#4,%sp
	tstl	%d0
	bne	Ldone	| here via longjmp

| Switch to our own stack.
	lea	estack,%a0
	movl	%a0,%sp
	subl	%a6,%a6

| Clear out BSS...
	lea	_edata,%a0
	lea	_end,%a1
Lclrbss:
	clrl	%a0@+
	cmpl	%a1,%a0
	ble	Lclrbss

| Call the run-time startup C code, which will:
|   initialize, call main, call exit.
	jsr	_C_LABEL(_start)

| Switch back to the monitor stack, then either
| "chain" to the next program or return.
ENTRY(exit)
	pea	estack
	jsr	_C_LABEL(longjmp)	| to next line
Ldone:
	movl	_C_LABEL(chain_to_func),%a0
	movl	%a0,%d0
	beq	Lret
	jmp	%a0@
Lret:
	rts

| function to clear the I-cache
ENTRY(ICIA)
	tstl	_C_LABEL(_is2)
	bne	Lret
	movl	#IC_CLEAR,%d0
	movc	%d0,%cacr
	rts

| function to get the vector base register
ENTRY(getvbr)
	movc	%vbr,%a0
	rts

| Kernel version of setjmp/longjmp (label_t is 16 words)

ENTRY(setjmp)
	movl	%sp@(4),%a0	| savearea pointer
	moveml	#0xFCFC,%a0@	| save d2-d7/a2-a7
	movl	%sp@,%a0@(48)	| and return address
	movl	#0,%d0		| return 0
	rts

ENTRY(longjmp)
	movl	%sp@(4),%a0	| savearea pointer
	moveml	%a0@+,#0xFCFC	| restore d2-d7/a2-a7
	| Note: just changed sp!
	movl	%a0@,%sp@	| and return address
	movl	#1,%d0		| return 1
	rts

| The end.
