/*	$NetBSD: freebsd_ipc.c,v 1.17 2014/11/09 18:30:38 maxv Exp $	*/

/*
 * Copyright (c) 1994 Adam Glass and Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Adam Glass and Charles M.
 *	Hannum.
 * 4. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: freebsd_ipc.c,v 1.17 2014/11/09 18:30:38 maxv Exp $");

#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <compat/sys/shm.h>
#include <compat/freebsd/freebsd_syscallargs.h>

#ifdef SYSVSEM
int
freebsd_sys_semsys(struct lwp *l, const struct freebsd_sys_semsys_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(int) a4;
		syscallarg(int) a5;
	} */
	struct compat_14_sys___semctl_args /* {
		syscallarg(int) semid;
		syscallarg(int) semnum;
		syscallarg(int) cmd;
		syscallarg(union __semun *) arg;
	} */ __semctl_args;
	struct sys_semget_args /* {
		syscallarg(key_t) key;
		syscallarg(int) nsems;
		syscallarg(int) semflg;
	} */ semget_args;
	struct sys_semop_args /* {
		syscallarg(int) semid;
		syscallarg(struct sembuf *) sops;
		syscallarg(u_int) nsops;
	} */ semop_args;
	struct sys_semconfig_args /* {
		syscallarg(int) flag;
	} */ semconfig_args;

	switch (SCARG(uap, which)) {
	case 0:						/* __semctl() */
		SCARG(&__semctl_args, semid) = SCARG(uap, a2);
		SCARG(&__semctl_args, semnum) = SCARG(uap, a3);
		SCARG(&__semctl_args, cmd) = SCARG(uap, a4);
		SCARG(&__semctl_args, arg) = (union __semun *)SCARG(uap, a5);
		return (compat_14_sys___semctl(l, &__semctl_args, retval));

	case 1:						/* semget() */
		SCARG(&semget_args, key) = SCARG(uap, a2);
		SCARG(&semget_args, nsems) = SCARG(uap, a3);
		SCARG(&semget_args, semflg) = SCARG(uap, a4);
		return (sys_semget(l, &semget_args, retval));

	case 2:						/* semop() */
		SCARG(&semop_args, semid) = SCARG(uap, a2);
		SCARG(&semop_args, sops) = (struct sembuf *)SCARG(uap, a3);
		SCARG(&semop_args, nsops) = SCARG(uap, a4);
		return (sys_semop(l, &semop_args, retval));

	case 3:						/* semconfig() */
		SCARG(&semconfig_args, flag) = SCARG(uap, a2);
		return (sys_semconfig(l, &semconfig_args, retval));

	default:
		return (EINVAL);
	}
}
#endif

#ifdef SYSVSHM
int
freebsd_sys_shmsys(struct lwp *l, const struct freebsd_sys_shmsys_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(int) a4;
	} */
	struct sys_shmat_args /* {
		syscallarg(int) shmid;
		syscallarg(void *) shmaddr;
		syscallarg(int) shmflg;
	} */ shmat_args;
	struct compat_14_sys_shmctl_args /* {
		syscallarg(int) shmid;
		syscallarg(int) cmd;
		syscallarg(struct shmid_ds14 *) buf;
	} */ shmctl_args;
	struct sys_shmdt_args /* {
		syscallarg(void *) shmaddr;
	} */ shmdt_args;
	struct sys_shmget_args /* {
		syscallarg(key_t) key;
		syscallarg(int) size;
		syscallarg(int) shmflg;
	} */ shmget_args;

	switch (SCARG(uap, which)) {
	case 0:						/* shmat() */
		SCARG(&shmat_args, shmid) = SCARG(uap, a2);
		SCARG(&shmat_args, shmaddr) = (void *)SCARG(uap, a3);
		SCARG(&shmat_args, shmflg) = SCARG(uap, a4);
		return (sys_shmat(l, &shmat_args, retval));

	case 1:						/* oshmctl() */
		/* XXX Need to translate shmid_ds format. */
		return (EINVAL);

	case 2:						/* shmdt() */
		SCARG(&shmdt_args, shmaddr) = (void *)SCARG(uap, a2);
		return (sys_shmdt(l, &shmdt_args, retval));

	case 3:						/* shmget() */
		SCARG(&shmget_args, key) = SCARG(uap, a2);
		SCARG(&shmget_args, size) = SCARG(uap, a3);
		SCARG(&shmget_args, shmflg) = SCARG(uap, a4);
		return (sys_shmget(l, &shmget_args, retval));

	case 4:						/* shmctl() */
		SCARG(&shmctl_args, shmid) = SCARG(uap, a2);
		SCARG(&shmctl_args, cmd) = SCARG(uap, a3);
		SCARG(&shmctl_args, buf) = (struct shmid_ds14 *)SCARG(uap, a4);
		return (compat_14_sys_shmctl(l, &shmctl_args, retval));

	default:
		return (EINVAL);
	}
}
#endif

#ifdef SYSVMSG
int
freebsd_sys_msgsys(struct lwp *l, const struct freebsd_sys_msgsys_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(int) a4;
		syscallarg(int) a5;
		syscallarg(int) a6;
	} */
	struct compat_14_sys_msgctl_args /* {
		syscallarg(int) msqid;
		syscallarg(int) cmd;
		syscallarg(struct msqid_ds14 *) buf;
	} */ msgctl_args;
	struct sys_msgget_args /* {
		syscallarg(key_t) key;
		syscallarg(int) msgflg;
	} */ msgget_args;
	struct sys_msgsnd_args /* {
		syscallarg(int) msqid;
		syscallarg(void *) msgp;
		syscallarg(size_t) msgsz;
		syscallarg(int) msgflg;
	} */ msgsnd_args;
	struct sys_msgrcv_args /* {
		syscallarg(int) msqid;
		syscallarg(void *) msgp;
		syscallarg(size_t) msgsz;
		syscallarg(long) msgtyp;
		syscallarg(int) msgflg;
	} */ msgrcv_args;

	switch (SCARG(uap, which)) {
	case 0:					/* msgctl()*/
		SCARG(&msgctl_args, msqid) = SCARG(uap, a2);
		SCARG(&msgctl_args, cmd) = SCARG(uap, a3);
		SCARG(&msgctl_args, buf) =
		    (struct msqid_ds14 *)SCARG(uap, a4);
		return (compat_14_sys_msgctl(l, &msgctl_args, retval));

	case 1:					/* msgget() */
		SCARG(&msgget_args, key) = SCARG(uap, a2);
		SCARG(&msgget_args, msgflg) = SCARG(uap, a3);
		return (sys_msgget(l, &msgget_args, retval));

	case 2:					/* msgsnd() */
		SCARG(&msgsnd_args, msqid) = SCARG(uap, a2);
		SCARG(&msgsnd_args, msgp) = (void *)SCARG(uap, a3);
		SCARG(&msgsnd_args, msgsz) = SCARG(uap, a4);
		SCARG(&msgsnd_args, msgflg) = SCARG(uap, a5);
		return (sys_msgsnd(l, &msgsnd_args, retval));

	case 3:					/* msgrcv() */
		SCARG(&msgrcv_args, msqid) = SCARG(uap, a2);
		SCARG(&msgrcv_args, msgp) = (void *)SCARG(uap, a3);
		SCARG(&msgrcv_args, msgsz) = SCARG(uap, a4);
		SCARG(&msgrcv_args, msgtyp) = SCARG(uap, a5);
		SCARG(&msgrcv_args, msgflg) = SCARG(uap, a6);
		return (sys_msgrcv(l, &msgrcv_args, retval));

	default:
		return (EINVAL);
	}
}
#endif
