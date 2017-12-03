/*	$NetBSD: netbsd32_compat_50_sysv.c,v 1.1.18.2 2017/12/03 11:36:55 jdolecek Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_50_sysv.c,v 1.1.18.2 2017/12/03 11:36:55 jdolecek Exp $");

#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

#if defined(COMPAT_50)

#if defined(SYSVSEM)

int
compat_50_netbsd32___semctl14(struct lwp *l, const struct compat_50_netbsd32___semctl14_args *uap, register_t *retval)
{
	return do_netbsd32___semctl14(l, uap, retval, NULL);
}

int
do_netbsd32___semctl14(struct lwp *l, const struct compat_50_netbsd32___semctl14_args *uap, register_t *retval, void *vkarg)
{
	/* {
		syscallarg(int) semid;
		syscallarg(int) semnum;
		syscallarg(int) cmd;
		syscallarg(netbsd32_semun50p_t) arg;
	} */
	struct semid_ds sembuf;
	struct netbsd32_semid_ds50 sembuf32;
	int cmd, error;
	void *pass_arg;
	union __semun karg;
	union netbsd32_semun50 karg32;

	cmd = SCARG(uap, cmd);

	switch (cmd) {
	case IPC_SET:
	case IPC_STAT:
		pass_arg = &sembuf;
		break;

	case GETALL:
	case SETVAL:
	case SETALL:
		pass_arg = &karg;
		break;
	default:
		pass_arg = NULL;
		break;
	}

	if (pass_arg) {
		if (vkarg != NULL)
			karg32 = *(union netbsd32_semun50 *)vkarg;
		else {
			error = copyin(SCARG_P32(uap, arg), &karg32,
					sizeof(karg32));
			if (error)
				return error;
		}
		if (pass_arg == &karg) {
			switch (cmd) {
			case GETALL:
			case SETALL:
				karg.array = NETBSD32PTR64(karg32.array);
				break;
			case SETVAL:
				karg.val = karg32.val;
				break;
			}
		}
		if (cmd == IPC_SET) {
			error = copyin(NETBSD32PTR64(karg32.buf), &sembuf32,
			    sizeof(sembuf32));
			if (error)
				return (error);
			netbsd32_to_semid_ds50(&sembuf32, &sembuf);
		}
	}

	error = semctl1(l, SCARG(uap, semid), SCARG(uap, semnum), cmd,
	    pass_arg, retval);

	if (error == 0 && cmd == IPC_STAT) {
		netbsd32_from_semid_ds50(&sembuf, &sembuf32);
		error = copyout(&sembuf32, NETBSD32PTR64(karg32.buf),
		    sizeof(sembuf32));
	}

	return (error);
}
#endif

#if defined(SYSVMSG)

int
compat_50_netbsd32___msgctl13(struct lwp *l, const struct compat_50_netbsd32___msgctl13_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) msqid;
		syscallarg(int) cmd;
		syscallarg(netbsd32_msqid_ds50p_t) buf;
	} */
	struct msqid_ds ds;
	struct netbsd32_msqid_ds50 ds32;
	int error, cmd;

	cmd = SCARG(uap, cmd);
	if (cmd == IPC_SET) {
		error = copyin(SCARG_P32(uap, buf), &ds32, sizeof(ds32));
		if (error)
			return error;
		netbsd32_to_msqid_ds50(&ds32, &ds);
	}

	error = msgctl1(l, SCARG(uap, msqid), cmd,
	    (cmd == IPC_SET || cmd == IPC_STAT) ? &ds : NULL);

	if (error == 0 && cmd == IPC_STAT) {
		netbsd32_from_msqid_ds50(&ds, &ds32);
		error = copyout(&ds32, SCARG_P32(uap, buf), sizeof(ds32));
	}

	return error;
}
#endif

#if defined(SYSVSHM)

int
compat_50_netbsd32___shmctl13(struct lwp *l, const struct compat_50_netbsd32___shmctl13_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) shmid;
		syscallarg(int) cmd;
		syscallarg(netbsd32_shmid_ds50p_t) buf;
	} */
	struct shmid_ds ds;
	struct netbsd32_shmid_ds50 ds32;
	int error, cmd;

	cmd = SCARG(uap, cmd);
	if (cmd == IPC_SET) {
		error = copyin(SCARG_P32(uap, buf), &ds32, sizeof(ds32));
		if (error)
			return error;
		netbsd32_to_shmid_ds50(&ds32, &ds);
	}

	error = shmctl1(l, SCARG(uap, shmid), cmd,
	    (cmd == IPC_SET || cmd == IPC_STAT) ? &ds : NULL);

	if (error == 0 && cmd == IPC_STAT) {
		netbsd32_from_shmid_ds50(&ds, &ds32);
		error = copyout(&ds32, SCARG_P32(uap, buf), sizeof(ds32));
	}

	return error;
}
#endif

#endif /* COMPAT_50 */
