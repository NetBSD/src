/* $NetBSD: compat_stub.c,v 1.1.2.41 2019/01/24 04:08:09 pgoyette Exp $	*/

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

#include <sys/cdefs.h>

#ifdef _KERNEL_OPT
#include "opt_ntp.h"
#include "opt_sctp.h"
#endif

#include <sys/systm.h>
#include <sys/compat_stub.h>

#ifdef NTP
#include <sys/timespec.h>
#include <sys/timex.h>
#endif

#ifdef SCTP
#include <netinet/sctp_asconf.h>
#endif

/*
 * Routine vectors for compat_50___sys_ntp_gettime
 *
 * MP-hooks not needed since the NTP code is not modular
 */

#ifdef NTP
void (*vec_ntp_gettime)(struct ntptimeval *) = ntp_gettime;
int (*vec_ntp_timestatus)(void) = ntp_timestatus;
void (*vec_ntp_adjtime1)(struct timex *) = ntp_adjtime1;
#else
void (*vec_ntp_gettime)(struct ntptimeval *) = NULL;
int (*vec_ntp_timestatus)(void) = NULL;
void (*vec_ntp_adjtime1)(struct timex *) = NULL;
#endif

/*
 * Routine vectors for sctp (called from within rtsock)
 *
 * MP-hooks not needed since the SCTP code is not modular
 */

#ifdef SCTP
void (*vec_sctp_add_ip_address)(struct ifaddr *) = sctp_add_ip_address;
void (*vec_sctp_delete_ip_address)(struct ifaddr *) = sctp_delete_ip_address;
#else
void (*vec_sctp_add_ip_address)(struct ifaddr *) = NULL;
void (*vec_sctp_delete_ip_address)(struct ifaddr *) = NULL;
#endif 


/*
 * usb device_info compatability
 */
struct usb_subr_30_fill_hook_t usb_subr_30_fill_hook;
struct usb_subr_30_copy_hook_t usb_subr_30_copy_hook;

/*
 * ccd device compatability ioctl
 */

struct ccd_ioctl_60_hook_t ccd_ioctl_60_hook;

/*
 * clockctl device compatability ioctl
 */

struct clockctl_ioctl_50_hook_t clockctl_ioctl_50_hook;

/*
 * if_sppp device compatability ioctl subroutine
 */

struct sppp_params_50_hook_t sppp_params_50_hook;

/*
 * cryptodev compatability ioctl
 */

struct ocryptof_50_hook_t ocryptof_50_hook;

/*
 * raidframe compatability
 */
struct raidframe50_ioctl_hook_t raidframe50_ioctl_hook;
struct raidframe80_ioctl_hook_t raidframe80_ioctl_hook;

/*
 * puffs compatability
 */

struct puffs_50_out_hook_t puffs_50_out_hook;
struct puffs_50_in_hook_t puffs_50_in_hook;

/* XXX
int (*puffs50_compat_outgoing)(struct puffs_req *, struct puffs_req **,
    ssize_t *) = (void *)enosys;
void (*puffs50_compat_incoming)(struct puffs_req *, struct puffs_req *) =
    (void *)voidop;
   XXX */

/*
 * wsevents compatability
 */
struct wsevent_50_copyout_events_hook_t wsevent_50_copyout_events_hook;

/*
 * sysmon_power compatability
 */
struct compat_sysmon_power_40_hook_t compat_sysmon_power_40_hook;

/*
 * compat_bio compatability
 */
struct compat_bio_30_hook_t compat_bio_30_hook;

/*
 * vnd ioctl compatability
 */
struct compat_vndioctl_30_hook_t compat_vndioctl_30_hook;
struct compat_vndioctl_50_hook_t compat_vndioctl_50_hook;

/*
 * ieee80211 ioctl compatability
 */
struct ieee80211_ostats_hook_t ieee80211_ostats_hook;

/*
 * if_43 compatability
 */
struct if_43_cvtcmd_hook_t if_43_cvtcmd_hook;
struct if_43_ifioctl_hook_t if_43_ifioctl_hook;

/*
 * if43_20 compatability
 */
struct if43_20_hook_t if43_20_hook;

/*
 * upic_syscalls_40 compatability
 */
struct uipc_syscalls_40_hook_t uipc_syscalls_40_hook;

/*
 * upic_syscalls_50 compatability
 */
struct uipc_syscalls_50_hook_t uipc_syscalls_50_hook;

/*
 * rtsock 14 compatability
 */
struct rtsock_14_oifmsg_hook_t rtsock_14_oifmsg_hook;
struct rtsock_14_iflist_hook_t rtsock_14_iflist_hook;

/*
 * rtsock 50 compatability
 */
struct rtsock_50_iflist_hook_t rtsock_50_iflist_hook;
struct rtsock_50_oifmsg_hook_t rtsock_50_oifmsg_hook;
struct rtsock_50_rt_missmsg_hook_t rtsock_50_rt_missmsg_hook;
struct rtsock_50_rt_ifmsg_hook_t rtsock_50_rt_ifmsg_hook;
struct rtsock_50_rt_ifannouncemsg_hook_t rtsock_50_rt_ifannouncemsg_hook;
struct rtsock_50_rt_newaddrmsg_hook_t rtsock_50_rt_newaddrmsg_hook;
struct rtsock_50_rt_ieee80211msg_hook_t rtsock_50_rt_ieee80211msg_hook;

/*
 * rtsock 70 compatability
 */
struct rtsock_70_newaddr_hook_t rtsock_70_newaddr_hook;
struct rtsock_70_iflist_hook_t rtsock_70_iflist_hook;

/*
 * modctl handler for old style OSTAT
 */
struct compat_modstat_80_hook_t compat_modstat_80_hook;

/*
 * mask for kern_sig_43's killpg (updated by compat_09)
 */
int kern_sig_43_pgid_mask = ~0;

/*
 * hook for kern_proc_32
 */
struct kern_proc_32_copyin_hook_t kern_proc_32_copyin_hook;
struct kern_proc_32_base_hook_t kern_proc_32_base_hook;

/*
 * Hook for sparc fpu code to check if a process is running 
 * under sunos emulation
 */
struct get_emul_sunos_hook_t get_emul_sunos_hook;

/*
 * Hook for rnd_ioctl_50 callbacks
 */
struct rnd_ioctl_50_hook_t rnd_ioctl_50_hook;
struct rnd_ioctl_50_32_hook_t rnd_ioctl_50_32_hook;

/* 
 * Hooks for compat_60 ttioctl and ptmioctl
 */
struct compat_60_ttioctl_hook_t compat_60_ttioctl_hook;
struct compat_60_ptmioctl_hook_t compat_60_ptmioctl_hook;

/*
 * Hook for compat_10 openat
 */
struct compat_10_openat_hook_t compat_10_openat_hook;

/*
 * Hook for compat_70_unp_addsockcred
 */
struct compat_70_unp_hook_t compat_70_unp_hook;

/*
 * Hook for sysvipc50_sysctl
 */
struct sysvipc50_sysctl_hook_t sysvipc50_sysctl_hook;
