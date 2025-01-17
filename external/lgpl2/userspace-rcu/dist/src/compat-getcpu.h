// SPDX-FileCopyrightText: 2015 Michael Jeanson <mjeanson@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _COMPAT_GETCPU_H
#define _COMPAT_GETCPU_H

#if defined(HAVE_SCHED_GETCPU)
#include <sched.h>

static inline
int urcu_sched_getcpu(void)
{
	return sched_getcpu();
}
#elif defined(HAVE_GETCPUID)
#include <sys/processor.h>

static inline
int urcu_sched_getcpu(void)
{
	return (int) getcpuid();
}
#else

static inline
int urcu_sched_getcpu(void)
{
        return -1;
}
#endif

#endif /* _COMPAT_GETCPU_H */
