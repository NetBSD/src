// SPDX-FileCopyrightText: 2012 Lai Jiangshan <laijs@cn.fujitsu.com>
// SPDX-FileCopyrightText: 2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Userspace RCU library - test multiple RCU flavors into one program
 */

extern int test_mf_memb(void);
extern int test_mf_mb(void);
extern int test_mf_qsbr(void);
extern int test_mf_bp(void);

