/*	$NetBSD: wsconsctl.c,v 1.18.60.1 2019/06/10 22:05:37 christos Exp $ */

/*-
 * Copyright (c) 1998, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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
#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "wsconsctl.h"

#define PATH_KEYBOARD		"/dev/wskbd"
#define PATH_MOUSE		"/dev/wsmouse"
#define PATH_DISPLAY		"/dev/ttyE0"

extern struct field keyboard_field_tab[];
extern struct field mouse_field_tab[];
extern struct field display_field_tab[];
extern int keyboard_field_tab_len;
extern int mouse_field_tab_len;
extern int display_field_tab_len;

static void usage(const char *) __dead;

static void
usage(const char *msg)
{
	const char *progname = getprogname();

	if (msg != NULL)
		(void)fprintf(stderr, "%s: %s\n\n", progname, msg);

	(void)fprintf(stderr,
	    "Usage: %s [-kmd] [-f file] [-n] name ...\n"
	    " -or-  %s [-kmd] [-f file] [-n] -w name=value ...\n"
	    " -or-  %s [-kmd] [-f file] [-n] -w name+=value ...\n"
	    " -or-  %s [-kmd] [-f file] [-n] -a\n",
	    progname, progname, progname, progname);

	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	int i, ch, fd;
	int aflag, dflag, kflag, mflag, wflag;
	char *p;
	const char *sep, *file;
	struct field *f, *field_tab;
	int do_merge, field_tab_len;
	void (*getval)(int);
	void (*putval)(int);

	aflag = 0;
	dflag = 0;
	kflag = 0;
	mflag = 0;
	wflag = 0;
	file = NULL;
	sep = "=";
	field_tab = NULL;
	field_tab_len = 0;
	getval = NULL;
	putval = NULL;

	while ((ch = getopt(argc, argv, "adf:kmnw")) != -1) {
		switch(ch) {
		case 'a':
			aflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'f':
			file = optarg;
			break;
		case 'k':
			kflag = 1;
			break;
		case 'm':
			mflag = 1;
			break;
		case 'n':
			sep = NULL;
			break;
		case 'w':
			wflag = 1;
			break;
		case '?':
		default:
			usage(NULL);
		}
	}

	argc -= optind;
	argv += optind;

	if (dflag + kflag + mflag == 0)
		kflag = 1;
	if (dflag + kflag + mflag > 1)
		usage("only one of -k, -d or -m may be given");
	if (argc > 0 && aflag != 0)
		usage("excess arguments after -a");
	if (aflag != 0 && wflag != 0)
		usage("only one of -a or -w may be given");

	if (kflag) {
		if (file == NULL)
			file = PATH_KEYBOARD;
		field_tab = keyboard_field_tab;
		field_tab_len = keyboard_field_tab_len;
		getval = keyboard_get_values;
		putval = keyboard_put_values;
	} else if (mflag) {
		if (file == NULL)
			file = PATH_MOUSE;
		field_tab = mouse_field_tab;
		field_tab_len = mouse_field_tab_len;
		getval = mouse_get_values;
		putval = mouse_put_values;
	} else if (dflag) {
		if (file == NULL)
			file = PATH_DISPLAY;
		field_tab = display_field_tab;
		field_tab_len = display_field_tab_len;
		getval = display_get_values;
		putval = display_put_values;
	}

	field_setup(field_tab, field_tab_len);

	fd = open(file, O_WRONLY);
	if (fd < 0)
		fd = open(file, O_RDONLY);
	if (fd < 0)
		err(EXIT_FAILURE, "%s", file);

	if (aflag != 0) {
		for (i = 0; i < field_tab_len; i++)
			if ((field_tab[i].flags & (FLG_NOAUTO|FLG_WRONLY)) == 0)
				field_tab[i].flags |= FLG_GET;
		(*getval)(fd);
		for (i = 0; i < field_tab_len; i++)
			if (field_tab[i].flags & FLG_NOAUTO)
				warnx("\"%s\" not shown with -a; use \"%s %s\""
				    " to view.",
				    field_tab[i].name,
				    getprogname(), field_tab[i].name);
			else if (field_tab[i].flags & FLG_GET &&
				 !(field_tab[i].flags & FLG_DISABLED))
				pr_field(field_tab + i, sep);
	} else if (argc > 0) {
		if (wflag != 0) {
			for (i = 0; i < argc; i++) {
				p = strchr(argv[i], '=');
				if (p == NULL)
					errx(EXIT_FAILURE, "'=' not found");
				if (p > argv[i] && *(p - 1) == '+') {
					*(p - 1) = '\0';
					do_merge = 1;
				} else
					do_merge = 0;
				*p++ = '\0';
				f = field_by_name(argv[i]);
				if ((f->flags & FLG_RDONLY) != 0)
					errx(EXIT_FAILURE, "%s: read only",
					    argv[i]);
				if (do_merge) {
					if ((f->flags & FLG_MODIFY) == 0)
						errx(EXIT_FAILURE,
						    "%s: can only be set",
						    argv[i]);
					f->flags |= FLG_GET;
					(*getval)(fd);
					f->flags &= ~FLG_GET;
				}
				rd_field(f, p, do_merge);
				f->flags |= FLG_SET;
				if (do_merge)
					f->flags |= FLG_MODIFIED;
				(*putval)(fd);
				f->flags &= ~(FLG_SET | FLG_MODIFIED);
			}
		} else {
			for (i = 0; i < argc; i++) {
				f = field_by_name(argv[i]);
				if ((f->flags & FLG_WRONLY) != 0)
					errx(EXIT_FAILURE, "%s: write only",
					    argv[i]);
				f->flags |= FLG_GET;
			}
			(*getval)(fd);
			for (i = 0; i < field_tab_len; i++) {
				if (field_tab[i].flags & FLG_DISABLED)
					errx(EXIT_FAILURE,
					    "%s: no kernel support",
					    field_tab[i].name);
				if (field_tab[i].flags & FLG_GET)
					pr_field(field_tab + i, sep);
			}
		}
	} else {
		close(fd);
		usage(NULL);
	}

	close(fd);

	return EXIT_SUCCESS;
}
