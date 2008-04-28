/*	$NetBSD: test_callout1.c,v 1.3 2008/04/28 20:23:06 martin Exp $	*/

/*-
 * Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: test_callout1.c,v 1.3 2008/04/28 20:23:06 martin Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/kernel.h>
#include <sys/callout.h>

int	testcall(struct lwp *, void *, register_t *);

void	test_callout(void *);
void	test_softint(void *);

kmutex_t	test_mutex;
kcondvar_t	test_cv;
int		test_done;
void		*test_sih;
callout_t	test_ch;

void
test_callout(void *cookie)
{
	int s;
	
	/* Trigger soft interrupt. */
	s = splhigh();
	softint_schedule(test_sih);
	splx(s);

	mutex_enter(&test_mutex);
	test_done = 1;
	cv_broadcast(&test_cv);
	mutex_exit(&test_mutex);
}

void
test_softint(void *cookie)
{

	printf("l_ncsw = %d\n", (int)curlwp->l_ncsw);
	callout_halt(&test_ch, NULL);
	printf("l_ncsw = %d\n", (int)curlwp->l_ncsw);
}

int
testcall(struct lwp *l, void *uap, register_t *retval)
{

	printf("test: initializing\n");

	mutex_init(&test_mutex, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&test_cv, "testcv");
	test_sih = softint_establish(SOFTINT_MPSAFE | SOFTINT_SERIAL,
	    test_softint, NULL);
	callout_init(&test_ch, CALLOUT_MPSAFE);
	callout_setfunc(&test_ch, test_callout, NULL);

	printf("test: firing\n");
	callout_schedule(&test_ch, hz / 10);

	printf("test: waiting\n");
	mutex_enter(&test_mutex);
	while (!test_done) {
		cv_wait(&test_cv, &test_mutex);
	}
	mutex_exit(&test_mutex);

	printf("test: finished\n");

	callout_destroy(&test_ch);
	softint_disestablish(test_sih);
	mutex_destroy(&test_mutex);
	cv_destroy(&test_cv);

	return 0;
}
