/*	$NetBSD: shm.h,v 1.33 2003/05/30 20:31:34 kleink Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1994 Adam Glass
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
 *      This product includes software developed by Adam Glass.
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

/*
 * As defined+described in "X/Open System Interfaces and Headers"
 *                         Issue 4, p. XXX
 */

#ifndef _SYS_SHM_H_
#define _SYS_SHM_H_

#include <sys/featuretest.h>

#include <sys/ipc.h>

#define	SHM_RDONLY	010000	/* Attach read-only (else read-write) */
#define	SHM_RND		020000	/* Round attach address to SHMLBA */
/* Segment low boundry address multiple */
#if defined(_KERNEL) || defined(_STANDALONE) || defined(_LKM)
#define	SHMLBA		PAGE_SIZE
#else
/* Use libc's internal __sysconf() to retrieve the machine's page size */
#include <sys/unistd.h> /* for _SC_PAGESIZE */
long			__sysconf __P((int));
#define	SHMLBA		(__sysconf(_SC_PAGESIZE))
#endif

typedef unsigned int	shmatt_t;

struct shmid_ds {
	struct ipc_perm	shm_perm;	/* operation permission structure */
	size_t		shm_segsz;	/* size of segment in bytes */
	pid_t		shm_lpid;	/* process ID of last shm operation */
	pid_t		shm_cpid;	/* process ID of creator */
	shmatt_t	shm_nattch;	/* number of current attaches */
	time_t		shm_atime;	/* time of last shmat() */
	time_t		shm_dtime;	/* time of last shmdt() */
	time_t		shm_ctime;	/* time of last change by shmctl() */

	/*
	 * These members are private and used only in the internal
	 * implementation of this interface.
	 */
	void		*_shm_internal;
};

#ifdef _KERNEL
struct shmid_ds14 {
	struct ipc_perm14 shm_perm;	/* operation permission structure */
	int		shm_segsz;	/* size of segment in bytes */
	pid_t		shm_lpid;	/* process ID of last shm op */
	pid_t		shm_cpid;	/* process ID of creator */
	short		shm_nattch;	/* number of current attaches */
	time_t		shm_atime;	/* time of last shmat() */
	time_t		shm_dtime;	/* time of last shmdt() */
	time_t		shm_ctime;	/* time of last change by shmctl() */
	void		*shm_internal;	/* sysv stupidity */
};
#endif /* _KERNEL */

#if defined(_NETBSD_SOURCE)
/*
 * Some systems (e.g. HP-UX) take these as the second (cmd) arg to shmctl().
 * XXX Currently not implemented.
 */
#define	SHM_LOCK	3	/* Lock segment in memory. */
#define	SHM_UNLOCK	4	/* Unlock a segment locked by SHM_LOCK. */
#endif /* _NETBSD_SOURCE */

#if defined(_NETBSD_SOURCE)
/*
 * Permission definitions used in shmflag arguments to shmat(2) and shmget(2).
 * Provided for source compatibility only; do not use in new code!
 */
#define	SHM_R		IPC_R	/* S_IRUSR, R for owner */
#define	SHM_W		IPC_W	/* S_IWUSR, W for owner */

/*
 * System 5 style catch-all structure for shared memory constants that
 * might be of interest to user programs.  Do we really want/need this?
 */
struct shminfo {
	int32_t	shmmax;		/* max shared memory segment size (bytes) */
	int32_t	shmmin;		/* min shared memory segment size (bytes) */
	int32_t	shmmni;		/* max number of shared memory identifiers */
	int32_t	shmseg;		/* max shared memory segments per process */
	int32_t	shmall;		/* max amount of shared memory (pages) */
};

/* Warning: 64-bit structure padding is needed here */
struct shmid_ds_sysctl {
	struct		ipc_perm_sysctl shm_perm;
	u_int64_t	shm_segsz;
	pid_t		shm_lpid;
	pid_t		shm_cpid;
	time_t		shm_atime;
	time_t		shm_dtime;
	time_t		shm_ctime;
	u_int32_t	shm_nattch;
};
struct shm_sysctl_info {
	struct	shminfo shminfo;
	int32_t	pad;	/* shminfo not a multiple of 64 bits */
	struct	shmid_ds_sysctl shmids[1];
};
#endif /* _NETBSD_SOURCE */

#ifdef _KERNEL
extern struct shminfo shminfo;
extern struct shmid_ds *shmsegs;

struct vmspace;

void	shminit __P((void));
void	shmfork __P((struct vmspace *, struct vmspace *));
void	shmexit __P((struct vmspace *));
int	shmctl1 __P((struct proc *, int, int, struct shmid_ds *));
int	shmat1 __P((struct proc *, int, const void *, int, vaddr_t *, int));
#else /* !_KERNEL */

#include <sys/cdefs.h>

__BEGIN_DECLS
void	*shmat __P((int, const void *, int));
int	shmctl __P((int, int, struct shmid_ds *)) __RENAME(__shmctl13);
int	shmdt __P((const void *));
int	shmget __P((key_t, size_t, int));
__END_DECLS

#endif /* !_KERNEL */

#endif /* !_SYS_SHM_H_ */
