/*	$NetBSD: netbsd32_select.c,v 1.4 2003/01/18 08:28:26 thorpej Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_select.c,v 1.4 2003/01/18 08:28:26 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>

#include <sys/proc.h>

#include <net/if.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

int
netbsd32_select(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_select_args /* {
		syscallarg(int) nd;
		syscallarg(netbsd32_fd_setp_t) in;
		syscallarg(netbsd32_fd_setp_t) ou;
		syscallarg(netbsd32_fd_setp_t) ex;
		syscallarg(netbsd32_timevalp_t) tv;
	} */ *uap = v;
/* This one must be done in-line 'cause of the timeval */
	struct proc *p = l->l_proc;
	struct netbsd32_timeval tv32;
	caddr_t bits;
	char smallbits[howmany(FD_SETSIZE, NFDBITS) * sizeof(fd_mask) * 6];
	struct timeval atv;
	int s, ncoll, error = 0, timo;
	size_t ni;
	extern int	selwait, nselcoll;
	extern int selscan __P((struct proc *, fd_mask *, fd_mask *, int, register_t *));

	if (SCARG(uap, nd) < 0)
		return (EINVAL);
	if (SCARG(uap, nd) > p->p_fd->fd_nfiles) {
		/* forgiving; slightly wrong */
		SCARG(uap, nd) = p->p_fd->fd_nfiles;
	}
	ni = howmany(SCARG(uap, nd), NFDBITS) * sizeof(fd_mask);
	if (ni * 6 > sizeof(smallbits))
		bits = malloc(ni * 6, M_TEMP, M_WAITOK);
	else
		bits = smallbits;

#define	getbits(name, x) \
	if (SCARG(uap, name)) { \
		error = copyin((caddr_t)NETBSD32PTR64(SCARG(uap, name)), \
		    bits + ni * x, ni); \
		if (error) \
			goto done; \
	} else \
		memset(bits + ni * x, 0, ni);
	getbits(in, 0);
	getbits(ou, 1);
	getbits(ex, 2);
#undef	getbits

	if (SCARG(uap, tv)) {
		error = copyin((caddr_t)NETBSD32PTR64(SCARG(uap, tv)),
		    (caddr_t)&tv32, sizeof(tv32));
		if (error)
			goto done;
		netbsd32_to_timeval(&tv32, &atv);
		if (itimerfix(&atv)) {
			error = EINVAL;
			goto done;
		}
		s = splclock();
		timeradd(&atv, &time, &atv);
		splx(s);
	} else
		timo = 0;
retry:
	ncoll = nselcoll;
	l->l_flag |= L_SELECT;
	error = selscan(p, (fd_mask *)(bits + ni * 0),
			   (fd_mask *)(bits + ni * 3), SCARG(uap, nd), retval);
	if (error || *retval)
		goto done;
	if (SCARG(uap, tv)) {
		/*
		 * We have to recalculate the timeout on every retry.
		 */
		timo = hzto(&atv);
		if (timo <= 0)
			goto done;
	}
	s = splhigh();
	if ((l->l_flag & L_SELECT) == 0 || nselcoll != ncoll) {
		splx(s);
		goto retry;
	}
	l->l_flag &= ~L_SELECT;
	error = tsleep((caddr_t)&selwait, PSOCK | PCATCH, "select", timo);
	splx(s);
	if (error == 0)
		goto retry;
done:
	l->l_flag &= ~L_SELECT;
	/* select is not restarted after signals... */
	if (error == ERESTART)
		error = EINTR;
	if (error == EWOULDBLOCK)
		error = 0;
	if (error == 0) {
#define	putbits(name, x) \
		if (SCARG(uap, name)) { \
			error = copyout(bits + ni * x, \
			    (caddr_t)NETBSD32PTR64(SCARG(uap, name)), ni); \
			if (error) \
				goto out; \
		}
		putbits(in, 3);
		putbits(ou, 4);
		putbits(ex, 5);
#undef putbits
	}
out:
	if (ni * 6 > sizeof(smallbits))
		free(bits, M_TEMP);
	return (error);
}
