
/* Copyright */

#ifndef _LIB_PTHREAD_SCHED_H
#define _LIB_PTHREAD_SCHED_H

/* Required by POSIX 1003.1, section 13.1, lines 12-13. */
#include <time.h>	

struct sched_param {
	int	sched_priority;
};

/* Scheduling policicies */
/* Required by POSIX: */
#define	SCHED_FIFO	1
#define SCHED_RR	2
#define SCHED_DEFAULT	3 /* Behavior can be like FIFO or RR */

/* Other nonstandard policies: */

/* Functions */

/* 
 * These are permitted to fail and return ENOSYS if
 * _POSIX_PRIORITY_SCHEDULING is not defined.
 */
int	sched_setparam(pid_t pid, const struct sched_param *param);
int	sched_getparam(pid_t pid, struct sched_param *param);
int	sched_setscheduler(pid_t pid, int policy,
    const struct sched_param *param);
int	sched_getscheduler(pid_t pid);
int	sched_get_priority_max(int policy);
int	sched_get_priority_min(int policy);
int	sched_rr_get_interval(pid_t pid, struct timespec *interval);


/* Not optional in the presence of _POSIX_THREADS */
int	sched_yield(void);

#endif /* _LIB_PTHREAD_SCHED_H */
