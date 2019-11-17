/*	$NetBSD: md.h,v 1.1.30.1 2019/11/17 07:04:38 martin Exp $	*/

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

/* md.h -- Machine specific definitions for the landisk */

/* landisk uses the mbr code. */
#include "mbr.h"

/* constants and defines */

/* Megs required for a full X installation. */
#define XNEEDMB			340

/* have support for booting from UFS2 */
#define HAVE_UFS2_BOOT

/* have file system specific primary boot loader */
#define HAVE_BOOTXX_xFS
#define BOOTXXDIR	"/usr/mdec"
#define BOOTXX_FFSV1	"bootxx_ffsv1"
#define BOOTXX_FFSV2	"bootxx_ffsv2"

/*
 *  Default filesets to fetch and install during installation or upgrade.
 *  The standard sets are: base etc comp games man misc rescue tests text
 */
#define SET_KERNEL_1_NAME	"kern-GENERIC"

#define MD_SETS_SELECTED	SET_KERNEL_1, SET_SYSTEM, SET_X11_NOSERVERS

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
