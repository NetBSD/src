/*	$NetBSD: quota_schema.c,v 1.2 2012/01/09 15:34:34 dholland Exp $	*/
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
__RCSID("$NetBSD: quota_schema.c,v 1.2 2012/01/09 15:34:34 dholland Exp $");

#include <sys/types.h>
#include <sys/statvfs.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <quota.h>
#include <quota/quotaprop.h>
#include "quotapvt.h"

/*
 * Functions for getting information about idtypes and such.
 */

const char *
quota_getimplname(struct quotahandle *qh)
{
	if (qh->qh_isnfs) {
		/* XXX this should maybe report the rquotad protocol version */
		return "nfs via rquotad";
	} else {
		return __quota_proplib_getimplname(qh);
	}
}

/* ARGSUSED */
unsigned
quota_getnumidtypes(struct quotahandle *qh)
{
	return QUOTA_NCLASS;
}

/* ARGSUSED */
const char *
quota_idtype_getname(struct quotahandle *qh, int idtype)
{
	if (idtype < 0 || idtype >= QUOTA_NCLASS) {
		return NULL;
	}
	return ufs_quota_class_names[idtype];
}

/* ARGSUSED */
unsigned
quota_getnumobjtypes(struct quotahandle *qh)
{
	return QUOTA_NLIMITS;
}

/* ARGSUSED */
const char *
quota_objtype_getname(struct quotahandle *qh, int objtype)
{
	if (objtype < 0 || objtype >= QUOTA_NLIMITS) {
		return NULL;
	}
	return ufs_quota_limit_names[objtype];
}

/* ARGSUSED */
int
quota_objtype_isbytes(struct quotahandle *qh, int objtype)
{
	switch (objtype) {
		case QUOTA_LIMIT_BLOCK: return 1;
		case QUOTA_LIMIT_FILE: return 0;
		default: break;
	}
	return 0;
}
