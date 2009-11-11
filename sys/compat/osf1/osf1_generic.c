/* $NetBSD: osf1_generic.c,v 1.16 2009/11/11 09:48:51 rmind Exp $ */

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 */

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: osf1_generic.c,v 1.16 2009/11/11 09:48:51 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/select.h>
#include <sys/syscallargs.h>
#include <sys/exec.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_syscallargs.h>
#include <compat/osf1/osf1_cvt.h>

/*
 * The structures end up being the same... but we can't be sure that
 * the other word of our iov_len is zero!
 */

#if __GNUC_PREREQ__(3, 0)
__attribute ((noinline))
#endif  
static int
osf1_get_iov(struct osf1_iovec *uiov, unsigned int iovcnt, struct iovec **iovp)
{
	struct iovec *iov = *iovp;
	struct osf1_iovec osf1_iov[UIO_SMALLIOV];
	int i, j, n, error;

	if (iovcnt > IOV_MAX)
		return EINVAL;

	if (iovcnt > UIO_SMALLIOV) {
		iov = malloc(iovcnt * sizeof *iov, M_IOV, M_WAITOK);
		*iovp = iov;
		/* Caller must free - even if we return an error */
	}

	for (i = 0; i < iovcnt; uiov += UIO_SMALLIOV, i += UIO_SMALLIOV) {
		n = iovcnt - i;
		if (n > UIO_SMALLIOV)
			n = UIO_SMALLIOV;
		error = copyin(uiov, osf1_iov, n * sizeof osf1_iov[0]);
		if (error != 0)
			return error;

		for (j = 0; j < n; iov++, j++) {
			iov->iov_base = osf1_iov[j].iov_base;
			iov->iov_len = osf1_iov[j].iov_len;
		}
	}

	return 0;
}

int
osf1_sys_readv(struct lwp *l, const struct osf1_sys_readv_args *uap, register_t *retval)
{
	struct iovec aiov[UIO_SMALLIOV], *niov = aiov;
	int error;

	error = osf1_get_iov(SCARG(uap, iovp), SCARG(uap, iovcnt), &niov);

	if (error == 0) {
		error = do_filereadv(SCARG(uap, fd), niov,
		    SCARG(uap, iovcnt), NULL,
		    FOF_UPDATE_OFFSET | FOF_IOV_SYSSPACE, retval);
	}

	if (niov != aiov)
		free(niov, M_IOV);

	return error;
}

int
osf1_sys_writev(struct lwp *l, const struct osf1_sys_writev_args *uap, register_t *retval)
{
	struct iovec aiov[UIO_SMALLIOV], *niov = aiov;
	int error;

	error = osf1_get_iov(SCARG(uap, iovp), SCARG(uap, iovcnt), &niov);

	if (error == 0) {
		error = do_filewritev(SCARG(uap, fd), niov,
		    SCARG(uap, iovcnt), NULL,
		    FOF_UPDATE_OFFSET | FOF_IOV_SYSSPACE, retval);
	}

	if (niov != aiov)
		free(niov, M_IOV);

	return error;
}

int
osf1_sys_select(struct lwp *l, const struct osf1_sys_select_args *uap,
    register_t *retval)
{
	struct osf1_timeval otv;
	struct timespec ats, *ts = NULL;
	int error;

	if (SCARG(uap, tv)) {
		/* get the OSF/1 timeval argument */
		error = copyin(SCARG(uap, tv), &otv, sizeof otv);
		if (error != 0)
			return error;

		ats.tv_sec = otv.tv_sec;
		ats.tv_nsec = otv.tv_usec * 1000;
		ts = &ats;
	}

	return selcommon(retval, SCARG(uap, nd), SCARG(uap, in),
	    SCARG(uap, ou), SCARG(uap, ex), ts, NULL);
}
