/*	$NetBSD: dig8.c,v 1.1.1.1.14.1 2012/10/30 18:55:44 yamt Exp $	*/

/*
 * Copyright (C) 2009  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef lint
static const char rcsid[] = "Id: dig8.c,v 1.4 2009/03/03 23:49:07 tbox Exp ";
#endif

/*
 * Copyright (c) 1989
 *    The Regents of the University of California.  All rights reserved.
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
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
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
 */

/*
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (c) 1996-1999 by Internet Software Consortium
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
 */

/*********************** Notes for the BIND 4.9 release (Paul Vixie, DEC)
 *	dig 2.0 was written by copying sections of libresolv.a and nslookup
 *	and modifying them to be more useful for a general lookup utility.
 *	as of BIND 4.9, the changes needed to support dig have mostly been
 *	incorporated into libresolv.a and nslookup; dig now links against
 *	some of nslookup's .o files rather than #including them or maintaining
 *	local copies of them.
 *
 *	while merging dig back into the BIND release, i made a number of
 *	structural changes.  for one thing, i put all of dig's private
 *	library routines into this file rather than maintaining them in
 *	separate, #included, files.  i don't like to #include ".c" files.
 *	i removed all calls to "bcopy", replacing them with structure
 *	assignments.  i removed all "extern"'s of standard functions,
 *	replacing them with #include's of standard header files.  this
 *	version of dig is probably as portable as the rest of BIND.
 *
 *	i had to remove the query-time and packet-count statistics since
 *	the current libresolv.a is a lot harder to modify to maintain these
 *	than the 4.8 one (used in the original dig) was.  for consolation,
 *	i added a "usage" message with extensive help text.
 *
 *	to save my (limited, albeit) sanity, i ran "indent" over the source.
 *	i also added the standard berkeley/DEC copyrights, since this file now
 *	contains a fair amount of non-USC code.  note that the berkeley and
 *	DEC copyrights do not prohibit redistribution, with or without fee;
 *	we add them only to protect ourselves (you have to claim copyright
 *	in order to disclaim liability and warranty).
 *
 *	Paul Vixie, Palo Alto, CA, April 1993
 ****************************************************************************

 ******************************************************************
 *      DiG -- Domain Information Groper                          *
 *                                                                *
 *        dig.c - Version 2.1 (7/12/94) ("BIND takeover")         *
 *                                                                *
 *        Developed by: Steve Hotz & Paul Mockapetris             *
 *        USC Information Sciences Institute (USC-ISI)            *
 *        Marina del Rey, California                              *
 *        1989                                                    *
 *                                                                *
 *        dig.c -                                                 *
 *           Version 2.0 (9/1/90)                                 *
 *               o renamed difftime() difftv() to avoid           *
 *                 clash with ANSI C                              *
 *               o fixed incorrect # args to strcmp,gettimeofday  *
 *               o incorrect length specified to strncmp          *
 *               o fixed broken -sticky -envsa -envset functions  *
 *               o print options/flags redefined & modified       *
 *                                                                *
 *           Version 2.0.beta (5/9/90)                            *
 *               o output format - helpful to `doc`               *
 *               o minor cleanup                                  *
 *               o release to beta testers                        *
 *                                                                *
 *           Version 1.1.beta (10/26/89)                          *
 *               o hanging zone transer (when REFUSED) fixed      *
 *               o trailing dot added to domain names in RDATA    *
 *               o ISI internal                                   *
 *                                                                *
 *           Version 1.0.tmp  (8/27/89)                           *
 *               o Error in prnttime() fixed                      *
 *               o no longer dumps core on large pkts             *
 *               o zone transfer (axfr) added                     *
 *               o -x added for inverse queries                   *
 *                               (i.e. "dig -x 128.9.0.32")       *
 *               o give address of default server                 *
 *               o accept broadcast to server @255.255.255.255    *
 *                                                                *
 *           Version 1.0  (3/27/89)                               *
 *               o original release                               *
 *                                                                *
 *     DiG is Public Domain, and may be used for any purpose as   *
 *     long as this notice is not removed.                        *
 ******************************************************************/

/* Import. */

#include "port_before.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <isc/dst.h>

#include <assert.h>
#include <ctype.h> 
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>	/* time(2), ctime(3) */

#include "port_after.h"

#include <resolv.h>

#include "res.h"

/* Global. */

#define VERSION 84
#define VSTRING "8.4"

#define PRF_DEF		(RES_PRF_STATS | RES_PRF_CMD | RES_PRF_QUES | \
			 RES_PRF_ANS | RES_PRF_AUTH | RES_PRF_ADD | \
			 RES_PRF_HEAD1 | RES_PRF_HEAD2 | RES_PRF_TTLID | \
			 RES_PRF_HEADX | RES_PRF_REPLY | RES_PRF_TRUNC)
#define PRF_MIN		(RES_PRF_QUES | RES_PRF_ANS | RES_PRF_HEAD1 | \
			 RES_PRF_HEADX | RES_PRF_REPLY | RES_PRF_TRUNC)
#define PRF_ZONE        (RES_PRF_STATS | RES_PRF_CMD | RES_PRF_QUES | \
			 RES_PRF_ANS | RES_PRF_AUTH | RES_PRF_ADD | \
			 RES_PRF_TTLID | RES_PRF_REPLY | RES_PRF_TRUNC)

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

#define SAVEENV "DiG.env"
#define DIG_MAXARGS 30

#ifndef DIG_PING
#define DIG_PING "ping"
#endif
#ifndef DIG_TAIL
#define DIG_TAIL "tail"
#endif
#ifndef DIG_PINGFMT
#define DIG_PINGFMT "%s -s %s 56 3 | %s -3"
#endif

static int		eecode = 0;
static FILE *		qfp;
static char		myhostname[MAXHOSTNAMELEN];
static struct sockaddr_in myaddress;
static struct sockaddr_in6 myaddress6;
static u_int32_t	ixfr_serial;
static char		ubuf[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:123.123.123.123")];

/* stuff for nslookup modules */
struct __res_state  res;
FILE		*filePtr;
jmp_buf		env;
HostInfo	*defaultPtr = NULL;
HostInfo	curHostInfo, defaultRec;
int		curHostValid = FALSE;
int		queryType, queryClass;
extern int	StringToClass(), StringToType();	/* subr.c */
#if defined(BSD) && BSD >= 199006 && !defined(RISCOS_BSD)
FILE		*yyin = NULL;
void		yyrestart(FILE *f);
void		yyrestart(FILE *f) { UNUSED(f); }
#endif
char		*pager = NULL;
/* end of nslookup stuff */

/* Forward. */

static void		Usage(void);
static int		setopt(const char *);
static void		res_re_init(void);
static int		xstrtonum(char *);
static int		printZone(ns_type, const char *,
				  const struct sockaddr_in *, ns_tsig_key *);
static int		print_axfr(FILE *output, const u_char *msg,
				   size_t msglen);
static struct timeval	difftv(struct timeval, struct timeval);
static void		prnttime(struct timeval);
static void		stackarg(char *, char **);
static void		reverse6(char *, struct in6_addr *);

/* Public. */

int
main(int argc, char **argv) {
	short port = htons(NAMESERVER_PORT);
	short lport;
	/* Wierd stuff for SPARC alignment, hurts nothing else. */
	union {
		HEADER header_;
		u_char packet_[PACKETSZ];
	} packet_;
#define header (packet_.header_)
#define	packet (packet_.packet_)
	union {
		HEADER u;
		u_char b[NS_MAXMSG];
	} answer;
	int n;
	char doping[90];
	char pingstr[50];
	char *afile;
	char *addrc, *addrend, *addrbegin;

	time_t exectime;
	struct timeval tv1, tv2, start_time, end_time, query_time;

	char *srv;
	int anyflag = 0;
	int sticky = 0;
	int tmp; 
	int qtypeSet;
	ns_type xfr = ns_t_invalid;
        int bytes_out, bytes_in;

	char cmd[512];
	char domain[MAXDNAME];
        char msg[120], **vtmp;
	char *args[DIG_MAXARGS];
	char **ax;
	int once = 1, dofile = 0; /* batch -vs- interactive control */
	char fileq[384];
	int  fp;
	int wait=0, delay;
	int envset=0, envsave=0;
	struct __res_state res_x, res_t;
	int r;
	struct in6_addr in6;

	ns_tsig_key key;
	char *keyfile = NULL, *keyname = NULL;
	const char *pingfmt = NULL;

	UNUSED(argc);

	res_ninit(&res);
	res.pfcode = PRF_DEF;
	qtypeSet = 0;
	memset(domain, 0, sizeof domain);
	gethostname(myhostname, (sizeof myhostname));
#ifdef HAVE_SA_LEN
	myaddress.sin_len = sizeof(struct sockaddr_in);
#endif
	myaddress.sin_family = AF_INET;
	myaddress.sin_addr.s_addr = INADDR_ANY;
	myaddress.sin_port = 0; /*INPORT_ANY*/;

#ifdef HAVE_SA_LEN
	myaddress6.sin6_len = sizeof(struct sockaddr_in6);
#endif
	myaddress6.sin6_family = AF_INET6;
	myaddress6.sin6_addr = in6addr_any;
	myaddress6.sin6_port = 0; /*INPORT_ANY*/;

	res_x = res;

/*
 * If LOCALDEF in environment, should point to file
 * containing local favourite defaults.  Also look for file
 * DiG.env (i.e. SAVEENV) in local directory.
 */

	if ((((afile = (char *) getenv("LOCALDEF")) != (char *) NULL) &&
	     ((fp = open(afile, O_RDONLY)) > 0)) ||
	    ((fp = open(SAVEENV, O_RDONLY)) > 0)) {
		read(fp, (char *)&res_x, (sizeof res_x));
		close(fp);
		res = res_x;
	}
/*
 * Check for batch-mode DiG; also pre-scan for 'help'.
 */
	vtmp = argv;
	ax = args;
	while (*vtmp != NULL) {
		if (strcmp(*vtmp, "-h") == 0 ||
		    strcmp(*vtmp, "-help") == 0 ||
		    strcmp(*vtmp, "-usage") == 0 ||
		    strcmp(*vtmp, "help") == 0) {
			Usage();
			exit(0);
		}

		if (strcmp(*vtmp, "-f") == 0) {
			dofile++; once=0;
			if ((qfp = fopen(*++vtmp, "r")) == NULL) {
				fflush(stdout);
				perror("file open");
				fflush(stderr);
				exit(10);
			}
		} else {
			if (ax - args == DIG_MAXARGS) {
				fprintf(stderr, "dig: too many arguments\n");
				exit(10);
			}
			*ax++ = *vtmp;
		}
		vtmp++;
	}

	gettimeofday(&tv1, NULL);

/*
 * Main section: once if cmd-line query
 *               while !EOF if batch mode
 */
	*fileq = '\0';
	while ((dofile && fgets(fileq, sizeof fileq, qfp) != NULL) || 
	       (!dofile && once--)) 
	{
		if (*fileq == '\n' || *fileq == '#' || *fileq==';') {
			printf("%s", fileq);	/* echo but otherwise ignore */
			continue;		/* blank lines and comments  */
		}

/*
 * "Sticky" requests that before current parsing args
 * return to current "working" environment (X******).
 */
		if (sticky) {
			printf(";; (using sticky settings)\n");
			res = res_x;
		}

/*
 * Concat cmd-line and file args.
 */
		stackarg(fileq, ax);

		/* defaults */
		queryType = ns_t_ns;
		queryClass = ns_c_in;
		xfr = ns_t_invalid;
		*pingstr = 0;
		srv = NULL;

		sprintf(cmd, "\n; <<>> DiG %s (libbind %d) <<>> ",
			VSTRING, __RES);
		argv = args;
		/* argc = ax - args; */
/*
 * More cmd-line options than anyone should ever have to
 * deal with ....
 */
		while (*(++argv) != NULL && **argv != '\0') { 
			if (strlen(cmd) + strlen(*argv) + 2 > sizeof (cmd)) {
				fprintf(stderr,
				   "Argument too large for input buffer\n");
				exit(1);
			}
			strcat(cmd, *argv);
			strcat(cmd, " ");
			if (**argv == '@') {
				srv = (*argv+1);
				continue;
			}
			if (**argv == '%')
				continue;
			if (**argv == '+') {
				setopt(*argv+1);
				continue;
			}
			if (**argv == '=') {
				ixfr_serial = strtoul(*argv+1, NULL, 0);
				continue;
			}
			if (strncmp(*argv, "-nost", 5) == 0) {
				sticky = 0;
				continue;
			} else if (strncmp(*argv, "-st", 3) == 0) {
				sticky++;
				continue;
			} else if (strncmp(*argv, "-envsa", 6) == 0) {
				envsave++;
				continue;
			} else if (strncmp(*argv, "-envse", 6) == 0) {
				envset++;
				continue;
			}

			if (**argv == '-') {
				switch (argv[0][1]) { 
				case 'T':
					if (*++argv == NULL)
						printf("; no arg for -T?\n");
					else
						wait = atoi(*argv);
					break;
				case 'c': 
					if(*++argv == NULL) 
						printf("; no arg for -c?\n");
					else if ((tmp = atoi(*argv))
						  || *argv[0] == '0') {
						queryClass = tmp;
					} else if ((tmp = StringToClass(*argv,
								       0, NULL)
						   ) != 0) {
						queryClass = tmp;
					} else {
						printf(
						  "; invalid class specified\n"
						       );
					}
					break;
				case 't': 
					if (*++argv == NULL)
						printf("; no arg for -t?\n");
					else if ((tmp = atoi(*argv))
					    || *argv[0]=='0') {
						if (ns_t_xfr_p(tmp)) {
							xfr = tmp;
						} else {
							queryType = tmp;
							qtypeSet++;
						}
					} else if ((tmp = StringToType(*argv,
								      0, NULL)
						   ) != 0) {
						if (ns_t_xfr_p(tmp)) {
							xfr = tmp;
						} else {
							queryType = tmp;
							qtypeSet++;
						}
					} else {
						printf(
						   "; invalid type specified\n"
						       );
					}
					break;
				case 'x':
					if (!qtypeSet) {
						queryType = T_ANY;
						qtypeSet++;
					}
					if ((addrc = *++argv) == NULL) {
						printf("; no arg for -x?\n");
						break;
					}
					r = inet_pton(AF_INET6, addrc, &in6);
					if (r > 0) {
						reverse6(domain, &in6);
						break;
					}
					addrend = addrc + strlen(addrc);
					if (*addrend == '.')
						*addrend = '\0';
					*domain = '\0';
					while ((addrbegin = strrchr(addrc,'.'))) {
						strcat(domain, addrbegin+1);
						strcat(domain, ".");
						*addrbegin = '\0';
					}
					strcat(domain, addrc);
					strcat(domain, ".in-addr.arpa.");
					break;
				case 'p':
					if (argv[0][2] != '\0')
						port = htons(atoi(argv[0]+2));
					else if (*++argv == NULL)
						printf("; no arg for -p?\n");
					else
						port = htons(atoi(*argv));
					break;
				case 'P':
					if (argv[0][2] != '\0') {
						strcpy(pingstr, argv[0]+2);
						pingfmt =
							"%s %s 56 3 | %s -3";
					} else {
						strcpy(pingstr, DIG_PING);
						pingfmt = DIG_PINGFMT;
					}
					break;
				case 'n':
					if (argv[0][2] != '\0')
						res.ndots = atoi(argv[0]+2);
					else if (*++argv == NULL)
						printf("; no arg for -n?\n");
					else
						res.ndots = atoi(*argv);
					break;
				case 'b': {
					char *a, *p;

					if (argv[0][2] != '\0')
						a = argv[0]+2;
					else if (*++argv == NULL) {
						printf("; no arg for -b?\n");
						break;
					} else
						a = *argv;
					if ((p = strchr(a, ':')) != NULL) {
						*p++ = '\0';
						lport = htons(atoi(p));
					} else
						lport = htons(0);
					if (inet_pton(AF_INET6, a,
					      &myaddress6.sin6_addr) == 1) {
					      myaddress6.sin6_port = lport;
					} else if (!inet_aton(a,
						   &myaddress.sin_addr)) {
						fprintf(stderr,
							";; bad -b addr\n");
						exit(1);
					} else
						myaddress.sin_port = lport;
				    }
				    break;
				case 'k':
					/* -k keydir:keyname */
					
					if (argv[0][2] != '\0')
						keyfile = argv[0]+2;
					else if (*++argv == NULL) {
						printf("; no arg for -k?\n");
						break;
					} else
						keyfile = *argv;

					keyname = strchr(keyfile, ':');
					if (keyname == NULL) {
						fprintf(stderr,
			     "key option argument should be keydir:keyname\n");
						exit(1);
					}
					*keyname++='\0';
					break;
				} /* switch - */
				continue;
			} /* if '-'   */

			if ((tmp = StringToType(*argv, -1, NULL)) != -1) { 
				if ((T_ANY == tmp) && anyflag++) {  
					queryClass = C_ANY; 	
					continue; 
				}
				if (ns_t_xfr_p(tmp) &&
				    (tmp == ns_t_axfr ||
				     (res.options & RES_USEVC) != 0)
				     ) {
					res.pfcode = PRF_ZONE;
					xfr = (ns_type)tmp;
				} else {
					queryType = tmp; 
					qtypeSet++;
				}
			} else if ((tmp = StringToClass(*argv, -1, NULL))
				   != -1) { 
				queryClass = tmp; 
			} else {
				memset(domain, 0, sizeof domain);
				sprintf(domain,"%s",*argv);
			}
		} /* while argv remains */

		/* process key options */
		if (keyfile) {
#ifdef PARSE_KEYFILE
			int i, n1;
			char buf[BUFSIZ], *p;
			FILE *fp = NULL;
			int file_major, file_minor, alg;

			fp = fopen(keyfile, "r");
			if (fp == NULL) {
				perror(keyfile);
				exit(1);
			}
			/* Now read the header info from the file. */
			i = fread(buf, 1, BUFSIZ, fp);
			if (i < 5) {
				fclose(fp);
	                	exit(1);
	        	}
			fclose(fp);
	
			p = buf;
	
			n=strlen(p);		/* get length of strings */
			n1=strlen("Private-key-format: v");
			if (n1 > n ||
			    strncmp(buf, "Private-key-format: v", n1)) {
				fprintf(stderr, "Invalid key file format\n");
				exit(1);	/* not a match */
			}
			p+=n1;		/* advance pointer */
			sscanf((char *)p, "%d.%d", &file_major, &file_minor);
			/* should do some error checking with these someday */
			while (*p++!='\n');	/* skip to end of line */
	
	        	n=strlen(p);		/* get length of strings */
	        	n1=strlen("Algorithm: ");
	        	if (n1 > n || strncmp(p, "Algorithm: ", n1)) {
				fprintf(stderr, "Invalid key file format\n");
	                	exit(1);	/* not a match */
			}
			p+=n1;		/* advance pointer */
			if (sscanf((char *)p, "%d", &alg)!=1) {
				fprintf(stderr, "Invalid key file format\n");
				exit(1);
			}
			while (*p++!='\n');	/* skip to end of line */
	
	        	n=strlen(p);		/* get length of strings */
	        	n1=strlen("Key: ");
	        	if (n1 > n || strncmp(p, "Key: ", n1)) {
				fprintf(stderr, "Invalid key file format\n");
				exit(1);	/* not a match */
			}
			p+=n1;		/* advance pointer */
			pp=p;
			while (*pp++!='\n');	/* skip to end of line,
						 * terminate it */
			*--pp='\0';
	
			key.data=malloc(1024*sizeof(char));
			key.len=b64_pton(p, key.data, 1024);
	
			strcpy(key.name, keyname);
			strcpy(key.alg, "HMAC-MD5.SIG-ALG.REG.INT");
#else
			/* use the dst* routines to parse the key files
			 * 
			 * This requires that both the .key and the .private
			 * files exist in your cwd, so the keyfile parmeter
			 * here is assumed to be a path in which the
			 * K*.{key,private} files exist.
			 */
			DST_KEY *dst_key;
			char cwd[PATH_MAX+1];
	
			if (getcwd(cwd, PATH_MAX)==NULL) {
				perror("unable to get current directory");
				exit(1);
			}
			if (chdir(keyfile)<0) {
				fprintf(stderr,
					"unable to chdir to %s: %s\n", keyfile,
					strerror(errno));
				exit(1);
			}
	
			dst_init();
			dst_key = dst_read_key(keyname,
					       0 /* not used for priv keys */,
					       KEY_HMAC_MD5, DST_PRIVATE);
			if (!dst_key) {
				fprintf(stderr,
					"dst_read_key: error reading key\n");
				exit(1);
			}
			key.data=malloc(1024*sizeof(char));
			dst_key_to_buffer(dst_key, key.data, 1024);
			key.len=dst_key->dk_key_size;
	
			strcpy(key.name, keyname);
			strcpy(key.alg, "HMAC-MD5.SIG-ALG.REG.INT");
	
			if (chdir(cwd)<0) {
				fprintf(stderr, "unable to chdir to %s: %s\n",
					cwd, strerror(errno));
				exit(1);
			}
#endif
		}

		if (res.pfcode & 0x80000)
			printf("; pfcode: %08lx, options: %08lx\n",
			       (unsigned long)res.pfcode,
			       (unsigned long)res.options);
	  
/*
 * Current env. (after this parse) is to become the
 * new "working" environmnet. Used in conj. with sticky.
 */
		if (envset) {
			res_x = res;
			envset = 0;
		}

/*
 * Current env. (after this parse) is to become the
 * new default saved environmnet. Save in user specified
 * file if exists else is SAVEENV (== "DiG.env").
 */
		if (envsave) {
			afile = (char *) getenv("LOCALDEF");
			if ((afile &&
			     ((fp = open(afile,
					 O_WRONLY|O_CREAT|O_TRUNC,
					 S_IREAD|S_IWRITE)) > 0))
			    ||
			    ((fp = open(SAVEENV,
					O_WRONLY|O_CREAT|O_TRUNC,
					S_IREAD|S_IWRITE)) > 0)) {
				write(fp, (char *)&res, (sizeof res));
				close(fp);
			}
			envsave = 0;
		}

		if (res.pfcode & RES_PRF_CMD)
			printf("%s\n", cmd);

		anyflag = 0;

/*
 * Find address of server to query. If not dot-notation, then
 * try to resolve domain-name (if so, save and turn off print 
 * options, this domain-query is not the one we want. Restore
 * user options when done.
 * Things get a bit wierd since we need to use resolver to be
 * able to "put the resolver to work".
 */

		if (srv != NULL) {
			int nscount = 0;
			union res_sockaddr_union u[MAXNS];
			struct addrinfo *answer = NULL;
			struct addrinfo *cur = NULL;
			struct addrinfo hint;

			memset(u, 0, sizeof(u));
			res_t = res;
			res_ninit(&res);
			res.pfcode = 0;
			res.options = RES_DEFAULT;
			memset(&hint, 0, sizeof(hint));
			hint.ai_socktype = SOCK_DGRAM;
			if (!getaddrinfo(srv, NULL, &hint, &answer)) {
				res = res_t;
				cur = answer;
				for (cur = answer;
				     cur != NULL;
				     cur = cur->ai_next) {
					if (nscount == MAXNS)
						break;
					switch (cur->ai_addr->sa_family) {
					case AF_INET6:
						u[nscount].sin6 =
					  *(struct sockaddr_in6*)cur->ai_addr;
						u[nscount++].sin6.sin6_port =
							port;
						break;
					case AF_INET:
						u[nscount].sin =
					   *(struct sockaddr_in*)cur->ai_addr;
						u[nscount++].sin.sin_port =
							port;
						break;
					}
				}
				if (nscount != 0)
					res_setservers(&res, u, nscount);
				freeaddrinfo(answer);
			} else {
				res = res_t;
				fflush(stdout);
				fprintf(stderr,
		"; Bad server: %s -- using default server and timer opts\n",
						srv);
				fflush(stderr);
				srv = NULL;
			}
			printf("; (%d server%s found)\n",
			       res.nscount, (res.nscount==1)?"":"s");
			res.id += res.retry;
		}

		if (ns_t_xfr_p(xfr)) {
			int i;
			int nscount;
			union res_sockaddr_union u[MAXNS];
			nscount = res_getservers(&res, u, MAXNS);
			for (i = 0; i < nscount; i++) {
				int x;

				if (keyfile)
					x = printZone(xfr, domain,
						      &u[i].sin,
						      &key);
				else
					x = printZone(xfr, domain,
						      &u[i].sin,
						      NULL);
				if (res.pfcode & RES_PRF_STATS) {
					exectime = time(NULL);
					printf(";; FROM: %s to SERVER: %s\n",
					       myhostname,
					       p_sockun(u[i], ubuf,
							sizeof(ubuf)));
					printf(";; WHEN: %s", ctime(&exectime));
				}
				if (!x)
					break;	/* success */
			}
			fflush(stdout);
			continue;
		}

		if (*domain && !qtypeSet) {
			queryType = T_A;
			qtypeSet++;
		}
		
		bytes_out = n = res_nmkquery(&res, QUERY, domain,
					     queryClass, queryType,
					     NULL, 0, NULL,
					     packet, sizeof packet);
		if (n < 0) {
			fflush(stderr);
			printf(";; res_nmkquery: buffer too small\n\n");
			fflush(stdout);
			continue;
		}
		if (queryType == T_IXFR) {
			HEADER *hp = (HEADER *) packet;
			u_char *cpp = packet + bytes_out;

			hp->nscount = htons(1+ntohs(hp->nscount));
			n = dn_comp(domain, cpp,
				    (sizeof packet) - (cpp - packet),
				    NULL, NULL);
			cpp += n;
			PUTSHORT(T_SOA, cpp); /* type */
			PUTSHORT(C_IN, cpp);  /* class */
			PUTLONG(0, cpp);      /* ttl */
			PUTSHORT(22, cpp);    /* dlen */
			*cpp++ = 0;           /* mname */
			*cpp++ = 0;           /* rname */
			PUTLONG(ixfr_serial, cpp);
			PUTLONG(0xDEAD, cpp); /* Refresh */
			PUTLONG(0xBEEF, cpp); /* Retry */
			PUTLONG(0xABCD, cpp); /* Expire */
			PUTLONG(0x1776, cpp); /* Min TTL */
			bytes_out = n = cpp - packet;
		};	

#if defined(RES_USE_EDNS0) && defined(RES_USE_DNSSEC)
		if (n > 0 &&
		    (res.options & (RES_USE_EDNS0|RES_USE_DNSSEC)) != 0)
			bytes_out = n = res_nopt(&res, n, packet,
						 sizeof(packet), 4096);
#endif

		eecode = 0;
		if (res.pfcode & RES_PRF_HEAD1)
			fp_resstat(&res, stdout);
		(void) gettimeofday(&start_time, NULL);
		if (keyfile)
			n = res_nsendsigned(&res, packet, n, &key,
					    answer.b, sizeof(answer.b));
		else
			n = res_nsend(&res, packet, n,
				      answer.b, sizeof(answer.b));
		if ((bytes_in = n) < 0) {
			fflush(stdout);
			n = 0 - n;
			if (keyfile)
				strcpy(msg, ";; res_nsendsigned");
			else
				strcpy(msg, ";; res_nsend");
			perror(msg);
			fflush(stderr);

			if (!dofile) {
				if (eecode)
					exit(eecode);
				else
					exit(9);
			}
		}
		(void) gettimeofday(&end_time, NULL);

		if (res.pfcode & RES_PRF_STATS) {
			union res_sockaddr_union u[MAXNS];

			(void) res_getservers(&res, u, MAXNS);
			query_time = difftv(start_time, end_time);
			printf(";; Total query time: ");
			prnttime(query_time);
			putchar('\n');
			exectime = time(NULL);
			printf(";; FROM: %s to SERVER: %s\n", myhostname,
			       p_sockun(u[RES_GETLAST(res)],
					ubuf, sizeof(ubuf)));
			printf(";; WHEN: %s", ctime(&exectime));
			printf(";; MSG SIZE  sent: %d  rcvd: %d\n",
			       bytes_out, bytes_in);
		}
	  
		fflush(stdout);
/*
 *   Argh ... not particularly elegant. Should put in *real* ping code.
 *   Would necessitate root priviledges for icmp port though!
 */
		if (*pingstr && srv != NULL) {
			sprintf(doping, pingfmt, pingstr, srv, DIG_TAIL);
			system(doping);
		}
		putchar('\n');

/*
 * Fairly crude method and low overhead method of keeping two
 * batches started at different sites somewhat synchronized.
 */
		gettimeofday(&tv2, NULL);
		delay = (int)(tv2.tv_sec - tv1.tv_sec);
		if (delay < wait) {
			sleep(wait - delay);
		}
		tv1 = tv2;
	}
	return (eecode);
}

/* Private. */

static void
Usage() {
	fputs("\
usage:  dig [@server] [domain] [q-type] [q-class] {q-opt} {d-opt} [%comment]\n\
where:	server,\n\
	domain	are names in the Domain Name System\n\
	q-class	is one of (in,any,...) [default: in]\n\
	q-type	is one of (a,any,mx,ns,soa,hinfo,axfr,txt,...) [default: a]\n\
", stderr);
	fputs("\
	q-opt	is one of:\n\
		-x dot-notation-address	(shortcut to in-addr.arpa lookups)\n\
		-f file			(batch mode input file name)\n\
		-T time			(batch mode time delay, per query)\n\
		-p port			(nameserver is on this port) [53]\n\
		-b addr[:port]		(bind AXFR to this tcp address) [*]\n\
		-P[ping-string]		(see man page)\n\
		-t query-type		(synonym for q-type)\n\
		-c query-class		(synonym for q-class)\n\
		-k keydir:keyname	(sign the query with this TSIG key)\n\
		-envsav,-envset		(see man page)\n\
		-[no]stick		(see man page)\n\
", stderr);
	fputs("\
	d-opt	is of the form ``+keyword=value'' where keyword is one of:\n\
		[no]debug [no]d2 [no]recurse retry=# time=# [no]ko [no]vc\n\
		[no]defname [no]search domain=NAME [no]ignore [no]primary\n\
		[no]aaonly [no]cmd [no]stats [no]Header [no]header [no]trunc\n\
		[no]ttlid [no]cl [no]qr [no]reply [no]ques [no]answer\n\
		[no]author [no]addit [no]dnssec pfdef pfmin\n\
		pfset=# pfand=# pfor=#\n\
", stderr);
	fprintf(stderr, "\
notes:	defname and search don't work; use fully-qualified names.\n\
	this is DiG version %s (libbind %d)\n\
	Id: dig8.c,v 1.4 2009/03/03 23:49:07 tbox Exp \n", VSTRING, __RES);
}

static int
setopt(const char *string) {
	char option[NAME_LEN], *ptr;
	int i;

	i = pickString(string, option, sizeof option);
	if (i == 0) {
		fprintf(stderr, ";*** Invalid option: %s\n",  string);

		/* this is ugly, but fixing the caller to behave
		   properly with an error return value would require a major
		   cleanup. */
		exit(9);
	} 
   
	if (strncmp(option, "aa", 2) == 0) {	/* aaonly */
		res.options |= RES_AAONLY;
	} else if (strncmp(option, "noaa", 4) == 0) {
		res.options &= ~RES_AAONLY;
	} else if (strncmp(option, "deb", 3) == 0) {	/* debug */
		res.options |= RES_DEBUG;
	} else if (strncmp(option, "nodeb", 5) == 0) {
		res.options &= ~(RES_DEBUG | RES_DEBUG2);
	} else if (strncmp(option, "ko", 2) == 0) {	/* keepopen */
		res.options |= (RES_STAYOPEN | RES_USEVC);
	} else if (strncmp(option, "noko", 4) == 0) {
		res.options &= ~RES_STAYOPEN;
	} else if (strncmp(option, "d2", 2) == 0) {	/* d2 (more debug) */
		res.options |= (RES_DEBUG | RES_DEBUG2);
	} else if (strncmp(option, "nod2", 4) == 0) {
		res.options &= ~RES_DEBUG2;
	} else if (strncmp(option, "def", 3) == 0) {	/* defname */
		res.options |= RES_DEFNAMES;
	} else if (strncmp(option, "nodef", 5) == 0) {
		res.options &= ~RES_DEFNAMES;
	} else if (strncmp(option, "dn", 2) == 0) {	/* dnssec */
		res.options |= RES_USE_DNSSEC;
	} else if (strncmp(option, "nodn", 4) == 0) {
		res.options &= ~RES_USE_DNSSEC;
	} else if (strncmp(option, "sea", 3) == 0) {	/* search list */
		res.options |= RES_DNSRCH;
	} else if (strncmp(option, "nosea", 5) == 0) {
		res.options &= ~RES_DNSRCH;
	} else if (strncmp(option, "do", 2) == 0) {	/* domain */
		ptr = strchr(option, '=');
		if (ptr != NULL) {
			i = pickString(++ptr, res.defdname, sizeof res.defdname);
			if (i == 0) { /* value's too long or non-existant. This actually
					 shouldn't happen due to pickString()
					 above */
				fprintf(stderr, "*** Invalid domain: %s\n", ptr) ;
				exit(9); /* see comment at previous call to exit()*/
			}
		}
	} else if (strncmp(option, "ti", 2) == 0) {      /* timeout */
		ptr = strchr(option, '=');
		if (ptr != NULL)
			sscanf(++ptr, "%d", &res.retrans);
	} else if (strncmp(option, "ret", 3) == 0) {    /* retry */
		ptr = strchr(option, '=');
		if (ptr != NULL)
			sscanf(++ptr, "%d", &res.retry);
	} else if (strncmp(option, "i", 1) == 0) {	/* ignore */
		res.options |= RES_IGNTC;
	} else if (strncmp(option, "noi", 3) == 0) {
		res.options &= ~RES_IGNTC;
	} else if (strncmp(option, "pr", 2) == 0) {	/* primary */
		res.options |= RES_PRIMARY;
	} else if (strncmp(option, "nop", 3) == 0) {
		res.options &= ~RES_PRIMARY;
	} else if (strncmp(option, "rec", 3) == 0) {	/* recurse */
		res.options |= RES_RECURSE;
	} else if (strncmp(option, "norec", 5) == 0) {
		res.options &= ~RES_RECURSE;
	} else if (strncmp(option, "v", 1) == 0) {	/* vc */
		res.options |= RES_USEVC;
	} else if (strncmp(option, "nov", 3) == 0) {
		res.options &= ~RES_USEVC;
	} else if (strncmp(option, "pfset", 5) == 0) {
		ptr = strchr(option, '=');
		if (ptr != NULL)
			res.pfcode = xstrtonum(++ptr);
	} else if (strncmp(option, "pfand", 5) == 0) {
		ptr = strchr(option, '=');
		if (ptr != NULL)
			res.pfcode = res.pfcode & xstrtonum(++ptr);
	} else if (strncmp(option, "pfor", 4) == 0) {
		ptr = strchr(option, '=');
		if (ptr != NULL)
			res.pfcode |= xstrtonum(++ptr);
	} else if (strncmp(option, "pfmin", 5) == 0) {
		res.pfcode = PRF_MIN;
	} else if (strncmp(option, "pfdef", 5) == 0) {
		res.pfcode = PRF_DEF;
	} else if (strncmp(option, "an", 2) == 0) {  /* answer section */
		res.pfcode |= RES_PRF_ANS;
	} else if (strncmp(option, "noan", 4) == 0) {
		res.pfcode &= ~RES_PRF_ANS;
	} else if (strncmp(option, "qu", 2) == 0) {  /* question section */
		res.pfcode |= RES_PRF_QUES;
	} else if (strncmp(option, "noqu", 4) == 0) {  
		res.pfcode &= ~RES_PRF_QUES;
	} else if (strncmp(option, "au", 2) == 0) {  /* authority section */
		res.pfcode |= RES_PRF_AUTH;
	} else if (strncmp(option, "noau", 4) == 0) {  
		res.pfcode &= ~RES_PRF_AUTH;
	} else if (strncmp(option, "ad", 2) == 0) {  /* addition section */
		res.pfcode |= RES_PRF_ADD;
	} else if (strncmp(option, "noad", 4) == 0) {  
		res.pfcode &= ~RES_PRF_ADD;
	} else if (strncmp(option, "tt", 2) == 0) {  /* TTL & ID */
		res.pfcode |= RES_PRF_TTLID;
	} else if (strncmp(option, "nott", 4) == 0) {  
		res.pfcode &= ~RES_PRF_TTLID;
	} else if (strncmp(option, "tr", 2) == 0) {  /* TTL & ID */
		res.pfcode |= RES_PRF_TRUNC;
	} else if (strncmp(option, "notr", 4) == 0) {  
		res.pfcode &= ~RES_PRF_TRUNC;
	} else if (strncmp(option, "he", 2) == 0) {  /* head flags stats */
		res.pfcode |= RES_PRF_HEAD2;
	} else if (strncmp(option, "nohe", 4) == 0) {  
		res.pfcode &= ~RES_PRF_HEAD2;
	} else if (strncmp(option, "H", 1) == 0) {  /* header all */
		res.pfcode |= RES_PRF_HEADX;
	} else if (strncmp(option, "noH", 3) == 0) {  
		res.pfcode &= ~(RES_PRF_HEADX);
	} else if (strncmp(option, "qr", 2) == 0) {  /* query */
		res.pfcode |= RES_PRF_QUERY;
	} else if (strncmp(option, "noqr", 4) == 0) {  
		res.pfcode &= ~RES_PRF_QUERY;
	} else if (strncmp(option, "rep", 3) == 0) {  /* reply */
		res.pfcode |= RES_PRF_REPLY;
	} else if (strncmp(option, "norep", 5) == 0) {  
		res.pfcode &= ~RES_PRF_REPLY;
	} else if (strncmp(option, "cm", 2) == 0) {  /* command line */
		res.pfcode |= RES_PRF_CMD;
	} else if (strncmp(option, "nocm", 4) == 0) {  
		res.pfcode &= ~RES_PRF_CMD;
	} else if (strncmp(option, "cl", 2) == 0) {  /* class mnemonic */
		res.pfcode |= RES_PRF_CLASS;
	} else if (strncmp(option, "nocl", 4) == 0) {  
		res.pfcode &= ~RES_PRF_CLASS;
	} else if (strncmp(option, "st", 2) == 0) {  /* stats*/
		res.pfcode |= RES_PRF_STATS;
	} else if (strncmp(option, "nost", 4) == 0) {  
		res.pfcode &= ~RES_PRF_STATS;
	} else {
		fprintf(stderr, "; *** Invalid option: %s\n",  option);
		return (ERROR);
	}
	res_re_init();
	return (SUCCESS);
}

/*
 * Force a reinitialization when the domain is changed.
 */
static void
res_re_init() {
	static char localdomain[] = "LOCALDOMAIN";
	u_long pfcode = res.pfcode, options = res.options;
	unsigned ndots = res.ndots;
	int retrans = res.retrans, retry = res.retry;
	char *buf;

	/*
	 * This is ugly but putenv() is more portable than setenv().
	 */
	buf = malloc((sizeof localdomain) + strlen(res.defdname) +10/*fuzz*/);
	sprintf(buf, "%s=%s", localdomain, res.defdname);
	putenv(buf);	/* keeps the argument, so we won't free it */
	res_ninit(&res);
	res.pfcode = pfcode;
	res.options = options;
	res.ndots = ndots;
	res.retrans = retrans;
	res.retry = retry;
}

/*
 * convert char string (decimal, octal, or hex) to integer
 */
static int
xstrtonum(char *p) {
	int v = 0;
	int i;
	int b = 10;
	int flag = 0;
	while (*p != 0) {
		if (!flag++)
			if (*p == '0') {
				b = 8; p++;
				continue;
			}
		if (isupper((unsigned char)*p))
			*p = tolower(*p);
		if (*p == 'x') {
			b = 16; p++;
			continue;
		}
		if (isdigit((unsigned char)*p)) {
			i = *p - '0';
		} else if (isxdigit((unsigned char)*p)) {
			i = *p - 'a' + 10;
		} else {
			fprintf(stderr,
				"; *** Bad char in numeric string..ignored\n");
			i = -1;
		}
		if (i >= b) {
			fprintf(stderr,
				"; *** Bad char in numeric string..ignored\n");
			i = -1;
		}
		if (i >= 0)
			v = v * b + i;
		p++;
	}
	return (v);
}

typedef union {
	HEADER qb1;
	u_char qb2[PACKETSZ];
} querybuf;

static int
printZone(ns_type xfr, const char *zone, const struct sockaddr_in *sin,
	  ns_tsig_key *key)
{
	static u_char *answer = NULL;
	static int answerLen = 0;

	querybuf buf;
	int msglen, amtToRead, numRead, result, sockFD, len;
	int count, type, rlen, done, n;
	int numAnswers, numRecords, soacnt;
	u_char *cp, tmp[NS_INT16SZ];
	char dname[2][NS_MAXDNAME];
	enum { NO_ERRORS, ERR_READING_LEN, ERR_READING_MSG, ERR_PRINTING }
		error;
	pid_t zpid = -1;
	u_char *newmsg;
	int newmsglen;
	ns_tcp_tsig_state tsig_state;
	int tsig_ret, tsig_required, tsig_present;

	switch (xfr) {
	case ns_t_axfr:
	case ns_t_zxfr:
		break;
	default:
		fprintf(stderr, ";; %s - transfer type not supported\n",
			p_type(xfr));
		return (ERROR);
	}

	/*
	 *  Create a query packet for the requested zone name.
	 */
	msglen = res_nmkquery(&res, ns_o_query, zone,
			      queryClass, ns_t_axfr, NULL,
			      0, 0, buf.qb2, sizeof buf);
	if (msglen < 0) {
		if (res.options & RES_DEBUG)
			fprintf(stderr, ";; res_nmkquery failed\n");
		return (ERROR);
	}

	/*
	 * Sign the message if a key was sent
	 */
	if (key == NULL) {
		newmsg = (u_char *)&buf;
		newmsglen = msglen;
	} else {
		DST_KEY *dstkey;
		int bufsize, siglen;
		u_char sig[64];
		int ret;
		
		/* ns_sign() also calls dst_init(), but there is no harm
		 * doing it twice
		 */
		dst_init();
		
		bufsize = msglen + 1024;
		newmsg = (u_char *) malloc(bufsize);
		if (newmsg == NULL) {
			errno = ENOMEM;
			return (-1);
		}
		memcpy(newmsg, (u_char *)&buf, msglen);
		newmsglen = msglen;
		
		if (strcmp(key->alg, NS_TSIG_ALG_HMAC_MD5) != 0)
			dstkey = NULL;
		else
			dstkey = dst_buffer_to_key(key->name, KEY_HMAC_MD5,
							NS_KEY_TYPE_AUTH_ONLY,
							NS_KEY_PROT_ANY,
							key->data, key->len);
		if (dstkey == NULL) {
			errno = EINVAL;
			if (key)
				free(newmsg);
			return (-1);
		}
		
		siglen = sizeof(sig);
/* newmsglen++; */
		ret = ns_sign(newmsg, &newmsglen, bufsize, NOERROR, dstkey, NULL, 0,
		      sig, &siglen, 0);
		if (ret < 0) {
			if (key)
				free (newmsg);
			if (ret == NS_TSIG_ERROR_NO_SPACE)
				errno  = EMSGSIZE;
			else if (ret == -1)
				errno  = EINVAL;
			return (ret);
		}
		ns_verify_tcp_init(dstkey, sig, siglen, &tsig_state);
	}

	/*
	 *  Set up a virtual circuit to the server.
	 */
	if ((sockFD = socket(sin->sin_family, SOCK_STREAM, 0)) < 0) {
		int e = errno;

		perror(";; socket");
		return (e);
	}
	
	switch (sin->sin_family) {
	case AF_INET:
		if (bind(sockFD, (struct sockaddr *)&myaddress,
			 sizeof myaddress) < 0){
			int e = errno;

			fprintf(stderr, ";; bind(%s port %u): %s\n",
				inet_ntoa(myaddress.sin_addr),
				ntohs(myaddress.sin_port),
				strerror(e));
			(void) close(sockFD);
			sockFD = -1;
			return (e);
		}
		if (connect(sockFD, (const struct sockaddr *)sin,
			    sizeof *sin) < 0) {
			int e = errno;

			perror(";; connect");
			(void) close(sockFD);
			sockFD = -1;
			return (e);
		}
		break;
	case AF_INET6:
		if (bind(sockFD, (struct sockaddr *)&myaddress6,
			 sizeof myaddress6) < 0){
			int e = errno;
			char buf[80];

			fprintf(stderr, ";; bind(%s port %u): %s\n",
				inet_ntop(AF_INET6, &myaddress6.sin6_addr,
					  buf, sizeof(buf)),
				ntohs(myaddress6.sin6_port),
				strerror(e));
			(void) close(sockFD);
			sockFD = -1;
			return (e);
		}
		if (connect(sockFD, (const struct sockaddr *)sin,
			    sizeof(struct sockaddr_in6)) < 0) {
			int e = errno;

			perror(";; connect");
			(void) close(sockFD);
			sockFD = -1;
			return (e);
		}
		break;
	}

	/*
	 * Send length & message for zone transfer
	 */

	ns_put16(newmsglen, tmp);
        if (write(sockFD, (char *)tmp, NS_INT16SZ) != NS_INT16SZ ||
            write(sockFD, (char *)newmsg, newmsglen) != newmsglen) {
		int e = errno;
		if (key)
			free (newmsg);
		perror(";; write");
		(void) close(sockFD);
		sockFD = -1;
		return (e);
	} else if (key)
		free (newmsg);

	/*
	 * If we're compressing, push a gzip into the pipeline.
	 */
	if (xfr == ns_t_zxfr) {
		enum { rd = 0, wr = 1 };
		int z[2];

		if (pipe(z) < 0) {
			int e = errno;

			perror(";; pipe");
			(void) close(sockFD);
			sockFD = -1;
			return (e);
		}
		zpid = vfork();
		if (zpid < 0) {
			int e = errno;

			perror(";; fork");
			(void) close(sockFD);
			sockFD = -1;
			return (e);
		} else if (zpid == 0) {
			/* Child. */
			(void) close(z[rd]);
			(void) dup2(sockFD, STDIN_FILENO);
			(void) close(sockFD);
			(void) dup2(z[wr], STDOUT_FILENO);
			(void) close(z[wr]);
			execlp("gzip", "gzip", "-d", "-v", NULL);
			perror(";; child: execlp(gunzip)");
			_exit(1);
		}
		/* Parent. */
		(void) close(z[wr]);
		(void) dup2(z[rd], sockFD);
		(void) close(z[rd]);
	}
	result = 0;
	numAnswers = 0;
	numRecords = 0;
	soacnt = 0;
	error = NO_ERRORS;
	numRead = 0;

	dname[0][0] = '\0';
	for (done = 0; !done; (void)NULL) {
		/*
		 * Read the length of the response.
		 */

		cp = tmp;
		amtToRead = INT16SZ;
		while (amtToRead > 0 &&
		   (numRead = read(sockFD, cp, amtToRead)) > 0) {
			cp += numRead;
			amtToRead -= numRead;
		}
		if (numRead <= 0) {
			error = ERR_READING_LEN;
			break;
		}

		len = ns_get16(tmp);
		if (len == 0)
			break;	/* nothing left to read */

		/*
		 * The server sent too much data to fit the existing buffer --
		 * allocate a new one.
		 */
		if (len > answerLen) {
			if (answerLen != 0)
				free(answer);
			answerLen = len;
			answer = (u_char *)malloc(answerLen);
		}

		/*
		 * Read the response.
		 */

		amtToRead = len;
		cp = answer;
		while (amtToRead > 0 &&
		       (numRead = read(sockFD, cp, amtToRead)) > 0) {
			cp += numRead;
			amtToRead -= numRead;
		}
		if (numRead <= 0) {
			error = ERR_READING_MSG;
			break;
		}

		result = print_axfr(stdout, answer, len);
		if (result != 0) {
			error = ERR_PRINTING;
			break;
		}
		numRecords += htons(((HEADER *)answer)->ancount);
		numAnswers++;

		/* Header. */
		cp = answer + HFIXEDSZ;
		/* Question. */
		for (count = ntohs(((HEADER *)answer)->qdcount);	
		     count > 0;
		     count--) {
			n = dn_skipname(cp, answer + len);
			if (n < 0) {
				error = ERR_PRINTING;
				done++;
				break;
			}
			cp += n + QFIXEDSZ;
			if (cp > answer + len) {
				error = ERR_PRINTING;
				done++;
				break;
			}
		}
		/* Answer. */
		for (count = ntohs(((HEADER *)answer)->ancount);
		     count > 0 && !done;
		     count--) {
			n = dn_expand(answer, answer + len, cp,
				      dname[soacnt], sizeof dname[0]);
			if (n < 0) {
				error = ERR_PRINTING;
				done++;
				break;
			}
			cp += n;
			if (cp + 3 * INT16SZ + INT32SZ > answer + len) {
				error = ERR_PRINTING;
				done++;
				break;
			}
			GETSHORT(type, cp);
			cp += INT16SZ;
			cp += INT32SZ;	/* ttl */
			GETSHORT(rlen, cp);
			cp += rlen;
			if (cp > answer + len) {
				error = ERR_PRINTING;
				done++;
				break;
			}
			if (type == T_SOA && soacnt++ &&
			    ns_samename(dname[0], dname[1]) == 1) {
				done++;
				break;
			}
		}

		/*
		 * Verify the TSIG
		 */

		if (key) {
			if (ns_find_tsig(answer, answer + len) != NULL)
				tsig_present = 1;
			else
				tsig_present = 0;
			if (numAnswers == 1 || soacnt > 1)
				tsig_required = 1;
			else
				tsig_required = 0;
			tsig_ret = ns_verify_tcp(answer, &len, &tsig_state,
						 tsig_required);
			if (tsig_ret == 0) {
				if (tsig_present)
					printf("; TSIG ok\n");
			}
			else
				printf("; TSIG invalid\n");
		}

	}

	printf(";; Received %d answer%s (%d record%s).\n",
	       numAnswers, (numAnswers != 1) ? "s" : "",
	       numRecords, (numRecords != 1) ? "s" : "");

	(void) close(sockFD);
	sockFD = -1;

	/*
	 * If we were uncompressing, reap the uncompressor.
	 */
	if (xfr == ns_t_zxfr) {
		pid_t pid;
		int status = 0;

		pid = wait(&status);
		if (pid < 0) {
			int e = errno;

			perror(";; wait");
			return (e);
		}
		if (pid != zpid) {
			fprintf(stderr, ";; wrong pid (%lu != %lu)\n",
				(u_long)pid, (u_long)zpid);
			return (ERROR);
		}
		printf(";; pid %lu: exit %d, signal %d, core %c\n",
		       (u_long)pid, WEXITSTATUS(status),
		       WIFSIGNALED(status) ? WTERMSIG(status) : 0,
		       WCOREDUMP(status) ? 't' : 'f');
	}

	switch (error) {
	case NO_ERRORS:
		return (0);

	case ERR_READING_LEN:
		return (EMSGSIZE);

	case ERR_PRINTING:
		return (result);

	case ERR_READING_MSG:
		return (EMSGSIZE);

	default:
		return (EFAULT);
	}
}

static int
print_axfr(FILE *file, const u_char *msg, size_t msglen) {
	ns_msg handle;

	if (ns_initparse(msg, msglen, &handle) < 0) {
		fprintf(file, ";; ns_initparse: %s\n", strerror(errno));
		return (ns_r_formerr);
	}
	if (ns_msg_getflag(handle, ns_f_rcode) != ns_r_noerror)
		return (ns_msg_getflag(handle, ns_f_rcode));

	/*
	 * We are looking for info from answer resource records.
	 * If there aren't any, return with an error. We assume
	 * there aren't any question records.
	 */
	if (ns_msg_count(handle, ns_s_an) == 0)
		return (NO_INFO);

#ifdef PROTOCOLDEBUG
	printf(";;; (message of %d octets has %d answers)\n",
	       msglen, ns_msg_count(handle, ns_s_an));
#endif
	for (;;) {
		static char origin[NS_MAXDNAME], name_ctx[NS_MAXDNAME];
		const char *name;
		char buf[2048];		/* XXX need to malloc/realloc. */
		ns_rr rr;

		if (ns_parserr(&handle, ns_s_an, -1, &rr)) {
			if (errno != ENODEV) {
				fprintf(file, ";; ns_parserr: %s\n",
					strerror(errno));
				return (FORMERR);
			}
			break;
		}
		name = ns_rr_name(rr);
		if (origin[0] == '\0' && name[0] != '\0') {
			if (strcmp(name, ".") != 0)
				strcpy(origin, name);
			fprintf(file, "$ORIGIN %s.\n", origin);
			if (strcmp(name, ".") == 0)
				strcpy(origin, name);
			if (res.pfcode & RES_PRF_TRUNC)
				strcpy(name_ctx, "@");
		}
		if (ns_sprintrr(&handle, &rr,
				(res.pfcode & RES_PRF_TRUNC) ? name_ctx : NULL,
				(res.pfcode & RES_PRF_TRUNC) ? origin : NULL,
				buf, sizeof buf) < 0) {
			fprintf(file, ";; ns_sprintrr: %s\n", strerror(errno));
			return (FORMERR);
		}
		strcpy(name_ctx, name);
		fputs(buf, file);
		fputc('\n', file);
	}
	return (SUCCESS);
}

static struct timeval
difftv(struct timeval a, struct timeval b) {
	static struct timeval diff;

	diff.tv_sec = b.tv_sec - a.tv_sec;
	if ((diff.tv_usec = b.tv_usec - a.tv_usec) < 0) {
		diff.tv_sec--;
		diff.tv_usec += 1000000;
	}
	return (diff);
}

static void
prnttime(struct timeval t) {
	printf("%lu msec", (u_long)(t.tv_sec * 1000 + (t.tv_usec / 1000)));
}

/*
 * Take arguments appearing in simple string (from file or command line)
 * place in char**.
 */
static void
stackarg(char *l, char **y) {
	int done = 0;

	while (!done) {
		switch (*l) {
		case '\t':
		case ' ':
			l++;
			break;
		case '\0':
		case '\n':
			done++;
			*y = NULL;
			break;
		default:
			*y++ = l;
			while (!isspace((unsigned char)*l))
				l++;
			if (*l == '\n')
				done++;
			*l++ = '\0';
			*y = NULL;
		}
	}
}

static void
reverse6(char *domain, struct in6_addr *in6) {
	sprintf(domain, "%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.ip6.arpa",
		in6->s6_addr[15] & 0x0f, (in6->s6_addr[15] >> 4) & 0x0f,
		in6->s6_addr[14] & 0x0f, (in6->s6_addr[14] >> 4) & 0x0f,
		in6->s6_addr[13] & 0x0f, (in6->s6_addr[13] >> 4) & 0x0f,
		in6->s6_addr[12] & 0x0f, (in6->s6_addr[12] >> 4) & 0x0f,
		in6->s6_addr[11] & 0x0f, (in6->s6_addr[11] >> 4) & 0x0f,
		in6->s6_addr[10] & 0x0f, (in6->s6_addr[10] >> 4) & 0x0f,
		in6->s6_addr[9] & 0x0f, (in6->s6_addr[9] >> 4) & 0x0f,
		in6->s6_addr[8] & 0x0f, (in6->s6_addr[8] >> 4) & 0x0f,
		in6->s6_addr[7] & 0x0f, (in6->s6_addr[7] >> 4) & 0x0f,
		in6->s6_addr[6] & 0x0f, (in6->s6_addr[6] >> 4) & 0x0f,
		in6->s6_addr[5] & 0x0f, (in6->s6_addr[5] >> 4) & 0x0f,
		in6->s6_addr[4] & 0x0f, (in6->s6_addr[4] >> 4) & 0x0f,
		in6->s6_addr[3] & 0x0f, (in6->s6_addr[3] >> 4) & 0x0f,
		in6->s6_addr[2] & 0x0f, (in6->s6_addr[2] >> 4) & 0x0f,
		in6->s6_addr[1] & 0x0f, (in6->s6_addr[1] >> 4) & 0x0f,
		in6->s6_addr[0] & 0x0f, (in6->s6_addr[0] >> 4) & 0x0f);
}
