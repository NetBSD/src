// SPDX-FileCopyrightText: 2009 Novell, Inc.
// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: MIT

#ifndef _URCU_UATOMIC_ARCH_S390_H
#define _URCU_UATOMIC_ARCH_S390_H

/*
 * Atomic exchange operations for the S390 architecture. Based on information
 * taken from the Principles of Operation Appendix A "Conditional Swapping
 * Instructions (CS, CDS)".
 *
 * Author: Jan Blunck <jblunck@suse.de>
 */

#include <urcu/compiler.h>
#include <urcu/system.h>

#ifdef __cplusplus
extern "C" {
#endif

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 2)
#define COMPILER_HAVE_SHORT_MEM_OPERAND
#endif

/*
 * MEMOP assembler operand rules:
 * - op refer to MEMOP_IN operand
 * - MEMOP_IN can expand to more than a single operand. Use it at the end of
 *   operand list only.
 */

#ifdef COMPILER_HAVE_SHORT_MEM_OPERAND

#define MEMOP_OUT(addr)	"=Q" (*(addr))
#define MEMOP_IN(addr)	"Q" (*(addr))
#define MEMOP_REF(op)	#op		/* op refer to MEMOP_IN operand */

#else /* !COMPILER_HAVE_SHORT_MEM_OPERAND */

#define MEMOP_OUT(addr)	"=m" (*(addr))
#define MEMOP_IN(addr)	"a" (addr), "m" (*(addr))
#define MEMOP_REF(op)	"0(" #op ")"	/* op refer to MEMOP_IN operand */

#endif /* !COMPILER_HAVE_SHORT_MEM_OPERAND */

/*
 * The __hp() macro casts the void pointer @x to a pointer to a structure
 * containing an array of char of the specified size. This allows passing the
 * @addr arguments of the following inline functions as "m" and "+m" operands
 * to the assembly. The @size parameter should be a constant to support
 * compilers such as clang which do not support VLA. Create typedefs because
 * C++ does not allow types be defined in casts.
 */

typedef struct { char v[4]; } __hp_4;
typedef struct { char v[8]; } __hp_8;

#define __hp(size, x)	((__hp_##size *)(x))

/* xchg */

static inline __attribute__((always_inline))
unsigned long _uatomic_exchange(volatile void *addr, unsigned long val, int len)
{
	switch (len) {
	case 4:
	{
		unsigned int old_val;

		__asm__ __volatile__(
			"0:	cs %0,%2," MEMOP_REF(%3) "\n"
			"	brc 4,0b\n"
			: "=&r" (old_val), MEMOP_OUT (__hp(4, addr))
			: "r" (val), MEMOP_IN (__hp(4, addr))
			: "memory", "cc");
		return old_val;
	}
#if (CAA_BITS_PER_LONG == 64)
	case 8:
	{
		unsigned long old_val;

		__asm__ __volatile__(
			"0:	csg %0,%2," MEMOP_REF(%3) "\n"
			"	brc 4,0b\n"
			: "=&r" (old_val), MEMOP_OUT (__hp(8, addr))
			: "r" (val), MEMOP_IN (__hp(8, addr))
			: "memory", "cc");
		return old_val;
	}
#endif
	default:
		__asm__ __volatile__(".long	0xd00d00");
	}

	return 0;
}

#define uatomic_xchg_mo(addr, v, mo)					\
	(__typeof__(*(addr))) _uatomic_exchange((addr),			    \
						caa_cast_long_keep_sign(v), \
						sizeof(*(addr)))

/* cmpxchg */

static inline __attribute__((always_inline))
unsigned long _uatomic_cmpxchg(void *addr, unsigned long old,
			       unsigned long _new, int len)
{
	switch (len) {
	case 4:
	{
		unsigned int old_val = (unsigned int)old;

		__asm__ __volatile__(
			"	cs %0,%2," MEMOP_REF(%3) "\n"
			: "+r" (old_val), MEMOP_OUT (__hp(4, addr))
			: "r" (_new), MEMOP_IN (__hp(4, addr))
			: "memory", "cc");
		return old_val;
	}
#if (CAA_BITS_PER_LONG == 64)
	case 8:
	{
		__asm__ __volatile__(
			"	csg %0,%2," MEMOP_REF(%3) "\n"
			: "+r" (old), MEMOP_OUT (__hp(8, addr))
			: "r" (_new), MEMOP_IN (__hp(8, addr))
			: "memory", "cc");
		return old;
	}
#endif
	default:
		__asm__ __volatile__(".long	0xd00d00");
	}

	return 0;
}

#define uatomic_cmpxchg_mo(addr, old, _new, mos, mof)			\
	(__typeof__(*(addr))) _uatomic_cmpxchg((addr),			     \
					       caa_cast_long_keep_sign(old), \
					       caa_cast_long_keep_sign(_new),\
					       sizeof(*(addr)))

#ifdef __cplusplus
}
#endif

#include <urcu/uatomic/generic.h>

#endif /* _URCU_UATOMIC_ARCH_S390_H */
