/*	$NetBSD: ibcs2_ipc.c,v 1.12.2.4 2002/04/01 07:43:54 nathanw Exp $	*/

/*
 * Copyright (c) 1995 Scott Bartram
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ibcs2_ipc.c,v 1.12.2.4 2002/04/01 07:43:54 nathanw Exp $");

#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/unistd.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/syscallargs.h>

#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_signal.h>
#include <compat/ibcs2/ibcs2_syscallargs.h>
#include <compat/ibcs2/ibcs2_util.h>

#define IBCS2_IPC_RMID	0
#define IBCS2_IPC_SET	1
#define IBCS2_IPC_STAT	2

struct ibcs2_ipc_perm {
	ibcs2_uid_t uid;
	ibcs2_gid_t gid;
	ibcs2_uid_t cuid;
	ibcs2_gid_t cgid;
	ibcs2_mode_t mode;
	u_short seq;
	ibcs2_key_t key;
};

#if defined(SYSVMSG) || defined(SYSVSEM) || defined(SYSVSHM)
static void cvt_perm2iperm __P((struct ipc_perm14 *, struct ibcs2_ipc_perm *));
static void cvt_iperm2perm __P((struct ibcs2_ipc_perm *, struct ipc_perm14 *));

static void
cvt_perm2iperm(bp, ibp)
	struct ipc_perm14 *bp;
	struct ibcs2_ipc_perm *ibp;
{
	ibp->cuid = bp->cuid;
	ibp->cgid = bp->cgid;
	ibp->uid = bp->uid;
	ibp->gid = bp->gid;
	ibp->mode = bp->mode;
	ibp->seq = bp->seq;
	ibp->key = bp->key;
}

static void
cvt_iperm2perm(ibp, bp)
	struct ibcs2_ipc_perm *ibp;
	struct ipc_perm14 *bp;
{
	bp->cuid = ibp->cuid;
	bp->cgid = ibp->cgid;
	bp->uid = ibp->uid;
	bp->gid = ibp->gid;
	bp->mode = ibp->mode;
	bp->seq = ibp->seq;
	bp->key = ibp->key;
}
#endif /* SYSVMSG || SYSVSEM || SYSVMSG */

#ifdef SYSVMSG

/*
 * iBCS2 msgsys call
 */

struct ibcs2_msqid_ds {
	struct ibcs2_ipc_perm msg_perm;
	struct __msg *msg_first;
	struct __msg *msg_last;
	u_short msg_cbytes;
	u_short msg_qnum;
	u_short msg_qbytes;
	u_short msg_lspid;
	u_short msg_lrpid;
	ibcs2_time_t msg_stime;
	ibcs2_time_t msg_rtime;
	ibcs2_time_t msg_ctime;
};

static void cvt_msqid2imsqid __P((struct msqid_ds14 *,
	struct ibcs2_msqid_ds *));
static void cvt_imsqid2msqid __P((struct ibcs2_msqid_ds *,
	struct msqid_ds14 *));

static void
cvt_msqid2imsqid(bp, ibp)
	struct msqid_ds14 *bp;
	struct ibcs2_msqid_ds *ibp;
{
	cvt_perm2iperm(&bp->msg_perm, &ibp->msg_perm);
	ibp->msg_first = bp->msg_first;
	ibp->msg_last = bp->msg_last;
	ibp->msg_cbytes = (u_short)bp->msg_cbytes;
	ibp->msg_qnum = (u_short)bp->msg_qnum;
	ibp->msg_qbytes = (u_short)bp->msg_qbytes;
	ibp->msg_lspid = (u_short)bp->msg_lspid;
	ibp->msg_lrpid = (u_short)bp->msg_lrpid;
	ibp->msg_stime = bp->msg_stime;
	ibp->msg_rtime = bp->msg_rtime;
	ibp->msg_ctime = bp->msg_ctime;
	return;
}

static void
cvt_imsqid2msqid(ibp, bp)
	struct ibcs2_msqid_ds *ibp;
	struct msqid_ds14 *bp;
{
	cvt_iperm2perm(&ibp->msg_perm, &bp->msg_perm);
	bp->msg_first = ibp->msg_first;
	bp->msg_last = ibp->msg_last;
	bp->msg_cbytes = ibp->msg_cbytes;
	bp->msg_qnum = ibp->msg_qnum;
	bp->msg_qbytes = ibp->msg_qbytes;
	bp->msg_lspid = ibp->msg_lspid;
	bp->msg_lrpid = ibp->msg_lrpid;
	bp->msg_stime = ibp->msg_stime;
	bp->msg_rtime = ibp->msg_rtime;
	bp->msg_ctime = ibp->msg_ctime;
	return;
}

int
ibcs2_sys_msgsys(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_msgsys_args /* {
		syscallarg(int) which;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(int) a4;
		syscallarg(int) a5;
		syscallarg(int) a6;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	switch (SCARG(uap, which)) {
#ifdef SYSVMSG
	case 0:				/* msgget */
		SCARG(uap, which) = 1;
		return compat_10_sys_msgsys(l, uap, retval);
	case 1: {			/* msgctl */
		int error;
		struct compat_10_sys_msgsys_args margs;
		caddr_t sg = stackgap_init(p, 0);

		SCARG(&margs, which) = 0;
		SCARG(&margs, a2) = SCARG(uap, a2);
		SCARG(&margs, a4) =
		    (int)stackgap_alloc(p, &sg, sizeof(struct msqid_ds14));
		SCARG(&margs, a3) = SCARG(uap, a3);
		switch (SCARG(&margs, a3)) {
		case IBCS2_IPC_STAT:
			error = compat_10_sys_msgsys(l, &margs, retval);
			if (!error)
				cvt_msqid2imsqid((struct msqid_ds14 *)
				    SCARG(&margs, a4),
				    (struct ibcs2_msqid_ds *)SCARG(uap, a4));
			return error;
		case IBCS2_IPC_SET:
			cvt_imsqid2msqid((struct ibcs2_msqid_ds *)SCARG(uap,
									a4),
					 (struct msqid_ds14 *) SCARG(&margs,
					 			     a4));
			return compat_10_sys_msgsys(l, &margs, retval);
		case IBCS2_IPC_RMID:
			return compat_10_sys_msgsys(l, &margs, retval);
		}
		return EINVAL;
	}
	case 2:				/* msgrcv */
		SCARG(uap, which) = 3;
		return compat_10_sys_msgsys(l, uap, retval);
	case 3:				/* msgsnd */
		SCARG(uap, which) = 2;
		return compat_10_sys_msgsys(l, uap, retval);
#endif
	default:
		return EINVAL;
	}
}

#endif /* SYSVMSG */

#ifdef SYSVSEM

/*
 * iBCS2 semsys call
 */

struct ibcs2_semid_ds {
	struct ibcs2_ipc_perm sem_perm;
	struct ibcs2_sem *sem_base;
	u_short sem_nsems;
	u_short pad1;
	ibcs2_time_t sem_otime;
	ibcs2_time_t sem_ctime;
};

struct ibcs2_sem {
        u_short semval;
	ibcs2_pid_t sempid;
	u_short semncnt;
	u_short semzcnt;
};

static void cvt_semid2isemid __P((struct semid_ds14 *,
	struct ibcs2_semid_ds *));
static void cvt_isemid2semid __P((struct ibcs2_semid_ds *,
	struct semid_ds14 *));
#ifdef notdef
static void cvt_sem2isem __P((struct sem *, struct ibcs2_sem *));
static void cvt_isem2sem __P((struct ibcs2_sem *, struct sem *));

static void
cvt_sem2isem(bp, ibp)
	struct __sem *bp;
	struct ibcs2_sem *ibp;
{
	ibp->semval = bp->semval;
	ibp->sempid = bp->sempid;
	ibp->semncnt = bp->semncnt;
	ibp->semzcnt = bp->semzcnt;
	return;
}

static void
cvt_isem2sem(ibp, bp)
	struct ibcs2_sem *ibp;
	struct __sem *bp;
{
	bp->semval = ibp->semval;
	bp->sempid = ibp->sempid;
	bp->semncnt = ibp->semncnt;
	bp->semzcnt = ibp->semzcnt;
	return;
}
#endif

static void
cvt_semid2isemid(bp, ibp)
	struct semid_ds14 *bp;
	struct ibcs2_semid_ds *ibp;
{
	cvt_perm2iperm(&bp->sem_perm, &ibp->sem_perm);
	ibp->sem_base = (struct ibcs2_sem *)bp->sem_base;
	ibp->sem_nsems = bp->sem_nsems;
	ibp->sem_otime = bp->sem_otime;
	ibp->sem_ctime = bp->sem_ctime;
	return;
}


static void
cvt_isemid2semid(ibp, bp)
	struct ibcs2_semid_ds *ibp;
	struct semid_ds14 *bp;
{
	cvt_iperm2perm(&ibp->sem_perm, &bp->sem_perm);
	bp->sem_base = (struct __sem *)ibp->sem_base;
	bp->sem_nsems = ibp->sem_nsems;
	bp->sem_otime = ibp->sem_otime;
	bp->sem_ctime = ibp->sem_ctime;
	return;
}

int
ibcs2_sys_semsys(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_semsys_args /* {
		syscallarg(int) which;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(int) a4;
		syscallarg(int) a5;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;

#ifdef SYSVSEM
	switch (SCARG(uap, which)) {
	case 0:					/* semctl */
		switch(SCARG(uap, a4)) {
		case IBCS2_IPC_STAT:
		    {
			    struct ibcs2_semid_ds *isp, isi;
			    struct semid_ds14 *sp, s;
			    caddr_t sg = stackgap_init(p, 0);
  
			    isp = (struct ibcs2_semid_ds *)SCARG(uap, a5);
			    sp = stackgap_alloc(p, &sg, sizeof(struct semid_ds14));
			    SCARG(uap, a5) = (int)sp;
			    error = compat_10_sys_semsys(l, uap, retval);
			    if (error)
				    return error;
			    error = copyin((caddr_t)sp, (caddr_t)&s,
					   sizeof(s));
			    if (error)
				    return error;
			    cvt_semid2isemid(&s, &isi);
			    return copyout((caddr_t)&isi, (caddr_t)isp,
					   sizeof(isi));
		    }
		case IBCS2_IPC_SET:
		    {
			    struct ibcs2_semid_ds isp;
			    struct semid_ds14 *sp, s;
			    caddr_t sg = stackgap_init(p, 0);
  
			    error = copyin((caddr_t)SCARG(uap, a5),
					   (caddr_t)&isp, sizeof(isp));
			    if (error)
				    return error;
			    cvt_isemid2semid(&isp, &s);
			    sp = stackgap_alloc(p, &sg, sizeof(s));
			    error = copyout((caddr_t)&s, (caddr_t)sp,
					    sizeof(s));
			    if (error)
				    return error;
			    SCARG(uap, a5) = (int)sp;
			    return compat_10_sys_semsys(l, uap, retval);
		    }
		}
		return compat_10_sys_semsys(l, uap, retval);

	case 1:				/* semget */
		return compat_10_sys_semsys(l, uap, retval);

	case 2:				/* semop */
		return compat_10_sys_semsys(l, uap, retval);
	}
#endif
	return EINVAL;
}

#endif /* SYSVSEM */

#ifdef SYSVSHM

/*
 * iBCS2 shmsys call
 */

struct ibcs2_shmid_ds {
        struct ibcs2_ipc_perm shm_perm;
	int shm_segsz;
	int pad1;
	char pad2[4];
	u_short shm_lpid;
	u_short shm_cpid;
	u_short shm_nattch;
	u_short shm_cnattch;
	ibcs2_time_t shm_atime;
	ibcs2_time_t shm_dtime;
	ibcs2_time_t shm_ctime;
};

static void cvt_shmid2ishmid __P((struct shmid_ds14 *,
	struct ibcs2_shmid_ds *));
static void cvt_ishmid2shmid __P((struct ibcs2_shmid_ds *,
	struct shmid_ds14 *));

static void
cvt_shmid2ishmid(bp, ibp)
	struct shmid_ds14 *bp;
	struct ibcs2_shmid_ds *ibp;
{
	cvt_perm2iperm(&bp->shm_perm, &ibp->shm_perm);
	ibp->shm_segsz = bp->shm_segsz;
	ibp->shm_lpid = bp->shm_lpid;
	ibp->shm_cpid = bp->shm_cpid;
	ibp->shm_nattch = bp->shm_nattch;
	ibp->shm_cnattch = 0;			/* ignored anyway */
	ibp->shm_atime = bp->shm_atime;
	ibp->shm_dtime = bp->shm_dtime;
	ibp->shm_ctime = bp->shm_ctime;
	return;
}

static void
cvt_ishmid2shmid(ibp, bp)
	struct ibcs2_shmid_ds *ibp;
	struct shmid_ds14 *bp;
{
	cvt_iperm2perm(&ibp->shm_perm, &bp->shm_perm);
	bp->shm_segsz = ibp->shm_segsz;
	bp->shm_lpid = ibp->shm_lpid;
	bp->shm_cpid = ibp->shm_cpid;
	bp->shm_nattch = ibp->shm_nattch;
	bp->shm_atime = ibp->shm_atime;
	bp->shm_dtime = ibp->shm_dtime;
	bp->shm_ctime = ibp->shm_ctime;
	bp->shm_internal = (void *)0;		/* ignored anyway */
	return;
}

int
ibcs2_sys_shmsys(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_shmsys_args /* {
		syscallarg(int) which;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(int) a4;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;

#ifdef SYSVSHM
	switch (SCARG(uap, which)) {
	case 0:						/* shmat */
		return compat_10_sys_shmsys(l, uap, retval);

	case 1:						/* shmctl */
		switch(SCARG(uap, a3)) {
		case IBCS2_IPC_STAT:
		    {
			    struct ibcs2_shmid_ds *isp, is;
			    struct shmid_ds14 *sp, s;
			    caddr_t sg = stackgap_init(p, 0);
  
			    isp = (struct ibcs2_shmid_ds *)SCARG(uap, a4);
			    sp = stackgap_alloc(p, &sg, sizeof(*sp));
			    SCARG(uap, a4) = (int)sp;
			    error = compat_10_sys_shmsys(l, uap, retval);
			    if (error)
				    return error;
			    error = copyin((caddr_t)sp, (caddr_t)&s,
					   sizeof(s));
			    if (error)
				    return error;
			    cvt_shmid2ishmid(&s, &is);
			    return copyout((caddr_t)&is, (caddr_t)isp,
					   sizeof(is));
		    }
		case IBCS2_IPC_SET:
		    {
			    struct ibcs2_shmid_ds is;
			    struct shmid_ds14 *sp, s;
			    caddr_t sg = stackgap_init(p, 0);
  
			    error = copyin((caddr_t)SCARG(uap, a4),
					   (caddr_t)&is, sizeof(is));
			    if (error)
				    return error;
			    cvt_ishmid2shmid(&is, &s);
			    sp = stackgap_alloc(p, &sg, sizeof(*sp));
			    SCARG(uap, a4) = (int)sp;
			    error = copyout((caddr_t)&s, (caddr_t)sp,
					    sizeof(s));
			    if (error)
				    return error;
			    return compat_10_sys_shmsys(l, uap, retval);
		    }
		}
		return compat_10_sys_shmsys(l, uap, retval);

	case 2:						/* shmdt */
		return compat_10_sys_shmsys(l, uap, retval);

	case 3:						/* shmget */
		return compat_10_sys_shmsys(l, uap, retval);
	}
#endif
	return EINVAL;
}

#endif /* SYSVSHM */
