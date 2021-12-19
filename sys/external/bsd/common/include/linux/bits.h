// SPDX-License-Identifier: MIT
/*
 * Copyright © 2020 The NetBSD Foundation, Inc.
 */

#ifndef _LINUX_BITS_H_
#define _LINUX_BITS_H_

#include <sys/param.h>
#include <sys/cdefs.h>

#define BIT_MASK(__n) __BITS(__n-1,0)

#endif /* _LINUX_BITS_H_ */
