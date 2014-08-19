/*	$NetBSD: linux_ipccall.c,v 1.32.22.1 2014/08/20 00:03:32 tls Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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
__KERNEL_RCSID(0, "$NetBSD: linux_ipccall.c,v 1.32.22.1 2014/08/20 00:03:32 tls Exp $");

#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#endif

#include <sys/param.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/proc.h>
#include <sys/systm.h>

/* real syscalls */
#include <sys/mount.h>
#include <sys/syscallargs.h>

/* sys_ipc + args prototype */
#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>

#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_syscall.h>

/* general ipc defines */
#include <compat/linux/common/linux_ipc.h>

/* prototypes for real/normal linux-emul syscalls */
#include <compat/linux/common/linux_msg.h>
#include <compat/linux/common/linux_shm.h>
#include <compat/linux/common/linux_sem.h>

/* prototypes for sys_ipc stuff */
#include <compat/linux/common/linux_ipccall.h>

/* Used on: arm, i386, m68k, mips, ppc, sparc, sparc64 */
/* Not used on: alpha */

/*
 * Stuff to deal with the SysV ipc/shm/semaphore interface in Linux.
 * The main difference is, that Linux handles it all via one
 * system call, which has the usual maximum amount of 5 arguments.
 * This results in a kludge for calls that take 6 of them.
 *
 * The SYSV??? options have to be enabled to get the appropriate
 * functions to work.
 */

int
linux_sys_ipc(struct lwp *l, const struct linux_sys_ipc_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(void *) ptr;
	} */

	switch (SCARG(uap, what)) {
#ifdef SYSVSEM
	case LINUX_SYS_SEMOP:
		return linux_semop(l, uap, retval);
	case LINUX_SYS_SEMGET:
		return linux_semget(l, uap, retval);
	case LINUX_SYS_SEMCTL: {
		struct linux_sys_semctl_args bsa;
		union linux_semun arg;
		int error;

		SCARG(&bsa, semid) = SCARG(uap, a1);
		SCARG(&bsa, semnum) = SCARG(uap, a2);
		SCARG(&bsa, cmd) = SCARG(uap, a3);
		/* Convert from (union linux_semun *) to (union linux_semun) */
		if ((error = copyin(SCARG(uap, ptr), &arg, sizeof arg)))
			return error;
		SCARG(&bsa, arg) = arg;

		return linux_sys_semctl(l, &bsa, retval);
	    }
#endif
#ifdef SYSVMSG
	case LINUX_SYS_MSGSND:
		return linux_msgsnd(l, uap, retval);
	case LINUX_SYS_MSGRCV:
		return linux_msgrcv(l, uap, retval);
	case LINUX_SYS_MSGGET:
		return linux_msgget(l, uap, retval);
	case LINUX_SYS_MSGCTL: {
		struct linux_sys_msgctl_args bsa;

		SCARG(&bsa, msqid) = SCARG(uap, a1);
		SCARG(&bsa, cmd) = SCARG(uap, a2);
		SCARG(&bsa, buf) = (struct linux_msqid_ds *)SCARG(uap, ptr);

		return linux_sys_msgctl(l, &bsa, retval);
	    }
#endif
#ifdef SYSVSHM
	case LINUX_SYS_SHMAT: {
		struct linux_sys_shmat_args bsa;

		SCARG(&bsa, shmid) = SCARG(uap, a1);
		SCARG(&bsa, shmaddr) = (void *)SCARG(uap, ptr);
		SCARG(&bsa, shmflg) = SCARG(uap, a2);
		/* XXX passing pointer inside int here */
		SCARG(&bsa, raddr) = (u_long *)SCARG(uap, a3);

		return linux_sys_shmat(l, &bsa, retval);
	    }
	case LINUX_SYS_SHMDT:
		return linux_shmdt(l, uap, retval);
	case LINUX_SYS_SHMGET:
		return linux_shmget(l, uap, retval);
	case LINUX_SYS_SHMCTL: {
		struct linux_sys_shmctl_args bsa;

		SCARG(&bsa, shmid) = SCARG(uap, a1);
		SCARG(&bsa, cmd) = SCARG(uap, a2);
		SCARG(&bsa, buf) = (struct linux_shmid_ds *)SCARG(uap, ptr);

		return linux_sys_shmctl(l, &bsa, retval);
	    }
#endif
	default:
		return ENOSYS;
	}
}

#ifdef SYSVSEM
int
linux_semop(struct lwp *l, const struct linux_sys_ipc_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(void *) ptr;
	} */
	struct sys_semop_args bsa;

	SCARG(&bsa, semid) = SCARG(uap, a1);
	SCARG(&bsa, sops) = (struct sembuf *)SCARG(uap, ptr);
	SCARG(&bsa, nsops) = SCARG(uap, a2);

	return sys_semop(l, &bsa, retval);
}

int
linux_semget(struct lwp *l, const struct linux_sys_ipc_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(void *) ptr;
	} */
	struct sys_semget_args bsa;

	SCARG(&bsa, key) = (key_t)SCARG(uap, a1);
	SCARG(&bsa, nsems) = SCARG(uap, a2);
	SCARG(&bsa, semflg) = SCARG(uap, a3);

	return sys_semget(l, &bsa, retval);
}

#endif /* SYSVSEM */

#ifdef SYSVMSG

int
linux_msgsnd(struct lwp *l, const struct linux_sys_ipc_args *uap, register_t *retval)
{
	struct sys_msgsnd_args bma;

	SCARG(&bma, msqid) = SCARG(uap, a1);
	SCARG(&bma, msgp) = SCARG(uap, ptr);
	SCARG(&bma, msgsz) = SCARG(uap, a2);
	SCARG(&bma, msgflg) = SCARG(uap, a3);

	return sys_msgsnd(l, &bma, retval);
}

int
linux_msgrcv(struct lwp *l, const struct linux_sys_ipc_args *uap, register_t *retval)
{
	struct sys_msgrcv_args bma;
	struct linux_msgrcv_msgarg kluge;
	int error;

	if ((error = copyin(SCARG(uap, ptr), &kluge, sizeof kluge)))
		return error;

	SCARG(&bma, msqid) = SCARG(uap, a1);
	SCARG(&bma, msgp) = kluge.msg;
	SCARG(&bma, msgsz) = SCARG(uap, a2);
	SCARG(&bma, msgtyp) = kluge.type;
	SCARG(&bma, msgflg) = SCARG(uap, a3);

	return sys_msgrcv(l, &bma, retval);
}

int
linux_msgget(struct lwp *l, const struct linux_sys_ipc_args *uap, register_t *retval)
{
	struct sys_msgget_args bma;

	SCARG(&bma, key) = (key_t)SCARG(uap, a1);
	SCARG(&bma, msgflg) = SCARG(uap, a2);

	return sys_msgget(l, &bma, retval);
}

#endif /* SYSVMSG */

#ifdef SYSVSHM
/*
 * shmdt(): this could have been mapped directly, if it wasn't for
 * the extra indirection by the linux_ipc system call.
 */
int
linux_shmdt(struct lwp *l, const struct linux_sys_ipc_args *uap, register_t *retval)
{
	struct sys_shmdt_args bsa;

	SCARG(&bsa, shmaddr) = SCARG(uap, ptr);

	return sys_shmdt(l, &bsa, retval);
}

/*
 * Same story as shmdt.
 */
int
linux_shmget(struct lwp *l, const struct linux_sys_ipc_args *uap, register_t *retval)
{
	struct linux_sys_shmget_args bsa;

	SCARG(&bsa, key) = SCARG(uap, a1);
	SCARG(&bsa, size) = SCARG(uap, a2);
	SCARG(&bsa, shmflg) = SCARG(uap, a3);

	return linux_sys_shmget(l, &bsa, retval);
}

#endif /* SYSVSHM */
