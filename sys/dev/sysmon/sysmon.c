/*	$NetBSD: sysmon.c,v 1.4.8.1 2001/09/18 19:13:51 fvdl Exp $	*/

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
 * handle environmental sensors and watchdog timers.
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/lock.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vnode.h>

#include <miscfs/specfs/specdev.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmonconf.h>

cdev_decl(sysmon);

/*
 * sysmonopen:
 *
 *	Open the system monitor device.
 */
int
sysmonopen(struct vnode *devvp, int flag, int mode, struct proc *p)
{
	int error;

	switch (minor(devvp->v_rdev)) {
#if NSYSMON_ENVSYS > 0
	case SYSMON_MINOR_ENVSYS:
		error = sysmonopen_envsys(devvp, flag, mode, p);
		break;
#endif
#if NSYSMON_WDOG > 0
	case SYSMON_MINOR_WDOG:
		error = sysmonopen_wdog(devvp, flag, mode, p);
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
sysmonclose(struct vnode *devvp, int flag, int mode, struct proc *p)
{
	int error;

	switch (minor(devvp->v_rdev)) {
#if NSYSMON_ENVSYS > 0
	case SYSMON_MINOR_ENVSYS:
		error = sysmonclose_envsys(devvp, flag, mode, p);
		break;
#endif
#if NSYSMON_WDOG > 0
	case SYSMON_MINOR_WDOG:
		error = sysmonclose_wdog(devvp, flag, mode, p);
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
sysmonioctl(struct vnode *devvp, u_long cmd, caddr_t data, int flag,
	    struct proc *p)
{
	int error;

	switch (minor(devvp->v_rdev)) {
#if NSYSMON_ENVSYS > 0
	case SYSMON_MINOR_ENVSYS:
		error = sysmonioctl_envsys(devvp, cmd, data, flag, p);
		break;
#endif
#if NSYSMON_WDOG > 0
	case SYSMON_MINOR_WDOG:
		error = sysmonioctl_wdog(devvp, cmd, data, flag, p);
		break;
#endif
	default:
		error = ENODEV;
	}

	return (error);
}
