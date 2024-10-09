/*	$NetBSD: random.c,v 1.10.4.1 2024/10/09 13:04:16 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: random.c,v 1.10.4.1 2024/10/09 13:04:16 martin Exp $");

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
#include <sys/random.h>
#include <sys/rnd.h>
#include <sys/rndsource.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/vnode.h>		/* IO_NDELAY */

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
	int gflags;

	/* Set the appropriate GRND_* mode.  */
	switch (minor(dev)) {
	case RND_DEV_RANDOM:
		gflags = GRND_RANDOM;
		break;
	case RND_DEV_URANDOM:
		gflags = GRND_INSECURE;
		break;
	default:
		return ENXIO;
	}

	/*
	 * Set GRND_NONBLOCK if the user requested IO_NDELAY (i.e., the
	 * file was opened with O_NONBLOCK).
	 */
	if (flags & IO_NDELAY)
		gflags |= GRND_NONBLOCK;

	/* Defer to getrandom.  */
	return dogetrandom(uio, gflags);
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

		/* Now's a good time to yield if needed.  */
		preempt_point();

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
	if (any && error == 0)
		error = entropy_consolidate_sig();

	return error;
}
