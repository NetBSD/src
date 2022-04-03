/*	$NetBSD: keama.c,v 1.1.1.2 2022/04/03 01:08:42 christos Exp $	*/

/*
 * Copyright(C) 2017-2022 Internet Systems Consortium, Inc.("ISC")
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   PO Box 360
 *   Newmarket, NH 03857 USA
 *   <info@isc.org>
 *   https://www.isc.org/
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: keama.c,v 1.1.1.2 2022/04/03 01:08:42 christos Exp $");

#include <sys/errno.h>
#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "keama.h"

#define KEAMA_USAGE	"Usage: keama [-4|-6] [-D] [-N]" \
			" [-r {perform|fatal|pass}\\n" \
			" [-l hook-library-path]" \
			" [-i input-file] [-o output-file]\n"

static void
usage(const char *sfmt, const char *sarg) {
	if (sfmt != NULL) {
		fprintf(stderr, sfmt, sarg);
		fprintf(stderr, "\n");
	}
	fputs(KEAMA_USAGE, stderr);
	exit(1);
}

enum resolve resolve;
struct parses parses;

int local_family = 0;
char *hook_library_path = NULL;
char *input_file = NULL;
char *output_file = NULL;
FILE *input = NULL;
FILE *output = NULL;
isc_boolean_t use_isc_lifetimes = ISC_FALSE;
isc_boolean_t global_hr = ISC_TRUE;
isc_boolean_t json = ISC_FALSE;

static const char use_noarg[] = "No argument for command: %s";
static const char bad_resolve[] = "Bad -r argument: %s";

int
main(int argc, char **argv) {
	int i, fd;
	char *inbuf = NULL;
	size_t oldsize = 0;
	size_t newsize = 0;
	ssize_t cc;
	struct parse *cfile;
	size_t cnt = 0;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-4") == 0)
			local_family = AF_INET;
		else if (strcmp(argv[i], "-6") == 0)
			local_family = AF_INET6;
		else if (strcmp(argv[i], "-D") == 0)
			use_isc_lifetimes = ISC_TRUE;
		else if (strcmp(argv[i], "-N") == 0)
			global_hr = ISC_FALSE;
		else if (strcmp(argv[i], "-T") == 0)
			json = ISC_TRUE;
		else if (strcmp(argv[i], "-r") == 0) {
			if (++i == argc)
				usage(use_noarg, argv[i -  1]);
			if (strcmp(argv[i], "perform") == 0)
				resolve = perform;
			else if (strcmp(argv[i], "fatal") == 0)
				resolve = fatal;
			else if (strcmp(argv[i], "pass") == 0)
				resolve = pass;
			else
				usage(bad_resolve, argv[i]);
		} else if (strcmp(argv[i], "-l") == 0) {
			if (++i == argc)
				usage(use_noarg, argv[i -  1]);
			hook_library_path = argv[i];
		} else if (strcmp(argv[i], "-i") == 0) {
			if (++i == argc)
				usage(use_noarg, argv[i -  1]);
			input_file = argv[i];
		} else if (strcmp(argv[i], "-o") == 0) {
			if (++i == argc)
				usage(use_noarg, argv[i -  1]);
			output_file = argv[i];
 		} else
			usage("Unknown command: %s", argv[i]);
	}

	if (!json && (local_family == 0))
		usage("address family must be set using %s", "-4 or -6");

	if (input_file == NULL) {
		input_file = "--stdin--";
		fd = fileno(stdin);
		for (;;) {
			if (newsize == 0)
				newsize = 1024;
			else {
				oldsize = newsize;
				newsize *= 4;
			}
			inbuf = (char *)realloc(inbuf, newsize);
			if (inbuf == 0)
				usage("out of memory reading standard "
				      "input: %s", strerror(errno));
			cc = read(fd, inbuf + oldsize, newsize - oldsize);
			if (cc < 0)
				usage("error reading standard input: %s",
				      strerror(errno));
			if (cc + oldsize < newsize) {
				newsize = cc + oldsize;
				break;
			}
		}
	} else {
		fd = open(input_file, O_RDONLY);
		if (fd < 0)
			usage("Cannot open '%s' for reading", input_file);
	}

	if (output_file) {
		output = fopen(output_file, "w");
		if (output == NULL)
			usage("Cannot open '%s' for writing", output_file);
	} else
		output = stdout;

	TAILQ_INIT(&parses);
	cfile = new_parse(fd, inbuf, newsize, input_file, 0);
	assert(cfile != NULL);

	if (json) {
		struct element *elem;

		elem = json_parse(cfile);
		if (elem != NULL) {
			print(output, elem, 0, 0);
			fprintf(output, "\n");
		}
	} else {
		spaces_init();
		options_init();
		cnt = conf_file_parse(cfile);
		if (cfile->stack_top > 0) {
			print(output, cfile->stack[0], 0, 0);
			fprintf(output, "\n");
		}
	}

	end_parse(cfile);

	exit(cnt);
}

void
stackPush(struct parse *pc, struct element *elem)
{
	if (pc->stack_top + 2 >= pc->stack_size) {
		size_t new_size = pc->stack_size + 10;
		size_t amount = new_size * sizeof(struct element *);

		pc->stack = (struct element **)realloc(pc->stack, amount);
		if (pc->stack == NULL)
			parse_error(pc, "can't resize element stack");
		pc->stack_size = new_size;
	}
	pc->stack_top++;
	pc->stack[pc->stack_top] = elem;
}

void
parse_error(struct parse *cfile, const char *fmt, ...)
{
	va_list list;
	char lexbuf[256];
	char mbuf[1024];
	char fbuf[1024];
	unsigned i, lix;

	snprintf(fbuf, sizeof(fbuf), "%s line %d: %s",
		 cfile->tlname, cfile->lexline, fmt);

	va_start(list, fmt);
	vsnprintf(mbuf, sizeof(mbuf), fbuf, list);
	va_end(list);

	lix = 0;
	for (i = 0;
	     cfile->token_line[i] && i < (cfile->lexchar - 1); i++) {
		if (lix < sizeof(lexbuf) - 1)
			lexbuf[lix++] = ' ';
		if (cfile->token_line[i] == '\t') {
			for (; lix < (sizeof lexbuf) - 1 && (lix & 7); lix++)
				lexbuf[lix] = ' ';
		}
	}
	lexbuf[lix] = 0;

	fprintf(stderr, "%s\n%s\n", mbuf, cfile->token_line);
	if (cfile->lexchar < 81)
		fprintf(stderr, "%s^\n", lexbuf);
	exit(-1);
}
