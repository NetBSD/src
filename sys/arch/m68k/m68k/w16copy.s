/*	$NetBSD: w16copy.s,v 1.1 2001/06/14 15:38:16 fredette Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fredette.
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

/*
 * NOTICE: This is not a standalone file.  To use it, #include it in
 * your port's locore.s, like so:
 *
 *	#include <m68k/m68k/w16copy.s>
 */

/*
 * Function to zero a region of memory using only 
 * properly aligned 8- and 16-bit accesses.
 */
ENTRY(w16zero)
	movl	%sp@(8), %d0		| get our count
	beq	6f			| return if count is zero
	movl	%sp@(4), %a0		| get our address

| align %a0 to a 16-bit boundary:
	movl	%a0, %d1
	btst	#0, %d1			| is %a0 even?
	beq	1f			| if so, skip ahead
	clrb	%a0@+			| zero one byte
	subql	#1, %d0			| decrement count

| now zero 32 bits at a time:
1:	movl	%d0, %d1		| copy count into %d1
	lsrl	#2, %d1			| turn %d1 into long count
	subqw	#1, %d1			| set up for dbf
	bcs	3f			| skip ahead if long count % 64k == 0
#ifdef	__mc68010__
| on a 68010, 32-bit accesses become 16-bit accesses externally:
2:	clrl	%a0@+
	dbf	%d1, 2b			| will use the 68010 loop mode
#else	/* !__mc68010__ */
2:	clrw	%a0@+
	clrw	%a0@+
	dbf	%d1, 2b
#endif	/* !__mc68010__ */
| since DBcc only uses the low 16-bits of the count
| register, we have to decrement the high 16-bits:
3:	subil	#0x10000, %d1
	bcc	2b

| zero the odd bytes:
4:	andil	#3, %d0			| odd byte count in %d0
	subqw	#1, %d0			| set up for dbf
	bcs	6f			| skip ahead if no odd bytes
5:	clrb	%a0@+
	dbf	%d0, 5b			| will use the 68010 loop mode

6:	rts

/*
 * Function to copy a region of memory to a nonoverlapping
 * region of memory using only properly aligned 8- and 
 * 16-bit accesses.
 */
ENTRY(w16copy)
	movl	%sp@(12), %d0		| get our count
	beq	8f			| return if count is zero
	movl	%sp@(4), %a0		| get our source address
	movl	%sp@(8), %a1		| get our destination address

| align %a0 to a 16-bit boundary:
	movl	%a0, %d1
	btst	#0, %d1			| is %a0 even?
	beq	1f			| if so, skip ahead
	movb	%a0@+, %a1@+		| copy one byte
	subql	#1, %d0			| decrement count

| branch on whether or not %a1 is aligned to a 16-bit boundary:
1:	movl	%a1, %d1
	btst	#0, %d1			| is %a1 even?
	bne	4f			| if not, skip ahead

| %a1 is also aligned to a 16-bit boundary, so we can copy 
| the easy way, 32 bits at a time:
	movl	%d0, %d1		| copy count into %d1
	lsrl	#2, %d1			| turn %d1 into long count
	subqw	#1, %d1			| set up for dbf
	bcs	3f			| skip ahead if long count % 64k == 0
#ifdef	__mc68010__
| on a 68010, 32-bit accesses become 16-bit accesses externally:
2:	movl	%a0@+, %a1@+
	dbf	%d1, 2b			| will use the 68010 loop mode
#else	/* !__mc68010__ */
2:	movw	%a0@+, %a1@+
	movw	%a0@+, %a1@+
	dbf	%d1, 2b
#endif	/* !__mc68010__ */
| since DBcc only uses the low 16-bits of the count
| register, we have to decrement the high 16-bits:
3:	subil	#0x10000, %d1
	bcc	2b
	bra	6f			| jump ahead to copy the odd bytes

| %a1 is misaligned, so we're forced to copy the hard way.
| if there are fewer than four bytes, copy only odd bytes:
4:	cmpil	#4, %d0			| is count less than 4?
	bcs	6f			| if so, skip ahead
| prime the FIFO:
	movw	%a0@+, %d1		| FIFO: xx01
	rorl	#8, %d1			| FIFO: 1xx0
	movb	%d1, %a1@+		| FIFO: 1xxx
	subql	#4, %d0			| subtract 4 from count
| run the FIFO:
5:	rorl	#8, %d1			| FIFO: x1xx
	movw	%a0@+, %d1		| FIFO: x123
	rorl	#8, %d1			| FIFO: 3x12
	movw	%d1, %a1@+		| FIFO: 3xxx -> 1xxx
	subql	#2, %d0			| two or more bytes remaining?
	bcc	5b			| loop back, if so.
| flush the FIFO:
	roll	#8, %d1			| FIFO: xxx1
	movb	%d1, %a1@+		| FIFO: xxxx
	addl	#2, %d0			| fix up count

| copy the odd bytes:
6:	andil	#3, %d0			| odd byte count in %d0
	subqw	#1, %d0			| set up for dbf
	bcs	8f			| skip ahead if no odd bytes
7:	movb	%a0@+, %a1@+
	dbf	%d0, 7b			| use loop mode

8:	rts
