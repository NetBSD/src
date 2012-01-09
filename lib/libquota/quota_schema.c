/*	$NetBSD: quota_schema.c,v 1.1 2012/01/09 15:22:39 dholland Exp $	*/
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
__RCSID("$NetBSD: quota_schema.c,v 1.1 2012/01/09 15:22:39 dholland Exp $");

#include <stddef.h>
#include <errno.h>

#include <quota.h>

/* ARGSUSED */
const char *
quota_getimplname(struct quotahandle *qh)
{
	errno = ENOSYS;
	return NULL;
}

/* ARGSUSED */
unsigned
quota_getnumidtypes(struct quotahandle *qh)
{
	return 0;
}

/* ARGSUSED */
const char *
quota_idtype_getname(struct quotahandle *qh, int idtype)
{
	errno = ENOSYS;
	return NULL;
}

/* ARGSUSED */
unsigned
quota_getnumobjtypes(struct quotahandle *qh)
{
	return 0;
}

/* ARGSUSED */
const char *
quota_objtype_getname(struct quotahandle *qh, int objtype)
{
	errno = ENOSYS;
	return NULL;
}

/* ARGSUSED */
int
quota_objtype_isbytes(struct quotahandle *qh, int objtype)
{
	return 0;
}
