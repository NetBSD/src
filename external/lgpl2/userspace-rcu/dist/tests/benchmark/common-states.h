// SPDX-FileCopyrightText: 2023 Olivier Dion <odion@efficios.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/* Common states for benchmarks. */

#include <unistd.h>

#include <urcu/uatomic.h>

extern volatile int _test_go, _test_stop;

static inline void complete_sleep(unsigned int seconds)
{
	while (seconds != 0) {
		seconds = sleep(seconds);
	}
}

static inline void begin_test(void)
{
	uatomic_store(&_test_go, 1, CMM_RELEASE);
}

static inline void end_test(void)
{
	uatomic_store(&_test_stop, 1, CMM_RELAXED);
}

static inline void test_for(unsigned int duration)
{
	begin_test();
	complete_sleep(duration);
	end_test();
}

static inline void wait_until_go(void)
{
	while (!uatomic_load(&_test_go, CMM_ACQUIRE))
	{
	}
}

/*
 * returns 0 if test should end.
 */
static inline int test_duration_write(void)
{
	return !uatomic_load(&_test_stop, CMM_RELAXED);
}

static inline int test_duration_read(void)
{
	return !uatomic_load(&_test_stop, CMM_RELAXED);
}
