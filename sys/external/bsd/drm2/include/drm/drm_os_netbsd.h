/*	$NetBSD: drm_os_netbsd.h,v 1.14.4.1 2019/11/06 09:48:32 martin Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _DRM_DRM_OS_NETBSD_H_
#define _DRM_DRM_OS_NETBSD_H_

#if defined(_KERNEL_OPT)
#include "opt_drmkms.h"
#endif

#if defined(__i386__) || defined(__x86_64__)
#define	CONFIG_X86	1
#define	CONFIG_X86_PAT	1
#endif

/*
 * Nothing meaningfully depends on this; defining this avoids patching
 * away some conditionalization in drmP.h.
 */
#define	CONFIG_PCI	1

#ifdef notyet
#if defined(__i386__)
#include "pnpbios.h"
#endif

#if NPNPBIOS > 0
#define CONFIG_PNP
#endif
#endif

#if defined(__i386__) || defined(__x86_64__)
#if defined(_KERNEL_OPT)
#include "opt_mtrr.h"
#endif
#endif

#ifdef MTRR
#define	CONFIG_MTRR	1
#endif

#include <drm/drm_agp_netbsd.h>
#include <drm/drm_auth_netbsd.h>
#include <drm/drm_irq_netbsd.h>
#include <drm/drm_wait_netbsd.h>

#include <sys/vmem.h>

#endif  /* _DRM_DRM_OS_NETBSD_H_ */
