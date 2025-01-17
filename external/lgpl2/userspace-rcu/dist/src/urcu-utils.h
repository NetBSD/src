// SPDX-FileCopyrightText: 2018 Michael Jeanson <mjeanson@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_UTILS_H
#define _URCU_UTILS_H

/*
 * Userspace RCU library internal utils
 */

#include <urcu/config.h>

#define urcu_stringify(a) _urcu_stringify(a)
#define _urcu_stringify(a) #a

#define max_t(type, x, y)				\
	({						\
		type __max1 = (x);              	\
		type __max2 = (y);              	\
		__max1 > __max2 ? __max1: __max2;	\
	})

#define min_t(type, x, y)				\
	({						\
		type __min1 = (x);              	\
		type __min2 = (y);              	\
		__min1 <= __min2 ? __min1: __min2;	\
	})

#endif /* _URCU_UTILS_H */
