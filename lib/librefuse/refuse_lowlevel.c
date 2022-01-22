/*	$NetBSD: refuse_lowlevel.c,v 1.4 2022/01/22 08:09:39 pho Exp $	*/

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

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: refuse_lowlevel.c,v 1.4 2022/01/22 08:09:39 pho Exp $");
#endif /* !lint */

#include <fuse_internal.h>
#include <fuse_opt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REFUSE_LOWLEVEL_OPT(t, p, v) \
	{ t, offsetof(struct fuse_cmdline_opts, p), v }

static struct fuse_opt fuse_lowlevel_opts[] = {
	REFUSE_LOWLEVEL_OPT("-h"       , show_help       , REFUSE_SHOW_HELP_FULL),
	REFUSE_LOWLEVEL_OPT("--help"   , show_help       , REFUSE_SHOW_HELP_FULL),
	REFUSE_LOWLEVEL_OPT("-ho"      , show_help       , REFUSE_SHOW_HELP_NO_HEADER),
	REFUSE_LOWLEVEL_OPT("-V"       , show_version    , 1),
	REFUSE_LOWLEVEL_OPT("--version", show_version    , 1),
	REFUSE_LOWLEVEL_OPT("-d"       , debug           , 1),
	REFUSE_LOWLEVEL_OPT("debug"    , debug           , 1),
	REFUSE_LOWLEVEL_OPT("-d"       , foreground      , 1),
	REFUSE_LOWLEVEL_OPT("debug"    , foreground      , 1),
	REFUSE_LOWLEVEL_OPT("-f"       , foreground      , 1),
	REFUSE_LOWLEVEL_OPT("-s"       , singlethread    , 1),
	REFUSE_LOWLEVEL_OPT("fsname="  , nodefault_fsname, 1),
	FUSE_OPT_KEY       ("fsname="  , FUSE_OPT_KEY_KEEP  ),
	FUSE_OPT_END
};

void fuse_lowlevel_version(void)
{
	/* XXX: Print something */
}

void fuse_cmdline_help(void)
{
	printf("refuse options:\n"
		   "    -d, -o debug    enable debug output, implies -f\n"
		   "    -f              foreground mode\n"
		   "    -s              single threaded mode (always enabled for now)\n"
		   "    -o fsname=NAME  explicitly set the name of the file system\n");
}

static int refuse_lowlevel_opt_proc(void* data, const char *arg, int key,
									struct fuse_args *outargs)
{
	struct fuse_cmdline_opts *opts = data;

	switch (key) {
	case FUSE_OPT_KEY_NONOPT:
		if (opts->mountpoint == NULL) {
			return fuse_opt_add_opt(&opts->mountpoint, arg);
		}
		else {
			(void)fprintf(stderr, "fuse: invalid argument: %s\n", arg);
			return -1;
		}

	default:
		return 1; /* keep the argument */
	}
}

static int add_default_fsname(struct fuse_args *args)
{
	const char *arg0 = args->argv[0];
	const char *slash;

	if (arg0 == NULL || arg0[0] == '\0') {
		return fuse_opt_add_arg(args, "-ofsname=refuse");
	} else {
		char *arg;
		int rv;

		if ((slash = strrchr(arg0, '/')) == NULL) {
			slash = arg0;
		} else {
			slash += 1;
		}

		if (asprintf(&arg, "-ofsname=refuse:%s", slash) == -1)
			return -1;

		rv = fuse_opt_add_arg(args, arg);
		free(arg);
		return rv;
	}
}

int
__fuse_parse_cmdline(struct fuse_args *args, struct fuse_cmdline_opts *opts)
{
	memset(opts, 0, sizeof(*opts));

	/*
	 * XXX: The single threaded mode is always enabled and cannot be
	 * disabled. This is because puffs currently does not support
	 * multithreaded operation.
	 */
	opts->singlethread = 1;

	if (fuse_opt_parse(args, opts, fuse_lowlevel_opts,
					   refuse_lowlevel_opt_proc) == -1)
		return -1;

	if (!opts->nodefault_fsname) {
		/* -o fsname=%s is not specified so add a default fsname
		 * generated from the program basename.
		 */
		if (add_default_fsname(args) == -1)
			return -1;
	}

	return 0;
}
