/*      $NetBSD: functions.c,v 1.1 2001/01/05 01:28:35 garbled Exp $       */

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Copyright (c) 2000 Tim Rightnour <garbled@netbsd.org>
 * Copyright (c) 2000 Hubert Feyrer <hubertf@netbsd.org>
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

#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "sushi.h"
#include "functions.h"

#ifndef NETBSD_PKG_BASE
#define NETBSD_PKG_BASE	"ftp://ftp.netbsd.org/pub/NetBSD/packages"
#endif

extern int scripting;
extern int logging;
extern FILE *logfile;
extern FILE *script;
extern int errno;
extern nl_catd catalog;

char *ftp_base(int truename);
void cleanup(void);

/* Required by libinstall */
void	cleanup(void){ ; }
int	ftp_cmd(const char *cmd, const char *expectstr);
void	ftp_stop(void);

func_record func_map[] = 
{
	{ "ftp_pkglist", ftp_pkglist },
	{ "script_do", script_do },
	{ "log_do", log_do },
	{(char *)NULL, (char **(*)(char *))NULL},
};

char **
log_do(char *what)
{
	int i;
	time_t tloc;

	i = logging;

	if (strcmp("off", what) == 0)
		logging = 0;
	else if (strcmp("on", what) == 0)
		logging = 1;
	else if (strcmp("OFF", what) == 0)
		logging = 0;
	else if (strcmp("ON", what) == 0)
		logging = 1;
	else
		bailout(catgets(catalog, 1, 1,
		    "log_do arguments must be on or off"));

	if (logging && i == 0) {
		logfile = fopen(LOGFILE_NAME, "w");
		if (logfile == NULL)
			bailout("fopen %s: %s", LOGFILE_NAME,  strerror(errno));
		fprintf(logfile, "%s: %s\n",
			catgets(catalog, 4, 3, "Log started at"),
			asctime(localtime(&tloc)));
		fflush(logfile);
	}

	return(NULL); /* XXX */
}

char **
script_do(char *what)
{
	int i;
	time_t tloc;

	i = scripting;

	if (strcmp("off", what) == 0)
		scripting = 0;
	else if (strcmp("on", what) == 0)
		scripting = 1;
	else if (strcmp("OFF", what) == 0)
		scripting = 0;
	else if (strcmp("ON", what) == 0)
		scripting = 1;
	else
		bailout(catgets(catalog, 1, 2,
		    "script_do arguments must be on or off"));

	if (scripting && i == 0) {
		script = fopen(SCRIPTFILE_NAME, "w");
		if (script == NULL)
			bailout("fopen %s: %s", SCRIPTFILE_NAME,
			    strerror(errno));
		fprintf(script, "#!/bin/sh\n");
		fprintf(script, "# %s: %s\n",
			catgets(catalog, 4, 4, "Script started at"),
			asctime(localtime(&tloc)));
		fflush(script);
	}

	return(NULL); /* XXX */
}

/*
 *   Return list of packages available at the given url
 *   or NULL on error. Returned pointer can be free(3)d
 *   later.
 */
char **
ftp_pkglist(char *subdir)
{
	int rc, tfd;
	char tmpname[FILENAME_MAX];
	char buf[FILENAME_MAX];
	char url[FILENAME_MAX];
	char **list;
	FILE *f;
	int nlines;

	extern int ftp_start(char *url);	/* pkg_install/lib stuff */
	extern int Verbose;			/* pkg_install/lib stuff */
	Verbose=0; /* debugging */

	/* ftp(1) must have a trailing '/' for directories */
	snprintf(url, sizeof(url), "%s/%s/", ftp_base(0), subdir);

	/*
	 * Start FTP coprocess
	 */
	rc = ftp_start(url);
	if (rc == -1)
		bailout(catgets(catalog, 1, 3, "ftp_start failed"));

	/*
	 * Generate tmp file
	 */
	strcpy(tmpname, TMPFILE_NAME);
	tfd=mkstemp(tmpname);
	if (tfd == -1)
		bailout("mkstemp: %s", strerror(errno));

	close(tfd); /* We don't need the file descriptor, but will use 
		       the file in a second */
	/*
	 * Setup & run the command for ftp(1)
	 */
	(void) snprintf(buf, sizeof(buf), "nlist *.tgz %s\n",
	    tmpname);
	rc = ftp_cmd(buf, "\n(550|226).*\n"); /* catch errors */
	if (rc != 226) {
		unlink(tmpname);	/* remove clutter */
		bailout(catgets(catalog, 1, 4, "nlist failed"));
	}

	f = fopen(tmpname, "r");
	if (!f)
		bailout("fopen: %s", strerror(errno));

	/* Read through file once to find out how many lines it has */
	nlines=0;
	while(fgets(buf, sizeof(buf), f))
		nlines++;
	rewind(f);

	list = malloc((nlines + 1) * sizeof(char *));
	if (list == NULL)
		bailout("malloc: %s", strerror(errno));

	/* alloc space for each line now */
	nlines = 0;
	while(fgets(buf, sizeof(buf), f)) {
		list[nlines] = strdup(buf);
		/* XXX 5 to get .tgz */
		list[nlines][strlen(list[nlines])-5] = '\0';
		nlines++;
	}
	list[nlines] = NULL;
	
	fclose(f);
	unlink(tmpname);
	
	/*
	 * Stop FTP coprocess
	 */
	ftp_stop();

	return list;
}

/*
 *	Return patch where binary packages for this OS version/arch
 *	are expected. If mirror is NULL, ftp.netbsd.org is used.
 *	If it's set, it's assumed to be the URL where the the
 *	OS version dirs are, e.g. ftp://ftp.netbsd.org/pub/NetBSD/packages.
 *	If $PKG_PATH is set, is returned unchanged, overriding everything.
 *	In any case, a trailing '/' is *not* passed.
 *	See also Appendix B of /usr/pkgsrc/Packages.txt.
 *
 *	The returned pointer will be overwritten on next call of
 *	this function.
 */
char *
ftp_base(int truename)
{
	char *pkg_path;
	struct utsname un;
	static char buf[256];
	int rc, i, founddot;

	founddot = 0;
	pkg_path = getenv("PKG_PATH");
	if (pkg_path)
		return strdup(pkg_path);

	strncpy(buf, NETBSD_PKG_BASE, sizeof(buf));;

	rc = uname(&un);
	if (rc == -1)
		bailout("uname: %s", strerror(errno));

	strcat(buf, "/");
	if (!truename)
		for (i = 0; i < _SYS_NMLN; i++) {
			if ((un.release[i] == '_') ||
			   (un.release[i] >= 'A' && un.release[i] <= 'Z'))
				un.release[i] = '\0';
			if (un.release[i] == '.') {
				if (founddot)
					un.release[i] = '\0';
				else
					founddot++;
			}
			if (un.release[i] == '\0')
				break;
		}
	strcat(buf, un.release);
	strcat(buf, "/");
	strcat(buf, un.machine);	/* sysctl hw.machine_arch? */

	return buf;
}
