/* Copyright */


#include <errno.h>

#include "sched.h"

/* ARGSUSED */
int
sched_setparam(pid_t pid, const struct sched_param *param)
{
	return ENOSYS;
}

/* ARGSUSED */
int
sched_getparam(pid_t pid, struct sched_param *param)
{
	return ENOSYS;
}

/* ARGSUSED */
int
sched_setscheduler(pid_t pid, int policy,
    const struct sched_param *param)
{
	return ENOSYS;
}

/* ARGSUSED */
int
sched_getscheduler(pid_t pid)
{
	return ENOSYS;
}

/* ARGSUSED */
int
sched_get_priority_max(int policy)
{
	return ENOSYS;
}

/* ARGSUSED */
int
sched_get_priority_min(int policy)
{
	return ENOSYS;
}

/* ARGSUSED */
int
sched_rr_get_interval(pid_t pid, struct timespec *interval)
{
	return ENOSYS;
}



