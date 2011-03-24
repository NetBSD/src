/* $NetBSD: quota.h,v 1.1 2011/03/24 17:05:39 bouyer Exp $ */
/*-
  * Copyright (c) 2010 Manuel Bouyer
  * All rights reserved.
  * This software is distributed under the following condiions
  * compliant with the NetBSD foundation policy.
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

#ifndef _QUOTA_QUOTA_H
#define _QUOTA_QUOTA_H
#include <sys/types.h>
#include <quota/quotaprop.h>

/* check a quota usage against limits (assumes UFS semantic) */
int quota_check_limit(uint64_t, uint64_t,  uint64_t, uint64_t, time_t, time_t);
/* return values for above */
#define QL_S_ALLOW_OK	0x00 /* below soft limit */
#define QL_S_ALLOW_SOFT	0x01 /* over soft limit */
#define QL_S_DENY_GRACE	0x02 /* over soft limit, grace time expired */
#define QL_S_DENY_HARD	0x03 /* over hard limit */
 
#define QL_F_CROSS	0x80 /* crossing soft limit */

#define QL_STATUS(x)	((x) & 0x0f)
#define QL_FLAGS(x)	((x) & 0xf0)

/*
 * retrieve quotas with ufs semantics from vfs, for the given id and class.
 * second argument points to a struct ufs_quota_entry array of QUOTA_NLIMITS
 * elements.
 */
int getufsquota(const char *, struct ufs_quota_entry *, uid_t, const char *);

/* same as above, but for NFS */
int getnfsquota(const char *, struct ufs_quota_entry *, uid_t, const char *);

/* call one of the above, if appropriate, after a statvfs(2) */
int getfsquota(const char *, struct ufs_quota_entry *, uid_t, const char *);

#endif /* _QUOTA_QUOTA_H */
