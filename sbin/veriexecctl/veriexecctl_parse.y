%{
/*	$NetBSD: veriexecctl_parse.y,v 1.19.2.1 2006/12/09 12:02:45 bouyer Exp $	*/

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

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/statvfs.h>
#include <sys/mount.h>

#include <sys/verified_exec.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <err.h>

#include <prop/proplib.h>

#include "veriexecctl.h"

prop_dictionary_t load_params;
static size_t convert(u_char *, u_char *);

%}

%union {
	char *string;
	int intval;
}

%token <string> PATH
%token <string> STRING
%token EOL TOKEN_COMMA

%%

statement	:	/* empty */
		|	statement path type fingerprint flags eol {
	struct stat sb;
	struct veriexec_up *p;
	struct statvfs sf;

	if (phase == 2) {
		phase2_load();
		goto phase_2_end;
	}

	if (stat(dict_gets(load_params, "file"), &sb) == -1) {
		warnx("Line %lu: Can't stat `%s'",
		    (unsigned long)line, dict_gets(load_params, "file"));
		goto phase_2_end;
	}

	/* Only regular files */
	if (!S_ISREG(sb.st_mode)) {
		warnx("Line %lu: %s is not a regular file",
		    (unsigned long)line, dict_gets(load_params, "file"));
		goto phase_2_end;
	}

	if (statvfs(dict_gets(load_params, "file"), &sf) == -1)
		err(1, "Cannot statvfs `%s'", dict_gets(load_params, "file"));

	if ((p = dev_lookup(sf.f_mntonname)) != NULL) {
		uint64_t n;

		prop_dictionary_get_uint64(p->vu_preload, "count", &n);
		n++;
		prop_dictionary_set_uint64(p->vu_preload, "count", n);

		goto phase_2_end;
	}

	if (verbose) {
		(void)printf( " => Adding mount `%s'.\n", sf.f_mntonname);
	}

	dev_add(sf.f_mntonname);

phase_2_end:
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
	if (phase == 2) {
		dict_sets(load_params, "fp-type", $1);
	}
}
		;


fingerprint	:	STRING {
	if (phase == 2) {
		char *fp;
		size_t n;

		fp = malloc(strlen($1) / 2);
		if (fp == NULL)
			err(1, "Fingerprint mem alloc failed");

		n = convert($1, fp);
		if (n == -1) {
			free(fp);
			yyerror("Bad fingerprint");
			YYERROR;
		}

		dict_setd(load_params, "fp", fp, n);
		free(fp);
	}
				
}
	    ;

flags		:	/* empty */
		|	flags_spec
		;

flags_spec	:	flag_spec
		|	flags_spec TOKEN_COMMA flag_spec
		;

flag_spec	:	STRING {
	if (phase == 2) {
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
			yyerror("Bad flag");
			YYERROR;
		}

		prop_dictionary_set_uint8(load_params, "entry-type", t);
	}

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
convert(u_char *fp, u_char *out)
{
	size_t i, count;
	u_char value;

	count = strlen(fp);

	/*
	 * if there are not an even number of hex digits then there is
	 * not an integral number of bytes in the fingerprint.
	 */
	if ((count % 2) != 0)
		return -1;
	
	count /= 2;

#define cvt(cv) \
	if (isdigit(cv)) \
		value += (cv) - '0'; \
	else if (isxdigit(cv)) \
		value += 10 + tolower(cv) - 'a'; \
	else \
		return -1

	for (i = 0; i < count; i++) {
		value = 0;
		cvt(fp[2 * i]);
		value <<= 4;
		cvt(fp[2 * i + 1]);
		out[i] = value;
	}

	return count;
}
