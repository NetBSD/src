/*	$NetBSD: ibcs2_ipc.c,v 1.24.14.2 2007/12/27 00:43:45 mjf Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ibcs2_ipc.c,v 1.24.14.2 2007/12/27 00:43:45 mjf Exp $");

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

#include <compat/sys/sem.h>
#include <compat/sys/shm.h>
#include <compat/sys/msg.h>

/* Verify that the standard values are correct. */
typedef char x[IPC_RMID == 0 && IPC_SET == 1 && IPC_STAT == 2 ? 1 : -1];

struct ibcs2_ipc_perm {
	ibcs2_uid_t uid;
	ibcs2_gid_t gid;
	ibcs2_uid_t cuid;
	ibcs2_gid_t cgid;
	ibcs2_mode_t mode;
	u_short seq;
	ibcs2_key_t key;
};

struct ibcs2_msqid_ds {
	struct ibcs2_ipc_perm msg_perm;
	struct __msg *msg_first;	/* kernel address don't copyout */
	struct __msg *msg_last;		/* kernel address don't copyout */
	u_short msg_cbytes;
	u_short msg_qnum;
	u_short msg_qbytes;
	u_short msg_lspid;
	u_short msg_lrpid;
	ibcs2_time_t msg_stime;
	ibcs2_time_t msg_rtime;
	ibcs2_time_t msg_ctime;
};

#if defined(SYSVMSG) || defined(SYSVSEM) || defined(SYSVSHM)
static void
cvt_perm2iperm(const struct ipc_perm *bp, struct ibcs2_ipc_perm *ibp)
{
	ibp->cuid = bp->cuid;
	ibp->cgid = bp->cgid;
	ibp->uid = bp->uid;
	ibp->gid = bp->gid;
	ibp->mode = bp->mode;
	ibp->seq = bp->_seq;
	ibp->key = bp->_key;
}

static void
cvt_iperm2perm(const struct ibcs2_ipc_perm *ibp, struct ipc_perm *bp)
{
	bp->cuid = ibp->cuid;
	bp->cgid = ibp->cgid;
	bp->uid = ibp->uid;
	bp->gid = ibp->gid;
	bp->mode = ibp->mode;
	bp->_seq = ibp->seq;
	bp->_key = ibp->key;
}
#endif /* SYSVMSG || SYSVSEM || SYSVMSG */

#ifdef SYSVMSG

/*
 * iBCS2 msgsys call
 */

static void
cvt_msqid2imsqid(const struct msqid_ds *bp, struct ibcs2_msqid_ds *ibp)
{
	cvt_perm2iperm(&bp->msg_perm, &ibp->msg_perm);
	ibp->msg_first = NULL;
	ibp->msg_last = NULL;
	ibp->msg_cbytes = bp->_msg_cbytes;
	ibp->msg_qnum = bp->msg_qnum;
	ibp->msg_qbytes = bp->msg_qbytes;
	ibp->msg_lspid = bp->msg_lspid;
	ibp->msg_lrpid = bp->msg_lrpid;
	ibp->msg_stime = bp->msg_stime;
	ibp->msg_rtime = bp->msg_rtime;
	ibp->msg_ctime = bp->msg_ctime;
}

static void
cvt_imsqid2msqid(struct ibcs2_msqid_ds *ibp, struct msqid_ds *bp)
{
	cvt_iperm2perm(&ibp->msg_perm, &bp->msg_perm);
	bp->_msg_first = NULL;
	bp->_msg_last = NULL;
	bp->_msg_cbytes = ibp->msg_cbytes;
	bp->msg_qnum = ibp->msg_qnum;
	bp->msg_qbytes = ibp->msg_qbytes;
	bp->msg_lspid = ibp->msg_lspid;
	bp->msg_lrpid = ibp->msg_lrpid;
	bp->msg_stime = ibp->msg_stime;
	bp->msg_rtime = ibp->msg_rtime;
	bp->msg_ctime = ibp->msg_ctime;
}

static int
do_compat_10_sys_msgsys(struct lwp *l, const struct ibcs2_sys_msgsys_args *uap,
    register_t *retval, int which)
{
    struct compat_10_sys_msgsys_args bsd_ua;

    SCARG(&bsd_ua, which) = which;
    SCARG(&bsd_ua, a2) = SCARG(uap, a2);
    SCARG(&bsd_ua, a3) = SCARG(uap, a3);
    SCARG(&bsd_ua, a4) = SCARG(uap, a4);
    SCARG(&bsd_ua, a5) = SCARG(uap, a5);
    SCARG(&bsd_ua, a6) = SCARG(uap, a6);

    return compat_10_sys_msgsys(l, &bsd_ua, retval);
}

int
ibcs2_sys_msgsys(struct lwp *l, const struct ibcs2_sys_msgsys_args *uap, register_t *retval)
{
#ifdef SYSVMSG
	/* {
		syscallarg(int) which;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(int) a4;
		syscallarg(int) a5;
		syscallarg(int) a6;
	} */
	int error;
	struct msqid_ds msqbuf;
	struct ibcs2_msqid_ds msqbuf_ibcs2, *ibp;

	switch (SCARG(uap, which)) {
	case 0:				/* msgget */
		return do_compat_10_sys_msgsys(l, uap, retval, 1);
	case 1: 			/* msgctl */
		ibp = (void *)SCARG(uap, a4);
		switch (SCARG(uap, a3)) {
		case IPC_STAT:
			error = msgctl1(l, SCARG(uap, a2), IPC_STAT, &msqbuf);
			if (error == 0) {
				cvt_msqid2imsqid(&msqbuf, &msqbuf_ibcs2);
				error = copyout(&msqbuf_ibcs2, ibp,
				    sizeof msqbuf_ibcs2);
			}
			return error;
		case IPC_SET:
			error = copyin(ibp, &msqbuf_ibcs2, sizeof msqbuf_ibcs2);
			if (error == 0) {
				cvt_imsqid2msqid(&msqbuf_ibcs2, &msqbuf);
				error = msgctl1(l, SCARG(uap, a2),
				    IPC_SET, &msqbuf);
			}
			return error;
		case IPC_RMID:
			return msgctl1(l, SCARG(uap, a2), IPC_RMID, NULL);
		}
		return EINVAL;
	case 2:				/* msgrcv */
		return do_compat_10_sys_msgsys(l, uap, retval, 3);
	case 3:				/* msgsnd */
		return do_compat_10_sys_msgsys(l, uap, retval, 2);
	default:
		break;
	}
#endif
	return EINVAL;
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

#ifdef notdef
static void cvt_sem2isem(struct sem *, struct ibcs2_sem *);
static void cvt_isem2sem(struct ibcs2_sem *, struct sem *);

static void
cvt_sem2isem(struct __sem *bp, struct ibcs2_sem *ibp)
{
	ibp->semval = bp->semval;
	ibp->sempid = bp->sempid;
	ibp->semncnt = bp->semncnt;
	ibp->semzcnt = bp->semzcnt;
}

static void
cvt_isem2sem(struct ibcs2_sem *ibp, struct __sem *bp)
{
	bp->semval = ibp->semval;
	bp->sempid = ibp->sempid;
	bp->semncnt = ibp->semncnt;
	bp->semzcnt = ibp->semzcnt;
}
#endif

static void
cvt_semid2isemid(const struct semid_ds *bp, struct ibcs2_semid_ds *ibp)
{
	cvt_perm2iperm(&bp->sem_perm, &ibp->sem_perm);
	ibp->sem_base = (struct ibcs2_sem *)bp->_sem_base;
	ibp->sem_nsems = bp->sem_nsems;
	ibp->sem_otime = bp->sem_otime;
	ibp->sem_ctime = bp->sem_ctime;
}


static void
cvt_isemid2semid(const struct ibcs2_semid_ds *ibp, struct semid_ds *bp)
{
	cvt_iperm2perm(&ibp->sem_perm, &bp->sem_perm);
	bp->_sem_base = (struct __sem *)ibp->sem_base;
	bp->sem_nsems = ibp->sem_nsems;
	bp->sem_otime = ibp->sem_otime;
	bp->sem_ctime = ibp->sem_ctime;
}

int
ibcs2_sys_semsys(struct lwp *l, const struct ibcs2_sys_semsys_args *uap, register_t *retval)
{
#ifdef SYSVSEM
	/* {
		syscallarg(int) which;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(int) a4;
		syscallarg(int) a5;
	} */
	struct semid_ds sembuf;
	struct ibcs2_semid_ds isembuf;
	void *pass_arg;
	int a5 = SCARG(uap, a5);
	int error;

	switch (SCARG(uap, which)) {
	case 0:					/* semctl */
#define	semctl_semid	SCARG(uap, a2)
#define	semctl_semnum	SCARG(uap, a3)
#define	semctl_cmd	SCARG(uap, a4)
#define	semctl_arg	((union __semun *)&a5)
		pass_arg = get_semctl_arg(semctl_cmd, &sembuf, semctl_arg);
		if (semctl_cmd == IPC_SET) {
			error = copyin(semctl_arg->buf, &isembuf, sizeof isembuf);
			if (error != 0)
				return error;
			cvt_isemid2semid(&isembuf, &sembuf);
		}
		error = semctl1(l, semctl_semid, semctl_semnum, semctl_cmd, 
		    pass_arg, retval);
		if (error == 0 && semctl_cmd == IPC_STAT) {
			cvt_semid2isemid(&sembuf, &isembuf);
			error = copyout(&isembuf, semctl_arg->buf, sizeof(isembuf));
		}
		return error;
#undef	semctl_semid
#undef	semctl_semnum
#undef	semctl_cmd
#undef	semctl_arg

	case 1:				/* semget */
		return compat_10_sys_semsys(l, (const void *)uap, retval);

	case 2:				/* semop */
		return compat_10_sys_semsys(l, (const void *)uap, retval);
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

static void
cvt_shmid2ishmid(const struct shmid_ds *bp, struct ibcs2_shmid_ds *ibp)
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
}

static void
cvt_ishmid2shmid(const struct ibcs2_shmid_ds *ibp, struct shmid_ds *bp)
{
	cvt_iperm2perm(&ibp->shm_perm, &bp->shm_perm);
	bp->shm_segsz = ibp->shm_segsz;
	bp->shm_lpid = ibp->shm_lpid;
	bp->shm_cpid = ibp->shm_cpid;
	bp->shm_nattch = ibp->shm_nattch;
	bp->shm_atime = ibp->shm_atime;
	bp->shm_dtime = ibp->shm_dtime;
	bp->shm_ctime = ibp->shm_ctime;
	bp->_shm_internal = (void *)0;		/* ignored anyway */
	return;
}

int
ibcs2_sys_shmsys(struct lwp *l, const struct ibcs2_sys_shmsys_args *uap, register_t *retval)
{
#ifdef SYSVSHM
	/* {
		syscallarg(int) which;
		syscallarg(int) a2;
		syscallarg(int) a3;
		syscallarg(int) a4;
	} */
	struct shmid_ds shmbuf;
	struct ibcs2_shmid_ds *isp, ishmbuf;
	int cmd, error;

	switch (SCARG(uap, which)) {
	case 0:						/* shmat */
		break;

	case 1:						/* shmctl */
		cmd = SCARG(uap, a3);
		isp = (struct ibcs2_shmid_ds *)SCARG(uap, a4);
		if (cmd == IPC_SET) {
			error = copyin(isp, &ishmbuf, sizeof(ishmbuf));
			if (error)
				return error;
			cvt_ishmid2shmid(&ishmbuf, &shmbuf);
		}

		error = shmctl1(l, SCARG(uap, a2), cmd,
		    (cmd == IPC_SET || cmd == IPC_STAT) ? &shmbuf : NULL);

		if (error == 0 && cmd == IPC_STAT) {
			cvt_shmid2ishmid(&shmbuf, &ishmbuf);
			error = copyout(&ishmbuf, isp, sizeof(ishmbuf));
		}
		return error;

	case 2:						/* shmdt */
		break;

	case 3:						/* shmget */
		break;

	default:
		return EINVAL;
	}
	return compat_10_sys_shmsys(l, (const void *)uap, retval);
#else
	return EINVAL;
#endif
}

#endif /* SYSVSHM */
