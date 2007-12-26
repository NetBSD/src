/*	$NetBSD: linux_ipc.c,v 1.40.16.1 2007/12/26 19:49:17 ad Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: linux_ipc.c,v 1.40.16.1 2007/12/26 19:49:17 ad Exp $");

#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#endif

#include <sys/param.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_msg.h>
#include <compat/linux/common/linux_shm.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_syscall.h>

#include <compat/linux/common/linux_ipccall.h>

/*
 * Note: Not all linux architechtures have explicit versions
 *	of the SYSV* syscalls.  On the ones that don't
 *	we pretend that they are defined anyway.  *_args and
 *	prototypes are defined in individual headers;
 *	syscalls.master lists those syscalls as NOARGS.
 *
 *	The functions in multiarch are the ones that just need
 *	the arguments shuffled around and then use the
 *	normal NetBSD syscall.
 *
 * Function in multiarch:
 *	linux_sys_ipc		: linux_ipccall.c
 *	liunx_semop		: linux_ipccall.c
 *	linux_semget		: linux_ipccall.c
 *	linux_msgsnd		: linux_ipccall.c
 *	linux_msgrcv		: linux_ipccall.c
 *	linux_msgget		: linux_ipccall.c
 *	linux_shmdt		: linux_ipccall.c
 *	linux_shmget		: linux_ipccall.c
 */

#if defined (SYSVSEM) || defined(SYSVSHM) || defined(SYSVMSG)
/*
 * Convert between Linux and NetBSD ipc_perm structures. Only the
 * order of the fields is different.
 */
void
linux_to_bsd_ipc_perm(struct linux_ipc_perm *lpp, struct ipc_perm *bpp)
{

	bpp->_key = lpp->l_key;
	bpp->uid = lpp->l_uid;
	bpp->gid = lpp->l_gid;
	bpp->cuid = lpp->l_cuid;
	bpp->cgid = lpp->l_cgid;
	bpp->mode = lpp->l_mode;
	bpp->_seq = lpp->l_seq;
}

void
linux_to_bsd_ipc64_perm(struct linux_ipc64_perm *lpp, struct ipc_perm *bpp)
{
	bpp->_key = lpp->l_key;
	bpp->uid = lpp->l_uid;
	bpp->gid = lpp->l_gid;
	bpp->cuid = lpp->l_cuid;
	bpp->cgid = lpp->l_cgid;
	bpp->mode = lpp->l_mode;
	bpp->_seq = lpp->l_seq;
}

void
bsd_to_linux_ipc_perm(struct ipc_perm *bpp, struct linux_ipc_perm *lpp)
{

	lpp->l_key = bpp->_key;
	lpp->l_uid = bpp->uid;
	lpp->l_gid = bpp->gid;
	lpp->l_cuid = bpp->cuid;
	lpp->l_cgid = bpp->cgid;
	lpp->l_mode = bpp->mode;
	lpp->l_seq = bpp->_seq;
}

void
bsd_to_linux_ipc64_perm(struct ipc_perm *bpp, struct linux_ipc64_perm *lpp)
{
	lpp->l_key = bpp->_key;
	lpp->l_uid = bpp->uid;
	lpp->l_gid = bpp->gid;
	lpp->l_cuid = bpp->cuid;
	lpp->l_cgid = bpp->cgid;
	lpp->l_mode = bpp->mode;
	lpp->l_seq = bpp->_seq;
}

#endif

#ifdef SYSVSEM
/*
 * Semaphore operations. Most constants and structures are the same on
 * both systems. Only semctl() needs some extra work.
 */

/*
 * Convert between Linux and NetBSD semid_ds structures.
 */
void
bsd_to_linux_semid_ds(struct semid_ds *bs, struct linux_semid_ds *ls)
{

	bsd_to_linux_ipc_perm(&bs->sem_perm, &ls->l_sem_perm);
	ls->l_sem_otime = bs->sem_otime;
	ls->l_sem_ctime = bs->sem_ctime;
	ls->l_sem_nsems = bs->sem_nsems;
	ls->l_sem_base = bs->_sem_base;
}

void
linux_to_bsd_semid_ds(struct linux_semid_ds *ls, struct semid_ds *bs)
{

	linux_to_bsd_ipc_perm(&ls->l_sem_perm, &bs->sem_perm);
	bs->sem_otime = ls->l_sem_otime;
	bs->sem_ctime = ls->l_sem_ctime;
	bs->sem_nsems = ls->l_sem_nsems;
	bs->_sem_base = ls->l_sem_base;
}

/*
 * Most of this can be handled by directly passing the arguments on; we
 * just need to frob the `cmd' and convert the semid_ds and semun.
 */
int
linux_sys_semctl(struct lwp *l, const struct linux_sys_semctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) semid;
		syscallarg(int) semnum;
		syscallarg(int) cmd;
		syscallarg(union linux_semun) arg;
	} */
	struct semid_ds sembuf;
	struct linux_semid_ds lsembuf;
	union __semun semun;
	int cmd, error;
	void *pass_arg = NULL;

	cmd = SCARG(uap, cmd);

	switch (cmd) {
	case LINUX_IPC_SET:
		pass_arg = &sembuf;
		cmd = IPC_SET;
		break;

	case LINUX_IPC_STAT:
		pass_arg = &sembuf;
		cmd = IPC_STAT;
		break;

	case LINUX_IPC_RMID:
		cmd = IPC_RMID;
		break;

	case LINUX_GETVAL:
		cmd = GETVAL;
		break;

	case LINUX_GETPID:
		cmd = GETPID;
		break;

	case LINUX_GETNCNT:
		cmd = GETNCNT;
		break;

	case LINUX_GETZCNT:
		cmd = GETZCNT;
		break;

	case LINUX_GETALL:
		pass_arg = &semun;
		semun.array = SCARG(uap, arg).l_array;
		cmd = GETALL;
		break;

	case LINUX_SETVAL:
		pass_arg = &semun;
		semun.val = SCARG(uap, arg).l_val;
		cmd = SETVAL;
		break;

	case LINUX_SETALL:
		pass_arg = &semun;
		semun.array = SCARG(uap, arg).l_array;
		cmd = SETALL;
		break;

	default:
		return (EINVAL);
	}

	if (cmd == IPC_SET) {
		error = copyin(SCARG(uap, arg).l_buf, &lsembuf,
		    sizeof(lsembuf));
		if (error)
			return (error);
		linux_to_bsd_semid_ds(&lsembuf, &sembuf);
	}

	error = semctl1(l, SCARG(uap, semid), SCARG(uap, semnum), cmd,
	    pass_arg, retval);

	if (error == 0 && cmd == IPC_STAT) {
		bsd_to_linux_semid_ds(&sembuf, &lsembuf);
		error = copyout(&lsembuf, SCARG(uap, arg).l_buf,
		    sizeof(lsembuf));
	}

	return (error);
}
#endif /* SYSVSEM */

#ifdef SYSVMSG

void
linux_to_bsd_msqid_ds(struct linux_msqid_ds *lmp, struct msqid_ds *bmp)
{

	linux_to_bsd_ipc_perm(&lmp->l_msg_perm, &bmp->msg_perm);
	bmp->_msg_first = lmp->l_msg_first;
	bmp->_msg_last = lmp->l_msg_last;
	bmp->_msg_cbytes = lmp->l_msg_cbytes;
	bmp->msg_qnum = lmp->l_msg_qnum;
	bmp->msg_qbytes = lmp->l_msg_qbytes;
	bmp->msg_lspid = lmp->l_msg_lspid;
	bmp->msg_lrpid = lmp->l_msg_lrpid;
	bmp->msg_stime = lmp->l_msg_stime;
	bmp->msg_rtime = lmp->l_msg_rtime;
	bmp->msg_ctime = lmp->l_msg_ctime;
}

void
bsd_to_linux_msqid_ds(struct msqid_ds *bmp, struct linux_msqid_ds *lmp)
{

	bsd_to_linux_ipc_perm(&bmp->msg_perm, &lmp->l_msg_perm);
	lmp->l_msg_first = bmp->_msg_first;
	lmp->l_msg_last = bmp->_msg_last;
	lmp->l_msg_cbytes = bmp->_msg_cbytes;
	lmp->l_msg_qnum = bmp->msg_qnum;
	lmp->l_msg_qbytes = bmp->msg_qbytes;
	lmp->l_msg_lspid = bmp->msg_lspid;
	lmp->l_msg_lrpid = bmp->msg_lrpid;
	lmp->l_msg_stime = bmp->msg_stime;
	lmp->l_msg_rtime = bmp->msg_rtime;
	lmp->l_msg_ctime = bmp->msg_ctime;
}

int
linux_sys_msgctl(struct lwp *l, const struct linux_sys_msgctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) msqid;
		syscallarg(int) cmd;
		syscallarg(struct linux_msqid_ds *) buf;
	} */
	struct msqid_ds bm;
	struct linux_msqid_ds lm;
	int error;

	switch (SCARG(uap, cmd)) {
	case LINUX_IPC_STAT:
		error = msgctl1(l, SCARG(uap, msqid), IPC_STAT, &bm);
		if (error == 0) {
			bsd_to_linux_msqid_ds(&bm, &lm);
			error = copyout(&lm, SCARG(uap, buf), sizeof lm);
		}
		return error;
	case LINUX_IPC_SET:
		if ((error = copyin(SCARG(uap, buf), &lm, sizeof lm)))
			return error;
		linux_to_bsd_msqid_ds(&lm, &bm);
		return msgctl1(l, SCARG(uap, msqid), IPC_SET, &bm);
	case LINUX_IPC_RMID:
		return msgctl1(l, SCARG(uap, msqid), IPC_RMID, NULL);
		break;
	default:
		return EINVAL;
	}
}
#endif /* SYSVMSG */

#ifdef SYSVSHM
/*
 * shmget(2). Just make sure the Linux-compatible shmat() semantics
 * is enabled for the segment, so that shmat() succeeds even when
 * the segment would be removed.
 */
int
linux_sys_shmget(struct lwp *l, const struct linux_sys_shmget_args *uap, register_t *retval)
{
	/* {
		syscallarg(key_t) key;
		syscallarg(size_t) size;
		syscallarg(int) shmflg;
	} */
	struct sys_shmget_args bsd_ua;

	SCARG(&bsd_ua, key) = SCARG(uap, key);
	SCARG(&bsd_ua, size) = SCARG(uap, size);
	SCARG(&bsd_ua, shmflg) = SCARG(uap, shmflg) | _SHM_RMLINGER;

	return sys_shmget(l, &bsd_ua, retval);
}

/*
 * shmat(2). Very straightforward, except that Linux passes a pointer
 * in which the return value is to be passed. This is subsequently
 * handled by libc, apparently.
 */
#ifndef __amd64__
int
linux_sys_shmat(struct lwp *l, const struct linux_sys_shmat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) shmid;
		syscallarg(void *) shmaddr;
		syscallarg(int) shmflg;
		syscallarg(u_long *) raddr;
	} */
	int error;

	if ((error = sys_shmat(l, (const void *)uap, retval)))
		return error;

#ifndef __amd64__
	if ((error = copyout(&retval[0], SCARG(uap, raddr), sizeof retval[0])))
		return error;

	retval[0] = 0;
#endif
	return 0;
}
#endif /* __amd64__ */

/*
 * Convert between Linux and NetBSD shmid_ds structures.
 * The order of the fields is once again the difference, and
 * we also need a place to store the internal data pointer
 * in, which is unfortunately stored in this structure.
 *
 * We abuse a Linux internal field for that.
 */
void
linux_to_bsd_shmid_ds(struct linux_shmid_ds *lsp, struct shmid_ds *bsp)
{

	linux_to_bsd_ipc_perm(&lsp->l_shm_perm, &bsp->shm_perm);
	bsp->shm_segsz = lsp->l_shm_segsz;
	bsp->shm_lpid = lsp->l_shm_lpid;
	bsp->shm_cpid = lsp->l_shm_cpid;
	bsp->shm_nattch = lsp->l_shm_nattch;
	bsp->shm_atime = lsp->l_shm_atime;
	bsp->shm_dtime = lsp->l_shm_dtime;
	bsp->shm_ctime = lsp->l_shm_ctime;
	bsp->_shm_internal = lsp->l_private2;	/* XXX Oh well. */
}

void
linux_to_bsd_shmid64_ds(struct linux_shmid64_ds *lsp, struct shmid_ds *bsp)
{

	linux_to_bsd_ipc64_perm(&lsp->l_shm_perm, &bsp->shm_perm);
	bsp->shm_segsz = lsp->l_shm_segsz;
	bsp->shm_lpid = lsp->l_shm_lpid;
	bsp->shm_cpid = lsp->l_shm_cpid;
	bsp->shm_nattch = lsp->l_shm_nattch;
	bsp->shm_atime = lsp->l_shm_atime;
	bsp->shm_dtime = lsp->l_shm_dtime;
	bsp->shm_ctime = lsp->l_shm_ctime;
	bsp->_shm_internal = (void*)lsp->l___unused5;	/* XXX Oh well. */
}

void
bsd_to_linux_shmid_ds(struct shmid_ds *bsp, struct linux_shmid_ds *lsp)
{

	bsd_to_linux_ipc_perm(&bsp->shm_perm, &lsp->l_shm_perm);
	lsp->l_shm_segsz = bsp->shm_segsz;
	lsp->l_shm_lpid = bsp->shm_lpid;
	lsp->l_shm_cpid = bsp->shm_cpid;
	lsp->l_shm_nattch = bsp->shm_nattch;
	lsp->l_shm_atime = bsp->shm_atime;
	lsp->l_shm_dtime = bsp->shm_dtime;
	lsp->l_shm_ctime = bsp->shm_ctime;
	lsp->l_private2 = bsp->_shm_internal;	/* XXX */
}

void
bsd_to_linux_shmid64_ds(struct shmid_ds *bsp, struct linux_shmid64_ds *lsp)
{
	bsd_to_linux_ipc64_perm(&bsp->shm_perm, &lsp->l_shm_perm);
	lsp->l_shm_segsz = bsp->shm_segsz;
	lsp->l_shm_lpid = bsp->shm_lpid;
	lsp->l_shm_cpid = bsp->shm_cpid;
	lsp->l_shm_nattch = bsp->shm_nattch;
	lsp->l_shm_atime = bsp->shm_atime;
	lsp->l_shm_dtime = bsp->shm_dtime;
	lsp->l_shm_ctime = bsp->shm_ctime;
	lsp->l___unused5 = (u_long)bsp->_shm_internal;	/* XXX */
}

/*
 * shmctl.SHM_LOCK and SHM_UNLOCK are passed on, but currently not implemented
 * by NetBSD itself.
 *
 * The usual structure conversion and massaging is done.
 */
int
linux_sys_shmctl(struct lwp *l, const struct linux_sys_shmctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) shmid;
		syscallarg(int) cmd;
		syscallarg(struct linux_shmid_ds *) buf;
	} */
	struct shmid_ds bs;
	struct linux_shmid_ds ls;
	struct linux_shmid64_ds ls64;
	struct linux_shminfo64 lsi64;
	struct linux_shm_info lsi;
	int error, i, cmd;

	cmd = SCARG(uap, cmd);
	switch (cmd) {
	case LINUX_IPC_STAT:
	case LINUX_SHM_STAT:
		error = shmctl1(l, SCARG(uap, shmid), IPC_STAT, &bs);
		if (error != 0)
			return error;
		bsd_to_linux_shmid_ds(&bs, &ls);
		if (cmd == LINUX_SHM_STAT)
			retval[0] = IXSEQ_TO_IPCID(bs.shm_perm._key,
			    bs.shm_perm);
		return copyout(&ls, SCARG(uap, buf), sizeof ls);

	case LINUX_IPC_STAT | LINUX_IPC_64:
	case LINUX_SHM_STAT | LINUX_IPC_64:
		error = shmctl1(l, SCARG(uap, shmid), IPC_STAT, &bs);
		if (error != 0)
			return error;
		bsd_to_linux_shmid64_ds(&bs, &ls64);
		if (cmd == (LINUX_SHM_STAT | LINUX_IPC_64)) {
			retval[0] = IXSEQ_TO_IPCID(bs.shm_perm._key,
						   bs.shm_perm);
		}
		return copyout(&ls64, SCARG(uap, buf), sizeof ls64);

	case LINUX_IPC_SET:
		if ((error = copyin(SCARG(uap, buf), &ls, sizeof ls)))
			return error;
		linux_to_bsd_shmid_ds(&ls, &bs);
		return shmctl1(l, SCARG(uap, shmid), IPC_SET, &bs);

	case LINUX_IPC_RMID:
		return shmctl1(l, SCARG(uap, shmid), IPC_RMID, NULL);

	case LINUX_SHM_LOCK:
		return shmctl1(l, SCARG(uap, shmid), SHM_LOCK, NULL);

	case LINUX_SHM_UNLOCK:
		return shmctl1(l, SCARG(uap, shmid), SHM_UNLOCK, NULL);

	case LINUX_IPC_INFO:
		memset(&lsi64, 0, sizeof lsi64);
		lsi64.l_shmmax = shminfo.shmmax;
		lsi64.l_shmmin = shminfo.shmmin;
		lsi64.l_shmmni = shminfo.shmmni;
		lsi64.l_shmseg = shminfo.shmseg;
		lsi64.l_shmall = shminfo.shmall;
		return copyout(&lsi64, SCARG(uap, buf), sizeof lsi64);

	case LINUX_SHM_INFO:
		(void)memset(&lsi, 0, sizeof lsi);
		lsi.l_used_ids = shm_nused;
		for (i = 0; i < shminfo.shmmni; i++)
			if (shmsegs[i].shm_perm.mode & SHMSEG_ALLOCATED)
				lsi.l_shm_tot += shmsegs[i].shm_segsz;
		lsi.l_shm_rss = 0;
		lsi.l_shm_swp = 0;
		lsi.l_swap_attempts = 0;
		lsi.l_swap_successes = 0;
		return copyout(&lsi, SCARG(uap, buf), sizeof lsi);

	default:
#ifdef DEBUG
		printf("linux_sys_shmctl cmd %d\n", SCARG(uap, cmd));
#endif
		return EINVAL;
	}
}
#endif /* SYSVSHM */
