/*	$NetBSD: sysmon_wdog.c,v 1.2.6.1 2001/09/18 19:13:51 fvdl Exp $	*/

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
 * Watchdog timer framework for sysmon.  Hardware (and software)
 * watchdog timers can register themselves here to provide a
 * watchdog function, which provides an abstract interface to the
 * user.
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

#include <dev/sysmon/sysmonvar.h>

LIST_HEAD(, sysmon_wdog) sysmon_wdog_list =
    LIST_HEAD_INITIALIZER(&sysmon_wdog_list);
int sysmon_wdog_count;
struct simplelock sysmon_wdog_list_slock = SIMPLELOCK_INITIALIZER;

struct simplelock sysmon_wdog_slock = SIMPLELOCK_INITIALIZER;
struct sysmon_wdog *sysmon_armed_wdog;
struct callout sysmon_wdog_callout = CALLOUT_INITIALIZER;
void *sysmon_wdog_sdhook;

#define	SYSMON_WDOG_LOCK(s)						\
do {									\
	s = splsoftclock();						\
	simple_lock(&sysmon_wdog_slock);				\
} while (0)

#define	SYSMON_WDOG_UNLOCK(s)						\
do {									\
	simple_unlock(&sysmon_wdog_slock);				\
	splx(s);							\
} while (0)

struct sysmon_wdog *sysmon_wdog_find(const char *);
void	sysmon_wdog_release(struct sysmon_wdog *);
int	sysmon_wdog_setmode(struct sysmon_wdog *, int, u_int);
void	sysmon_wdog_ktickle(void *);
void	sysmon_wdog_shutdown(void *);

#define	SYSMON_MINOR_ENVSYS	0
#define	SYSMON_MINOR_WDOG	1

/*
 * sysmonopen_wdog:
 *
 *	Open the system monitor device.
 */
int
sysmonopen_wdog(struct vnode *devvp, int flag, int mode, struct proc *p)
{

	simple_lock(&sysmon_wdog_list_slock);
	if (sysmon_wdog_sdhook == NULL) {
		sysmon_wdog_sdhook =
		    shutdownhook_establish(sysmon_wdog_shutdown, NULL);
		if (sysmon_wdog_sdhook == NULL)
			printf("WARNING: unable to register watchdog "
			    "shutdown hook\n");
	}
	simple_unlock(&sysmon_wdog_list_slock);

	return (0);
}

/*
 * sysmonclose_wdog:
 *
 *	Close the system monitor device.
 */
int
sysmonclose_wdog(struct vnode *devvp, int flag, int mode, struct proc *p)
{
	struct sysmon_wdog *smw;
	int omode, s, error;

	/*
	 * If this is the last close, and there is a watchdog
	 * running in UTICKLE mode, we need to disable it,
	 * otherwise the system will reset in short order.
	 *
	 * XXX Maybe we should just go into KTICKLE mode?
	 */
	SYSMON_WDOG_LOCK(s);
	if ((smw = sysmon_armed_wdog) != NULL) {
		if ((omode = smw->smw_mode) == WDOG_MODE_UTICKLE) {
			error = sysmon_wdog_setmode(smw,
			    WDOG_MODE_DISARMED, smw->smw_period);
			if (error) {
				printf("WARNING: UNABLE TO DISARM "
				    "WATCHDOG %s ON CLOSE!\n",
				    smw->smw_name);
				/*
				 * ...we will probably reboot soon.
				 */
			}
		}
	}
	SYSMON_WDOG_UNLOCK(s);

	return (error);
}

/*
 * sysmonioctl_wdog:
 *
 *	Perform a watchdog control request.
 */
int
sysmonioctl_wdog(struct vnode *devvp, u_long cmd, caddr_t data, int flag,
		 struct proc *p)
{
	struct sysmon_wdog *smw;
	int s, error = 0;

	switch (cmd) {
	case WDOGIOC_GMODE:
	    {
		struct wdog_mode *wm = (void *) data;

		wm->wm_name[sizeof(wm->wm_name) - 1] = '\0';
		smw = sysmon_wdog_find(wm->wm_name);
		if (smw == NULL) {
			error = ESRCH;
			break;
		}

		wm->wm_mode = smw->smw_mode;
		wm->wm_period = smw->smw_period;
		sysmon_wdog_release(smw);
		break;
	    }

	case WDOGIOC_SMODE:
	    {
		struct wdog_mode *wm = (void *) data;

		if ((flag & FWRITE) == 0) {
			error = EPERM;
			break;
		}

		wm->wm_name[sizeof(wm->wm_name) - 1] = '\0';
		smw = sysmon_wdog_find(wm->wm_name);
		if (smw == NULL) {
			error = ESRCH;
			break;
		}

		if (wm->wm_mode & ~(WDOG_MODE_MASK|WDOG_FEATURE_MASK))
			error = EINVAL;
		else {
			SYSMON_WDOG_LOCK(s);
			error = sysmon_wdog_setmode(smw, wm->wm_mode,
			    wm->wm_period);
			SYSMON_WDOG_UNLOCK(s);
		}

		sysmon_wdog_release(smw);
		break;
	    }

	case WDOGIOC_WHICH:
	    {
		struct wdog_mode *wm = (void *) data;

		SYSMON_WDOG_LOCK(s);
		if ((smw = sysmon_armed_wdog) != NULL) {
			strcpy(wm->wm_name, smw->smw_name);
			wm->wm_mode = smw->smw_mode;
			wm->wm_period = smw->smw_period;
		} else
			error = ESRCH;
		SYSMON_WDOG_UNLOCK(s);
		break;
	    }

	case WDOGIOC_TICKLE:
		if ((flag & FWRITE) == 0) {
			error = EPERM;
			break;
		}

		SYSMON_WDOG_LOCK(s);
		if ((smw = sysmon_armed_wdog) != NULL) {
			error = (*smw->smw_tickle)(smw);
			if (error == 0)
				smw->smw_tickler = p->p_pid;
		} else
			error = ESRCH;
		SYSMON_WDOG_UNLOCK(s);
		break;

	case WDOGIOC_GTICKLER:
		*(pid_t *)data = smw->smw_tickler;
		break;

	case WDOGIOC_GWDOGS:
	    {
		struct wdog_conf *wc = (void *) data;
		char *cp;
		int i;

		simple_lock(&sysmon_wdog_list_slock);
		if (wc->wc_names == NULL)
			wc->wc_count = sysmon_wdog_count;
		else {
			for (i = 0, cp = wc->wc_names,
			       smw = LIST_FIRST(&sysmon_wdog_list);
			     i < sysmon_wdog_count && smw != NULL && error == 0;
			     i++, cp += WDOG_NAMESIZE,
			       smw = LIST_NEXT(smw, smw_list))
				error = copyout(smw->smw_name, cp,
				    strlen(smw->smw_name) + 1);
			wc->wc_count = i;
		}
		simple_unlock(&sysmon_wdog_list_slock);
		break;
	    }

	default:
		error = ENOTTY;
	}

	return (error);
}

/*
 * sysmon_wdog_register:
 *
 *	Register a watchdog device.
 */
int
sysmon_wdog_register(struct sysmon_wdog *smw)
{
	struct sysmon_wdog *lsmw;
	int error = 0;

	simple_lock(&sysmon_wdog_list_slock);

	for (lsmw = LIST_FIRST(&sysmon_wdog_list); lsmw != NULL;
	     lsmw = LIST_NEXT(lsmw, smw_list)) {
		if (strcmp(lsmw->smw_name, smw->smw_name) == 0) {
			error = EEXIST;
			goto out;
		}
	}

	smw->smw_mode = WDOG_MODE_DISARMED;
	smw->smw_tickler = (pid_t) -1;
	smw->smw_refcnt = 0;
	sysmon_wdog_count++;
	LIST_INSERT_HEAD(&sysmon_wdog_list, smw, smw_list);

 out:
	simple_unlock(&sysmon_wdog_list_slock);
	return (error);
}

/*
 * sysmon_wdog_unregister:
 *
 *	Unregister a watchdog device.
 */
void
sysmon_wdog_unregister(struct sysmon_wdog *smw)
{

	simple_lock(&sysmon_wdog_list_slock);
	sysmon_wdog_count--;
	LIST_REMOVE(smw, smw_list);
	simple_unlock(&sysmon_wdog_list_slock);
}

/*
 * sysmon_wdog_find:
 *
 *	Find a watchdog device.  We increase the reference
 *	count on a match.
 */
struct sysmon_wdog *
sysmon_wdog_find(const char *name)
{
	struct sysmon_wdog *smw;

	simple_lock(&sysmon_wdog_list_slock);

	for (smw = LIST_FIRST(&sysmon_wdog_list); smw != NULL;
	     smw = LIST_NEXT(smw, smw_list)) {
		if (strcmp(smw->smw_name, name) == 0)
			break;
	}

	if (smw != NULL)
		smw->smw_refcnt++;

	simple_unlock(&sysmon_wdog_list_slock);
	return (smw);
}

/*
 * sysmon_wdog_release:
 *
 *	Release a watchdog device.
 */
void
sysmon_wdog_release(struct sysmon_wdog *smw)
{

	simple_lock(&sysmon_wdog_list_slock);
	KASSERT(smw->smw_refcnt != 0);
	smw->smw_refcnt--;
	simple_unlock(&sysmon_wdog_list_slock);
}

/*
 * sysmon_wdog_setmode:
 *
 *	Set the mode of a watchdog device.
 */
int
sysmon_wdog_setmode(struct sysmon_wdog *smw, int mode, u_int period)
{
	u_int operiod = smw->smw_period;
	int omode = smw->smw_mode;
	int error = 0;

	smw->smw_period = period;
	smw->smw_mode = mode;

	switch (mode & WDOG_MODE_MASK) {
	case WDOG_MODE_DISARMED:
		if (smw != sysmon_armed_wdog) {
			error = EINVAL;
			goto out;
		}
		break;

	case WDOG_MODE_KTICKLE:
	case WDOG_MODE_UTICKLE:
		if (sysmon_armed_wdog != NULL) {
			error = EBUSY;
			goto out;
		}
		break;

	default:
		error = EINVAL;
		goto out;
	}

	error = (*smw->smw_setmode)(smw);

 out:
	if (error) {
		smw->smw_period = operiod;
		smw->smw_mode = omode;
	} else {
		if ((mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
			sysmon_armed_wdog = NULL;
			smw->smw_tickler = (pid_t) -1;
			smw->smw_refcnt--;
			if ((omode & WDOG_MODE_MASK) == WDOG_MODE_KTICKLE)
				callout_stop(&sysmon_wdog_callout);
		} else {
			sysmon_armed_wdog = smw;
			smw->smw_refcnt++;
			if ((mode & WDOG_MODE_MASK) == WDOG_MODE_KTICKLE) {
				callout_reset(&sysmon_wdog_callout,
				    WDOG_PERIOD_TO_TICKS(smw->smw_period) / 2,
				    sysmon_wdog_ktickle, NULL);
			}
		}
	}
	return (error);
}

/*
 * sysmon_wdog_ktickle:
 *
 *	Kernel watchdog tickle routine.
 */
void
sysmon_wdog_ktickle(void *arg)
{
	struct sysmon_wdog *smw;
	int s;

	SYSMON_WDOG_LOCK(s);
	if ((smw = sysmon_armed_wdog) != NULL) {
		if ((*smw->smw_tickle)(smw) != 0) {
			printf("WARNING: KERNEL TICKLE OF WATCHDOG %s "
			    "FAILED!\n", smw->smw_name);
			/*
			 * ...we will probably reboot soon.
			 */
		}
		callout_reset(&sysmon_wdog_callout,
		    WDOG_PERIOD_TO_TICKS(smw->smw_period) / 2,
		    sysmon_wdog_ktickle, NULL);
	}
	SYSMON_WDOG_UNLOCK(s);
}

/*
 * sysmon_wdog_shutdown:
 *
 *	Perform shutdown-time operations.
 */
void
sysmon_wdog_shutdown(void *arg)
{
	struct sysmon_wdog *smw;

	/*
	 * XXX Locking here?  I don't think it's necessary.
	 */

	if ((smw = sysmon_armed_wdog) != NULL) {
		if (sysmon_wdog_setmode(smw, WDOG_MODE_DISARMED,
		    smw->smw_period))
			printf("WARNING: FAILED TO SHUTDOWN WATCHDOG %s!\n",
			    smw->smw_name);
	}
}
