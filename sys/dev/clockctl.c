/*      $NetBSD: clockctl.c,v 1.1.2.2 2001/09/21 22:35:27 nathanw Exp $ */

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef NTP
#include <sys/timex.h>
#endif /* NTP */

#include <sys/clockctl.h>

struct clockctl_softc {
	struct device   clockctl_dev;
};


void
clockctlattach(parent, self, aux)
	struct device *self;
	struct device *parent;
	void *aux;
{
	/* Nothing to set up before open is called */
	return;
}

int
clockctlopen(dev, flags, fmt, p)
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	return 0; 
}

int
clockctlclose(dev, flags, fmt, p)
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	return 0;
}

int
clockctlioctl(dev, cmd, data, flags, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flags;
	struct proc *p;
{
	int error = 0;
	
	switch (cmd) {
		case CLOCKCTL_SETTIMEOFDAY: {
			struct clockctl_settimeofday_args *args =
			    (struct clockctl_settimeofday_args *)&data;

			error = settimeofday1(&args->tv, &args->tzp, p);
			if (error)
				return (error);
			break;
		}
		case CLOCKCTL_ADJTIME: {
			struct clockctl_adjtime_args *args = 
			    (struct clockctl_adjtime_args *)&data;

			error = adjtime1(&args->delta, &args->olddelta, p);
			if (error)
				return (error);	
			break;
		}
		case CLOCKCTL_CLOCK_SETTIME: {
			struct clockctl_clock_settime_args *args = 
			    (struct clockctl_clock_settime_args *)&data;

			error = clock_settime1(args->clock_id, &args->tp);
			if (error)
				return (error);	
			break;
		}
#ifdef NTP
		case CLOCKCTL_NTP_ADJTIME: {
			struct clockctl_ntp_adjtime_args *args =
			    (struct clockctl_ntp_adjtime_args *)&data;

			(void*)ntp_adjtime1(&args->tp, &error);
			return (error);	
		}
#endif /* NTP */
		default:
			error = EINVAL;
	}

	return (error);
}


