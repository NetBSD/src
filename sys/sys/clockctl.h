/*      $NetBSD: clockctl.h,v 1.1.2.2 2001/09/21 22:37:00 nathanw Exp $ */

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
 *
 *
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/time.h>
#include <sys/timex.h>

#ifndef SYS_CLOCKCTL_H
#define SYS_CLOCKCTL_H

struct clockctl_settimeofday_args {
	struct timeval tv;
	struct timezone tzp;
};
struct clockctl_adjtime_args {
	struct timeval delta;
	struct timeval olddelta;
};
struct clockctl_clock_settime_args {
	clockid_t clock_id;
	struct timespec tp;
};

struct clockctl_ntp_adjtime_args {
	struct timex tp;
};

#define CLOCKCTL_SETTIMEOFDAY _IOW('C', 0x1, struct clockctl_settimeofday_args)
#define CLOCKCTL_ADJTIME _IOWR('C', 0x2, struct clockctl_adjtime_args)
#define CLOCKCTL_CLOCK_SETTIME \
    _IOW('C', 0x3, struct clockctl_clock_settime_args)
#define CLOCKCTL_NTP_ADJTIME _IOWR('C', 0x4, struct clockctl_ntp_adjtime_args)

#ifdef _KERNEL
void    clockctlattach __P((struct device *, struct device *, void *));
int     clockctlopen __P((dev_t, int, int, struct proc *));
int     clockctlclose __P((dev_t, int, int, struct proc *));
int     clockctlioctl __P((dev_t, u_long, caddr_t, int, struct proc *));
#endif
#endif

