/*	$NetBSD: mount.h,v 1.1 2009/08/07 20:57:58 haad Exp $	*/

/*-
 * Copyright (c) 2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/compat/opensolaris/sys/mount.h,v 1.1 2007/04/06 01:09:06 pjd Exp $
 */

#include_next <sys/mount.h>

#ifndef _OPENSOLARIS_SYS_MOUNT_H_
#define	_OPENSOLARIS_SYS_MOUNT_H_

#define	MS_OVERLAY	0
#define	MS_FORCE	MNT_FORCE
#define	MS_REMOUNT	MNT_UPDATE
#define	MS_OPTIONSTR	__MNT_UNUSED1
#define	MS_NOMNTTAB	__MNT_UNUSED2

typedef	struct fid		fid_t;

#define	mount(a,b,c,d,e,f,g,h)	zmount(a,b,c,d,e,f,g,h)

struct zfs_args {
	char fspec[MAXNAMELEN - 1];
	char dataptr[MAXPATHLEN];
	char optptr[MAXPATHLEN];
	char *fstype;
	int  mflag;
	int  datalen;
	int  optlen;
	int  flags;
} za;

typedef struct zfs_args zfs_args_t;

int
zmount(const char *spec, const char *dir, int mflag, char *fstype,
    char *dataptr, int datalen, char *optptr, int optlen);

int
umount2(const char *spec, int mflag);

#endif	/* !_OPENSOLARIS_SYS_MOUNT_H_ */
