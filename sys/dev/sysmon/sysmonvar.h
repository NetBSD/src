/*	$NetBSD: sysmonvar.h,v 1.22.2.1 2007/12/26 19:47:23 ad Exp $	*/

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

#ifndef _DEV_SYSMON_SYSMONVAR_H_
#define	_DEV_SYSMON_SYSMONVAR_H_

#include <sys/envsys.h>
#include <sys/wdog.h>
#include <sys/power.h>
#include <sys/queue.h>
#include <sys/callout.h>
#include <sys/workqueue.h>

struct lwp;
struct proc;
struct knote;
struct uio;

#define	SYSMON_MINOR_ENVSYS	0
#define	SYSMON_MINOR_WDOG	1
#define	SYSMON_MINOR_POWER	2

/*****************************************************************************
 * Environmental sensor support
 *****************************************************************************/

struct sme_event;
struct sme_sensor_names;

struct sysmon_envsys {
	const char *sme_name;		/* envsys device name */
	u_int sme_nsensors;		/* sensors count, from driver */
	u_int sme_fsensor;		/* sensor index base, from sysmon */
#define SME_SENSOR_IDX(sme, idx) 	((idx) - (sme)->sme_fsensor)
	int sme_class;			/* class of device */
#define SME_CLASS_BATTERY	1		/* device is a battery */
#define SME_CLASS_ACADAPTER	2		/* device is an AC adapter */
	int sme_flags;			/* additional flags */
#define SME_FLAG_BUSY 		0x00000001 	/* device busy */
#define SME_DISABLE_REFRESH	0x00000002	/* disable sme_refresh */
#define SME_CALLOUT_INITIALIZED	0x00000004	/* callout was initialized */

	void *sme_cookie;		/* for ENVSYS back-end */

	/* 
	 * Function callback to receive data from device.
	 */
	void (*sme_refresh)(struct sysmon_envsys *, envsys_data_t *);

	struct workqueue *sme_wq;	/* the workqueue for the events */
	struct callout sme_callout;	/* for the events */
	uint64_t sme_events_timeout;	/* the timeout used in the callout */

	/* 
	 * linked list for the sysmon envsys devices.
	 */
	LIST_ENTRY(sysmon_envsys) sme_list;

	/* 
	 * linked list for the events that a device maintains.
	 */
	LIST_HEAD(, sme_event) sme_events_list;

	/*
	 * tailq for the sensors that a device maintains.
	 */
	TAILQ_HEAD(, envsys_data) sme_sensors_list;
};

int	sysmonopen_envsys(dev_t, int, int, struct lwp *);
int	sysmonclose_envsys(dev_t, int, int, struct lwp *);
int	sysmonioctl_envsys(dev_t, u_long, void *, int, struct lwp *);

struct sysmon_envsys 	*sysmon_envsys_create(void);
void 			sysmon_envsys_destroy(struct sysmon_envsys *);

int	sysmon_envsys_register(struct sysmon_envsys *);
void	sysmon_envsys_unregister(struct sysmon_envsys *);

int	sysmon_envsys_sensor_attach(struct sysmon_envsys *, envsys_data_t *);
int	sysmon_envsys_sensor_detach(struct sysmon_envsys *, envsys_data_t *);

void	sysmon_envsys_init(void);

/*****************************************************************************
 * Watchdog timer support
 *****************************************************************************/

struct sysmon_wdog {
	const char *smw_name;		/* watchdog device name */

	LIST_ENTRY(sysmon_wdog) smw_list;

	void *smw_cookie;		/* for watchdog back-end */
	int (*smw_setmode)(struct sysmon_wdog *);
	int (*smw_tickle)(struct sysmon_wdog *);
	u_int smw_period;		/* timer period (in seconds) */
	int smw_mode;			/* timer mode */
	u_int smw_refcnt;		/* references */
	pid_t smw_tickler;		/* last process to tickle */
};

int	sysmonopen_wdog(dev_t, int, int, struct lwp *);
int	sysmonclose_wdog(dev_t, int, int, struct lwp *);
int	sysmonioctl_wdog(dev_t, u_long, void *, int, struct lwp *);

int     sysmon_wdog_register(struct sysmon_wdog *);
int     sysmon_wdog_unregister(struct sysmon_wdog *);

void	sysmon_wdog_init(void);

/*****************************************************************************
 * Power management support
 *****************************************************************************/

struct sysmon_pswitch {
	const char *smpsw_name;		/* power switch name */
	int smpsw_type;			/* power switch type */

	LIST_ENTRY(sysmon_pswitch) smpsw_list;
};

int	sysmonopen_power(dev_t, int, int, struct lwp *);
int	sysmonclose_power(dev_t, int, int, struct lwp *);
int	sysmonread_power(dev_t, struct uio *, int);
int	sysmonpoll_power(dev_t, int, struct lwp *);
int	sysmonkqfilter_power(dev_t, struct knote *);
int	sysmonioctl_power(dev_t, u_long, void *, int, struct lwp *);

void	sysmon_power_settype(const char *);

int	sysmon_pswitch_register(struct sysmon_pswitch *);
void	sysmon_pswitch_unregister(struct sysmon_pswitch *);

void	sysmon_pswitch_event(struct sysmon_pswitch *, int);
void	sysmon_penvsys_event(struct penvsys_state *, int);

void	sysmon_power_init(void);

#endif /* _DEV_SYSMON_SYSMONVAR_H_ */
