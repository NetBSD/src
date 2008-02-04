/*	$NetBSD: auth.c,v 1.2.6.4 2008/02/04 09:24:51 yamt Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by
 * The Finnish Cultural Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/kauth.h>
#include <sys/kmem.h>

#include "rump.h"
#include "rumpuser.h"

struct kauth_cred {
	uid_t cr_uid;
	gid_t cr_gid;

	size_t cr_ngroups;
	gid_t cr_groups[NGROUPS];
};

kauth_cred_t
rump_cred_create(uid_t uid, gid_t gid, size_t ngroups, gid_t *groups)
{
	kauth_cred_t cred;

	KASSERT(ngroups <= NGROUPS);
	cred = kmem_alloc(sizeof(struct kauth_cred), KM_SLEEP);

	cred->cr_uid = uid;
	cred->cr_gid = gid;
	cred->cr_ngroups = ngroups;
	memcpy(cred->cr_groups, groups, ngroups * sizeof(gid_t));

	return cred;
}

void
rump_cred_destroy(kauth_cred_t cred)
{

	kmem_free(cred, sizeof(struct kauth_cred));
}

int
kauth_authorize_generic(kauth_cred_t cred, kauth_action_t op, void *arg0)
{

	if (op != KAUTH_GENERIC_ISSUSER)
		panic("%s: op %d not implemented", __func__, op);

	if (cred == RUMPCRED_SUSER || cred->cr_uid == 0)
		return 0;

	return EPERM;
}

int
kauth_authorize_system(kauth_cred_t cred, kauth_action_t op,
	enum kauth_system_req req, void *arg1, void *arg2, void *arg3)
{

	if (op != KAUTH_SYSTEM_CHSYSFLAGS)
		panic("%s: op %d not implemented", __func__, op);

	/* always allow */
	return 0;
}

uid_t
kauth_cred_getuid(kauth_cred_t cred)
{

	return kauth_cred_geteuid(cred);
}

uid_t
kauth_cred_geteuid(kauth_cred_t cred)
{

	return cred == RUMPCRED_SUSER ? 0 : cred->cr_uid;
}

gid_t
kauth_cred_getegid(kauth_cred_t cred)
{

	return cred == RUMPCRED_SUSER ? 0 : cred->cr_gid;
}

int
kauth_cred_ismember_gid(kauth_cred_t cred, gid_t gid, int *resultp)
{
	int i;

	if (cred == RUMPCRED_SUSER) {
		*resultp = 1;
		return 0;
	}

	*resultp = 1;
	if (cred->cr_gid == gid)
		return 0;
	for (i = 0; i < cred->cr_ngroups; i++)
		if (cred->cr_groups[i] == gid)
			break;
	if (i == cred->cr_ngroups)
		*resultp = 0;

	return 0;
}

u_int
kauth_cred_ngroups(kauth_cred_t cred)
{

	if (cred == RUMPCRED_SUSER)
		return 1;

	return cred->cr_ngroups;
}

gid_t
kauth_cred_group(kauth_cred_t cred, u_int idx)
{

	if (cred == RUMPCRED_SUSER)
		return 0;

	KASSERT(idx < cred->cr_ngroups);

	return cred->cr_groups[idx];
}

void
kauth_cred_to_uucred(struct uucred *uucred, const kauth_cred_t cred)
{

	if (cred == RUMPCRED_SUSER) {
		memset(uucred, 0, sizeof(struct uucred));
		return;
	}

	uucred->cr_uid = cred->cr_uid;
	uucred->cr_gid = cred->cr_gid;
	uucred->cr_ngroups = cred->cr_ngroups;
	memcpy(uucred->cr_groups, cred->cr_groups,
	    cred->cr_ngroups * sizeof(gid_t));
}

void
kauth_cred_hold(kauth_cred_t cred)
{

	/* nada: creds are always destroyed upon exit from VOP */
}

void
kauth_cred_free(kauth_cred_t cred)
{

	/* nada: see above */
}

kauth_cred_t
kauth_cred_get()
{

	return curlwp->l_cred;
}
