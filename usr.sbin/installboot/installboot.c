/*	$NetBSD: installboot.c,v 1.7 2002/04/30 14:21:17 lukem Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn of Wasabi Systems.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(__lint)
__RCSID("$NetBSD: installboot.c,v 1.7 2002/04/30 14:21:17 lukem Exp $");
#endif	/* !__lint */

#include <sys/utsname.h>

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "installboot.h"

int		main(int, char *[]);
static	int	getmachine(ib_params *, const char *, const char *);
static	int	getfstype(ib_params *, const char *, const char *);
static	void	usage(void);

static	ib_params	installboot_params;

int
main(int argc, char *argv[])
{
	struct utsname	utsname;
	ib_params	*params;
	unsigned long	lval;
	int		ch, rv, mode;
	char 		*p;
	const char	*op;

	setprogname(argv[0]);
	params = &installboot_params;
	memset(params, 0, sizeof(*params));
	if ((p = getenv("MACHINE")) != NULL)
		if (! getmachine(params, p, "$MACHINE"))
			exit(1);

	while ((ch = getopt(argc, argv, "b:cm:no:t:v")) != -1) {
		switch (ch) {

		case 'b':
			if (*optarg == '\0')
				goto badblock;
			lval = strtoul(optarg, &p, 0);
			if (lval > UINT32_MAX || *p != '\0') {
 badblock:
				errx(1, "Invalid block number `%s'", optarg);
			}
			params->startblock = (uint32_t)lval;
			params->flags |= IB_STARTBLOCK;
			break;

		case 'c':
			params->flags |= IB_CLEAR;
			break;

		case 'm':
			if (! getmachine(params, optarg, "-m"))
				exit(1);
			break;

		case 'n':
			params->flags |= IB_NOWRITE;
			break;

		case 'o':
			if (params->machine == NULL)
				errx(1,
				    "Machine needs to be specified before -o");
			while ((p = strsep(&optarg, ",")) != NULL) {
				if (*p == '\0')
					errx(1, "Empty `-o' option");
				if (! params->machine->parseopt(params, p))
					exit(1);
			}
			break;

		case 't':
			if (! getfstype(params, optarg, "-t"))
				exit(1);
			break;

		case 'v':
			params->flags |= IB_VERBOSE;
			break;

		case '?':
		default:
			usage();
			/* NOTREACHED */

		}
	}
	argc -= optind;
	argv += optind;

	if (((params->flags & IB_CLEAR) != 0 && argc != 1) ||
	    ((params->flags & IB_CLEAR) == 0 && (argc < 2 || argc > 3)))
		usage();

		/* set missing defaults */
	if (params->machine == NULL) {
		if (uname(&utsname) == -1)
			err(1, "Determine uname");
		if (! getmachine(params, utsname.machine, "uname()"))
			exit(1);
	}

	params->filesystem = argv[0];
	if (params->flags & IB_NOWRITE) {
		op = "only";
		mode = O_RDONLY;
	} else {
		op = "write";
		mode = O_RDWR;
	}
	if ((params->fsfd = open(params->filesystem, mode, 0600)) == -1)
		err(1, "Opening file system `%s' read-%s",
		    params->filesystem, op);
	if (params->fstype != NULL) {
		if (! params->fstype->match(params))
			err(1, "File system `%s' is not of type %s",
			    params->filesystem, params->fstype->name);
	} else {
		params->fstype = &fstypes[0];
		while (params->fstype->name != NULL &&
			! params->fstype->match(params))
			params->fstype++;
		if (params->fstype->name == NULL)
			errx(1, "File system `%s' is of an unknown type",
			    params->filesystem);
	}

	if (argc >= 2) {
		params->stage1 = argv[1];
		if ((params->s1fd = open(params->stage1, O_RDONLY, 0600))
		    == -1)
			err(1, "Opening primary bootstrap `%s'",
			    params->stage1);
	}
	if (argc == 3) {
		params->stage2 = argv[2];
	}
	assert(params->machine != NULL);

	if (params->flags & IB_VERBOSE) {
		printf("File system:         %s\n", params->filesystem);
		printf("Primary bootstrap:   %s\n",
		    (params->flags & IB_CLEAR) ? "(to be cleared)"
		    : params->stage1);
		if (params->stage2 != NULL)
			printf("Secondary bootstrap: %s\n", params->stage2);
	}

	if (params->flags & IB_CLEAR) {
		op = "Clear";
		rv = params->machine->clearboot(params);
	} else {
		op = "Set";
		rv = params->machine->setboot(params);
	}
	if (rv == 0)
		errx(1, "%s bootstrap operation failed", op);

	if (fsync(params->fsfd) == -1)
		err(1, "Synchronising file system `%s'", params->filesystem);
	if (close(params->fsfd) == -1)
		err(1, "Closing file system `%s'", params->filesystem);
	if (argc == 2)
		if (close(params->s1fd) == -1)
			err(1, "Closing primary bootstrap `%s'",
			    params->stage1);

	exit(0);
	/* NOTREACHED */
}

int
parseoptionflag(ib_params *params, const char *option, ib_flags wantflags)
{
	struct {
		const char	*name;
		ib_flags	flag;
	} flags[] = {
		{ "alphasum",	IB_ALPHASUM },
		{ "append",	IB_APPEND },
		{ "sunsum",	IB_SUNSUM },
		{ NULL,		0 },
	};

	int	i;

	assert(params != NULL);
	assert(option != NULL);

	for (i = 0; flags[i].name != NULL; i++) {
		if ((strcmp(flags[i].name, option) == 0) &&
		    (wantflags & flags[i].flag)) {
			params->flags |= flags[i].flag;
			return (1);
		}
	}
	return (0);
}

int
no_parseopt(ib_params *params, const char *option)
{

	assert(params != NULL);
	assert(option != NULL);

		/* all options are unsupported */
	warnx("Unsupported -o option `%s'", option);
	return (0);
}

int
no_setboot(ib_params *params)
{

	assert(params != NULL);

		/* bootstrap installation is not supported */
	warnx("%s: bootstrap installation is not supported",
	    params->machine->name);
	return (0);
}

int
no_clearboot(ib_params *params)
{

	assert(params != NULL);

		/* bootstrap removal is not supported */
	warnx("%s: bootstrap removal is not supported",
	    params->machine->name);
	return (0);
}


static int
getmachine(ib_params *param, const char *mach, const char *provider)
{
	int	i;

	assert(param != NULL);
	assert(mach != NULL);
	assert(provider != NULL);

	for (i = 0; machines[i].name != NULL; i++) {
		if (strcmp(machines[i].name, mach) == 0) {
			param->machine = &machines[i];
			return (1);
		}
	}
	warnx("Invalid machine `%s' from %s", mach, provider);
	warnx("Supported machines are:");
#define MACHS_PER_LINE	10
	for (i = 0; machines[i].name != NULL; i++) {
		fputs((i % MACHS_PER_LINE) ? ", " : "\t", stderr);
		fputs(machines[i].name, stderr);
		if ((i % MACHS_PER_LINE) == (MACHS_PER_LINE - 1))
			fputs("\n", stderr);
	}
	if ((i % MACHS_PER_LINE) != 0)
		fputs("\n", stderr);
	return (0);
}

static int
getfstype(ib_params *param, const char *fstype, const char *provider)
{
	int	i;

	assert(param != NULL);
	assert(fstype != NULL);
	assert(provider != NULL);

	for (i = 0; fstypes[i].name != NULL; i++) {
		if (strcmp(fstypes[i].name, fstype) == 0) {
			param->fstype = &fstypes[i];
			return (1);
		}
	}
	warnx("Invalid file system type `%s' from %s", fstype, provider);
	warnx("Supported file system types are:");
#define FSTYPES_PER_LINE	10
	for (i = 0; fstypes[i].name != NULL; i++) {
		fputs((i % FSTYPES_PER_LINE) ? ", " : "\t", stderr);
		fputs(fstypes[i].name, stderr);
		if ((i % FSTYPES_PER_LINE) == (FSTYPES_PER_LINE - 1))
			fputs("\n", stderr);
	}
	if ((i % FSTYPES_PER_LINE) != 0)
		fputs("\n", stderr);
	return (0);
}

static void
usage(void)
{
	const char	*prog;

	prog = getprogname();
	fprintf(stderr,
"Usage: %s [-nv] [-m machine] [-o options] [-t fstype]\n"
"\t\t   [-b block] filesystem primary [secondary]\n"
"Usage: %s -c [-nv] [-m machine] [-o options] [-t fstype] filesystem\n",
	    prog, prog);
	exit(1);
}
