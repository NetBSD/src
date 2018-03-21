/*      $NetBSD: clockctl_50.c,v 1.1.2.1 2018/03/21 04:48:31 pgoyette Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clockctl_50.c,v 1.1.2.1 2018/03/21 04:48:31 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/time.h>
#include <sys/conf.h>
#include <sys/timex.h>
#include <sys/kauth.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/compat_stub.h>

#include <sys/clockctl.h>
#include <compat/sys/clockctl.h>
#include <compat/sys/time_types.h>

int
compat50_clockctlioctl(dev_t dev, u_long cmd, void *data, int flags,
    struct lwp *l)
{
	int error = 0;
	const struct cdevsw *cd = cdevsw_lookup(dev);

	if (cd == NULL || cd->d_ioctl == NULL)
		return ENXIO;

	switch (cmd) {
	case CLOCKCTL_OSETTIMEOFDAY: {
		struct timeval50 tv50;
		struct timeval tv;
		struct clockctl50_settimeofday *args = data;

		error = copyin(args->tv, &tv50, sizeof(tv50));
		if (error)
			return (error);
		timeval50_to_timeval(&tv50, &tv);
		error = settimeofday1(&tv, false, args->tzp, l, false);
		break;
	}
	case CLOCKCTL_OADJTIME: {
		struct timeval atv, oldatv;
		struct timeval50 atv50;
		struct clockctl50_adjtime *args = data;

		if (args->delta) {
			error = copyin(args->delta, &atv50, sizeof(atv50));
			if (error)
				return (error);
			timeval50_to_timeval(&atv50, &atv);
		}
		adjtime1(args->delta ? &atv : NULL,
		    args->olddelta ? &oldatv : NULL, l->l_proc);
		if (args->olddelta) {
			timeval_to_timeval50(&oldatv, &atv50);
			error = copyout(&atv50, args->olddelta, sizeof(atv50));
		}
		break;
	}
	case CLOCKCTL_OCLOCK_SETTIME: {
		struct timespec50 tp50;
		struct timespec tp;
		struct clockctl50_clock_settime *args = data;

		error = copyin(args->tp, &tp50, sizeof(tp50));
		if (error)
			return (error);
		timespec50_to_timespec(&tp50, &tp);
		error = clock_settime1(l->l_proc, args->clock_id, &tp, true);
		break;
	}
	case CLOCKCTL_ONTP_ADJTIME: {
		if (vec_ntp_timestatus == NULL) {
			error = ENOTTY;
			break;
		}
		/* The ioctl number changed but the data did not change. */
		error = (cd->d_ioctl)(dev, CLOCKCTL_NTP_ADJTIME,
		    data, flags, l);
		break;
	}
	default:
		error = ENOTTY;
	}

	return (error);
}

void
clockctl_50_init(void)
{

	compat_clockctl_ioctl_50 = compat50_clockctlioctl;
}

void
clockctl_50_fini(void)
{

	compat_clockctl_ioctl_50 = (void *)enosys;
}
