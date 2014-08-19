/*	$NetBSD: sysmon.c,v 1.17.48.1 2014/08/20 00:03:50 tls Exp $	*/

/*-
 * Copyright (c) 2000 Zembu Labs, Inc.
 * All rights reserved.
 *
 * Author: Jason R. Thorpe <thorpej@zembu.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Zembu Labs, Inc.
 * 4. Neither the name of Zembu Labs nor the names of its employees may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ZEMBU LABS, INC. ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAR-
 * RANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DIS-
 * CLAIMED.  IN NO EVENT SHALL ZEMBU LABS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Clearing house for system monitoring hardware.  We currently
 * handle environmental sensors, watchdog timers, and power management.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysmon.c,v 1.17.48.1 2014/08/20 00:03:50 tls Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmonconf.h>

dev_type_open(sysmonopen);
dev_type_close(sysmonclose);
dev_type_ioctl(sysmonioctl);
dev_type_read(sysmonread);
dev_type_poll(sysmonpoll);
dev_type_kqfilter(sysmonkqfilter);

const struct cdevsw sysmon_cdevsw = {
	.d_open = sysmonopen,
	.d_close = sysmonclose,
	.d_read = sysmonread,
	.d_write = nowrite,
	.d_ioctl = sysmonioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = sysmonpoll,
	.d_mmap = nommap,
	.d_kqfilter = sysmonkqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

/*
 * sysmonopen:
 *
 *	Open the system monitor device.
 */
int
sysmonopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int error;

	switch (minor(dev)) {
#if NSYSMON_ENVSYS > 0
	case SYSMON_MINOR_ENVSYS:
		error = sysmonopen_envsys(dev, flag, mode, l);
		break;
#endif
#if NSYSMON_WDOG > 0
	case SYSMON_MINOR_WDOG:
		error = sysmonopen_wdog(dev, flag, mode, l);
		break;
#endif
#if NSYSMON_POWER > 0
	case SYSMON_MINOR_POWER:
		error = sysmonopen_power(dev, flag, mode, l);
		break;
#endif
	default:
		error = ENODEV;
	}

	return (error);
}

/*
 * sysmonclose:
 *
 *	Close the system monitor device.
 */
int
sysmonclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	int error;

	switch (minor(dev)) {
#if NSYSMON_ENVSYS > 0
	case SYSMON_MINOR_ENVSYS:
		error = sysmonclose_envsys(dev, flag, mode, l);
		break;
#endif
#if NSYSMON_WDOG > 0
	case SYSMON_MINOR_WDOG:
		error = sysmonclose_wdog(dev, flag, mode, l);
		break;
#endif
#if NSYSMON_POWER > 0
	case SYSMON_MINOR_POWER:
		error = sysmonclose_power(dev, flag, mode, l);
		break;
#endif
	default:
		error = ENODEV;
	}

	return (error);
}

/*
 * sysmonioctl:
 *
 *	Perform a control request.
 */
int
sysmonioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int error;

	switch (minor(dev)) {
#if NSYSMON_ENVSYS > 0
	case SYSMON_MINOR_ENVSYS:
		error = sysmonioctl_envsys(dev, cmd, data, flag, l);
		break;
#endif
#if NSYSMON_WDOG > 0
	case SYSMON_MINOR_WDOG:
		error = sysmonioctl_wdog(dev, cmd, data, flag, l);
		break;
#endif
#if NSYSMON_POWER > 0
	case SYSMON_MINOR_POWER:
		error = sysmonioctl_power(dev, cmd, data, flag, l);
		break;
#endif
	default:
		error = ENODEV;
	}

	return (error);
}

/*
 * sysmonread:
 *
 *	Perform a read request.
 */
int
sysmonread(dev_t dev, struct uio *uio, int flags)
{
	int error;

	switch (minor(dev)) {
#if NSYSMON_POWER > 0
	case SYSMON_MINOR_POWER:
		error = sysmonread_power(dev, uio, flags);
		break;
#endif
	default:
		error = ENODEV;
	}

	return (error);
}

/*
 * sysmonpoll:
 *
 *	Poll the system monitor device.
 */
int
sysmonpoll(dev_t dev, int events, struct lwp *l)
{
	int rv;

	switch (minor(dev)) {
#if NSYSMON_POWER > 0
	case SYSMON_MINOR_POWER:
		rv = sysmonpoll_power(dev, events, l);
		break;
#endif
	default:
		rv = events;
	}

	return (rv);
}

/*
 * sysmonkqfilter:
 *
 *	Kqueue filter for the system monitor device.
 */
int
sysmonkqfilter(dev_t dev, struct knote *kn)
{
	int error;

	switch (minor(dev)) {
#if NSYSMON_POWER > 0
	case SYSMON_MINOR_POWER:
		error = sysmonkqfilter_power(dev, kn);
		break;
#endif
	default:
		error = 1;
	}

	return (error);
}
