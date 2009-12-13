/*	$NetBSD: ntp_assert.h,v 1.1.1.1 2009/12/13 16:54:50 kardel Exp $	*/

/*
 * ntp_assert.h - design by contract stuff
 *
 * example:
 *
 * int foo(char *a) {
 *	int result;
 *	int value;
 *
 *	NTP_REQUIRE(a != NULL);
 *	...
 *	bar(&value);
 *	NTP_INSIST(value > 2);
 *	...
 *
 *	NTP_ENSURE(result != 12);
 *	return result;
 * }
 *
 * open question: when would we use NTP_INVARIANT()?
 */

#ifndef NTP_ASSERT_H
#define NTP_ASSERT_H

# ifdef CALYSTO 

extern void calysto_assume(unsigned char cnd); /* assume this always holds */ 
extern void calysto_assert(unsigned char cnd); /* check whether this holds */ 
#define NTP_REQUIRE(x)		calysto_assert(x)
#define NTP_INSIST(x)		calysto_assume(x) /* DLH calysto_assert()? */
#define NTP_INVARIANT(x)	calysto_assume(x)
#define NTP_ENSURE(x)		calysto_assert(x)

# elif defined(__COVERITY__)

/*
 * Coverity has special knowledge that assert(x) terminates the process
 * if x is not true.  Rather than teach it about our assertion macros,
 * just use the one it knows about for Coverity Prevent scans.  This
 * means our assertion code (and ISC's) escapes Coverity analysis, but
 * that seems to be a reasonable trade-off.
 */

#define NTP_REQUIRE(x)		assert(x)
#define NTP_INSIST(x)		assert(x)
#define NTP_INVARIANT(x)	assert(x)
#define NTP_ENSURE(x)		assert(x)

# else	/* neither Coverity nor Calysto */

#include "isc/assertions.h"

#define NTP_REQUIRE(x)		ISC_REQUIRE(x)
#define NTP_INSIST(x)		ISC_INSIST(x)
#define NTP_INVARIANT(x)	ISC_INVARIANT(x)
#define NTP_ENSURE(x)		ISC_ENSURE(x)

# endif /* neither Coverity nor Calysto */
#endif	/* NTP_ASSERT_H */
