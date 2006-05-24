/*	$NetBSD: devopen.c,v 1.1.42.1 2006/05/24 15:47:49 tron Exp $	*/

/*-
 * Copyright (c) 2001, 2006 Ben Harris
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
 *    derived from this software without specific prior written permission.
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

#include <sys/disklabel.h>
#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include <riscosdisk.h>

int
devopen(struct open_file *f, const char *fname, char **file)
{
#ifdef LIBSA_SINGLE_FILESYSTEM

	/* We don't support any devices yet. */
	f->f_flags |= F_NODEV;
	*file = (char *)fname;
	return 0;
#else
	char *p, *fsname;
	int part, drive, err, fslen;

	p = strchr(fname, ':');
	if (p == NULL || p == fname) return ENOENT;
	*file = p + 1;
	p--;
	if (p == fname) return ENOENT;
	if (islower((unsigned char)*p))
		part = *p-- - 'a';
	else
		part = RAW_PART;
	if (p == fname) return ENOENT;
	drive = 0;
	while (isdigit((unsigned char)*p) && p >= fname)
		drive = drive * 10 + *p-- - '0';
	if (p <= fname) return ENOENT;

	fslen = p + 1 - fname;
	fsname = alloc(fslen + 1);
	memcpy(fsname, fname, fslen);
	fsname[fslen] = 0;
	err =  rodisk_open(f, fsname, drive, part);
	dealloc(fsname, fslen + 1);
	return err;
#endif
}
