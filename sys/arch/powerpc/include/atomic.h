/* $NetBSD: atomic.h,v 1.4 2005/12/28 19:09:29 perry Exp $ */

/*-
 */

/*
 * Misc. `atomic' operations.
 */

#ifndef _POWERPC_ATOMIC_H_
#define	_POWERPC_ATOMIC_H_

/*
 * atomic_setbits_ulong:
 *
 *	Atomically set bits in a `unsigned long'.
 */
static __inline void
atomic_setbits_ulong(volatile unsigned long *ulp, unsigned long v)
{
	unsigned long tmp;

	__asm volatile(
"# BEGIN atomic_setbits_ulong	\n"
"1:	lwarx	%0,0,%2		\n"
"	or	%0,%1,%0	\n"
"	stwcx.	%0,0,%2		\n"
"	bne-	1b		\n"
"	sync			\n"
"# END atomic_setbits_ulong	\n"
	: "=&r" (tmp)
	: "r" (v), "r" (ulp)
	: "memory");
}

/*
 * atomic_clearbits_ulong:
 *
 *	Atomically clear bits in a `unsigned long'.
 */
static __inline void
atomic_clearbits_ulong(volatile unsigned long *ulp, unsigned long v)
{
	unsigned long tmp;

	__asm volatile(
"# BEGIN atomic_clearbits_ulong	\n"
"1:	lwarx	%0,0,%2		\n"
"	and	%0,%1,%0	\n"
"	stwcx.	%0,0,%2		\n"
"	bne-	1b		\n"
"	sync			\n"
"# END atomic_clearbits_ulong	\n"
	: "=&r" (tmp)
	: "r" (~v), "r" (ulp)
	: "memory");
}

/*
 * atomic_add_ulong:
 *
 *	Atomically add a value to a `unsigned long'.
 */
static __inline void
atomic_add_ulong(volatile unsigned long *ulp, unsigned long v)
{
	unsigned long tmp;

	__asm volatile(
"# BEGIN atomic_add_ulong	\n"
"1:	lwarx	%0,0,%2		\n"
"	add	%0,%1,%0	\n"
"	stwcx.	%0,0,%2		\n"
"	bne-	1b		\n"
"	sync			\n"
"# END atomic_add_ulong		\n"
	: "=&r" (tmp)
	: "r" (v), "r" (ulp)
	: "memory");
}

/*
 * atomic_sub_ulong:
 *
 *	Atomically subtract a value from a `unsigned long'.
 */
static __inline void
atomic_sub_ulong(volatile unsigned long *ulp, unsigned long v)
{
	unsigned long tmp;

	__asm volatile(
"# BEGIN atomic_sub_ulong	\n"
"1:	lwarx	%0,0,%2		\n"
"	sub	%0,%0,%1	\n"
"	stwcx.	%0,0,%2		\n"
"	bne-	1b		\n"
"	sync			\n"
"# END atomic_sub_ulong		\n"
	: "=&r" (tmp)
	: "r" (v), "r" (ulp)
	: "memory");
}

/*
 * atomic_loadlatch_ulong:
 *
 *	Atomically load and latch a `unsigned long' value.
 */
static __inline unsigned long
atomic_loadlatch_ulong(volatile unsigned long *ulp, unsigned long v)
{
	unsigned long tmp;

	__asm volatile(
"# BEGIN atomic_loadlatch_ulong	\n"
"1:	lwarx	%0,0,%2		\n"
"	stwcx.	%1,0,%2		\n"
"	bne-	1b		\n"
"	sync			\n"
"# END atomic_loadlatch_ulong	\n"
	: "=r" (tmp)
	: "r" (v), "r" (ulp)
	: "memory");

	return (tmp);
}

#endif /* _POWERPC_ATOMIC_H_ */
