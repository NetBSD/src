/*	$NetBSD: linux_futex.c,v 1.1 2005/11/04 16:54:11 manu Exp $ */

/*-
 * Copyright (c) 2005 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: linux_futex.c,v 1.1 2005/11/04 16:54:11 manu Exp $");

#include <sys/types.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/queue.h>

#include <compat/linux/common/linux_futex.h>
#include <compat/linux/linux_syscallargs.h>

int
linux_sys_futex(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_futex_args /* {
		syscallarg(int *) uaddr;
		syscallarg(int) op;
		syscallarg(int) val;
		syscallarg(const struct timespec *) timeout;
		syscallarg(int *) uaddr2;
		syscallarg(int) val3;
	} */ *uap = v;
	int val;
	int ret;
	struct timespec timeout = { 0, 0 };
	int error = 0;

	switch (SCARG(uap, op)) {
	case LINUX_FUTEX_WAIT:
		if ((error = copyin(SCARG(uap, uaddr), 
		    &val, sizeof(val))) != 0)
			return error;

		if (val != SCARG(uap, val))
			return EWOULDBLOCK;

		if (SCARG(uap, timeout) != NULL) {
			if ((error = copyin(SCARG(uap, timeout), 
			    &timeout, sizeof(timeout))) != 0)
				return error;
		}

#ifdef DEBUG_LINUX
		printf("FUTEX_WAIT %d.%d: val = %d, uaddr = %p, "
		    "*uaddr = %d, timeout = %d.%09ld\n", 
		    l->l_proc->p_pid, l->l_lid, SCARG(uap, val), 
		    SCARG(uap, uaddr), val, timeout.tv_sec, timeout.tv_nsec); 
#endif
		ret = tsleep(SCARG(uap, uaddr), PCATCH|PZERO, 
		    "linuxfutex", timeout.tv_sec * hz);
#ifdef DEBUG_LINUX
		printf("FUTEX_WAIT %d.%d: uaddr = %p, "
		    "ret = %d\n", l->l_proc->p_pid, l->l_lid, 
		    SCARG(uap, uaddr), ret); 
#endif

		switch (ret) {
		case EWOULDBLOCK:	/* timeout */
			return ETIMEDOUT;
			break;
		case EINTR:		/* signal */
			return EINTR;
			break;
		case 0:			/* FUTEX_WAKE received */
#ifdef DEBUG_LINUX
			printf("FUTEX_WAIT %d.%d: uaddr = %p, got FUTEX_WAKE\n",
			    l->l_proc->p_pid, l->l_lid, SCARG(uap, uaddr)); 
#endif
			return 0;
			break;
		default:
			printf("FUTEX_WAIT: unexpected ret = %d\n", ret);
			break;
		}

		/* NOTREACHED */
		break;
		
	case LINUX_FUTEX_WAKE:
		/* 
		 * XXX We should only wake up SCARG(uap, val) processes 
		 * waiting on the futex, not all of them.
		 * More XXX: Linux is able cope with different addresses 
		 * corresponding to the same mapped memory in the sleeping 
		 * and the waker process.
		 */
#ifdef DEBUG_LINUX
		printf("FUTEX_WAKE %d.%d: uaddr = %p, val = %d\n", 
		    l->l_proc->p_pid, l->l_lid, 
		    SCARG(uap, uaddr), SCARG(uap, val)); 
#endif
		wakeup(SCARG(uap, uaddr));

		*retval = 1; /* XXX should be number of processes */
		break;

	case LINUX_FUTEX_FD:
	case LINUX_FUTEX_REQUEUE:
	case LINUX_FUTEX_CMP_REQUEUE:
		printf("linux_sys_futex: unimplemented op %d\n", 
		    SCARG(uap, op));
		break;
	default:
		printf("linux_sys_futex: unkonwn op %d\n", 
		    SCARG(uap, op));
		break;
	}
	return 0;
}
