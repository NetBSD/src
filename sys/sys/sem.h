/*	$NetBSD: sem.h,v 1.13 2002/03/17 22:21:59 christos Exp $	*/

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
 * SVID compatible sem.h file
 *
 * Author: Daniel Boulet
 */

#ifndef _SYS_SEM_H_
#define _SYS_SEM_H_

#include <sys/featuretest.h>

#include <sys/ipc.h>

#ifdef _KERNEL
struct __sem {
	unsigned short	semval;		/* semaphore value */
	pid_t		sempid;		/* pid of last operation */
	unsigned short	semncnt;	/* # awaiting semval > cval */
	unsigned short	semzcnt;	/* # awaiting semval = 0 */
};
#endif /* _KERNEL */

struct semid_ds {
	struct ipc_perm	sem_perm;	/* operation permission structure */
	unsigned short	sem_nsems;	/* number of semaphores in set */
	time_t		sem_otime;	/* last semop() time */
	time_t		sem_ctime;	/* last time changed by semctl() */

	/*
	 * These members are private and used only in the internal
	 * implementation of this interface.
	 */
	struct __sem	*_sem_base;	/* pointer to first semaphore in set */
};

#ifdef _KERNEL
struct semid_ds14 {
	struct ipc_perm14 sem_perm;	/* operation permission struct */
	struct __sem	*sem_base;	/* pointer to first semaphore in set */
	unsigned short	sem_nsems;	/* number of sems in set */
	time_t		sem_otime;	/* last operation time */
	long		sem_pad1;	/* SVABI/386 says I need this here */
	time_t		sem_ctime;	/* last change time */
    					/* Times measured in secs since */
    					/* 00:00:00 GMT, Jan. 1, 1970 */
	long		sem_pad2;	/* SVABI/386 says I need this here */
	long		sem_pad3[4];	/* SVABI/386 says I need this here */
};
#endif /* _KERNEL */

/*
 * semop's sops parameter structure
 */
struct sembuf {
	unsigned short	sem_num;	/* semaphore # */
	short		sem_op;		/* semaphore operation */
	short		sem_flg;	/* operation flags */
};
#define SEM_UNDO	010000		/* undo changes on process exit */

/*
 * commands for semctl
 */
#define GETNCNT	3	/* Return the value of semncnt {READ} */
#define GETPID	4	/* Return the value of sempid {READ} */
#define GETVAL	5	/* Return the value of semval {READ} */
#define GETALL	6	/* Return semvals into arg.array {READ} */
#define GETZCNT	7	/* Return the value of semzcnt {READ} */
#define SETVAL	8	/* Set the value of semval to arg.val {ALTER} */
#define SETALL	9	/* Set semvals from arg.array {ALTER} */

#ifdef _KERNEL
/*
 * Kernel implementation stuff
 */
#define SEMVMX	32767		/* semaphore maximum value */
#define SEMAEM	16384		/* adjust on exit max value */

#define MAX_SOPS	5	/* maximum # of sembuf's per semop call */

/*
 * Permissions
 */
#define SEM_A		0200	/* alter permission */
#define SEM_R		0400	/* read permission */

/*
 * Undo structure (one per process)
 */
struct sem_undo {
	struct	sem_undo *un_next;	/* ptr to next active undo structure */
	struct	proc *un_proc;		/* owner of this structure */
	short	un_cnt;			/* # of active entries */
	struct undo {
		short	un_adjval;	/* adjust on exit values */
		short	un_num;		/* semaphore # */
		int	un_id;		/* semid */
	} un_ent[1];			/* undo entries */
};
#endif /* _KERNEL */

#if !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)
/*
 * semaphore info struct
 */
struct seminfo {
	int32_t	semmap;		/* # of entries in semaphore map */
	int32_t	semmni;		/* # of semaphore identifiers */
	int32_t	semmns;		/* # of semaphores in system */
	int32_t	semmnu;		/* # of undo structures in system */
	int32_t	semmsl;		/* max # of semaphores per id */
	int32_t	semopm;		/* max # of operations per semop call */
	int32_t	semume;		/* max # of undo entries per process */
	int32_t	semusz;		/* size in bytes of undo structure */
	int32_t	semvmx;		/* semaphore maximum value */
	int32_t	semaem;		/* adjust on exit max value */
};

/* Warning: 64-bit structure padding is needed here */
struct semid_ds_sysctl {
	struct	ipc_perm_sysctl sem_perm;
	int16_t	sem_nsems;
	int16_t	pad2;
	int32_t	pad3;
	time_t	sem_otime;
	time_t	sem_ctime;
};
struct sem_sysctl_info {
	struct	seminfo seminfo;
	struct	semid_ds_sysctl semids[1];
};

/*
 * Internal "mode" bits.  The first of these is used by ipcs(1), and so
 * is defined outside the kernel as well.
 */
#define	SEM_ALLOC	01000	/* semaphore is allocated */
#endif /* !_POSIX_C_SOURCE && !_XOPEN_SOURCE */

#ifdef _KERNEL
#define	SEM_DEST	02000	/* semaphore will be destroyed on last detach */

/*
 * Configuration parameters
 */
#ifndef SEMMNI
#define SEMMNI	10		/* # of semaphore identifiers */
#endif
#ifndef SEMMNS
#define SEMMNS	60		/* # of semaphores in system */
#endif
#ifndef SEMUME
#define SEMUME	10		/* max # of undo entries per process */
#endif
#ifndef SEMMNU
#define SEMMNU	30		/* # of undo structures in system */
#endif

/* shouldn't need tuning */
#ifndef SEMMAP
#define SEMMAP	30		/* # of entries in semaphore map */
#endif
#ifndef SEMMSL
#define SEMMSL	SEMMNS		/* max # of semaphores per id */
#endif
#ifndef SEMOPM
#define SEMOPM	100		/* max # of operations per semop call */
#endif

/* actual size of an undo structure */
#define SEMUSZ	(sizeof(struct sem_undo)+sizeof(struct undo)*SEMUME)

/*
 * Structures allocated in machdep.c
 */
extern struct seminfo seminfo;
extern struct semid_ds *sema;		/* semaphore id pool */
extern struct __sem *sem;		/* semaphore pool */
extern struct map *semmap;		/* semaphore allocation map */
extern struct sem_undo *semu_list;	/* list of active undo structures */
extern int *semu;			/* undo structure pool */

/*
 * Macro to find a particular sem_undo vector
 */
#define SEMU(ix)	((struct sem_undo *)(((long)semu)+ix * SEMUSZ))

/*
 * Parameters to the semconfig system call
 */
#define	SEM_CONFIG_FREEZE	0	/* Freeze the semaphore facility. */
#define	SEM_CONFIG_THAW		1	/* Thaw the semaphore facility. */
#endif /* _KERNEL */

#ifndef _KERNEL
#include <sys/cdefs.h>

__BEGIN_DECLS
#if defined(__LIBC12_SOURCE__)
int	semctl __P((int, int, int, union __semun));
int	__semctl __P((int, int, int, union __semun *));
int	__semctl13 __P((int, int, int, ...));
#else
int	semctl __P((int, int, int, ...)) __RENAME(__semctl13);
#endif
int	semget __P((key_t, int, int));
int	semop __P((int, struct sembuf *, size_t));
#if !defined(_XOPEN_SOURCE)
int	semconfig __P((int));
#endif
__END_DECLS
#else
void	seminit __P((void));
void	semexit __P((struct proc *, void *));

int	semctl1 __P((struct proc *, int, int, int, void *, register_t *));
#endif /* !_KERNEL */

#endif /* !_SEM_H_ */
