
/* Copyright */

#ifndef _LIB_PTHREAD_MUTEX_H
#define _LIB_PTHREAD_MUTEX_H

struct	pthread_mutex_st {
	unsigned int	ptm_magic;

	pt_spin_t	ptm_interlock;
	unsigned int	ptm_locked;

	pthread_t	ptm_owner;
	SIMPLEQ_HEAD(, pthread_st)	ptm_blocked;
};


struct	pthread_mutexattr_st {

};

#define	PT_MUTEX_MAGIC	0xCAFEFEED
#define	PT_MUTEX_DEAD	0xFEEDDEAD

#define PT_MUTEX_UNLOCKED	0xAAAAAAAA
#define PT_MUTEX_LOCKED		0x55555555

#endif	/* _LIB_PTHREAD_MUTEX_H */
