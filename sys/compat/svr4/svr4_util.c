/*	$NetBSD: svr4_util.c,v 1.4 1994/10/29 00:43:31 christos Exp $	 */

/*
 * Copyright (c) 1994 Christos Zoulas
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <compat/svr4/svr4_util.h>

const char      svr4_emul_path[] = "/emul/svr4";

int
svr4_emul_find(p, sgp, prefix, path, pbuf)
	struct proc	 *p;
	caddr_t		 *sgp;		/* Pointer to stackgap memory */
	const char	 *prefix;
	char		 *path;
	char		**pbuf;
{
	struct nameidata	 nd;
	int			 error;
	char			*ptr, *buf;
	size_t			 sz, len;

	buf = (char *) malloc(MAXPATHLEN, M_TEMP, M_WAITOK);
	*pbuf = path;

	for (ptr = buf; (*ptr = *prefix) != '\0'; ptr++, prefix++)
		continue;

	sz = MAXPATHLEN - (ptr - buf);

	/* 
	 * If sgp is not given then the path is already in kernel space
	 */
	if (sgp == NULL)
		error = copystr(path, ptr, sz, &len);
	else
		error = copyinstr(path, ptr, sz, &len);

	if (error) {
		DPRINTF(("copy failed %d\n", error));
		free(buf, M_TEMP);
		return error;
	}
	DPRINTF(("looking for %s [%d, %d]: ", buf, sz, len));

	if (*ptr != '/') {
		DPRINTF(("no slash\n"));
		free(buf, M_TEMP);
		return EINVAL;
	}
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, buf, p);

	if ((error = namei(&nd)) != 0) {
		DPRINTF(("not found\n"));
		free(buf, M_TEMP);
		return error;
	}
	if (sgp == NULL)
		*pbuf = buf;
	else {
		sz = &ptr[len] - buf;
		*pbuf = stackgap_alloc(sgp, sz + 1);
		error = copyout(buf, *pbuf, sz);
		free(buf, M_TEMP);
	}

	DPRINTF(("ok\n"));

	vrele(nd.ni_vp);
	return error;
}
