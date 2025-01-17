// SPDX-FileCopyrightText: 2015 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: MIT

#ifndef _URCU_DEBUG_H
#define _URCU_DEBUG_H

/*
 * Userspace RCU debugging facilities.
 */

#include <urcu/assert.h>

/*
 * For backward compatibility reasons, this file must expose the urcu_assert()
 * macro.
 */
#define urcu_assert(_cond) urcu_assert_debug(_cond)

#endif /* _URCU_DEBUG_H */
