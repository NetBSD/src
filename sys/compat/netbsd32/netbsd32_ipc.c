/*	$NetBSD: netbsd32_ipc.c,v 1.2 2001/05/30 11:37:28 mrg Exp $	*/

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

#include <sys/syscallargs.h>
#include <sys/proc.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

#if defined(SYSVSEM)
/*
 * XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 *
 * This is BSD.  We won't support System V IPC.
 * Too much work.
 *
 * XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 */
int
netbsd32___semctl14(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32___semctl_args /* {
		syscallarg(int) semid;
		syscallarg(int) semnum;
		syscallarg(int) cmd;
		syscallarg(netbsd32_semunu_t *) arg;
	} */ *uap = v;
	union netbsd32_semun sem32;
	int semid = SCARG(uap, semid);
	int semnum = SCARG(uap, semnum);
	int cmd = SCARG(uap, cmd);
	union netbsd32_semun *arg = (void*)(u_long)SCARG(uap, arg);
	union netbsd32_semun real_arg;
	struct ucred *cred = p->p_ucred;
	int i, rval, eval;
	struct netbsd32_semid_ds sbuf;
	struct semid_ds *semaptr;

	semlock(p);

	semid = IPCID_TO_IX(semid);
	if (semid < 0 || semid >= seminfo.semmsl)
		return(EINVAL);

	semaptr = &sema[semid];
	if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0 ||
	    semaptr->sem_perm.seq != IPCID_TO_SEQ(SCARG(uap, semid)))
		return(EINVAL);

	eval = 0;
	rval = 0;

	switch (cmd) {
	case IPC_RMID:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_M)) != 0)
			return(eval);
		semaptr->sem_perm.cuid = cred->cr_uid;
		semaptr->sem_perm.uid = cred->cr_uid;
		semtot -= semaptr->sem_nsems;
		for (i = semaptr->_sem_base - sem; i < semtot; i++)
			sem[i] = sem[i + semaptr->sem_nsems];
		for (i = 0; i < seminfo.semmni; i++) {
			if ((sema[i].sem_perm.mode & SEM_ALLOC) &&
			    sema[i]._sem_base > semaptr->_sem_base)
				sema[i]._sem_base -= semaptr->sem_nsems;
		}
		semaptr->sem_perm.mode = 0;
		semundo_clear(semid, -1);
		wakeup((caddr_t)semaptr);
		break;

	case IPC_SET:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_M)))
			return(eval);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		if ((eval = copyin((caddr_t)(u_long)real_arg.buf, (caddr_t)&sbuf,
		    sizeof(sbuf))) != 0)
			return(eval);
		semaptr->sem_perm.uid = sbuf.sem_perm.uid;
		semaptr->sem_perm.gid = sbuf.sem_perm.gid;
		semaptr->sem_perm.mode = (semaptr->sem_perm.mode & ~0777) |
		    (sbuf.sem_perm.mode & 0777);
		semaptr->sem_ctime = time.tv_sec;
		break;

	case IPC_STAT:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		eval = copyout((caddr_t)semaptr, (caddr_t)(u_long)real_arg.buf,
		    sizeof(struct semid_ds));
		break;

	case GETNCNT:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		rval = semaptr->_sem_base[semnum].semncnt;
		break;

	case GETPID:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		rval = semaptr->_sem_base[semnum].sempid;
		break;

	case GETVAL:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		rval = semaptr->_sem_base[semnum].semval;
		break;

	case GETALL:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		for (i = 0; i < semaptr->sem_nsems; i++) {
			eval = copyout((caddr_t)&semaptr->_sem_base[i].semval,
			    &real_arg.array[i], sizeof(real_arg.array[0]));
			if (eval != 0)
				break;
		}
		break;

	case GETZCNT:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		rval = semaptr->_sem_base[semnum].semzcnt;
		break;

	case SETVAL:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_W)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		semaptr->_sem_base[semnum].semval = real_arg.val;
		semundo_clear(semid, semnum);
		wakeup((caddr_t)semaptr);
		break;

	case SETALL:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_W)))
			return(eval);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		for (i = 0; i < semaptr->sem_nsems; i++) {
			eval = copyin(&real_arg.array[i],
			    (caddr_t)&semaptr->_sem_base[i].semval,
			    sizeof(real_arg.array[0]));
			if (eval != 0)
				break;
		}
		semundo_clear(semid, -1);
		wakeup((caddr_t)semaptr);
		break;

	default:
		return(EINVAL);
	}

	if (eval == 0)
		*retval = rval;
	return(eval);
#else
	return (ENOSYS);
#endif
}

int
netbsd32_semget(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_semget_args /* {
		syscallarg(netbsd32_key_t) key;
		syscallarg(int) nsems;
		syscallarg(int) semflg;
	} */ *uap = v;
	struct sys_semget_args ua;

	NETBSD32TOX_UAP(key, key_t);
	NETBSD32TO64_UAP(nsems);
	NETBSD32TO64_UAP(semflg);
	return (sys_semget(p, &ua, retval));
}

int
netbsd32_semop(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_semop_args /* {
		syscallarg(int) semid;
		syscallarg(netbsd32_sembufp_t) sops;
		syscallarg(netbsd32_size_t) nsops;
	} */ *uap = v;
	struct sys_semop_args ua;

	NETBSD32TO64_UAP(semid);
	NETBSD32TOP_UAP(sops, struct sembuf);
	NETBSD32TOX_UAP(nsops, size_t);
	return (sys_semop(p, &ua, retval));
}

int
netbsd32_semconfig(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_semconfig_args /* {
		syscallarg(int) flag;
	} */ *uap = v;
	struct sys_semconfig_args ua;

	NETBSD32TO64_UAP(flag);
	return (sys_semconfig(p, &ua, retval));
}
#endif /* SYSVSEM */

#if defined(SYSVMSG)

int
netbsd32___msgctl13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_msgctl_args /* {
		syscallarg(int) msqid;
		syscallarg(int) cmd;
		syscallarg(netbsd32_msqid_dsp_t) buf;
	} */ *uap = v;
	struct sys_msgctl_args ua;
	struct msqid_ds ds;
	struct netbsd32_msqid_ds *ds32p;
	int error;

	NETBSD32TO64_UAP(msqid);
	NETBSD32TO64_UAP(cmd);
	ds32p = (struct netbsd32_msqid_ds *)(u_long)SCARG(uap, buf);
	if (ds32p) {
		SCARG(&ua, buf) = NULL;
		netbsd32_to_msqid_ds(ds32p, &ds);
	} else
		SCARG(&ua, buf) = NULL;
	error = sys_msgctl(p, &ua, retval);
	if (error)
		return (error);

	if (ds32p)
		netbsd32_from_msqid_ds(&ds, ds32p);
	return (0);
#else
	return (ENOSYS);
#endif
}

int
netbsd32_msgget(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_msgget_args /* {
		syscallarg(netbsd32_key_t) key;
		syscallarg(int) msgflg;
	} */ *uap = v;
	struct sys_msgget_args ua;

	NETBSD32TOX_UAP(key, key_t);
	NETBSD32TO64_UAP(msgflg);
	return (sys_msgget(p, &ua, retval));
#else
	return (ENOSYS);
#endif
}

int
netbsd32_msgsnd(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_msgsnd_args /* {
		syscallarg(int) msqid;
		syscallarg(const netbsd32_voidp) msgp;
		syscallarg(netbsd32_size_t) msgsz;
		syscallarg(int) msgflg;
	} */ *uap = v;
	struct sys_msgsnd_args ua;

	NETBSD32TO64_UAP(msqid);
	NETBSD32TOP_UAP(msgp, void);
	NETBSD32TOX_UAP(msgsz, size_t);
	NETBSD32TO64_UAP(msgflg);
	return (sys_msgsnd(p, &ua, retval));
#else
	return (ENOSYS);
#endif
}

int
netbsd32_msgrcv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_msgrcv_args /* {
		syscallarg(int) msqid;
		syscallarg(netbsd32_voidp) msgp;
		syscallarg(netbsd32_size_t) msgsz;
		syscallarg(netbsd32_long) msgtyp;
		syscallarg(int) msgflg;
	} */ *uap = v;
	struct sys_msgrcv_args ua;
	ssize_t rt;
	int error;

	NETBSD32TO64_UAP(msqid);
	NETBSD32TOP_UAP(msgp, void);
	NETBSD32TOX_UAP(msgsz, size_t);
	NETBSD32TOX_UAP(msgtyp, long);
	NETBSD32TO64_UAP(msgflg);
	error = sys_msgrcv(p, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
#else
	return (ENOSYS);
#endif
}
#endif /* SYSVMSG */

#if defined(SYSVSHM)

int
netbsd32_shmat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_shmat_args /* {
		syscallarg(int) shmid;
		syscallarg(const netbsd32_voidp) shmaddr;
		syscallarg(int) shmflg;
	} */ *uap = v;
	struct sys_shmat_args ua;
	void *rt;
	int error;

	NETBSD32TO64_UAP(shmid);
	NETBSD32TOP_UAP(shmaddr, void);
	NETBSD32TO64_UAP(shmflg);
	error = sys_shmat(p, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
#else
	return (ENOSYS);
#endif
}

int
netbsd32___shmctl13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_shmctl_args /* {
		syscallarg(int) shmid;
		syscallarg(int) cmd;
		syscallarg(netbsd32_shmid_dsp_t) buf;
	} */ *uap = v;
	struct sys_shmctl_args ua;
	struct shmid_ds ds;
	struct netbsd32_shmid_ds *ds32p;
	int error;

	NETBSD32TO64_UAP(shmid);
	NETBSD32TO64_UAP(cmd);
	ds32p = (struct netbsd32_shmid_ds *)(u_long)SCARG(uap, buf);
	if (ds32p) {
		SCARG(&ua, buf) = NULL;
		netbsd32_to_shmid_ds(ds32p, &ds);
	} else
		SCARG(&ua, buf) = NULL;
	error = sys_shmctl(p, &ua, retval);
	if (error)
		return (error);

	if (ds32p)
		netbsd32_from_shmid_ds(&ds, ds32p);
	return (0);
#else
	return (ENOSYS);
#endif
}

int
netbsd32_shmdt(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_shmdt_args /* {
		syscallarg(const netbsd32_voidp) shmaddr;
	} */ *uap = v;
	struct sys_shmdt_args ua;

	NETBSD32TOP_UAP(shmaddr, const char);
	return (sys_shmdt(p, &ua, retval));
#else
	return (ENOSYS);
#endif
}

int
netbsd32_shmget(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_shmget_args /* {
		syscallarg(netbsd32_key_t) key;
		syscallarg(netbsd32_size_t) size;
		syscallarg(int) shmflg;
	} */ *uap = v;
	struct sys_shmget_args ua;

	NETBSD32TOX_UAP(key, key_t)
	NETBSD32TOX_UAP(size, size_t)
	NETBSD32TO64_UAP(shmflg);
	return (sys_shmget(p, &ua, retval));
#else
	return (ENOSYS);
#endif
}
#endif /* SYSVSHM */
