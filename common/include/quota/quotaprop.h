/* $NetBSD: quotaprop.h,v 1.1.2.1 2011/06/23 14:17:49 cherry Exp $ */
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

#ifndef _QUOTA_QUOTAPROP_H
#define _QUOTA_QUOTAPROP_H
#include <prop/proplib.h>
#include <sys/quota.h>

int quotaprop_dict_get_uint64(prop_dictionary_t, uint64_t[],
    const char *[], int, bool);
int proptoquota64(prop_dictionary_t, uint64_t *[], const char *[], int,
    const char *[], int);

int quota_get_cmds(prop_dictionary_t, prop_array_t *);
prop_dictionary_t quota_prop_create(void);
bool quota_prop_add_command(prop_array_t, const char *, const char *,
    prop_array_t);

prop_dictionary_t limits64toprop(uint64_t[], const char *[], int);
prop_dictionary_t quota64toprop(uid_t, int, uint64_t *[], const char *[], int,
    const char *[], int);

/* structure used to describe a UFS-like quota entry */
struct ufs_quota_entry {
        uint64_t ufsqe_hardlimit; /* absolute limit */
	uint64_t ufsqe_softlimit; /* overflowable limit */
	uint64_t ufsqe_cur; /* current usage */
	int64_t ufsqe_time; /* grace expiration date for softlimit overflow */
	int64_t ufsqe_grace; /* allowed time for softlimit overflow */
};

/* array of strings for the above */
#define UFS_QUOTA_ENTRY_NAMES \
    {QUOTADICT_LIMIT_HARD, \
     QUOTADICT_LIMIT_SOFT, \
     QUOTADICT_LIMIT_USAGE, \
     QUOTADICT_LIMIT_ETIME, \
     QUOTADICT_LIMIT_GTIME \
    }
#define UFS_QUOTA_NENTRIES 5
extern const char *ufs_quota_entry_names[];

/* array of strings for limit types and associated #define */
extern const char *ufs_quota_limit_names[];
#define QUOTA_LIMIT_BLOCK 0
#define QUOTA_LIMIT_FILE 1
#define QUOTA_NLIMITS 2
#define QUOTA_LIMIT_NAMES { QUOTADICT_LTYPE_BLOCK, QUOTADICT_LTYPE_FILE }

/* array of strings for quota class and associated #define */
extern const char *ufs_quota_class_names[];
#define QUOTA_CLASS_USER 0
#define QUOTA_CLASS_GROUP 1
#define QUOTA_NCLASS 2
#define QUOTA_CLASS_NAMES { QUOTADICT_CLASS_USER, QUOTADICT_CLASS_GROUP }

#endif /* _QUOTA_QUOTAPROP_H */
