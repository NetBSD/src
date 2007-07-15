/*	$NetBSD: sysmonvar.h,v 1.13.2.2 2007/07/15 13:21:46 ad Exp $	*/

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

#ifndef _LKM
#include "opt_compat_netbsd.h"
#endif
#include <sys/envsys.h>
#include <sys/wdog.h>
#include <sys/power.h>
#include <sys/queue.h>

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

struct sysmon_envsys {
	const char *sme_name;			/* envsys device name */
	uint32_t sme_nsensors;			/* sensor count, from driver */
	int sme_flags;				/* additional flags */
#define SME_FLAG_BUSY 		0x00000001 	/* sme is busy */
#define SME_FLAG_WANTED 	0x00000002 	/* someone waiting for this */
#define SME_DISABLE_GTREDATA	0x00000004	/* disable sme_gtredata */

	envsys_data_t *sme_sensor_data;		/* pointer to device data */

	/* linked list for the sysmon envsys devices */
	LIST_ENTRY(sysmon_envsys) sme_list;

	void *sme_cookie;			/* for ENVSYS back-end */

	/* Function callback to recieve data from device */
	int (*sme_gtredata)(struct sysmon_envsys *, envsys_data_t *);

#ifdef COMPAT_40
	u_int sme_fsensor;		/* sensor index base, from sysmon */
#define SME_SENSOR_IDX(sme, idx)	((idx) - (sme)->sme_fsensor)
#endif
};

int	sysmonopen_envsys(dev_t, int, int, struct lwp *);
int	sysmonclose_envsys(dev_t, int, int, struct lwp *);
int	sysmonioctl_envsys(dev_t, u_long, void *, int, struct lwp *);

int	sysmon_envsys_register(struct sysmon_envsys *);
void	sysmon_envsys_unregister(struct sysmon_envsys *);
struct	sysmon_envsys *sysmon_envsys_find(const char *);

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
void    sysmon_wdog_unregister(struct sysmon_wdog *);

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
