/* $NetBSD: devopen.c,v 1.4.4.3 2008/01/09 01:48:37 matt Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

int net_open(struct open_file *, ...);
int net_close(struct open_file *);
int net_strategy(void *, int, daddr_t, size_t, void *, size_t *);

struct devsw devsw[] = {
	{ "net", net_strategy, net_open, net_close, noioctl },
};
int ndevs = sizeof(devsw) / sizeof(devsw[0]);

struct fs_ops ops_nfs = FS_OPS(nfs);
struct fs_ops file_system[1];
int nfsys = 1;

int
devopen(of, name, file)
	struct open_file *of;
	const char *name;
	char **file;
{
	int error;
	extern char bootfile[]; /* handed by DHCP */

	if (of->f_flags != F_READ)
		return EPERM;

	if (strcmp("net:", name) == 0) {
		of->f_dev = &devsw[0];
		if ((error = net_open(of, name)) != 0)
			return error;
		file_system[0] = ops_nfs;
		*file = bootfile;	/* resolved fname */
		return 0;		/* NFS */
	}
	return ENOENT;
}

/* ARGSUSED */
int
noioctl(f, cmd, data)
	struct open_file *f;
	u_long cmd;
	void *data;
{

	return EINVAL;
}
