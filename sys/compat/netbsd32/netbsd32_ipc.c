/*	$NetBSD: netbsd32_ipc.c,v 1.14 2007/12/20 23:03:01 dsl Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_ipc.c,v 1.14 2007/12/20 23:03:01 dsl Exp $");

#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/mount.h>
#include <sys/dirent.h>

#include <sys/syscallargs.h>
#include <sys/proc.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

#if defined(SYSVSEM)

int
netbsd32___semctl14(struct lwp *l, const struct netbsd32___semctl14_args *uap, register_t *retval)
{
	return do_netbsd32___semctl14(l, uap, retval, NULL);
}

int
do_netbsd32___semctl14(struct lwp *l, const struct netbsd32___semctl14_args *uap, register_t *retval, void *vkarg)
{
	/* {
		syscallarg(int) semid;
		syscallarg(int) semnum;
		syscallarg(int) cmd;
		syscallarg(netbsd32_semunp_t) arg;
	} */
	struct semid_ds sembuf;
	struct netbsd32_semid_ds sembuf32;
	int cmd, error;
	void *pass_arg;
	union __semun karg;
	union netbsd32_semun karg32;

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
			karg32 = *(union netbsd32_semun *)vkarg;
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
			netbsd32_to_semid_ds(&sembuf32, &sembuf);
		}
	}

	error = semctl1(l, SCARG(uap, semid), SCARG(uap, semnum), cmd,
	    pass_arg, retval);

	if (error == 0 && cmd == IPC_STAT) {
		netbsd32_from_semid_ds(&sembuf, &sembuf32);
		error = copyout(&sembuf32, NETBSD32PTR64(karg32.buf),
		    sizeof(sembuf32));
	}

	return (error);
}

int
netbsd32_semget(struct lwp *l, const struct netbsd32_semget_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_key_t) key;
		syscallarg(int) nsems;
		syscallarg(int) semflg;
	} */
	struct sys_semget_args ua;

	NETBSD32TOX_UAP(key, key_t);
	NETBSD32TO64_UAP(nsems);
	NETBSD32TO64_UAP(semflg);
	return (sys_semget(l, &ua, retval));
}

int
netbsd32_semop(struct lwp *l, const struct netbsd32_semop_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) semid;
		syscallarg(netbsd32_sembufp_t) sops;
		syscallarg(netbsd32_size_t) nsops;
	} */
	struct sys_semop_args ua;

	NETBSD32TO64_UAP(semid);
	NETBSD32TOP_UAP(sops, struct sembuf);
	NETBSD32TOX_UAP(nsops, size_t);
	return (sys_semop(l, &ua, retval));
}

int
netbsd32_semconfig(struct lwp *l, const struct netbsd32_semconfig_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) flag;
	} */
	struct sys_semconfig_args ua;

	NETBSD32TO64_UAP(flag);
	return (sys_semconfig(l, &ua, retval));
}
#endif /* SYSVSEM */

#if defined(SYSVMSG)

int
netbsd32___msgctl13(struct lwp *l, const struct netbsd32___msgctl13_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) msqid;
		syscallarg(int) cmd;
		syscallarg(netbsd32_msqid_dsp_t) buf;
	} */
	struct msqid_ds ds;
	struct netbsd32_msqid_ds ds32;
	int error, cmd;

	cmd = SCARG(uap, cmd);
	if (cmd == IPC_SET) {
		error = copyin(SCARG_P32(uap, buf), &ds32, sizeof(ds32));
		if (error)
			return error;
		netbsd32_to_msqid_ds(&ds32, &ds);
	}

	error = msgctl1(l, SCARG(uap, msqid), cmd,
	    (cmd == IPC_SET || cmd == IPC_STAT) ? &ds : NULL);

	if (error == 0 && cmd == IPC_STAT) {
		netbsd32_from_msqid_ds(&ds, &ds32);
		error = copyout(&ds32, SCARG_P32(uap, buf), sizeof(ds32));
	}

	return error;
}

int
netbsd32_msgget(struct lwp *l, const struct netbsd32_msgget_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_key_t) key;
		syscallarg(int) msgflg;
	} */
	struct sys_msgget_args ua;

	NETBSD32TOX_UAP(key, key_t);
	NETBSD32TO64_UAP(msgflg);
	return sys_msgget(l, &ua, retval);
}

static int
netbsd32_msgsnd_fetch_type(const void *src, void *dst, size_t size)
{
	netbsd32_long l32;
	long *l = dst;
	int error;

	KASSERT(size == sizeof(netbsd32_long));

	error = copyin(src, &l32, sizeof(l32));
	if (!error)
		*l = l32;
	return error;
}

int
netbsd32_msgsnd(struct lwp *l, const struct netbsd32_msgsnd_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) msqid;
		syscallarg(const netbsd32_voidp) msgp;
		syscallarg(netbsd32_size_t) msgsz;
		syscallarg(int) msgflg;
	} */

	return msgsnd1(l, SCARG(uap, msqid),
	    SCARG_P32(uap, msgp), SCARG(uap, msgsz),
	    SCARG(uap, msgflg), sizeof(netbsd32_long),
	    netbsd32_msgsnd_fetch_type);
}

static int
netbsd32_msgrcv_put_type(const void *src, void *dst, size_t size)
{
	netbsd32_long l32;
	const long *l = src;

	KASSERT(size == sizeof(netbsd32_long));

	l32 = (netbsd32_long)(*l);
	return copyout(&l32, dst, sizeof(l32));
}

int
netbsd32_msgrcv(struct lwp *l, const struct netbsd32_msgrcv_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) msqid;
		syscallarg(netbsd32_voidp) msgp;
		syscallarg(netbsd32_size_t) msgsz;
		syscallarg(netbsd32_long) msgtyp;
		syscallarg(int) msgflg;
	} */

	return msgrcv1(l, SCARG(uap, msqid),
	    SCARG_P32(uap, msgp), SCARG(uap, msgsz),
	    SCARG(uap, msgtyp), SCARG(uap, msgflg), sizeof(netbsd32_long),
	    netbsd32_msgrcv_put_type, retval);
}
#endif /* SYSVMSG */

#if defined(SYSVSHM)

int
netbsd32_shmat(struct lwp *l, const struct netbsd32_shmat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) shmid;
		syscallarg(const netbsd32_voidp) shmaddr;
		syscallarg(int) shmflg;
	} */
	struct sys_shmat_args ua;

	NETBSD32TO64_UAP(shmid);
	NETBSD32TOP_UAP(shmaddr, void);
	NETBSD32TO64_UAP(shmflg);
	return sys_shmat(l, &ua, retval);
}

int
netbsd32___shmctl13(struct lwp *l, const struct netbsd32___shmctl13_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) shmid;
		syscallarg(int) cmd;
		syscallarg(netbsd32_shmid_dsp_t) buf;
	} */
	struct shmid_ds ds;
	struct netbsd32_shmid_ds ds32;
	int error, cmd;

	cmd = SCARG(uap, cmd);
	if (cmd == IPC_SET) {
		error = copyin(SCARG_P32(uap, buf), &ds32, sizeof(ds32));
		if (error)
			return error;
		netbsd32_to_shmid_ds(&ds32, &ds);
	}

	error = shmctl1(l, SCARG(uap, shmid), cmd,
	    (cmd == IPC_SET || cmd == IPC_STAT) ? &ds : NULL);

	if (error == 0 && cmd == IPC_STAT) {
		netbsd32_from_shmid_ds(&ds, &ds32);
		error = copyout(&ds32, SCARG_P32(uap, buf), sizeof(ds32));
	}

	return error;
}

int
netbsd32_shmdt(struct lwp *l, const struct netbsd32_shmdt_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_voidp) shmaddr;
	} */
	struct sys_shmdt_args ua;

	NETBSD32TOP_UAP(shmaddr, const char);
	return (sys_shmdt(l, &ua, retval));
}

int
netbsd32_shmget(struct lwp *l, const struct netbsd32_shmget_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_key_t) key;
		syscallarg(netbsd32_size_t) size;
		syscallarg(int) shmflg;
	} */
	struct sys_shmget_args ua;

	NETBSD32TOX_UAP(key, key_t)
	NETBSD32TOX_UAP(size, size_t)
	NETBSD32TO64_UAP(shmflg);
	return (sys_shmget(l, &ua, retval));
}
#endif /* SYSVSHM */
