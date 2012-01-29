/*	$NetBSD: quotactl.h,v 1.30 2012/01/29 07:13:43 dholland Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David A. Holland.
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

#ifndef _SYS_QUOTACTL_H_
#define _SYS_QUOTACTL_H_

/*
 * Note - this is an internal interface. Application code (and,
 * really, anything that isn't libquota or inside the kernel) should
 * use the <quota.h> API instead.
 */

/* Size of random quota strings */
#define QUOTA_NAMELEN   32

/*
 * Restrictions for qs_restrictions.
 */
#define QUOTA_RESTRICT_NEEDSQUOTACHECK	0x1	/* quotacheck(8) required */
#define QUOTA_RESTRICT_UNIFORMGRACE	0x2	/* grace time is global */
#define QUOTA_RESTRICT_32BIT		0x4	/* values limited to 2^32 */
        
/*              
 * Structure for QUOTACTL_STAT.
 */             
struct quotastat {
	char qs_implname[QUOTA_NAMELEN];
	unsigned qs_numidtypes;
	unsigned qs_numobjtypes;
	unsigned qs_restrictions;
};
                        
/*
 * Semi-opaque structure for cursors. This holds the cursor state in
 * userland; the size is exposed only to libquota, not to client code,
 * and is meant to be large enough to accomodate all likely future
 * expansion without being unduly bloated, as it will need to be
 * copied in and out for every call using it.
 */
struct quotakcursor {
	union {
		char qkc_space[64];
		uintmax_t __qkc_forcealign;
	} u;
};

/* Command codes. */
#define QUOTACTL_STAT		0
#define QUOTACTL_GET		1
#define QUOTACTL_PUT		2
#define QUOTACTL_DELETE		3
#define QUOTACTL_CURSOROPEN	4
#define QUOTACTL_CURSORCLOSE	5
#define QUOTACTL_CURSORSKIPIDTYPE 6
#define QUOTACTL_CURSORGET	7
#define QUOTACTL_CURSORATEND	8
#define QUOTACTL_CURSORREWIND	9
#define QUOTACTL_QUOTAON	10
#define QUOTACTL_QUOTAOFF	11

/* Argument encoding. */
struct vfs_quotactl_args {
	unsigned qc_op;
	union {
		struct {
			struct quotastat *qc_ret;
		} stat;
		struct {
			const struct quotakey *qc_key;
			struct quotaval *qc_ret;
		} get;
		struct {
			const struct quotakey *qc_key;
			const struct quotaval *qc_val;
		} put;
		struct {
			const struct quotakey *qc_key;
		} delete;
		struct {
			struct quotakcursor *qc_cursor;
		} cursoropen;
		struct {
			struct quotakcursor *qc_cursor;
		} cursorclose;
		struct {
			struct quotakcursor *qc_cursor;
			unsigned qc_idtype;
		} cursorskipidtype;
		struct {
			struct quotakcursor *qc_cursor;
			struct quotakey *qc_keys;
			struct quotaval *qc_vals;
			unsigned qc_maxnum;
			unsigned *qc_ret;
		} cursorget;
		struct {
			struct quotakcursor *qc_cursor;
			int *qc_ret; /* really boolean */
		} cursoratend;
		struct {
			struct quotakcursor *qc_cursor;
		} cursorrewind;
		struct {
			int qc_idtype;
			const char *qc_quotafile;
		} quotaon;
		struct {
			int qc_idtype;
		} quotaoff;
	} u;
};

#endif /* _SYS_QUOTACTL_H_ */
