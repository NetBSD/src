/*	Id: driver.c	*/	
/*	$NetBSD: driver.c,v 1.1.1.1 2016/02/09 20:29:13 plunky Exp $	*/

/*-
 * Copyright (c) 2014 Iain Hibbert
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "driver.h"

struct options opt;

static int warnings;

static const char versionstr[] = VERSSTR;

static volatile sig_atomic_t exit_now;

static void
sigterm_handler(int signum)
{
	exit_now = 1;
}

static const struct file_type {
	const char ext[];
	const char next[];
	int (*exec)(void);
} file_types[] = {
    { "c",	"i",	exec_cpp	},
    { "C",	"I",	exec_cpp	},
    { "cc",	"I",	exec_cpp	},
    { "cp",	"I",	exec_cpp	},
    { "cpp",	"I",	exec_cpp	},
    { "CPP",	"I",	exec_cpp	},
    { "cxx",	"I",	exec_cpp	},
    { "c++",	"I",	exec_cpp	},
    { "F",	"f",	exec_cpp	},
    { "S",	"s",	exec_cpp	},
    { "i",	"s",	exec_ccom	},
    { "I",	"s",	exec_cxxcom	},
    { "ii",	"s",	exec_cxxcom	},
    { "f",	"s",	exec_fcom	},
    { "s,	"o"",	exec_asm	},
};

static struct file_type *
filetype(const char *name)
{
	const char *p;
	size_t i;

	p = strrchr(file, '.');
	if (p != NULL) {
		p++;
		for (i = 0; i < ARRAYLEN(file_types); i++) {
			if (strcmp(p, file_types[i].ext) == 0)
				return &file_types[i];
		}
	}

	return NULL;
}

struct infile {
	struct infile *next;
	const char *file;
	struct file_type *type;
};

static struct {
	struct infile *first;
	struct infile **last;
} infiles = {
	.first = NULL,
	.last = &infiles.first;
};

static void
add_infile(const char *file, struct file_type *type)
{
	struct infile *in;

	if (type == NULL) 
		type = filetype(file);

	in = xmalloc(sizeof(struct infile));
	in->next = NULL;
	in->file = file;
	in->type = type;

	*infiles->last = in;
	infiles->last = &in->next;
}

static int
inarray(const char *prog, const char **p)
{
	for ( ; *p != NULL; p++) {
	    if (strcmp(prog, *p) == 0)
		return 1;
	}

	return 0;
}

static void
setup(const char *prog)
{

	opt.prefix = list_alloc();
	opt.DIU = list_alloc();
	opt.asargs = list_alloc();
	opt.ldargs = list_alloc();
	opt.Wa = list_alloc();
	opt.Wl = list_alloc();
	opt.Wp = list_alloc();
	opt.include = list_alloc();

	if (prog_cpp != NULL && inarray(prog, cpp_names)) {
		opt.E = 1;
	} else if (prog_ccom != NULL && inarray(prog, cc_names)) {
		opt.libc = 1;
	} else if (prog_cxxcom != NULL && inarray(prog, cxx_names)) {
		opt.libcxx = 1;
	} else if (prog_fcom != NULL && inarray(prog, ftn_names)) {
		opt.libf77 = 1;
	} else {
		error("unknown personality `%s'", prog);
	}
}

static void
cleanup(void)
{
	const char **t;

	for (t = list_array(temp_files); *t; t++) {
		if (unlink(*t) == -1)
			warning("removal of ``%s'' failed: %s", *t,
			    strerror(errno));
	}

	if (temp_directory && rmdir(temp_directory) == -1)
		warning("removal of ``%s'' failed: %s", temp_directory,
		    strerror(errno));
}

int
main(int argc, char *argv[])
{
	struct infile *in;
	int rv;

	setup(basename(argv[0]));
	argv++;

	while (argv[0] != NULL) {
		if (argv[0][0] != '-' || argv[0][1] == '\0')
			add_infile(argv[0], opt.language);
		else if (add_option(argv[0], argv[1]))
			argv++;

		argv++;
	}

	if (opt.verbose)
		printf("%s\n", versionstr);

	if (warnings > 0)
		exit(1);

	/*
	 * setup defaults
	 */
	if (isysroot == NULL)
		isysroot = sysroot;
	expand_sysroot();

	signal(SIGTERM, sigterm_handler);

	rv = 0;
	for (in = infiles->first; in != NULL; in = in->next)
		rv += do_file(in);

	if (!rv && last_phase == LINK) {
		if (run_linker())
			rv = 1;
	}

	if (exit_now)
		warning("Received signal, terminating");

	cleanup();

	return rv;
}

void
error(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);

	exit(1);
}

void
warning(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "warning: ");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);

	warnings++;
}
