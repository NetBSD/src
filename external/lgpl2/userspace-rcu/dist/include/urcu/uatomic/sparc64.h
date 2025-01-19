// SPDX-FileCopyrightText: 1991-1994 by Xerox Corporation.  All rights reserved.
// SPDX-FileCopyrightText: 1996-1999 by Silicon Graphics.  All rights reserved.
// SPDX-FileCopyrightText: 1999-2003 Hewlett-Packard Development Company, L.P.
// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LicenseRef-Boehm-GC

#ifndef _URCU_ARCH_UATOMIC_SPARC64_H
#define _URCU_ARCH_UATOMIC_SPARC64_H

/*
 * Code inspired from libuatomic_ops-1.2, inherited in part from the
 * Boehm-Demers-Weiser conservative garbage collector.
 */

#include <urcu/compiler.h>
#include <urcu/system.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __sparc_v9__
/* cmpxchg */

static inline __attribute__((always_inline))
unsigned long _uatomic_cmpxchg(void *addr, unsigned long old,
			      unsigned long _new, int len)
{
	switch (len) {
	case 4:
	{
		__asm__ __volatile__ (
			"membar #StoreLoad | #LoadLoad\n\t"
                        "cas [%1],%2,%0\n\t"
                        "membar #StoreLoad | #StoreStore\n\t"
                        : "+&r" (_new)
                        : "r" (addr), "r" (old)
                        : "memory");

		return _new;
	}
#if (CAA_BITS_PER_LONG == 64)
	case 8:
	{
		__asm__ __volatile__ (
			"membar #StoreLoad | #LoadLoad\n\t"
                        "casx [%1],%2,%0\n\t"
                        "membar #StoreLoad | #StoreStore\n\t"
                        : "+&r" (_new)
                        : "r" (addr), "r" (old)
                        : "memory");

		return _new;
	}
#endif
	}
	__builtin_trap();
	return 0;
}


#define uatomic_cmpxchg_mo(addr, old, _new, mos, mof)			\
	((__typeof__(*(addr))) _uatomic_cmpxchg((addr),			       \
						caa_cast_long_keep_sign(old),  \
						caa_cast_long_keep_sign(_new), \
						sizeof(*(addr))))
#endif
#ifdef __cplusplus
}
#endif

#include <urcu/uatomic/generic.h>

#endif /* _URCU_ARCH_UATOMIC_SPARC64_H */
