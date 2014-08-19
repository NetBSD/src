/* $NetBSD: devopen.c,v 1.2.16.1 2014/08/20 00:03:22 tls Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/param.h>

#include <netinet/in.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/nfs.h>
#include <lib/libsa/ufs.h>
#include <lib/libsa/tftp.h>
#include <lib/libkern/libkern.h>

#include "globals.h"
#include "memfs.h"

struct devsw devnet = { "net", net_strategy, net_open, net_close, noioctl };
struct devsw devdsk = { "dsk", dsk_strategy, dsk_open, dsk_close, noioctl };

struct fs_ops file_system[1] = { FS_OPS(null) };
int nfsys = 1;
struct fs_ops fs_nfs   = FS_OPS(nfs);
struct fs_ops fs_tftp  = FS_OPS(tftp);
struct fs_ops fs_ffsv2 = FS_OPS(ffsv2);
struct fs_ops fs_ffsv1 = FS_OPS(ffsv1);
struct fs_ops fs_mem   = FS_OPS(mem);

static void parseunit(const char *, int *, int *, char **);

int
devopen(struct open_file *of, const char *name, char **file)
{
	int error;
	int unit, part;
	extern char bootfile[]; /* handed by DHCP */

	if (of->f_flags != F_READ)
		return EPERM;

	if (strncmp("mem:", name, 4) == 0) {
		of->f_dev = NULL;
		of->f_flags |= F_NODEV;
		file_system[0] = fs_mem;
		*file = (char *)&name[4];
		return 0;		/* MEM */
	}
	if (strncmp("net:", name, 4) == 0 || strncmp("nfs:", name, 4) == 0) {
		of->f_dev = &devnet;
		if ((error = net_open(of, &name[4], "nfs")) != 0)
			return error;
		file_system[0] = fs_nfs;
		*file = bootfile;	/* resolved fname */
		return 0;		/* NFS */
	}
	if (strncmp("tftp:", name, 5) == 0) {
		of->f_dev = &devnet;
		if ((error = net_open(of, &name[5], "tftp")) != 0)
			return error;
		file_system[0] = fs_tftp;
		*file = bootfile;	/* resolved fname */
		return 0;		/* TFTP */
	}
	if (name[0] == 'w' && name[1] == 'd') {
		parseunit(&name[2], &unit, &part, file);
		of->f_dev = &devdsk;
		if (*file == NULL || **file <= ' ')
			*file = "netbsd";
		if ((error = dsk_open(of, unit, part, *file)) != 0)
			return error;
		file_system[0] = *dsk_fsops(of);
		return 0;		/* FFS */
	}
	return ENOENT;
}

static void
parseunit(const char *name, int *unitp, int *partp, char **pathp)
{
	const char *p = name;
	int unit, part;

	unit = part = -1;
	while (*p != ':' && *p != '\0') {
		if (unit == -1 && *p >= '0' && *p <= '9')
			unit = *p - '0';
		if (part == -1 && *p >= 'a' && *p < 'a' + 16)
			part = *p - 'a';
		p += 1;
	}
	*unitp = (unit == -1) ? 0 : unit;
	*partp = (part == -1) ? 0 : part;
	*pathp = (*p == ':') ? (char *)p + 1 : NULL;
}

/* ARGSUSED */
int
noioctl(struct open_file *f, u_long cmd, void *data)
{

	return EINVAL;
}
