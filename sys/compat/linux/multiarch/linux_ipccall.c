/*	$NetBSD: linux_ipccall.c,v 1.13 1998/01/22 16:33:57 mycroft Exp $	*/

/*
 * Copyright (c) 1995 Frank van der Linden
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project
 *      by Frank van der Linden
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/systm.h>
#include <sys/stat.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <compat/linux/linux_types.h>
#include <compat/linux/linux_signal.h>
#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_util.h>
#include <compat/linux/linux_ipc.h>
#include <compat/linux/linux_msg.h>
#include <compat/linux/linux_shm.h>
#include <compat/linux/linux_sem.h>
#include <compat/linux/linux_ipccall.h>

/*
 * Stuff to deal with the SysV ipc/shm/semaphore interface in Linux.
 * The main difference is, that Linux handles it all via one
 * system call, which has the usual maximum amount of 5 arguments.
 * This results in a kludge for calls that take 6 of them.
 *
 * The SYSVXXXX options have to be enabled to get the appropriate
 * functions to work.
 */

#ifdef SYSVSEM
static int linux_semop __P((struct proc *, struct linux_sys_ipc_args *,
				register_t *));
static int linux_semget __P((struct proc *, struct linux_sys_ipc_args *,
				register_t *));
static int linux_semctl __P((struct proc *, struct linux_sys_ipc_args *,
				register_t *));
static void bsd_to_linux_semid_ds __P((struct semid_ds *,
				       struct linux_semid_ds *));
static void linux_to_bsd_semid_ds __P((struct linux_semid_ds *,
				       struct semid_ds *));
#endif

#ifdef SYSVMSG
static int linux_msgsnd __P((struct proc *, struct linux_sys_ipc_args *,
				register_t *));
static int linux_msgrcv __P((struct proc *, struct linux_sys_ipc_args *,
				register_t *));
static int linux_msgget __P((struct proc *, struct linux_sys_ipc_args *,
				register_t *));
static int linux_msgctl __P((struct proc *, struct linux_sys_ipc_args *,
				register_t *));
static void linux_to_bsd_msqid_ds __P((struct linux_msqid_ds *,
				       struct msqid_ds *));
static void bsd_to_linux_msqid_ds __P((struct msqid_ds *,
				       struct linux_msqid_ds *));
#endif

#ifdef SYSVSHM
static int linux_shmat __P((struct proc *, struct linux_sys_ipc_args *,
				register_t *));
static int linux_shmdt __P((struct proc *, struct linux_sys_ipc_args *,
				register_t *));
static int linux_shmget __P((struct proc *, struct linux_sys_ipc_args *,
				register_t *));
static int linux_shmctl __P((struct proc *, struct linux_sys_ipc_args *,
				register_t *));
static void linux_to_bsd_shmid_ds __P((struct linux_shmid_ds *,
				       struct shmid_ds *));
static void bsd_to_linux_shmid_ds __P((struct shmid_ds *,
				       struct linux_shmid_ds *));
#endif


#if defined (SYSVSEM) || defined(SYSVSHM) || defined(SYSVMSG)
static void linux_to_bsd_ipc_perm __P((struct linux_ipc_perm *,
				       struct ipc_perm *));
static void bsd_to_linux_ipc_perm __P((struct ipc_perm *,
				       struct linux_ipc_perm *));
#endif

int
linux_sys_ipc(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap = v;

	switch (SCARG(uap, what)) {
#ifdef SYSVSEM
	case LINUX_SYS_semop:
		return linux_semop(p, uap, retval);
	case LINUX_SYS_semget:
		return linux_semget(p, uap, retval);
	case LINUX_SYS_semctl:
		return linux_semctl(p, uap, retval);
#endif
#ifdef SYSVMSG
	case LINUX_SYS_msgsnd:
		return linux_msgsnd(p, uap, retval);
	case LINUX_SYS_msgrcv:
		return linux_msgrcv(p, uap, retval);
	case LINUX_SYS_msgget:
		return linux_msgget(p, uap, retval);
	case LINUX_SYS_msgctl:
		return linux_msgctl(p, uap, retval);
#endif
#ifdef SYSVSHM
	case LINUX_SYS_shmat:
		return linux_shmat(p, uap, retval);
	case LINUX_SYS_shmdt:
		return linux_shmdt(p, uap, retval);
	case LINUX_SYS_shmget:
		return linux_shmget(p, uap, retval);
	case LINUX_SYS_shmctl:
		return linux_shmctl(p, uap, retval);
#endif
	default:
		return ENOSYS;
	}
}

#if defined (SYSVSEM) || defined(SYSVSHM) || defined(SYSVMSG)
/*
 * Convert between Linux and NetBSD ipc_perm structures. Only the
 * order of the fields is different.
 */
static void
linux_to_bsd_ipc_perm(lpp, bpp)
	struct linux_ipc_perm *lpp;
	struct ipc_perm *bpp;
{

	bpp->key = lpp->l_key;
	bpp->uid = lpp->l_uid;
	bpp->gid = lpp->l_gid;
	bpp->cuid = lpp->l_cuid;
	bpp->cgid = lpp->l_cgid;
	bpp->mode = lpp->l_mode;
	bpp->seq = lpp->l_seq;
}

static void
bsd_to_linux_ipc_perm(bpp, lpp)
	struct ipc_perm *bpp;
	struct linux_ipc_perm *lpp;
{

	lpp->l_key = bpp->key;
	lpp->l_uid = bpp->uid;
	lpp->l_gid = bpp->gid;
	lpp->l_cuid = bpp->cuid;
	lpp->l_cgid = bpp->cgid;
	lpp->l_mode = bpp->mode;
	lpp->l_seq = bpp->seq;
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
static void
bsd_to_linux_semid_ds(bs, ls)
	struct semid_ds *bs;
	struct linux_semid_ds *ls;
{

	bsd_to_linux_ipc_perm(&bs->sem_perm, &ls->l_sem_perm);
	ls->l_sem_otime = bs->sem_otime;
	ls->l_sem_ctime = bs->sem_ctime;
	ls->l_sem_nsems = bs->sem_nsems;
	ls->l_sem_base = bs->sem_base;
}

static void
linux_to_bsd_semid_ds(ls, bs)
	struct linux_semid_ds *ls;
	struct semid_ds *bs;
{

	linux_to_bsd_ipc_perm(&ls->l_sem_perm, &bs->sem_perm);
	bs->sem_otime = ls->l_sem_otime;
	bs->sem_ctime = ls->l_sem_ctime;
	bs->sem_nsems = ls->l_sem_nsems;
	bs->sem_base = ls->l_sem_base;
}

int
linux_semop(p, uap, retval)
	struct proc *p;
	struct linux_sys_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	struct sys_semop_args bsa;

	SCARG(&bsa, semid) = SCARG(uap, a1);
	SCARG(&bsa, sops) = (struct sembuf *)SCARG(uap, ptr);
	SCARG(&bsa, nsops) = SCARG(uap, a2);

	return sys_semop(p, &bsa, retval);
}

int
linux_semget(p, uap, retval)
	struct proc *p;
	struct linux_sys_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	struct sys_semget_args bsa;

	SCARG(&bsa, key) = (key_t)SCARG(uap, a1);
	SCARG(&bsa, nsems) = SCARG(uap, a2);
	SCARG(&bsa, semflg) = SCARG(uap, a3);

	return sys_semget(p, &bsa, retval);
}

/*
 * Most of this can be handled by directly passing the arguments on,
 * buf IPC_* require a lot of copy{in,out} because of the extra indirection
 * (we are passed a pointer to a union cointaining a pointer to a semid_ds
 * structure.
 */
int
linux_semctl(p, uap, retval)
	struct proc *p;
	struct linux_sys_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	caddr_t sg;
	struct sys___semctl_args nua;
	struct semid_ds *bmp, bm;
	struct linux_semid_ds lm;
	union semun *bup;
	union linux_semun lu;
	int error;

	SCARG(&nua, semid) = SCARG(uap, a1);
	SCARG(&nua, semnum) = SCARG(uap, a2);
	SCARG(&nua, arg) = (union semun *)SCARG(uap, ptr);
	switch (SCARG(uap, a3)) {
	case LINUX_IPC_STAT:
		sg = stackgap_init(p->p_emul);
		bup = stackgap_alloc(&sg, sizeof (union semun));
		bmp = stackgap_alloc(&sg, sizeof (struct semid_ds));
		if ((error = copyout(&bmp, bup, sizeof bmp)))
			return error;
		SCARG(&nua, cmd) = IPC_STAT;
		SCARG(&nua, arg) = bup;
		if ((error = sys___semctl(p, &nua, retval)))
			return error;
		if ((error = copyin(bmp, &bm, sizeof bm)))
			return error;
		bsd_to_linux_semid_ds(&bm, &lm);
		if ((error = copyin(SCARG(uap, ptr), &lu, sizeof lu)))
			return error;
		return copyout(&lm, lu.l_buf, sizeof lm);
	case LINUX_IPC_SET:
		if ((error = copyin(SCARG(uap, ptr), &lu, sizeof lu)))
			return error;
		if ((error = copyin(lu.l_buf, &lm, sizeof lm)))
			return error;
		linux_to_bsd_semid_ds(&lm, &bm);
		sg = stackgap_init(p->p_emul);
		bup = stackgap_alloc(&sg, sizeof (union semun));
		bmp = stackgap_alloc(&sg, sizeof (struct semid_ds));
		if ((error = copyout(&bm, bmp, sizeof bm)))
			return error;
		if ((error = copyout(&bmp, bup, sizeof bmp)))
			return error;
		SCARG(&nua, cmd) = IPC_SET;
		SCARG(&nua, arg) = bup;
		return sys___semctl(p, &nua, retval);
	case LINUX_IPC_RMID:
		SCARG(&nua, cmd) = IPC_RMID;
		break;
	case LINUX_GETVAL:
		SCARG(&nua, cmd) = GETVAL;
		break;
	case LINUX_GETPID:
		SCARG(&nua, cmd) = GETPID;
		break;
	case LINUX_GETNCNT:
		SCARG(&nua, cmd) = GETNCNT;
		break;
	case LINUX_GETZCNT:
		SCARG(&nua, cmd) = GETZCNT;
		break;
	case LINUX_SETVAL:
		SCARG(&nua, cmd) = SETVAL;
		break;
	default:
		return EINVAL;
	}
	return sys___semctl(p, &nua, retval);
}
#endif /* SYSVSEM */

#ifdef SYSVMSG

static void
linux_to_bsd_msqid_ds(lmp, bmp)
	struct linux_msqid_ds *lmp;
	struct msqid_ds *bmp;
{

	linux_to_bsd_ipc_perm(&lmp->l_msg_perm, &bmp->msg_perm);
	bmp->msg_first = lmp->l_msg_first;
	bmp->msg_last = lmp->l_msg_last;
	bmp->msg_cbytes = lmp->l_msg_cbytes;
	bmp->msg_qnum = lmp->l_msg_qnum;
	bmp->msg_qbytes = lmp->l_msg_qbytes;
	bmp->msg_lspid = lmp->l_msg_lspid;
	bmp->msg_lrpid = lmp->l_msg_lrpid;
	bmp->msg_stime = lmp->l_msg_stime;
	bmp->msg_rtime = lmp->l_msg_rtime;
	bmp->msg_ctime = lmp->l_msg_ctime;
}

static void
bsd_to_linux_msqid_ds(bmp, lmp)
	struct msqid_ds *bmp;
	struct linux_msqid_ds *lmp;
{

	bsd_to_linux_ipc_perm(&bmp->msg_perm, &lmp->l_msg_perm);
	lmp->l_msg_first = bmp->msg_first;
	lmp->l_msg_last = bmp->msg_last;
	lmp->l_msg_cbytes = bmp->msg_cbytes;
	lmp->l_msg_qnum = bmp->msg_qnum;
	lmp->l_msg_qbytes = bmp->msg_qbytes;
	lmp->l_msg_lspid = bmp->msg_lspid;
	lmp->l_msg_lrpid = bmp->msg_lrpid;
	lmp->l_msg_stime = bmp->msg_stime;
	lmp->l_msg_rtime = bmp->msg_rtime;
	lmp->l_msg_ctime = bmp->msg_ctime;
}

static int
linux_msgsnd(p, uap, retval)
	struct proc *p;
	struct linux_sys_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	struct sys_msgsnd_args bma;

	SCARG(&bma, msqid) = SCARG(uap, a1);
	SCARG(&bma, msgp) = SCARG(uap, ptr);
	SCARG(&bma, msgsz) = SCARG(uap, a2);
	SCARG(&bma, msgflg) = SCARG(uap, a3);

	return sys_msgsnd(p, &bma, retval);
}

static int
linux_msgrcv(p, uap, retval)
	struct proc *p;
	struct linux_sys_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
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

	return sys_msgrcv(p, &bma, retval);
}

static int
linux_msgget(p, uap, retval)
	struct proc *p;
	struct linux_sys_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	struct sys_msgget_args bma;

	SCARG(&bma, key) = (key_t)SCARG(uap, a1);
	SCARG(&bma, msgflg) = SCARG(uap, a2);

	return sys_msgget(p, &bma, retval);
}

static int
linux_msgctl(p, uap, retval)
	struct proc *p;
	struct linux_sys_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	caddr_t sg;
	struct sys_msgctl_args nua;
	struct msqid_ds *bmp, bm;
	struct linux_msqid_ds lm;
	int error;

	SCARG(&nua, msqid) = SCARG(uap, a1);
	switch (SCARG(uap, a2)) {
	case LINUX_IPC_STAT:
		sg = stackgap_init(p->p_emul);
		bmp = stackgap_alloc(&sg, sizeof (struct msqid_ds));
		SCARG(&nua, cmd) = IPC_STAT;
		SCARG(&nua, buf) = bmp;
		if ((error = sys_msgctl(p, &nua, retval)))
			return error;
		if ((error = copyin(bmp, &bm, sizeof bm)))
			return error;
		bsd_to_linux_msqid_ds(&bm, &lm);
		return copyout(&lm, SCARG(uap, ptr), sizeof lm);
	case LINUX_IPC_SET:
		if ((error = copyin(SCARG(uap, ptr), &lm, sizeof lm)))
			return error;
		linux_to_bsd_msqid_ds(&lm, &bm);
		sg = stackgap_init(p->p_emul);
		bmp = stackgap_alloc(&sg, sizeof bm);
		if ((error = copyout(&bm, bmp, sizeof bm)))
			return error;
		SCARG(&nua, cmd) = IPC_SET;
		SCARG(&nua, buf) = bmp;
		return sys_msgctl(p, &nua, retval);
	case LINUX_IPC_RMID:
		SCARG(&nua, cmd) = IPC_RMID;
		SCARG(&nua, buf) = NULL;
		break;
	default:
		return EINVAL;
	}
	return sys_msgctl(p, &nua, retval);
}
#endif /* SYSVMSG */

#ifdef SYSVSHM
/*
 * shmat(2). Very straightforward, except that Linux passes a pointer
 * in which the return value is to be passed. This is subsequently
 * handled by libc, apparently.
 */
static int
linux_shmat(p, uap, retval)
	struct proc *p;
	struct linux_sys_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	struct sys_shmat_args bsa;
	int error;

	SCARG(&bsa, shmid) = SCARG(uap, a1);
	SCARG(&bsa, shmaddr) = SCARG(uap, ptr);
	SCARG(&bsa, shmflg) = SCARG(uap, a2);

	if ((error = sys_shmat(p, &bsa, retval)))
		return error;

	if ((error = copyout(&retval[0], (caddr_t) SCARG(uap, a3),
	     sizeof retval[0])))
		return error;

	retval[0] = 0;
	return 0;
}

/*
 * shmdt(): this could have been mapped directly, if it wasn't for
 * the extra indirection by the linux_ipc system call.
 */
static int
linux_shmdt(p, uap, retval)
	struct proc *p;
	struct linux_sys_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	struct sys_shmdt_args bsa;

	SCARG(&bsa, shmaddr) = SCARG(uap, ptr);

	return sys_shmdt(p, &bsa, retval);
}

/*
 * Same story as shmdt.
 */
static int
linux_shmget(p, uap, retval)
	struct proc *p;
	struct linux_sys_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	struct sys_shmget_args bsa;

	SCARG(&bsa, key) = SCARG(uap, a1);
	SCARG(&bsa, size) = SCARG(uap, a2);
	SCARG(&bsa, shmflg) = SCARG(uap, a3);

	return sys_shmget(p, &bsa, retval);
}

/*
 * Convert between Linux and NetBSD shmid_ds structures.
 * The order of the fields is once again the difference, and
 * we also need a place to store the internal data pointer
 * in, which is unfortunately stored in this structure.
 *
 * We abuse a Linux internal field for that.
 */
static void
linux_to_bsd_shmid_ds(lsp, bsp)
	struct linux_shmid_ds *lsp;
	struct shmid_ds *bsp;
{

	linux_to_bsd_ipc_perm(&lsp->l_shm_perm, &bsp->shm_perm);
	bsp->shm_segsz = lsp->l_shm_segsz;
	bsp->shm_lpid = lsp->l_shm_lpid;
	bsp->shm_cpid = lsp->l_shm_cpid;
	bsp->shm_nattch = lsp->l_shm_nattch;
	bsp->shm_atime = lsp->l_shm_atime;
	bsp->shm_dtime = lsp->l_shm_dtime;
	bsp->shm_ctime = lsp->l_shm_ctime;
	bsp->shm_internal = lsp->l_private2;	/* XXX Oh well. */
}

static void
bsd_to_linux_shmid_ds(bsp, lsp)
	struct shmid_ds *bsp;
	struct linux_shmid_ds *lsp;
{

	bsd_to_linux_ipc_perm(&bsp->shm_perm, &lsp->l_shm_perm);
	lsp->l_shm_segsz = bsp->shm_segsz;
	lsp->l_shm_lpid = bsp->shm_lpid;
	lsp->l_shm_cpid = bsp->shm_cpid;
	lsp->l_shm_nattch = bsp->shm_nattch;
	lsp->l_shm_atime = bsp->shm_atime;
	lsp->l_shm_dtime = bsp->shm_dtime;
	lsp->l_shm_ctime = bsp->shm_ctime;
	lsp->l_private2 = bsp->shm_internal;	/* XXX */
}

/*
 * shmctl. Not implemented (for now): IPC_INFO, SHM_INFO, SHM_STAT
 * SHM_LOCK and SHM_UNLOCK are passed on, but currently not implemented
 * by NetBSD itself.
 *
 * The usual structure conversion and massaging is done.
 */
static int
linux_shmctl(p, uap, retval)
	struct proc *p;
	struct linux_sys_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	caddr_t sg;
	struct sys_shmctl_args nua;
	struct shmid_ds *bsp, bs;
	struct linux_shmid_ds ls;
	int error;

	SCARG(&nua, shmid) = SCARG(uap, a1);
	switch (SCARG(uap, a2)) {
	case LINUX_IPC_STAT:
		sg = stackgap_init(p->p_emul);
		bsp = stackgap_alloc(&sg, sizeof (struct shmid_ds));
		SCARG(&nua, cmd) = IPC_STAT;
		SCARG(&nua, buf) = bsp;
		if ((error = sys_shmctl(p, &nua, retval)))
			return error;
		if ((error = copyin(bsp, &bs, sizeof bs)))
			return error;
		bsd_to_linux_shmid_ds(&bs, &ls);
		return copyout(&ls, SCARG(uap, ptr), sizeof ls);
	case LINUX_IPC_SET:
		if ((error = copyin(SCARG(uap, ptr), &ls, sizeof ls)))
			return error;
		linux_to_bsd_shmid_ds(&ls, &bs);
		sg = stackgap_init(p->p_emul);
		bsp = stackgap_alloc(&sg, sizeof bs);
		if ((error = copyout(&bs, bsp, sizeof bs)))
			return error;
		SCARG(&nua, cmd) = IPC_SET;
		SCARG(&nua, buf) = bsp;
		return sys_shmctl(p, &nua, retval);
	case LINUX_IPC_RMID:
		SCARG(&nua, cmd) = IPC_RMID;
		SCARG(&nua, buf) = NULL;
		break;
	case LINUX_SHM_LOCK:
		SCARG(&nua, cmd) = SHM_LOCK;
		SCARG(&nua, buf) = NULL;
		break;
	case LINUX_SHM_UNLOCK:
		SCARG(&nua, cmd) = SHM_UNLOCK;
		SCARG(&nua, buf) = NULL;
		break;
	case LINUX_IPC_INFO:
	case LINUX_SHM_STAT:
	case LINUX_SHM_INFO:
	default:
		return EINVAL;
	}
	return sys_shmctl(p, &nua, retval);
}
#endif /* SYSVSHM */
