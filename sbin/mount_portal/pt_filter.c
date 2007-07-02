/*	$NetBSD: pt_filter.c,v 1.9 2007/07/02 18:07:45 pooka Exp $	*/

/*
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD
 * Foundation by Brian Grayson, and is dedicated to Rebecca
 * Margaret Pollard-Grayson.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: pt_filter.c,v 1.9 2007/07/02 18:07:45 pooka Exp $");
#endif				/* not lint */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/syslog.h>

#include "portald.h"

/*
 * Key will be <key><path>.  We let the configuration file
 * tell us how to filter the file.
 */

#define FILTER_CMD_SIZE	8192

static void fill_cmd(char **, char *, char *, int);

static void
fill_cmd(char **cmdv, char *path, char *buff, int n)
{
	int     i;
	/* Make tempbuff at least as large as buff. */
	char	*tempbuff = malloc(n);;
	if (tempbuff == NULL)
		err(1, NULL);

	strncpy(tempbuff, cmdv[0], n);
	for (i = 1; cmdv[i]; i++) {
		strncat(tempbuff, " ", n - strlen(tempbuff));
		strncat(tempbuff, cmdv[i], n - strlen(tempbuff));
	}
	strncat(tempbuff, " ", n - strlen(tempbuff));
	/* Now do the snprintf into buff. */
	snprintf(buff, n, tempbuff, path);
	free(tempbuff);
}


/*
 * Strip v[1], replace %s in v[2] v[3] ... with the remainder
 * of the path, and exec v[2] v[3] ... on the remainder.
 */
int
portal_rfilter(struct portal_cred *pcr, char *key, char **v, int *fdp)
{
	char    cmd[FILTER_CMD_SIZE];
	char   *path;
	FILE   *fp;
	int     error = 0;
	char	percent_s[] = "%s";

	error = lose_credentials(pcr);
	if (error != 0)
		return error;

#ifdef DEBUG
	fprintf(stderr, "rfilter:  Got key %s\n", key);
#endif

	if (!v[1] || !v[2]) {
		syslog(LOG_ERR,
		    "rfilter: got strip-key of %s, and command start of %s\n",
		    v[1], v[2]);
		exit(1);
	}
	/*
	 * Format for rfilter in config file:
	 * 
	 * matchkey rfilter stripkey cmd [arg1] [arg2] ...
	 * any of arg1, arg2, etc. can have %s, in which case %s
	 * will be replaced by the full path.  If arg1 is
	 * missing, %s is assumed, i.e.
	 *   bogus1 rfilter bogus1/ cmd1
	 * is equivalent to
	 *   bogus1 rfilter bogus1/ cmd1 %s
	 */
	/*
	 * v[3] could be NULL, or could point to "".
	 */
	if (!v[3] || strlen(v[3]) == 0)
		v[3] = percent_s;	/* Handle above assumption. */
	path = key;
	/* Strip out stripkey if it matches leading part of key. */
	if (!strncmp(v[1], key, strlen(v[1])))
		path += strlen(v[1]);
	/*
	 * v[0] is key match, v[1] says how much to strip, v[2]
	 * is beginning of command proper.  The first %s in v[2]
	 * ... will be replaced with the path.
	 */
	fill_cmd(v + 2, path, cmd, FILTER_CMD_SIZE);
	if (strlen(cmd) >= FILTER_CMD_SIZE) {
		syslog(LOG_WARNING,
		    "Warning:  potential overflow on string!  Length was %lu\n",
		    (unsigned long)strlen(cmd));
		return ENAMETOOLONG;
	}
#ifdef DEBUG
	fprintf(stderr, "rfilter:  Using cmd of %s\n", cmd);
#endif
	fp = popen(cmd, "r");
	if (fp == NULL)
	  	return errno;

	/* Before returning, restore original uid and gid. */
	/* But only do this if we were root to start with. */
	if (getuid() == 0) {
		if ((seteuid((uid_t) 0) < 0) || (setegid((gid_t) 0) < 0)) {
			error = errno;
			syslog(LOG_WARNING, "setcred: %m");
			fclose(fp);
			fp = NULL;
		}
	}
	if (fp)
		fdp[0] = fileno(fp);
	return error;
}

int
portal_wfilter(struct portal_cred *pcr, char *key, char **v, int *fdp)
{
	char    cmd[FILTER_CMD_SIZE];
	char   *path;
	FILE   *fp;
	int     error = 0;
	int     cred_change_err = 0;

	cred_change_err = lose_credentials(pcr);
	if (cred_change_err != 0)
		return cred_change_err;

	path = key + (v[1] ? strlen(v[1]) : 0);
	/*
	 * v[0] is key match, v[1] says how much to strip, v[2]
	 * is beginning of command proper.
	 */
	fill_cmd(v + 2, path, cmd, FILTER_CMD_SIZE);
	if (strlen(cmd) >= FILTER_CMD_SIZE) {
		syslog(LOG_WARNING,
		    "Warning:  potential overflow on string!  Length was %lu\n",
		    (unsigned long)strlen(cmd));
		return ENAMETOOLONG;
	}
	fp = popen(cmd, "w");
	if (fp == NULL) {
	  	return errno;
	}
	/* Before returning, restore original uid and gid. */
	/* But only do this if we were root to start with. */
	if (getuid() == 0) {
		if ((seteuid((uid_t) 0) < 0) || (setegid((gid_t) 0) < 0)) {
			error = errno;
			syslog(LOG_WARNING, "setcred: %m");
			fclose(fp);
			fp = NULL;
		}
	}
	if (fp)
		fdp[0] = fileno(fp);
	return error;
}
