/*	$NetBSD: except.c,v 1.6 2004/03/05 16:37:57 drochner Exp $	*/

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
#include <setjmp.h>

void sigfpe(int, siginfo_t *, void *);
volatile sig_atomic_t signal_caught;
volatile int sicode;

static volatile const double one  = 1.0;
static volatile const double zero = 0.0;
static volatile const double huge = DBL_MAX;
static volatile const double tiny = DBL_MIN;

static sigjmp_buf b;

main()
{
	struct sigaction sa;
	fp_except ex1, ex2;
	int r;
	volatile double x;

	/*
	 * check to make sure that all exceptions are masked and 
	 * that the accumulated exception status is clear.
 	 */
	assert(fpgetmask() == 0);
	assert(fpgetsticky() == 0);

	/* set up signal handler */
	sa.sa_sigaction = sigfpe;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGFPE, &sa, 0);
	signal_caught = 0;

	/* trip divide by zero */
	x = one / zero;
	ex1 = fpgetsticky();
	assert (ex1 & FP_X_DZ);
	assert (signal_caught == 0);
	ex2 = fpsetsticky(0);
	assert(fpgetsticky() == 0);
	assert(ex1 == ex2);

	/* trip invalid operation */
	x = zero / zero;
	ex1 = fpgetsticky();
	assert (ex1 & FP_X_INV);
	assert (signal_caught == 0);
	ex2 = fpsetsticky(0);
	assert(fpgetsticky() == 0);
	assert(ex1 == ex2);

	/* trip overflow */
	x = huge * huge;
	ex1 = fpgetsticky();
	assert (ex1 & FP_X_OFL);
	assert (signal_caught == 0);
	ex2 = fpsetsticky(0);
	assert(fpgetsticky() == 0);
	assert(ex1 == ex2);

	/* trip underflow */
	x = tiny * tiny;
	ex1 = fpgetsticky();
	assert (ex1 & FP_X_UFL);
	assert (signal_caught == 0);
	ex2 = fpsetsticky(0);
	assert(fpgetsticky() == 0);
	assert(ex1 == ex2);

#if 1
	/* unmask and then trip divide by zero */
	fpsetmask(FP_X_DZ);
	r = sigsetjmp(b, 1);
	if (!r)
		x = one / zero;
	assert (signal_caught == 1);
	if (sicode != FPE_FLTDIV)
		printf("si_code=%d, should be FPE_FLTDIV\n", sicode);
	signal_caught = 0;

	/* unmask and then trip invalid operation */
	fpsetmask(FP_X_INV);
	r = sigsetjmp(b, 1);
	if (!r)
		x = zero / zero;
	assert (signal_caught == 1);
	if (sicode != FPE_FLTINV)
		printf("si_code=%d, should be FPE_FLTINV\n", sicode);
	signal_caught = 0;

	/* unmask and then trip overflow */
	fpsetmask(FP_X_OFL);
	r = sigsetjmp(b, 1);
	if (!r)
		x = huge * huge;
	assert (signal_caught == 1);
	if (sicode != FPE_FLTOVF)
		printf("si_code=%d, should be FPE_FLTOVF\n", sicode);
	signal_caught = 0;

	/* unmask and then trip underflow */
	fpsetmask(FP_X_UFL);
	r = sigsetjmp(b, 1);
	if (!r)
		x = tiny * tiny;
	assert (signal_caught == 1);
	if (sicode != FPE_FLTUND)
		printf("si_code=%d, should be FPE_FLTUND\n", sicode);
	signal_caught = 0;
#endif

	exit(0);
}

void
sigfpe(int s, siginfo_t *si, void *c)
{
	signal_caught = 1;
	sicode = si->si_code;
	siglongjmp(b, 1);
}
