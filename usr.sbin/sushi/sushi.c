/*      $NetBSD: sushi.c,v 1.18 2004/03/24 19:10:58 garbled Exp $       */

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Copyright (c) 2000 Tim Rightnour <garbled@NetBSD.org>
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
void parse_config __P((void));

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
char		*keylabel[10];
chtype		keybinding[10];

int
main(int argc, char **argv)
{
	char *p;
	int i;
	MTREE_ENTRY *mte = NULL;
	
	parse_config();

	catalog = catopen("sushi", 0);
	if (getenv("LANG") == NULL)
		lang_id = NULL;
	else
		lang_id = strdup(getenv("LANG"));

	if (initscr() == NULL)
		errx(1, "%s", "Cannot initialize curses");
		
	cdkscreen = initCDKScreen(stdscr);

	ioctl(0, TIOCGWINSZ, &ws);

	use_default_colors();

	initCDKColor();
	curs_set(0);
	raw();

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

	if (mte == NULL)
		navigate_menu(cqMenuHeadp, "sushi_topmenu", 
		    catgets(catalog, 4, 5, "<C></5>Sushi\n\n"));
	else
		navigate_submenu(mte);

	destroyCDKScreen(cdkscreen);
	endCDK();
	curs_set(1);
	endwin();
#ifdef DEBUG
	tree_printtree(cqMenuHeadp);
#endif

	catclose(catalog);
	return(EXIT_SUCCESS);
}

static char *
next_word(char **line)
{
	char *word, *p;

	p = *line;
	while (*++p && isspace((unsigned char)*p));
	word = p;
	for (; *p != '\0' && !isspace((unsigned char)*p); ++p);
	*p = '\0';
	*line = p;
	return(word);
}


static chtype
parse_keybinding(char *key)
{
	if (tolower(key[0]) == 'f' && isdigit(key[1]))
		/* we have an F key */
		return(KEY_F(atoi(key + 1)));
	else if (key[0] == '^')
		return((chtype)(toupper(key[1]) & ~0x40));
	else if (isalpha(key[0]))
		/* we have an insane user */
		return((chtype)tolower(key[0]));

	bailout("%s: %s", catgets(catalog, 1, 20, "Bad keybinding"), key);
	/* NOTREACHED */
	return(0);
}

void
parse_config(void)
{
	FILE *conf;
	size_t len;
	int i, j;
	char *p, *t, *word;
	char *key, *env;
	char **n;

	if ((conf = fopen("/etc/sushi.conf", "r")) != NULL) {
		i = 0;
		while ((p = fgetln(conf, &len))) {
			if (len == 1 || p[len - 1] == '#' || p[len - 1] != '\n')
				continue;
			p[len - 1] = '\0';
			for (; *p != '\0' && isspace((unsigned char)*p); ++p);
			if (*p == '\0' || *p == '#')
				continue;
			for (t = p; *t && !isspace((unsigned char)*t); ++t);
			if (*t == '\0')
				continue;
			*t = '\0';
			word = p;
			while (*++p && !isspace((unsigned char)*p));
			key = strdup(word);
			if (strcmp(key, "searchpath") == 0) {
				word = next_word(&p);
				n = (char **)realloc(searchpaths,
				    sizeof(char *) * (i + 2));
				if (n == NULL)
					err(1, "realloc");
				searchpaths = n;
				searchpaths[i] = (char *)malloc(sizeof(char)
				    * len + 1);
				if (searchpaths[i] == NULL)
					err(1, "malloc");
				searchpaths[i] = strdup(word);
				for (j = 0; j < len; j++)
					if (searchpaths[i][j] == '\n' ||
					    searchpaths[i][j] == '\r')
						searchpaths[i][j] = '\0';
				i++;
			} else if (strcmp(key, "bind") == 0) {
				word = next_word(&p);
				word++;
				j = atoi(word);
				j--;
				/* now get the character */
				word = next_word(&p);
				keybinding[j] = parse_keybinding(word);
				/* now get the string */
				word = next_word(&p);
				keylabel[j] = strdup(word);
			} else if (strcmp(key, "env") == 0) {
				env = next_word(&p);
				word = next_word(&p);
				if (strcmp(word, "") == 0)
					errx(1, "Invalid env value");
				setenv(env, word, 1);
			} else {
				errx(1, "%s: %s", catgets(catalog, 1, 21,
				    "Bad keyword in config file"), key);
			}
		} /* while */
	}

	if (searchpaths == NULL) {
		searchpaths = (char **)malloc(sizeof(char *) * 6);
		if (searchpaths == NULL)
			err(1, "malloc");
		searchpaths[0] = "/usr/share/sushi";
		searchpaths[1] = "/usr/pkg/share/sushi";
		searchpaths[2] = "/usr/X11R6/share/sushi";
		searchpaths[3] = "/etc/sushi";
		i = 4;
	}

	searchpaths[i] = (char *)malloc(sizeof(char) * PATH_MAX);
	if (searchpaths[i] == NULL)
		err(1, "malloc");
	if (getenv("HOME") != NULL) {
		strlcpy(searchpaths[i], getenv("HOME"), PATH_MAX);
		strlcat(searchpaths[i], "/sushi", PATH_MAX);
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
	CDKSCROLL *scrollp;
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
		curs_set(1);
		endwin();
		(void)fprintf(stderr, "%s\n", catgets(catalog, 1, 19,
		    "No menu hierarchy found"));
		catclose(catalog);
		exit(EXIT_FAILURE);
	}

	bottom_help(1);

	scrollp = newCDKScroll(cdkscreen, CENTER, TOP, RIGHT,
		ws.ws_row-3, ws.ws_col, title, menu, items, FALSE, A_REVERSE,
		TRUE, FALSE);

	if (scrollp == NULL)
		bailout(catgets(catalog, 1, 16,
		    "Cannot allocate scroll widget"));

	bind_menu(scrollp, basedir);

loop:
	selection = activateCDKScroll(scrollp, NULL);

	mte = tree_getentry(cqm, selection);
	if (selection != -1 && strcmp(mte->filename, "BLANK") == 0)
		goto loop;

	destroyCDKScroll(scrollp);

	for(i=0; i<items; i++)
		free(menu[i]);
	free(menu);

	return(mte);
}

void
bailout(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	destroyCDKScreen(cdkscreen);
	endCDK();
	curs_set(1);
	fprintf(stderr, "%s: ", getprogname());
	if (fmt != NULL)
		vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(EXIT_FAILURE);
}
