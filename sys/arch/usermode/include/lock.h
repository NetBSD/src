/* $NetBSD: lock.h,v 1.3.12.1 2017/12/03 11:36:47 jdolecek Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARCH_USERMODE_INCLUDE_LOCK_H
#define _ARCH_USERMODE_INCLUDE_LOCK_H

__inline static void
__cpu_simple_lock_init(__cpu_simple_lock_t *lockp)
{
	*lockp = __SIMPLELOCK_UNLOCKED;
}

__inline static int
__cpu_simple_lock_try(__cpu_simple_lock_t *lockp)
{
	if (*lockp == __SIMPLELOCK_LOCKED)
		return 0;
	*lockp = __SIMPLELOCK_LOCKED;
	return 1;
}

__inline static void
__cpu_simple_lock(__cpu_simple_lock_t *lockp)
{
	while (!__cpu_simple_lock_try(lockp))
		;
}

__inline static void
__cpu_simple_unlock(__cpu_simple_lock_t *lockp)
{
	*lockp = __SIMPLELOCK_UNLOCKED;
}

__inline static int
__SIMPLELOCK_LOCKED_P(const __cpu_simple_lock_t *lockp)
{
	return *lockp == __SIMPLELOCK_LOCKED;
}

__inline static int
__SIMPLELOCK_UNLOCKED_P(const __cpu_simple_lock_t *lockp)
{
	return *lockp == __SIMPLELOCK_UNLOCKED;
}

#endif /* !_ARCH_USERMODE_INCLUDE_LOCK_H */
