/*	$NetBSD: fuse_lowlevel.h,v 1.2 2022/01/22 08:03:32 pho Exp $	*/

/*
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FUSE_LOWLEVEL_H_
#define _FUSE_LOWLEVEL_H_

#include <fuse.h>

#ifdef __cplusplus
extern "C" {
#endif

struct fuse_cmdline_opts {
	int singlethread;
	int foreground;
	int debug;
	int nodefault_fsname;
	char *mountpoint;
	int show_version;
	int show_help;
};

int fuse_parse_cmdline(struct fuse_args *args, struct fuse_cmdline_opts *opts);
/* Print low-level version information to stdout. Appeared on FUSE
 * 3.0. */
void fuse_lowlevel_version(void);

/* Print available options for fuse_parse_cmdline(). Appeared on FUSE
 * 3.0. */
void fuse_cmdline_help(void);

#ifdef __cplusplus
}
#endif

#endif /* _FUSE_LOWLEVEL_H_ */
