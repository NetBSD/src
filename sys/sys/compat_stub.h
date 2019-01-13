/* $NetBSD: compat_stub.h,v 1.1.2.46 2019/01/13 10:49:51 pgoyette Exp $	*/

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
#include <sys/param.h>

/*
 * Routine hooks for compat_50___sys_ntp_gettime
 *
 * MP-hooks not needed since the NTP code is not modular
 */

struct ntptimeval;
struct timex;

extern void (*vec_ntp_gettime)(struct ntptimeval *);
extern int (*vec_ntp_timestatus)(void);
extern void (*vec_ntp_adjtime1)(struct timex *);

/*
MODULE_HOOK(ntp_gettime_hook, (struct ntptimeval *));
MODULE_HOOK(ntp_timestatus_hook, (void);
MODULE_HOOK(ntp_adjtime1_hook, (struct timex *));
*/

/*
 * usb devinfo compatability
 */

struct usbd_device;
struct usb_device_info_old;
struct usb_event;
struct usb_event_old;
struct uio;
MODULE_HOOK(usb_subr_30_fill_hook,
    (struct usbd_device *, struct usb_device_info_old *, int,
      void (*)(struct usbd_device *, char *, size_t, char *, size_t, int, int),
      int (*)(char *, size_t, int)));
MODULE_HOOK(usb_subr_30_copy_hook,
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

struct fcrypt;
struct session_op;
struct csession;
struct crypt_op;
struct crypt_n_op;
struct kmutex_t;
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
MODULE_HOOK(puffs_50_out_hook,
    (struct puffs_req *, struct puffs_req **, ssize_t *));	/* outgoing */
MODULE_HOOK(puffs_50_in_hook,
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
 */
struct ieee80211_ostats;
struct ieee80211_stats; 

MODULE_HOOK(ieee80211_ostats_hook, (struct ieee80211_ostats *,
    struct ieee80211_stats *));

/*
 * if_43 compatability
 */
struct socket;

MODULE_HOOK(if_43_cvtcmd_hook, (u_long *, u_long));
MODULE_HOOK(if_43_ifioctl_hook,
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
MODULE_HOOK(rtsock_14_oifmsg_hook, (struct ifnet *));
MODULE_HOOK(rtsock_14_iflist_hook,
    (struct ifnet *, struct rt_walkarg *, struct rt_addrinfo *, size_t));

/*
 * Hooks for rtsock_50
 */

MODULE_HOOK(rtsock_50_hook,
    (struct ifnet *, struct rt_walkarg *, struct rt_addrinfo *, size_t len));

/*
 * Hooks for rtsock_70
 */
struct ifaddr;
MODULE_HOOK(rtsock_70_newaddr_hook, (int, struct ifaddr *));
MODULE_HOOK(rtsock_70_iflist_hook,
    (struct rt_walkarg *, struct ifaddr *, struct rt_addrinfo *));

/*
 * modctl handler for old style OSTAT
 */
struct iovec;
MODULE_HOOK(compat_modstat_80_hook, (int, struct iovec *, void *));

/*
 * mask for kern_sig_43's killpg
 */
extern int kern_sig_43_pgid_mask;

/*
 * Hooks for kern_proc.c for netbsd32 compat
 */
struct ps_strings;
MODULE_HOOK(kern_proc_32_copyin_hook, (struct proc *, struct ps_strings *));
MODULE_HOOK(kern_proc_32_base_hook, (char **, size_t, vaddr_t *));

/*
 * Hook to allow sparc fpu code to see if a process is using sunos
 * emulation, and select proper fup codes
 */
struct emul;
MODULE_HOOK(get_emul_sunos_hook, (const struct emul **));

/*
 * Hooks for rnd_ioctl_50
 */
MODULE_HOOK(rnd_ioctl_50_hook, (struct file *, u_long, void *));
MODULE_HOOK(rnd_ioctl_50_32_hook, (struct file *, u_long, void *));

/*
 * Hooks for compat_60 ttioctl and ptmioctl
 */
MODULE_HOOK(compat_60_ttioctl_hook, (dev_t, u_long, void *, int, struct lwp *));
MODULE_HOOK(compat_60_ptmioctl_hook, (dev_t, u_long, void *, int, struct lwp *));

/*
 * Hook for compat_10 openat
 */
struct pathbuf;
MODULE_HOOK(compat_10_openat_hook, (struct pathbuf **));

/*
 * Hook for compat_70_unp_addsockcred
 */
struct mbuf;
MODULE_HOOK(compat_70_unp_hook, (struct mbuf **, struct lwp *, struct mbuf *));

/*
 * Hook for sysvipc50 sysctl
 */
#include <sys/sysctl.h>
MODULE_HOOK(sysvipc50_sysctl_hook, (SYSCTLFN_PROTO));

#endif	/* _SYS_COMPAT_STUB_H */
