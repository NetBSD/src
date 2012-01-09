/*	$NetBSD: quota_cursor.c,v 1.1 2012/01/09 15:22:38 dholland Exp $	*/
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
__RCSID("$NetBSD: quota_cursor.c,v 1.1 2012/01/09 15:22:38 dholland Exp $");

#include <stdlib.h>
#include <errno.h>

#include <quota.h>

/* ARGSUSED */
struct quotacursor *
quota_opencursor(struct quotahandle *qh)
{
	errno = ENOSYS;
	return NULL;
}

/* ARGSUSED */
void
quotacursor_close(struct quotacursor *qc)
{
}

/* ARGSUSED */
int
quotacursor_skipidtype(struct quotacursor *qc, unsigned idtype)
{
	errno = ENOSYS;
	return -1;
}

/* ARGSUSED */
int
quotacursor_get(struct quotacursor *qc,
		struct quotakey *qk_ret, struct quotaval *qv_ret)
{
	errno = ENOSYS;
	return -1;
}

/* ARGSUSED */
int
quotacursor_getn(struct quotacursor *qc,
		 struct quotakey *keys, struct quotaval *vals, 
		 unsigned maxnum)
{
	errno = ENOSYS;
	return -1;
}

/* ARGSUSED */
int
quotacursor_atend(struct quotacursor *qc)
{
	return 0;
}

/* ARGSUSED */
int
quotacursor_rewind(struct quotacursor *qc)
{
	errno = ENOSYS;
	return -1;
}
