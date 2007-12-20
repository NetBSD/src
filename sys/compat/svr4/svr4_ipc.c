/*	$NetBSD: svr4_ipc.c,v 1.25 2007/12/20 23:03:05 dsl Exp $	*/

/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: svr4_ipc.c,v 1.25 2007/12/20 23:03:05 dsl Exp $");

#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#endif

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/systm.h>
#include <sys/stat.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_signal.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_ipc.h>

#if defined(SYSVMSG) || defined(SYSVSHM) || defined(SYSVSEM)

static void
svr4_to_bsd_ipc_perm(const struct svr4_ipc_perm *spp, struct ipc_perm *bpp)
{
	bpp->_key = spp->key;
	bpp->uid = spp->uid;
	bpp->gid = spp->gid;
	bpp->cuid = spp->cuid;
	bpp->cgid = spp->cgid;
	bpp->mode = spp->mode;
	bpp->_seq = spp->seq;
}

static void
bsd_to_svr4_ipc_perm(const struct ipc_perm *bpp, struct svr4_ipc_perm *spp)
{
	spp->key = bpp->_key;
	spp->uid = bpp->uid;
	spp->gid = bpp->gid;
	spp->cuid = bpp->cuid;
	spp->cgid = bpp->cgid;
	spp->mode = bpp->mode;
	spp->seq = bpp->_seq;
}
#endif

#ifdef SYSVSEM
static void
bsd_to_svr4_semid_ds(const struct semid_ds *bds, struct svr4_semid_ds *sds)
{
	bsd_to_svr4_ipc_perm(&bds->sem_perm, &sds->sem_perm);
	sds->sem_base = (struct svr4_sem *) bds->_sem_base;
	sds->sem_nsems = bds->sem_nsems;
	sds->sem_otime = bds->sem_otime;
	sds->sem_ctime = bds->sem_ctime;
}

static void
svr4_to_bsd_semid_ds(const struct svr4_semid_ds *sds, struct semid_ds *bds)
{
	svr4_to_bsd_ipc_perm(&sds->sem_perm, &bds->sem_perm);
	bds->_sem_base = (struct __sem *) sds->sem_base;
	bds->sem_nsems = sds->sem_nsems;
	bds->sem_otime = sds->sem_otime;
	bds->sem_ctime = sds->sem_ctime;
}

struct svr4_sys_semctl_args {
	syscallarg(int) what;
	syscallarg(int) semid;
	syscallarg(int) semnum;
	syscallarg(int) cmd;
	syscallarg(union __semun) arg;
};

static int
svr4_semctl(struct lwp *l, const struct svr4_sys_semctl_args *uap, register_t *retval)
{
	struct semid_ds sembuf;
	struct svr4_semid_ds ssembuf;
	int cmd, error;
	void *pass_arg = NULL;
	union __semun arg = SCARG(uap, arg);

	cmd = SCARG(uap, cmd);

	switch (cmd) {
	case SVR4_IPC_SET:
		pass_arg = &sembuf;
		cmd = IPC_SET;
		break;

	case SVR4_IPC_STAT:
		pass_arg = &sembuf;
		cmd = IPC_STAT;
		break;

	case SVR4_IPC_RMID:
		cmd = IPC_RMID;
		break;

	case SVR4_SEM_GETVAL:
		cmd = GETVAL;
		break;

	case SVR4_SEM_GETPID:
		cmd = GETPID;
		break;

	case SVR4_SEM_GETNCNT:
		cmd = GETNCNT;
		break;

	case SVR4_SEM_GETZCNT:
		cmd = GETZCNT;
		break;

	case SVR4_SEM_GETALL:
		pass_arg = &arg;
		cmd = GETALL;
		break;

	case SVR4_SEM_SETVAL:
		pass_arg = &arg;
		cmd = SETVAL;
		break;

	case SVR4_SEM_SETALL:
		pass_arg = &arg;
		cmd = SETALL;
		break;

	default:
		return (EINVAL);
	}

	if (cmd == IPC_SET) {
		error = copyin(SCARG(uap, arg).buf, &ssembuf, sizeof(ssembuf));
		if (error)
			return (error);
		svr4_to_bsd_semid_ds(&ssembuf, &sembuf);
	}

	error = semctl1(l, SCARG(uap, semid), SCARG(uap, semnum), cmd,
	    pass_arg, retval);

	if (error == 0 && cmd == IPC_STAT) {
		bsd_to_svr4_semid_ds(&sembuf, &ssembuf);
		error = copyout(&ssembuf, SCARG(uap, arg).buf, sizeof(ssembuf));
	}

	return (error);
}

struct svr4_sys_semget_args {
	syscallarg(int) what;
	syscallarg(svr4_key_t) key;
	syscallarg(int) nsems;
	syscallarg(int) semflg;
};

static int
svr4_semget(struct lwp *l, const struct svr4_sys_semget_args *uap, register_t *retval)
{
	struct sys_semget_args ap;

	SCARG(&ap, key) = SCARG(uap, key);
	SCARG(&ap, nsems) = SCARG(uap, nsems);
	SCARG(&ap, semflg) = SCARG(uap, semflg);

	return sys_semget(l, &ap, retval);
}

struct svr4_sys_semop_args {
	syscallarg(int) what;
	syscallarg(int) semid;
	syscallarg(struct svr4_sembuf *) sops;
	syscallarg(u_int) nsops;
};

static int
svr4_semop(struct lwp *l, const struct svr4_sys_semop_args *uap, register_t *retval)
{
	struct sys_semop_args ap;

	SCARG(&ap, semid) = SCARG(uap, semid);
	/* These are the same */
	SCARG(&ap, sops) = (struct sembuf *) SCARG(uap, sops);
	SCARG(&ap, nsops) = SCARG(uap, nsops);

	return sys_semop(l, &ap, retval);
}

int
svr4_sys_semsys(struct lwp *l, const struct svr4_sys_semsys_args *uap, register_t *retval)
{

	DPRINTF(("svr4_semsys(%d)\n", SCARG(uap, what)));

	switch (SCARG(uap, what)) {
	case SVR4_semctl:
		return svr4_semctl(l, (const void *)uap, retval);
	case SVR4_semget:
		return svr4_semget(l, (const void *)uap, retval);
	case SVR4_semop:
		return svr4_semop(l, (const void *)uap, retval);
	default:
		return EINVAL;
	}
}
#endif

#ifdef SYSVMSG
static void
bsd_to_svr4_msqid_ds(const struct msqid_ds *bds, struct svr4_msqid_ds *sds)
{
	bsd_to_svr4_ipc_perm(&bds->msg_perm, &sds->msg_perm);
	sds->msg_first = (struct svr4_msg *) bds->_msg_first;
	sds->msg_last = (struct svr4_msg *) bds->_msg_last;
	sds->msg_cbytes = bds->_msg_cbytes;
	sds->msg_qnum = bds->msg_qnum;
	sds->msg_qbytes = bds->msg_qbytes;
	sds->msg_lspid = bds->msg_lspid;
	sds->msg_lrpid = bds->msg_lrpid;
	sds->msg_stime = bds->msg_stime;
	sds->msg_rtime = bds->msg_rtime;
	sds->msg_ctime = bds->msg_ctime;

#if 0
	/* XXX What to put here? */
	sds->msg_cv = 0;
	sds->msg_qnum_cv = 0;
#endif
}

static void
svr4_to_bsd_msqid_ds(const struct svr4_msqid_ds *sds, struct msqid_ds *bds)
{
	svr4_to_bsd_ipc_perm(&sds->msg_perm, &bds->msg_perm);
	bds->_msg_first = (struct __msg *) sds->msg_first;
	bds->_msg_last = (struct __msg *) sds->msg_last;
	bds->_msg_cbytes = sds->msg_cbytes;
	bds->msg_qnum = sds->msg_qnum;
	bds->msg_qbytes = sds->msg_qbytes;
	bds->msg_lspid = sds->msg_lspid;
	bds->msg_lrpid = sds->msg_lrpid;
	bds->msg_stime = sds->msg_stime;
	bds->msg_rtime = sds->msg_rtime;
	bds->msg_ctime = sds->msg_ctime;

#if 0
	XXX sds->msg_cv
	XXX sds->msg_qnum_cv
#endif
}

struct svr4_sys_msgsnd_args {
	syscallarg(int) what;
	syscallarg(int) msqid;
	syscallarg(void *) msgp;
	syscallarg(size_t) msgsz;
	syscallarg(int) msgflg;
};

static int
svr4_msgsnd(struct lwp *l, const struct svr4_sys_msgsnd_args *uap, register_t *retval)
{
	struct sys_msgsnd_args ap;

	SCARG(&ap, msqid) = SCARG(uap, msqid);
	SCARG(&ap, msgp) = SCARG(uap, msgp);
	SCARG(&ap, msgsz) = SCARG(uap, msgsz);
	SCARG(&ap, msgflg) = SCARG(uap, msgflg);

	return sys_msgsnd(l, &ap, retval);
}

struct svr4_sys_msgrcv_args {
	syscallarg(int) what;
	syscallarg(int) msqid;
	syscallarg(void *) msgp;
	syscallarg(size_t) msgsz;
	syscallarg(long) msgtyp;
	syscallarg(int) msgflg;
};

static int
svr4_msgrcv(struct lwp *l, const struct svr4_sys_msgrcv_args *uap, register_t *retval)
{
	struct sys_msgrcv_args ap;

	SCARG(&ap, msqid) = SCARG(uap, msqid);
	SCARG(&ap, msgp) = SCARG(uap, msgp);
	SCARG(&ap, msgsz) = SCARG(uap, msgsz);
	SCARG(&ap, msgtyp) = SCARG(uap, msgtyp);
	SCARG(&ap, msgflg) = SCARG(uap, msgflg);

	return sys_msgrcv(l, &ap, retval);
}

struct svr4_sys_msgget_args {
	syscallarg(int) what;
	syscallarg(svr4_key_t) key;
	syscallarg(int) msgflg;
};

static int
svr4_msgget(struct lwp *l, const struct svr4_sys_msgget_args *uap, register_t *retval)
{
	struct sys_msgget_args ap;

	SCARG(&ap, key) = SCARG(uap, key);
	SCARG(&ap, msgflg) = SCARG(uap, msgflg);

	return sys_msgget(l, &ap, retval);
}

struct svr4_sys_msgctl_args {
	syscallarg(int) what;
	syscallarg(int) msqid;
	syscallarg(int) cmd;
	syscallarg(struct svr4_msqid_ds *) buf;
};

static int
svr4_msgctl(struct lwp *l, const struct svr4_sys_msgctl_args *uap, register_t *retval)
{
	struct svr4_msqid_ds ss;
	struct msqid_ds bs;
	int error;

	switch (SCARG(uap, cmd)) {
	case SVR4_IPC_STAT:
		error = msgctl1(l, SCARG(uap, msqid), IPC_STAT, &bs);
		if (error == 0) {
			bsd_to_svr4_msqid_ds(&bs, &ss);
			error = copyout(&ss, SCARG(uap, buf), sizeof ss);
		}
		return error;

	case SVR4_IPC_SET:
		error = copyin(SCARG(uap, buf), &ss, sizeof ss);
		if (error)
			return error;
		svr4_to_bsd_msqid_ds(&ss, &bs);
		return msgctl1(l, SCARG(uap, msqid), IPC_SET, &bs);

	case SVR4_IPC_RMID:
		return msgctl1(l, SCARG(uap, msqid), IPC_RMID, NULL);

	default:
		return EINVAL;
	}
}

int
svr4_sys_msgsys(struct lwp *l, const struct svr4_sys_msgsys_args *uap, register_t *retval)
{

	DPRINTF(("svr4_msgsys(%d)\n", SCARG(uap, what)));

	switch (SCARG(uap, what)) {
	case SVR4_msgsnd:
		return svr4_msgsnd(l, (const void *)uap, retval);
	case SVR4_msgrcv:
		return svr4_msgrcv(l, (const void *)uap, retval);
	case SVR4_msgget:
		return svr4_msgget(l, (const void *)uap, retval);
	case SVR4_msgctl:
		return svr4_msgctl(l, (const void *)uap, retval);
	default:
		return EINVAL;
	}
}
#endif

#ifdef SYSVSHM

static void
bsd_to_svr4_shmid_ds(const struct shmid_ds *bds, struct svr4_shmid_ds *sds)
{
	bsd_to_svr4_ipc_perm(&bds->shm_perm, &sds->shm_perm);
	sds->shm_segsz = bds->shm_segsz;
	sds->shm_lkcnt = 0;
	sds->shm_lpid = bds->shm_lpid;
	sds->shm_cpid = bds->shm_cpid;
	sds->shm_amp = bds->_shm_internal;
	sds->shm_nattch = bds->shm_nattch;
	sds->shm_cnattch = 0;
	sds->shm_atime = bds->shm_atime;
	sds->shm_pad1 = 0;
	sds->shm_dtime = bds->shm_dtime;
	sds->shm_pad2 = 0;
	sds->shm_ctime = bds->shm_ctime;
	sds->shm_pad3 = 0;
}

static void
svr4_to_bsd_shmid_ds(const struct svr4_shmid_ds *sds, struct shmid_ds *bds)
{
	svr4_to_bsd_ipc_perm(&sds->shm_perm, &bds->shm_perm);
	bds->shm_segsz = sds->shm_segsz;
	bds->shm_lpid = sds->shm_lpid;
	bds->shm_cpid = sds->shm_cpid;
	bds->_shm_internal = sds->shm_amp;
	bds->shm_nattch = sds->shm_nattch;
	bds->shm_atime = sds->shm_atime;
	bds->shm_dtime = sds->shm_dtime;
	bds->shm_ctime = sds->shm_ctime;
}

struct svr4_sys_shmat_args {
	syscallarg(int) what;
	syscallarg(int) shmid;
	syscallarg(void *) shmaddr;
	syscallarg(int) shmflg;
};

static int
svr4_shmat(struct lwp *l, const struct svr4_sys_shmat_args *uap, register_t *retval)
{
	struct sys_shmat_args ap;

	SCARG(&ap, shmid) = SCARG(uap, shmid);
	SCARG(&ap, shmaddr) = SCARG(uap, shmaddr);
	SCARG(&ap, shmflg) = SCARG(uap, shmflg);

	return sys_shmat(l, &ap, retval);
}

struct svr4_sys_shmdt_args {
	syscallarg(int) what;
	syscallarg(void *) shmaddr;
};

static int
svr4_shmdt(struct lwp *l, const struct svr4_sys_shmdt_args *uap, register_t *retval)
{
	struct sys_shmdt_args ap;

	SCARG(&ap, shmaddr) = SCARG(uap, shmaddr);

	return sys_shmdt(l, &ap, retval);
}

struct svr4_sys_shmget_args {
	syscallarg(int) what;
	syscallarg(key_t) key;
	syscallarg(int) size;
	syscallarg(int) shmflg;
};

static int
svr4_shmget(struct lwp *l, const struct svr4_sys_shmget_args *uap, register_t *retval)
{
	struct sys_shmget_args ap;

	SCARG(&ap, key) = SCARG(uap, key);
	SCARG(&ap, size) = SCARG(uap, size);
	SCARG(&ap, shmflg) = SCARG(uap, shmflg);

	return sys_shmget(l, &ap, retval);
}

struct svr4_sys_shmctl_args {
	syscallarg(int) what;
	syscallarg(int) shmid;
	syscallarg(int) cmd;
	syscallarg(struct svr4_shmid_ds *) buf;
};

static int
svr4_shmctl(struct lwp *l, const struct svr4_sys_shmctl_args *uap, register_t *retval)
{
	struct shmid_ds bs;
	struct svr4_shmid_ds ss;
	int error;

	switch (SCARG(uap, cmd)) {
	case SVR4_IPC_STAT:
		error = shmctl1(l, SCARG(uap, shmid), IPC_STAT, &bs);
		if (error == 0) {
			bsd_to_svr4_shmid_ds(&bs, &ss);
			error = copyout(&ss, SCARG(uap, buf), sizeof ss);
		}
		return error;

	case SVR4_IPC_SET:
		error = copyin(SCARG(uap, buf), &ss, sizeof ss);
		if (error)
			return error;
		svr4_to_bsd_shmid_ds(&ss, &bs);
		return shmctl1(l, SCARG(uap, shmid), IPC_SET, &bs);

	case SVR4_IPC_RMID:
		return shmctl1(l, SCARG(uap, shmid), IPC_RMID, NULL);

	case SVR4_SHM_LOCK:
		return shmctl1(l, SCARG(uap, shmid), SHM_LOCK, NULL);

	case SVR4_SHM_UNLOCK:
		return shmctl1(l, SCARG(uap, shmid), SHM_UNLOCK, NULL);

	default:
		return EINVAL;
	}
}

int
svr4_sys_shmsys(struct lwp *l, const struct svr4_sys_shmsys_args *uap, register_t *retval)
{

	DPRINTF(("svr4_shmsys(%d)\n", SCARG(uap, what)));

	switch (SCARG(uap, what)) {
	case SVR4_shmat:
		return svr4_shmat(l, (const void *)uap, retval);
	case SVR4_shmdt:
		return svr4_shmdt(l, (const void *)uap, retval);
	case SVR4_shmget:
		return svr4_shmget(l, (const void *)uap, retval);
	case SVR4_shmctl:
		return svr4_shmctl(l, (const void *)uap, retval);
	default:
		return ENOSYS;
	}
}
#endif /* SYSVSHM */
