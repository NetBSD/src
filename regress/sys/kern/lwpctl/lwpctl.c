/*	$NetBSD: lwpctl.c,v 1.2 2008/04/28 20:23:07 martin Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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

#include <sys/types.h>
#include <sys/lwpctl.h>

#include <lwp.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>

int
main(void)
{
	lwpctl_t *lc;
	struct timespec ts;
	int ctr1, ctr2;

	if (_lwp_ctl(LWPCTL_FEATURE_PCTR, &lc) != 0)
		err(1, "_lwp_ctl");

	/* Ensure that preemption is reported. */
	ctr1 = lc->lc_pctr;
	printf("pctr = %d\n", ctr1);
	ts.tv_nsec = 10*1000000;
	ts.tv_sec = 0;
	nanosleep(&ts, 0);
	ctr2 = lc->lc_pctr;
	printf("pctr = %d\n", ctr2);

	if (ctr1 == ctr2)
		errx(1, "counters match");

	return 0;
}
