/*	$NetBSD: quotapvt.h,v 1.13 2012/02/01 05:34:41 dholland Exp $	*/
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

#define QUOTA_MODE_NFS		1
#define QUOTA_MODE_OLDFILES	2
#define QUOTA_MODE_KERNEL	3

struct quotahandle {
	char *qh_mountpoint;
	char *qh_mountdevice;
	int qh_mode;

	/* these are used only by quota_oldfiles */
	int qh_oldfilesopen;
	int qh_userfile;
	int qh_groupfile;
};

struct quotacursor {
	struct quotahandle *qc_qh;
	enum { QC_OLDFILES, QC_KERNEL } qc_type;
	union {
		struct oldfiles_quotacursor *qc_oldfiles;
		struct kernel_quotacursor *qc_kernel;
	} u;
};


/* new non-proplib kernel interface */
const char *__quota_kernel_getimplname(struct quotahandle *);
unsigned __quota_kernel_getrestrictions(struct quotahandle *);
unsigned __quota_kernel_getnumidtypes(struct quotahandle *);
const char *__quota_kernel_idtype_getname(struct quotahandle *, int idtype);
unsigned __quota_kernel_getnumobjtypes(struct quotahandle *);
const char *__quota_kernel_objtype_getname(struct quotahandle *, int objtype);
int __quota_kernel_objtype_isbytes(struct quotahandle *, int objtype);
int __quota_kernel_quotaon(struct quotahandle *, int idtype);
int __quota_kernel_quotaoff(struct quotahandle *, int idtype);
int __quota_kernel_get(struct quotahandle *qh, const struct quotakey *qk,
			struct quotaval *qv);
int __quota_kernel_put(struct quotahandle *qh, const struct quotakey *qk,
			const struct quotaval *qv);
int __quota_kernel_delete(struct quotahandle *qh, const struct quotakey *qk);
struct kernel_quotacursor *__quota_kernel_cursor_create(struct quotahandle *);
void __quota_kernel_cursor_destroy(struct quotahandle *,
				   struct kernel_quotacursor *);
int __quota_kernel_cursor_skipidtype(struct quotahandle *,
				     struct kernel_quotacursor *,
				     unsigned idtype);
int __quota_kernel_cursor_get(struct quotahandle *,
			      struct kernel_quotacursor *,
			      struct quotakey *, struct quotaval *);
int __quota_kernel_cursor_getn(struct quotahandle *,
				struct kernel_quotacursor *,
				struct quotakey *, struct quotaval *,
				unsigned);
int __quota_kernel_cursor_atend(struct quotahandle *,
				struct kernel_quotacursor *);
int __quota_kernel_cursor_rewind(struct quotahandle *,
				 struct kernel_quotacursor *);

/* nfs rquotad interface */
int __quota_nfs_get(struct quotahandle *qh, const struct quotakey *qk,
		    struct quotaval *qv);


/* direct interface to old (quota1-type) files */
void __quota_oldfiles_load_fstab(void);
int __quota_oldfiles_infstab(const char *);
int __quota_oldfiles_initialize(struct quotahandle *qh);
const char *__quota_oldfiles_getimplname(struct quotahandle *);
const char *__quota_oldfiles_getquotafile(struct quotahandle *, int idtype,
					  char *buf, size_t maxlen);
int __quota_oldfiles_quotaon(struct quotahandle *, int idtype);
int __quota_oldfiles_get(struct quotahandle *qh, const struct quotakey *qk,
			struct quotaval *qv);
int __quota_oldfiles_put(struct quotahandle *qh, const struct quotakey *qk,
			const struct quotaval *qv);
int __quota_oldfiles_delete(struct quotahandle *qh, const struct quotakey *qk);
struct oldfiles_quotacursor *
	__quota_oldfiles_cursor_create(struct quotahandle *);
void __quota_oldfiles_cursor_destroy(struct oldfiles_quotacursor *);
int __quota_oldfiles_cursor_skipidtype(struct oldfiles_quotacursor *,
				      unsigned idtype);
int __quota_oldfiles_cursor_get(struct quotahandle *,
			       struct oldfiles_quotacursor *,
			       struct quotakey *, struct quotaval *);
int __quota_oldfiles_cursor_getn(struct quotahandle *,
				struct oldfiles_quotacursor *,
				struct quotakey *, struct quotaval *,
				unsigned);
int __quota_oldfiles_cursor_atend(struct oldfiles_quotacursor *);
int __quota_oldfiles_cursor_rewind(struct oldfiles_quotacursor *);


/* compat for old library */
int __quota_getquota(const char *path, struct quotaval *qv, uid_t id,
		     const char *class);

