/*	$NetBSD: sysmon_envsys.c,v 1.12 2006/10/12 01:31:59 christos Exp $	*/

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
 * Environmental sensor framework for sysmon.  Hardware monitors
 * such as the LM78 and VIA 82C686A (or even ACPI, eventually) can
 * register themselves to provide backplane fan and temperature
 * information, etc.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysmon_envsys.c,v 1.12 2006/10/12 01:31:59 christos Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/lock.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <dev/sysmon/sysmonvar.h>

/*
 * We run at ENVSYS version 1.
 */
#define	SYSMON_ENVSYS_VERSION	(1 * 1000)

struct lock sysmon_envsys_lock;

LIST_HEAD(, sysmon_envsys) sysmon_envsys_list =
    LIST_HEAD_INITIALIZER(&sysmon_envsys_list);
struct simplelock sysmon_envsys_list_slock = SIMPLELOCK_INITIALIZER;
u_int	sysmon_envsys_next_sensor_index;

int	sysmon_envsys_initialized;
struct simplelock sysmon_envsys_initialized_slock = SIMPLELOCK_INITIALIZER;

#define	SYSMON_ENVSYS_LOCK()	\
	lockmgr(&sysmon_envsys_lock, LK_EXCLUSIVE, NULL)

#define SYSMON_ENVSYS_UNLOCK()		\
	lockmgr(&sysmon_envsys_lock, LK_RELEASE, NULL)

struct sysmon_envsys *sysmon_envsys_find(u_int);
void	sysmon_envsys_release(struct sysmon_envsys *);

/*
 * sysmonopen_envsys:
 *
 *	Open the system monitor device.
 */
int
sysmonopen_envsys(dev_t dev __unused, int flag __unused, int mode __unused,
    struct lwp *l __unused)
{
	simple_lock(&sysmon_envsys_initialized_slock);
	if (sysmon_envsys_initialized == 0) {
		lockinit(&sysmon_envsys_lock, PWAIT|PCATCH, "smenv", 0, 0);
		sysmon_envsys_initialized = 1;
	}
	simple_unlock(&sysmon_envsys_initialized_slock);

	return (0);
}

/*
 * sysmonclose_envsys:
 *
 *	Close the system monitor device.
 */
int
sysmonclose_envsys(dev_t dev __unused, int flag __unused, int mode __unused,
    struct lwp *l __unused)
{

	/* Nothing to do */
	return (0);
}

/*
 * sysmonioctl_envsys:
 *
 *	Perform an envsys control request.
 */
int
sysmonioctl_envsys(dev_t dev __unused, u_long cmd, caddr_t data,
    int flag __unused, struct lwp *l __unused)
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
		int i;


		/* Return empty range unless we find something better */
		rng->low = 1;
		rng->high = 0;

		if (rng->units == -1) {
			rng->low = 0;
			rng->high = sysmon_envsys_next_sensor_index;
			break;
		}

		sme = sysmon_envsys_find(0);	/* XXX */
		if (sme == NULL) {
			/* Return empty range for `no sensors'. */
			break;
		}
		for (i = 0;
		     sme->sme_ranges[i].low <= sme->sme_ranges[i].high;
		     i++) {
			if (sme->sme_ranges[i].units == rng->units) {
				*rng = sme->sme_ranges[i];
				break;
			}
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
		if (tred->sensor < sme->sme_nsensors
		    && sme->sme_gtredata != NULL) {
			SYSMON_ENVSYS_LOCK();
			error = (*sme->sme_gtredata)(sme, tred);
			SYSMON_ENVSYS_UNLOCK();
		}
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
		if (binfo->sensor < sme->sme_nsensors
		    && sme->sme_streinfo != NULL) {
			SYSMON_ENVSYS_LOCK();
			error = (*sme->sme_streinfo)(sme, binfo);
			SYSMON_ENVSYS_UNLOCK();
		} else
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

	KASSERT((sme->sme_flags & (SME_FLAG_BUSY | SME_FLAG_WANTED)) == 0);
	simple_lock(&sysmon_envsys_list_slock);

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
	while (sme->sme_flags & SME_FLAG_BUSY) {
		sme->sme_flags |= SME_FLAG_WANTED;
		ltsleep(sme, PWAIT, "smeunreg", 0, &sysmon_envsys_list_slock);
	}
	LIST_REMOVE(sme, sme_list);
	simple_unlock(&sysmon_envsys_list_slock);
}

/*
 * sysmon_envsys_find:
 *
 *	Find an ENVSYS device.
 *	the found device should be sysmon_envsys_release'ed by the caller.
 */
struct sysmon_envsys *
sysmon_envsys_find(u_int idx)
{
	struct sysmon_envsys *sme;

	simple_lock(&sysmon_envsys_list_slock);
again:
	for (sme = LIST_FIRST(&sysmon_envsys_list); sme != NULL;
	     sme = LIST_NEXT(sme, sme_list)) {
		if (idx >= sme->sme_fsensor &&
		    idx < (sme->sme_fsensor + sme->sme_nsensors)) {
			if (sme->sme_flags & SME_FLAG_BUSY) {
				sme->sme_flags |= SME_FLAG_WANTED;
				ltsleep(sme, PWAIT, "smefind", 0,
				    &sysmon_envsys_list_slock);
				goto again;
			}
			sme->sme_flags |= SME_FLAG_BUSY;
			break;
		}
	}

	simple_unlock(&sysmon_envsys_list_slock);
	return sme;
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

	KASSERT(sme->sme_flags & SME_FLAG_BUSY);

	simple_lock(&sysmon_envsys_list_slock);
	if (sme->sme_flags & SME_FLAG_WANTED)
		wakeup(sme);
	sme->sme_flags &= ~(SME_FLAG_BUSY | SME_FLAG_WANTED);
	simple_unlock(&sysmon_envsys_list_slock);
}
