/*      $NetBSD: clockctl.c,v 1.22 2007/05/12 20:27:13 dsl Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: clockctl.c,v 1.22 2007/05/12 20:27:13 dsl Exp $");

#include "opt_ntp.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/time.h>
#include <sys/conf.h>
#ifdef NTP
#include <sys/timex.h>
#endif /* NTP */

#include <sys/clockctl.h>

struct clockctl_softc {
	struct device   clockctl_dev;
};

dev_type_ioctl(clockctlioctl);

const struct cdevsw clockctl_cdevsw = {
	nullopen, nullclose, noread, nowrite, clockctlioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_OTHER,
};

/*ARGSUSED*/
void
clockctlattach(int num)
{
	/* Nothing to set up before open is called */
	return;
}

int
clockctlioctl(
    dev_t dev,
    u_long cmd,
    void *data,
    int flags,
    struct lwp *l)
{
	int error = 0;

	switch (cmd) {
		case CLOCKCTL_SETTIMEOFDAY: {
			struct clockctl_settimeofday *args =
			    (struct clockctl_settimeofday *)data;

			error = settimeofday1(args->tv, true, args->tzp, l, false);
			if (error)
				return (error);
			break;
		}
		case CLOCKCTL_ADJTIME: {
			struct clockctl_adjtime *args =
			    (struct clockctl_adjtime *)data;

			error = adjtime1(args->delta, args->olddelta,
			    l->l_proc);
			if (error)
				return (error);
			break;
		}
		case CLOCKCTL_CLOCK_SETTIME: {
			struct clockctl_clock_settime *args =
			    (struct clockctl_clock_settime *)data;

			error = clock_settime1(l->l_proc, args->clock_id,
			    args->tp);
			if (error)
				return (error);
			break;
		}
#ifdef NTP
		case CLOCKCTL_NTP_ADJTIME: {
			struct clockctl_ntp_adjtime *args =
			    (struct clockctl_ntp_adjtime *)data;
			struct timex ntv;
			register_t retval;

			error = copyin(args->tp, &ntv, sizeof(ntv));
			if (error)
				return (error);

			ntp_adjtime1(&ntv);

			error = copyout(&ntv, args->tp, sizeof(ntv));
			if (error == 0)
				(void)copyout(&retval, &args->retval, sizeof(retval));

			return (error);
		}
#endif /* NTP */
		default:
			error = EINVAL;
	}

	return (error);
}


