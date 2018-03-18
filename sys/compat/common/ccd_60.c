/*	$NetBSD: ccd_60.c,v 1.1 2018/03/18 20:33:52 christos Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ccd_60.c,v 1.1 2018/03/18 20:33:52 christos Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/disk.h>
#include <sys/lwp.h>

#include <dev/ccdvar.h>
#include <compat/sys/ccdvar.h>

/*
 * Compat code must not be called if on a platform where
 * sizeof (size_t) == sizeof (uint64_t) as CCDIOCSET will
 * be the same as CCDIOCSET_60
 */
static int
compat_60_ccdioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l,
    int (*f)(dev_t, u_long, void *, int, struct lwp *))
{
	switch (cmd) {
#if defined(COMPAT_60) && !defined(_LP64)
	case CCDIOCSET_60: {
		if (data == NULL)
			return 0;
		
		struct ccd_ioctl ccio;
       		struct ccd_ioctl_60 *ccio60 = data;

		ccio.ccio_disks = ccio60->ccio_disks;
		ccio.ccio_ndisks = ccio60->ccio_ndisks;
		ccio.ccio_ileave = ccio60->ccio_ileave;
		ccio.ccio_flags = ccio60->ccio_flags;
		ccio.ccio_unit = ccio60->ccio_unit;
		error = (*f)(dev, CCDIOCSET, &ccio, flag, l);
		if (!error) {
			/* Copy data back, adjust types if necessary */
			ccio60->ccio_disks = ccio.ccio_disks;
			ccio60->ccio_ndisks = ccio.ccio_ndisks;
			ccio60->ccio_ileave = ccio.ccio_ileave;
			ccio60->ccio_flags = ccio.ccio_flags;
			ccio60->ccio_unit = ccio.ccio_unit;
			ccio60->ccio_size = (size_t)ccio.ccio_size;
		}
		return error;
	}

	case CCDIOCCLR_60:
		if (data == NULL)
			return ENOSYS;
		/*
		 * ccio_size member not used, so existing struct OK
		 * drop through to existing non-compat version
		 */
		return (*f)(dev, CCDIOCLR, data, flag, l);
#endif /* COMPAT_60 && !_LP64*/
	default:
		return ENOSYS;
	}
}

void
ccd_60_init(void)
{
    compat_ccd_ioctl_60 = compat_60_ccdioctl;
}

void
ccd_60_fini(void)
{
    compat_ccd_ioctl_60 = (void *)enosys;
}
