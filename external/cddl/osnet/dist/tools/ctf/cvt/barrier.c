/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

#ifdef HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

/*
 * Copyright 2002 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * This file implements a barrier, a synchronization primitive designed to allow
 * threads to wait for each other at given points.  Barriers are initialized
 * with a given number of threads, n, using barrier_init().  When a thread calls
 * barrier_wait(), that thread blocks until n - 1 other threads reach the
 * barrier_wait() call using the same barrier_t.  When n threads have reached
 * the barrier, they are all awakened and sent on their way.  One of the threads
 * returns from barrier_wait() with a return code of 1; the remaining threads
 * get a return code of 0.
 */

#include <errno.h>
#include <pthread.h>
#ifdef illumos
#include <synch.h>
#endif
#include <stdio.h>

#include "barrier.h"
#include "ctftools.h"

void
barrier_init(barrier_t *bar, int nthreads)
{
	if ((errno = pthread_mutex_init(&bar->bar_lock, NULL)) != 0)
		terminate("%s: pthread_mutex_init(bar_lock)", __func__);
#ifdef illumos
	if ((errno = sema_init(&bar->bar_sem, 0, USYNC_THREAD, NULL)) != 0)
		terminate("%s: sema_init(bar_sem)", __func__);
#elif defined(HAVE_DISPATCH_SEMAPHORE_CREATE)
	if ((bar->bar_sem = dispatch_semaphore_create(0)) == NULL)
		terminate("%s: dispatch_semaphore_create()\n", __func__);
#else
	if (sem_init(&bar->bar_sem, 0, 0) == -1)
		terminate("%s: sem_init(bar_sem)", __func__);
#endif

	bar->bar_numin = 0;
	bar->bar_nthr = nthreads;
}

int
barrier_wait(barrier_t *bar)
{
#if defined(HAVE_DISPATCH_SEMAPHORE_CREATE)
	long error;
#endif
	if ((errno = pthread_mutex_lock(&bar->bar_lock)) != 0)
		terminate("%s: pthread_mutex_lock(bar_lock)", __func__);

	if (++bar->bar_numin < bar->bar_nthr) {
		if ((errno = pthread_mutex_unlock(&bar->bar_lock)) != 0)
			terminate("%s: pthread_mutex_unlock(bar_lock)",
			    __func__);
#ifdef illumos
		if ((errno = sema_wait(&bar->bar_sem)) != 0)
			terminate("%s: sema_wait(bar_sem)", __func__);
#elif defined(HAVE_DISPATCH_SEMAPHORE_CREATE)
		if ((error = dispatch_semaphore_wait(bar->bar_sem, DISPATCH_TIME_FOREVER)) != 0)
			terminate("%s: dispatch_semaphore_wait(bar_sem) = %ld\n",
			    __func__, error);
#else
		if (sem_wait(&bar->bar_sem) == -1)
			terminate("%s: sem_wait(bar_sem)", __func__);
#endif

		return (0);

	} else {
		int i;

		/* reset for next use */
		bar->bar_numin = 0;
		for (i = 1; i < bar->bar_nthr; i++) {
#ifdef illumos
			if ((errno = sema_post(&bar->bar_sem)) != 0)
				terminate("%s: sema_post(bar_sem)", __func__);
#elif defined(HAVE_DISPATCH_SEMAPHORE_CREATE)
			/* return value doesn't matter */
			dispatch_semaphore_signal(bar->bar_sem);
#else
			if (sem_post(&bar->bar_sem) == -1)
				terminate("%s: sem_post(bar_sem)", __func__);
#endif
		}
		if ((errno = pthread_mutex_unlock(&bar->bar_lock)) != 0)
			terminate("%s: pthread_mutex_unlock(bar_lock)",
			    __func__);

		return (1);
	}
}
