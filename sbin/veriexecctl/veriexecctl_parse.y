%{
/*	$NetBSD: veriexecctl_parse.y,v 1.25.4.1 2009/05/13 19:19:07 jym Exp $	*/

/*-
 * Copyright 2005 Elad Efrat <elad@NetBSD.org>
 * Copyright 2005 Brett Lymn <blymn@netbsd.org>
 *
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#include <sys/stat.h>
#include <sys/verified_exec.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <stdbool.h>
#include <prop/proplib.h>

#include "veriexecctl.h"

extern int yylex(void);
extern size_t line;
extern int verbose, error;

bool keep_filename = false, eval_on_load = false;
prop_dictionary_t load_params;

%}

%union {
	char *string;
}

%token <string> PATH
%token <string> STRING
%token EOL TOKEN_COMMA

%%

statement	:	/* empty */
		|	statement path type fingerprint flags eol {
	extern int gfd;
	struct stat sb;

	if (stat(dict_gets(load_params, "file"), &sb) == -1) {
		if (verbose)
			warnx("Line %zu: Can't stat `%s'", line,
			    dict_gets(load_params, "file"));
		error = EXIT_FAILURE;
		goto skip;
	}

	/* Only regular files */
	if (!S_ISREG(sb.st_mode)) {
		if (verbose)
			warnx("Line %zu: %s is not a regular file", line,
			    dict_gets(load_params, "file"));
		error = EXIT_FAILURE;
		goto skip;
	}

	if (verbose) {
		(void)printf( "Adding file `%s'.\n",
		    dict_gets(load_params, "file"));
	}

	prop_dictionary_set(load_params, "keep-filename",
	    prop_bool_create(keep_filename));

	prop_dictionary_set(load_params, "eval-on-load",
	    prop_bool_create(eval_on_load));

	if (prop_dictionary_send_ioctl(load_params, gfd, VERIEXEC_LOAD) != 0) {
		if (verbose)
			warn("Cannot load params from `%s'",
			    dict_gets(load_params, "file"));
		error = EXIT_FAILURE;
	}

 skip:
	prop_object_release(load_params);
	load_params = NULL;
}
		|	statement eol
		|	statement error eol {
	yyerrok;
}
		;

path		:	PATH {
	if (load_params == NULL)
		load_params = prop_dictionary_create();

	dict_sets(load_params, "file", $1);
}
		;

type		:	STRING {
	dict_sets(load_params, "fp-type", $1);
}
		;


fingerprint	:	STRING {
	u_char *fp;
	size_t n;

	fp = malloc(strlen($1) / 2);
	if (fp == NULL)
		err(1, "Cannot allocate memory for fingerprint");

	n = convert($1, fp);
	if (n == (size_t)-1) {
		free(fp);
		if (verbose)
			warnx("Bad fingerprint `%s' in line %zu", $1, line);
		error = EXIT_FAILURE;
		YYERROR;
	}

	dict_setd(load_params, "fp", fp, n);
	free(fp);
}
	    ;

flags		:	/* empty */
		|	flags_spec
		;

flags_spec	:	flag_spec
		|	flags_spec TOKEN_COMMA flag_spec
		;

flag_spec	:	STRING {
	uint8_t t = 0;

	prop_dictionary_get_uint8(load_params, "entry-type", &t);

	if (strcasecmp($1, "direct") == 0) {
		t |= VERIEXEC_DIRECT;
	} else if (strcasecmp($1, "indirect") == 0) {
		t |= VERIEXEC_INDIRECT;
	} else if (strcasecmp($1, "file") == 0) {
		t |= VERIEXEC_FILE;
	} else if (strcasecmp($1, "program") == 0) {
		t |= VERIEXEC_DIRECT;
	} else if (strcasecmp($1, "interpreter") == 0) {
		t |= VERIEXEC_INDIRECT;
	} else if (strcasecmp($1, "script") == 0) {
		t |= (VERIEXEC_FILE | VERIEXEC_DIRECT);
	} else if (strcasecmp($1, "library") == 0) {
		t |= (VERIEXEC_FILE | VERIEXEC_INDIRECT);
	} else if (strcasecmp($1, "untrusted") == 0) {
		t |= VERIEXEC_UNTRUSTED;
	} else {
		if (verbose)
			warnx("Bad flag `%s' in line %zu", $1, line);
		error = EXIT_FAILURE;
		YYERROR;
	}

	prop_dictionary_set_uint8(load_params, "entry-type", t);
}
		;

eol		:	EOL
		;

%%

/*
 * Takes the hexadecimal string pointed to by "fp" and converts it to a
 * "count" byte binary number which is stored in the array pointed to
 * by "out".  Returns the number of bytes converted or -1 if the conversion
 * fails.
 */
static size_t
convert(char *fp, u_char *out)
{
	size_t i, count;
	u_char value;

	count = strlen(fp);

	/*
	 * if there are not an even number of hex digits then there is
	 * not an integral number of bytes in the fingerprint.
	 */
	if ((count % 2) != 0)
		return (size_t)-1;

	count /= 2;

#define cvt(cv) \
	if (isdigit((unsigned char) cv)) \
		value += (cv) - '0'; \
	else if (isxdigit((unsigned char) cv)) \
		value += 10 + tolower((unsigned char) cv) - 'a'; \
	else \
		return (size_t)-1

	for (i = 0; i < count; i++) {
		value = 0;
		cvt(fp[2 * i]);
		value <<= 4;
		cvt(fp[2 * i + 1]);
		out[i] = value;
	}

	return count;
}

static void
yyerror(const char *msg)
{
	if (verbose)
		warnx("%s in line %zu", msg, line);
}
