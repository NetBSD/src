/*	$NetBSD: omap2_intr.h,v 1.7 2012/07/14 07:42:57 matt Exp $ */

/*
 * Define the SDP2430 specific information and then include the generic OMAP
 * interrupt header.
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_OMAP_OMAP2_INTR_H_
#define _ARM_OMAP_OMAP2_INTR_H_

#ifdef _KERNEL_OPT
#include "opt_omap.h"
#endif

#ifndef _LOCORE
#include <arm/armreg.h>
#include <arm/cpufunc.h>
#endif

#if defined(OMAP2) || defined(OMAP3)
#include <arm/omap/omap2430_intr.h>
#endif
#if defined(OMAP_4430)
#include <arm/omap/omap4430_intr.h>
#endif

#ifndef _LOCORE

#include <arm/pic/picvar.h>

#endif /* ! _LOCORE */

#endif /* _ARM_OMAP_OMAP2_INTR_H_ */
