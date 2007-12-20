/*	$NetBSD: netbsd32_compat_14.c,v 1.21 2007/12/20 23:03:01 dsl Exp $	*/

/*
 * Copyright (c) 1999 Eduardo E. Horvath
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_14.c,v 1.21 2007/12/20 23:03:01 dsl Exp $");

#include <sys/param.h>
#include <sys/ipc.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>

#ifndef	SYSVMSG
#define	SYSVMSG
#endif
#ifndef	SYSVSEM
#define	SYSVSEM
#endif
#ifndef	SYSVSHM
#define	SYSVSHM
#endif

#include <sys/syscallargs.h>
#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/sys/shm.h>

static inline void
netbsd32_ipc_perm14_to_native(struct netbsd32_ipc_perm14 *, struct ipc_perm *);
static inline void
native_to_netbsd32_ipc_perm14(struct ipc_perm *, struct netbsd32_ipc_perm14 *);
static inline void
native_to_netbsd32_msqid_ds14(struct msqid_ds *, struct netbsd32_msqid_ds14 *);
static inline void
netbsd32_msqid_ds14_to_native(struct netbsd32_msqid_ds14 *, struct msqid_ds *);
static inline void
native_to_netbsd32_semid_ds14(struct semid_ds *, struct netbsd32_semid_ds14 *);
static inline void
netbsd32_semid_ds14_to_native(struct netbsd32_semid_ds14 *, struct semid_ds *);
static inline void
netbsd32_shmid_ds14_to_native(struct netbsd32_shmid_ds14 *, struct shmid_ds *);
static inline void
native_to_netbsd32_shmid_ds14(struct shmid_ds *, struct netbsd32_shmid_ds14 *);

static inline void
netbsd32_ipc_perm14_to_native(struct netbsd32_ipc_perm14 *operm, struct ipc_perm *perm)
{

#define	CVT(x)	perm->x = operm->x
	CVT(uid);
	CVT(gid);
	CVT(cuid);
	CVT(cgid);
	CVT(mode);
#undef CVT
}

static inline void
native_to_netbsd32_ipc_perm14(struct ipc_perm *perm, struct netbsd32_ipc_perm14 *operm)
{

#define	CVT(x)	operm->x = perm->x
	CVT(uid);
	CVT(gid);
	CVT(cuid);
	CVT(cgid);
	CVT(mode);
#undef CVT

	/*
	 * Not part of the API, but some programs might look at it.
	 */
	operm->seq = perm->_seq;
	operm->key = (key_t)perm->_key;
}

static inline void
netbsd32_msqid_ds14_to_native(struct netbsd32_msqid_ds14 *omsqbuf, struct msqid_ds *msqbuf)
{

	netbsd32_ipc_perm14_to_native(&omsqbuf->msg_perm, &msqbuf->msg_perm);

#define	CVT(x)	msqbuf->x = omsqbuf->x
	CVT(msg_qnum);
	CVT(msg_qbytes);
	CVT(msg_lspid);
	CVT(msg_lrpid);
	CVT(msg_stime);
	CVT(msg_rtime);
	CVT(msg_ctime);
#undef CVT
}

static inline void
native_to_netbsd32_msqid_ds14(struct msqid_ds *msqbuf, struct netbsd32_msqid_ds14 *omsqbuf)
{

	native_to_netbsd32_ipc_perm14(&msqbuf->msg_perm, &omsqbuf->msg_perm);

#define	CVT(x)	omsqbuf->x = msqbuf->x
	CVT(msg_qnum);
	CVT(msg_qbytes);
	CVT(msg_lspid);
	CVT(msg_lrpid);
	CVT(msg_stime);
	CVT(msg_rtime);
	CVT(msg_ctime);
#undef CVT

	/*
	 * Not part of the API, but some programs might look at it.
	 */
	omsqbuf->msg_cbytes = msqbuf->_msg_cbytes;
}

static inline void
netbsd32_semid_ds14_to_native(struct netbsd32_semid_ds14 *osembuf, struct semid_ds *sembuf)
{

	netbsd32_ipc_perm14_to_native(&osembuf->sem_perm, &sembuf->sem_perm);

#define	CVT(x)	sembuf->x = osembuf->x
	CVT(sem_nsems);
	CVT(sem_otime);
	CVT(sem_ctime);
#undef CVT
}

static inline void
native_to_netbsd32_semid_ds14(struct semid_ds *sembuf, struct netbsd32_semid_ds14 *osembuf)
{

	native_to_netbsd32_ipc_perm14(&sembuf->sem_perm, &osembuf->sem_perm);

#define	CVT(x)	osembuf->x = sembuf->x
	CVT(sem_nsems);
	CVT(sem_otime);
	CVT(sem_ctime);
#undef CVT
}

static inline void
netbsd32_shmid_ds14_to_native(struct netbsd32_shmid_ds14 *oshmbuf, struct shmid_ds *shmbuf)
{

	netbsd32_ipc_perm14_to_native(&oshmbuf->shm_perm, &shmbuf->shm_perm);

#define	CVT(x)	shmbuf->x = oshmbuf->x
	CVT(shm_segsz);
	CVT(shm_lpid);
	CVT(shm_cpid);
	CVT(shm_nattch);
	CVT(shm_atime);
	CVT(shm_dtime);
	CVT(shm_ctime);
#undef CVT
}

static inline void
native_to_netbsd32_shmid_ds14(struct shmid_ds *shmbuf, struct netbsd32_shmid_ds14 *oshmbuf)
{

	native_to_netbsd32_ipc_perm14(&shmbuf->shm_perm, &oshmbuf->shm_perm);

#define	CVT(x)	oshmbuf->x = shmbuf->x
	CVT(shm_segsz);
	CVT(shm_lpid);
	CVT(shm_cpid);
	CVT(shm_nattch);
	CVT(shm_atime);
	CVT(shm_dtime);
	CVT(shm_ctime);
#undef CVT
}

/*
 * the compat_14 system calls
 */
int
compat_14_netbsd32_msgctl(struct lwp *l, const struct compat_14_netbsd32_msgctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) msqid;
		syscallarg(int) cmd;
		syscallarg(struct msqid_ds14 *) buf;
	} */
	struct msqid_ds msqbuf;
	struct netbsd32_msqid_ds14 omsqbuf;
	int cmd, error;

	cmd = SCARG(uap, cmd);

	if (cmd == IPC_SET) {
		error = copyin(SCARG_P32(uap, buf),
		    &omsqbuf, sizeof(omsqbuf));
		if (error)
			return (error);
		netbsd32_msqid_ds14_to_native(&omsqbuf, &msqbuf);
	}

	error = msgctl1(l, SCARG(uap, msqid), cmd,
	    (cmd == IPC_SET || cmd == IPC_STAT) ? &msqbuf : NULL);

	if (error == 0 && cmd == IPC_STAT) {
		native_to_netbsd32_msqid_ds14(&msqbuf, &omsqbuf);
		error = copyout(&omsqbuf,
		    SCARG_P32(uap, buf), sizeof(omsqbuf));
	}

	return (error);
}

int
compat_14_netbsd32___semctl(struct lwp *l, const struct compat_14_netbsd32___semctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) semid;
		syscallarg(int) semnum;
		syscallarg(int) cmd;
		syscallarg(union __semun *) arg;
	} */
	union __semun arg;
	struct semid_ds sembuf;
	struct netbsd32_semid_ds14 osembuf;
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
		pass_arg = &arg;
		break;
	}

	if (pass_arg != NULL) {
		error = copyin(NETBSD32IPTR64(SCARG(uap, arg)), &arg,
		    sizeof(arg));
		if (error)
			return (error);
		if (cmd == IPC_SET) {
			error = copyin(arg.buf, &osembuf, sizeof(osembuf));
			if (error)
				return (error);
			netbsd32_semid_ds14_to_native(&osembuf, &sembuf);
		}
	}

	error = semctl1(l, SCARG(uap, semid), SCARG(uap, semnum), cmd,
	    pass_arg, retval);

	if (error == 0 && cmd == IPC_STAT) {
		native_to_netbsd32_semid_ds14(&sembuf, &osembuf);
		error = copyout(&osembuf, arg.buf, sizeof(osembuf));
	}

	return (error);
}

int
compat_14_netbsd32_shmctl(struct lwp *l, const struct compat_14_netbsd32_shmctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) shmid;
		syscallarg(int) cmd;
		syscallarg(struct netbsd32_shmid_ds14 *) buf;
	} */
	struct shmid_ds shmbuf;
	struct netbsd32_shmid_ds14 oshmbuf;
	int cmd, error;

	cmd = SCARG(uap, cmd);

	if (cmd == IPC_SET) {
		error = copyin(SCARG_P32(uap, buf), &oshmbuf, sizeof(oshmbuf));
		if (error)
			return (error);
		netbsd32_shmid_ds14_to_native(&oshmbuf, &shmbuf);
	}

	error = shmctl1(l, SCARG(uap, shmid), cmd,
	    (cmd == IPC_SET || cmd == IPC_STAT) ? &shmbuf : NULL);

	if (error == 0 && cmd == IPC_STAT) {
		native_to_netbsd32_shmid_ds14(&shmbuf, &oshmbuf);
		error = copyout(&oshmbuf, SCARG_P32(uap, buf), sizeof(oshmbuf));
	}

	return (error);
}
