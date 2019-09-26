/*	$NetBSD: devopen.h,v 1.5 2019/09/26 12:21:03 nonaka Exp $	*/

/*-
 * Copyright (c) 2016 Kimihiro Nonaka <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

extern int boot_biosdev;
extern daddr_t boot_biossector;
extern const int nfsys_disk;
extern struct fs_ops file_system_disk[];
extern struct fs_ops file_system_nfs;
extern struct fs_ops file_system_tftp;
extern struct fs_ops file_system_null;

#define	MAXDEVNAME	39 /* mxmimum is "NAME=" + 34 char part_name */

void bios2dev(int, daddr_t, char **, int *, int *, const char **);

struct devdesc {
	char	d_name[MAXDEVNAME];
	char	d_unit;
};

struct netboot_fstab {
	const char *name;
	struct fs_ops *ops;
};

extern const struct netboot_fstab netboot_fstab[];
extern const int nnetboot_fstab;

const struct netboot_fstab *netboot_fstab_find(const char *);
