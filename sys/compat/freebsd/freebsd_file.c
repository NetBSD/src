/*	$NetBSD: freebsd_file.c,v 1.32.20.1 2017/12/03 11:36:53 jdolecek Exp $	*/

/*
 * Copyright (c) 1995 Frank van der Linden
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project
 *      by Frank van der Linden
 * 4. The name of the author may not be used to endorse or promote products
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
 *
 *	from: linux_file.c,v 1.3 1995/04/04 04:21:30 mycroft Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: freebsd_file.c,v 1.32.20.1 2017/12/03 11:36:53 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mount.h>

#include <sys/syscallargs.h>

#include <compat/freebsd/freebsd_syscallargs.h>
#include <compat/common/compat_util.h>
#include <compat/sys/mount.h>

#define	ARRAY_LENGTH(array)	(sizeof(array)/sizeof(array[0]))

static const char * convert_from_freebsd_mount_type(int);

static const char *
convert_from_freebsd_mount_type(int type)
{
	static const char * const netbsd_mount_type[] = {
		NULL,     /*  0 = MOUNT_NONE */
		"ffs",	  /*  1 = "Fast" Filesystem */
		"nfs",	  /*  2 = Network Filesystem */
		"mfs",	  /*  3 = Memory Filesystem */
		"msdos",  /*  4 = MSDOS Filesystem */
		"lfs",	  /*  5 = Log-based Filesystem */
		"lofs",	  /*  6 = Loopback filesystem */
		"fdesc",  /*  7 = File Descriptor Filesystem */
		"portal", /*  8 = Portal Filesystem */
		"null",	  /*  9 = Minimal Filesystem Layer */
		"umap",	  /* 10 = User/Group Identifier Remapping Filesystem */
		"kernfs", /* 11 = Kernel Information Filesystem */
		"procfs", /* 12 = /proc Filesystem */
		"afs",	  /* 13 = Andrew Filesystem */
		"cd9660", /* 14 = ISO9660 (aka CDROM) Filesystem */
		"union",  /* 15 = Union (translucent) Filesystem */
		NULL,     /* 16 = "devfs" - existing device Filesystem */
#if 0 /* These filesystems don't exist in FreeBSD */
		"adosfs", /* ?? = AmigaDOS Filesystem */
#endif
	};

	if (type < 0 || type >= ARRAY_LENGTH(netbsd_mount_type))
		return (NULL);
	return (netbsd_mount_type[type]);
}

int
freebsd_sys_mount(struct lwp *l, const struct freebsd_sys_mount_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) type;
		syscallarg(char *) path;
		syscallarg(int) flags;
		syscallarg(void *) data;
	} */
	const char *type;
	register_t dummy;

	if ((type = convert_from_freebsd_mount_type(SCARG(uap, type))) == NULL)
		return ENODEV;

	return do_sys_mount(l, type, UIO_SYSSPACE, SCARG(uap, path),
	    SCARG(uap, flags), SCARG(uap, data), UIO_USERSPACE, 0, &dummy);
}

/* XXX - UNIX domain: int freebsd_sys_bind(int s, void *name, int namelen); */
/* XXX - UNIX domain: int freebsd_sys_connect(int s, void *name, int namelen); */
