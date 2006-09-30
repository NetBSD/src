%{
/*	$NetBSD: veriexecctl_parse.y,v 1.17 2006/09/30 10:56:31 elad Exp $	*/

/*-
 * Copyright 2005 Elad Efrat <elad@bsd.org.il>
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

#include "veriexecctl.h"

struct veriexec_params params;
static int convert(u_char *, u_char *);

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

	if (stat(params.file, &sb) == -1) {
		warnx("Line %lu: Can't stat `%s'",
		    (unsigned long)line, params.file);
		goto phase_2_end;
	}

	/* Only regular files */
	if (!S_ISREG(sb.st_mode)) {
		warnx("Line %lu: %s is not a regular file",
		    (unsigned long)line, params.file);
		goto phase_2_end;
	}

	if (statvfs(params.file, &sf) == -1)
		err(1, "Cannot statvfs `%s'", params.file);

	if ((p = dev_lookup(sf.f_mntonname)) != NULL) {
	    (p->vu_param.hash_size)++;
	    goto phase_2_end;
	}

	if (verbose) {
		(void)printf( " => Adding mount `%s'.\n", sf.f_mntonname);
	}

	dev_add(sf.f_mntonname);

phase_2_end:
	(void)memset(&params, 0, sizeof(params));
}
		|	statement eol
		|	statement error eol {
	yyerrok;
}
		;

path		:	PATH {
	(void)strlcpy(params.file, $1, MAXPATHLEN);
}
		;

type		:	STRING {
	if (phase == 2) {
		if (strlen($1) >= sizeof(params.fp_type)) {
			yyerror("Fingerprint type too long");
			YYERROR;
		}
	
		(void)strlcpy(params.fp_type, $1, sizeof(params.fp_type));
	}
}
		;


fingerprint	:	STRING {
	if (phase == 2) {
		params.fingerprint = malloc(strlen($1) / 2);
		if (params.fingerprint == NULL)
			err(1, "Fingerprint mem alloc failed");
					
		if ((params.size = convert($1, params.fingerprint)) == -1) {
			free(params.fingerprint);
			yyerror("Bad fingerprint");
			YYERROR;
		}
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
		if (strcasecmp($1, "direct") == 0) {
			params.type |= VERIEXEC_DIRECT;
		} else if (strcasecmp($1, "indirect") == 0) {
			params.type |= VERIEXEC_INDIRECT;
		} else if (strcasecmp($1, "file") == 0) {
			params.type |= VERIEXEC_FILE;
		} else if (strcasecmp($1, "program") == 0) {
			params.type |= VERIEXEC_DIRECT;
		} else if (strcasecmp($1, "interpreter") == 0) {
			params.type |= VERIEXEC_INDIRECT;
		} else if (strcasecmp($1, "script") == 0) {
			params.type |= (VERIEXEC_FILE | VERIEXEC_DIRECT);
		} else if (strcasecmp($1, "library") == 0) {
			params.type |= (VERIEXEC_FILE | VERIEXEC_INDIRECT);
		} else if (strcasecmp($1, "untrusted") == 0) {
			params.type |= VERIEXEC_UNTRUSTED;
		} else {
			yyerror("Bad flag");
			YYERROR;
		}
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
static int
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
