/*      $NetBSD: clockctl.c,v 1.35.14.1 2018/03/21 04:48:32 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: clockctl.c,v 1.35.14.1 2018/03/21 04:48:32 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_ntp.h"
#include "opt_compat_netbsd.h"
#endif

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
#include <sys/kauth.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/compat_stub.h>

#include <sys/clockctl.h>
#include <compat/sys/clockctl.h>
#include <compat/sys/time_types.h>


kmutex_t clockctl_mtx;
int clockctl_refcnt;

#include "ioconf.h"

dev_type_ioctl(clockctlioctl);

const struct cdevsw clockctl_cdevsw = {
	.d_open = clockctlopen,
	.d_close = clockctlclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = clockctlioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER,
};

static kauth_listener_t clockctl_listener;

static int
clockctl_listener_cb(kauth_cred_t cred, kauth_action_t action, void *cookie,
    void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;
	enum kauth_system_req req;
	bool device_context;

	result = KAUTH_RESULT_DEFER;
	req = (enum kauth_system_req)arg0;

	if ((action != KAUTH_SYSTEM_TIME) ||
	    (req != KAUTH_REQ_SYSTEM_TIME_SYSTEM))
		return result;

	device_context = (bool)arg3;

	/* Device is controlled by permissions, so allow. */
	if (device_context)
		result = KAUTH_RESULT_ALLOW;

	return result;
}

/*ARGSUSED*/
void
clockctlattach(int num)
{

/*
 * Don't initialize the listener here - it will get handled as part
 * of module initialization.
 */
#if 0
	clockctl_listener = kauth_listen_scope(KAUTH_SCOPE_SYSTEM,
	    clockctl_listener_cb, NULL);
#endif
}

/*
 * Maintain a refcount for each open/close, so we know when it is
 * safe to call devsw_detach()
 */
int
clockctlopen(dev_t dev, int flag, int mode, struct lwp *l)
{

	mutex_enter(&clockctl_mtx);
	clockctl_refcnt++;
	mutex_exit(&clockctl_mtx);

	return 0;
}

int
clockctlclose(dev_t dev, int flag, int mode, struct lwp *l)
{

	mutex_enter(&clockctl_mtx);
	clockctl_refcnt--;
	mutex_exit(&clockctl_mtx);

	return 0;
}

MODULE(MODULE_CLASS_DRIVER, clockctl, NULL);

int
clockctl_modcmd(modcmd_t cmd, void *data)
{
	int error;
#ifdef _MODULE
	int bmajor, cmajor;
#endif

	error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
		mutex_init(&clockctl_mtx, MUTEX_DEFAULT, IPL_NONE);

		clockctl_listener = kauth_listen_scope(KAUTH_SCOPE_SYSTEM,
		    clockctl_listener_cb, NULL);

#ifdef _MODULE
		bmajor = cmajor = -1;
		error = devsw_attach("clockctl", NULL, &bmajor,
		    &clockctl_cdevsw, &cmajor);
		if (error != 0)
			kauth_unlisten_scope(clockctl_listener);
#endif

		break;

	case MODULE_CMD_FINI:
		mutex_enter(&clockctl_mtx);
		if (clockctl_refcnt != 0) {
			mutex_exit(&clockctl_mtx);
			return EBUSY;
		}
#ifdef _MODULE
		error = devsw_detach(NULL, &clockctl_cdevsw);
#endif
		mutex_exit(&clockctl_mtx);

		if (error == 0) {
			kauth_unlisten_scope(clockctl_listener);
			mutex_destroy(&clockctl_mtx);
		}
		break;

	default:
		error = ENOTTY;
		break;
	}

	return error;
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
		struct clockctl_settimeofday *args = data;

		error = settimeofday1(args->tv, true, args->tzp, l, false);
		break;
	}
	case CLOCKCTL_ADJTIME: {
		struct timeval atv, oldatv;
		struct clockctl_adjtime *args = data;

		if (args->delta) {
			error = copyin(args->delta, &atv, sizeof(atv));
			if (error)
				return (error);
		}
		adjtime1(args->delta ? &atv : NULL,
		    args->olddelta ? &oldatv : NULL, l->l_proc);
		if (args->olddelta)
			error = copyout(&oldatv, args->olddelta,
			    sizeof(oldatv));
		break;
	}
	case CLOCKCTL_CLOCK_SETTIME: {
		struct clockctl_clock_settime *args = data;
		struct timespec ts;

		error = copyin(args->tp, &ts, sizeof ts);
		if (error)
			return (error);
		error = clock_settime1(l->l_proc, args->clock_id, &ts, false);
		break;
	}
#ifdef NTP
	case CLOCKCTL_NTP_ADJTIME: {
		struct clockctl_ntp_adjtime *args = data;
		struct timex ntv;

		error = copyin(args->tp, &ntv, sizeof(ntv));
		if (error)
			return (error);

		ntp_adjtime1(&ntv);

		error = copyout(&ntv, args->tp, sizeof(ntv));
		if (error == 0)
			args->retval = ntp_timestatus();
		break;
	}
#endif /* NTP */
	default:
		error = (*compat_clockctl_ioctl_50)(dev, cmd, data, flags, l);
		if (error == ENOSYS)
			error = ENOTTY;
	}

	return (error);
}
