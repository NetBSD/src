/*	$NetBSD: hpux_ipc.c,v 1.3 2001/05/30 11:37:23 mrg Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * HP-UX System V IPC compatibility module.
 *
 * The HP-UX System V IPC calls are essentially compatible with the
 * native NetBSD calls; the flags, commands, and data types are the same.
 * The only differences are the structures used for IPC_SET and IPC_STAT.
 *
 * HP-UX has both `old' (before HP-UX A8.00) and `new' variants of these
 * calls.
 */

#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <compat/hpux/hpux.h>
#include <compat/hpux/hpux_types.h>
#include <compat/hpux/hpux_ipc.h>
#include <compat/hpux/hpux_syscallargs.h>

#if defined(SYSVMSG) || defined(SYSVSEM) || defined(SYSVSHM)
void	bsd_to_hpux_ipc_perm __P((const struct ipc_perm *,
	    struct hpux_ipc_perm *));
void	bsd_to_hpux_oipc_perm __P((const struct ipc_perm *,
	    struct hpux_oipc_perm *));
void	hpux_to_bsd_ipc_perm __P((const struct hpux_ipc_perm *,
	    struct ipc_perm *));
void	hpux_to_bsd_oipc_perm __P((const struct hpux_oipc_perm *,
	    struct ipc_perm *));

void
bsd_to_hpux_ipc_perm(bp, hp)
	const struct ipc_perm *bp;
	struct hpux_ipc_perm *hp;
{

	hp->uid = bp->uid;
	hp->gid = bp->gid;
	hp->cuid = bp->cuid;
	hp->cgid = bp->cgid;
	hp->mode = bp->mode;
	hp->seq = bp->_seq;
	hp->key = bp->_key;
}

void
bsd_to_hpux_oipc_perm(bp, hp)
	const struct ipc_perm *bp;
	struct hpux_oipc_perm *hp;
{

	hp->uid = bp->uid;
	hp->gid = bp->gid;
	hp->cuid = bp->cuid;
	hp->cgid = bp->cgid;
	hp->mode = bp->mode;
	hp->seq = bp->_seq;
	hp->key = bp->_key;
}

void
hpux_to_bsd_ipc_perm(hp, bp)
	const struct hpux_ipc_perm *hp;
	struct ipc_perm *bp;
{

	bp->uid = hp->uid;
	bp->gid = hp->gid;
	bp->cuid = hp->cuid;
	bp->cgid = hp->cgid;
	bp->mode = hp->mode;
	bp->_seq = hp->seq;
	bp->_key = hp->key;
}

void
hpux_to_bsd_oipc_perm(hp, bp)
	const struct hpux_oipc_perm *hp;
	struct ipc_perm *bp;
{

	bp->uid = hp->uid;
	bp->gid = hp->gid;
	bp->cuid = hp->cuid;
	bp->cgid = hp->cgid;
	bp->mode = hp->mode;
	bp->_seq = hp->seq;
	bp->_key = hp->key;
}
#endif /* SYSVMSG || SYSVSEM || SYSVSHM */

#ifdef SYSVMSG
void	bsd_to_hpux_msqid_ds __P((const struct msqid_ds *,
	    struct hpux_msqid_ds *));
void	bsd_to_hpux_omsqid_ds __P((const struct msqid_ds *,
	    struct hpux_omsqid_ds *));

void	hpux_to_bsd_msqid_ds __P((const struct hpux_msqid_ds *,
	    struct msqid_ds *));
void	hpux_to_bsd_omsqid_ds __P((const struct hpux_omsqid_ds *,
	    struct msqid_ds *));

void
bsd_to_hpux_msqid_ds(bp, hp)
	const struct msqid_ds *bp;
	struct hpux_msqid_ds *hp;
{

	bsd_to_hpux_ipc_perm(&bp->msg_perm, &hp->msg_perm);
	hp->msg_first = bp->_msg_first;
	hp->msg_last = bp->_msg_last;
	hp->msg_qnum = bp->msg_qnum;
	hp->msg_qbytes = bp->msg_qbytes;
	hp->msg_lspid = bp->msg_lspid;
	hp->msg_lrpid = bp->msg_lrpid;
	hp->msg_stime = bp->msg_stime;
	hp->msg_rtime = bp->msg_rtime;
	hp->msg_ctime = bp->msg_ctime;
	hp->msg_cbytes = bp->_msg_cbytes;
}

void
bsd_to_hpux_omsqid_ds(bp, hp)
	const struct msqid_ds *bp;
	struct hpux_omsqid_ds *hp;
{

	bsd_to_hpux_oipc_perm(&bp->msg_perm, &hp->msg_perm);
	hp->msg_first = bp->_msg_first;
	hp->msg_last = bp->_msg_last;
	hp->msg_cbytes = bp->_msg_cbytes;
	hp->msg_qnum = bp->msg_qnum;
	hp->msg_qbytes = bp->msg_qbytes;
	hp->msg_lspid = bp->msg_lspid;
	hp->msg_lrpid = bp->msg_lrpid;
	hp->msg_rtime = bp->msg_rtime;
	hp->msg_ctime = bp->msg_ctime;
}

void
hpux_to_bsd_msqid_ds(hp, bp)
	const struct hpux_msqid_ds *hp;
	struct msqid_ds *bp;
{

	hpux_to_bsd_ipc_perm(&hp->msg_perm, &bp->msg_perm);
	bp->msg_qnum = hp->msg_qnum;
	bp->msg_qbytes = hp->msg_qbytes;
	bp->msg_lspid = hp->msg_lspid;
	bp->msg_lrpid = hp->msg_lrpid;
	bp->msg_stime = hp->msg_stime;
	bp->msg_rtime = hp->msg_rtime;
	bp->msg_ctime = hp->msg_ctime;
	bp->_msg_first = hp->msg_first;
	bp->_msg_last = hp->msg_last;
	bp->_msg_cbytes = hp->msg_cbytes;
}

void
hpux_to_bsd_omsqid_ds(hp, bp)
	const struct hpux_omsqid_ds *hp;
	struct msqid_ds *bp;
{

	hpux_to_bsd_oipc_perm(&hp->msg_perm, &bp->msg_perm);
	bp->msg_qnum = hp->msg_qnum;
	bp->msg_qbytes = hp->msg_qbytes;
	bp->msg_lspid = hp->msg_lspid;
	bp->msg_lrpid = hp->msg_lrpid;
	bp->msg_stime = hp->msg_stime;
	bp->msg_rtime = hp->msg_rtime;
	bp->msg_ctime = hp->msg_ctime;
	bp->_msg_first = hp->msg_first;
	bp->_msg_last = hp->msg_last;
	bp->_msg_cbytes = hp->msg_cbytes;
}

int
hpux_sys_omsgctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct hpux_sys_omsgctl_args /* {
		syscallarg(int) msqid;
		syscallarg(int) cmd;
		syscallarg(struct hpux_omsqid_ds *) buf;
	} */ *uap = v;
	struct msqid_ds msqbuf;
	struct hpux_omsqid_ds hmsqbuf;
	int cmd, error;

	cmd = SCARG(uap, cmd);

	if (cmd == IPC_SET) {
		error = copyin(SCARG(uap, buf), &hmsqbuf, sizeof(hmsqbuf));
		if (error)
			return (error);
		hpux_to_bsd_omsqid_ds(&hmsqbuf, &msqbuf);
	}

	error = msgctl1(p, SCARG(uap, msqid), cmd,
	    (cmd == IPC_SET || cmd == IPC_STAT) ? &msqbuf : NULL);

	if (error == 0 && cmd == IPC_STAT) {
		bsd_to_hpux_omsqid_ds(&msqbuf, &hmsqbuf);
		error = copyout(&hmsqbuf, SCARG(uap, buf), sizeof(hmsqbuf));
	}

	return (error);
}

int
hpux_sys_msgctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct hpux_sys_msgctl_args /* {
		syscallarg(int) msqid;
		syscallarg(int) cmd;
		syscallarg(struct hpux_msqid_ds *) buf;
	} */ *uap = v;
	struct msqid_ds msqbuf;
	struct hpux_msqid_ds hmsqbuf;
	int cmd, error;

	cmd = SCARG(uap, cmd);

	if (cmd == IPC_SET) {
		error = copyin(SCARG(uap, buf), &hmsqbuf, sizeof(hmsqbuf));
		if (error)
			return (error);
		hpux_to_bsd_msqid_ds(&hmsqbuf, &msqbuf);
	}

	error = msgctl1(p, SCARG(uap, msqid), cmd,
	    (cmd == IPC_SET || cmd == IPC_STAT) ? &msqbuf : NULL);

	if (error == 0 && cmd == IPC_STAT) {
		bsd_to_hpux_msqid_ds(&msqbuf, &hmsqbuf);
		error = copyout(&hmsqbuf, SCARG(uap, buf), sizeof(hmsqbuf));
	}

	return (error);
}
#endif /* SYSVMSG */

#ifdef SYSVSEM
void	bsd_to_hpux_semid_ds __P((const struct semid_ds *,
	    struct hpux_semid_ds *));
void	bsd_to_hpux_osemid_ds __P((const struct semid_ds *,
	    struct hpux_osemid_ds *));

void	hpux_to_bsd_semid_ds __P((const struct hpux_semid_ds *,
	    struct semid_ds *));
void	hpux_to_bsd_osemid_ds __P((const struct hpux_osemid_ds *,
	    struct semid_ds *));

void
bsd_to_hpux_semid_ds(bp, hp)
	const struct semid_ds *bp;
	struct hpux_semid_ds *hp;
{

	bsd_to_hpux_ipc_perm(&bp->sem_perm, &hp->sem_perm);
	hp->sem_base = bp->_sem_base;
	hp->sem_otime = bp->sem_otime;
	hp->sem_ctime = bp->sem_ctime;
	hp->sem_nsems = bp->sem_nsems;
}

void
bsd_to_hpux_osemid_ds(bp, hp)
	const struct semid_ds *bp;
	struct hpux_osemid_ds *hp;
{

	bsd_to_hpux_oipc_perm(&bp->sem_perm, &hp->sem_perm);
	hp->sem_base = bp->_sem_base;
	hp->sem_otime = bp->sem_otime;
	hp->sem_ctime = bp->sem_ctime;
	hp->sem_nsems = bp->sem_nsems;
}

void
hpux_to_bsd_semid_ds(hp, bp)
	const struct hpux_semid_ds *hp;
	struct semid_ds *bp;
{

	hpux_to_bsd_ipc_perm(&hp->sem_perm, &bp->sem_perm);
	bp->sem_nsems = hp->sem_nsems;
	bp->sem_otime = hp->sem_otime;
	bp->sem_ctime = hp->sem_ctime;
	bp->_sem_base = hp->sem_base;
}

void
hpux_to_bsd_osemid_ds(hp, bp)
	const struct hpux_osemid_ds *hp;
	struct semid_ds *bp;
{

	hpux_to_bsd_oipc_perm(&hp->sem_perm, &bp->sem_perm);
	bp->sem_nsems = hp->sem_nsems;
	bp->sem_otime = hp->sem_otime;
	bp->sem_ctime = hp->sem_ctime;
	bp->_sem_base = hp->sem_base;
}

int
hpux_sys_osemctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct hpux_sys_osemctl_args /* {
		syscallarg(int) semid;
		syscallarg(int) semnum;
		syscallarg(int) cmd;
		syscallarg(union __semun) arg;
	} */ *uap = v;
	struct hpux_osemid_ds hsembuf;
	struct semid_ds sembuf;
	int cmd, error;
	void *pass_arg = NULL;

	cmd = SCARG(uap, cmd);

	switch (cmd) {
	case IPC_SET:
	case IPC_STAT:
		pass_arg = &sembuf;
		break;

	case GETALL:
	case SETVAL:
	case SETALL:
		pass_arg = &SCARG(uap, arg);
		break;
	}

	if (cmd == IPC_SET) {
		error = copyin(SCARG(uap, arg).buf, &hsembuf, sizeof(hsembuf));
		if (error)
			return (error);
		hpux_to_bsd_osemid_ds(&hsembuf, &sembuf);
	}

	error = semctl1(p, SCARG(uap, semid), SCARG(uap, semnum), cmd,
	    pass_arg, retval);

	if (error == 0 && cmd == IPC_STAT) {
		bsd_to_hpux_osemid_ds(&sembuf, &hsembuf);
		error = copyout(&hsembuf, SCARG(uap, arg).buf, sizeof(hsembuf));
	}

	return (error);
}

int
hpux_sys_semctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct hpux_sys_semctl_args /* {
		syscallarg(int) semid;
		syscallarg(int) semnum;
		syscallarg(int) cmd;
		syscallarg(union __semun) arg;
	} */ *uap = v;
	struct hpux_semid_ds hsembuf;
	struct semid_ds sembuf;
	int cmd, error;
	void *pass_arg = NULL;

	cmd = SCARG(uap, cmd);

	switch (cmd) {
	case IPC_SET:
	case IPC_STAT:
		pass_arg = &sembuf;
		break;

	case GETALL:
	case SETVAL:
	case SETALL:
		pass_arg = &SCARG(uap, arg);
		break;
	}

	if (cmd == IPC_SET) {
		error = copyin(SCARG(uap, arg).buf, &hsembuf, sizeof(hsembuf));
		if (error)
			return (error);
		hpux_to_bsd_semid_ds(&hsembuf, &sembuf);
	}

	error = semctl1(p, SCARG(uap, semid), SCARG(uap, semnum), cmd,
	    pass_arg, retval);

	if (error == 0 && cmd == IPC_STAT) {
		bsd_to_hpux_semid_ds(&sembuf, &hsembuf);
		error = copyout(&hsembuf, SCARG(uap, arg).buf, sizeof(hsembuf));
	}

	return (error);
}
#endif /* SYSVSEM */

#ifdef SYSVSHM
void	bsd_to_hpux_shmid_ds __P((const struct shmid_ds *,
	    struct hpux_shmid_ds *));
void	bsd_to_hpux_oshmid_ds __P((const struct shmid_ds *,
	    struct hpux_oshmid_ds *));

void	hpux_to_bsd_shmid_ds __P((const struct hpux_shmid_ds *,
	    struct shmid_ds *));
void	hpux_to_bsd_oshmid_ds __P((const struct hpux_oshmid_ds *,
	    struct shmid_ds *));

void
bsd_to_hpux_shmid_ds(bp, hp)
	const struct shmid_ds *bp;
	struct hpux_shmid_ds *hp;
{

	bsd_to_hpux_ipc_perm(&bp->shm_perm, &hp->shm_perm);
	hp->shm_segsz = bp->shm_segsz;
	hp->shm_vas = NULL;
	hp->shm_lpid = bp->shm_lpid;
	hp->shm_cpid = bp->shm_cpid;
	hp->shm_nattch = bp->shm_nattch;
	hp->shm_cnattch = bp->shm_nattch;
	hp->shm_atime = bp->shm_atime;
	hp->shm_dtime = bp->shm_dtime;
	hp->shm_ctime = bp->shm_ctime;
}

void
bsd_to_hpux_oshmid_ds(bp, hp)
	const struct shmid_ds *bp;
	struct hpux_oshmid_ds *hp;
{

	bsd_to_hpux_oipc_perm(&bp->shm_perm, &hp->shm_perm);
	hp->shm_segsz = bp->shm_segsz;
	hp->shm_vas = NULL;
	hp->shm_lpid = bp->shm_lpid;
	hp->shm_cpid = bp->shm_cpid;
	hp->shm_nattch = bp->shm_nattch;
	hp->shm_cnattch = bp->shm_nattch;
	hp->shm_atime = bp->shm_atime;
	hp->shm_dtime = bp->shm_dtime;
	hp->shm_ctime = bp->shm_ctime;
}

void
hpux_to_bsd_shmid_ds(hp, bp)
	const struct hpux_shmid_ds *hp;
	struct shmid_ds *bp;
{

	hpux_to_bsd_ipc_perm(&hp->shm_perm, &bp->shm_perm);
	bp->shm_segsz = hp->shm_segsz;
	bp->shm_lpid = hp->shm_lpid;
	bp->shm_cpid = hp->shm_cpid;
	bp->shm_nattch = hp->shm_nattch;
	bp->shm_atime = hp->shm_atime;
	bp->shm_dtime = hp->shm_dtime;
	bp->shm_ctime = hp->shm_ctime;
	bp->_shm_internal = NULL;
}

void
hpux_to_bsd_oshmid_ds(hp, bp)
	const struct hpux_oshmid_ds *hp;
	struct shmid_ds *bp;
{

	hpux_to_bsd_oipc_perm(&hp->shm_perm, &bp->shm_perm);
	bp->shm_segsz = hp->shm_segsz;
	bp->shm_lpid = hp->shm_lpid;
	bp->shm_cpid = hp->shm_cpid;
	bp->shm_nattch = hp->shm_nattch;
	bp->shm_atime = hp->shm_atime;
	bp->shm_dtime = hp->shm_dtime;
	bp->shm_ctime = hp->shm_ctime;
	bp->_shm_internal = NULL;
}

int
hpux_sys_oshmctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct hpux_sys_oshmctl_args /* {
		syscallarg(int) shmid;
		syscallarg(int) cmd;
		syscallarg(struct hpux_oshmid_ds *) buf;
	} */ *uap = v;
	struct shmid_ds shmbuf;
	struct hpux_oshmid_ds hshmbuf;
	int cmd, error;

	cmd = SCARG(uap, cmd);

	if (cmd == IPC_SET) {
		error = copyin(SCARG(uap, buf), &hshmbuf, sizeof(hshmbuf));
		if (error)
			return (error);
		hpux_to_bsd_oshmid_ds(&hshmbuf, &shmbuf);
	}

	error = shmctl1(p, SCARG(uap, shmid), cmd,
	    (cmd == IPC_SET || cmd == IPC_STAT) ? &shmbuf : NULL);

	if (error == 0 && cmd == IPC_STAT) {
		bsd_to_hpux_oshmid_ds(&shmbuf, &hshmbuf);
		error = copyout(&hshmbuf, SCARG(uap, buf), sizeof(hshmbuf));
	}

	return (error);
}

int
hpux_sys_shmctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct hpux_sys_shmctl_args /* {
		syscallarg(int) shmid;
		syscallarg(int) cmd;
		syscallarg(struct hpux_shmid_ds *) buf;
	} */ *uap = v;
	struct shmid_ds shmbuf;
	struct hpux_shmid_ds hshmbuf;
	int cmd, error;

	cmd = SCARG(uap, cmd);

	if (cmd == IPC_SET) {
		error = copyin(SCARG(uap, buf), &hshmbuf, sizeof(hshmbuf));
		if (error)
			return (error);
		hpux_to_bsd_shmid_ds(&hshmbuf, &shmbuf);
	}

	error = shmctl1(p, SCARG(uap, shmid), cmd,
	    (cmd == IPC_SET || cmd == IPC_STAT) ? &shmbuf : NULL);

	if (error == 0 && cmd == IPC_STAT) {
		bsd_to_hpux_shmid_ds(&shmbuf, &hshmbuf);
		error = copyout(&hshmbuf, SCARG(uap, buf), sizeof(hshmbuf));
	}

	return (error);
}
#endif /* SYSVSHM */
