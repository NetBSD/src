/*	$NetBSD: readdir.c,v 1.3.22.1 2013/02/25 00:28:45 tls Exp $	*/

/*-
 * Copyright (c) 1999,2000 Jonathan Lemon <jlemon@freebsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>


#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include <sys/param.h>
#include <sys/dirent.h>

#include <efi/libefi/efifsdev.h>
#include <bootstrap.h>

int skifs_readdir(struct open_file *f, struct dirent *d);

struct dirent *
readdirfd(int fd)
{
	static struct dirent dir;		/* XXX not thread safe. eh ??? */
	struct open_file *f = &files[fd];

	if ((unsigned)fd >= SOPEN_MAX) {
		errno = EBADF;
		return (NULL);
	}
	if (f->f_flags & F_RAW) {
		errno = EIO;
		return (NULL);
	}

	/* XXXwill be f->f_ops->fo_readdir)(f, &dir); when/if we fix stand.h */
	errno = FS_READDIR(f, &dir); 

	if (errno)
		return (NULL);
	return (&dir);
}
