/*	$NetBSD: shm.h,v 1.24 1999/12/04 12:33:03 ragge Exp $	*/

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
#define	SHMLBA		NBPG	/* Segment low boundry address multiple XXX */

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

#if !defined(_XOPEN_SOURCE)
/*
 * Some systems (e.g. HP-UX) take these as the second (cmd) arg to shmctl().
 * XXX Currently not implemented.
 */
#define	SHM_LOCK	3	/* Lock segment in memory. */
#define	SHM_UNLOCK	4	/* Unlock a segment locked by SHM_LOCK. */
#endif /* _XOPEN_SOURCE */

/*
 * System 5 style catch-all structure for shared memory constants that
 * might be of interest to user programs.  Do we really want/need this?
 */
struct shminfo {
	int	shmmax;		/* max shared memory segment size (bytes) */
	int	shmmin;		/* min shared memory segment size (bytes) */
	int	shmmni;		/* max number of shared memory identifiers */
	int	shmseg;		/* max shared memory segments per process */
	int	shmall;		/* max amount of shared memory (pages) */
};
struct shminfo shminfo;
struct shmid_ds *shmsegs;

struct vmspace;

void	shminit __P((void));
void	shmfork __P((struct vmspace *, struct vmspace *));
void	shmexit __P((struct vmspace *));
int	shmctl1 __P((struct proc *, int, int, struct shmid_ds *));

void	shmid_ds14_to_native __P((struct shmid_ds14 *, struct shmid_ds *));
void	native_to_shmid_ds14 __P((struct shmid_ds *, struct shmid_ds14 *));
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
