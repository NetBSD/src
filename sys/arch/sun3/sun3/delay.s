/*
 * Copyright (c) 1994 Gordon W. Ross
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *	$Id: delay.s,v 1.2 1994/07/19 02:39:18 gwr Exp $
 */

| These two routines depend on the variable "cpuspeed"
| which should be set to the CPU clock rate in MHz.
| XXX - Currently this is set in sun3_startup.c based on the
| CPU model but this should be determined at run time...
	.data
	.even
	.globl	_cpuspeed
_cpuspeed:
	.long	20	| assume 20 MHz (Sun3/60)

	.text
| delay(int usecs)
| Delay for "usec" microseconds.  Minimum delay is about 5 uS.
	.even
.globl	_delay
_delay:	|
	| d0 = (cpuspeed * usecs)
	movel	_cpuspeed,d0
	mulsl	sp@(4),d0
	| subtract some overhead
	moveq	#80,d1
	subl	d1,d0
| This loop takes 8 clocks per cycle.
Ldelay:
	subql	#8,d0
	jgt	Ldelay
	rts

	.text
| delay2us()
| Delay for at least 2.2 uS. (for the ZS driver).
| This actually takes about 2.4 uS.  Just right.
| XXX - Need to make this work for any cpuspeed.
	.even
.globl _delay2us	
_delay2us:
	movel	_cpuspeed,d0
	moveq	#16,d1
	subl	d1,d0	| d0=cpuspeed-16
	jle	Lrts
	subql	#4,d0	| d0=cpuspeed-20
	jle	Lrts
	subql	#5,d0	| d0=cpuspeed-25
	jle	Lrts
	subql	#5,d0	| d0=cpuspeed-30
	nop ; nop
Lrts:	rts
