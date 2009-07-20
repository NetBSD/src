/* $NetBSD: devopen.c,v 1.10 2009/07/20 11:43:09 nisimura Exp $ */

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
#include <lib/libkern/libkern.h>

#include "globals.h"

struct devsw devsw[] = {
	{ "net", net_strategy, net_open, net_close, noioctl },
};
int ndevs = sizeof(devsw) / sizeof(devsw[0]);

struct fs_ops fssw[] = {
	FS_OPS(nfs),
};
struct fs_ops file_system[1];
int nfsys = 1;

int
devopen(struct open_file *of, const char *name, char **file)
{
	int error;
	extern char bootfile[]; /* handed by DHCP */

	if (of->f_flags != F_READ)
		return EPERM;

	if (strcmp("net:", name) == 0) {
		of->f_dev = &devsw[0];
		if ((error = net_open(of, name)) != 0)
			return error;
		file_system[0] = fssw[0];
		*file = bootfile;	/* resolved fname */
		return 0;		/* NFS */
	}
#if 0 /* later */
	if (name[0] == 'w' && name[1] == 'd') {
		parseunit(&name[2], &unit, &part, file);
		of->f_dev = &devsw[1];
		if ((error = wdopen(of, unit, part)) != 0)
			return error;
		switch (parsefstype(of->f_devdata)) {
		default:
		case FS_BSDFFS:
			file_system[0] = fssw[1]; break;
		case FS_EX2FS:
			file_system[0] = fssw[2]; break;
		case FS_MSDOS:
			file_system[0] = fssw[3]; break;
		}
		return 0;
	}
#endif
	return ENOENT;
}

/* ARGSUSED */
int
noioctl(struct open_file *f, u_long cmd, void *data)
{

	return EINVAL;
}

#if 0
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
#endif
