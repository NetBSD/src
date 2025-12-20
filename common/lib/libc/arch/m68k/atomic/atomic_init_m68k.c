/*	$NetBSD: atomic_init_m68k.c,v 1.2 2025/12/20 16:25:09 thorpej Exp $	*/

/*-
 * Copyright (c) 2008, 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: atomic_init_m68k.c,v 1.2 2025/12/20 16:25:09 thorpej Exp $");

#include "extern.h"
#include "../../../atomic/atomic_op_namespace.h"

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/ras.h>
#include <sys/sysctl.h>

 /* __sysctl syscall stub */
#include "../../../../../../lib/libc/include/__sysctl.h"

#include <machine/cpu.h>

#include <stdlib.h>

/*
 * libc glue for m68k atomic operations.
 *
 * 68020 and later have a CAS instruction that can be used to implement
 * everything.  However, CAS (and TAS) are unusable on some 68020 systems
 * (see __HAVE_M68K_BROKEN_RMC).
 *
 * The 68010 has no CAS instruction.
 *
 * So, for BROKEN_RMC and 68010, we use a restartable atomic sequence.
 *
 * Whichever compare-and-swap implementation is chosen is used to implement
 * all of the other atomic operations:
 *
 * ==> The *_nv() variants generally require a compare-and-swap implementation
 *     anyway.
 *
 * ==> The other single-instruction operations (ADDx, ORx, etc.) are not
 *     truly atomic in that they do not generate a non-interruptible bus
 *     cycle, and thus would not work in a multi-processor environment.
 *     (We don't support any multiprocessor m68k systems today, but hey,
 *     it could happen!)
 */

extern uint32_t _atomic_cas_32_ras(volatile uint32_t *, uint32_t, uint32_t);
RAS_DECL(_atomic_cas_32_ras);

extern uint16_t _atomic_cas_16_ras(volatile uint16_t *, uint16_t, uint16_t);
RAS_DECL(_atomic_cas_16_ras);

extern uint8_t  _atomic_cas_8_ras(volatile uint8_t *, uint8_t, uint8_t);
RAS_DECL(_atomic_cas_8_ras);

#ifdef __mc68010__
#define	CAS32_DEFAULT	\
	((uint32_t (*)(volatile uint32_t *, uint32_t, uint32_t))abort)
#define	CAS16_DEFAULT	\
	((uint16_t (*)(volatile uint16_t *, uint16_t, uint16_t))abort)
#define	CAS8_DEFAULT	\
	((uint8_t (*)(volatile uint8_t *, uint8_t, uint8_t))abort)
#else
extern uint32_t _atomic_cas_32_casl(volatile uint32_t *, uint32_t, uint32_t);
extern uint16_t _atomic_cas_16_casw(volatile uint16_t *, uint16_t, uint16_t);
extern uint8_t  _atomic_cas_8_casb(volatile uint8_t *, uint8_t, uint8_t);

/* Default to CASx implementation, fall back on RAS only when necessary. */
#define	CAS32_DEFAULT	_atomic_cas_32_casl
#define	CAS16_DEFAULT	_atomic_cas_16_casw
#define	CAS8_DEFAULT	_atomic_cas_8_casb
#endif /* ! __mc68010__ */

static uint32_t (*_atomic_cas_32_fn)(volatile uint32_t *, uint32_t, uint32_t);
static uint16_t (*_atomic_cas_16_fn)(volatile uint16_t *, uint16_t, uint16_t);
static uint8_t (*_atomic_cas_8_fn)(volatile uint8_t *, uint8_t, uint8_t);

void *_atomic_cas_32_a0(volatile uint32_t *, uint32_t, uint32_t);

void *
_atomic_cas_32_a0(volatile uint32_t *ptr, uint32_t old, uint32_t new)
{
	/* Force return value to be duplicated into %a0. */
	return (void *)(*_atomic_cas_32_fn)(ptr, old, new);
}

#undef atomic_cas_32
#undef atomic_cas_uint
#undef atomic_cas_ulong
#undef atomic_cas_ptr
#undef atomic_cas_32_ni
#undef atomic_cas_uint_ni
#undef atomic_cas_ulong_ni
#undef atomic_cas_ptr_ni

__strong_alias(_atomic_cas_32,_atomic_cas_32_a0)
atomic_op_alias(atomic_cas_32,_atomic_cas_32_a0)
atomic_op_alias(atomic_cas_uint,_atomic_cas_32_a0)
__strong_alias(_atomic_cas_uint,_atomic_cas_32_a0)
atomic_op_alias(atomic_cas_ulong,_atomic_cas_32_a0)
__strong_alias(_atomic_cas_ulong,_atomic_cas_32_a0)
atomic_op_alias(atomic_cas_ptr,_atomic_cas_32_a0)
__strong_alias(_atomic_cas_ptr,_atomic_cas_32_a0)

atomic_op_alias(atomic_cas_32_ni,_atomic_cas_32_a0)
__strong_alias(_atomic_cas_32_ni,_atomic_cas_32_a0)
atomic_op_alias(atomic_cas_uint_ni,_atomic_cas_32_a0)
__strong_alias(_atomic_cas_uint_ni,_atomic_cas_32_a0)
atomic_op_alias(atomic_cas_ulong_ni,_atomic_cas_32_a0)
__strong_alias(_atomic_cas_ulong_ni,_atomic_cas_32_a0)
atomic_op_alias(atomic_cas_ptr_ni,_atomic_cas_32_a0)
__strong_alias(_atomic_cas_ptr_ni,_atomic_cas_32_a0)

crt_alias(__sync_val_compare_and_swap_4,_atomic_cas_32_a0)

uint16_t
_atomic_cas_16(volatile uint16_t *ptr, uint16_t old, uint16_t new)
{
	return (*_atomic_cas_16_fn)(ptr, old, new);
}

#undef atomic_cas_16
atomic_op_alias(atomic_cas_16,_atomic_cas_16)
crt_alias(__sync_val_compare_and_swap_2,_atomic_cas_16)

uint8_t
_atomic_cas_8(volatile uint8_t *ptr, uint8_t old, uint8_t new)
{
	return (*_atomic_cas_8_fn)(ptr, old, new);
}

#undef atomic_cas_8
atomic_op_alias(atomic_cas_8,_atomic_cas_8)
crt_alias(__sync_val_compare_and_swap_1,_atomic_cas_8)

void __section(".text.startup") __attribute__ ((__visibility__("hidden")))
__libc_atomic_init(void)
{
	_atomic_cas_32_fn = CAS32_DEFAULT;
	_atomic_cas_16_fn = CAS16_DEFAULT;
	_atomic_cas_8_fn = CAS8_DEFAULT;

#ifndef __mc68010__
	int mib[2];
	size_t len;
	bool broken_rmc;

	/*
	 * Check to see if this system has a non-working /RMC.  If
	 * the __sysctl() call fails, or if it indicates that /RMC
	 * works fine, then we have no further work to do because
	 * the stubs default to the CASx-using _atomic_cas_*()
	 * functions.
	 */
	mib[0] = CTL_MACHDEP;
	mib[1] = CPU_BROKEN_RMC;
	len = sizeof(broken_rmc);
	if (__sysctl(mib, 2, &broken_rmc, &len, NULL, 0) == -1 || !broken_rmc) {
		return;
	}
#endif /* ! __mc68010__ */

	/*
	 * If we get here, we either have a broken RMC system or a
	 * 68010.  In either case, we need to register the restartable
	 * atomic sequences with the kernel.
	 *
	 * XXX Should consider a lazy initialization of these.
	 */
	if (rasctl(RAS_ADDR(_atomic_cas_32_ras), RAS_SIZE(_atomic_cas_32_ras),
		   RAS_INSTALL) == 0) {
		_atomic_cas_32_fn = _atomic_cas_32_ras;
	}
	if (rasctl(RAS_ADDR(_atomic_cas_16_ras), RAS_SIZE(_atomic_cas_16_ras),
		   RAS_INSTALL) == 0) {
		_atomic_cas_16_fn = _atomic_cas_16_ras;
	}
	if (rasctl(RAS_ADDR(_atomic_cas_8_ras), RAS_SIZE(_atomic_cas_8_ras),
		   RAS_INSTALL) == 0) {
		_atomic_cas_8_fn = _atomic_cas_8_ras;
	}
}
