/*	$NetBSD: sysmon_power.c,v 1.8 2003/07/14 15:47:28 lukem Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Power management framework for sysmon.
 *
 * We defer to a power management daemon running in userspace, since
 * power management is largely a policy issue.  This merely provides
 * for power management event notification to that daemon.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysmon_power.c,v 1.8 2003/07/14 15:47:28 lukem Exp $");

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/vnode.h>

#include <dev/sysmon/sysmonvar.h>

static LIST_HEAD(, sysmon_pswitch) sysmon_pswitch_list =
    LIST_HEAD_INITIALIZER(sysmon_pswitch_list);
static struct simplelock sysmon_pswitch_list_slock =
    SIMPLELOCK_INITIALIZER;

static struct proc *sysmon_power_daemon;

#define	SYSMON_MAX_POWER_EVENTS		32

static struct simplelock sysmon_power_event_queue_slock =
    SIMPLELOCK_INITIALIZER;
static power_event_t sysmon_power_event_queue[SYSMON_MAX_POWER_EVENTS];
static int sysmon_power_event_queue_head;
static int sysmon_power_event_queue_tail;
static int sysmon_power_event_queue_count;
static int sysmon_power_event_queue_flags;
static struct selinfo sysmon_power_event_queue_selinfo;

static char sysmon_power_type[32];

#define	PEVQ_F_WAITING		0x01	/* daemon waiting for event */

#define	SYSMON_NEXT_EVENT(x)		(((x) + 1) / SYSMON_MAX_POWER_EVENTS)

/*
 * sysmon_queue_power_event:
 *
 *	Enqueue a power event for the power mangement daemon.  Returns
 *	non-zero if we were able to enqueue a power event.
 */
static int
sysmon_queue_power_event(power_event_t *pev)
{

	LOCK_ASSERT(simple_lock_held(&sysmon_power_event_queue_slock));

	if (sysmon_power_event_queue_count == SYSMON_MAX_POWER_EVENTS)
		return (0);

	sysmon_power_event_queue[sysmon_power_event_queue_head] = *pev;
	sysmon_power_event_queue_head =
	    SYSMON_NEXT_EVENT(sysmon_power_event_queue_head);
	sysmon_power_event_queue_count++;

	return (1);
}

/*
 * sysmon_get_power_event:
 *
 *	Get a power event from the queue.  Returns non-zero if there
 *	is an event available.
 */
static int
sysmon_get_power_event(power_event_t *pev)
{

	LOCK_ASSERT(simple_lock_held(&sysmon_power_event_queue_slock));

	if (sysmon_power_event_queue_count == 0)
		return (0);

	*pev = sysmon_power_event_queue[sysmon_power_event_queue_tail];
	sysmon_power_event_queue_tail =
	    SYSMON_NEXT_EVENT(sysmon_power_event_queue_tail);
	sysmon_power_event_queue_count--;

	return (1);
}

/*
 * sysmon_power_event_queue_flush:
 *
 *	Flush the event queue, and reset all state.
 */
static void
sysmon_power_event_queue_flush(void)
{

	sysmon_power_event_queue_head = 0;
	sysmon_power_event_queue_tail = 0;
	sysmon_power_event_queue_count = 0;
	sysmon_power_event_queue_flags = 0;
}

/*
 * sysmonopen_power:
 *
 *	Open the system monitor device.
 */
int
sysmonopen_power(dev_t dev, int flag, int mode, struct proc *p)
{
	int error = 0;

	simple_lock(&sysmon_power_event_queue_slock);
	if (sysmon_power_daemon != NULL)
		error = EBUSY;
	else {
		sysmon_power_daemon = p;
		sysmon_power_event_queue_flush();
	}
	simple_unlock(&sysmon_power_event_queue_slock);

	return (error);
}

/*
 * sysmonclose_power:
 *
 *	Close the system monitor device.
 */
int
sysmonclose_power(dev_t dev, int flag, int mode, struct proc *p)
{
	int count;

	simple_lock(&sysmon_power_event_queue_slock);
	count = sysmon_power_event_queue_count;
	sysmon_power_daemon = NULL;
	sysmon_power_event_queue_flush();
	simple_unlock(&sysmon_power_event_queue_slock);

	if (count)
		printf("WARNING: %d power events lost by exiting daemon\n",
		    count);

	return (0);
}

/*
 * sysmonread_power:
 *
 *	Read the system monitor device.
 */
int
sysmonread_power(dev_t dev, struct uio *uio, int flags)
{
	power_event_t pev;
	int error;

	/* We only allow one event to be read at a time. */
	if (uio->uio_resid != POWER_EVENT_MSG_SIZE)
		return (EINVAL);

	simple_lock(&sysmon_power_event_queue_slock);
 again:
	if (sysmon_get_power_event(&pev)) {
		simple_unlock(&sysmon_power_event_queue_slock);
		return (uiomove(&pev, POWER_EVENT_MSG_SIZE, uio));
	}

	if (flags & IO_NDELAY) {
		simple_unlock(&sysmon_power_event_queue_slock);
		return (EWOULDBLOCK);
	}

	sysmon_power_event_queue_flags |= PEVQ_F_WAITING;
	error = ltsleep(&sysmon_power_event_queue_count,
	    PRIBIO|PCATCH, "smpower", 0, &sysmon_power_event_queue_slock);
	if (error) {
		simple_unlock(&sysmon_power_event_queue_slock);
		return (error);
	}
	goto again;
}

/*
 * sysmonpoll_power:
 *
 *	Poll the system monitor device.
 */
int
sysmonpoll_power(dev_t dev, int events, struct proc *p)
{
	int revents;

	revents = events & (POLLOUT | POLLWRNORM);

	/* Attempt to save some work. */
	if ((events & (POLLIN | POLLRDNORM)) == 0)
		return (revents);

	simple_lock(&sysmon_power_event_queue_slock);
	if (sysmon_power_event_queue_count)
		revents |= events & (POLLIN | POLLRDNORM);
	else
		selrecord(p, &sysmon_power_event_queue_selinfo);
	simple_unlock(&sysmon_power_event_queue_slock);

	return (revents);
}

static void
filt_sysmon_power_rdetach(struct knote *kn)
{

	simple_lock(&sysmon_power_event_queue_slock);
	SLIST_REMOVE(&sysmon_power_event_queue_selinfo.sel_klist,
	    kn, knote, kn_selnext);
	simple_unlock(&sysmon_power_event_queue_slock);
}

static int
filt_sysmon_power_read(struct knote *kn, long hint)
{

	simple_lock(&sysmon_power_event_queue_slock);
	kn->kn_data = sysmon_power_event_queue_count;
	simple_unlock(&sysmon_power_event_queue_slock);

	return (kn->kn_data > 0);
}

static const struct filterops sysmon_power_read_filtops =
    { 1, NULL, filt_sysmon_power_rdetach, filt_sysmon_power_read };

static const struct filterops sysmon_power_write_filtops =
    { 1, NULL, filt_sysmon_power_rdetach, filt_seltrue };

/*
 * sysmonkqfilter_power:
 *
 *	Kqueue filter for the system monitor device.
 */
int
sysmonkqfilter_power(dev_t dev, struct knote *kn)
{
	struct klist *klist;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sysmon_power_event_queue_selinfo.sel_klist;
		kn->kn_fop = &sysmon_power_read_filtops;
		break;

	case EVFILT_WRITE:
		klist = &sysmon_power_event_queue_selinfo.sel_klist;
		kn->kn_fop = &sysmon_power_write_filtops;
		break;

	default:
		return (1);
	}

	simple_lock(&sysmon_power_event_queue_slock);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	simple_unlock(&sysmon_power_event_queue_slock);

	return (0);
}

/*
 * sysmonioctl_power:
 *
 *	Perform a power managmenet control request.
 */
int
sysmonioctl_power(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	int error = 0;

	switch (cmd) {
	case POWER_IOC_GET_TYPE:
	    {
		struct power_type *power_type = (void *) data;

		strcpy(power_type->power_type, sysmon_power_type);
		break;
	    }
	default:
		error = ENOTTY;
	}

	return (error);
}

/*
 * sysmon_power_settype:
 *
 *	Sets the back-end power management type.  This information can
 *	be used by the power management daemon.
 */
void
sysmon_power_settype(const char *type)
{

	/*
	 * Don't bother locking this; it's going to be set
	 * during autoconfiguration, and then only read from
	 * then on.
	 */
	strcpy(sysmon_power_type, type);
}

/*
 * sysmon_pswitch_register:
 *
 *	Register a power switch device.
 */
int
sysmon_pswitch_register(struct sysmon_pswitch *smpsw)
{

	simple_lock(&sysmon_pswitch_list_slock);
	LIST_INSERT_HEAD(&sysmon_pswitch_list, smpsw, smpsw_list);
	simple_unlock(&sysmon_pswitch_list_slock);

	return (0);
}

/*
 * sysmon_pswitch_unregister:
 *
 *	Unregister a power switch device.
 */
void
sysmon_pswitch_unregister(struct sysmon_pswitch *smpsw)
{

	simple_lock(&sysmon_pswitch_list_slock);
	LIST_REMOVE(smpsw, smpsw_list);
	simple_unlock(&sysmon_pswitch_list_slock);
}

/*
 * sysmon_pswitch_event:
 *
 *	Register an event on a power switch device.
 */
void
sysmon_pswitch_event(struct sysmon_pswitch *smpsw, int event)
{

	/*
	 * If a power management daemon is connected, then simply
	 * deliver the event to them.  If not, we need to try to
	 * do something reasonable ourselves.
	 */
	simple_lock(&sysmon_power_event_queue_slock);
	if (sysmon_power_daemon != NULL) {
		power_event_t pev;
		int rv;

		pev.pev_type = POWER_EVENT_SWITCH_STATE_CHANGE;
		pev.pev_switch.psws_state = event;
		pev.pev_switch.psws_type = smpsw->smpsw_type;
		strcpy(pev.pev_switch.psws_name, smpsw->smpsw_name);

		rv = sysmon_queue_power_event(&pev);
		if (rv == 0) {
			simple_unlock(&sysmon_power_event_queue_slock);
			printf("%s: WARNING: state change event %d lost; "
			    "queue full\n", smpsw->smpsw_name,
			    pev.pev_type);
			return;
		} else {
			if (sysmon_power_event_queue_flags & PEVQ_F_WAITING) {
				sysmon_power_event_queue_flags &= ~PEVQ_F_WAITING;
				simple_unlock(&sysmon_power_event_queue_slock);
				wakeup(&sysmon_power_event_queue_count);
			} else {
				simple_unlock(&sysmon_power_event_queue_slock);
			}
			selnotify(&sysmon_power_event_queue_selinfo, 0);
			return;
		}
	}
	simple_unlock(&sysmon_power_event_queue_slock);

	switch (smpsw->smpsw_type) {
	case PSWITCH_TYPE_POWER:
		if (event != PSWITCH_EVENT_PRESSED) {
			/* just ignore it */
			return;
		}

		/*
		 * Attempt a somewhat graceful shutdown of the system,
		 * as if the user has issued a reboot(2) call with
		 * RB_POWERDOWN.
		 */
		printf("%s: power button pressed, shutting down!\n",
		    smpsw->smpsw_name);
		cpu_reboot(RB_POWERDOWN, NULL);
		break;

	case PSWITCH_TYPE_RESET:
		if (event != PSWITCH_EVENT_PRESSED) {
			/* just ignore it */
			return;
		}

		/*
		 * Attempt a somewhat graceful reboot of the system,
		 * as if the user had issued a reboot(2) call.
		 */
		printf("%s: reset button pressed, rebooting!\n",
		    smpsw->smpsw_name);
		cpu_reboot(0, NULL);
		break;

	case PSWITCH_TYPE_SLEEP:
		if (event != PSWITCH_EVENT_PRESSED) {
			/* just ignore it */
			return;
		}

		/*
		 * Try to enter a "sleep" state.
		 */
		/* XXX */
		printf("%s: sleep button pressed.\n", smpsw->smpsw_name);
		break;

	case PSWITCH_TYPE_LID:
		switch (event) {
		case PSWITCH_EVENT_PRESSED:
			/*
			 * Try to enter a "standby" state.
			 */
			/* XXX */
			printf("%s: lid closed.\n", smpsw->smpsw_name);
			break;

		case PSWITCH_EVENT_RELEASED:
			/*
			 * Come out of "standby" state.
			 */
			/* XXX */
			printf("%s: lid opened.\n", smpsw->smpsw_name);
			break;

		default:
			printf("%s: unknown lid switch event: %d\n",
			    smpsw->smpsw_name, event);
		}
		break;

	default:
		printf("%s: sysmon_pswitch_event can't handle me.\n",
		    smpsw->smpsw_name);
	}
}
