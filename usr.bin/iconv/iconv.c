/*	$NetBSD: iconv.c,v 1.3 2003/07/23 13:18:59 wiz Exp $	*/

/*-
 * Copyright (c)2003 Citrus Project,
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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: iconv.c,v 1.3 2003/07/23 13:18:59 wiz Exp $");
#endif /* LIBC_SCCS and not lint */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include <unistd.h>
#include <err.h>

static void
usage(void)
{
	fprintf(stderr,
		"usage:\n"
		"\t%s [-cs] -f <from> -t <to> [file ...]\n"
		"\t%s -l\n",
		getprogname(), getprogname());
	exit(1);
}

static void
show_codesets(void)
{
	char **list;
	size_t sz, i;

	if (__iconv_get_list(&list, &sz))
		err(EXIT_FAILURE, "__iconv_get_list()");

	for (i=0; i<sz; i++) {
		printf("%s\n", list[i]);
	}

	__iconv_free_list(list, sz);
}

#define INBUFSIZE 1024
#define OUTBUFSIZE (INBUFSIZE*2)
static void
do_conv(const char *fn, FILE *fp, const char *from, const char *to, int silent,
	int hide_invalid)
{
	char inbuf[INBUFSIZE], outbuf[OUTBUFSIZE], *out;
	const char *in;
	size_t inbytes, outbytes, ret, invalids;
	iconv_t cd;
	u_int32_t flags = 0;

	if (hide_invalid)
		flags |= __ICONV_F_HIDE_INVALID;
	cd = iconv_open(to, from);
	if (cd == (iconv_t)-1)
		err(EXIT_FAILURE, "iconv_open(%s, %s)", to, from);

	invalids = 0;
	while ((inbytes = fread(inbuf, 1, INBUFSIZE, fp)) > 0) {
		in = inbuf;
		while (inbytes>0) {
			out = outbuf;
			outbytes = OUTBUFSIZE;
			ret = __iconv(cd, &in, &inbytes, &out, &outbytes,
				      flags, &invalids);
			if (ret==(size_t)-1 && errno != E2BIG) {
				/*
				 * XXX: iconv(3) is bad interface.
				 *   invalid character count is lost here.
				 *   instead, we just provide __iconv function.
				 */
				if (errno != EINVAL || in == inbuf)
					err(EXIT_FAILURE, "iconv()");

				/* incomplete input character */
				memmove(inbuf, in, inbytes);
				ret = fread(inbuf+inbytes, 1,
					    INBUFSIZE-inbytes, fp);
				if (ret == 0) {
					if (feof(fp))
						errx(EXIT_FAILURE,
						     "iconv(): %s",
						     strerror(EINVAL));
					else
						err(EXIT_FAILURE, "fread()");
				}
				in = inbuf;
				inbytes += ret;
			}
			if (outbytes < OUTBUFSIZE)
				fwrite(outbuf, 1, OUTBUFSIZE-outbytes, stdout);
		}
	}
	/* reset the shift state of the output buffer */
	outbytes = OUTBUFSIZE;
	out = outbuf;
	ret = iconv(cd, NULL, NULL, &out, &outbytes);
	if (ret == -1)
		err(EXIT_FAILURE, "iconv()");
	if (outbytes < OUTBUFSIZE)
		fwrite(outbuf, 1, OUTBUFSIZE-outbytes, stdout);

	if (invalids > 0 && !silent)
		warnx("warning: invalid characters: %lu",
		      (unsigned long)invalids);

	iconv_close(cd);
}

int
main(int argc, char **argv)
{
	int ch, i;
	extern char *optarg;
	extern int optind;
	int opt_l = 0, opt_s = 0, opt_c = 0;
	char *opt_f = NULL, *opt_t = NULL;
	FILE *fp;

	while ((ch=getopt(argc, argv, "cslf:t:")) != EOF) {
		switch (ch) {
		case 'c':
			opt_c = 1;
			break;
		case 's':
			opt_s = 1;
			break;
		case 'l':
			/* list */
			opt_l = 1;
			break;
		case 'f':
			/* from */
			opt_f = strdup(optarg);
			break;
		case 't':
			/* to */
			opt_t = strdup(optarg);
			break;
		default:
			usage();
		}
	}
	argc-=optind;
	argv+=optind;
	if (opt_l) {
		if (argc>0 || opt_s || opt_f != NULL || opt_t != NULL) {
			warnx("%s: -l should be specified solely.",
			      getprogname());
			usage();
		}
		show_codesets();
	} else {
		if (opt_f == NULL || opt_t == NULL)
			usage();

		if (argc == 0)
			do_conv("<stdin>", stdin, opt_f, opt_t, opt_s, opt_c);
		else {
			for (i=0; i<argc; i++) {
				fp = fopen(argv[i], "r");
				if (fp == NULL)
					errx(EXIT_FAILURE, "%s: %s:%s",
					     getprogname(), argv[i],
					     strerror(errno));
				do_conv(argv[i], fp, opt_f, opt_t, opt_s,
					opt_c);
				fclose(fp);
			}
		}
	}

	return EXIT_SUCCESS;
}
