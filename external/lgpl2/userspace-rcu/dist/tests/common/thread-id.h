// SPDX-FileCopyrightText: 2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LicenseRef-Boehm-GC

#ifndef _TEST_THREAD_ID_H
#define _TEST_THREAD_ID_H

/*
 * Userspace RCU library - thread ID
 */

#ifdef __linux__
# include <urcu/syscall-compat.h>

# if defined(HAVE_GETTID)
/*
 * Do not redefine gettid() as it is already included
 * in bionic through <unistd.h>. Some other libc
 * may also already contain an implementation of gettid.
 */
# elif defined(_syscall0)
_syscall0(pid_t, gettid)
# elif defined(__NR_gettid)
static inline pid_t gettid(void)
{
	return syscall(__NR_gettid);
}
# endif

static inline
unsigned long urcu_get_thread_id(void)
{
	return (unsigned long) gettid();
}
#elif defined(__FreeBSD__)
# include <pthread_np.h>

static inline
unsigned long urcu_get_thread_id(void)
{
	return (unsigned long) pthread_getthreadid_np();
}
#elif defined(__sun__) || defined(__APPLE__)
#include <pthread.h>

static inline
unsigned long urcu_get_thread_id(void)
{
	return (unsigned long) pthread_self();
}
#elif defined(__CYGWIN__)
#include <pthread.h>

extern unsigned long pthread_getsequence_np(pthread_t *);

static inline
unsigned long urcu_get_thread_id(void)
{
	pthread_t thr = pthread_self();
	return pthread_getsequence_np(&thr);
}
#elif defined(__OpenBSD__)
#include <unistd.h>

static inline
unsigned long urcu_get_thread_id(void)
{
	return (unsigned long) getthrid();
}
#else
# warning "use pid as thread ID"
static inline
unsigned long urcu_get_thread_id(void)
{
	return (unsigned long) getpid();
}
#endif

#endif /* _TEST_THREAD_ID_H */
