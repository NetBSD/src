/*	$NetBSD: except.c,v 1.5 2002/02/21 07:38:16 itojun Exp $	*/

/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <ieeefp.h>
#include <float.h>

void sigfpe();
volatile sig_atomic_t signal_caught;

static volatile const double one  = 1.0;
static volatile const double zero = 0.0;
static volatile const double huge = DBL_MAX;
static volatile const double tiny = DBL_MIN;

main()
{
	volatile double x;

	/*
	 * check to make sure that all exceptions are masked and 
	 * that the accumulated exception status is clear.
 	 */
	assert(fpgetmask() == 0);
	assert(fpgetsticky() == 0);

	/* set up signal handler */
	signal (SIGFPE, sigfpe);
	signal_caught = 0;

	/* trip divide by zero */
	x = one / zero;
	assert (fpgetsticky() & FP_X_DZ);
	assert (signal_caught == 0);
	fpsetsticky(0);

	/* trip invalid operation */
	x = zero / zero;
	assert (fpgetsticky() & FP_X_INV);
	assert (signal_caught == 0);
	fpsetsticky(0);

	/* trip overflow */
	x = huge * huge;
	assert (fpgetsticky() & FP_X_OFL);
	assert (signal_caught == 0);
	fpsetsticky(0);

	/* trip underflow */
	x = tiny * tiny;
	assert (fpgetsticky() & FP_X_UFL);
	assert (signal_caught == 0);
	fpsetsticky(0);

#if 1
	/* unmask and then trip divide by zero */
	fpsetmask(FP_X_DZ);
	x = one / zero;
	assert (signal_caught == 1);
	signal_caught = 0;

	/* unmask and then trip invalid operation */
	fpsetmask(FP_X_INV);
	x = zero / zero;
	assert (signal_caught == 1);
	signal_caught = 0;

	/* unmask and then trip overflow */
	fpsetmask(FP_X_OFL);
	x = huge * huge;
	assert (signal_caught == 1);
	signal_caught = 0;

	/* unmask and then trip underflow */
	fpsetmask(FP_X_UFL);
	x = tiny * tiny;
	assert (signal_caught == 1);
	signal_caught = 0;
#endif

	exit(0);
}

void
sigfpe()
{
	signal_caught = 1;
}
