/*	$NetBSD: irix_usync.c,v 1.2 2002/05/22 21:32:21 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: irix_usync.c,v 1.2 2002/05/22 21:32:21 manu Exp $");

#include <sys/types.h>
#include <sys/signal.h> 
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/syscallargs.h>

#include <compat/irix/irix_types.h>
#include <compat/irix/irix_usema.h>
#include <compat/irix/irix_usync.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_syscallargs.h>

static TAILQ_HEAD(irix_usync_reclist, irix_usync_rec) irix_usync_reclist =
	{ NULL, NULL };

int
irix_sys_usync_cntl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_usync_cntl_args /* {
		syscallarg(int) cmd;
		syscallarg(void *) arg;
	} */ *uap = v;
	int error;
	struct irix_usync_arg iua;
	struct irix_semaphore is;
	struct irix_usync_rec *iur;

	/* 
	 * Initialize irix_usync_reclist if it has not 
	 * been initialized yet. Not sure it is the best 
	 * place to do this...
	 */
	if (irix_usync_reclist.tqh_last == NULL)
		TAILQ_INIT(&irix_usync_reclist);

	switch (SCARG(uap, cmd)) {
	case IRIX_USYNC_BLOCK:
		if ((error = copyin(SCARG(uap, arg), &iua, sizeof(iua))) != 0)
			return error;

		iur = malloc(sizeof(*iur), M_DEVBUF, M_WAITOK);
		iur->iur_sem = iua.iua_sem;
		iur->iur_p = p;
		TAILQ_INSERT_TAIL(&irix_usync_reclist, iur, iur_list);

		(void)tsleep(iur, PZERO, "irix_usema", 0);
		break;	

	case IRIX_USYNC_INTR_BLOCK:
		if ((error = copyin(SCARG(uap, arg), &iua, sizeof(iua))) != 0)
			return error;

		iur = malloc(sizeof(*iur), M_DEVBUF, M_WAITOK|M_ZERO);
		iur->iur_sem = iua.iua_sem;
		iur->iur_p = p;
		TAILQ_INSERT_TAIL(&irix_usync_reclist, iur, iur_list);

		(void)tsleep(iur, PZERO|PCATCH, "irix_usema", 0);
		break;	

	case IRIX_USYNC_UNBLOCK_ALL: {
		int found = 0;

		if ((error = copyin(SCARG(uap, arg), &iua, sizeof(iua))) != 0)
			return error;

		TAILQ_FOREACH(iur, &irix_usync_reclist, iur_list) {
			if (iur->iur_sem == iua.iua_sem) {
				found = 1;
				wakeup(iur);
				TAILQ_REMOVE(&irix_usync_reclist, 
				    iur, iur_list);
				free(iur, M_DEVBUF);
			}
		}
		if (found == 0)
			return EINVAL;
		break;
	}

	case IRIX_USYNC_UNBLOCK: 
		if ((error = copyin(SCARG(uap, arg), &iua, sizeof(iua))) != 0)
			return error;

		TAILQ_FOREACH(iur, &irix_usync_reclist, iur_list) {
			if (iur->iur_sem == iua.iua_sem) {
				wakeup(iur);
				TAILQ_REMOVE(&irix_usync_reclist, 
				    iur, iur_list);
				free(iur, M_DEVBUF);
				break;
			}
		}
		if (iur == NULL) /* Nothing was found */
			return EINVAL; 
		break;

	case IRIX_USYNC_GET_STATE:
		if ((error = copyin(SCARG(uap, arg), &iua, sizeof(iua))) != 0)
			return error;
		if ((error = copyin(iua.iua_sem, &is, sizeof(is))) != 0)
			return error;
		if (is.is_val < 0)
			*retval = -1;
		else
			*retval = 0;
		break;
	default:
		printf("Warning: unimplemented IRIX usync_cntl command %d\n",
		    SCARG(uap, cmd));
		return EINVAL;
	}

	return 0;
}
