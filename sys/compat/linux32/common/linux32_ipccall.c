/* $NetBSD: linux32_ipccall.c,v 1.2.8.1 2009/05/13 17:18:59 jym Exp $ */

/*
 * Copyright (c) 2008 Nicolas Joly
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
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
__KERNEL_RCSID(0, "$NetBSD: linux32_ipccall.c,v 1.2.8.1 2009/05/13 17:18:59 jym Exp $");

#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#endif

#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/linux32_syscallargs.h>
#include <compat/linux32/common/linux32_ipc.h>
#include <compat/linux32/common/linux32_sem.h>
#include <compat/linux32/common/linux32_shm.h>

#define LINUX32_IPC_semop         1
#define LINUX32_IPC_semget        2
#define LINUX32_IPC_semctl        3
#define LINUX32_IPC_msgsnd        11
#define LINUX32_IPC_msgrcv        12
#define LINUX32_IPC_msgget        13
#define LINUX32_IPC_msgctl        14
#define LINUX32_IPC_shmat         21
#define LINUX32_IPC_shmdt         22
#define LINUX32_IPC_shmget        23
#define LINUX32_IPC_shmctl        24

#ifdef SYSVSEM
static void
bsd_to_linux32_semid_ds(struct semid_ds *, struct linux32_semid_ds *);
static void
bsd_to_linux32_semid64_ds(struct semid_ds *, struct linux32_semid64_ds *);
static void
linux32_to_bsd_semid_ds(struct linux32_semid_ds *, struct semid_ds *);
static void
linux32_to_bsd_semid64_ds(struct linux32_semid64_ds *, struct semid_ds *);

static int
linux32_semop(struct lwp *, const struct linux32_sys_ipc_args *, register_t *);
static int
linux32_semget(struct lwp *, const struct linux32_sys_ipc_args *, register_t *);
static int
linux32_semctl(struct lwp *, const struct linux32_sys_ipc_args *, register_t *);
#endif /* SYSVSEM */

#ifdef SYSVSHM
static void
bsd_to_linux32_shmid_ds(struct shmid_ds *, struct linux32_shmid_ds *);
static void
linux32_to_bsd_shmid_ds(struct linux32_shmid_ds *, struct shmid_ds *);
static void
bsd_to_linux32_shmid64_ds(struct shmid_ds *, struct linux32_shmid64_ds *);
static void
linux32_to_bsd_shmid64_ds(struct linux32_shmid64_ds *, struct shmid_ds *);

static int
linux32_shmat(struct lwp *, const struct linux32_sys_ipc_args *, register_t *);
static int
linux32_shmdt(struct lwp *, const struct linux32_sys_ipc_args *, register_t *);
static int
linux32_shmget(struct lwp *, const struct linux32_sys_ipc_args *, register_t *);
static int
linux32_shmctl(struct lwp *, const struct linux32_sys_ipc_args *, register_t *);
#endif /* SYSVSHM */

int
linux32_sys_ipc(struct lwp *l, const struct linux32_sys_ipc_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(netbsd32_voidp) ptr;
	} */

	switch (SCARG(uap, what)) {
#ifdef SYSVSEM
	case LINUX32_IPC_semop:
		return linux32_semop(l, uap, retval);;
	case LINUX32_IPC_semget:
		return linux32_semget(l, uap, retval);
	case LINUX32_IPC_semctl:
		return linux32_semctl(l, uap, retval);
#endif /* SYSVSEM */
#ifdef SYSVSHM
	case LINUX32_IPC_shmat:
		return linux32_shmat(l, uap, retval);
	case LINUX32_IPC_shmdt:
		return linux32_shmdt(l, uap, retval);
	case LINUX32_IPC_shmget:
		return linux32_shmget(l, uap, retval);
	case LINUX32_IPC_shmctl:
		return linux32_shmctl(l, uap, retval);
#endif /* SYSVSHM */
	default:
		return ENOSYS;
	}

}

static void
bsd_to_linux32_ipc_perm(struct ipc_perm *bpp, struct linux32_ipc_perm *lpp)
{
	lpp->l_key = bpp->_key;
	lpp->l_uid = bpp->uid;
	lpp->l_gid = bpp->gid; 
	lpp->l_cuid = bpp->cuid;
	lpp->l_cgid = bpp->cgid;
	lpp->l_mode = bpp->mode; 
	lpp->l_seq = bpp->_seq; 
}

static void
linux32_to_bsd_ipc_perm(struct linux32_ipc_perm *lpp, struct ipc_perm *bpp)
{
	bpp->_key = lpp->l_key;
	bpp->uid = lpp->l_uid;
	bpp->gid = lpp->l_gid; 
	bpp->cuid = lpp->l_cuid;
	bpp->cgid = lpp->l_cgid;
	bpp->mode = lpp->l_mode; 
	bpp->_seq = lpp->l_seq; 
}

static void
bsd_to_linux32_ipc64_perm(struct ipc_perm *bpp, struct linux32_ipc64_perm *lpp)
{
	lpp->l_key = bpp->_key;
	lpp->l_uid = bpp->uid;
	lpp->l_gid = bpp->gid;
	lpp->l_cuid = bpp->cuid;
	lpp->l_cgid = bpp->cgid;
	lpp->l_mode = bpp->mode;
	lpp->l_seq = bpp->_seq;
}

static void
linux32_to_bsd_ipc64_perm(struct linux32_ipc64_perm *lpp, struct ipc_perm *bpp)
{
	bpp->_key = lpp->l_key;
	bpp->uid = lpp->l_uid;
	bpp->gid = lpp->l_gid;
	bpp->cuid = lpp->l_cuid; 
	bpp->cgid = lpp->l_cgid;
	bpp->mode = lpp->l_mode;
	bpp->_seq = lpp->l_seq;
}

#ifdef SYSVSEM
static void
bsd_to_linux32_semid_ds(struct semid_ds *bsp, struct linux32_semid_ds *lsp)
{
	bsd_to_linux32_ipc_perm(&bsp->sem_perm, &lsp->l_sem_perm);
	lsp->l_sem_otime = bsp->sem_otime;
	lsp->l_sem_ctime = bsp->sem_ctime;
	lsp->l_sem_nsems = bsp->sem_nsems;
	NETBSD32PTR32(lsp->l_sem_base, bsp->_sem_base);
}

static void
bsd_to_linux32_semid64_ds(struct semid_ds *bsp, struct linux32_semid64_ds *lsp)
{
	bsd_to_linux32_ipc64_perm(&bsp->sem_perm, &lsp->l_sem_perm);
	lsp->l_sem_otime = bsp->sem_otime;
	lsp->l_sem_ctime = bsp->sem_ctime;
	lsp->l_sem_nsems = bsp->sem_nsems;
}

static void
linux32_to_bsd_semid_ds(struct linux32_semid_ds *lsp, struct semid_ds *bsp)
{
	linux32_to_bsd_ipc_perm(&lsp->l_sem_perm, &bsp->sem_perm);
	bsp->sem_otime = lsp->l_sem_otime;
	bsp->sem_ctime = lsp->l_sem_ctime;
	bsp->sem_nsems = lsp->l_sem_nsems;
	bsp->_sem_base = NETBSD32PTR64(lsp->l_sem_base);
}

static void
linux32_to_bsd_semid64_ds(struct linux32_semid64_ds *lsp, struct semid_ds *bsp)
{
	linux32_to_bsd_ipc64_perm(&lsp->l_sem_perm, &bsp->sem_perm);
	bsp->sem_otime = lsp->l_sem_otime;
	bsp->sem_ctime = lsp->l_sem_ctime;
	bsp->sem_nsems = lsp->l_sem_nsems;
}

static int
linux32_semop(struct lwp *l, const struct linux32_sys_ipc_args *uap,
    register_t *retval)
{
	struct sys_semop_args ua;

	SCARG(&ua, semid) = SCARG(uap, a1);
	SCARG(&ua, sops) = SCARG_P32(uap, ptr);
	SCARG(&ua, nsops) = SCARG(uap, a2);

	return sys_semop(l, &ua, retval);
}

static int
linux32_semget(struct lwp *l, const struct linux32_sys_ipc_args *uap,
    register_t *retval)
{
	struct sys_semget_args ua;

	SCARG(&ua, key) = SCARG(uap, a1);
	SCARG(&ua, nsems) = SCARG(uap, a2);
	SCARG(&ua, semflg) = SCARG(uap, a3);

	return sys_semget(l, &ua, retval);
}

static int
linux32_semctl(struct lwp *l, const struct linux32_sys_ipc_args *uap,
    register_t *retval)
{
	int lcmd, cmd, error;
	struct semid_ds bs;
	struct linux32_semid_ds ls;
	struct linux32_semid64_ds ls64;
	union linux32_semun lsem;
	union __semun bsem;
	void *buf = NULL;

	if ((error = copyin(SCARG_P32(uap, ptr), &lsem, sizeof lsem)))
		return error;

	lcmd = SCARG(uap, a3);

	switch (lcmd & ~LINUX32_IPC_64) {
	case LINUX32_IPC_RMID:
		cmd = IPC_RMID;
		break;
	case LINUX32_IPC_STAT:
		cmd = IPC_STAT;
		buf = &bs;
		break;
	case LINUX32_IPC_SET:
		if (lcmd & LINUX32_IPC_64) {
			error = copyin(NETBSD32PTR64(lsem.l_buf), &ls64,
			    sizeof ls64);
			linux32_to_bsd_semid64_ds(&ls64, &bs);
		} else {
			error = copyin(NETBSD32PTR64(lsem.l_buf), &ls,
			    sizeof ls);
			linux32_to_bsd_semid_ds(&ls, &bs);
		}
		if (error)
			return error;
		cmd = IPC_SET;
		buf = &bs;
		break;
	case LINUX32_GETVAL:
		cmd = GETVAL;
		break;
	case LINUX32_SETVAL:
		cmd = SETVAL;
		bsem.val = lsem.l_val;
		buf = &bsem;
		break;
	case LINUX32_GETPID:
		cmd = GETPID;
		break;
	case LINUX32_GETNCNT:
		cmd = GETNCNT;
		break;
	case LINUX32_GETZCNT:
		cmd = GETZCNT;
		break;
	case LINUX32_GETALL:
		cmd = GETALL;
		bsem.array = NETBSD32PTR64(lsem.l_array);
		buf = &bsem;
		break;
	case LINUX32_SETALL:
		cmd = SETALL;
		bsem.array = NETBSD32PTR64(lsem.l_array);
		buf = &bsem;
		break;
	default:
		return EINVAL;
	}

	error = semctl1(l, SCARG(uap, a1), SCARG(uap, a2), cmd, buf, retval);
	if (error)
		return error;

	switch (lcmd) {
	case LINUX32_IPC_STAT:
		bsd_to_linux32_semid_ds(&bs, &ls);
		error = copyout(&ls, NETBSD32PTR64(lsem.l_buf), sizeof ls);
		break;
	case LINUX32_IPC_STAT|LINUX32_IPC_64:
		bsd_to_linux32_semid64_ds(&bs, &ls64);
		error = copyout(&ls64, NETBSD32PTR64(lsem.l_buf), sizeof ls64);
		break;
	default:
		break;
	}

	return error;
}
#endif /* SYSVSEM */

#ifdef SYSVSHM
static void
bsd_to_linux32_shmid_ds(struct shmid_ds *bsp, struct linux32_shmid_ds *lsp)
{
	bsd_to_linux32_ipc_perm(&bsp->shm_perm, &lsp->l_shm_perm);
	lsp->l_shm_segsz = bsp->shm_segsz;
	lsp->l_shm_atime = bsp->shm_atime;
	lsp->l_shm_dtime = bsp->shm_dtime;
	lsp->l_shm_ctime = bsp->shm_ctime;
	lsp->l_shm_cpid = bsp->shm_cpid;
	lsp->l_shm_lpid = bsp->shm_lpid;
	lsp->l_shm_nattch = bsp->shm_nattch;
	NETBSD32PTR32(lsp->l_private2, bsp->_shm_internal);
}

static void
linux32_to_bsd_shmid_ds(struct linux32_shmid_ds *lsp, struct shmid_ds *bsp)
{
	linux32_to_bsd_ipc_perm(&lsp->l_shm_perm, &bsp->shm_perm);
	bsp->shm_segsz = lsp->l_shm_segsz;
	bsp->shm_atime = lsp->l_shm_atime;
	bsp->shm_dtime = lsp->l_shm_dtime;
	bsp->shm_ctime = lsp->l_shm_ctime;
	bsp->shm_cpid = lsp->l_shm_cpid;
	bsp->shm_lpid = lsp->l_shm_lpid;
	bsp->shm_nattch = lsp->l_shm_nattch;
	bsp->_shm_internal = NETBSD32PTR64(lsp->l_private2);
}

static void
bsd_to_linux32_shmid64_ds(struct shmid_ds *bsp, struct linux32_shmid64_ds *lsp)
{
	bsd_to_linux32_ipc64_perm(&bsp->shm_perm, &lsp->l_shm_perm);
	lsp->l_shm_segsz = bsp->shm_segsz;
	lsp->l_shm_atime = bsp->shm_atime;
	lsp->l_shm_dtime = bsp->shm_dtime;
	lsp->l_shm_ctime = bsp->shm_ctime;
	lsp->l_shm_cpid = bsp->shm_cpid;
	lsp->l_shm_lpid = bsp->shm_lpid;
	lsp->l_shm_nattch = bsp->shm_nattch;
	lsp->l___unused5 = NETBSD32PTR32I(bsp->_shm_internal);
}

static void
linux32_to_bsd_shmid64_ds(struct linux32_shmid64_ds *lsp, struct shmid_ds *bsp)
{
	linux32_to_bsd_ipc64_perm(&lsp->l_shm_perm, &bsp->shm_perm);
	bsp->shm_segsz = lsp->l_shm_segsz; 
	bsp->shm_atime = lsp->l_shm_atime;
	bsp->shm_dtime = lsp->l_shm_dtime;
	bsp->shm_ctime = lsp->l_shm_ctime;
	bsp->shm_cpid = lsp->l_shm_cpid;
	bsp->shm_lpid = lsp->l_shm_lpid; 
	bsp->shm_nattch = lsp->l_shm_nattch;
	bsp->_shm_internal = NETBSD32IPTR64(lsp->l___unused5);
}

static int
linux32_shmat(struct lwp *l, const struct linux32_sys_ipc_args *uap,
    register_t *retval)
{
	struct sys_shmat_args ua;
	netbsd32_pointer_t addr32;
	int error;

	SCARG(&ua, shmid) = SCARG(uap, a1);
	SCARG(&ua, shmaddr) = SCARG_P32(uap, ptr);
	SCARG(&ua, shmflg) = SCARG(uap, a2);

	if ((error = sys_shmat(l, &ua, retval)))
		return error;

	NETBSD32PTR32(addr32, (const void *)(uintptr_t)retval[0]);

	error = copyout(&addr32, NETBSD32IPTR64(SCARG(uap, a3)), sizeof addr32);
	if (error == 0)
		retval[0] = 0;

	return error;
}

static int
linux32_shmdt(struct lwp *l, const struct linux32_sys_ipc_args *uap,
    register_t *retval)
{
	struct sys_shmdt_args ua;

	SCARG(&ua, shmaddr) = SCARG_P32(uap, ptr);

	return sys_shmdt(l, &ua, retval);
}

static int
linux32_shmget(struct lwp *l, const struct linux32_sys_ipc_args *uap,
    register_t *retval)
{
	struct sys_shmget_args ua;

	SCARG(&ua, key) = SCARG(uap, a1);
	SCARG(&ua, size) = SCARG(uap, a2);
	SCARG(&ua, shmflg) = SCARG(uap, a3) | _SHM_RMLINGER;

	return sys_shmget(l, &ua, retval);
}

static int
linux32_shmctl(struct lwp *l, const struct linux32_sys_ipc_args *uap,
    register_t *retval)
{
	int shmid, cmd, error;
	struct shmid_ds bs;
	struct linux32_shmid_ds ls;
	struct linux32_shmid64_ds ls64;

	shmid = SCARG(uap, a1);
	cmd = SCARG(uap, a2);

	switch (cmd & ~LINUX32_IPC_64) {

	case LINUX32_SHM_STAT:
		return ENOSYS;

	case LINUX32_IPC_STAT:
		error = shmctl1(l, shmid, IPC_STAT, &bs);
		if (error != 0)
			return error;
		if (cmd & LINUX32_IPC_64) {
			bsd_to_linux32_shmid64_ds(&bs, &ls64);
			error = copyout(&ls64, SCARG_P32(uap, ptr), sizeof ls64);
		} else {
			bsd_to_linux32_shmid_ds(&bs, &ls);
			error = copyout(&ls, SCARG_P32(uap, ptr), sizeof ls);
		}
		return error;

	case LINUX32_IPC_SET:
		if (cmd & LINUX32_IPC_64) {
			error = copyin(SCARG_P32(uap, ptr), &ls64, sizeof ls64);
			linux32_to_bsd_shmid64_ds(&ls64, &bs);
		} else {
			error = copyin(SCARG_P32(uap, ptr), &ls, sizeof ls);
			linux32_to_bsd_shmid_ds(&ls, &bs);
		}
		if (error != 0)
			return error;
		return shmctl1(l, shmid, IPC_SET, &bs);

	case LINUX32_IPC_RMID:
		return shmctl1(l, shmid, IPC_RMID, NULL);

	case LINUX32_SHM_LOCK:
		return shmctl1(l, shmid, SHM_LOCK, NULL);

	case LINUX32_SHM_UNLOCK:
		return shmctl1(l, shmid, SHM_UNLOCK, NULL);

	case LINUX32_IPC_INFO:
	case LINUX32_SHM_INFO:
		return ENOSYS;

	default:
		return EINVAL;
	}
}               
#endif /* SYSVSHM */
