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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *
 *	$Id: microtime.s,v 1.8 1994/05/04 02:07:24 mycroft Exp $
 */

#include <machine/asm.h>
#include <i386/isa/isareg.h>
#include <i386/isa/timerreg.h>
#include <i386/isa/icu.h>

/*
 * Use a higher resolution version of microtime if HZ is not
 * overridden (i.e. it is 100Hz).
 */
#ifndef HZ
ENTRY(microtime)
	pushl	%edi
	pushl	%esi
	pushl	%ebx

	movl	$_time,%ebx		# get pointer to time

	cli				# disable interrupts

	movl	(%ebx),%edi		# sec = time.tv_sec
	movl	4(%ebx),%esi		# usec = time.tv_usec

	movl	$(TIMER_SEL0|TIMER_LATCH),%eax
	outb	%al,$TIMER_MODE		# latch timer 0's counter

	# Read counter value into ebx, LSB first
	xorl	%ebx,%ebx
	inb	$TIMER_CNTR0,%al
	movb	%al,%bl
	inb	$TIMER_CNTR0,%al
	movb	%al,%bh

	# Now check for counter overflow.  This is tricky because the
	# timer chip doesn't let us atomically read the current counter
	# value and the output state (i.e., overflow state).  We have
	# to read the ICU interrupt request register (IRR) to see if the
	# overflow has occured.  Because we lack atomicity, we use
	# the (very accurate) heuristic that we do not check for
	# overflow if the value read is close to 0.
	# E.g., if we just checked the IRR, we might read a non-overflowing
	# value close to 0, experience overflow, then read this overflow
	# from the IRR, and mistakenly add a correction to the "close
	# to zero" value.
	#
	# We compare the counter value to heuristic constant 12.
	# If the counter value is less than this, we assume the counter
	# didn't overflow between disabling interrupts above and latching
	# the counter value.  For example, we assume that the above 10 or so
	# instructions take less than 12 microseconds to execute.
	#
	# We used to check for overflow only if the value read was close to
	# the timer limit, but this doesn't work very well if we're at the
	# clock's ipl or higher.
	#
	# Otherwise, the counter might have overflowed.  We check for this
	# condition by reading the interrupt request register out of the ICU.
	# If it overflowed, we add in one clock period.

	movl	$11932,%edx	# subtract counter value from limit since
	subl	%ebx,%edx	#   it counts down

	cmpl	$12,%ebx	# check for potential overflow
	jbe	1f
	
	inb	$IO_ICU1,%al	# read IRR in ICU
	orb	_ipending,%al	# and soft intr reg
	testb	$IRQ0,%al	# is a timer interrupt pending?
	jz	1f
	addl	$11932,%edx	# add another tick
	
1:	sti			# enable interrupts

	movl	%edx,%eax	# movl %edx,%eax; imull $1000,%eax,%eax
	sall	$10,%eax
	sall	$3,%edx
	subl	%edx,%eax
	sall	$1,%edx
	subl	%edx,%eax

	xorl	%edx,%edx	# zero extend eax for div
	movl	$1193,%ecx
	idivl	%ecx		# convert to usecs: mult by 1000/1193

	addl	%eax,%esi	# add counter usecs to time.tv_usec
	cmpl	$1000000,%esi	# carry in timeval?
	jl	3f
	subl	$1000000,%esi	# adjust usec
	incl	%edi		# bump sec
	
3:	movl	16(%esp),%ecx	# load timeval pointer arg
	movl	%edi,(%ecx)	# tvp->tv_sec = sec
	movl	%esi,4(%ecx)	# tvp->tv_usec = usec

	popl	%ebx		# restore regs
	popl	%esi
	popl	%edi
	ret
#endif
