/*	$NetBSD: getextattr.c,v 1.10.8.1 2014/08/10 06:58:04 tls Exp $	*/

/*-
 * Copyright (c) 2002, 2003 Networks Associates Technology, Inc.
 * Copyright (c) 2002 Poul-Henning Kamp.
 * Copyright (c) 1999, 2000, 2001, 2002 Robert N. M. Watson
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Poul-Henning
 * Kamp and Network Associates Laboratories, the Security Research Division
 * of Network Associates, Inc. under DARPA/SPAWAR contract N66001-01-C-8035
 * ("CBOSS"), as part of the DARPA CHATS research program
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * FreeBSD: src/usr.sbin/extattr/rmextattr.c,v 1.6 2003/06/05 04:30:00 rwatson Exp
 */

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/extattr.h>

#include <err.h>
#include <errno.h>
//#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>
#include <fcntl.h>
#include <sys/stat.h>
//#include <util.h>

static enum { EADUNNO, EAGET, EASET, EARM, EALS } what = EADUNNO;

__dead static void
usage(void) 
{

	switch (what) {
	case EAGET:
		fprintf(stderr, "usage: %s [-fhq] [-s | -x | -v style] "
		    "attrnamespace attrname filename ...\n", getprogname());
		exit(1);

	case EASET:
		fprintf(stderr, "usage: %s [-fhnq] "
		    "attrnamespace attrname attrvalue filename ...\n",
		    getprogname());
		fprintf(stderr, "usage: %s [-fhnq] -i attrvalue_file "
		    "attrnamespace attrname filename ...\n",
		    getprogname());
		exit(1);

	case EARM:
		fprintf(stderr, "usage: %s [-fhq] "
		    "attrnamespace attrname filename ...\n", getprogname());
		exit(1);

	case EALS:
		fprintf(stderr, "usage: %s [-fhq] "
		    "attrnamespace filename ...\n", getprogname());
		exit(1);

	case EADUNNO:
	default:
		fprintf(stderr,
		    "usage: (getextattr|lsextattr|rmextattr|setextattr)\n");
		exit (1);
	}
}

static void
mkbuf(char **buf, int *oldlen, int newlen)
{

	if (*oldlen >= newlen)
		return;
	if (*buf != NULL)
		free(*buf);
	*buf = malloc(newlen);
	if (*buf == NULL)
		err(1, "malloc");
	*oldlen = newlen;
	return;
}

static int
parse_flag_vis(const char *opt)
{
	if (strcmp(opt, "default") == 0)
		return 0;
	else if (strcmp(opt, "cstyle") == 0)
		return VIS_CSTYLE;
	else if (strcmp(opt, "octal") == 0)
		return VIS_OCTAL;
	else if (strcmp(opt, "httpstyle") == 0)
		return VIS_HTTPSTYLE;

	/* Convenient aliases */
	else if (strcmp(opt, "vis") == 0)
		return 0;
	else if (strcmp(opt, "c") == 0)
		return VIS_CSTYLE;
	else if (strcmp(opt, "http") == 0)
		return VIS_HTTPSTYLE;
	else
		fprintf(stderr, "%s: invalid -s option \"%s\"", 
			getprogname(), opt);

	return -1;
}

#define HEXDUMP_PRINT(x) ((uint8_t)x >= 32 && (uint8_t)x < 128)  ? x : '.'
static void
hexdump(const char *addr, size_t len)
{
	unsigned int i, j;

	for (i = 0; i < len; i += 16) {
		printf("   %03x   ", i);
		for (j = 0; j < 16; j++) {
			if (i + j >= len)
				printf("   ");
			else
				printf("%02x ", addr[i + j] & 0xff);
		}
		printf("   ");
		for (j = 0; j < 16; j++) {
			if (i + j >= len)
				printf(" ");
			else
				printf("%c", HEXDUMP_PRINT(addr[i + j]));
		}
		printf("\n");
	}
}
#undef HEXDUMP_PRINT

int
main(int argc, char *argv[])
{
	char	*buf, *visbuf;
	const char *p;

	const char *options, *attrname;
	int	 buflen, visbuflen, ch, error, i, arg_counter, attrnamespace,
		 minargc, val_len = 0;

	int	flag_force = 0;
	int	flag_nofollow = 0;
	int	flag_null = 0;
	int	flag_quiet = 0;
	int	flag_vis = -1;
	int	flag_hex = 0;
	char	*filename = NULL;

	options = NULL;
	minargc = 0;
	visbuflen = buflen = 0;
	visbuf = buf = NULL;

	p = getprogname();
	if (strcmp(p, "getextattr") == 0) {
		what = EAGET;
		options = "fhqsxv:";
		minargc = 2;
	} else if (strcmp(p, "setextattr") == 0) {
		what = EASET;
		options = "fhnqi:";
		minargc = 3;
	} else if (strcmp(p, "rmextattr") == 0) {
		what = EARM;
		options = "fhq";
		minargc = 2;
	} else if (strcmp(p, "lsextattr") == 0) {
		what = EALS;
		options = "fhq";
		minargc = 2;
	} else
		usage();

	while ((ch = getopt(argc, argv, options)) != -1) {
		switch (ch) {
		case 'f':
			flag_force = 1;
			break;
		case 'h':
			flag_nofollow = 1;
			break;
		case 'n':
			flag_null = 1;
			break;
		case 'q':
			flag_quiet = 1;
			break;
		case 's':
			flag_vis = VIS_SAFE | VIS_WHITE;
			break;
		case 'v':
			flag_vis = parse_flag_vis(optarg);
			break;
		case 'x':
			flag_hex = 1;
			break;
		case 'i':
			filename = optarg;
			minargc--;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	/*
	 * Check for missing argument.
	 */
	if (argc < minargc)
		usage();

	/*
	 * Normal case "namespace attribute".
	 */
	error = extattr_string_to_namespace(argv[0], &attrnamespace);
	if (error == 0) {
		/*
		 * Namespace was specified, so we need one more argument
		 * for the attribute (except for listing)
		 */
		if ((what != EALS) && (argc < minargc + 1))
			usage();
		argc--; argv++;
	} else {
		/*
		 * The namespace was not valid. Perhaps it was omited.
		 * Try to guess a missing namespace by using
		 * linux layout "namespace.attribute". While
		 * we are here, also test the Linux namespaces.
		 */
		if (strstr(argv[0], "user.") == argv[0]) {
			attrnamespace = EXTATTR_NAMESPACE_USER;
		} else
		if ((strstr(argv[0], "system.") == argv[0]) ||
		    (strstr(argv[0], "trusted.") == argv[0]) ||
		    (strstr(argv[0], "security.") == argv[0])) {
			attrnamespace = EXTATTR_NAMESPACE_SYSTEM;
		} else {
			err(1, "%s", argv[0]);
		}
	}


	if (what != EALS) {
		attrname = argv[0];
		argc--; argv++;
	} else
		attrname = NULL;

	if (what == EASET) {
		/*
		 * Handle -i option, reading value from a file.
		 */
		if (filename != NULL) {
			int fd;
			struct stat st;
			ssize_t readen, remain;

			if ((fd = open(filename, O_RDONLY, 0)) == -1)
				err(1, "%s: cannot open \"%s\"",
				     getprogname(), filename);

			if (fstat(fd, &st) != 0)
				err(1, "%s: cannot stat \"%s\"",
				     getprogname(), filename);

			val_len = st.st_size;
			mkbuf(&buf, &buflen, val_len);

			for (remain = val_len; remain > 0; remain -= readen) {
				if ((readen = read(fd, buf, remain)) == -1)
					err(1, "%s: cannot read \"%s\"",
					    getprogname(), filename);
			}

			(void)close(fd);
		} else {
			val_len = strlen(argv[0]);
			mkbuf(&buf, &buflen, val_len + 1);
			strcpy(buf, argv[0]); /* safe */
			argc--; argv++;
		}
	}

	for (arg_counter = 0; arg_counter < argc; arg_counter++) {
		switch (what) {
		case EARM:
			if (flag_nofollow)
				error = extattr_delete_link(argv[arg_counter],
				    attrnamespace, attrname);
			else
				error = extattr_delete_file(argv[arg_counter],
				    attrnamespace, attrname);
			if (error >= 0)
				continue;
			break;
		case EASET:
			if (flag_nofollow)
				error = extattr_set_link(argv[arg_counter],
				    attrnamespace, attrname, buf,
				    val_len + flag_null);
			else
				error = extattr_set_file(argv[arg_counter],
				    attrnamespace, attrname, buf,
				    val_len + flag_null);
			if (error >= 0)
				continue;
			break;
		case EALS:
			if (flag_nofollow)
				error = extattr_list_link(argv[arg_counter],
				    attrnamespace, NULL, 0);
			else
				error = extattr_list_file(argv[arg_counter],
				    attrnamespace, NULL, 0);
			if (error < 0)
				break;
			mkbuf(&buf, &buflen, error);
			if (flag_nofollow)
				error = extattr_list_link(argv[arg_counter],
				    attrnamespace, buf, buflen);
			else
				error = extattr_list_file(argv[arg_counter],
				    attrnamespace, buf, buflen);
			if (error < 0)
				break;
			if (!flag_quiet)
				printf("%s\t", argv[arg_counter]);
			for (i = 0; i < error; i += buf[i] + 1)
			    printf("%s%*.*s", i ? "\t" : "",
				buf[i], buf[i], buf + i + 1);
			printf("\n");
			continue;
		case EAGET:
			if (flag_nofollow)
				error = extattr_get_link(argv[arg_counter],
				    attrnamespace, attrname, NULL, 0);
			else
				error = extattr_get_file(argv[arg_counter],
				    attrnamespace, attrname, NULL, 0);
			if (error < 0)
				break;
			mkbuf(&buf, &buflen, error);
			if (flag_nofollow)
				error = extattr_get_link(argv[arg_counter],
				    attrnamespace, attrname, buf, buflen);
			else
				error = extattr_get_file(argv[arg_counter],
				    attrnamespace, attrname, buf, buflen);
			if (error < 0)
				break;
			if (!flag_quiet)
				printf("%s\t", argv[arg_counter]);
			
			/*
			 * Check for binary string and terminal output
			 */
#if 0
			for (i = 0; i < error; i++)
				if (!isprint((int)buf[i]))
					err(1, "binary data, use -x flag");
#endif

			if (flag_vis != -1) {
				mkbuf(&visbuf, &visbuflen, error * 4 + 1);
				strvisx(visbuf, buf, error, flag_vis);
				printf("\"%s\"\n", visbuf);
				continue;
			} else if (flag_hex) {
				printf("\n");
				hexdump(buf, error);
				continue;
			} else {
				fwrite(buf, error, 1, stdout);
				printf("\n");
				continue;
			}
		default:
			break;
		}
		if (!flag_quiet) 
			warn("%s: failed", argv[arg_counter]);
		if (flag_force)
			continue;
		return(1);
	}
	return (0);
}
