/* $NetBSD: compat_stub.c,v 1.1.2.15 2018/04/17 07:24:55 pgoyette Exp $	*/

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
#include "usb.h"
#endif

#include <sys/systm.h>
#include <sys/compat_stub.h>

/*
 * Routine vectors for compat_50___sys_ntp_gettime
 */

#ifdef NTP
void (*vec_ntp_gettime)(struct ntptimeval *) = ntp_gettime;
int (*vec_ntp_timestatus)(void) = ntp_timestatus;
#else
void (*vec_ntp_gettime)(struct ntptimeval *) = NULL;
int (*vec_ntp_timestatus)(void) = NULL;
#endif

/*
 * ccd device compatability ioctl
 */
int (*compat_ccd_ioctl_60)(dev_t, u_long, void *, int, struct lwp *,
    int (*f)(dev_t, u_long, void *, int, struct lwp *)) = (void *)enosys;

/*
 * clockctl device compatability ioctl
 */
int (*compat_clockctl_ioctl_50)(dev_t, u_long, void *, int, struct lwp *) =
    (void *)enosys;

/*
 * if_sppp device compatability ioctl subroutine
 */
int (*sppp_params50)(struct sppp *, u_long, void *) = (void *)enosys;

/*
 * cryptodev compatability ioctl
 */
int (*ocryptof50_ioctl)(struct file *, u_long, void *) = (void *)enosys;

/*
 * raidframe compatability
 */
int (*raidframe50_ioctl)(int, int, struct RF_Raid_s *, int, void *,
    struct RF_Config_s **) = (void *)enosys;
int (*raidframe80_ioctl)(int, int, struct RF_Raid_s *, int, void *,
    struct RF_Config_s **) = (void *)enosys;

/*
 * puffs compatability
 */
int (*puffs50_compat_outgoing)(struct puffs_req *, struct puffs_req **,
    ssize_t *) = (void *)enosys;
void (*puffs50_compat_incoming)(struct puffs_req *, struct puffs_req *) =
    (void *)voidop;

/*
 * wsevents compatability
 */
int (*wsevent_50_copyout_events)(const struct wscons_event *, int,
    struct uio *) = (void *)enosys;

/*
 * sysmon_power compatability
 */
void (*compat_sysmon_power_40)(struct power_event *, struct sysmon_pswitch *,
    int) = (void *)voidop;

/*
 * bio compatability
 */

int (*compat_bio_30)(void *, u_long, void *, int(*)(void *, u_long, void *)) =
    (void *)enosys;

/*
 * vnd ioctl compatability
 */
int (*compat_vndioctl_30)(u_long, struct lwp *, void *, int, struct vattr *,
    int (*get)(struct lwp *, void *, int, struct vattr *)) = (void *)enosys;

/*
 * usb device_info compatability
 */
int (*usbd30_fill_deviceinfo_old)(struct usbd_device *,
    struct usb_device_info_old *, int) = (void *)enosys;

int (*usb30_copy_to_old)(struct usb_event *, struct usb_event_old *,
    struct uio *) = (void *)enosys;

#if NUSB > 0
void (*vec_usbd_devinfo_vp)(struct usbd_device *, char *, size_t, char *,
    size_t, int, int) = usbd_devinfo_vp;
int (*vec_usbd_printBCD)(char *cp, size_t l, int bcd) = usbd_printBCD;

#else
void (*vec_usbd_devinfo_vp)(struct usbd_device *, char *, size_t, char *,
    size_t, int, int) = NULL;
int (*vec_usbd_printBCD)(char *cp, size_t l, int bcd) = NULL;
#endif

/*
 * ieee80211 ioctl compatability
 */
int (*ieee80211_get_ostats_20)(struct ieee80211_ostats *,
    struct ieee80211_stats *) = (void *)enosys;

int (*if43_20_cvtcmd)(int) = (void *)enosys;

/*
 * rtsock 14 compatability
 */
void (*rtsock14_oifmsg)(struct ifnet *) = (void *)voidop;
int (*rtsock14_iflist)(struct ifnet *, struct rt_walkarg *,
    struct rt_addrinfo *, size_t) = (void *)enosys;

/*
 * modctl handler for old style OSTAT
 */
int (*compat_modstat_80)(int, struct iovec *, void *) = (void *)enosys;

/*
 * mask for kern_sig_43's killpg (updated by compat_09)
 */
int kern_sig_43_pgid_mask = ~0;
