/*	$NetBSD: subr_once.c,v 1.7 2019/03/19 08:16:51 ryo Exp $	*/

/*-
 * Copyright (c)2005 YAMAMOTO Takashi,
 * Copyright (c)2008 Antti Kantee,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_once.c,v 1.7 2019/03/19 08:16:51 ryo Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/once.h>

static kmutex_t oncemtx;
static kcondvar_t oncecv;

void
once_init(void)
{

	mutex_init(&oncemtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&oncecv, "runonce");
}

int
_init_once(once_t *o, int (*fn)(void))
{
	/* Fastpath handled by RUN_ONCE() */

	int error;

	mutex_enter(&oncemtx);
	while (o->o_status == ONCE_RUNNING)
		cv_wait(&oncecv, &oncemtx);

	if (o->o_refcnt++ == 0) {
		o->o_status = ONCE_RUNNING;
		mutex_exit(&oncemtx);
		o->o_error = fn();
		mutex_enter(&oncemtx);
		o->o_status = ONCE_DONE;
		cv_broadcast(&oncecv);
	}
	KASSERT(o->o_refcnt != 0);	/* detect overflow */

	while (o->o_status == ONCE_RUNNING)
		cv_wait(&oncecv, &oncemtx);
	error = o->o_error;
	mutex_exit(&oncemtx);

	return error;
}

void
_fini_once(once_t *o, void (*fn)(void))
{
	mutex_enter(&oncemtx);
	while (o->o_status == ONCE_RUNNING)
		cv_wait(&oncecv, &oncemtx);

	KASSERT(o->o_refcnt != 0);	/* we need to call _init_once() once */
	if (--o->o_refcnt == 0) {
		o->o_status = ONCE_RUNNING;
		mutex_exit(&oncemtx);
		fn();
		mutex_enter(&oncemtx);
		o->o_status = ONCE_VIRGIN;
		cv_broadcast(&oncecv);
	}

	mutex_exit(&oncemtx);
}
