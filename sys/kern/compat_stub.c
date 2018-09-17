/* $NetBSD: compat_stub.c,v 1.1.2.18 2018/09/17 11:04:31 pgoyette Exp $	*/

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
#include "usb.h"	/* XXX No other way to determine if USB is present */
#endif

#include <sys/systm.h>
#include <sys/compat_stub.h>

#ifdef NTP
#include <sys/timespec.h>
#include <sys/timex.h>
#endif

/*
 * Routine vectors for compat_50___sys_ntp_gettime
 *
 * MP-hooks not needed since the NTP code is not modular
 */

#ifdef NTP
void (*vec_ntp_gettime)(struct ntptimeval *) = ntp_gettime;
int (*vec_ntp_timestatus)(void) = ntp_timestatus;
#else
void (*vec_ntp_gettime)(struct ntptimeval *) = NULL;
int (*vec_ntp_timestatus)(void) = NULL;
#endif

#if NUSB > 0
#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#endif

/*
 * usb device_info compatability
 *
 * MP-hooks not needed since the USB code is not modular
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

struct puffs50_compat_hook_t puffs50_compat_hook;

int (*puffs50_compat_outgoing)(struct puffs_req *, struct puffs_req **,
    ssize_t *) = (void *)enosys;
void (*puffs50_compat_incoming)(struct puffs_req *, struct puffs_req *) =
    (void *)voidop;

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

/*
 * ieee80211 ioctl compatability
 */
struct ieee80211_ostats_hook_t ieee80211_ostats_hook;
struct ieee80211_get_ostats_20_hook_t ieee80211_get_ostats_20_hook;

int (*ieee80211_get_ostats_20)(struct ieee80211_ostats *,
    struct ieee80211_stats *) = (void *)enosys;

int (*if43_20_cvtcmd)(int) = (void *)enosys;

/*
 * rtsock 14 compatability
 */
struct rtsock14_hook_t rtsock14_hook;

/*
 * modctl handler for old style OSTAT
 */
struct compat_modstat_80_hook_t compat_modstat_80_hook;

/*
 * mask for kern_sig_43's killpg (updated by compat_09)
 */
int kern_sig_43_pgid_mask = ~0;
