/* $NetBSD: compat_stub.h,v 1.1.2.19 2018/09/16 00:40:27 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Goyette
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_COMPAT_STUB_H
#define _SYS_COMPAT_STUB_H

#include <sys/param.h>	/* for COHERENCY_UNIT, for __cacheline_aligned */
#include <sys/mutex.h>
#include <sys/localcount.h>
#include <sys/condvar.h>
#include <sys/pserialize.h>

/*
 * Macros for creating MP-safe vectored function calls, where
 * the function implementations are in modules which could be
 * unloaded.
 */

#define COMPAT_HOOK(hook,type,args)				\
extern struct __CONCAT(hook,_t) {				\
	kmutex_t		lock;				\
	kcondvar_t		cv;				\
	struct localcount	lc;				\
	pserialize_t		psz;				\
        bool			hooked;				\
	type			(*func)args;			\
} hook __cacheline_aligned;

#define COMPAT_HOOK2(hook,type1,args1,type2,args2)		\
extern struct __CONCAT(hook,_t) {				\
	kmutex_t		lock;				\
	kcondvar_t		cv;				\
	struct localcount	lc;				\
	pserialize_t		psz;				\
        bool			hooked;				\
	type1			(*func1)args1;			\
	type2			(*func2)args2;			\
} hook __cacheline_aligned;

#define COMPAT_SET_HOOK(hook, waitchan, f)			\
static void __CONCAT(hook,sethook)(void)			\
{								\
								\
	KASSERT(!hook.hooked);					\
								\
	hook.psz = pserialize_create();				\
	mutex_init(&hook.mtx, MUTEX_DEFAULT, IPL_NONE);		\
	cv_init(&hook.cv, waitchan);				\
	localcount_init(&hook.lc);				\
	hook.func = f;						\
								\
	/* Make sure it's initialized before anyone uses it */	\
	membar_producer();					\
								\
	/* Let them use it */					\
	hook.hooked = true;					\
}

#define COMPAT_SET_HOOK2(hook, waitchan, f1, f2)		\
static void __CONCAT(hook,sethook)(void)			\
{								\
								\
	KASSERT(!hook.hooked);					\
								\
	hook.psz = pserialize_create();				\
	mutex_init(&hook.mtx, MUTEX_DEFAULT, IPL_NONE);		\
	cv_init(&hook.cv, waitchan);				\
	localcount_init(&hook.lc);				\
	hook.func1 = f1;					\
	hook.func2 = f2;					\
								\
	/* Make sure it's initialized before anyone uses it */	\
	membar_producer();					\
								\
	/* Let them use it */					\
	hook.hooked = true;					\
}

#define COMPAT_UNSET_HOOK(hook)					\
static void __CONCAT(hook,unsethook)(void)			\
{								\
								\
	KASSERT(kernconfig_is_held());				\
	KASSERT(hook.hooked);					\
	KASSERT(hook.func);					\
								\
	/* Prevent new localcount_acquire calls.  */		\
	hook.hooked = false;					\
								\
	/* Wait for existing localcount_acquire calls to drain.  */ \
	pserialize_perform(hook.psz);				\
								\
	/* Wait for existing localcount references to drain.  */\
	localcount_drain(&hook.lc, &hook.cv, &hook.mtx);	\
								\
	localcount_fini(&hook.lc);				\
	cv_destroy(&hook.cv);					\
	mutex_destroy(&hook.mtx);				\
	pserialize_destroy(hook.psz);				\
}

#define COMPAT_UNSET_HOOK2(hook)				\
static void __CONCAT(hook,unsethook)(void)			\
{								\
								\
	KASSERT(kernconfig_is_held());				\
	KASSERT(hook.hooked);					\
	KASSERT(hook.func1);					\
	KASSERT(hook.func2);					\
								\
	/* Prevent new localcount_acquire calls.  */		\
	hook.hooked = false;					\
								\
	/* Wait for existing localcount_acquire calls to drain.  */ \
	pserialize_perform(hook.psz);				\
								\
	/* Wait for existing localcount references to drain.  */\
	localcount_drain(&hook.lc, &hook.cv, &hook.mtx);	\
								\
	localcount_fini(&hook.lc);				\
	cv_destroy(&hook.cv);					\
	mutex_destroy(&hook.mtx);				\
	pserialize_destroy(hook.psz);				\
}

#define COMPAT_CALL_HOOK(hook, which, args, no_hook)		\
type								\
__CONCAT(call_,hook)(args)					\
{								\
	bool hooked;						\
	int error, s;						\
								\
	s = pserialize_read_enter();				\
	hooked = hook.hooked;					\
	if (hooked) {						\
		membar_consumer():				\
		localcount_acquire(&hook.lc);			\
	}							\
	pserialize_read_exit();					\
								\
	if (hooked) {						\
		error = (*hook.which)(args);			\
		localcount_release(&hook.lc, &hook.cv,		\
		    &hook.mtx);					\
	} else {						\
		error = no_hook(args);				\
	}							\
	return error;						\
;

/*
 * Routine hooks for compat_50___sys_ntp_gettime
 */

struct ntptimeval;

extern void (*vec_ntp_gettime)(struct ntptimeval *);
extern int (*vec_ntp_timestatus)(void);

COMPAT_HOOK2(ntp_gettime_hooks, void, (struct ntptimeval *), int, (void))

/*
 * Routine vector for dev/ccd ioctl()
 */

extern int (*compat_ccd_ioctl_60)(dev_t, u_long, void *, int, struct lwp *,
    int (*f)(dev_t, u_long, void *, int, struct lwp *));

COMPAT_HOOK(ccd_ioctl_hook, int, (dev_t, u_long, void *, int, struct lwp *,
    int (*f)(dev_t, u_long, void *, int, struct lwp *)))

/*
 * Routine vector for dev/clockctl ioctl()
 */

extern int (*compat_clockctl_ioctl_50)(dev_t, u_long, void *, int,
    struct lwp *);

/*
 * if_sppp device compatability ioctl subroutine
 */
struct sppp;
extern int (*sppp_params50)(struct sppp *, u_long, void *);

/*
 * cryptodev compatability ioctl
 */
extern int (*ocryptof50_ioctl)(struct file *, u_long, void *);

/*
 * raidframe compatability
 */

struct RF_Raid_s;
struct RF_Config_s;
struct raid_softc;

extern int (*raidframe50_ioctl)(int, int, struct RF_Raid_s *, int, void *,
    struct RF_Config_s **);
extern int (*raidframe80_ioctl)(int, int, struct RF_Raid_s *, int, void *,
    struct RF_Config_s **);

/*
 * puffs compatability
 */

struct puffs_req;

extern int (*puffs50_compat_outgoing)(struct puffs_req *, struct puffs_req **,
    ssize_t *);
extern void (*puffs50_compat_incoming)(struct puffs_req *, struct puffs_req *);

/*
 * wsevents compatability
 */

struct wscons_event;
struct uio;

extern int (*wsevent_50_copyout_events)(const struct wscons_event *, int,
    struct uio *);

/*
 * sysmon_power compatability
 */

struct power_event;
struct sysmon_pswitch;

extern void (*compat_sysmon_power_40)(struct power_event *,
    struct sysmon_pswitch *, int);

/*
 * compat_30 indirect function pointer
 */
extern int (*compat_bio_30)(void *, u_long, void *,
    int(*)(void *, u_long, void *));

/*
 * vnd_30 ioctl compatability
 */
struct vattr;

extern int (*compat_vndioctl_30)(u_long, struct lwp *, void *, int,
    struct vattr *, int (*)(struct lwp *, void *, int, struct vattr *));

/*
 * usb devinfo compatability
 */

struct usbd_device;
struct usb_device_info;
struct usb_device_info_old;
struct usb_event;
struct usb_event_old;
struct uio;

extern int (*usbd30_fill_deviceinfo_old)(struct usbd_device *,
    struct usb_device_info_old *, int);

extern int (*usb30_copy_to_old)(struct usb_event *ue, struct usb_event_old *ueo,
    struct uio *uio);

extern void (*vec_usbd_devinfo_vp)(struct usbd_device *, char *, size_t, char *,
    size_t, int, int);

extern int (*vec_usbd_printBCD)(char *cp, size_t l, int bcd);

/*
 * ieee80211 ioctl compatability
 */
struct ieee80211_ostats;
struct ieee80211_stats; 

extern int (*ieee80211_get_ostats_20)(struct ieee80211_ostats *, 
    struct ieee80211_stats *);

extern int (*if43_20_cvtcmd)(int);

/*
 * rtsock 14 compatability
 */
struct ifnet;
struct rt_walkarg;
struct rt_addrinfo;

extern void (*rtsock14_oifmsg)(struct ifnet *);
extern int (*rtsock14_iflist)(struct ifnet *, struct rt_walkarg *,
    struct rt_addrinfo *, size_t);

/*
 * modctl handler for old style OSTAT
 */
struct iovec;

extern int (*compat_modstat_80)(int, struct iovec *, void *);
#endif	/* _SYS_COMPAT_STUB_H */

/*
 * mask for kern_sig_43's killpg
 */
extern int kern_sig_43_pgid_mask;
