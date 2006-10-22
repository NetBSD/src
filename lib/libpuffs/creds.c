/*	$NetBSD: creds.c,v 1.1 2006/10/22 22:52:21 pooka Exp $	*/

/*
 * Copyright (c) 2006  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the Ulla Tuominen Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: creds.c,v 1.1 2006/10/22 22:52:21 pooka Exp $");
#endif /* !lint */

/*
 * Interface for dealing with credits.
 */

#include <sys/types.h>
#include <sys/param.h>

#include <puffs.h>
#include <errno.h>
#include <string.h>

#define UUCCRED(a) (a->pcr_type == PUFFCRED_TYPE_UUC)
#define INTCRED(a) (a->pcr_type == PUFFCRED_TYPE_INTERNAL)

int
puffs_cred_getuid(const struct puffs_cred *pcr, uid_t *ruid)
{

	if (!UUCCRED(pcr))
		return EINVAL;
	*ruid = pcr->pcr_uuc.cr_uid;

	return 0;
}

int
puffs_cred_getgid(const struct puffs_cred *pcr, gid_t *rgid)
{

	if (!UUCCRED(pcr))
		return EINVAL;
	*rgid = pcr->pcr_uuc.cr_gid;

	return 0;
}

int
puffs_cred_getgroups(const struct puffs_cred *pcr, gid_t *rgids, short *ngids)
{
	short ncopy;

	if (!UUCCRED(pcr))
		return EINVAL;

	ncopy = MIN(*ngids, NGROUPS);
	memcpy(rgids, pcr->pcr_uuc.cr_groups, ncopy);
	*ngids = ncopy;

	return 0;
}

int
puffs_cred_isuid(const struct puffs_cred *pcr, uid_t uid)
{

	return UUCCRED(pcr) && pcr->pcr_uuc.cr_uid == uid;
}

int
puffs_cred_hasgroup(const struct puffs_cred *pcr, gid_t gid)
{
	short i;

	if (!UUCCRED(pcr))
		return 0;

	if (pcr->pcr_uuc.cr_gid == gid)
		return 1;
	for (i = 0; i < pcr->pcr_uuc.cr_ngroups; i++)
		if (pcr->pcr_uuc.cr_groups[i] == gid)
			return 1;

	return 0;
}

int
puffs_cred_iskernel(const struct puffs_cred *pcr)
{

	return INTCRED(pcr) && pcr->pcr_internal == PUFFCRED_CRED_NOCRED;
}

int
puffs_cred_isfs(const struct puffs_cred *pcr)
{

	return INTCRED(pcr) && pcr->pcr_internal == PUFFCRED_CRED_FSCRED;
}

int
puffs_cred_isjuggernaut(const struct puffs_cred *pcr)
{

	return puffs_cred_isuid(pcr, 0) || puffs_cred_iskernel(pcr)
	    || puffs_cred_isfs(pcr);
}
