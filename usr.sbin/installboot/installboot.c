/*	$NetBSD: installboot.c,v 1.13 2003/05/08 20:33:44 petrov Exp $	*/

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
__RCSID("$NetBSD: installboot.c,v 1.13 2003/05/08 20:33:44 petrov Exp $");
#endif	/* !__lint */

#include <sys/utsname.h>

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "installboot.h"

int		main(int, char *[]);
static	void	getmachine(ib_params *, const char *, const char *);
static	void	getfstype(ib_params *, const char *, const char *);
static	void	parseoptions(ib_params *, const char *);
static	void	usage(void);
static	void	options_usage(void);
static	void	machine_usage(void);
static	void	fstype_usage(void);

static	ib_params	installboot_params;

#define OFFSET(field)    offsetof(ib_params, field)
const struct option {
	const char	*name;		/* Name of option */
	ib_flags	flag;		/* Corresponding IB_xxx flag */
	enum {				/* Type of option value... */
		OPT_BOOL,		/* no value */
		OPT_INT,		/* numeric value */
		OPT_WORD,		/* space/tab/, terminated */
		OPT_STRING		/* null terminated */
	}		type;
	int		offset;		/* of field in ib_params */
} options[] = {
	{ "alphasum",	IB_ALPHASUM,	OPT_BOOL },
	{ "append",	IB_APPEND,	OPT_BOOL },
	{ "command",	IB_COMMAND,	OPT_STRING,	OFFSET(command) },
	{ "console",	IB_CONSOLE,	OPT_WORD,	OFFSET(console) },
	{ "password",	IB_PASSWORD,	OPT_WORD,	OFFSET(password) },
	{ "resetvideo",	IB_RESETVIDEO,	OPT_BOOL },
	{ "speed",	IB_CONSPEED,	OPT_INT,	OFFSET(conspeed) },
	{ "sunsum",	IB_SUNSUM,	OPT_BOOL },
	{ "timeout",	IB_TIMEOUT,	OPT_INT,	OFFSET(timeout) },
	{ NULL },
};
#undef OFFSET
#define OPTION(params, type, opt) (*(type *)((char *)(params) + (opt)->offset))

int
main(int argc, char *argv[])
{
	struct utsname	utsname;
	ib_params	*params;
	unsigned long	lval;
	int		ch, rv, mode;
	char 		*p;
	const char	*op;
	ib_flags	unsupported_flags;

	setprogname(argv[0]);
	params = &installboot_params;
	memset(params, 0, sizeof(*params));
	params->fsfd = -1;
	params->s1fd = -1;
	if ((p = getenv("MACHINE")) != NULL)
		getmachine(params, p, "$MACHINE");

	while ((ch = getopt(argc, argv, "b:B:cm:no:t:v")) != -1) {
		switch (ch) {

		case 'b':
		case 'B':
			if (*optarg == '\0')
				goto badblock;
			lval = strtoul(optarg, &p, 0);
			if (lval > UINT32_MAX || *p != '\0') {
 badblock:
				errx(1, "Invalid block number `%s'", optarg);
			}
			if (ch == 'b') {
				params->s1start = (uint32_t)lval;
				params->flags |= IB_STAGE1START;
			} else {
				params->s2start = (uint32_t)lval;
				params->flags |= IB_STAGE2START;
			}
			break;

		case 'c':
			params->flags |= IB_CLEAR;
			break;

		case 'm':
			getmachine(params, optarg, "-m");
			break;

		case 'n':
			params->flags |= IB_NOWRITE;
			break;

		case 'o':
			parseoptions(params, optarg);
			break;

		case 't':
			getfstype(params, optarg, "-t");
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
		getmachine(params, utsname.machine, "uname()");
	}

	/* Check that options are supported by this system */
	unsupported_flags = params->flags & ~params->machine->valid_flags;
	unsupported_flags &= ~(IB_VERBOSE | IB_NOWRITE |IB_CLEAR);
	if (unsupported_flags != 0) {
		int ndx;
		for (ndx = 0; options[ndx].name != NULL; ndx++) {
			if (unsupported_flags & options[ndx].flag)
				warnx("`-o %s' is not supported for %s",
				    options[ndx].name, params->machine->name);
		}
		if (unsupported_flags & IB_STAGE1START)
			warnx("`-b bno' is not supported for %s",
			    params->machine->name);
		if (unsupported_flags & IB_STAGE2START)
			warnx("`-B bno' is not supported for %s",
			    params->machine->name);
		exit(1);
	}
	/* and some illegal combinations */
	if (params->flags & IB_STAGE1START && params->flags & IB_APPEND) {
		warnx("Can't use `-b bno' with `-o append'");
		exit(1);
	}
	if (params->flags & IB_CLEAR &&
	    params->flags & (IB_STAGE1START | IB_STAGE2START | IB_APPEND)) {
		warnx("Can't use `-b bno', `-B bno' or `-o append' with `-c'");
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
	if (fstat(params->fsfd, &params->fsstat) == -1)
		err(1, "Examining file system `%s'", params->filesystem);
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
		if (fstat(params->s1fd, &params->s1stat) == -1)
			err(1, "Examining primary bootstrap `%s'",
			    params->stage1);
		if (!S_ISREG(params->s1stat.st_mode))
			err(1, "`%s' must be a regular file", params->stage1);
	}
	if (argc == 3) {
		params->stage2 = argv[2];
	}
	assert(params->machine != NULL);

	if (params->flags & IB_VERBOSE) {
		printf("File system:         %s\n", params->filesystem);
		printf("File system type:    %s (blocksize %u, needswap %d)\n",
		    params->fstype->name,
		    params->fstype->blocksize, params->fstype->needswap);
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

	if (S_ISREG(params->fsstat.st_mode)) {
		if (fsync(params->fsfd) == -1)
			err(1, "Synchronising file system `%s'",
			    params->filesystem);
	} else {
		/* Sync filesystems (to clean in-memory superblock?) */
		sync();
	}
	if (close(params->fsfd) == -1)
		err(1, "Closing file system `%s'", params->filesystem);
	if (argc == 2)
		if (close(params->s1fd) == -1)
			err(1, "Closing primary bootstrap `%s'",
			    params->stage1);

	exit(0);
	/* NOTREACHED */
}

static void
parseoptions(ib_params *params, const char *option)
{
	char *cp;
	const struct option *opt;
	int len;
	unsigned long val;

	assert(params != NULL);
	assert(option != NULL);

	for (;; option += len) {
		option += strspn(option, ", \t");
		if (*option == 0)
			return;
		len = strcspn(option, "=,");
		for (opt = options; opt->name != NULL; opt++) {
			if (memcmp(option, opt->name, len) == 0
			    && opt->name[len] == 0)
				break;
		}
		if (opt->name == NULL) {
			len = strcspn(option, ",");
			warnx("Unknown option `-o %.*s'", len, option);
			break;
		}
		params->flags |= opt->flag;
		if (opt->type == OPT_BOOL) {
			if (option[len] != '=')
				continue;
			warnx("Option `%s' must not have a value", opt->name);
			break;
		}
		if (option[len] != '=') {
			warnx("Option `%s' must have a value", opt->name);
			break;
		}
		option += len + 1;
		len = strcspn(option, ",");
		switch (opt->type) {
		case OPT_STRING:
			len = strlen(option);
			/* FALLTHROUGH */
		case OPT_WORD:
			cp = strdup(option);
			if (cp == NULL)
				err(1, "strdup");
			cp[len] = 0;
			OPTION(params, char *, opt) = cp;
			continue;
		case OPT_INT:
			val = strtoul(option, &cp, 0);
			if (cp > option + len || (*cp != 0 && *cp != ','))
				break;
			OPTION(params, int, opt) = val;
			if (OPTION(params, int, opt) != val)
				/* value got truncated on int convertion */
				break;
			continue;
		default:
			errx(1, "Internal error: option `%s' has invalid type %d",
				opt->name, opt->type);
		}
		warnx("Invalid option value `%s=%.*s'", opt->name, len, option);
		break;
	}
	options_usage();
	exit(1);
}

static void
options_usage(void)
{
	int ndx;
	const char *pfx;

	warnx("Valid options are:");
	pfx = "\t";
	for (ndx = 0; options[ndx].name != 0; ndx++) {
		fprintf(stderr, "%s%s", pfx, options[ndx].name);
		switch (options[ndx].type) {
		case OPT_INT:
			fprintf(stderr, "=number");
			break;
		case OPT_WORD:
			fprintf(stderr, "=word");
			break;
		case OPT_STRING:
			fprintf(stderr, "=string");
			break;
		default:
			break;
		}
		if ((ndx % 5) == 4)
			pfx = ",\n\t";
		else
			pfx = ", ";
	}
	fprintf(stderr, "\n");
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


static void
getmachine(ib_params *param, const char *mach, const char *provider)
{
	int	i;

	assert(param != NULL);
	assert(mach != NULL);
	assert(provider != NULL);

	for (i = 0; machines[i].name != NULL; i++) {
		if (strcmp(machines[i].name, mach) == 0) {
			param->machine = &machines[i];
			return;
		}
	}
	warnx("Invalid machine `%s' from %s", mach, provider);
	machine_usage();
	exit(1);
}

static void
machine_usage(void)
{
	const char *prefix;
	int	i;

	warnx("Supported machines are:");
#define MACHS_PER_LINE	9
	prefix="";
	for (i = 0; machines[i].name != NULL; i++) {
		if (i == 0)
			prefix="\t";
		else if (i % MACHS_PER_LINE)
			prefix=", ";
		else
			prefix=",\n\t";
		fprintf(stderr, "%s%s", prefix, machines[i].name);
	}
	fputs("\n", stderr);
}

static void
getfstype(ib_params *param, const char *fstype, const char *provider)
{
	int i;

	assert(param != NULL);
	assert(fstype != NULL);
	assert(provider != NULL);

	for (i = 0; fstypes[i].name != NULL; i++) {
		if (strcmp(fstypes[i].name, fstype) == 0) {
			param->fstype = &fstypes[i];
			return;
		}
	}
	warnx("Invalid file system type `%s' from %s", fstype, provider);
	fstype_usage();
	exit(1);
}

static void
fstype_usage(void)
{
	const char *prefix;
	int	i;

	warnx("Supported file system types are:");
#define FSTYPES_PER_LINE	9
	prefix="";
	for (i = 0; fstypes[i].name != NULL; i++) {
		if (i == 0)
			prefix="\t";
		else if (i % FSTYPES_PER_LINE)
			prefix=", ";
		else
			prefix=",\n\t";
		fprintf(stderr, "%s%s", prefix, fstypes[i].name);
	}
	fputs("\n", stderr);
}

static void
usage(void)
{
	const char	*prog;

	prog = getprogname();
	fprintf(stderr,
"Usage: %s [-nv] [-m machine] [-o options] [-t fstype]\n"
"\t\t   [-b s1start] [-B s2start] filesystem primary [secondary]\n"
"Usage: %s -c [-nv] [-m machine] [-o options] [-t fstype] filesystem\n",
	    prog, prog);
	machine_usage();
	fstype_usage();
	options_usage();
	exit(1);
}
