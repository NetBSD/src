/* $NetBSD: opt_ah.h,v 1.1.10.2 2009/08/07 06:43:50 snj Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
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
#ifndef OPT_AH_H
#define OPT_AH_H

#include "opt_athhal.h"

#ifdef ATHHAL_ASSERT
#define AH_ASSERT		1
#endif

#ifdef ATHHAL_DEBUG
#define AH_DEBUG		1
#endif

#ifdef ATHHAL_DEBUG_ALQ
#define AH_DEBUG_ALQ		1
#endif

#ifdef ATHHAL_AR5210
#define AH_SUPPORT_5210		1
#endif

#ifdef ATHHAL_AR5211
#define AH_SUPPORT_5211		1
#endif

#ifdef ATHHAL_AR5212
#define AH_SUPPORT_5212		1
#endif

#ifdef ATHHAL_AR5311
#define AH_SUPPORT_5311		1
#endif

#ifdef ATHHAL_AR5312
#define AH_SUPPORT_AR5312	1
#endif

#ifdef ATHHAL_AR2316
#define AH_SUPPORT_2316		1
#endif

#ifdef ATHHAL_AR2317
#define AH_SUPPORT_2317		1
#endif

#ifdef ATHHAL_AR5416
#define AH_SUPPORT_AR5416	1
#endif

#ifdef ATHHAL_AR9280
#define AH_SUPPORT_AR9280	1
#endif

#if defined(ATHHAL_RF2316) || \
	defined(ATHHAL_RF2317) || \
	defined(ATHHAL_RF2413) || \
	defined(ATHHAL_RF2425) || \
	defined(ATHHAL_RF5111) || \
	defined(ATHHAL_RF5112) || \
	defined(ATHHAL_RF5413)
#define AH_HAS_RF
#endif

#endif /* OPT_AH_H */
