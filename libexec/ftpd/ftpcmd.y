/*	$NetBSD: ftpcmd.y,v 1.48 2000/06/19 15:15:03 lukem Exp $	*/

/*-
 * Copyright (c) 1997-2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Copyright (c) 1985, 1988, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ftpcmd.y	8.3 (Berkeley) 4/6/94
 */

/*
 * Grammar for FTP commands.
 * See RFC 959.
 */

%{
#include <sys/cdefs.h>

#ifndef lint
#if 0
static char sccsid[] = "@(#)ftpcmd.y	8.3 (Berkeley) 4/6/94";
#else
__RCSID("$NetBSD: ftpcmd.y,v 1.48 2000/06/19 15:15:03 lukem Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/ftp.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <glob.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>
#include <netdb.h>

#ifdef KERBEROS5
#include <krb5/krb5.h>
#endif

#include "extern.h"
#include "version.h"

static	int cmd_type;
static	int cmd_form;
static	int cmd_bytesz;

char	cbuf[512];
char	*fromname;

%}

%union {
	int	i;
	char   *s;
}

%token
	A	B	C	E	F	I
	L	N	P	R	S	T
	ALL

	SP	CRLF	COMMA

	USER	PASS	ACCT	CWD	CDUP	SMNT
	QUIT	REIN	PORT	PASV	TYPE	STRU
	MODE	RETR	STOR	STOU	APPE	ALLO
	REST	RNFR	RNTO	ABOR	DELE	RMD
	MKD	PWD	LIST	NLST	SITE	SYST
	STAT	HELP	NOOP

	AUTH	ADAT	PROT	PBSZ	CCC	MIC
	CONF	ENC

	FEAT	OPTS

	SIZE	MDTM	MLST	MLSD

	LPRT	LPSV	EPRT	EPSV

	MAIL	MLFL	MRCP	MRSQ	MSAM	MSND
	MSOM

	CHMOD	IDLE	RATEGET	RATEPUT	UMASK

	LEXERR

%token	<s> STRING
%token	<s> ALL
%token	<i> NUMBER

%type	<i> check_login check_modify check_upload octal_number byte_size
%type	<i> struct_code mode_code type_code form_code decimal_integer
%type	<s> pathstring pathname password username
%type	<s> mechanism_name base64data prot_code

%start	cmd_list

%%

cmd_list
	: /* empty */

	| cmd_list cmd
		{
			fromname = NULL;
			restart_point = (off_t) 0;
		}

	| cmd_list rcmd

	;

cmd
						/* RFC 959 */
	: USER SP username CRLF
		{
			user($3);
			free($3);
		}

	| PASS SP password CRLF
		{
			pass($3);
			memset($3, 0, strlen($3));
			free($3);
		}

	| CWD check_login CRLF
		{
			if ($2)
				cwd(pw->pw_dir);
		}

	| CWD check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				cwd($4);
			if ($4 != NULL)
				free($4);
		}

	| CDUP check_login CRLF
		{
			if ($2)
				cwd("..");
		}

	| QUIT CRLF
		{
			if (logged_in) {
				reply(-221, "");
				reply(0,
	    "Data traffic for this session was %qd byte%s in %qd file%s.",
				    (qdfmt_t)total_data, PLURAL(total_data),
				    (qdfmt_t)total_files, PLURAL(total_files));
				reply(0,
	    "Total traffic for this session was %qd byte%s in %qd transfer%s.",
				    (qdfmt_t)total_bytes, PLURAL(total_bytes),
				    (qdfmt_t)total_xfers, PLURAL(total_xfers));
			}
			reply(221,
			    "Thank you for using the FTP service on %s.",
			    hostname);
			if (logged_in) {
				syslog(LOG_INFO,
				    "Data traffic: %qd byte%s in %qd file%s",
				    (qdfmt_t)total_data, PLURAL(total_data),
				    (qdfmt_t)total_files, PLURAL(total_files));
				syslog(LOG_INFO,
				  "Total traffic: %qd byte%s in %qd transfer%s",
				    (qdfmt_t)total_bytes, PLURAL(total_bytes),
				    (qdfmt_t)total_xfers, PLURAL(total_xfers));
			}

			dologout(0);
		}

	| PORT check_login SP host_port CRLF
		{
			if ($2) {
					/* be paranoid, if told so */
			if (curclass.checkportcmd &&
			    ((ntohs(data_dest.su_port) < IPPORT_RESERVED) ||
			    memcmp(&data_dest.su_sin.sin_addr,
			    &his_addr.su_sin.sin_addr,
			    sizeof(data_dest.su_sin.sin_addr)) != 0)) {
				reply(500,
				    "Illegal PORT command rejected");
			} else if (epsvall) {
				reply(501, "PORT disallowed after EPSV ALL");
			} else {
				usedefault = 0;
				if (pdata >= 0) {
					(void) close(pdata);
					pdata = -1;
				}
				reply(200, "PORT command successful.");
			}

			}
		}

	| LPRT check_login SP host_long_port4 CRLF
		{
			if ($2) {

			/* reject invalid host_long_port4 */
			if (data_dest.su_family != AF_INET) {
				reply(500, "Illegal LPRT command rejected");
				return (NULL);
			}
			/* be paranoid, if told so */
			if (curclass.checkportcmd &&
			    ((ntohs(data_dest.su_port) < IPPORT_RESERVED) ||
			     memcmp(&data_dest.su_sin.sin_addr,
				    &his_addr.su_sin.sin_addr,
			     sizeof(data_dest.su_sin.sin_addr)) != 0)) {
				reply(500, "Illegal LPRT command rejected");
				return (NULL);
			}
			if (epsvall)
				reply(501, "LPRT disallowed after EPSV ALL");
			else {
				usedefault = 0;
				if (pdata >= 0) {
					(void) close(pdata);
					pdata = -1;
				}
				reply(200, "LPRT command successful.");
			}

			}
		}

	| LPRT check_login SP host_long_port6 CRLF
		{
			if ($2) {

			/* reject invalid host_long_port6 */
			if (data_dest.su_family != AF_INET6) {
				reply(500, "Illegal LPRT command rejected");
				return (NULL);
			}
			/* be paranoid, if told so */
			if (curclass.checkportcmd &&
			    ((ntohs(data_dest.su_port) < IPPORT_RESERVED) ||
			     memcmp(&data_dest.su_sin6.sin6_addr,
				    &his_addr.su_sin6.sin6_addr,
			     sizeof(data_dest.su_sin6.sin6_addr)) != 0)) {
				reply(500, "Illegal LPRT command rejected");
				return (NULL);
			}
			if (epsvall)
				reply(501, "LPRT disallowed after EPSV ALL");
			else {
				usedefault = 0;
				if (pdata >= 0) {
					(void) close(pdata);
					pdata = -1;
				}
				reply(200, "LPRT command successful.");
			}

			}
		}

	| EPRT check_login SP STRING CRLF
		{
			char *tmp = NULL;
			char *result[3];
			char *p, *q;
			char delim;
			struct addrinfo hints;
			struct addrinfo *res;
			int i;

			if ($2) {

			if (epsvall) {
				reply(501, "EPRT disallowed after EPSV ALL");
				goto eprt_done;
			}
			usedefault = 0;
			if (pdata >= 0) {
				(void) close(pdata);
				pdata = -1;
			}

			tmp = xstrdup($4);
			p = tmp;
			delim = p[0];
			p++;
			memset(result, 0, sizeof(result));
			for (i = 0; i < 3; i++) {
				q = strchr(p, delim);
				if (!q || *q != delim) {
		parsefail:
					reply(500,
					    "Invalid argument, rejected.");
					usedefault = 1;
					goto eprt_done;
				}
				*q++ = '\0';
				result[i] = p;
				p = q;
			}

			/* some more sanity check */
			p = result[0];
			while (*p) {
				if (!isdigit(*p))
					goto parsefail;
				p++;
			}
			p = result[2];
			while (*p) {
				if (!isdigit(*p))
					goto parsefail;
				p++;
			}

			memset(&hints, 0, sizeof(hints));
			if (atoi(result[0]) == 1)
				hints.ai_family = PF_INET;
			if (atoi(result[0]) == 2)
				hints.ai_family = PF_INET6;
			else
				hints.ai_family = PF_UNSPEC;	/*XXX*/
			hints.ai_socktype = SOCK_STREAM;
			if (getaddrinfo(result[1], result[2], &hints, &res))
				goto parsefail;
			memcpy(&data_dest, res->ai_addr, res->ai_addrlen);
			if (his_addr.su_family == AF_INET6
			 && data_dest.su_family == AF_INET6) {
				/* XXX more sanity checks! */
				data_dest.su_sin6.sin6_scope_id =
					his_addr.su_sin6.sin6_scope_id;
			}
			/* be paranoid, if told so */
			if (curclass.checkportcmd) {
				int fail;
				fail = 0;
				if (ntohs(data_dest.su_port) < IPPORT_RESERVED)
					fail++;
				if (data_dest.su_family != his_addr.su_family)
					fail++;
				if (data_dest.su_len != his_addr.su_len)
					fail++;
				switch (data_dest.su_family) {
				case AF_INET:
					fail += memcmp(
					    &data_dest.su_sin.sin_addr,
					    &his_addr.su_sin.sin_addr,
					    sizeof(data_dest.su_sin.sin_addr));
					break;
				case AF_INET6:
					fail += memcmp(
					    &data_dest.su_sin6.sin6_addr,
					    &his_addr.su_sin6.sin6_addr,
					    sizeof(data_dest.su_sin6.sin6_addr));
					break;
				default:
					fail++;
				}
				if (fail) {
					reply(500,
					    "Illegal EPRT command rejected");
					return (NULL);
				}
			}
			if (pdata >= 0) {
				(void) close(pdata);
				pdata = -1;
			}
			reply(200, "EPRT command successful.");
		eprt_done:;
			if (tmp != NULL)
				free(tmp);

			}
			free($4);
		}

	| PASV check_login CRLF
		{
			if ($2) {
				if (curclass.passive)
					passive();
				else
					reply(500, "PASV mode not available.");
			}
		}

	| LPSV check_login CRLF
		{
			if ($2) {
				if (epsvall)
					reply(501,
					    "LPSV disallowed after EPSV ALL");
				else
					long_passive("LPSV", PF_UNSPEC);
			}
		}

	| EPSV check_login SP NUMBER CRLF
		{
			if ($2) {
				int pf;

				switch ($4) {
				case 1:
					pf = PF_INET;
					break;
				case 2:
					pf = PF_INET6;
					break;
				default:
					pf = -1;	/*junk*/
					break;
				}
				long_passive("EPSV", pf);
			}
		}

	| EPSV check_login SP ALL CRLF
		{
			if ($2) {
				reply(200, "EPSV ALL command successful.");
				epsvall++;
			}
		}

	| EPSV check_login CRLF
		{
			if ($2)
				long_passive("EPSV", PF_UNSPEC);
		}

	| TYPE check_login SP type_code CRLF
		{
			if ($2) {

			switch (cmd_type) {

			case TYPE_A:
				if (cmd_form == FORM_N) {
					reply(200, "Type set to A.");
					type = cmd_type;
					form = cmd_form;
				} else
					reply(504, "Form must be N.");
				break;

			case TYPE_E:
				reply(504, "Type E not implemented.");
				break;

			case TYPE_I:
				reply(200, "Type set to I.");
				type = cmd_type;
				break;

			case TYPE_L:
#if NBBY == 8
				if (cmd_bytesz == 8) {
					reply(200,
					    "Type set to L (byte size 8).");
					type = cmd_type;
				} else
					reply(504, "Byte size must be 8.");
#else /* NBBY == 8 */
				UNIMPLEMENTED for NBBY != 8
#endif /* NBBY == 8 */
			}
			
			}
		}

	| STRU check_login SP struct_code CRLF
		{
			if ($2) {
				switch ($4) {

				case STRU_F:
					reply(200, "STRU F ok.");
					break;

				default:
					reply(504, "Unimplemented STRU type.");
				}
			}
		}

	| MODE check_login SP mode_code CRLF
		{
			if ($2) {
				switch ($4) {

				case MODE_S:
					reply(200, "MODE S ok.");
					break;

				default:
					reply(502, "Unimplemented MODE type.");
				}
			}
		}

	| RETR check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				retrieve(NULL, $4);
			if ($4 != NULL)
				free($4);
		}

	| STOR check_upload SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				store($4, "w", 0);
			if ($4 != NULL)
				free($4);
		}

	| STOU check_upload SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				store($4, "w", 1);
			if ($4 != NULL)
				free($4);
		}
		
	| APPE check_upload SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				store($4, "a", 0);
			if ($4 != NULL)
				free($4);
		}

	| ALLO check_login SP NUMBER CRLF
		{
			if ($2)
				reply(202, "ALLO command ignored.");
		}

	| ALLO check_login SP NUMBER SP R SP NUMBER CRLF
		{
			if ($2)
				reply(202, "ALLO command ignored.");
		}

	| RNTO check_login SP pathname CRLF
		{
			if ($2) {
				if (fromname) {
					renamecmd(fromname, $4);
					free(fromname);
					fromname = NULL;
				} else {
					reply(503, "Bad sequence of commands.");
				}
			}
			free($4);
		}

	| ABOR check_login CRLF
		{
			if ($2)
				reply(225, "ABOR command successful.");
		}

	| DELE check_modify SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				delete($4);
			if ($4 != NULL)
				free($4);
		}

	| RMD check_modify SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				removedir($4);
			if ($4 != NULL)
				free($4);
		}

	| MKD check_modify SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				makedir($4);
			if ($4 != NULL)
				free($4);
		}

	| PWD check_login CRLF
		{
			if ($2)
				pwd();
		}

	| LIST check_login CRLF
		{
			char *argv[] = { INTERNAL_LS, "-lgA", NULL };
			
			if ($2)
				retrieve(argv, "");
		}

	| LIST check_login SP pathname CRLF
		{
			char *argv[] = { INTERNAL_LS, "-lgA", NULL, NULL };

			if ($2 && $4 != NULL) {
				argv[2] = $4;
				retrieve(argv, $4);
			}
			if ($4 != NULL)
				free($4);
		}

	| NLST check_login CRLF
		{
			if ($2)
				send_file_list(".");
		}

	| NLST check_login SP STRING CRLF
		{
			if ($2)
				send_file_list($4);
			free($4);
		}

	| SITE SP HELP CRLF
		{
			help(sitetab, NULL);
		}

	| SITE SP CHMOD check_modify SP octal_number SP pathname CRLF
		{
			if ($4 && ($8 != NULL)) {
				if ($6 > 0777)
					reply(501,
				"CHMOD: Mode value must be between 0 and 0777");
				else if (chmod($8, $6) < 0)
					perror_reply(550, $8);
				else
					reply(200, "CHMOD command successful.");
			}
			if ($8 != NULL)
				free($8);
		}

	| SITE SP HELP SP STRING CRLF
		{
			help(sitetab, $5);
			free($5);
		}

	| SITE SP IDLE check_login CRLF
		{
			if ($4) {
				reply(200,
			    "Current IDLE time limit is %d seconds; max %d",
				    curclass.timeout, curclass.maxtimeout);
			}
		}

	| SITE SP IDLE check_login SP NUMBER CRLF
		{
			if ($4) {
				if ($6 < 30 || $6 > curclass.maxtimeout) {
					reply(501,
			    "IDLE time limit must be between 30 and %d seconds",
					    curclass.maxtimeout);
				} else {
					curclass.timeout = $6;
					(void) alarm(curclass.timeout);
					reply(200,
					    "IDLE time limit set to %d seconds",
					    curclass.timeout);
				}
			}
		}

	| SITE SP RATEGET check_login CRLF
		{
			if ($4) {
				reply(200, "Current RATEGET is %d bytes/sec",
				    curclass.rateget);
			}
		}

	| SITE SP RATEGET check_login SP STRING CRLF
		{
			char *p = $6;
			int rate;

			if ($4) {
				rate = strsuftoi(p);
				if (rate == -1)
					reply(501, "Invalid RATEGET %s", p);
				else if (curclass.maxrateget &&
				    rate > curclass.maxrateget)
					reply(501,
				"RATEGET %d is larger than maximum RATEGET %d",
					    rate, curclass.maxrateget);
				else {
					curclass.rateget = rate;
					reply(200,
					    "RATEGET set to %d bytes/sec",
					    curclass.rateget);
				}
			}
			free($6);
		}

	| SITE SP RATEPUT check_login CRLF
		{
			if ($4) {
				reply(200, "Current RATEPUT is %d bytes/sec",
				    curclass.rateput);
			}
		}

	| SITE SP RATEPUT check_login SP STRING CRLF
		{
			char *p = $6;
			int rate;

			if ($4) {
				rate = strsuftoi(p);
				if (rate == -1)
					reply(501, "Invalid RATEPUT %s", p);
				else if (curclass.maxrateput &&
				    rate > curclass.maxrateput)
					reply(501,
				"RATEPUT %d is larger than maximum RATEPUT %d",
					    rate, curclass.maxrateput);
				else {
					curclass.rateput = rate;
					reply(200,
					    "RATEPUT set to %d bytes/sec",
					    curclass.rateput);
				}
			}
			free($6);
		}

	| SITE SP UMASK check_login CRLF
		{
			int oldmask;

			if ($4) {
				oldmask = umask(0);
				(void) umask(oldmask);
				reply(200, "Current UMASK is %03o", oldmask);
			}
		}

	| SITE SP UMASK check_modify SP octal_number CRLF
		{
			int oldmask;

			if ($4) {
				if (($6 == -1) || ($6 > 0777)) {
					reply(501, "Bad UMASK value");
				} else {
					oldmask = umask($6);
					reply(200,
					    "UMASK set to %03o (was %03o)",
					    $6, oldmask);
				}
			}
		}

	| SYST CRLF
		{
			reply(215, "UNIX Type: L%d Version: %s", NBBY,
			    FTPD_VERSION);
		}

	| STAT check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				statfilecmd($4);
			if ($4 != NULL)
				free($4);
		}
		
	| STAT CRLF
		{
			statcmd();
		}

	| HELP CRLF
		{
			help(cmdtab, NULL);
		}

	| HELP SP STRING CRLF
		{
			char *cp = $3;

			if (strncasecmp(cp, "SITE", 4) == 0) {
				cp = $3 + 4;
				if (*cp == ' ')
					cp++;
				if (*cp)
					help(sitetab, cp);
				else
					help(sitetab, NULL);
			} else
				help(cmdtab, $3);
			free($3);
		}

	| NOOP CRLF
		{
			reply(200, "NOOP command successful.");
		}

						/* RFC 2228 */
	| AUTH SP mechanism_name CRLF
		{
			reply(502, "RFC 2228 authentication not implemented.");
			free($3);
		}

	| ADAT SP base64data CRLF
		{
			reply(503,
			    "Please set authentication state with AUTH.");
			free($3);
		}

	| PROT SP prot_code CRLF
		{
			reply(503,
			    "Please set protection buffer size with PBSZ.");
			free($3);
		}

	| PBSZ SP decimal_integer CRLF
		{
			reply(503,
			    "Please set authentication state with AUTH.");
		}

	| CCC CRLF
		{
			reply(533, "No protection enabled.");
		}

	| MIC SP base64data CRLF
		{
			reply(502, "RFC 2228 authentication not implemented.");
			free($3);
		}

	| CONF SP base64data CRLF
		{
			reply(502, "RFC 2228 authentication not implemented.");
			free($3);
		}

	| ENC SP base64data CRLF
		{
			reply(502, "RFC 2228 authentication not implemented.");
			free($3);
		}

						/* RFC 2389 */
	| FEAT CRLF
		{

			feat();
		}

	| OPTS SP STRING CRLF
		{
			
			opts($3);
			free($3);
		}


				/* extensions from draft-ietf-ftpext-mlst-10 */

		/*
		 * Return size of file in a format suitable for
		 * using with RESTART (we just count bytes).
		 */
	| SIZE check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				sizecmd($4);
			if ($4 != NULL)
				free($4);
		}

		/*
		 * Return modification time of file as an ISO 3307
		 * style time. E.g. YYYYMMDDHHMMSS or YYYYMMDDHHMMSS.xxx
		 * where xxx is the fractional second (of any precision,
		 * not necessarily 3 digits)
		 */
	| MDTM check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL) {
				struct stat stbuf;
				if (stat($4, &stbuf) < 0)
					perror_reply(550, $4);
				else if (!S_ISREG(stbuf.st_mode)) {
					reply(550, "%s: not a plain file.", $4);
				} else {
					struct tm *t;

					t = gmtime(&stbuf.st_mtime);
					reply(213,
					    "%04d%02d%02d%02d%02d%02d",
					    TM_YEAR_BASE + t->tm_year,
					    t->tm_mon+1, t->tm_mday,
					    t->tm_hour, t->tm_min, t->tm_sec);
				}
			}
			if ($4 != NULL)
				free($4);
		}

	| MLST check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				mlst($4);
			if ($4 != NULL)
				free($4);
		}
		
	| MLST check_login CRLF
		{
			mlst(NULL);
		}

	| MLSD check_login SP pathname CRLF
		{
			if ($2 && $4 != NULL)
				mlsd($4);
			if ($4 != NULL)
				free($4);
		}
		
	| MLSD check_login CRLF
		{
			mlsd(NULL);
		}

	| error CRLF
		{
			yyerrok;
		}
	;

rcmd
	: REST check_login SP byte_size CRLF
		{
			if ($2) {
				fromname = NULL;
				restart_point = $4; /* XXX $3 is only "int" */
				reply(350, "Restarting at %qd. %s",
				    (qdfmt_t)restart_point,
			    "Send STORE or RETRIEVE to initiate transfer.");
			}
		}

	| RNFR check_modify SP pathname CRLF
		{
			restart_point = (off_t) 0;
			if ($2 && $4) {
				fromname = renamefrom($4);
			}
			if ($4)
				free($4);
		}
	;

username
	: STRING
	;

password
	: /* empty */
		{
			$$ = (char *)calloc(1, sizeof(char));
		}

	| STRING
	;

byte_size
	: NUMBER
	;

host_port
	: NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER COMMA
		NUMBER COMMA NUMBER
		{
			char *a, *p;

			data_dest.su_len = sizeof(struct sockaddr_in);
			data_dest.su_family = AF_INET;
			p = (char *)&data_dest.su_sin.sin_port;
			p[0] = $9; p[1] = $11;
			a = (char *)&data_dest.su_sin.sin_addr;
			a[0] = $1; a[1] = $3; a[2] = $5; a[3] = $7;
		}
	;

host_long_port4
	: NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER COMMA
		NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER COMMA
		NUMBER
		{
			char *a, *p;

			data_dest.su_sin.sin_len =
				sizeof(struct sockaddr_in);
			data_dest.su_family = AF_INET;
			p = (char *)&data_dest.su_port;
			p[0] = $15; p[1] = $17;
			a = (char *)&data_dest.su_sin.sin_addr;
			a[0] =  $5;  a[1] =  $7;  a[2] =  $9;  a[3] = $11;

			/* reject invalid LPRT command */
			if ($1 != 4 || $3 != 4 || $13 != 2)
				memset(&data_dest, 0, sizeof(data_dest));
		}
	;

host_long_port6
	: NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER COMMA
		NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER COMMA
		NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER COMMA
		NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER COMMA
		NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER COMMA
		NUMBER
		{
			char *a, *p;

			data_dest.su_sin6.sin6_len =
				sizeof(struct sockaddr_in6);
			data_dest.su_family = AF_INET6;
			p = (char *)&data_dest.su_port;
			p[0] = $39; p[1] = $41;
			a = (char *)&data_dest.su_sin6.sin6_addr;
			 a[0] =  $5;  a[1] =  $7;  a[2] =  $9;  a[3] = $11;
			 a[4] = $13;  a[5] = $15;  a[6] = $17;  a[7] = $19;
			 a[8] = $21;  a[9] = $23; a[10] = $25; a[11] = $27;
			a[12] = $29; a[13] = $31; a[14] = $33; a[15] = $35;
			if (his_addr.su_family == AF_INET6) {
				/* XXX more sanity checks! */
				data_dest.su_sin6.sin6_scope_id =
					his_addr.su_sin6.sin6_scope_id;
			}

			/* reject invalid LPRT command */
			if ($1 != 6 || $3 != 16 || $37 != 2)
				memset(&data_dest, 0, sizeof(data_dest));
		}
	;

form_code
	: N
		{
			$$ = FORM_N;
		}

	| T
		{
			$$ = FORM_T;
		}

	| C
		{
			$$ = FORM_C;
		}
	;

type_code
	: A
		{
			cmd_type = TYPE_A;
			cmd_form = FORM_N;
		}

	| A SP form_code
		{
			cmd_type = TYPE_A;
			cmd_form = $3;
		}

	| E
		{
			cmd_type = TYPE_E;
			cmd_form = FORM_N;
		}

	| E SP form_code
		{
			cmd_type = TYPE_E;
			cmd_form = $3;
		}

	| I
		{
			cmd_type = TYPE_I;
		}

	| L
		{
			cmd_type = TYPE_L;
			cmd_bytesz = NBBY;
		}

	| L SP byte_size
		{
			cmd_type = TYPE_L;
			cmd_bytesz = $3;
		}

		/* this is for a bug in the BBN ftp */
	| L byte_size
		{
			cmd_type = TYPE_L;
			cmd_bytesz = $2;
		}
	;

struct_code
	: F
		{
			$$ = STRU_F;
		}

	| R
		{
			$$ = STRU_R;
		}

	| P
		{
			$$ = STRU_P;
		}
	;

mode_code
	: S
		{
			$$ = MODE_S;
		}

	| B
		{
			$$ = MODE_B;
		}

	| C
		{
			$$ = MODE_C;
		}
	;

pathname
	: pathstring
		{
			/*
			 * Problem: this production is used for all pathname
			 * processing, but only gives a 550 error reply.
			 * This is a valid reply in some cases but not in
			 * others.
			 */
			if (logged_in && $1 && *$1 == '~') {
				glob_t gl;
				int flags =
				 GLOB_BRACE|GLOB_NOCHECK|GLOB_TILDE;

				if ($1[1] == '\0')
					$$ = xstrdup(pw->pw_dir);
				else {
					memset(&gl, 0, sizeof(gl));
					if (glob($1, flags, NULL, &gl) ||
					    gl.gl_pathc == 0) {
						reply(550, "not found");
						$$ = NULL;
					} else
						$$ = xstrdup(gl.gl_pathv[0]);
					globfree(&gl);
				}
				free($1);
			} else
				$$ = $1;
		}
	;

pathstring
	: STRING
	;

octal_number
	: NUMBER
		{
			int ret, dec, multby, digit;

			/*
			 * Convert a number that was read as decimal number
			 * to what it would be if it had been read as octal.
			 */
			dec = $1;
			multby = 1;
			ret = 0;
			while (dec) {
				digit = dec%10;
				if (digit > 7) {
					ret = -1;
					break;
				}
				ret += digit * multby;
				multby *= 8;
				dec /= 10;
			}
			$$ = ret;
		}
	;

mechanism_name
	: STRING
	;

base64data
	: STRING
	;

prot_code
	: STRING
	;

decimal_integer
	: NUMBER
	;

check_login
	: /* empty */
		{
			if (logged_in)
				$$ = 1;
			else {
				reply(530, "Please login with USER and PASS.");
				$$ = 0;
				hasyyerrored = 1;
			}
		}
	;

check_modify
	: /* empty */
		{
			if (logged_in) {
				if (curclass.modify)
					$$ = 1;
				else {
					reply(502,
					"No permission to use this command.");
					$$ = 0;
					hasyyerrored = 1;
				}
			} else {
				reply(530, "Please login with USER and PASS.");
				$$ = 0;
				hasyyerrored = 1;
			}
		}

check_upload
	: /* empty */
		{
			if (logged_in) {
				if (curclass.upload)
					$$ = 1;
				else {
					reply(502,
					"No permission to use this command.");
					$$ = 0;
					hasyyerrored = 1;
				}
			} else {
				reply(530, "Please login with USER and PASS.");
				$$ = 0;
				hasyyerrored = 1;
			}
		}


%%

#define	CMD	0	/* beginning of command */
#define	ARGS	1	/* expect miscellaneous arguments */
#define	STR1	2	/* expect SP followed by STRING */
#define	STR2	3	/* expect STRING */
#define	OSTR	4	/* optional SP then STRING */
#define	ZSTR1	5	/* SP then optional STRING */
#define	ZSTR2	6	/* optional STRING after SP */
#define	SITECMD	7	/* SITE command */
#define	NSTR	8	/* Number followed by a string */
#define NOARGS	9	/* No arguments allowed */

struct tab cmdtab[] = {
				/* From RFC 959, in order defined (5.3.1) */
	{ "USER", USER, STR1,	1,	"<sp> username" },
	{ "PASS", PASS, ZSTR1,	1,	"<sp> password" },
	{ "ACCT", ACCT, STR1,	0,	"(specify account)" },
	{ "CWD",  CWD,  OSTR,	1,	"[ <sp> directory-name ]" },
	{ "CDUP", CDUP, NOARGS,	1,	"(change to parent directory)" },
	{ "SMNT", SMNT, ARGS,	0,	"(structure mount)" },
	{ "QUIT", QUIT, NOARGS,	1,	"(terminate service)" },
	{ "REIN", REIN, NOARGS,	0,	"(reinitialize server state)" },
	{ "PORT", PORT, ARGS,	1,	"<sp> b0, b1, b2, b3, b4" },
	{ "LPRT", LPRT, ARGS,	1,	"<sp> af, hal, h1, h2, h3,..., pal, p1, p2..." },
	{ "EPRT", EPRT, STR1,	1,	"<sp> |af|addr|port|" },
	{ "PASV", PASV, NOARGS,	1,	"(set server in passive mode)" },
	{ "LPSV", LPSV, ARGS,	1,	"(set server in passive mode)" },
	{ "EPSV", EPSV, ARGS,	1,	"[<sp> af|ALL]" },
	{ "TYPE", TYPE, ARGS,	1,	"<sp> [ A | E | I | L ]" },
	{ "STRU", STRU, ARGS,	1,	"(specify file structure)" },
	{ "MODE", MODE, ARGS,	1,	"(specify transfer mode)" },
	{ "RETR", RETR, STR1,	1,	"<sp> file-name" },
	{ "STOR", STOR, STR1,	1,	"<sp> file-name" },
	{ "STOU", STOU, STR1,	1,	"<sp> file-name" },
	{ "APPE", APPE, STR1,	1,	"<sp> file-name" },
	{ "ALLO", ALLO, ARGS,	1,	"allocate storage (vacuously)" },
	{ "REST", REST, ARGS,	1,	"<sp> offset (restart command)" },
	{ "RNFR", RNFR, STR1,	1,	"<sp> file-name" },
	{ "RNTO", RNTO, STR1,	1,	"<sp> file-name" },
	{ "ABOR", ABOR, NOARGS,	1,	"(abort operation)" },
	{ "DELE", DELE, STR1,	1,	"<sp> file-name" },
	{ "RMD",  RMD,  STR1,	1,	"<sp> path-name" },
	{ "MKD",  MKD,  STR1,	1,	"<sp> path-name" },
	{ "PWD",  PWD,  NOARGS,	1,	"(return current directory)" },
	{ "LIST", LIST, OSTR,	1,	"[ <sp> path-name ]" },
	{ "NLST", NLST, OSTR,	1,	"[ <sp> path-name ]" },
	{ "SITE", SITE, SITECMD, 1,	"site-cmd [ <sp> arguments ]" },
	{ "SYST", SYST, NOARGS,	1,	"(get type of operating system)" },
	{ "STAT", STAT, OSTR,	1,	"[ <sp> path-name ]" },
	{ "HELP", HELP, OSTR,	1,	"[ <sp> <string> ]" },
	{ "NOOP", NOOP, NOARGS,	2,	"" },

				/* From RFC 2228, in order defined */
	{ "AUTH", AUTH, STR1,	1,	"<sp> mechanism-name" },
	{ "ADAT", ADAT, STR1,	1,	"<sp> base-64-data" },
	{ "PROT", PROT, STR1,	1,	"<sp> prot-code" },
	{ "PBSZ", PBSZ, ARGS,	1,	"<sp> decimal-integer" },
	{ "CCC",  CCC,  NOARGS,	1,	"(Disable data protection)" },
	{ "MIC",  MIC,  STR1,	1,	"<sp> base64data" },
	{ "CONF", CONF, STR1,	1,	"<sp> base64data" },
	{ "ENC",  ENC,  STR1,	1,	"<sp> base64data" },

				/* From RFC 2389, in order defined */
	{ "FEAT", FEAT, NOARGS,	1,	"(display extended features)" },
	{ "OPTS", OPTS, STR1,	1,	"<sp> command [ <sp> options ]" },

				/* from draft-ietf-ftpext-mlst-10 */
	{ "MDTM", MDTM, OSTR,	1,	"<sp> path-name" },
	{ "SIZE", SIZE, OSTR,	1,	"<sp> path-name" },
	{ "MLST", MLST, OSTR,	2,	"[ <sp> path-name ]" },
	{ "MLSD", MLSD, OSTR,	1,	"[ <sp> directory-name ]" },

				/* obsolete commands */
	{ "MAIL", MAIL, OSTR,	0,	"(mail to user)" },
	{ "MLFL", MLFL, OSTR,	0,	"(mail file)" },
	{ "MRCP", MRCP, STR1,	0,	"(mail recipient)" },
	{ "MRSQ", MRSQ, OSTR,	0,	"(mail recipient scheme question)" },
	{ "MSAM", MSAM, OSTR,	0,	"(mail send to terminal and mailbox)" },
	{ "MSND", MSND, OSTR,	0,	"(mail send to terminal)" },
	{ "MSOM", MSOM, OSTR,	0,	"(mail send to terminal or mailbox)" },
	{ "XCUP", CDUP, NOARGS,	1,	"(change to parent directory)" },
	{ "XCWD", CWD,  OSTR,	1,	"[ <sp> directory-name ]" },
	{ "XMKD", MKD,  STR1,	1,	"<sp> path-name" },
	{ "XPWD", PWD,  NOARGS,	1,	"(return current directory)" },
	{ "XRMD", RMD,  STR1,	1,	"<sp> path-name" },

	{  NULL,  0,	0,	0,	0 }
};

struct tab sitetab[] = {
	{ "CHMOD",   	CHMOD,	NSTR, 1,	"<sp> mode <sp> file-name" },
	{ "HELP",    	HELP,	OSTR, 1,	"[ <sp> <string> ]" },
	{ "IDLE",    	IDLE,	ARGS, 1,	"[ <sp> maximum-idle-time ]" },
	{ "RATEGET", 	RATEGET,OSTR, 1,	"[ <sp> get-throttle-rate ]" },
	{ "RATEPUT", 	RATEPUT,OSTR, 1,	"[ <sp> put-throttle-rate ]" },
	{ "UMASK",   	UMASK,	ARGS, 1,	"[ <sp> umask ]" },
	{ NULL,		0,     0,     0,	NULL }
};

static	void		help(struct tab *, const char *);
static	void		toolong(int);
static	int		yylex(void);

extern int epsvall;

struct tab *
lookup(struct tab *p, const char *cmd)
{

	for (; p->name != NULL; p++)
		if (strcasecmp(cmd, p->name) == 0)
			return (p);
	return (0);
}

#include <arpa/telnet.h>

/*
 * getline - a hacked up version of fgets to ignore TELNET escape codes.
 */
char *
getline(char *s, int n, FILE *iop)
{
	int c;
	char *cs;

	cs = s;
/* tmpline may contain saved command from urgent mode interruption */
	for (c = 0; tmpline[c] != '\0' && --n > 0; ++c) {
		*cs++ = tmpline[c];
		if (tmpline[c] == '\n') {
			*cs++ = '\0';
			if (debug)
				syslog(LOG_DEBUG, "command: %s", s);
			tmpline[0] = '\0';
			return(s);
		}
		if (c == 0)
			tmpline[0] = '\0';
	}
	while ((c = getc(iop)) != EOF) {
		total_bytes++;
		total_bytes_in++;
		c &= 0377;
		if (c == IAC) {
		    if ((c = getc(iop)) != EOF) {
			total_bytes++;
			total_bytes_in++;
			c &= 0377;
			switch (c) {
			case WILL:
			case WONT:
				c = getc(iop);
				total_bytes++;
				total_bytes_in++;
				cprintf(stdout, "%c%c%c", IAC, DONT, 0377&c);
				(void) fflush(stdout);
				continue;
			case DO:
			case DONT:
				c = getc(iop);
				total_bytes++;
				total_bytes_in++;
				cprintf(stdout, "%c%c%c", IAC, WONT, 0377&c);
				(void) fflush(stdout);
				continue;
			case IAC:
				break;
			default:
				continue;	/* ignore command */
			}
		    }
		}
		*cs++ = c;
		if (--n <= 0 || c == '\n')
			break;
	}
	if (c == EOF && cs == s)
		return (NULL);
	*cs++ = '\0';
	if (debug) {
		if (curclass.type != CLASS_GUEST &&
		    strncasecmp("pass ", s, 5) == 0) {
			/* Don't syslog passwords */
			syslog(LOG_DEBUG, "command: %.5s ???", s);
		} else {
			char *cp;
			int len;

			/* Don't syslog trailing CR-LF */
			len = strlen(s);
			cp = s + len - 1;
			while (cp >= s && (*cp == '\n' || *cp == '\r')) {
				--cp;
				--len;
			}
			syslog(LOG_DEBUG, "command: %.*s", len, s);
		}
	}
	return (s);
}

static void
toolong(int signo)
{

	reply(421,
	    "Timeout (%d seconds): closing control connection.",
	    curclass.timeout);
	if (logging)
		syslog(LOG_INFO, "User %s timed out after %d seconds",
		    (pw ? pw->pw_name : "unknown"), curclass.timeout);
	dologout(1);
}

static int
yylex(void)
{
	static int cpos, state;
	char *cp, *cp2;
	struct tab *p;
	int n;
	char c;

	switch (state) {

	case CMD:
		hasyyerrored = 0;
		(void) signal(SIGALRM, toolong);
		(void) alarm(curclass.timeout);
		if (getline(cbuf, sizeof(cbuf)-1, stdin) == NULL) {
			reply(221, "You could at least say goodbye.");
			dologout(0);
		}
		(void) alarm(0);
		if ((cp = strchr(cbuf, '\r'))) {
			*cp = '\0';
#ifdef HASSETPROCTITLE
			if (strncasecmp(cbuf, "PASS", 4) != 0)
				setproctitle("%s: %s", proctitle, cbuf);
#endif /* HASSETPROCTITLE */
			*cp++ = '\n';
			*cp = '\0';
		}
		if ((cp = strpbrk(cbuf, " \n")))
			cpos = cp - cbuf;
		if (cpos == 0)
			cpos = 4;
		c = cbuf[cpos];
		cbuf[cpos] = '\0';
		p = lookup(cmdtab, cbuf);
		cbuf[cpos] = c;
		if (p != NULL) {
			if (! CMD_IMPLEMENTED(p)) {
				reply(502, "%s command not implemented.",
				    p->name);
				hasyyerrored = 1;
				break;
			}
			state = p->state;
			yylval.s = p->name;
			return (p->token);
		}
		break;

	case SITECMD:
		if (cbuf[cpos] == ' ') {
			cpos++;
			return (SP);
		}
		cp = &cbuf[cpos];
		if ((cp2 = strpbrk(cp, " \n")))
			cpos = cp2 - cbuf;
		c = cbuf[cpos];
		cbuf[cpos] = '\0';
		p = lookup(sitetab, cp);
		cbuf[cpos] = c;
		if (p != NULL) {
			if (!CMD_IMPLEMENTED(p)) {
				reply(502, "SITE %s command not implemented.",
				    p->name);
				hasyyerrored = 1;
				break;
			}
			state = p->state;
			yylval.s = p->name;
			return (p->token);
		}
		break;

	case OSTR:
		if (cbuf[cpos] == '\n') {
			state = CMD;
			return (CRLF);
		}
		/* FALLTHROUGH */

	case STR1:
	case ZSTR1:
	dostr1:
		if (cbuf[cpos] == ' ') {
			cpos++;
			state = state == OSTR ? STR2 : state+1;
			return (SP);
		}
		break;

	case ZSTR2:
		if (cbuf[cpos] == '\n') {
			state = CMD;
			return (CRLF);
		}
		/* FALLTHROUGH */

	case STR2:
		cp = &cbuf[cpos];
		n = strlen(cp);
		cpos += n - 1;
		/*
		 * Make sure the string is nonempty and \n terminated.
		 */
		if (n > 1 && cbuf[cpos] == '\n') {
			cbuf[cpos] = '\0';
			yylval.s = xstrdup(cp);
			cbuf[cpos] = '\n';
			state = ARGS;
			return (STRING);
		}
		break;

	case NSTR:
		if (cbuf[cpos] == ' ') {
			cpos++;
			return (SP);
		}
		if (isdigit(cbuf[cpos])) {
			cp = &cbuf[cpos];
			while (isdigit(cbuf[++cpos]))
				;
			c = cbuf[cpos];
			cbuf[cpos] = '\0';
			yylval.i = atoi(cp);
			cbuf[cpos] = c;
			state = STR1;
			return (NUMBER);
		}
		state = STR1;
		goto dostr1;

	case ARGS:
		if (isdigit(cbuf[cpos])) {
			cp = &cbuf[cpos];
			while (isdigit(cbuf[++cpos]))
				;
			c = cbuf[cpos];
			cbuf[cpos] = '\0';
			yylval.i = atoi(cp);
			cbuf[cpos] = c;
			return (NUMBER);
		}
		if (strncasecmp(&cbuf[cpos], "ALL", 3) == 0
		 && !isalnum(cbuf[cpos + 3])) {
			yylval.s = xstrdup("ALL");
			cpos += 3;
			return ALL;
		}
		switch (cbuf[cpos++]) {

		case '\n':
			state = CMD;
			return (CRLF);

		case ' ':
			return (SP);

		case ',':
			return (COMMA);

		case 'A':
		case 'a':
			return (A);

		case 'B':
		case 'b':
			return (B);

		case 'C':
		case 'c':
			return (C);

		case 'E':
		case 'e':
			return (E);

		case 'F':
		case 'f':
			return (F);

		case 'I':
		case 'i':
			return (I);

		case 'L':
		case 'l':
			return (L);

		case 'N':
		case 'n':
			return (N);

		case 'P':
		case 'p':
			return (P);

		case 'R':
		case 'r':
			return (R);

		case 'S':
		case 's':
			return (S);

		case 'T':
		case 't':
			return (T);

		}
		break;

	case NOARGS:
		if (cbuf[cpos] == '\n') {
			state = CMD;
			return (CRLF);
		}
		c = cbuf[cpos];
		cbuf[cpos] = '\0';
		reply(501, "'%s' command does not take any arguments.", cbuf);
		hasyyerrored = 1;
		cbuf[cpos] = c;
		break;

	default:
		fatal("Unknown state in scanner.");
	}
	yyerror(NULL);
	state = CMD;
	longjmp(errcatch, 0);
	/* NOTREACHED */
}

/* ARGSUSED */
void
yyerror(char *s)
{
	char *cp;

	if (hasyyerrored)
		return;
	if ((cp = strchr(cbuf,'\n')) != NULL)
		*cp = '\0';
	reply(500, "'%s': command not understood.", cbuf);
	hasyyerrored = 1;
}

static void
help(struct tab *ctab, const char *s)
{
	struct tab *c;
	int width, NCMDS;
	char *type;

	if (ctab == sitetab)
		type = "SITE ";
	else
		type = "";
	width = 0, NCMDS = 0;
	for (c = ctab; c->name != NULL; c++) {
		int len = strlen(c->name);

		if (len > width)
			width = len;
		NCMDS++;
	}
	width = (width + 8) &~ 7;
	if (s == 0) {
		int i, j, w;
		int columns, lines;

		reply(-214, "");
		reply(0, "The following %scommands are recognized.", type);
		reply(0, "(`-' = not implemented, `+' = supports options)");
		columns = 76 / width;
		if (columns == 0)
			columns = 1;
		lines = (NCMDS + columns - 1) / columns;
		for (i = 0; i < lines; i++) {
			cprintf(stdout, "    ");
			for (j = 0; j < columns; j++) {
				c = ctab + j * lines + i;
				cprintf(stdout, "%s", c->name);
				w = strlen(c->name);
				if (! CMD_IMPLEMENTED(c)) {
					CPUTC('-', stdout);
					w++;
				}
				if (CMD_HAS_OPTIONS(c)) {
					CPUTC('+', stdout);
					w++;
				}
				if (c + lines >= &ctab[NCMDS])
					break;
				while (w < width) {
					CPUTC(' ', stdout);
					w++;
				}
			}
			cprintf(stdout, "\r\n");
		}
		(void) fflush(stdout);
		reply(214, "Direct comments to ftp-bugs@%s.", hostname);
		return;
	}
	c = lookup(ctab, s);
	if (c == (struct tab *)0) {
		reply(502, "Unknown command %s.", s);
		return;
	}
	if (CMD_IMPLEMENTED(c))
		reply(214, "Syntax: %s%s %s", type, c->name, c->help);
	else
		reply(214, "%s%-*s\t%s; not implemented.", type, width,
		    c->name, c->help);
}
