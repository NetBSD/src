/* $NetBSD: compat_stub.h,v 1.1.2.32 2018/09/21 02:56:22 pgoyette Exp $	*/

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

#include <sys/module_hook.h>

/*
 * Routine hooks for compat_50___sys_ntp_gettime
 *
 * MP-hooks not needed since the NTP code is not modular
 */

struct ntptimeval;

extern void (*vec_ntp_gettime)(struct ntptimeval *);
extern int (*vec_ntp_timestatus)(void);

MODULE_HOOK2(ntp_gettime_hooks, (struct ntptimeval *), (void))

/*
 * usb devinfo compatability
 */

struct usbd_device;
struct usb_device_info_old;
struct usb_event;
struct usb_event_old;
struct uio;
MODULE_HOOK2(usb_subr_30_hook,
    (struct usbd_device *, struct usb_device_info_old *, int,
      void (*)(struct usbd_device *, char *, size_t, char *, size_t, int, int),
      int (*)(char *, size_t, int)),
    (struct usb_event *, struct usb_event_old *, struct uio *));

/*
 * Routine vector for dev/ccd ioctl()
 */

MODULE_HOOK(ccd_ioctl_60_hook, (dev_t, u_long, void *, int, struct lwp *,
    int (*f)(dev_t, u_long, void *, int, struct lwp *)))

/*
 * Routine vector for dev/clockctl ioctl()
 */

MODULE_HOOK(clockctl_ioctl_50_hook, (dev_t, u_long, void *, int, struct lwp *));

/*
 * if_sppp device compatability ioctl subroutine
 */

struct sppp;
MODULE_HOOK(sppp_params_50_hook, (struct sppp *, u_long, void *));

/*
 * cryptodev compatability ioctl
 */

MODULE_HOOK(ocryptof_50_hook, (struct file *, u_long, void *));

/*
 * raidframe compatability
 */

struct RF_Config_s;
struct RF_Raid_s;
MODULE_HOOK(raidframe50_ioctl_hook, (int, int, struct RF_Raid_s *, int, void *,
    struct RF_Config_s **));
MODULE_HOOK(raidframe80_ioctl_hook, (int, int, struct RF_Raid_s *, int, void *,
    struct RF_Config_s **));

/*
 * puffs compatability
 */

struct puffs_req;
MODULE_HOOK2(puffs50_compat_hook,
    (struct puffs_req *, struct puffs_req **, ssize_t *),	/* outgoing */
    (struct puffs_req *, struct puffs_req *));			/* incoming */

/*
 * wsevents compatability
 */

struct wscons_event;
struct uio;
MODULE_HOOK(wsevent_50_copyout_events_hook,
    (const struct wscons_event *, int, struct uio *));

/*
 * sysmon_power compatability
 */

struct power_event;
struct sysmon_pswitch;
MODULE_HOOK(compat_sysmon_power_40_hook, (struct power_event *,
    struct sysmon_pswitch *, int));

/*
 * compat_bio indirect function pointer
 */

MODULE_HOOK(compat_bio_30_hook, (void *, u_long, void *,
    int(*)(void *, u_long, void *)));

/*
 * vnd_30 ioctl compatability
 */
struct vattr;
MODULE_HOOK(compat_vndioctl_30_hook, (u_long, struct lwp *, void *, int,
    struct vattr *, int (*)(struct lwp *, void *, int, struct vattr *)));

/*
 * vnd_50 ioctl compatability
 */
struct vattr;
MODULE_HOOK(compat_vndioctl_50_hook, (u_long, struct lwp *, void *, int,
    struct vattr *, int (*)(struct lwp *, void *, int, struct vattr *)));

/*
 * ieee80211 ioctl compatability
 * XXX need to review this
 */
struct ieee80211_ostats;
struct ieee80211_stats; 

MODULE_HOOK(ieee80211_ostats_hook, (struct ieee80211_ostats *,
    struct ieee80211_stats *));
MODULE_HOOK(ieee80211_get_ostats_20_hook, (int));

/* XXX PRG*/extern int (*ieee80211_get_ostats_20)(struct ieee80211_ostats *, 
    struct ieee80211_stats *);

/*
 * if_43 compatability
 */
struct socket;

MODULE_HOOK2(if_43_hook,
    (u_long *, u_long),
    (struct socket *, u_long, u_long, void *, struct lwp *));

/*
 * if43_20 compatability
 */
MODULE_HOOK(if43_20_hook, (u_long cmd));

/*
 * uipc_syscalls_40 compatability
 */

MODULE_HOOK(uipc_syscalls_40_hook, (u_long cmd, void *data));

/*
 * uipc_syscalls_50 compatability
 */

MODULE_HOOK(uipc_syscalls_50_hook, (struct lwp *, u_long, void *));

/*
 * rtsock 14 compatability
 */
struct ifnet;
struct rt_walkarg;
struct rt_addrinfo;
MODULE_HOOK2(rtsock14_hook, (struct ifnet *),
    (struct ifnet *, struct rt_walkarg *, struct rt_addrinfo *, size_t));

/*
 * modctl handler for old style OSTAT
 */
struct iovec;
MODULE_HOOK(compat_modstat_80_hook, (int, struct iovec *, void *));

/*
 * mask for kern_sig_43's killpg
 */
extern int kern_sig_43_pgid_mask;

#endif	/* _SYS_COMPAT_STUB_H */
