/* $NetBSD: quota.h,v 1.1.2.1 2011/01/20 14:25:01 bouyer Exp $ */
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

#ifndef _SYS_QUOTA_H_
#define _SYS_QUOTA_H_

/*
 * quota types available in the system. We expect this to be shared
 * by all filesystems.
 */
#define MAXQUOTAS	2
#define USRQUOTA	0	/* element used for user quotas */
#define GRPQUOTA	1	/* element used for group quotas */

/*
 * Definitions for the default names for types above
 */
#define INITQFNAMES { \
	"user",		/* USRQUOTA */ \
	"group",	/* GRPQUOTA */ \
	"undefined", \
}

#ifndef _KERNEL
__BEGIN_DECLS
int quotactl(const char *, struct plistref *) __RENAME(__quotactl50);
__END_DECLS
#endif
#endif /* _SYS_QUOTA_H_ */
