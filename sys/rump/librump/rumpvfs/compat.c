/*	$NetBSD: compat.c,v 1.1 2009/05/22 08:59:32 pooka Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the Nokia Foundation
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat.c,v 1.1 2009/05/22 08:59:32 pooka Exp $");

#include <sys/param.h>
#include <sys/syscallargs.h>

#include <rump/rump.h>
#include <rump/rumpuser.h>

/*
 * XXX: these are handwritten for now.  They provide compat for
 * calling post-time_t file systems from a pre-time_t userland.
 * (mknod is missing.  I don't care very much)
 *
 * Doing remote system calls with these does not (obviously) work.
 */
#if     BYTE_ORDER == BIG_ENDIAN
#define SPARG(p,k)      ((p)->k.be.datum)
#else /* LITTLE_ENDIAN, I hope dearly */
#define SPARG(p,k)      ((p)->k.le.datum)
#endif

int
rump_sys___stat30(const char *path, struct stat *sb)
{
	struct compat_50_sys___stat30_args args;
	register_t retval = 0;
	int error = 0;

	SPARG(&args, path) = path;
	SPARG(&args, ub) = (struct stat30 *)sb;

	error = compat_50_sys___stat30(curlwp, &args, &retval);
	if (error) {
		retval = -1;
		rumpuser_seterrno(error);
	}
	return retval;
}

int
rump_sys___lstat30(const char *path, struct stat *sb)
{
	struct compat_50_sys___lstat30_args args;
	register_t retval = 0;
	int error = 0;

	SPARG(&args, path) = path;
	SPARG(&args, ub) = (struct stat30 *)sb;

	error = compat_50_sys___lstat30(curlwp, &args, &retval);
	if (error) {
		retval = -1;
		rumpuser_seterrno(error);
	}
	return retval;
}
