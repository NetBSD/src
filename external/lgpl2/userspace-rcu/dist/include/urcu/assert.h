// SPDX-FileCopyrightText: 2021 Francis Deslauriers <francis.deslauriers@efficios.com>
//
// SPDX-License-Identifier: MIT

#ifndef _URCU_ASSERT_H
#define _URCU_ASSERT_H

/*
 * Userspace RCU assertion facilities.
 */

#include <urcu/config.h>

/*
 * Force usage of an expression to prevent unused expression compiler warning.
 */
#define _urcu_use_expression(_expr) ((void) sizeof((void) (_expr), 0))

#ifdef NDEBUG
/*
 * Vanilla assert() replacement. When NDEBUG is defined, the expression is
 * consumed to prevent unused variable compile warnings.
 */
# define urcu_posix_assert(_cond) _urcu_use_expression(_cond)
#else
# include <assert.h>
# define urcu_posix_assert(_cond) assert(_cond)
#endif

#if defined(DEBUG_RCU) || defined(CONFIG_RCU_DEBUG)

/*
 * Enables debugging/expensive assertions to be used in fast paths and only
 * enabled on demand. When disabled, the expression is consumed to prevent
 * unused variable compile warnings.
 */
# define urcu_assert_debug(_cond) urcu_posix_assert(_cond)
#else
# define urcu_assert_debug(_cond) _urcu_use_expression(_cond)
#endif

#endif /* _URCU_ASSERT_H */
