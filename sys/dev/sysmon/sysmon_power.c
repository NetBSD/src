/*	$NetBSD: sysmon_power.c,v 1.1 2003/04/17 01:02:21 thorpej Exp $	*/

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

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <dev/sysmon/sysmonvar.h>

static LIST_HEAD(, sysmon_pswitch) sysmon_pswitch_list =
    LIST_HEAD_INITIALIZER(sysmon_pswitch_list);
static struct simplelock sysmon_pswitch_list_slock =
    SIMPLELOCK_INITIALIZER;

static struct proc *sysmon_power_daemon;

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
	if (sysmon_power_daemon != NULL) {
		/* XXX */
		return;
	}

	switch (smpsw->smpsw_type) {
	case SMPSW_TYPE_POWER:
		if (event != SMPSW_EVENT_PRESSED) {
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

	case SMPSW_TYPE_SLEEP:
		if (event != SMPSW_EVENT_PRESSED) {
			/* just ignore it */
			return;
		}

		/*
		 * Try to enter a "sleep" state.
		 */
		/* XXX */
		printf("%s: sleep button pressed.\n", smpsw->smpsw_name);
		break;

	case SMPSW_TYPE_LID:
		switch (event) {
		case SMPSW_EVENT_PRESSED:
			/*
			 * Try to enter a "standby" state.
			 */
			/* XXX */
			printf("%s: lid closed.\n", smpsw->smpsw_name);
			break;

		case SMPSW_EVENT_RELEASED:
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
