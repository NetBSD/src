/*	$NetBSD: sysmon.c,v 1.2 2000/06/28 06:51:17 thorpej Exp $	*/

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
 * Clearing house for system monitoring hardware.  This pseudo-device
 * is a place where hardware monitors such as the LM78 and VIA 82C686A
 * (or even ACPI, eventually) can register themselves to provide
 * backplane fan and temperature information, etc.
 *
 * Eventually, we will also provide a way for a hardware watchdog timer
 * to hook itself up here (with an option to fall back on a software
 * watchdog timer if hardware is not available).
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/lock.h>

#include <dev/sysmon/sysmonvar.h>

/*
 * We run at ENVSYS version 1.
 */
#define	SYSMON_ENVSYS_VERSION	(1 * 1000)

struct lock sysmon_lock;

LIST_HEAD(, sysmon_envsys) sysmon_envsys_list;
struct simplelock sysmon_envsys_list_slock = SIMPLELOCK_INITIALIZER;
u_int	sysmon_envsys_next_sensor_index;

int	sysmon_initialized;
struct simplelock sysmon_initialized_slock = SIMPLELOCK_INITIALIZER;

cdev_decl(sysmon);

void	sysmon_init(void);

struct sysmon_envsys *sysmon_envsys_find(u_int);
void	sysmon_envsys_release(struct sysmon_envsys *);

/*
 * sysmon_init:
 *
 *	Initialize the system monitor.
 */
void
sysmon_init(void)
{

	lockinit(&sysmon_lock, PWAIT|PCATCH, "sysmon", 0, 0);
	sysmon_initialized = 1;
}

/*
 * sysmonopen:
 *
 *	Open the system monitor device.
 */
int
sysmonopen(dev_t dev, int flag, int mode, struct proc *p)
{
	int error;

	simple_lock(&sysmon_initialized_slock);
	if (sysmon_initialized == 0)
		sysmon_init();

	error = lockmgr(&sysmon_lock,
	    LK_EXCLUSIVE | LK_INTERLOCK |
	    ((flag & O_NONBLOCK) ? LK_NOWAIT : 0), &sysmon_initialized_slock);

	return (error);
}

/*
 * sysmonclose:
 *
 *	Close the system monitor device.
 */
int
sysmonclose(dev_t dev, int flag, int mode, struct proc *p)
{

	(void) lockmgr(&sysmon_lock, LK_RELEASE, NULL);
	return (0);
}

/*
 * sysmonioctl:
 *
 *	Perform a control request.
 */
int
sysmonioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct sysmon_envsys *sme;
	int error = 0;
	u_int oidx;

	switch (cmd) {
	/*
	 * For ENVSYS commands, we translate the absolute sensor index
	 * to a device-relative sensor index.
	 */
	case ENVSYS_VERSION:
		*(int32_t *)data = SYSMON_ENVSYS_VERSION;
		break;

	case ENVSYS_GRANGE:
	    {
		struct envsys_range *rng = (void *) data;

		sme = sysmon_envsys_find(0);	/* XXX */
		if (sme == NULL) {
			/* Return empty range for `no sensors'. */
			rng->low = 1;
			rng->high = 0;
			break;
		}

		if (rng->units < ENVSYS_NSENSORS)
			*rng = sme->sme_ranges[rng->units];
		else {
			/* Return empty range for unsupported sensor types. */
			rng->low = 1;
			rng->high = 0;
		}
		sysmon_envsys_release(sme);
		break;
	    }

	case ENVSYS_GTREDATA:
	    {
		struct envsys_tre_data *tred = (void *) data;

		tred->validflags = 0;

		sme = sysmon_envsys_find(tred->sensor);
		if (sme == NULL)
			break;
		oidx = tred->sensor;
		tred->sensor = SME_SENSOR_IDX(sme, tred->sensor);
		if (tred->sensor < sme->sme_nsensors)
			error = (*sme->sme_gtredata)(sme, tred);
		tred->sensor = oidx;
		sysmon_envsys_release(sme);
		break;
	    }

	case ENVSYS_STREINFO:
	    {
		struct envsys_basic_info *binfo = (void *) data;

		sme = sysmon_envsys_find(binfo->sensor);
		if (sme == NULL) {
			binfo->validflags = 0;
			break;
		}
		oidx = binfo->sensor;
		binfo->sensor = SME_SENSOR_IDX(sme, binfo->sensor);
		if (binfo->sensor < sme->sme_nsensors)
			error = (*sme->sme_streinfo)(sme, binfo);
		else
			binfo->validflags = 0;
		binfo->sensor = oidx;
		sysmon_envsys_release(sme);
		break;
	    }

	case ENVSYS_GTREINFO:
	    {
		struct envsys_basic_info *binfo = (void *) data;

		binfo->validflags = 0;

		sme = sysmon_envsys_find(binfo->sensor);
		if (sme == NULL)
			break;
		oidx = binfo->sensor;
		binfo->sensor = SME_SENSOR_IDX(sme, binfo->sensor);
		if (binfo->sensor < sme->sme_nsensors)
			*binfo = sme->sme_sensor_info[binfo->sensor];
		binfo->sensor = oidx;
		sysmon_envsys_release(sme);
		break;
	    }

	default:
		error = ENOTTY;
	}

	return (error);
}

/*
 * sysmon_envsys_register:
 *
 *	Register an ENVSYS device.
 */
int
sysmon_envsys_register(struct sysmon_envsys *sme)
{
	int error = 0;

	simple_lock(&sysmon_envsys_list_slock);

	/* XXX Only get to register one, for now. */
	if (LIST_FIRST(&sysmon_envsys_list) != NULL) {
		error = EEXIST;
		goto out;
	}

	if (sme->sme_envsys_version != SYSMON_ENVSYS_VERSION) {
		error = EINVAL;
		goto out;
	}

	sme->sme_fsensor = sysmon_envsys_next_sensor_index;
	sysmon_envsys_next_sensor_index += sme->sme_nsensors;
	LIST_INSERT_HEAD(&sysmon_envsys_list, sme, sme_list);

 out:
	simple_unlock(&sysmon_envsys_list_slock);
	return (error);
}

/*
 * sysmon_envsys_unregister:
 *
 *	Unregister an ENVSYS device.
 */
void
sysmon_envsys_unregister(struct sysmon_envsys *sme)
{

	simple_lock(&sysmon_envsys_list_slock);
	LIST_REMOVE(sme, sme_list);
	simple_unlock(&sysmon_envsys_list_slock);
}

/*
 * sysmon_envsys_find:
 *
 *	Find an ENVSYS device.  The list remains locked upon
 *	a match.
 */
struct sysmon_envsys *
sysmon_envsys_find(u_int idx)
{
	struct sysmon_envsys *sme;

	simple_lock(&sysmon_envsys_list_slock);

	for (sme = LIST_FIRST(&sysmon_envsys_list); sme != NULL;
	     sme = LIST_NEXT(sme, sme_list)) {
		if (idx >= sme->sme_fsensor &&
		    idx < (sme->sme_fsensor + sme->sme_nsensors))
			return (sme);
	}

	simple_unlock(&sysmon_envsys_list_slock);
	return (NULL);
}

/*
 * sysmon_envsys_release:
 *
 *	Release an ENVSYS device.
 */
/* ARGSUSED */
void
sysmon_envsys_release(struct sysmon_envsys *sme)
{

	simple_unlock(&sysmon_envsys_list_slock);
}
