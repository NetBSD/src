/*	$NetBSD: mach_syscall.c,v 1.4.2.2 2002/11/11 22:03:02 nathanw Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "opt_compat_mach.h"
#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: mach_syscall.c,v 1.4.2.2 2002/11/11 22:03:02 nathanw Exp $");

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/syscall.h>

#include <compat/mach/mach_syscall.h> 
#include <compat/mach/arch/powerpc/ppccalls/mach_ppccalls_syscall.h>
#include <compat/mach/arch/powerpc/fasttraps/mach_fasttraps_syscall.h>

extern const struct sysent mach_sysent[];
extern const struct sysent mach_ppccalls_sysent[];
extern const struct sysent mach_fasttraps_sysent[];

#define EMULNAME(x)	__CONCAT(mach_,x)
#define EMULNAMEU(x)	(x)	/* COMPAT_MACH uses the native syscalls */

#define MACH_FASTTRAPS		0x00007ff0
#define MACH_PPCCALLS		0x00006000
#define MACH_ODD_SYSCALL_MASK	0x0000fff0

static inline const struct sysent *mach_syscall_dispatch __P((register_t));

#include "syscall.c"

static inline const struct sysent *
mach_syscall_dispatch(register_t code)
{
#ifdef DEBUG_MACH
	if (code >= MACH_PPCCALLS);
		printf("->mach(0x%x)\n", code);
#endif
	switch (code & MACH_ODD_SYSCALL_MASK) {
	case MACH_PPCCALLS:
		return mach_ppccalls_sysent +
		    ((code - MACH_PPCCALLS) & (MACH_PPCCALLS_SYS_NSYSENT-1));

	case MACH_FASTTRAPS:
		return mach_fasttraps_sysent +
		    ((code - MACH_FASTTRAPS) & (MACH_FASTTRAPS_SYS_NSYSENT-1));

	default:
		if (code < 0) {
#ifdef DEBUG_MACH
			printf("->mach(%d)\n", code);
#endif /* DEBUG_MACH */
			return mach_sysent + (-code & (MACH_SYS_NSYSENT-1));
		}
		break;
	}

	return NULL;
}
