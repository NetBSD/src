/*	$NetBSD: t_arc4random.c,v 1.1.2.2 2024/10/09 13:25:13 martin Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

#define	_REENTRANT

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_arc4random.c,v 1.1.2.2 2024/10/09 13:25:13 martin Exp $");

#include <sys/resource.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

#include <atf-c.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "arc4random.h"
#include "reentrant.h"
#include "h_macros.h"

/*
 * iszero(buf, len)
 *
 *	True if len bytes at buf are all zero, false if any one of them
 *	is nonzero.
 */
static bool
iszero(const void *buf, size_t len)
{
	const unsigned char *p = buf;
	size_t i;

	for (i = 0; i < len; i++) {
		if (p[i] != 0)
			return false;
	}
	return true;
}

/*
 * arc4random_prng()
 *
 *	Get a pointer to the current arc4random state, without updating
 *	any of the state, not even lazy initialization.
 */
static struct arc4random_prng *
arc4random_prng(void)
{
	struct arc4random_prng *prng = NULL;

	/*
	 * If arc4random has been initialized and there is a thread key
	 * (i.e., libc was built with _REENTRANT), get the thread-local
	 * arc4random state if there is one.
	 */
	if (arc4random_global.initialized)
		prng = thr_getspecific(arc4random_global.thread_key);

	/*
	 * If we couldn't get the thread-local state, get the global
	 * state instead.
	 */
	if (prng == NULL)
		prng = &arc4random_global.prng;

	return prng;
}

/*
 * arc4random_global_buf(buf, len)
 *
 *	Same as arc4random_buf, but force use of the global state.
 *	Must happen before any other use of arc4random.
 */
static void
arc4random_global_buf(void *buf, size_t len)
{
	struct rlimit rlim, orlim;
	struct arc4random_prng *prng;

	/*
	 * Save the address space limit.
	 */
	RL(getrlimit(RLIMIT_AS, &orlim));
	memcpy(&rlim, &orlim, sizeof(rlim));

	/*
	 * Get a sample while the address space limit is zero.  This
	 * should try, and fail, to allocate a thread-local arc4random
	 * state with mmap(2).
	 */
	rlim.rlim_cur = 0;
	RL(setrlimit(RLIMIT_AS, &rlim));
	arc4random_buf(buf, len);
	RL(setrlimit(RLIMIT_AS, &orlim));

	/*
	 * Restore the address space limit.
	 */
	RL(setrlimit(RLIMIT_AS, &orlim));

	/*
	 * Verify the PRNG is the global one, not the thread-local one,
	 * and that it was initialized.
	 */
	prng = arc4random_prng();
	ATF_CHECK_EQ(prng, &arc4random_global.prng);
	ATF_CHECK(!iszero(&prng->arc4_prng, sizeof(prng->arc4_prng)));
	ATF_CHECK(prng->arc4_epoch != 0);
}

/*
 * arc4random_global_thread(cookie)
 *
 *	Start routine for a thread that just grabs an output from the
 *	global state.
 */
static void *
arc4random_global_thread(void *cookie)
{
	unsigned char buf[32];

	arc4random_global_buf(buf, sizeof(buf));

	return NULL;
}

ATF_TC(addrandom);
ATF_TC_HEAD(addrandom, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test arc4random_addrandom updates the state");
}
ATF_TC_BODY(addrandom, tc)
{
	unsigned char buf[32], zero32[32] = {0};
	struct arc4random_prng *prng, copy;

	/*
	 * Get a sample to start things off.
	 */
	arc4random_buf(buf, sizeof(buf));
	ATF_CHECK(!iszero(buf, sizeof(buf)));	/* Pr[fail] = 1/2^256 */

	/*
	 * By this point, the global state must be initialized -- if
	 * not, the process should have aborted.
	 */
	ATF_CHECK(arc4random_global.initialized);

	/*
	 * Get the PRNG, global or local.  By this point, the PRNG
	 * state should be nonzero (with overwhelmingly high
	 * probability) and the epoch should also be nonzero.
	 */
	prng = arc4random_prng();
	ATF_CHECK(!iszero(&prng->arc4_prng, sizeof(prng->arc4_prng)));
	ATF_CHECK(prng->arc4_epoch != 0);

	/*
	 * Save a copy and update the state with arc4random_addrandom.
	 */
	copy = *prng;
	arc4random_addrandom(zero32, sizeof(zero32));

	/*
	 * The state should have changed.  (The epoch may or may not.)
	 */
	ATF_CHECK(memcmp(&prng->arc4_prng, &copy.arc4_prng,
		sizeof(copy.arc4_prng)) != 0);

	/*
	 * Save a copy and update the state with arc4random_stir.
	 */
	copy = *prng;
	arc4random_stir();

	/*
	 * The state should have changed.  (The epoch may or may not.)
	 */
	ATF_CHECK(memcmp(&prng->arc4_prng, &copy.arc4_prng,
		sizeof(copy.arc4_prng)) != 0);
}

ATF_TC(consolidate);
ATF_TC_HEAD(consolidate, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test consolidating entropy resets the epoch");
}
ATF_TC_BODY(consolidate, tc)
{
	unsigned char buf[32];
	struct arc4random_prng *local, *global = &arc4random_global.prng;
	unsigned localepoch, globalepoch;
	const int consolidate = 1;
	pthread_t thread;

	/*
	 * Get a sample from the global state to make sure the global
	 * state is initialized.  Remember the epoch.
	 */
	arc4random_global_buf(buf, sizeof(buf));
	ATF_CHECK(!iszero(buf, sizeof(buf)));	/* Pr[fail] = 1/2^256 */
	ATF_CHECK(!iszero(&global->arc4_prng, sizeof(global->arc4_prng)));
	ATF_CHECK((globalepoch = global->arc4_epoch) != 0);

	/*
	 * Get a sample from the local state too to make sure the local
	 * state is initialized.  Remember the epoch.
	 */
	arc4random_buf(buf, sizeof(buf));
	ATF_CHECK(!iszero(buf, sizeof(buf)));	/* Pr[fail] = 1/2^256 */
	local = arc4random_prng();
	ATF_CHECK(!iszero(&local->arc4_prng, sizeof(local->arc4_prng)));
	ATF_CHECK((localepoch = local->arc4_epoch) != 0);

	/*
	 * Trigger entropy consolidation.
	 */
	RL(sysctlbyname("kern.entropy.consolidate", /*oldp*/NULL, /*oldlen*/0,
		&consolidate, sizeof(consolidate)));

	/*
	 * Verify the epoch cache isn't changed yet until we ask for
	 * more data.
	 */
	ATF_CHECK_EQ_MSG(globalepoch, global->arc4_epoch,
	    "global epoch was %u, now %u", globalepoch, global->arc4_epoch);
	ATF_CHECK_EQ_MSG(localepoch, local->arc4_epoch,
	    "local epoch was %u, now %u", localepoch, local->arc4_epoch);

	/*
	 * Request new output and verify the local epoch cache has
	 * changed.
	 */
	arc4random_buf(buf, sizeof(buf));
	ATF_CHECK(!iszero(buf, sizeof(buf)));	/* Pr[fail] = 1/2^256 */
	ATF_CHECK_MSG(localepoch != local->arc4_epoch,
	    "local epoch unchanged from %u", localepoch);

	/*
	 * Create a new thread to grab output from the global state,
	 * wait for it to complete, and verify the global epoch cache
	 * has changed.  (Now that we have already used the local state
	 * in this thread, we can't use the global state any more.)
	 */
	RZ(pthread_create(&thread, NULL, &arc4random_global_thread, NULL));
	RZ(pthread_join(thread, NULL));
	ATF_CHECK_MSG(globalepoch != global->arc4_epoch,
	    "global epoch unchanged from %u", globalepoch);
}

ATF_TC(fork);
ATF_TC_HEAD(fork, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test fork zeros the state and gets independent state");
}
ATF_TC_BODY(fork, tc)
{
	unsigned char buf[32];
	struct arc4random_prng *local, *global = &arc4random_global.prng;
	struct arc4random_prng childstate;
	int fd[2];
	pid_t child, pid;
	ssize_t nread;
	int status;

	/*
	 * Get a sample from the global state to make sure the global
	 * state is initialized.
	 */
	arc4random_global_buf(buf, sizeof(buf));
	ATF_CHECK(!iszero(buf, sizeof(buf)));	/* Pr[fail] = 1/2^256 */
	ATF_CHECK(!iszero(&global->arc4_prng, sizeof(global->arc4_prng)));
	ATF_CHECK(global->arc4_epoch != 0);

	/*
	 * Get a sample from the local state too to make sure the local
	 * state is initialized.
	 */
	arc4random_buf(buf, sizeof(buf));
	ATF_CHECK(!iszero(buf, sizeof(buf)));	/* Pr[fail] = 1/2^256 */
	local = arc4random_prng();
	ATF_CHECK(!iszero(&local->arc4_prng, sizeof(local->arc4_prng)));
	ATF_CHECK(local->arc4_epoch != 0);

	/*
	 * Create a pipe to transfer the state from child to parent.
	 */
	RL(pipe(fd));

	/*
	 * Fork a child.
	 */
	RL(child = fork());
	if (child == 0) {
		status = 0;

		/*
		 * Verify the states have been zero'd on fork.
		 */
		if (!iszero(local, sizeof(*local))) {
			fprintf(stderr, "failed to zero local state\n");
			status = 1;
		}
		if (!iszero(global, sizeof(*global))) {
			fprintf(stderr, "failed to zero global state\n");
			status = 1;
		}

		/*
		 * Verify we generate nonzero output.
		 */
		arc4random_buf(buf, sizeof(buf));
		if (iszero(buf, sizeof(buf))) {
			fprintf(stderr, "failed to generate nonzero output\n");
			status = 1;
		}

		/*
		 * Share the state to compare with parent.
		 */
		if ((size_t)write(fd[1], local, sizeof(*local)) !=
		    sizeof(*local)) {
			fprintf(stderr, "failed to share local state\n");
			status = 1;
		}
		_exit(status);
	}

	/*
	 * Verify the global state has been zeroed as expected.  (This
	 * way it is never available to the child, even shortly after
	 * the fork syscall returns before the atfork handler is
	 * called.)
	 */
	ATF_CHECK(iszero(global, sizeof(*global)));

	/*
	 * Read the state from the child.
	 */
	RL(nread = read(fd[0], &childstate, sizeof(childstate)));
	ATF_CHECK_EQ_MSG(nread, sizeof(childstate),
	    "nread=%zu sizeof(childstate)=%zu", nread, sizeof(childstate));

	/*
	 * Verify the child state is distinct.  (The global state has
	 * been zero'd so it's OK it if coincides.)  Check again after
	 * we grab another output.
	 */
	ATF_CHECK(memcmp(local, &childstate, sizeof(*local)) != 0);
	arc4random_buf(buf, sizeof(buf));
	ATF_CHECK(!iszero(buf, sizeof(buf)));	/* Pr[fail] = 1/2^256 */
	ATF_CHECK(memcmp(local, &childstate, sizeof(*local)) != 0);

	/*
	 * Wait for the child to complete and verify it passed.
	 */
	RL(pid = waitpid(child, &status, 0));
	ATF_CHECK_EQ_MSG(status, 0, "child exited with nonzero status=%d",
	    status);
}

ATF_TC(global);
ATF_TC_HEAD(global, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test the global state is used when address space limit is hit");
}
ATF_TC_BODY(global, tc)
{
	unsigned char buf[32], buf1[32];

	/*
	 * Get a sample from the global state (and verify it was using
	 * the global state).
	 */
	arc4random_global_buf(buf, sizeof(buf));

	/*
	 * Verify we got a sample.
	 */
	ATF_CHECK(!iszero(buf, sizeof(buf)));	/* Pr[fail] = 1/2^256 */

	/*
	 * Get a sample from whatever state and make sure it wasn't
	 * repeated, which happens only with probability 1/2^256.
	 */
	arc4random_buf(buf1, sizeof(buf1));
	ATF_CHECK(!iszero(buf1, sizeof(buf1)));	/* Pr[fail] = 1/2^256 */
	ATF_CHECK(memcmp(buf, buf1, sizeof(buf)) != 0);
}

ATF_TC(local);
ATF_TC_HEAD(local, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test arc4random uses thread-local state");
	/* XXX skip if libc was built without _REENTRANT */
}
ATF_TC_BODY(local, tc)
{
	unsigned char buf[32], buf1[32];
	struct arc4random_prng *prng;

	/*
	 * Get a sample to start things off.
	 */
	arc4random_buf(buf, sizeof(buf));
	ATF_CHECK(!iszero(buf, sizeof(buf)));	/* Pr[fail] = 1/2^256 */

	/*
	 * Verify the arc4random state is _not_ the global state.
	 */
	prng = arc4random_prng();
	ATF_CHECK(prng != &arc4random_global.prng);
	ATF_CHECK(!iszero(&prng->arc4_prng, sizeof(prng->arc4_prng)));
	ATF_CHECK(prng->arc4_epoch != 0);

	/*
	 * Get another sample and make sure it wasn't repeated, which
	 * happens only with probability 1/2^256.
	 */
	arc4random_buf(buf1, sizeof(buf1));
	ATF_CHECK(!iszero(buf1, sizeof(buf1)));	/* Pr[fail] = 1/2^256 */
	ATF_CHECK(memcmp(buf, buf1, sizeof(buf)) != 0);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, addrandom);
	ATF_TP_ADD_TC(tp, consolidate);
	ATF_TP_ADD_TC(tp, fork);
	ATF_TP_ADD_TC(tp, global);
	ATF_TP_ADD_TC(tp, local);

	return atf_no_error();
}
