/*	$NetBSD: sem.h,v 1.3 2005/11/12 00:39:22 simonb Exp $	*/

/*
 * SVID compatible sem.h file
 *
 * Author: Daniel Boulet
 */

#ifndef _COMPAT_SYS_SEM_H_
#define _COMPAT_SYS_SEM_H_

#ifdef _KERNEL

#include <compat/sys/ipc.h>

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

#else /* !_KERNEL */

__BEGIN_DECLS
int	semctl(int, int, int, union __semun);
int	__semctl(int, int, int, union __semun *);
int	__semctl13(int, int, int, ...);
#if defined(_NETBSD_SOURCE)
int	semconfig(int);
#endif
__END_DECLS

#endif /* !_KERNEL */

#endif /* !_COMPAT_SYS_SEM_H_ */
