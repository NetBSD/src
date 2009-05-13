/*	$NetBSD: mkalias.c,v 1.15.10.1 2009/05/13 19:20:45 jym Exp $ */

/*
 * Copyright (c) 1997 Mats O Jansson <moj@stacken.kth.se>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mats O Jansson
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: mkalias.c,v 1.15.10.1 2009/05/13 19:20:45 jym Exp $");
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/nameser.h>

#include <ctype.h>
#include <err.h>
#include <netdb.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <rpc/rpc.h>

#include "protos.h"
#include "ypdb.h"
#include "ypdef.h"

void	capitalize(char *, int);
int	check_host(char *, char *, int, int, int);
int	main(int, char *[]);
void	split_address(char *, int, char *, char *);
void	usage(void);

void
split_address(char *address, int len, char *user, char *host)
{
	char *c, *s, *r;
	int  i = 0;

	if ((strchr(address, '@')) != NULL) {
		
		s = user;
		
		for(c = address; i < len; i++) {
			if (*c == '@') {
				*s = '\0';
				s = host;
			} else
				*s++ = *c;
			c++;
		}
		*s = '\0';
	}
		
	if ((r = strrchr(address, '!')) != NULL) {
		
		s = host;
		
		for(c = address; i < len; i++) {
			if (c == r) {
				*s = '\0';
				s = user;
			} else
				*s++ = *c;
			c++;
		}
		*s = '\0';
	}
}

int
check_host(char *address, char *host, int dflag, int uflag, int Eflag)
{
	u_char answer[PACKETSZ];
	int  status;

	if ((dflag && strchr(address, '@')) ||
	    (uflag && strchr(address, '!')))
		return(0);

	if ((_res.options & RES_INIT) == 0)
		res_init();

	status = res_search(host, C_IN, T_AAAA, answer, sizeof(answer));

	if (status == -1)
		status = res_search(host, C_IN, T_A, answer, sizeof(answer));

	if ((status == -1) && Eflag)
		status = res_search(host, C_IN, T_MX, answer, sizeof(answer));

	return(status == -1);
}

void
capitalize(char *name, int len)
{
	char last = ' ';
	char *c;
	int  i = 0;

	for(c = name; i < len; i++) {
		if (*c == '.') last = '.';
		c++;
	}
	
	i = 0;
	if (last == '.') {
		for(c = name; i < len; i++) {
			if (last == '.')
				*c = toupper((unsigned char)*c);
			last = *c++;
		}
	}
}

int
main(int argc, char *argv[])
{
	int	eflag = 0;
	int	dflag = 0;
	int	nflag = 0;
	int	sflag = 0;
	int	uflag = 0;
	int	vflag = 0;
	int	Eflag = 0;
	int	ch;
	char	*input = NULL;
	char	*output = NULL;
	DBM	*db;
	datum	key, val;
	char	*slash;
	DBM	*new_db = NULL;
	static	const char template[] = "ypdbXXXXXX";
	char	db_mapname[MAXPATHLEN], db_outfile[MAXPATHLEN];
	int	status;
	char	user[4096], host[4096]; /* XXX: DB bsize = 4096 in ypdb.c */
	char	datestr[11];
	char	myname[MAXHOSTNAMELEN];
	
	while ((ch = getopt(argc, argv, "Edensuv")) != -1) {
		switch(ch) {
		case 'd':
			dflag++;		/* Don't check DNS hostname */
			break;

		case 'e':
			eflag++;		/* Check hostname */
			break;

		case 'E':
			eflag++;		/* Check hostname */
			Eflag++;		/* .. even check MX records */
			break;

		case 'n':
			nflag++;		/* Capitalize name parts */
			break;

		case 's':
			sflag++;		/* Don't know... */
			break;

		case 'u':
			uflag++;		/* Don't check UUCP hostname */
			break;

		case 'v':
			vflag++;		/* Verbose */
			break;

		default:
			usage();
		}
	}

	if (optind == argc)
		usage();

	input = argv[optind++];
	if (optind < argc)
		output = argv[optind++];
	if (optind < argc)
		usage();
	
	db = ypdb_open(input);
	if (db == NULL)
		err(1, "Unable to open input database `%s'", input);

	if (output != NULL) {
		if (strlen(output) + strlen(YPDB_SUFFIX) >
		    (sizeof(db_outfile) + 1))
			warnx("file name `%s' too long", output);
		snprintf(db_outfile, sizeof(db_outfile),
			 "%s%s", output, YPDB_SUFFIX);

		slash = strrchr(output, '/');
		if (slash != NULL) 
			slash[1] = 0; 			/* truncate to dir */
		else
			*output = 0;			/* elminate */
	
		/* note: output is now directory where map goes ! */
	
		if (strlen(output) + strlen(template) + strlen(YPDB_SUFFIX) >
		    (sizeof(db_mapname) - 1))
			errx(1, "Directory name `%s' too long", output);
	
		snprintf(db_mapname, sizeof(db_mapname), "%s%s",
		    output, template);
	
		new_db = ypdb_mktemp(db_mapname);
		if (new_db == NULL)
			err(1, "Unable to open output database `%s'",
			    db_outfile);
	}

	for (key = ypdb_firstkey(db);
	     key.dptr != NULL;
	     key = ypdb_nextkey(db)) {
		
	        val = ypdb_fetch(db, key);

		if (val.dptr == NULL)
			continue;			/* No value */
		if ((*key.dptr == '@') && (key.dsize == 1))
			continue;			/* Sendmail token */
		if (strncmp(key.dptr, "YP_", 3)==0)	/* YP token */
			continue;
		if (strchr(val.dptr, ','))		/* List... */
			continue;
		if (strchr(val.dptr, '|'))		/* Pipe... */
			continue;

		if (!((strchr(val.dptr, '@')) ||
		    (strchr(val.dptr, '!'))))
			continue;			/* Skip local users */

		split_address(val.dptr, val.dsize, user, host);

		if (eflag && check_host(val.dptr, host, dflag, uflag, Eflag)) {
			printf("Invalid host %s in %*.*s:%*.*s\n", host,
			       key.dsize, key.dsize, key.dptr,
			       val.dsize, val.dsize, val.dptr);
			continue;
		}

		if (nflag)
			capitalize(key.dptr, key.dsize);

		if (new_db != NULL) {
			status = ypdb_store(new_db, val, key, YPDB_INSERT);
			if (status != 0) {
				printf("%s: problem storing %*.*s %*.*s\n",
				       getprogname(),
				       val.dsize, val.dsize, val.dptr,
				       key.dsize, key.dsize, key.dptr);
			}
		}

		if (vflag) {
			printf("%*.*s --> %*.*s\n",
			       val.dsize, val.dsize, val.dptr,
			       key.dsize, key.dsize, key.dptr);
		}

	}

	if (new_db != NULL) {
	  	snprintf(datestr, sizeof(datestr), "%010d", (int)time(NULL));
		key.dptr = __UNCONST(YP_LAST_KEY);
		key.dsize = strlen(YP_LAST_KEY);
		val.dptr = datestr;
		val.dsize = strlen(datestr);
		status = ypdb_store(new_db, key, val, YPDB_INSERT);
		if (status != 0)
			warnx("problem storing %*.*s %*.*s",
			       key.dsize, key.dsize, key.dptr,
			       val.dsize, val.dsize, val.dptr);
	}

	if (new_db != NULL) {
	  	localhostname(myname, sizeof(myname) - 1);
		key.dptr = __UNCONST(YP_MASTER_KEY);
		key.dsize = strlen(YP_MASTER_KEY);
		val.dptr = myname;
		val.dsize = strlen(myname);
		status = ypdb_store(new_db, key, val, YPDB_INSERT);
		if (status != 0)
			warnx("problem storing %*.*s %*.*s",
			       key.dsize, key.dsize, key.dptr,
			       val.dsize, val.dsize, val.dptr);
	}

	ypdb_close(db);

	if (new_db != NULL) {
		ypdb_close(new_db);
		if (rename(db_mapname, db_outfile) < 0)
			err(1, "rename `%s' to `%s' failed", db_mapname,
				db_outfile);
	}
	
	exit(0);
}

void
usage(void)
{
	fprintf(stderr,
		"usage: %s [-e|-E [-d] [-u]] [-n] [-v] input [output]\n",
		getprogname());
	exit(1);
}
