/*	$NetBSD: microtime.S,v 1.3 2003/08/07 16:27:55 agc Exp $	*/

/*-
 * Copyright (c) 1993 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/asm.h>
#include <dev/isa/isareg.h>
#include <dev/ic/i8253reg.h>

#include "assym.h"

/* LINTSTUB: include <sys/time.h> */

#define	IRQ_BIT(irq_num)	(1 << ((irq_num) % 8))
#define	IRQ_BYTE(irq_num)	((irq_num) >> 3)

/* LINTSTUB: Func: void i8254_microtime(struct timeval *tv) */
ENTRY(i8254_microtime)
	# clear registers and do whatever we can up front
	pushl	%edi
	pushl	%ebx
	xorl	%edx,%edx
	movl	$(TIMER_SEL0|TIMER_LATCH),%eax

	cli				# disable interrupts

	# select timer 0 and latch its counter
	outb	%al,$IO_TIMER1+TIMER_MODE
	inb	$IO_ICU1,%al		# as close to timer latch as possible
	movb	%al,%ch			# %ch is current ICU mask

	# Read counter value into [%al %dl], LSB first
	inb	$IO_TIMER1+TIMER_CNTR0,%al
	movb	%al,%dl				# %dl has LSB
	inb	$IO_TIMER1+TIMER_CNTR0,%al	# %al has MSB

	/*
	 * XXX this assumes that irq 0 is slot 0 on the primary CPU.
	 *     Currently always true, and not likely to change, but
	 *     still not a beauty.
	 */
	# save state of IIR in ICU, and of ipending, for later perusal
	movb	CPUVAR(IPENDING) + IRQ_BYTE(0),%cl # %cl is interrupt pending
	
	# save the current value of _time
	movl	_C_LABEL(time),%edi	# get time.tv_sec
	movl	_C_LABEL(time)+4,%ebx	#  and time.tv_usec

	sti				# enable interrupts, we're done

	# At this point we've collected all the state we need to
	# compute the time.  First figure out if we've got a pending
	# interrupt.  If the IRQ0 bit is set in ipending we've taken
	# a clock interrupt without incrementing time, so we bump
	# time.tv_usec by a tick.  Otherwise if the ICU shows a pending
	# interrupt for IRQ0 we (or the caller) may have blocked an interrupt
	# with the cli.  If the counter is not a very small value (3 as
	# a heuristic), i.e. in pre-interrupt state, we add a tick to
	# time.tv_usec

	testb	$IRQ_BIT(0),%cl		# pending interrupt?
	jnz	1f			# yes, increment count

	testb	$IRQ_BIT(0),%ch		# hardware interrupt pending?
	jz	2f			# no, continue
	testb	%al,%al			# MSB zero?
	jnz	1f			# no, add a tick
	cmpb	$3,%dl			# is this small number?
	jbe	2f			# yes, continue
1:	addl	_C_LABEL(isa_timer_tick),%ebx	# add a tick

	# We've corrected for pending interrupts.  Now do a table lookup
	# based on each of the high and low order counter bytes to increment
	# time.tv_usec
2:	movw	_C_LABEL(isa_timer_msb_table)(,%eax,2),%ax
	subw	_C_LABEL(isa_timer_lsb_table)(,%edx,2),%ax
	addl	%eax,%ebx		# add msb increment

	# Normalize the struct timeval.  We know the previous increments
	# will be less than a second, so we'll only need to adjust accordingly
	cmpl	$1000000,%ebx	# carry in timeval?
	jb	3f
	subl	$1000000,%ebx	# adjust usec
	incl	%edi		# bump sec
	
3:	movl	12(%esp),%ecx	# load timeval pointer arg
	movl	%edi,(%ecx)	# tvp->tv_sec = sec
	movl	%ebx,4(%ecx)	# tvp->tv_usec = usec

	popl	%ebx
	popl	%edi
	ret
