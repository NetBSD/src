/*	$NetBSD: sem.h,v 1.4 2007/06/17 10:23:27 dsl Exp $	*/

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
void	semid_ds14_to_native(struct semid_ds14 *, struct semid_ds *);
void	native_to_semid_ds14(struct semid_ds *, struct semid_ds14 *);

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
