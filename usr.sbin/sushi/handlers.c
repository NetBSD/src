/*      $NetBSD: handlers.c,v 1.1 2001/01/05 01:28:36 garbled Exp $       */

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Copyright (c) 2000 Tim Rightnour <garbled@netbsd.org>
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
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>

#include <cdk/cdk.h>
#include <curses.h>
#include <form.h>

#include "sushi.h"
#include "functions.h"
#include "formtree.h"
#include "run.h"
#include "handlers.h"

extern func_record func_map[];
extern CDKSCREEN *cdkscreen;
extern nl_catd catalog;
extern char *lang_id;
extern struct winsize ws;

int
handle_script(char *path)
{
	char *args[2];

	args[0] = strdup(path);
	args[1] = NULL;
	run_prog(1, args);
	/* don't run this thing 10 billion times if it fails */
	return(0);
}

int
handle_exec(char *path)
{
	char *args[51];
	FILE *f;
	int lcnt, i;
	char *exec, *p;
	size_t len;

	f = fopen(path, "r");
	for (lcnt = 1; (exec = fgetln(f, &len)) != NULL; ++lcnt) {
		if (len == 1) /* skip blank */
			continue;
		if (exec[len - 1] == '#') /* skip comments */
			continue;
		if (exec[len - 1] != '\n') { /* corrupted? */
			warnx("%s: line %d corrupted", path, lcnt);
			continue;
		}
		exec[len - 1] = '\0'; /* NUL terminate */
		for (; *exec != '\0' && isspace((unsigned char)*exec);
			++exec);
		if (*exec == '\0' || *exec == '#')
			continue;
		p = strsep(&exec, " ");
		for (i = 0; p != NULL; p = strsep(&exec, " "), i++)
			args[i] = strdup(p);
	}
	args[i] = NULL;
	fclose(f);

	run_prog(1, args);

	return(0);
}

int
handle_func(char *path)
{
	FILE *f;
	int lcnt, i;
	char *exec, *p, *arg;
	size_t len;

	f = fopen(path, "r");
	for (lcnt = 1; (exec = fgetln(f, &len)) != NULL; ++lcnt) {
		if (len == 1) /* skip blank */
			continue;
		if (exec[len - 1] == '#') /* skip comments */
			continue;
		if (exec[len - 1] != '\n') { /* corrupted? */
			warnx("%s: line %d corrupted", path, lcnt);
			continue;
		}
		exec[len - 1] = '\0'; /* NUL terminate */
		for (; *exec != '\0' && isspace((unsigned char)*exec);
			++exec);
		if (*exec == '\0' || *exec == '#')
			continue;
		p = strdup(strsep(&exec, ","));
		arg = strdup(strsep(&exec, ","));
	}
	fclose(f);

	for (i=0; func_map[i].funcname != NULL; i++)
		if (strcmp(func_map[i].funcname, p) == 0)
			break;

	if (func_map[i].function == NULL)
		bailout("%s: %s",catgets(catalog, 1, 5,
		    "function not found") , p);

	(void) func_map[i].function(arg);
	
	return(0);
}

int
handle_help(char *path)
{
	char **data;
	int lnum;
	char buf[255];
	char *buttons[1];
	FILE *f;
	CDKVIEWER *viewer;

	f = fopen(path, "r");
	if (!f)
		return(0); /* hrmm */
	lnum = 0;
	while (fgets(buf, sizeof(buf), f))
		lnum++;
	rewind(f);

	data = malloc((lnum + 1) * sizeof(char *));
	if (data == NULL)
		bailout("malloc: %s", strerror(errno));

	lnum = 0;
	while(fgets(buf, sizeof(buf), f)) {
		data[lnum] = strdup(buf);
		lnum++;
	}
	fclose(f);
	data[lnum] = NULL;

	buttons[0] = catgets(catalog, 2, 11, "</5><OK><!5>");
	viewer = newCDKViewer(cdkscreen, CENTER, CENTER, ws.ws_row-4, 0,
		buttons, 1, A_NORMAL, TRUE, FALSE);
	setCDKViewer(viewer, catgets(catalog, 2, 12, "Help"), data, lnum+1,
		A_REVERSE, FALSE, TRUE, TRUE);
	activateCDKViewer(viewer, NULL);
	destroyCDKViewer(viewer);

	return(0);
}

int
simple_lang_handler(char *path, char *file, int(* handler)(char *) )
{
	struct stat sb;
	char buf[PATH_MAX+30];

	if (lang_id == NULL)
		sprintf(buf, "%s/%s", path, file);
	else
		sprintf(buf, "%s/%s.%s", path, file, lang_id);
	if (stat(buf, &sb) == 0)
		return(handler(buf));
	else {
		sprintf(buf, "%s/%s", path, file);
		if (stat(buf, &sb) == 0)
			return(handler(buf));
	}
	return(-2); /* special */
}

int
handle_endpoint(char *path)
{
	struct stat sb;
	char buf[PATH_MAX+30];
	char *args[2];
	int rc;

	if (lang_id == NULL)
		sprintf(buf, "%s/%s", path, PREFORMFILE);
	else
		sprintf(buf, "%s/%s.%s", path, PREFORMFILE, lang_id);
	if (stat(buf, &sb) == 0)
		return(handle_preform(path, buf));
	else {
		sprintf(buf, "%s/%s", path, PREFORMFILE);
		if (stat(buf, &sb) == 0)
			return(handle_preform(path, buf));
	}

	args[0] = NULL;
	if (lang_id == NULL)
		sprintf(buf, "%s/%s", path, FORMFILE);
	else
		sprintf(buf, "%s/%s.%s", path, FORMFILE, lang_id);
	if (stat(buf, &sb) == 0)
		return(handle_form(path, buf, args));
	else {
		sprintf(buf, "%s/%s", path, FORMFILE);
		if (stat(buf, &sb) == 0)
			return(handle_form(path, buf, args));
	}

	rc = simple_lang_handler(path, SCRIPTFILE, handle_script);
	if (rc != -2)
		return(rc);

	rc = simple_lang_handler(path, EXECFILE, handle_exec);
	if (rc != -2)
		return(rc);

	rc = simple_lang_handler(path, FUNCFILE, handle_func);
	if (rc != -2)
		return(rc);

	rc = simple_lang_handler(path, HELPFILE, handle_help);
	if (rc != -2)
		return(rc);

	bailout("%s: %s", catgets(catalog, 1, 6, "empty endpoint"), path);
	return(0);
}
