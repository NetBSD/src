/*	$NetBSD: quota_schema.c,v 1.5 2012/01/30 16:44:09 dholland Exp $	*/
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: quota_schema.c,v 1.5 2012/01/30 16:44:09 dholland Exp $");

#include <sys/types.h>
#include <sys/statvfs.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <quota.h>
#include "quotapvt.h"

/*
 * Functions for getting information about idtypes and such.
 */

const char *
quota_getimplname(struct quotahandle *qh)
{
	switch (qh->qh_mode) {
	    case QUOTA_MODE_NFS:
		/* XXX this should maybe report the rquotad protocol version */
		return "nfs via rquotad";

	    case QUOTA_MODE_PROPLIB:
		return __quota_proplib_getimplname(qh);

	    case QUOTA_MODE_OLDFILES:
		return __quota_proplib_getimplname(qh);

	    default:
		break;
	}
	errno = EINVAL;
	return NULL;
}

unsigned
quota_getrestrictions(struct quotahandle *qh)
{
	switch (qh->qh_mode) {
	    case QUOTA_MODE_NFS:
		/* XXX this should maybe report the rquotad protocol version */
		return QUOTA_RESTRICT_32BIT | QUOTA_RESTRICT_READONLY;

	    case QUOTA_MODE_PROPLIB:
		return __quota_proplib_getrestrictions(qh);

	    case QUOTA_MODE_OLDFILES:
		return QUOTA_RESTRICT_NEEDSQUOTACHECK |
			QUOTA_RESTRICT_UNIFORMGRACE |
			QUOTA_RESTRICT_32BIT;
	    default:
		break;
	}
	errno = EINVAL;
	return 0;
}

unsigned
quota_getnumidtypes(struct quotahandle *qh)
{
	switch (qh->qh_mode) {
	    case QUOTA_MODE_NFS:
		/* XXX for old rquotad versions this should be 1... */
		return 2;

	    case QUOTA_MODE_PROPLIB:
		return __quota_proplib_getnumidtypes();

	    case QUOTA_MODE_OLDFILES:
	    default:
		break;
	}
	return 2;
}

const char *
quota_idtype_getname(struct quotahandle *qh, int idtype)
{
	switch (qh->qh_mode) {
	    case QUOTA_MODE_PROPLIB:
		return __quota_proplib_idtype_getname(idtype);

	    case QUOTA_MODE_NFS:
	    case QUOTA_MODE_OLDFILES:
		break;
	}

	switch (idtype) {
	    case QUOTA_IDTYPE_USER:
		return "user";

	    case QUOTA_IDTYPE_GROUP:
		return "group";

	    default:
		break;
	}
	errno = EINVAL;
	return "???";
}

unsigned
quota_getnumobjtypes(struct quotahandle *qh)
{
	switch (qh->qh_mode) {
	    case QUOTA_MODE_PROPLIB:
		return __quota_proplib_getnumobjtypes();

	    case QUOTA_MODE_NFS:
	    case QUOTA_MODE_OLDFILES:
	    default:
		break;
	}
	return 2;
}

const char *
quota_objtype_getname(struct quotahandle *qh, int objtype)
{
	switch (qh->qh_mode) {
	    case QUOTA_MODE_PROPLIB:
		return __quota_proplib_objtype_getname(objtype);
	    case QUOTA_MODE_NFS:
	    case QUOTA_MODE_OLDFILES:
	    default:
		break;
	}

	switch (objtype) {
	    case QUOTA_OBJTYPE_BLOCKS:
		return "block";
	    case QUOTA_OBJTYPE_FILES:
		return "file";
	    default:
		break;
	}
	errno = EINVAL;
	return "???"; /* ? */
}

int
quota_objtype_isbytes(struct quotahandle *qh, int objtype)
{
	switch (qh->qh_mode) {
	    case QUOTA_MODE_PROPLIB:
		return __quota_proplib_objtype_isbytes(objtype);
	    case QUOTA_MODE_NFS:
	    case QUOTA_MODE_OLDFILES:
	    default:
		break;
	}

	switch (objtype) {
	    case QUOTA_OBJTYPE_BLOCKS:
		return 1;
	    case QUOTA_OBJTYPE_FILES:
		return 0;
	    default:
		break;
	}
	errno = EINVAL;
	return 0; /* ? */
}
