%{
/*	$NetBSD: veriexecctl_parse.y,v 1.4.2.2 2005/06/10 14:51:51 tron Exp $	*/

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

%}

%union {
	char *string;
	int intval;
}

%token <string> PATH
%token <string> STRING
%token EOL

%%

statement	:	/* empty */
		|	statement path type fingerprint flags eol {
				struct stat sb;

				if (phase == 2) {
					phase2_load();
					goto phase_2_end;
				}

				if (stat(params.file, &sb) == 0) {
					struct vexec_up *p;

					/* Only regular files */
					if (!S_ISREG(sb.st_mode)) {
						(void) fprintf(stderr,
							       "Line %u: "
						    "%s is not a regular file.\n",
							       line,
							       params.file);
					}

					if ((p = dev_lookup(sb.st_dev)) != NULL) {
						(p->vu_param.hash_size)++;
					} else {
						if (verbose) {
							struct statvfs sf;

							statvfs(params.file,
							       &sf);

							(void) printf(
							    " => Adding device"
							    " ID %d. (%s)\n",
							    sb.st_dev,
							    sf.f_mntonname);
						}

						dev_add(sb.st_dev);
					}
				} else {
					(void) fprintf(stderr,
						       "Line %u: Can't stat"
						       " %s.\n",
					    line, params.file);
				}

phase_2_end:
				bzero(&params, sizeof(params));
			}
		|	statement eol
		|	statement error eol {
				yyerrok;
			}
		;

path		:	PATH {
				strlcpy(params.file, $1, MAXPATHLEN);
			}
		;

type		:	STRING {
				if (phase != 1) {
					if (strlen($1) >=
					    sizeof(params.fp_type)) {
						yyerror("Fingerprint type too "								"long");
						YYERROR;
					}
				
					strlcpy(params.fp_type, $1,
						sizeof(params.fp_type));
				}
			}
		;


fingerprint	:	STRING {
				if (phase != 1) {
					params.fingerprint = (char *)
						malloc(strlen($1) / 2);
					if (params.fingerprint == NULL) {
						fprintf(stderr, "Fingerprint"
							"mem alloc failed, "
							"cannot continue.\n");
						exit(1);
					}
					
					if ((params.size =
					       convert($1, params.fingerprint))
					     == -1) {
						free(params.fingerprint);
						yyerror("Bad fingerprint");
						YYERROR;
					}
				}
				
			}
		;

flags		:	/* empty */ {
				if (phase == 2)
					params.type = VERIEXEC_DIRECT;
			}
		|	flags flag_spec
		;

flag_spec	:	STRING {
				if (phase != 1) {
					if (strcasecmp($1, "direct") == 0) {
						params.type = VERIEXEC_DIRECT;
					} else if (strcasecmp($1, "indirect")
						   == 0) {
						params.type = VERIEXEC_INDIRECT;
/*					} else if (strcasecmp($1, "shell") == 0) {
						params.vxp_type = VEXEC_SHELL;*/
					} else if (strcasecmp($1, "file")
						   == 0) {
						params.type = VERIEXEC_FILE;
					} else {
						yyerror("Bad option");
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
int convert(u_char *fp, u_char *out) {
        int i, value, error, count;

	count = strlen(fp);

	  /*
	   * if there are not an even number of hex digits then there is
	   * not an integral number of bytes in the fingerprint.
	   */
	if ((count % 2) != 0)
		return -1;
	
	count /= 2;
	error = count;

	for (i = 0; i < count; i++) {
		if ((fp[2*i] >= '0') && (fp[2*i] <= '9')) {
			value = 16 * (fp[2*i] - '0');
		} else if ((tolower(fp[2*i]) >= 'a')
			   && (tolower(fp[2*i]) <= 'f')) {
			value = 16 * (10 + tolower(fp[2*i]) - 'a');
		} else {
			error = -1;
			break;
		}

		if ((fp[2*i + 1] >= '0') && (fp[2*i + 1] <= '9')) {
			value += fp[2*i + 1] - '0';
		} else if ((tolower(fp[2*i + 1]) >= 'a')
			   && (tolower(fp[2*i + 1]) <= 'f')) {
			value += tolower(fp[2*i + 1]) - 'a' + 10;
		} else {
			error = -1;
			break;
		}

		out[i] = value;
	}

	return (error);
}
