/*	$NetBSD: random.c,v 1.7 2020/05/08 16:05:36 riastradh Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

/*
 * /dev/random, /dev/urandom -- stateless version
 *
 *	For short reads from /dev/urandom, up to 256 bytes, read from a
 *	per-CPU NIST Hash_DRBG instance that is reseeded as soon as the
 *	system has enough entropy.
 *
 *	For all other reads, instantiate a fresh NIST Hash_DRBG from
 *	the global entropy pool, and draw from it.
 *
 *	Each read is independent; there is no per-open state.
 *	Concurrent reads from the same open run in parallel.
 *
 *	Reading from /dev/random may block until entropy is available.
 *	Either device may return short reads if interrupted.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: random.c,v 1.7 2020/05/08 16:05:36 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/conf.h>
#include <sys/cprng.h>
#include <sys/entropy.h>
#include <sys/errno.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/poll.h>
#include <sys/rnd.h>
#include <sys/rndsource.h>
#include <sys/signalvar.h>
#include <sys/systm.h>

#include <crypto/nist_hash_drbg/nist_hash_drbg.h>

#include "ioconf.h"

static dev_type_open(random_open);
static dev_type_close(random_close);
static dev_type_ioctl(random_ioctl);
static dev_type_poll(random_poll);
static dev_type_kqfilter(random_kqfilter);
static dev_type_read(random_read);
static dev_type_write(random_write);

const struct cdevsw rnd_cdevsw = {
	.d_open = random_open,
	.d_close = random_close,
	.d_read = random_read,
	.d_write = random_write,
	.d_ioctl = random_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = random_poll,
	.d_mmap = nommap,
	.d_kqfilter = random_kqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER|D_MPSAFE,
};

#define	RANDOM_BUFSIZE	512	/* XXX pulled from arse */

/* Entropy source for writes to /dev/random and /dev/urandom */
static krndsource_t	user_rndsource;

void
rndattach(int num)
{

	rnd_attach_source(&user_rndsource, "/dev/random", RND_TYPE_UNKNOWN,
	    RND_FLAG_COLLECT_VALUE);
}

static int
random_open(dev_t dev, int flags, int fmt, struct lwp *l)
{

	/* Validate minor.  */
	switch (minor(dev)) {
	case RND_DEV_RANDOM:
	case RND_DEV_URANDOM:
		break;
	default:
		return ENXIO;
	}

	return 0;
}

static int
random_close(dev_t dev, int flags, int fmt, struct lwp *l)
{

	/* Success!  */
	return 0;
}

static int
random_ioctl(dev_t dev, unsigned long cmd, void *data, int flag, struct lwp *l)
{

	/*
	 * No non-blocking/async options; otherwise defer to
	 * entropy_ioctl.
	 */
	switch (cmd) {
	case FIONBIO:
	case FIOASYNC:
		return 0;
	default:
		return entropy_ioctl(cmd, data);
	}
}

static int
random_poll(dev_t dev, int events, struct lwp *l)
{

	/* /dev/random may block; /dev/urandom is always ready.  */
	switch (minor(dev)) {
	case RND_DEV_RANDOM:
		return entropy_poll(events);
	case RND_DEV_URANDOM:
		return events & (POLLIN|POLLRDNORM | POLLOUT|POLLWRNORM);
	default:
		return 0;
	}
}

static int
random_kqfilter(dev_t dev, struct knote *kn)
{

	/* Validate the event filter.  */
	switch (kn->kn_filter) {
	case EVFILT_READ:
	case EVFILT_WRITE:
		break;
	default:
		return EINVAL;
	}

	/* /dev/random may block; /dev/urandom never does.  */
	switch (minor(dev)) {
	case RND_DEV_RANDOM:
		if (kn->kn_filter == EVFILT_READ)
			return entropy_kqfilter(kn);
		/* FALLTHROUGH */
	case RND_DEV_URANDOM:
		kn->kn_fop = &seltrue_filtops;
		return 0;
	default:
		return ENXIO;
	}
}

/*
 * random_read(dev, uio, flags)
 *
 *	Generate data from a PRNG seeded from the entropy pool.
 *
 *	- If /dev/random, block until we have full entropy, or fail
 *	  with EWOULDBLOCK, and if `depleting' entropy, return at most
 *	  the entropy pool's capacity at once.
 *
 *	- If /dev/urandom, generate data from whatever is in the
 *	  entropy pool now.
 *
 *	On interrupt, return a short read, but not shorter than 256
 *	bytes (actually, no shorter than RANDOM_BUFSIZE bytes, which is
 *	512 for hysterical raisins).
 */
static int
random_read(dev_t dev, struct uio *uio, int flags)
{
	uint8_t seed[NIST_HASH_DRBG_SEEDLEN_BYTES] = {0};
	struct nist_hash_drbg drbg;
	uint8_t *buf;
	int extractflags;
	int error;

	/* Get a buffer for transfers.  */
	buf = kmem_alloc(RANDOM_BUFSIZE, KM_SLEEP);

	/*
	 * If it's a short read from /dev/urandom, just generate the
	 * output directly with per-CPU cprng_strong.
	 */
	if (minor(dev) == RND_DEV_URANDOM &&
	    uio->uio_resid <= RANDOM_BUFSIZE) {
		/* Generate data and transfer it out.  */
		cprng_strong(user_cprng, buf, uio->uio_resid, 0);
		error = uiomove(buf, uio->uio_resid, uio);
		goto out;
	}

	/*
	 * If we're doing a blocking read from /dev/random, wait
	 * interruptibly.  Otherwise, don't wait.
	 */
	if (minor(dev) == RND_DEV_RANDOM && !ISSET(flags, FNONBLOCK))
		extractflags = ENTROPY_WAIT|ENTROPY_SIG;
	else
		extractflags = 0;

	/*
	 * Query the entropy pool.  For /dev/random, stop here if this
	 * fails.  For /dev/urandom, go on either way --
	 * entropy_extract will always fill the buffer with what we
	 * have from the global pool.
	 */
	error = entropy_extract(seed, sizeof seed, extractflags);
	if (minor(dev) == RND_DEV_RANDOM && error)
		goto out;

	/* Instantiate the DRBG.  */
	if (nist_hash_drbg_instantiate(&drbg, seed, sizeof seed, NULL, 0,
		NULL, 0))
		panic("nist_hash_drbg_instantiate");

	/* Promptly zero the seed.  */
	explicit_memset(seed, 0, sizeof seed);

	/* Generate data.  */
	error = 0;
	while (uio->uio_resid) {
		size_t n = MIN(uio->uio_resid, RANDOM_BUFSIZE);

		/*
		 * Clamp /dev/random output to the entropy capacity and
		 * seed size.  Programs can't rely on long reads.
		 */
		if (minor(dev) == RND_DEV_RANDOM) {
			n = MIN(n, ENTROPY_CAPACITY);
			n = MIN(n, sizeof seed);
			/*
			 * Guarantee never to return more than one
			 * buffer in this case to minimize bookkeeping.
			 */
			CTASSERT(ENTROPY_CAPACITY <= RANDOM_BUFSIZE);
			CTASSERT(sizeof seed <= RANDOM_BUFSIZE);
		}

		/*
		 * Try to generate a block of data, but if we've hit
		 * the DRBG reseed interval, reseed.
		 */
		if (nist_hash_drbg_generate(&drbg, buf, n, NULL, 0)) {
			/*
			 * Get a fresh seed without blocking -- we have
			 * already generated some output so it is not
			 * useful to block.  This can fail only if the
			 * request is obscenely large, so it is OK for
			 * either /dev/random or /dev/urandom to fail:
			 * we make no promises about gigabyte-sized
			 * reads happening all at once.
			 */
			error = entropy_extract(seed, sizeof seed, 0);
			if (error)
				break;

			/* Reseed and try again.  */
			if (nist_hash_drbg_reseed(&drbg, seed, sizeof seed,
				NULL, 0))
				panic("nist_hash_drbg_reseed");

			/* Promptly zero the seed.  */
			explicit_memset(seed, 0, sizeof seed);

			/* If it fails now, that's a bug.  */
			if (nist_hash_drbg_generate(&drbg, buf, n, NULL, 0))
				panic("nist_hash_drbg_generate");
		}

		/* Transfer n bytes out.  */
		error = uiomove(buf, n, uio);
		if (error)
			break;

		/*
		 * If this is /dev/random, stop here, return what we
		 * have, and force the next read to reseed.  Programs
		 * can't rely on /dev/random for long reads.
		 */
		if (minor(dev) == RND_DEV_RANDOM) {
			error = 0;
			break;
		}

		/* Yield if requested.  */
		if (curcpu()->ci_schedstate.spc_flags & SPCF_SHOULDYIELD)
			preempt();

		/* Check for interruption after at least 256 bytes.  */
		CTASSERT(RANDOM_BUFSIZE >= 256);
		if (__predict_false(curlwp->l_flag & LW_PENDSIG) &&
		    sigispending(curlwp, 0)) {
			error = EINTR;
			break;
		}
	}

out:	/* Zero the buffer and free it.  */
	explicit_memset(buf, 0, RANDOM_BUFSIZE);
	kmem_free(buf, RANDOM_BUFSIZE);

	return error;
}

/*
 * random_write(dev, uio, flags)
 *
 *	Enter data from uio into the entropy pool.
 *
 *	Assume privileged users provide full entropy, and unprivileged
 *	users provide no entropy.  If you have a nonuniform source of
 *	data with n bytes of min-entropy, hash it with an XOF like
 *	SHAKE128 into exactly n bytes first.
 */
static int
random_write(dev_t dev, struct uio *uio, int flags)
{
	kauth_cred_t cred = kauth_cred_get();
	uint8_t *buf;
	bool privileged = false, any = false;
	int error = 0;

	/* Verify user's authorization to affect the entropy pool.  */
	error = kauth_authorize_device(cred, KAUTH_DEVICE_RND_ADDDATA,
	    NULL, NULL, NULL, NULL);
	if (error)
		return error;

	/*
	 * Check whether user is privileged.  If so, assume user
	 * furnishes full-entropy data; if not, accept user's data but
	 * assume it has zero entropy when we do accounting.  If you
	 * want to specify less entropy, use ioctl(RNDADDDATA).
	 */
	if (kauth_authorize_device(cred, KAUTH_DEVICE_RND_ADDDATA_ESTIMATE,
		NULL, NULL, NULL, NULL) == 0)
		privileged = true;

	/* Get a buffer for transfers.  */
	buf = kmem_alloc(RANDOM_BUFSIZE, KM_SLEEP);

	/* Consume data.  */
	while (uio->uio_resid) {
		size_t n = MIN(uio->uio_resid, RANDOM_BUFSIZE);

		/* Transfer n bytes in and enter them into the pool.  */
		error = uiomove(buf, n, uio);
		if (error)
			break;
		rnd_add_data(&user_rndsource, buf, n, privileged ? n*NBBY : 0);
		any = true;

		/* Yield if requested.  */
		if (curcpu()->ci_schedstate.spc_flags & SPCF_SHOULDYIELD)
			preempt();

		/* Check for interruption.  */
		if (__predict_false(curlwp->l_flag & LW_PENDSIG) &&
		    sigispending(curlwp, 0)) {
			error = EINTR;
			break;
		}
	}

	/* Zero the buffer and free it.  */
	explicit_memset(buf, 0, RANDOM_BUFSIZE);
	kmem_free(buf, RANDOM_BUFSIZE);

	/* If we added anything, consolidate entropy now.  */
	if (any)
		entropy_consolidate();

	return error;
}
