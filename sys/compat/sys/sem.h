/*	$NetBSD: sem.h,v 1.4.30.1 2008/03/29 20:46:59 christos Exp $	*/

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
	int32_t		sem_otime;	/* last operation time */
	long		sem_pad1;	/* SVABI/386 says I need this here */
	int32_t		sem_ctime;	/* last change time */
    					/* Times measured in secs since */
    					/* 00:00:00 GMT, Jan. 1, 1970 */
	long		sem_pad2;	/* SVABI/386 says I need this here */
	long		sem_pad3[4];	/* SVABI/386 says I need this here */
};

struct semid_ds13 {
	struct ipc_perm	sem_perm;	/* operation permission structure */
	unsigned short	sem_nsems;	/* number of semaphores in set */
	int32_t		sem_otime;	/* last semop() time */
	int32_t		sem_ctime;	/* last time changed by semctl() */

	/*
	 * These members are private and used only in the internal
	 * implementation of this interface.
	 */
	struct __sem	*_sem_base;	/* pointer to first semaphore in set */
};
void	semid_ds14_to_native(struct semid_ds14 *, struct semid_ds *);
void	native_to_semid_ds14(struct semid_ds *, struct semid_ds14 *);
void	semid_ds13_to_native(struct semid_ds13 *, struct semid_ds *);
void	native_to_semid_ds13(struct semid_ds *, struct semid_ds13 *);

#else /* !_KERNEL */

__BEGIN_DECLS
int	semctl(int, int, int, union __semun);
int	__semctl(int, int, int, union __semun *);
int	__semctl13(int, int, int, ...);
int	__semctl14(int, int, int, ...);
__END_DECLS

#endif /* !_KERNEL */

#endif /* !_COMPAT_SYS_SEM_H_ */
