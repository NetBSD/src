/*	$NetBSD: iosf_mbi.h,v 1.2 2021/12/19 11:49:12 riastradh Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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

#ifndef _LINUX_ASM_IOSF_MBI_H_
#define _LINUX_ASM_IOSF_MBI_H_

struct notifier_block;

enum {
	MBI_PMIC_BUS_ACCESS_BEGIN,
	MBI_PMIC_BUS_ACCESS_END,
};

static inline void
iosf_mbi_punit_acquire(void)
{
}

static inline void
iosf_mbi_punit_release(void)
{
}

static inline void
iosf_mbi_assert_punit_acquired(void)
{
}

static inline void
iosf_mbi_register_pmic_bus_access_notifier(struct notifier_block *notifier)
{
}

static inline void
iosf_mbi_unregister_pmic_bus_access_notifier_unlocked(
    struct notifier_block *notifier)
{
}

#endif  /* _LINUX_ASM_IOSF_MBI_H_ */
