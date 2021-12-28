/*	$NetBSD: sys_getrandom.c,v 1.2 2021/12/28 13:22:43 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
 * getrandom() system call
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_getrandom.c,v 1.2 2021/12/28 13:22:43 riastradh Exp $");

#include <sys/types.h>
#include <sys/param.h>

#include <sys/atomic.h>
#include <sys/cprng.h>
#include <sys/entropy.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/random.h>
#include <sys/sched.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>
#include <sys/uio.h>

#include <crypto/nist_hash_drbg/nist_hash_drbg.h>

#define	RANDOM_BUFSIZE	512

int
dogetrandom(struct uio *uio, unsigned int flags)
{
	uint8_t seed[NIST_HASH_DRBG_SEEDLEN_BYTES] = {0};
	struct nist_hash_drbg drbg;
	uint8_t *buf;
	int extractflags = 0;
	int error;

	KASSERT((flags & ~(GRND_RANDOM|GRND_INSECURE|GRND_NONBLOCK)) == 0);
	KASSERT((flags & (GRND_RANDOM|GRND_INSECURE)) !=
	    (GRND_RANDOM|GRND_INSECURE));

	/* Get a buffer for transfers.  */
	buf = kmem_alloc(RANDOM_BUFSIZE, KM_SLEEP);

	/*
	 * Fast path: for short reads other than from /dev/random, if
	 * seeded or if INSECURE, just draw from per-CPU cprng_strong.
	 */
	if (uio->uio_resid <= RANDOM_BUFSIZE &&
	    !ISSET(flags, GRND_RANDOM) &&
	    (entropy_ready() || ISSET(flags, GRND_INSECURE))) {
		/* Generate data and transfer it out.  */
		cprng_strong(user_cprng, buf, uio->uio_resid, 0);
		error = uiomove(buf, uio->uio_resid, uio);
		goto out;
	}

	/*
	 * Try to get a seed from the entropy pool.  Fail if we would
	 * block.  If GRND_INSECURE, always return something even if it
	 * is partial entropy; if !GRND_INSECURE, set ENTROPY_HARDFAIL
	 * in order to tell entropy_extract not to bother drawing
	 * anything from a partial pool if we can't get full entropy.
	 */
	if (!ISSET(flags, GRND_NONBLOCK) && !ISSET(flags, GRND_INSECURE))
		extractflags |= ENTROPY_WAIT|ENTROPY_SIG;
	if (!ISSET(flags, GRND_INSECURE))
		extractflags |= ENTROPY_HARDFAIL;
	error = entropy_extract(seed, sizeof seed, extractflags);
	if (error && !ISSET(flags, GRND_INSECURE))
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
		if (ISSET(flags, GRND_RANDOM)) {
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
			error = entropy_extract(seed, sizeof seed,
			    ENTROPY_HARDFAIL);
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
		if (ISSET(flags, GRND_RANDOM)) {
			error = 0;
			break;
		}

		/* Now's a good time to yield if needed.  */
		preempt_point();

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

int
sys_getrandom(struct lwp *l, const struct sys_getrandom_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(void *)	buf;
		syscallarg(size_t)	buflen;
		syscallarg(unsigned)	flags;
	} */
	void *buf = SCARG(uap, buf);
	size_t buflen = SCARG(uap, buflen);
	int flags = SCARG(uap, flags);
	int error;

	/* Set up an iov and uio to read into the user's buffer.  */
	struct iovec iov = { .iov_base = buf, .iov_len = buflen };
	struct uio uio = {
		.uio_iov = &iov,
		.uio_iovcnt = 1,
		.uio_offset = 0,
		.uio_resid = buflen,
		.uio_rw = UIO_READ,
		.uio_vmspace = curproc->p_vmspace,
	};

	/* Validate the flags.  */
	if (flags & ~(GRND_RANDOM|GRND_INSECURE|GRND_NONBLOCK)) {
		/* Unknown flags.  */
		error = EINVAL;
		goto out;
	}
	if ((flags & (GRND_RANDOM|GRND_INSECURE)) ==
	    (GRND_RANDOM|GRND_INSECURE)) {
		/* Nonsensical combination.  */
		error = EINVAL;
		goto out;
	}

	/* Do it.  */
	error = dogetrandom(&uio, flags);

out:	/*
	 * If we transferred anything, return the number of bytes
	 * transferred and suppress error; otherwise return the error.
	 */
	*retval = buflen - uio.uio_resid;
	if (*retval)
		error = 0;
	return error;
}
