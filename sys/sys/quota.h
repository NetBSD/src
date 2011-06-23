/* $NetBSD: quota.h,v 1.3.4.1 2011/06/23 14:20:29 cherry Exp $ */
/*-
  * Copyright (c) 2010 Manuel Bouyer
  * All rights reserved.
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

#if !defined(_KERNEL) && !defined(_STANDALONE)
__BEGIN_DECLS
int quotactl(const char *, struct plistref *) __RENAME(__quotactl50);
__END_DECLS
#endif

/* strings used in dictionary for the different quota class */
#define QUOTADICT_CLASS_USER "user"
#define QUOTADICT_CLASS_GROUP "group"

/* strings used in dictionary for the different limit types */
#define QUOTADICT_LTYPE_BLOCK "block"
#define QUOTADICT_LTYPE_FILE "file"

/* strings used in dictionary for the different limit and usage values */
#define QUOTADICT_LIMIT_SOFT "soft"
#define QUOTADICT_LIMIT_HARD "hard"
#define QUOTADICT_LIMIT_GTIME "grace time"
#define QUOTADICT_LIMIT_USAGE "usage"
#define QUOTADICT_LIMIT_ETIME "expire time"

#endif /* _SYS_QUOTA_H_ */
