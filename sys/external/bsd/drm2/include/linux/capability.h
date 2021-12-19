/*	$NetBSD: capability.h,v 1.3 2021/12/19 01:22:01 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#ifndef _LINUX_CAPABILITY_H_
#define _LINUX_CAPABILITY_H_

#include <sys/kauth.h>

enum linux_capability {
	LINUX_CAP_SYS_ADMIN,
	LINUX_CAP_SYS_NICE,
#define	CAP_SYS_ADMIN	LINUX_CAP_SYS_ADMIN
#define	CAP_SYS_NICE	LINUX_CAP_SYS_NICE
};

static inline bool
capable(enum linux_capability cap)
{
	switch (cap) {
	case CAP_SYS_ADMIN:
	case CAP_SYS_NICE:
		return (kauth_authorize_generic(kauth_cred_get(),
			KAUTH_GENERIC_ISSUSER, NULL) == 0);
	default:
		panic("unknown cap %d", cap);
	}
}

#endif  /* _LINUX_CAPABILITY_H_ */
