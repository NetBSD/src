/*	$NetBSD: md.h,v 1.17.4.1 2012/04/17 00:02:51 yamt Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.
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

/* md.h -- Machine specific definitions for evbarm */

#include "mbr.h"

/* Constants and defines */

/* Megs required for a full X installation. */
#define XNEEDMB 60

#define HAVE_UFS2_BOOT

/*
 *  Default filesets to fetch and install during installation
 *  or upgrade. The standard sets are:
 *      base etc comp games man misc tests text xbase xcomp xetc xfont xserver
 */
#define SET_KERNEL_1_NAME	"kern-ADI_BRH"
#define SET_KERNEL_2_NAME	"kern-INTEGRATOR"
#define SET_KERNEL_3_NAME	"kern-IQ80310"
#define SET_KERNEL_4_NAME	"kern-IQ80321"
#define SET_KERNEL_5_NAME	"kern-MINI2440"
#define SET_KERNEL_6_NAME	"kern-TEAMASA_NPWR"
#define SET_KERNEL_7_NAME	"kern-TS7200"

#define MD_SETS_SELECTED SET_SYSTEM

/*
 * Machine-specific command to write a new label to a disk.
 * For example, shark uses "/sbin/disklabel -w -r".
 * Sun ports should probably use  DISKLABEL_CMD "/sbin/disklabel -w"
 * to get incore to ondisk inode translation for the Sun proms.
 * If not defined, we assume the port does not support disklabels and
 * hand-edited disklabel will NOT be written by MI code.
 */
#define DISKLABEL_CMD "disklabel -w -r"
