/*	$NetBSD: linux_ipccall.c,v 1.2 1995/03/08 17:27:42 fvdl Exp $	*/

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
#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_util.h>
#include <compat/linux/linux_ipc.h>
#include <compat/linux/linux_msg.h>
#include <compat/linux/linux_shm.h>
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
static int linux_semop __P((struct proc *, struct linux_ipc_args *,
				register_t *));
static int linux_semget __P((struct proc *, struct linux_ipc_args *,
				register_t *));
static int linux_semctl __P((struct proc *, struct linux_ipc_args *,
				register_t *));
#endif

#ifdef SYSVMSG
static int linux_msgsnd __P((struct proc *, struct linux_ipc_args *,
				register_t *));
static int linux_msgrcv __P((struct proc *, struct linux_ipc_args *,
				register_t *));
static int linux_msgop __P((struct proc *, struct linux_ipc_args *,
				register_t *));
static int linux_msgctl __P((struct proc *, struct linux_ipc_args *,
				register_t *));
#endif

#ifdef SYSVSHM
static int linux_shmat __P((struct proc *, struct linux_ipc_args *,
				register_t *));
static int linux_shmdt __P((struct proc *, struct linux_ipc_args *,
				register_t *));
static int linux_shmget __P((struct proc *, struct linux_ipc_args *,
				register_t *));
static int linux_shmctl __P((struct proc *, struct linux_ipc_args *,
				register_t *));
#endif


int
linux_ipc(p, uap, retval)
	struct proc *p;
	struct linux_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	int what, error;

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

#ifdef SYSVSEM
/*
 * Semaphore operations: not implemented yet.
 */
int
linux_semop(p, uap, retval)
	struct proc *p;
	struct linux_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	return ENOSYS;
}

int
linux_semget(p, uap, retval)
	struct proc *p;
	struct linux_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	return ENOSYS;
}

int
linux_semctl(p, uap, retval)
	struct proc *p;
	struct linux_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	return ENOSYS;
}
#endif /* SYSVSEM */

#ifdef SYSVMSG
/*
 * Msg functions: not implemented yet.
 */
int
linux_msgsnd(p, uap, retval)
	struct proc *p;
	struct linux_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	return ENOSYS;
}

int
linux_msgrcv(p, uap, retval)
	struct proc *p;
	struct linux_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	return ENOSYS;
}

int
linux_msgget(p, uap, retval)
	struct proc *p;
	struct linux_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	return ENOSYS;
}

int
linux_msgctl(p, uap, retval)
	struct proc *p;
	struct linux_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	return ENOSYS;
}
#endif /* SYSVMSG */

#ifdef SYSVSHM
/*
 * shmat(2). Very straightforward, except that Linux passes a pointer
 * in which the return value is to be passed. This is subsequently
 * handled by libc, apparently.
 */
int
linux_shmat(p, uap, retval)
	struct proc *p;
	struct linux_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	struct shmat_args bsa;
	int error;

	SCARG(&bsa, shmid) = SCARG(uap, a1);
	SCARG(&bsa, shmaddr) = SCARG(uap, ptr);
	SCARG(&bsa, shmflg) = SCARG(uap, a2);

	if ((error = shmat(p, &bsa, retval)))
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
int
linux_shmdt(p, uap, retval)
	struct proc *p;
	struct linux_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	struct shmdt_args bsa;

	SCARG(&bsa, shmaddr) = SCARG(uap, ptr);
	return shmdt(p, &bsa, retval);
}

/*
 * Same story as shmdt.
 */
int
linux_shmget(p, uap, retval)
	struct proc *p;
	struct linux_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	struct shmget_args bsa;

	SCARG(&bsa, key) = SCARG(uap, a1);
	SCARG(&bsa, size) = SCARG(uap, a2);
	SCARG(&bsa, shmflg) = SCARG(uap, a3);
	return shmget(p, &bsa, retval);
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
int
linux_shmctl(p, uap, retval)
	struct proc *p;
	struct linux_ipc_args /* {
		syscallarg(int) what;
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(caddr_t) ptr;
	} */ *uap;
	register_t *retval;
{
	int error;
	caddr_t sg;
	struct shmctl_args bsa;
	struct shmid_ds *bsp, bs;
	struct linux_shmid_ds lseg;

	switch (SCARG(uap, a2)) {
	case LINUX_IPC_STAT:
		sg = stackgap_init();
		bsp = stackgap_alloc(&sg, sizeof (struct shmid_ds));
		SCARG(&bsa, shmid) = SCARG(uap, a1);
		SCARG(&bsa, cmd) = IPC_STAT;
		SCARG(&bsa, buf) = bsp;
		if ((error = shmctl(p, &bsa, retval)))
			return error;
		if ((error = copyin((caddr_t) &bs, (caddr_t) bsp, sizeof bs)))
			return error;
		bsd_to_linux_shmid_ds(&bs, &lseg);
		return copyout((caddr_t) &lseg, SCARG(uap, ptr), sizeof lseg);
	case LINUX_IPC_SET:
		if ((error = copyin(SCARG(uap, ptr), (caddr_t) &lseg,
		     sizeof lseg)))
			return error;
		linux_to_bsd_shmid_ds(&lseg, &bs);
		sg = stackgap_init();
		bsp = stackgap_alloc(&sg, sizeof (struct shmid_ds));
		if ((error = copyout((caddr_t) &bs, (caddr_t) bsp, sizeof bs)))
			return error;
		SCARG(&bsa, shmid) = SCARG(uap, a1);
		SCARG(&bsa, cmd) = IPC_SET;
		SCARG(&bsa, buf) = bsp;
		return shmctl(p, &bsa, retval);
	case LINUX_IPC_RMID:
	case LINUX_SHM_LOCK:
	case LINUX_SHM_UNLOCK:
		SCARG(&bsa, shmid) = SCARG(uap, a1);
		switch (SCARG(uap, a2)) {
		case LINUX_IPC_RMID:
			SCARG(&bsa, cmd) = IPC_RMID;
			break;
		case LINUX_SHM_LOCK:
			SCARG(&bsa, cmd) = SHM_LOCK;
			break;
		case LINUX_SHM_UNLOCK:
			SCARG(&bsa, cmd) = SHM_UNLOCK;
			break;
		}
		if ((error = copyin(SCARG(uap, ptr), (caddr_t) &lseg,
		     sizeof lseg)))
			return error;
		linux_to_bsd_shmid_ds(&lseg, &bs);
		sg = stackgap_init();
		bsp = stackgap_alloc(&sg, sizeof (struct shmid_ds));
		if ((error = copyout((caddr_t) &bs, (caddr_t) bsp, sizeof bs)))
			return error;
		SCARG(&bsa, buf) = bsp;
		return shmctl(p, &bsa, retval);
	case LINUX_IPC_INFO:
	case LINUX_SHM_STAT:
	case LINUX_SHM_INFO:
	default:
		return EINVAL;
	}
}
#endif /* SYSVSHM */
