/*      $NetBSD: sushi.c,v 1.4 2001/01/09 09:41:41 garbled Exp $       */

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

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <cdk/cdk.h>
#include <form.h>

#include "sushi.h"
#include "menutree.h"
#include "scandir.h"
#include "formtree.h"
#include "handlers.h"
#include "blabel.h"

MTREE_ENTRY *navigate_menu __P((struct cqMenu *, char *, char *));
MTREE_ENTRY *navigate_submenu __P((MTREE_ENTRY *));
MTREE_ENTRY *display_menu __P((struct cqMenu *, char *, char *));
void prepare_searchpath __P((void));

extern char *__progname;

CDKSCREEN	*cdkscreen;
int		scripting;
int		logging;
FILE		*logfile;
FILE		*script;
WINDOW		*funcwin;
struct winsize	ws;
nl_catd		catalog;
char		*lang_id;

char		**searchpaths;

int
main(int argc, char **argv)
{
	char *p;
	int i;
	MTREE_ENTRY *mte = NULL;
	
	catalog = catopen("sushi", 0);
	if (getenv("LANG") == NULL)
		lang_id = NULL;
	else
		lang_id = strdup(getenv("LANG"));

	prepare_searchpath();
	tree_init();
	i = 0;
	for (p = searchpaths[i]; p != NULL; i++) {
		p = searchpaths[i];
		scan_dir(cqMenuHeadp, p);
	}
	if (argc > 1) {
		mte = tree_gettreebyname(cqMenuHeadp, argv[1]);
		if (!mte) {
			printf("QuickName %s not found in any menus.\n",
				argv[1]);
			catclose(catalog);
			exit(EXIT_FAILURE);
		}
	}

	initscr();
	cdkscreen = initCDKScreen(stdscr);

	ioctl(0, TIOCGWINSZ, &ws);

	initCDKColor();
	raw();

	if (mte == NULL)
		navigate_menu(cqMenuHeadp, "sushi_topmenu", 
		    catgets(catalog, 4, 5, "<C></5>Sushi\n\n"));
	else
		navigate_submenu(mte);

	destroyCDKScreen(cdkscreen);
	endCDK();
	endwin();
#ifdef DEBUG
	tree_printtree(cqMenuHeadp);
#endif

	catclose(catalog);
	exit(EXIT_SUCCESS);
}

void
prepare_searchpath(void)
{
	FILE *conf;
	size_t len;
	int i, j;
	char *p;

	conf = fopen("/etc/sushi.conf", "r");
	if (conf == NULL) {
		searchpaths = (char **)malloc(sizeof(char *) * 6);
		if (searchpaths == NULL)
			bailout("malloc: %s", strerror(errno));
		for (i = 0; i < 4; i++) {
			searchpaths[i] = (char *)malloc(sizeof(char) *
			    PATH_MAX);
			if (searchpaths[i] == NULL)
				bailout("malloc: %s", strerror(errno));
		}
		searchpaths[0] = "/usr/share/sushi";
		searchpaths[1] = "/usr/pkg/share/sushi";
		searchpaths[2] = "/usr/X11R6/share/sushi";
		searchpaths[3] = "/etc/sushi";
		i++;
	} else {
		searchpaths = (char **)malloc(sizeof(char *));
		if (searchpaths == NULL)
			bailout("malloc: %s", strerror(errno));
		i = 0;
		while ((p = fgetln(conf, &len))) {
			searchpaths = (char **)realloc(searchpaths,
			    sizeof(char *) * (i + 2));
			if (searchpaths == NULL)
				bailout("malloc: %s", strerror(errno));
			searchpaths[i] = (char *)malloc(sizeof(char)
			    * len + 1);
			if (searchpaths[i] == NULL)
				bailout("malloc: %s", strerror(errno));
			searchpaths[i] = strdup(p);
			for (j = 0; j < len; j++)
				if (searchpaths[i][j] == '\n' ||
				    searchpaths[i][j] == '\r')
					searchpaths[i][j] = '\0';
			i++;
		}
	}
	searchpaths[i] = (char *)malloc(sizeof(char) * PATH_MAX);
	if (searchpaths[i] == NULL)
		bailout("malloc: %s", strerror(errno));
	if (getenv("HOME") != NULL) {
		searchpaths[i] = strcat(getenv("HOME"), "/sushi");
		i++;
	}
	searchpaths[i] = NULL;
}


MTREE_ENTRY *
navigate_menu(struct cqMenu *cqm, char *basedir, char *ititle)
{
	MTREE_ENTRY *mte;
	char title[90];

	mte = display_menu(cqm, ititle, basedir);
	if(mte) {
		if(!TREE_ISEMPTY(&mte->cqSubMenuHead)) {
			snprintf(title, sizeof(title), "<C></5>%s\n\n",
			    mte->itemname);
			navigate_menu(&mte->cqSubMenuHead, mte->path, title);
		} else
			for (;;)
				if (handle_endpoint(mte->path) == 0)
					break;

		navigate_menu(cqm, basedir, ititle);
	}

	return(mte);
}

MTREE_ENTRY *
navigate_submenu(MTREE_ENTRY *mte)
{
	char title[90];
	MTREE_ENTRY *lastmte;

	lastmte = mte;

	if (TREE_ISEMPTY(&mte->cqSubMenuHead))
		for (;;)
			if (handle_endpoint(mte->path) == 0)
				exit(EXIT_SUCCESS);

	snprintf(title, sizeof(title), "<C></5>%s\n\n", mte->itemname);
	mte = display_menu(&mte->cqSubMenuHead, title, mte->path);
	if(mte) {
		if(!TREE_ISEMPTY(&mte->cqSubMenuHead)) {
			snprintf(title, sizeof(title), "<C></5>%s\n\n",
			    mte->itemname);
			navigate_submenu(mte);
		} else
			for (;;)
				if (handle_endpoint(mte->path) == 0)
					break;

		navigate_submenu(lastmte);
	}

	return(mte);
}

MTREE_ENTRY *
display_menu(cqm, title, basedir)
	struct cqMenu *cqm;
	char *title;
	char *basedir;
{
	CDKSCROLL *scroll;
	MTREE_ENTRY *mte;
	char **menu;
	int i, items, selection;

	items = tree_entries(cqm);
	if(!(menu = malloc(sizeof(char *) * (items+1))))
		bailout("malloc: %s", strerror(errno));

	items = 0;
	for (mte = CIRCLEQ_FIRST(cqm); mte != (void *)cqm;
	     mte = CIRCLEQ_NEXT(mte, cqMenuEntries)){
		if(!strcmp(mte->itemname, "BLANK")) {
			if(!(menu[items] = strdup("")))
				bailout("strdup: %s", strerror(errno));
		} else {
			if(!(menu[items] = strdup(mte->itemname)))
				bailout("strdup: %s", strerror(errno));
		}
		++items;
	}

	if (items == 0) {
		destroyCDKScreen(cdkscreen);
		endCDK();
		endwin();
		(void)fprintf(stderr, "%s\n", catgets(catalog, 1, 19,
		    "No menu hierarchy found"));
		catclose(catalog);
		exit(EXIT_FAILURE);
	}

	bottom_help(1);

	scroll = newCDKScroll(cdkscreen, CENTER, TOP, RIGHT,
		ws.ws_row-3, ws.ws_col, title, menu, items, FALSE, A_REVERSE,
		TRUE, FALSE);

	if (scroll == NULL)
		bailout(catgets(catalog, 1, 16,
		    "Cannot allocate scroll widget"));

	bind_menu(scroll, basedir);

	selection = activateCDKScroll(scroll, NULL);

	destroyCDKScroll(scroll);

	for(i=0; i<items; i++)
		free(menu[i]);
	free(menu);

	mte = tree_getentry(cqm, selection);

	return(mte);
}

void
bailout(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	destroyCDKScreen(cdkscreen);
	endCDK();
	fprintf(stderr, "%s: ", __progname);
	if (fmt != NULL)
		vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(EXIT_FAILURE);
}
