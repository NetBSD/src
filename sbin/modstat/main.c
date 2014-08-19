/*	$NetBSD: main.c,v 1.15.2.1 2014/08/20 00:02:26 tls Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
#ifndef lint
__RCSID("$NetBSD: main.c,v 1.15.2.1 2014/08/20 00:02:26 tls Exp $");
#endif /* !lint */

#include <sys/module.h>
#include <sys/param.h>
#include <sys/sysctl.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "prog_ops.h"

int	main(int, char **);
static void	usage(void) __dead;
static int	modstatcmp(const void *, const void *);

static const char *classes[] = {
	"any",
	"misc",
	"vfs",
	"driver",
	"exec",
	"secmodel"
};
const unsigned int class_max = __arraycount(classes);

static const char *sources[] = {
	"builtin",
	"boot",
	"filesys",
};
const unsigned int source_max = __arraycount(sources);

int
main(int argc, char **argv)
{
	struct iovec iov;
	modstat_t *ms;
	size_t len;
	const char *name;
	char sbuf[32];
	int ch, rc, modauto = 1;
	size_t maxnamelen = 16, i, modautolen;
	char loadable = '\0';

	name = NULL;

	while ((ch = getopt(argc, argv, "Aaen:")) != -1) {
		switch (ch) {
		case 'A':			/* FALLTHROUGH */
		case 'a':			/* FALLTHROUGH */
		case 'e':
			loadable = (char)ch;
			break;
		case 'n':
			name = optarg;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;
	if (argc == 1 && name == NULL)
		name = argv[0];
	else if (argc != 0)
		usage();

	if (prog_init && prog_init() == -1)
		err(1, "prog init failed");

	if (loadable == 'A' || loadable == 'a') {
		if (prog_modctl(MODCTL_EXISTS, (void *)(uintptr_t)1)) {
			switch (errno) {
			case ENOSYS:
				errx(EXIT_FAILURE, "The kernel was compiled "
				    "without options MODULAR.");
				break;
			case EPERM:
				errx(EXIT_FAILURE, "Modules can not be "
				    "autoloaded right now.");
				break;
			default:
				err(EXIT_FAILURE, "modctl_exists for autoload");
				break;
			}
		} else {
			if (loadable == 'A') {
				modautolen = sizeof(modauto);
				rc = sysctlbyname("kern.module.autoload",
				    &modauto, &modautolen, NULL, 0);
				if (rc != 0) {
					err(EXIT_FAILURE, "sysctl "
					    "kern.module.autoload failed.");
				}
			}
			errx(EXIT_SUCCESS, "Modules can be autoloaded%s.",
			modauto ? "" : ", but kern.module.autoload = 0");
		}
	}

	if (loadable == 'e') {
		if (prog_modctl(MODCTL_EXISTS, (void *)(uintptr_t)0)) {
			switch (errno) {
			case ENOSYS:
				errx(EXIT_FAILURE, "The kernel was compiled "
				    "without options MODULAR.");
				break;
			case EPERM:
				errx(EXIT_FAILURE, "You are not allowed to "
				    "load modules right now.");
				break;
			default:
				err(EXIT_FAILURE, "modctl_exists for autoload");
				break;
			}
		} else {
			errx(EXIT_SUCCESS, "You can load modules.");
		}
	}

	for (len = 8192;;) {
		iov.iov_base = malloc(len);
		iov.iov_len = len;
		if (prog_modctl(MODCTL_STAT, &iov)) {
			err(EXIT_FAILURE, "modctl(MODCTL_STAT)");
		}
		if (len >= iov.iov_len) {
			break;
		}
		free(iov.iov_base);
		len = iov.iov_len;
	}

	len = iov.iov_len / sizeof(modstat_t);
	qsort(iov.iov_base, len, sizeof(modstat_t), modstatcmp);
	for (i = 0, ms = iov.iov_base; i < len; i++, ms++) {
		size_t namelen = strlen(ms->ms_name);
		if (maxnamelen < namelen)
			maxnamelen = namelen;
	}
	printf("%-*s %-10s %-10s %-5s %-16s %-8s %s \n",
	    (int)maxnamelen, "NAME", "CLASS", "SOURCE", "REFS", "ADDRESS",
	    "SIZE", "REQUIRES");
	for (ms = iov.iov_base; len != 0; ms++, len--) {
		const char *class;
		const char *source;

		if (name != NULL && strcmp(ms->ms_name, name) != 0) {
			continue;
		}
		if (ms->ms_required[0] == '\0') {
			ms->ms_required[0] = '-';
			ms->ms_required[1] = '\0';
		}
		if (ms->ms_size == 0) {
			sbuf[0] = '-';
			sbuf[1] = '\0';
		} else {
			snprintf(sbuf, sizeof(sbuf), "%u", ms->ms_size);
		}
		if (ms->ms_class <= class_max)
			class = classes[ms->ms_class];
		else
			class = "UNKNOWN";
		if (ms->ms_source < source_max)
			source = sources[ms->ms_source];
		else
			source = "UNKNOWN";

		printf("%-*s %-10s %-10s %-5d %-16" PRIx64 " %-8s %s\n",
		    (int)maxnamelen, ms->ms_name, class, source, ms->ms_refcnt,
		    ms->ms_addr, sbuf, ms->ms_required);
	}

	exit(EXIT_SUCCESS);
}

static void
usage(void)
{

	(void)fprintf(stderr, "Usage: %s [-Aaen] [name]\n", getprogname());
	exit(EXIT_FAILURE);
}

static int
modstatcmp(const void *a, const void *b)
{
	const modstat_t *msa, *msb;

	msa = a;
	msb = b;

	return strcmp(msa->ms_name, msb->ms_name);
}
