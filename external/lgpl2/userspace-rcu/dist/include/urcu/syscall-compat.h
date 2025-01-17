// SPDX-FileCopyrightText: 2013 Pierre-Luc St-Charles <pierre-luc.st-charles@polymtl.ca>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_SYSCALL_COMPAT_H
#define _URCU_SYSCALL_COMPAT_H

/*
 * Userspace RCU library - Syscall Compatibility Header
 */

#if defined(__ANDROID__) || defined(__sun__) || defined(__GNU__)
#include <sys/syscall.h>
#elif defined(__linux__) || defined(__GLIBC__)
#include <syscall.h>

#elif defined(__CYGWIN__) || defined(__APPLE__) || \
	defined(__FreeBSD__) || defined(__DragonFly__) || \
	defined(__OpenBSD__)
/* Don't include anything on these platforms. */

#else
#error "Add platform support to urcu/syscall-compat.h"
#endif

#endif /* _URCU_SYSCALL_COMPAT_H */
