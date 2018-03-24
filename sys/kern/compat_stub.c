/* $NetBSD: compat_stub.c,v 1.1.2.6 2018/03/24 08:24:40 pgoyette Exp $	*/

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
