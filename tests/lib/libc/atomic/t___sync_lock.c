/*	$NetBSD: t___sync_lock.c,v 1.2 2025/04/07 01:34:43 riastradh Exp $	*/

/*
 * Copyright (C) 2019 Tetsuya Isaki. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t___sync_lock.c,v 1.2 2025/04/07 01:34:43 riastradh Exp $");

#include <atf-c.h>
#include <inttypes.h>
#include <machine/types.h>	// for __HAVE_ATOMIC64_OPS

/*
 * These tests don't examine the atomicity.
 */

/* XXX
 * Depending on a combination of arch and compiler, __sync_* is
 * implemented as compiler's builtin function.  In that case, even
 * if libc exports the function symbol, it is not used.  As a result
 * this tests will examine compiler's builtin functions.
 * It's better to run only when target is actually in libc.
 */

#if defined __vax__
/*
 * On VAX, __sync_lock_test_and_set_* test and set the low-order bit
 * with BBSSI, and __sync_lock_release_* clear the low-order bit with
 * BBCCI, so the other bits are not relevant.
 *
 * It is possible that, by using values other than 0 and 1, we are
 * relying on more than gcc guarantees about __sync_lock_test_and_set_*
 * and __sync_lock_release_*.  But, well, if so, we will be alerted by
 * a failing test.
 */
#define INITVAL 0x1122334455667788
#define LOCKVAL 1
#define LOCKRET 0
#define LOCKEDVAL 0x1122334455667789
#define UNLOCKEDVAL 0x1122334455667788
#elif 0 && defined __hppa__
/*
 * On HPPA, the native atomic r/m/w instruction, LDCW, atomically loads
 * a word and clears it, so the obvious choice is for the unlocked
 * state to be nonzero and the locked state to be zero.
 *
 * But gcc doesn't do that.
 *
 * Instead, it uses zero for unlocked and nonzero for locked.  So for
 * __sync_lock_test_and_set_* it issues an out-of-line call (which on
 * NetBSD implements by atomic_swap_N), and for __sync_lock_release_*,
 * it issues LDCW on a scratch stack location only as a barrier and
 * then issues STW to store a zero.
 *
 * So we don't use this branch after all.  But I'm leaving it here as a
 * reminder to anyone who suspects something might be wrong on HPPA.
 */
#define INITVAL 0x1122334455667788
#define LOCKVAL 0
#define LOCKRET 0x1122334455667788
#define LOCKEDVAL 0
#define UNLOCKEDVAL 1
#else
/*
 * According to GCC documentation at
 * <https://gcc.gnu.org/onlinedocs/gcc-12.4.0/gcc/_005f_005fsync-Builtins.html>,
 * the only guaranteed supported value for LOCKVAL is 1, and it is not
 * guaranteed that __sync_lock_release_* stores zero.  But on many
 * architectures other values work too, and __sync_lock_release_* does
 * just store zero, so let's test these by default; the exceptions can
 * be listed above.
 */
#define INITVAL 0x1122334455667788
#define LOCKVAL 0x8090a0b0c0d0e0f0
#define LOCKRET 0x1122334455667788
#define LOCKEDVAL 0x8090a0b0c0d0e0f0
#define UNLOCKEDVAL 0
#endif

#define atf_sync_tas(NAME, TYPE, FMT) \
ATF_TC(NAME); \
ATF_TC_HEAD(NAME, tc) \
{ \
	atf_tc_set_md_var(tc, "descr", #NAME); \
} \
ATF_TC_BODY(NAME, tc) \
{ \
	volatile TYPE val; \
	TYPE newval; \
	TYPE expval; \
	TYPE expres; \
	TYPE res; \
	val = (TYPE)INITVAL; \
	newval = (TYPE)LOCKVAL; \
	expval = (TYPE)LOCKEDVAL; \
	expres = (TYPE)LOCKRET; \
	res = NAME(&val, newval); \
	ATF_REQUIRE_MSG(val == expval, \
	    "val expects 0x%" FMT " but 0x%" FMT, expval, val); \
	ATF_REQUIRE_MSG(res == expres, \
	    "res expects 0x%" FMT " but 0x%" FMT, expres, res); \
}

atf_sync_tas(__sync_lock_test_and_set_1, uint8_t,  PRIx8);
atf_sync_tas(__sync_lock_test_and_set_2, uint16_t, PRIx16);
atf_sync_tas(__sync_lock_test_and_set_4, uint32_t, PRIx32);
#ifdef __HAVE_ATOMIC64_OPS
atf_sync_tas(__sync_lock_test_and_set_8, uint64_t, PRIx64);
#endif

#define atf_sync_rel(NAME, TYPE, FMT) \
ATF_TC(NAME); \
ATF_TC_HEAD(NAME, tc) \
{ \
	atf_tc_set_md_var(tc, "descr", #NAME); \
} \
ATF_TC_BODY(NAME, tc) \
{ \
	volatile TYPE val; \
	TYPE expval; \
	val = (TYPE)LOCKEDVAL; \
	expval = (TYPE)UNLOCKEDVAL; \
	NAME(&val); \
	ATF_REQUIRE_MSG(val == expval, \
	    "val expects 0x%" FMT " but 0x%" FMT, expval, val); \
}

atf_sync_rel(__sync_lock_release_1, uint8_t,  PRIx8);
atf_sync_rel(__sync_lock_release_2, uint16_t, PRIx16);
atf_sync_rel(__sync_lock_release_4, uint32_t, PRIx32);
#ifdef __HAVE_ATOMIC64_OPS
atf_sync_rel(__sync_lock_release_8, uint64_t, PRIx64);
#endif

/*
 * __sync_synchronize(): This is just a link-time test.
 */
ATF_TC(__sync_synchronize);
ATF_TC_HEAD(__sync_synchronize, tc)
{
	atf_tc_set_md_var(tc, "descr", "__sync_synchronize");
}
ATF_TC_BODY(__sync_synchronize, tc)
{
	__sync_synchronize();
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, __sync_lock_test_and_set_1);
	ATF_TP_ADD_TC(tp, __sync_lock_test_and_set_2);
	ATF_TP_ADD_TC(tp, __sync_lock_test_and_set_4);
#ifdef __HAVE_ATOMIC64_OPS
	ATF_TP_ADD_TC(tp, __sync_lock_test_and_set_8);
#endif

	ATF_TP_ADD_TC(tp, __sync_lock_release_1);
	ATF_TP_ADD_TC(tp, __sync_lock_release_2);
	ATF_TP_ADD_TC(tp, __sync_lock_release_4);
#ifdef __HAVE_ATOMIC64_OPS
	ATF_TP_ADD_TC(tp, __sync_lock_release_8);
#endif

	ATF_TP_ADD_TC(tp, __sync_synchronize);

	return atf_no_error();
}
