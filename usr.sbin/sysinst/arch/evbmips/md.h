/* $NetBSD: md.h,v 1.1.2.2 2014/08/10 07:00:25 tls Exp $ */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* md.h -- Machine specific definitions for the evbmips */

/* evbmips uses the mbr code. */
#include "mbr.h"

/* constants and defines */

/* Megs required for a full X installation. */
#define XNEEDMB			50

/*
 *  Default filesets to fetch and install during installation or upgrade.
 *  The standard sets are: base etc comp games man misc tests text
 */

#if defined(ARCH_mipsel) || defined(ARCH_mipseb)
#define SET_KERNEL_1_NAME	"kern-ALCHEMY"
#define SET_KERNEL_2_NAME	"kern-AR531X"
#define SET_KERNEL_3_NAME	"kern-DBAU1500"
#define SET_KERNEL_4_NAME	"kern-DBAU1550"
#define SET_KERNEL_5_NAME	"kern-MALTA"
#endif
#if defined(ARCH_mipsel)
#define SET_KERNEL_6_NAME	"kern-MTX-1"
#define SET_KERNEL_7_NAME	"kern-OMSAL400"
#endif

#if defined(ARCH_mips64eb) || defined(ARCH_mips64el)
#define SET_KERNEL_4_NAME	"kern-MALTA32"
#define SET_KERNEL_3_NAME	"kern-MALTA64"
#define SET_KERNEL_2_NAME	"kern-XLSATX32"
#define SET_KERNEL_1_NAME	"kern-XLSATX64"
#endif

#undef evbmips
#undef evbmips64

#define MD_SETS_SELECTED	SET_SYSTEM

/*
 * Machine-specific command to write a new label to a disk.
 * For example, bebox  uses "/sbin/disklabel -w -r", just like bebox
 * miniroot scripts, though this may leave a bogus incore label.
 * Sun ports should probably use  DISKLABEL_CMD "/sbin/disklabel -w"
 * to get incore  to ondisk inode translation for the Sun proms.
 * If not defined, we assume the port does not support disklabels and
 * hand-edited disklabel will NOT be written by MI code.
 */
#define DISKLABEL_CMD		"disklabel -w -r"
