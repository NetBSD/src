// SPDX-FileCopyrightText: 2023 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_POLL_H
#define _URCU_POLL_H

/*
 * Userspace RCU polling header
 */

struct urcu_gp_poll_state {
	unsigned long grace_period_id;
};

#endif /* _URCU_POLL_H */
