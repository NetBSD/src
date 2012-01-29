/*	$NetBSD: quotactl.h,v 1.15 2012/01/29 06:52:39 dholland Exp $	*/
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

/* Command codes. */
#define QUOTACTL_GETVERSION	0
#define QUOTACTL_QUOTAON	1
#define QUOTACTL_QUOTAOFF	2
#define QUOTACTL_GET		3
#define QUOTACTL_PUT		4
#define QUOTACTL_GETALL		5
#define QUOTACTL_CLEAR		6

/* Argument encoding. */
enum vfs_quotactl_argtypes {
	QCT_PROPLIB,	/* quotaon/off, get, set, getall, clear */
	QCT_GETVERSION,	/* getversion */
	QCT_GET,	/* get */
	QCT_PUT,	/* put */
	QCT_CLEAR,	/* clear */
};
struct vfs_quotactl_args {
	enum vfs_quotactl_argtypes qc_type;
	union {
		struct {
			prop_dictionary_t qc_cmddict;
			int qc_q2type;
			prop_array_t qc_datas;
		} proplib;
		struct {
			int *qc_version_ret;
		} getversion;
		struct {
			const struct quotakey *qc_key;
			struct quotaval *qc_ret;
		} get;
		struct {
			const struct quotakey *qc_key;
			const struct quotaval *qc_val;
		} put;
		struct {
			int qc_idtype;
			id_t qc_id;
			int qc_defaultq;
		} clear;
	} u;
};

#endif /* _SYS_QUOTACTL_H_ */
