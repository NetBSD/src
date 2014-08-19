/*	$NetBSD: md.h,v 1.1.6.2 2014/08/20 00:05:15 tls Exp $	*/

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

/*
 * Symbolic names for disk partitions
 */
#include <machine/disklabel.h>

#define	PART_ROOT	PART_A
#define	PART_SWAP	PART_B
#define	PART_RAW	RAW_PART
#define	PART_USR	PART_D
#define	PART_FIRST_FREE	PART_D

#define	DEFROOTSIZE	200	/* Default root size */
#define	XNEEDMB		100	/* Extra megs for full X installation */

#define	SET_KERNEL_1_NAME	"kern-T_SH7706LAN"

/*
 * Machine-specific command to write a new label to a disk.
 * If not defined, we assume the port does not support disklabels and
 * the hand-edited disklabel will NOT be written by MI code.
 * ews4800mips stores disklabel and EWS-UX compatible PDINFO+VTOC.
 * Don't use -r option.
 */
#define	DISKLABEL_CMD	"disklabel -w -r"
