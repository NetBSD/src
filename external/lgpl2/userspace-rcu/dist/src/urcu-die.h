// SPDX-FileCopyrightText: 2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_DIE_H
#define _URCU_DIE_H

/*
 * Userspace RCU library unrecoverable error handling
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define urcu_die(cause)								\
do {										\
	fprintf(stderr, "(" __FILE__ ":%s@%u) Unrecoverable error: %s\n",	\
		__func__, __LINE__, strerror(cause));				\
	abort();								\
} while (0)

#endif /* _URCU_DIE_H */
