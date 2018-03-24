/* $NetBSD: compat_stub.h,v 1.1.2.7 2018/03/24 23:52:19 pgoyette Exp $	*/

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

/*
 * Routine vectors for compat_50___sys_ntp_gettime
 */

#include <sys/timespec.h>
#include <sys/timex.h>

extern void (*vec_ntp_gettime)(struct ntptimeval *);
extern int (*vec_ntp_timestatus)(void);

/*
 * Routine vector for dev/ccd ioctl()
 */

extern int (*compat_ccd_ioctl_60)(dev_t, u_long, void *, int, struct lwp *,
    int (*f)(dev_t, u_long, void *, int, struct lwp *));

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

#endif	/* _SYS_COMPAT_STUB_H */
