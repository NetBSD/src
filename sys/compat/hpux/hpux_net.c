/*	$NetBSD: hpux_net.c,v 1.38 2007/12/20 23:02:48 dsl Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: hpux_net.c 1.8 93/08/02$
 *
 *	@(#)hpux_net.c	8.2 (Berkeley) 9/9/93
 */

/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: hpux_net.c 1.8 93/08/02$
 *
 *	@(#)hpux_net.c	8.2 (Berkeley) 9/9/93
 */

/*
 * Network related HP-UX compatibility routines
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpux_net.c,v 1.38 2007/12/20 23:02:48 dsl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/uio.h>
#include <sys/ktrace.h>
#include <sys/syscallargs.h>

#include <compat/hpux/hpux.h>
#include <compat/hpux/hpux_syscallargs.h>
#include <compat/hpux/hpux_util.h>

struct hpux_sys_setsockopt_args {
	syscallarg(int) s;
	syscallarg(int) level;
	syscallarg(int) name;
	syscallarg(void *) val;
	syscallarg(int) valsize;
};

struct hpux_sys_getsockopt_args {
	syscallarg(int) s;
	syscallarg(int) level;
	syscallarg(int) name;
	syscallarg(void *) val;
	syscallarg(int *) avalsize;
};

int	hpux_sys_setsockopt(struct lwp *, const struct hpux_sys_setsockopt_args *, register_t *);
int	hpux_sys_getsockopt(struct lwp *, const struct hpux_sys_getsockopt_args *, register_t *);

void	socksetsize(int, struct mbuf *);


#define MINBSDIPCCODE	0x3EE
#define NUMBSDIPC	32

/*
 * HPUX netioctl() to BSD syscall map.
 * Indexed by callno - MINBSDIPCCODE
 */

#define n(fn, ac) {ac, sizeof (register_t) * (ac), 0, (sy_call_t *)fn}
struct sysent hpuxtobsdipc[NUMBSDIPC] = {
	n( compat_30_sys_socket,	3 ), /* 3ee */
	n( sys_listen,			2 ), /* 3ef */
	n( sys_bind,			3 ), /* 3f0 */
	n( compat_43_sys_accept,	3 ), /* 3f1 */
	n( sys_connect,			3 ), /* 3f2 */
	n( compat_43_sys_recv,		4 ), /* 3f3 */
	n( compat_43_sys_send,		4 ), /* 3f4 */
	n( sys_shutdown,		2 ), /* 3f5 */
	n( compat_43_sys_getsockname,	3 ), /* 3f6 */
	n( hpux_sys_setsockopt,		5 ), /* 3f7 */
	n( sys_sendto,			6 ), /* 3f8 */
	n( compat_43_sys_recvfrom,	6 ), /* 3f9 */
	n( compat_43_sys_getpeername,	3 ), /* 3fa */
	n( NULL,			0 ), /* 3fb */
	n( NULL,			0 ), /* 3fc */
	n( NULL,			0 ), /* 3fd */
	n( NULL,			0 ), /* 3fe */
	n( NULL,			0 ), /* 3ff */
	n( NULL,			0 ), /* 400 */
	n( NULL,			0 ), /* 401 */
	n( NULL,			0 ), /* 402 */
	n( NULL,			0 ), /* 403 */
	n( NULL,			0 ), /* 404 */
	n( NULL,			0 ), /* 405 */
	n( NULL,			0 ), /* 406 */
	n( NULL,			0 ), /* 407 */
	n( NULL,			0 ), /* 408 */
	n( NULL,			0 ), /* 409 */
	n( NULL,			0 ), /* 40a */
	n( hpux_sys_getsockopt,		5 ), /* 40b */
	n( NULL,			0 ), /* 40c */
	n( NULL,			0 ), /* 40d */
};
#undef n
/*
 * Single system call entry to BSD style IPC.
 * Gleened from disassembled libbsdipc.a syscall entries.
 */
int
hpux_sys_netioctl(struct lwp *l, const struct hpux_sys_netioctl_args *uap, register_t *retval)
{
	int code;
	int error;
	struct sysent *hp_sysent;
	register_t bsd_ua[6];    /* Max of table at top */

	code = SCARG(uap, call);
	if (code < MINBSDIPCCODE || code >= MINBSDIPCCODE + NUMBSDIPC)
		return EINVAL;
	hp_sysent = hpuxtobsdipc + code;
	if (hp_sysent->sy_call == NULL)
		return EINVAL;
	error = copyin(SCARG(uap, args), &bsd_ua, hp_sysent->sy_argsize);
	if (error != 0) {
		/* ktrsyscall(code + MINBSDIPCCODE, code + MINBSDIPCCODE, NULL, &bsd_ua); */
		return (error);
	}
	ktrsyscall(code, code, hp_sysent, bsd_ua);
	return hp_sysent->sy_call(l, &bsd_ua, retval);
}

void
socksetsize(int size, struct mbuf *m)
{
	int tmp;

	if (size < sizeof(int)) {
		switch(size) {
	    	case 1:
			tmp = (int) *mtod(m, char *);
			break;
	    	case 2:
			tmp = (int) *mtod(m, short *);
			break;
	    	case 3:
		default:	/* XXX uh, what if sizeof(int) > 4? */
			tmp = (((int) *mtod(m, int *)) >> 8) & 0xffffff;
			break;
		}
		*mtod(m, int *) = tmp;
		m->m_len = sizeof(int);
	} else {
		m->m_len = size;
	}
}

/* ARGSUSED */
int
hpux_sys_setsockopt(struct lwp *l, const struct hpux_sys_setsockopt_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct file *fp;
	struct mbuf *m = NULL;
	int tmp, error;
	int name = SCARG(uap, name);

	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, SCARG(uap, s), &fp)))
		return (error);
	if (SCARG(uap, valsize) > MLEN) {
		error = EINVAL;
		goto out;
	}
	if (SCARG(uap, val)) {
		m = m_get(M_WAIT, MT_SOOPTS);
		if ((error = copyin(SCARG(uap, val), mtod(m, void *),
		    (u_int)SCARG(uap, valsize)))) {
			(void) m_free(m);
			goto out;
		}
		if (name == SO_LINGER) {
			tmp = *mtod(m, int *);
			mtod(m, struct linger *)->l_onoff = 1;
			mtod(m, struct linger *)->l_linger = tmp;
			m->m_len = sizeof(struct linger);
		} else
			socksetsize(SCARG(uap, valsize), m);
	} else if (name == ~SO_LINGER) {
		name = SO_LINGER;
		m = m_get(M_WAIT, MT_SOOPTS);
		mtod(m, struct linger *)->l_onoff = 0;
		m->m_len = sizeof(struct linger);
	}

	error = sosetopt((struct socket *)fp->f_data, SCARG(uap, level),
	    name, m);
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

/* ARGSUSED */
int
hpux_sys_setsockopt2(struct lwp *l, const struct hpux_sys_setsockopt2_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct file *fp;
	struct mbuf *m = NULL;
	int error;

	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, SCARG(uap, s), &fp)))
		return (error);
	if (SCARG(uap, valsize) > MLEN) {
		error = EINVAL;
		goto out;
	}
	if (SCARG(uap, val)) {
		m = m_get(M_WAIT, MT_SOOPTS);
		if ((error = copyin(SCARG(uap, val), mtod(m, void *),
		    (u_int)SCARG(uap, valsize)))) {
			(void) m_free(m);
			goto out;
		}
		socksetsize(SCARG(uap, valsize), m);
	}

	error = sosetopt((struct socket *)fp->f_data, SCARG(uap, level),
	    SCARG(uap, name), m);
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

int
hpux_sys_getsockopt(struct lwp *l, const struct hpux_sys_getsockopt_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct file *fp;
	struct mbuf *m = NULL;
	int valsize, error;

	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, SCARG(uap, s), &fp)))
		return (error);
	if (SCARG(uap, val)) {
		if ((error = copyin((void *)SCARG(uap, avalsize),
		    (void *)&valsize, sizeof (valsize))))
			goto out;
	} else
		valsize = 0;
	if ((error = sogetopt((struct socket *)fp->f_data, SCARG(uap, level),
	    SCARG(uap, name), &m)))
		goto bad;
	if (SCARG(uap, val) && valsize && m != NULL) {
		if (SCARG(uap, name) == SO_LINGER) {
			if (mtod(m, struct linger *)->l_onoff)
				*mtod(m, int *) =
				    mtod(m, struct linger *)->l_linger;
			else
				*mtod(m, int *) = 0;
			m->m_len = sizeof(int);
		}
		if (valsize > m->m_len)
			valsize = m->m_len;
		error = copyout(mtod(m, void *), SCARG(uap, val),
		    (u_int)valsize);
		if (error == 0)
			error = copyout((void *)&valsize,
			    (void *)SCARG(uap, avalsize), sizeof (valsize));
	}
 bad:
	if (m != NULL)
		(void) m_free(m);
 out:
	FILE_UNUSE(fp, l);
	return (error);
}
