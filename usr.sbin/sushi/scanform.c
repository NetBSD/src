/*      $NetBSD: scanform.c,v 1.21 2002/04/02 18:59:54 christos Exp $       */

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

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <curses.h>
#include <form.h>
#include <cdk/cdk.h>

#include "sushi.h"
#include "functions.h"
#include "scanform.h"
#include "formtree.h"
#include "handlers.h"
#include "run.h"
#include "blabel.h"

extern func_record func_map[];
extern CDKSCREEN *cdkscreen;
extern struct winsize ws;
extern nl_catd catalog;
extern char *lang_id;
extern char *keylabel[10];
extern chtype keybinding[10];

static void scan_formindex(struct cqForm *cqf, char *row);

static void
form_status(FORM *form)
{
	char buf[20];

	wstandout(stdscr);
	mvwaddstr(stdscr, ws.ws_row-3, 0, catgets(catalog, 4, 9,
	    "PGUP/PGDN to change page, UP/DOWN switch field, ENTER=Do."));
	wstandend(stdscr);
	sprintf(buf, "%s (%d/%d)", catgets(catalog, 4, 8, "Form Page:"),
	    form_page(form)+1, form->max_page); /* XXX */
	mvwaddstr(stdscr, ws.ws_row-3, 60, buf);
	wrefresh(stdscr);
}

static int
scan_form(struct cqForm *cqf, char *path)
{
	FILE *filep;
	int lcnt;
	char *p, *t;
	size_t len;

	if((filep = fopen(path, "r"))) {
		for (lcnt = 1; (p = fgetln(filep, &len)) != NULL; ++lcnt) {
			if (len == 1)		/* Skip empty lines. */
				continue;
			if (p[len - 1] == '#')	/* Skip remarked lines. */
				continue;
			if (p[len - 1] != '\n') {/* Skip corrupted lines. */
				warnx("%s: line %d corrupted", path, lcnt);
				continue;
			}
			p[len - 1] = '\0';	/* Terminate the line. */

						/* Skip leading spaces. */
			for (; *p != '\0' && isspace((unsigned char)*p); ++p);
						/* Skip empty/comment lines. */
			if (*p == '\0' || *p == '#')
				continue;
						/* Find first token. */
			for (t = p; *t && !isspace((unsigned char)*t); ++t);
			if (*t == '\0')		/* Need more than one token.*/
				continue;
			*t = '\0';

			scan_formindex(cqf, p);
		}

		fclose(filep);
	} else
		return(1);
	return(0);
}

static void
form_appenditem(struct cqForm *cqf, char *desc, int type, char *data, int req)
{
        FTREE_ENTRY *fte;

        if ((fte = malloc(sizeof(FTREE_ENTRY))) == NULL ||
                ((fte->desc = strdup(desc)) == NULL) ||
                ((fte->type = type) == NULL) ||
		((fte->data = strdup(data)) == NULL))
                        bailout("malloc: %s", strerror(errno));
	fte->required = req;
	fte->elen = 0;
	fte->origdata = fte->data;

	CIRCLEQ_INIT(&fte->cqSubFormHead);
	CIRCLEQ_INSERT_TAIL(cqf, fte, cqFormEntries);
}

static void
scan_formindex(struct cqForm *cqf, char *row)
{
	char *t = row;
	char *x;
	char desc[80];
	int type;
	char data[80];
	int req = 0;

	while (*++t && !isspace((unsigned char)*t));
	x = (char *)strsep(&row, ":");
	if (strcmp(x, "entry") == 0)
		type = DATAT_ENTRY;
	else if (strcmp(x, "req-entry") == 0) {
		type = DATAT_ENTRY;
		req = 1;
	} else if (strcmp(x, "escript") == 0)
		type = DATAT_ESCRIPT;
	else if (strcmp(x, "req-escript") == 0) {
		type = DATAT_ESCRIPT;
		req = 1;
	} else if (strcmp(x, "nescript") == 0)
		type = DATAT_NESCRIPT;
	else if (strcmp(x, "list") == 0)
		type = DATAT_LIST;
	else if (strcmp(x, "multilist") == 0)
		type = DATAT_MLIST;
	else if (strcmp(x, "req-list") == 0) {
		type = DATAT_LIST;
		req = 1;
	} else if (strcmp(x, "blank") == 0)
		type = DATAT_BLANK;
	else if (strcmp(x, "func") == 0)
		type = DATAT_FUNC;
	else if (strcmp(x, "multifunc") == 0)
		type = DATAT_MFUNC;
	else if (strcmp(x, "req-func") == 0) {
		type = DATAT_FUNC;
		req = 1;
	} else if (strcmp(x, "script") == 0)
		type = DATAT_SCRIPT;
	else if (strcmp(x, "multiscript") == 0)
		type = DATAT_MSCRIPT;
	else if (strcmp(x, "req-script") == 0) {
		type = DATAT_SCRIPT;
		req = 1;
	} else if (strcmp(x, "noedit") == 0)
		type = DATAT_NOEDIT;
	else if (strcmp(x, "invis") == 0)
		type = DATAT_INVIS;
	else if (strcmp(x, "integer") == 0)
		type = DATAT_INTEGER;
	else if (strcmp(x, "req-integer") == 0) {
		type = DATAT_INTEGER;
		req = 1;
	} else if (strcmp(x, "iscript") == 0)
		type = DATAT_ISCRIPT;
	else if (strcmp(x, "req-iscript") == 0) {
		type = DATAT_ISCRIPT;
		req = 1;
	} else if (strcmp(x, "ipv4") == 0)
		type = DATAT_V4;
	else if (strcmp(x, "req-ipv4") == 0) {
		type = DATAT_V4;
		req = 1;
	} else if (strcmp(x, "ipv4script") == 0)
		type = DATAT_V4SCRIPT;
	else if (strcmp(x, "req-ipv4script") == 0) {
		type = DATAT_V4SCRIPT;
		req = 1;
	} else if (strcmp(x, "ipv6") == 0)
		type = DATAT_V6;
	else if (strcmp(x, "req-ipv6") == 0) {
		type = DATAT_V6;
		req = 1;
	} else if (strcmp(x, "ipv6script") == 0)
		type = DATAT_V6SCRIPT;
	else if (strcmp(x, "req-ipv6script") == 0) {
		type = DATAT_V6SCRIPT;
		req = 1;
	} else
		bailout("%s: %s",
		    catgets(catalog, 1, 11, "invalid data type"), x);

	snprintf(data, (size_t)(t-row+1), "%s", row);

	while (*++t && isspace((unsigned char)*t));
	if (strlen(t) > 50)
		bailout(catgets(catalog, 1, 12, "description too long"));

	snprintf(desc, sizeof(desc), "%s", t);
	if (strcmp(desc, "BLANK") == 0)
		sprintf(desc, " ");

	form_appenditem(cqf, desc, type, data, req);
}

int
form_entries(struct cqForm *cqf)
{
	FTREE_ENTRY *fte;
	int entries = 0;

	for (fte = CIRCLEQ_FIRST(cqf); fte != (void *)cqf;
		fte = CIRCLEQ_NEXT(fte, cqFormEntries))
		++entries;

	return(entries);
}

FTREE_ENTRY *
form_getentry(struct cqForm *cqf, int entry)
{
	FTREE_ENTRY *fte;
	int entries = 0;

	for (fte = CIRCLEQ_FIRST(cqf); (fte != (void *)cqf);
	     fte = CIRCLEQ_NEXT(fte, cqFormEntries)) {
		if(entry == entries)
			return(fte);
		++entries;
	}

	return(NULL);
}

void
form_printtree(struct cqForm *cqf)
{
	FTREE_ENTRY *ftp;

	for (ftp = CIRCLEQ_FIRST(cqf); ftp != (void *)cqf;
	     ftp = CIRCLEQ_NEXT(ftp, cqFormEntries))
		printf("%s\t- %d:%s\n", ftp->desc, ftp->type, ftp->data);
}

/* This is the keymap for the forms */

static int 
get_request(WINDOW *w)			/* virtual key mapping */
{
	static int      mode = REQ_INS_MODE;
	int             c = wgetch(w);	/* read a character */
	int 		i;

	/* printf("GOT A THINGIE: 0x%x %d\n", c, c); */

	/* convert to an FKEY for case */
	if (c != 0)
	for (i=0; i < 10; i++)
		if (c == keybinding[i])
			c = KEY_F(i+1);

	switch (c) {
	case KEY_F(1):
		return SHOWHELP;
	case KEY_F(2):
		return REFRESH;
	case KEY_F(3):
		return BAIL;
	case KEY_F(4):
		return GENLIST;
	case KEY_F(5):
		return RESET;
	case KEY_F(6):
		return COMMAND;
	case KEY_F(7):
		return EDIT;
	case KEY_F(8):
		return DUMPSCREEN;
	case KEY_F(9):
		return SHELL;
	case KEY_F(10):
		return FASTBAIL;
	case 0x1b:		/* ESC */
		return BAIL;
	case 0x0d:
		return QUIT;
	case KEY_ENTER:
		return QUIT;
	case 0xa:		/* LF */
		return QUIT;
	case KEY_NPAGE:		/* ^F */
		return REQ_NEXT_PAGE;
	case KEY_PPAGE:		/* ^B */
		return REQ_PREV_PAGE;
	case KEY_DOWN:
		return REQ_NEXT_FIELD;
	case KEY_UP:
		return REQ_PREV_FIELD;
	case 0x06:		/* ^F */
		return REQ_NEXT_PAGE;
	case 0x02:		/* ^B */
		return REQ_PREV_PAGE;
	case 0x0e:		/* ^N */
		return REQ_NEXT_FIELD;
	case 0x10:		/* ^P */
		return REQ_PREV_FIELD;
	case KEY_HOME:
		return REQ_FIRST_FIELD;
	case KEY_LL:
		return REQ_LAST_FIELD;
	case 0x0c:		/* ^L */
		return REQ_LEFT_FIELD;
	case 0x12:		/* ^R */
		return REQ_RIGHT_FIELD;
	case 0x15:		/* ^U */
		return REQ_UP_FIELD;
	case 0x04:		/* ^D */
		return REQ_DOWN_FIELD;
	case 0x17:		/* ^W */
		return REQ_NEXT_WORD;
	case 0x14:		/* ^T */
		return REQ_PREV_WORD;
	case 0x01:		/* ^A */
		return REQ_BEG_FIELD;
	case 0x05:		/* ^E */
		return REQ_END_FIELD;
	case KEY_LEFT:
		return REQ_LEFT_CHAR;
	case KEY_RIGHT:
		return REQ_RIGHT_CHAR;
	case 0x0f:		/* ^O */
		return REQ_INS_LINE;
	case KEY_DC:
	case 0x16:		/* ^V */
		return REQ_DEL_CHAR;
	case KEY_BACKSPACE:
	case 0x08:		/* ^H */
		return REQ_DEL_PREV;
	case 0x19:		/* ^Y */
		return REQ_DEL_LINE;
	case 0x07:		/* ^G */
		return REQ_DEL_WORD;
	case 0x03:		/* ^C */
		return REQ_CLR_EOL;
	case 0x0b:		/* ^K */
		return REQ_CLR_EOF;
	case 0x18:		/* ^X */
		return REQ_CLR_FIELD;
	case 0x9:		/* tab*/
		return REQ_NEXT_CHOICE;
	case 0x1a:		/* ^Z 0x1a*/
		return REQ_PREV_CHOICE;
	case KEY_IC:		/* INS */

		if (mode == REQ_INS_MODE)
			return mode =
				REQ_OVL_MODE;
		else
			return mode =
				REQ_INS_MODE;

	}
	return c;
}

static int 
my_driver(FORM * form, int c, char *path)
{
	WINDOW *subwindow;
	CDKSCROLL *plist;
	CDKSELECTION *slist;
	CDKENTRY *entry;
	FIELD *curfield;
	char **list;
	int i, j, y, n, dcols, drows, dmax;
	char *tmp, *otmp, *p;
	char *choices[] = {" ", "+"};
	char buf[1024];

	switch (c) {
	case EDIT:
		curfield = current_field(form);
		if (field_opts(curfield) & O_STATIC) {
			field_info(curfield, &drows, &dcols, &j, &j, &j, &j);
			dmax = 0;
		} else
			dynamic_field_info(curfield, &drows, &dcols, &dmax);
		if (dmax == 0)
			dmax = dcols;
		entry = newCDKEntry(cdkscreen, BOTTOM, CENTER,
		    catgets(catalog, 4, 6, "Enter the field data "
		    "below, and hit enter to return to the form."),
		    catgets(catalog, 4, 7, "Data Entry: "),
		    A_REVERSE, field_pad(curfield), vMIXED, ws.ws_col-20,
		    0, dmax, TRUE, FALSE);
		if (field_buffer(curfield, 0) == NULL)
			setCDKEntry(entry, "", 0, dmax, TRUE);
		else
			setCDKEntry(entry, field_buffer(curfield, 0), 0,
			    dmax, TRUE);
		injectCDKEntry(entry, CDK_BEGOFLINE);
		tmp = activateCDKEntry(entry, NULL);
		if (entry->exitType == vESCAPE_HIT) {
			destroyCDKEntry(entry);
			return(FALSE);
		}
		set_field_buffer(curfield, 0, tmp);
		destroyCDKEntry(entry);
		touchwin(stdscr);
		wrefresh(stdscr);
		return(FALSE);
		/* NOTREACHED */
		break;
	case COMMAND:
		/* for now, ignore this.. it's messy */
		return(FALSE);
		/* NOTREACHED */
		break;
	case RESET:
		return(2); /* hrmmm... */
		/* NOTREACHED */
		break;
	case REFRESH:
		touchwin(stdscr);
		wrefresh(stdscr);
		return(FALSE);
		/* NOTREACHED */
		break;
	case SHELL:
		endwin();
		system("/bin/sh");
		wrefresh(stdscr);
		return(FALSE);
		/* NOTREACHED */
		break;
	case DUMPSCREEN:
		do_scrdump(NULL, NULL, NULL, NULL);
		return(FALSE);
		/* NOTREACHED */
		break;
	case SHOWHELP:
	        if (simple_lang_handler(path, HELPFILE, handle_help) == -2)
			nohelp();
		subwindow = form_win(form);
		touchwin(subwindow);
		wrefresh(subwindow);
		return(FALSE);
		/* NOTREACHED */
		break;
	case GENLIST:
		/* pull the grand popup list */
		curfield = current_field(form);
		list = (char **) field_userptr(curfield);
		if (list == NULL)
			break;
		for (i=0, y=0; list[i] != NULL; i++)
			if ((strlen(list[i]) + 1) > y)
				y = strlen(list[i]) + 1;
		if (field_buffer(curfield, 1) == NULL || 
		    field_buffer(curfield, 0) == NULL)
			return(FALSE);
		otmp = tmp = strdup(field_buffer(curfield, 1));
		stripWhiteSpace(vBOTH, tmp);
		if (*tmp == 'm') {
			slist = newCDKSelection(cdkscreen, RIGHT, CENTER,
			    RIGHT, 10, y+2,
			    catgets(catalog, 4, 1, "Select choices"),
			    list, i, choices, 2, A_REVERSE ,TRUE, FALSE);
			free(otmp);
			otmp = tmp = strdup(field_buffer(curfield, 0));
			stripWhiteSpace(vBOTH, tmp);
			if (*tmp != '\0')
				for (p = tmp; p != NULL; p = strsep(&tmp, ","))
					for (y = 0; y < i; y++)
						if (strcmp(p, list[y]) == 0)
							slist->selections[y] = 1;
			activateCDKSelection(slist, NULL);
			for (y = 0; y < i; y++)
				if (slist->selections[y] == 1) {
					memset(&buf, '\0', sizeof(buf));
					break;
				}
			for (y = 0, n = 0; y < i; y++)
				if (slist->selections[y] == 1) {
					n++;
					if (n != 1)
						sprintf(buf, "%s,", buf);
					sprintf(buf, "%s%s", buf, list[y]);
				}
			free(otmp);
			tmp = buf;
			if (n)
				set_field_buffer(curfield, 0, tmp);
			destroyCDKSelection(slist);
		} else if (*tmp ==  's') {
			plist = newCDKScroll(cdkscreen, RIGHT, CENTER, RIGHT,
			    10, y+2, catgets(catalog, 4, 1, "Select choice"),
			    list, i, FALSE, A_REVERSE ,TRUE, FALSE);
			i = activateCDKScroll(plist, NULL);
			if (i != -1)
				set_field_buffer(curfield, 0, list[i]);
			destroyCDKScroll(plist);
			free(otmp);
		}
		return(FALSE);
		/* NOTREACHED */
		break;
	case QUIT:
		/* do something useful */
		if (form_driver(form, REQ_VALIDATION) == E_OK) {
			if (process_form(form, path) == 0)
				return TRUE;
			else
				return(2); /* special meaning */
		}
		/* NOTREACHED */
		break;
	case BAIL:
		return(3); /* TRUE, so handle_preform can free F */
		/* NOTREACHED */
		break;
	case FASTBAIL:
		endwin();
		exit(EXIT_SUCCESS);
		break;
	}
	beep();
	return FALSE;
}

static FIELD *
LABEL(FIELD_RECORD *x)
{
	char *tmp;
	FIELD *f = new_field(1, (int)(strlen(x->v)+2), x->frow, x->fcol, 0, 0);

	if (f) {
		x->v = realloc(x->v, sizeof(char *) * (strlen(x->v)+2));
		tmp = malloc(sizeof(char *) * strlen(x->v));
		if (x->v == NULL || tmp == NULL)
			bailout("malloc: %s", strerror(errno));

		if (x->required == 1)
			(void)strcpy(tmp, "* ");
		else
			(void)strcpy(tmp, "  ");
		(void)strcat(tmp, x->v);
		(void)strcpy(x->v, tmp);
		set_field_buffer(f, 0, x->v);
		free(tmp);
		field_opts_off(f, O_ACTIVE);
		if (x->newpage == 1)
			set_new_page(f, TRUE);
	}
	return f;
}

static FIELD *
STRING(FIELD_RECORD *x)
{				/* create a STRING field */
	FIELD *f = new_field(x->rows, x->cols, x->frow, x->fcol, 0, 0);

	if (f) {
		set_field_back(f, A_UNDERLINE);
		if (x->newpage == 1)
			set_new_page(f, TRUE);
		if (x->required == 1)
			field_opts_off(f, O_NULLOK);
		if (x->bigfield) {
			field_opts_off(f, O_STATIC);
			set_max_field(f, x->rcols);
		}
		set_field_buffer(f, 0, x->v);
	}
	return f;
}

static FIELD *
IPV4(FIELD_RECORD *x)
{				/* create an IPV4 field */
	FIELD *f = new_field(x->rows, x->cols, x->frow, x->fcol, 0, 0);

	if (f) {
		set_field_type(f, TYPE_IPV4);
		set_field_back(f, A_UNDERLINE);
		if (x->newpage == 1)
			set_new_page(f, TRUE);
		if (x->required == 1)
			field_opts_off(f, O_NULLOK);
		if (x->bigfield) {
			field_opts_off(f, O_STATIC);
			set_max_field(f, x->rcols);
		}
		set_field_buffer(f, 0, x->v);
	}
	return f;
}

static FIELD *
IPV6(FIELD_RECORD *x)
{				/* create an IPV6 field */
	FIELD *f = new_field(x->rows, x->cols, x->frow, x->fcol, 0, 0);

	if (f) {
		set_field_type(f, TYPE_IPV6);
		set_field_back(f, A_UNDERLINE);
		if (x->newpage == 1)
			set_new_page(f, TRUE);
		if (x->required == 1)
			field_opts_off(f, O_NULLOK);
		if (x->bigfield) {
			field_opts_off(f, O_STATIC);
			set_max_field(f, x->rcols);
		}
		set_field_buffer(f, 0, x->v);
	}
	return f;
}

static FIELD *
MULTI(FIELD_RECORD *x)
{				/* create a MULTI field */
	FIELD *f = new_field(x->rows, x->cols, x->frow, x->fcol, 0, 1);

	if (f) {
		set_field_back(f, A_REVERSE);
		if (x->newpage == 1)
			set_new_page(f, TRUE);
		field_opts_off(f, O_STATIC);
		field_opts_off(f, O_EDIT);
		set_field_userptr(f, x->list);
		set_field_buffer(f, 1, "multi");
	}
	return f;
}

static FIELD *
INTEGER(FIELD_RECORD *x)
{				/* create an INTEGER field */
	FIELD *f = new_field(x->rows, x->cols, x->frow, x->fcol, 0, 0);
	int pre, min, max;
	char *p, *q, *n;

	p = x->v;
	q = strsep(&p, ",");
	pre = atoi(q);
	q = strsep(&p, ",");
	min = atoi(q);
	n = strdup(p);
	q = strsep(&p, ",");
	if (p == NULL)
		max = atoi(n);
	else
		max = atoi(q);

	if (f) {
		set_field_back(f, A_UNDERLINE);
		set_field_type(f, TYPE_INTEGER, pre, min, max);
		if (x->newpage == 1)
			set_new_page(f, TRUE);
		if (x->required == 1)
			field_opts_off(f, O_NULLOK);
		if (x->bigfield)
			field_opts_off(f, O_STATIC);
		if (p != NULL)
			set_field_buffer(f, 0, p);
	}
	return f;
}

static FIELD *
INVIS(FIELD_RECORD *x)
{				/* create an INVIS field */
	FIELD *f = new_field(x->rows, x->cols, x->frow, x->fcol, 0, 0);

	if (f) {
		set_field_opts(f, field_opts(f) & ~(O_ACTIVE|O_VISIBLE));
		if (x->bigfield) {
			field_opts_off(f, O_STATIC);
			set_max_field(f, x->rcols);
		}
		if (x->newpage == 1)
			set_new_page(f, TRUE);
		set_field_buffer(f, 0, x->v);
	}
	return f;
}

static FIELD *
NOEDIT(FIELD_RECORD *x)
{				/* create a NOEDIT field */
	FIELD *f = new_field(x->rows, x->cols, x->frow, x->fcol, 0, 0);

	if (f) {
		set_field_opts(f, field_opts(f) & ~(O_EDIT|O_ACTIVE));
		if (x->newpage == 1)
			set_new_page(f, TRUE);
		if (x->bigfield) {
			field_opts_off(f, O_STATIC);
			set_max_field(f, x->rcols);
		}
		set_field_buffer(f, 0, x->v);
	}
	return f;
}

static FIELD *
ENUM(FIELD_RECORD *x)
{				/* create a ENUM field */
	FIELD *f = new_field(x->rows, x->cols, x->frow, x->fcol, 0, 1);

	if (f) {
		set_field_back(f, A_REVERSE);
		set_field_type(f, TYPE_ENUM, x->list, FALSE, FALSE);
		set_field_userptr(f, x->list);
		if (x->newpage == 1)
			set_new_page(f, TRUE);
		if (x->required == 1)
			field_opts_off(f, O_NULLOK);
		field_opts_off(f, O_EDIT);
		set_field_buffer(f, 1, "single");
		if (x->bigfield)
			field_opts_off(f, O_STATIC);
		set_field_buffer(f, 0, x->list[0]);
	}
	return f;
}

FIELD_RECORD *F;

static int
process_preform(FORM *form, char *path)
{
	char file[PATH_MAX];
	struct stat sb;
	char *p;
	int lcnt, i, j;
	FIELD **f;
	char **args;

	if (lang_id == NULL) {
		sprintf(file, "%s/%s", path, FORMFILE);
	} else {
		sprintf(file, "%s/%s.%s", path, FORMFILE, lang_id);
		if (stat(file, &sb) != 0)
			sprintf(file, "%s/%s", path, FORMFILE);
	}

	args = malloc(sizeof(char *) * 2);
	if (args == NULL)
		bailout("malloc: %s", strerror(errno));
	lcnt = field_count(form);
	args = realloc(args, sizeof(char *) * (lcnt+1));
	f = malloc(sizeof(FIELD *) * lcnt);
	if (f == NULL || args == NULL)
		bailout("malloc: %s", strerror(errno));

	f = form_fields(form);
	for (lcnt=0, i=0; f[lcnt] != NULL; lcnt++)
		if (F[lcnt].type != (PF_field)LABEL) {
			if (field_buffer(f[lcnt], 0) == NULL)
				args[i] = "";
			else
				args[i] = strdup(field_buffer(f[lcnt], 0));
			if (args[i] != NULL) {
				p = &args[i][strlen(args[i]) - 1];
					p--;
				while(isspace(*p))
					*p-- = '\0';
			}
			i++;
		}
	args[i] = NULL;

	for (i=0; F[i].type != NULL; i++) {
		free(F[i].v);
		if (F[i].list != NULL) {
			for (j=0; F[i].list[j] != NULL; j++)
				free(F[i].list[j]);
			free(F[i].list);
		}
	}
	free(F);

	i = handle_form(path, file, args);

	for (j=0; args[j] != NULL; j++)
		free(args[j]);
	free(args);
	return(i);
}

int
process_form(FORM *form, char *path)
{
	FILE *fp;
	char file[PATH_MAX], file2[PATH_MAX];
	struct stat sb;
	char *exec, *t, *p;
	size_t len;
	int lcnt, i, j;
	FIELD **f;
	char **args;

	/* handle the preform somewhere else */
	if (strcmp("pre", form_userptr(form)) == 0)
		return(process_preform(form, path));

	if (lang_id == NULL) {
		sprintf(file, "%s/%s", path, EXECFILE);
		sprintf(file2, "%s/%s", path, SCRIPTFILE);
	} else {
		sprintf(file, "%s/%s.%s", path, EXECFILE, lang_id);
		sprintf(file2, "%s/%s.%s", path, SCRIPTFILE, lang_id);
		if (stat(file, &sb) != 0)
			sprintf(file, "%s/%s", path, EXECFILE);
		if (stat(file2, &sb) != 0)
			sprintf(file2, "%s/%s", path, SCRIPTFILE);
	}

	args = malloc(sizeof(char *) * 2);
	if (args == NULL)
		bailout("malloc: %s", strerror(errno));

	if ((fp = fopen(file, "r"))) {
		for (lcnt = 1; (exec = fgetln(fp, &len)) != NULL; ++lcnt) {
			if (len == 1)	/* skip blank */
				continue;
			if (exec[len - 1] == '#')	/* Skip comments */
				continue;
			if (exec[len - 1] != '\n') { /* corruped? */
				warnx("%s: line %d corrupted", path, lcnt);
				continue;
			}
			exec[len - 1] = '\0';	/* NUL terminate */
			for (; *exec != '\0' && isspace((unsigned char)*exec);
			     ++exec);
			if (*exec == '\0' || *exec == '#')
				continue;
			p = strsep(&exec, " ");
			for (i = 0; p != NULL; p = strsep(&exec, " "), i++) {
				args = realloc(args, sizeof(char *) * (i+2));
				if (args == NULL)
					bailout("realloc: %s", strerror(errno));
				args[i] = strdup(p);
			}
			t = NULL;
		}
		fclose(fp);
	} else if (stat(file2, &sb) == 0) {
		t = strdup(file2);
		i = 1;
	} else
		bailout(catgets(catalog, 1, 13, "no files"));

	lcnt = field_count(form);
	args = realloc(args, sizeof(char *) * (lcnt+1+i));
	f = malloc(sizeof(FIELD *) * lcnt);
	if (f == NULL || args == NULL)
		bailout("malloc: %s", strerror(errno));

	f = form_fields(form);
	for (lcnt=0; f[lcnt] != NULL; lcnt++)
		if (F[lcnt].type != (PF_field)LABEL) {
			if (field_buffer(f[lcnt], 0) == NULL)
				args[i] = "";
			else
				args[i] = strdup(field_buffer(f[lcnt], 0));
			if (args[i] != NULL) {
				p = &args[i][strlen(args[i]) - 1];
					p--;
				while(isspace(*p))
					*p-- = '\0';
			}
			i++;
		}
	if (t != NULL)
		args[0] = t;
	args[i] = NULL;

	i = run_prog(1, args);

	for (j=0; args[j] != NULL; j++)
		free(args[j]);
	free(args);

	return(i);
}

static FIELD  **
make_fields(void)
{				/* create the fields */
	FIELD **f, **fields;
	int i;

	f = malloc(sizeof(FIELD *) * (MAX_FIELD+1));
	if (f == NULL)
		bailout("malloc: %s", strerror(errno));
	fields = f;

	for (i = 0; i < MAX_FIELD && F[i].type; ++i, ++f)
		*f = (F[i].type)(&F[i]);

	*f = (FIELD *) 0;

	return fields;
}

struct cqForm cqFormHead, *cqFormHeadp;

static int
tstring(int max, char *string)
{
	char hold[10];
	int cur;

	if (max == 0)
		return(0);
	for (cur=0; cur <= max; cur++) {
		sprintf(hold, "@@@%d@@@", cur);
		if (strcmp(hold, string) == 0)
			return(cur);
	}
	return(0);
}

static int
strlen_data(FTREE_ENTRY *ftp)
{
	int i, j;
	char *p, *q, *o;

	i = 0;
	switch(ftp->type) {
	case DATAT_BLANK:
		return(1);
		/* NOTREACHED */
		break;
	case DATAT_ENTRY:
	case DATAT_ESCRIPT:
	case DATAT_NESCRIPT:
	case DATAT_V4:
	case DATAT_V4SCRIPT:
	case DATAT_V6:
	case DATAT_V6SCRIPT:
	case DATAT_INVIS:
	case DATAT_NOEDIT:
		return(ftp->elen);
		/* NOTREACHED */
		break;
	case DATAT_LIST:
	case DATAT_FUNC:
	case DATAT_SCRIPT:
	case DATAT_MLIST:
	case DATAT_MFUNC:
	case DATAT_MSCRIPT:
		if (ftp->list == NULL)
			return 0;
		for (i = 0, j = 0; ftp->list[i] != NULL; i++) {
			size_t k;
			if ((k = strlen(ftp->list[i])) > j)
				j = k;
		}
		return(j);
		/* NOTREACHED */
		break;
	case DATAT_INTEGER:
	case DATAT_ISCRIPT:
		o = p = strdup(ftp->data);
		q = strsep(&p, ",");
		i = atoi(q);
		free(o);
		return(i);
		/* NOTREACHED */
		break;
	default:
		bailout(catgets(catalog, 1, 14, "invalid field type"));
		break;
	}
	return(1); /* eep! */
}

static void
gen_list(FTREE_ENTRY *ftp, int max, char **args)
{
	int i=0;
	int lmax = 10;
	int cur;
	char *p, *q;

	ftp->list = malloc(sizeof(char*) * lmax);
	if (ftp->list == NULL)
		bailout("malloc: %s", strerror(errno));
	for (p = ftp->data; p != NULL;) {
		q = (char *)strsep(&p, ",");
		if (q != NULL) {
			cur = tstring(max, q);
			if (cur)
				ftp->list[i++] = strdup(args[cur-1]);
			else
				ftp->list[i++] = strdup(q);
		}
		if (i == lmax - 2) {
			lmax += 10;
			ftp->list = realloc(ftp->list, sizeof(char*) * lmax);
		}
		if (ftp->list == NULL)
			bailout("realloc: %s", strerror(errno));
	}
	ftp->list[i] = NULL;
}

static void
gen_func(FTREE_ENTRY *ftp, int max, char **args)
{
	int i, cur;
	char *p, *q;

	p = strsep(&ftp->data, ",");
	q = strsep(&ftp->data, ",");
	for (i=0; func_map[i].funcname != NULL; i++)
		if (strcmp(func_map[i].funcname, p) == 0)
			break;

	if (func_map[i].function == NULL)
		bailout("%s: %s",
		    catgets(catalog, 1, 5, "function not found"), p);

	cur = tstring(max, q);
	if (cur)
		ftp->list = func_map[i].function(args[cur-1]);
	else
		ftp->list = func_map[i].function(q);
}

static void
gen_script(FTREE_ENTRY *ftp, char *dir, int max, char **args)
{
	char *p, *q, *qo, *po, *comm, *test;
	FILE *file;
	char buf[PATH_MAX+30];
#if 0
	struct stat sb;
#endif
	size_t len;
	int i, cur;
	int lmax = 10;

	qo = q = strdup(ftp->data);
	comm = malloc(sizeof(char) * strlen(q) + 2);
	if (comm == NULL)
		bailout("malloc: %s", strerror(errno));
	p = strsep(&q, ",");
	(void)strcpy(comm, p);
	po = NULL;
	if (q != NULL)
		for (po=p=strdup(q); p != NULL; p = strsep(&q, ",")) {
			(void)strcat(comm, " ");
			for (test=p; *test != '\0'; test++)
				if (*test == ',') {
					*test = '\0';
					test++;
					q = test;
					break;
				}
			cur = tstring(max, p);
			if (cur) {
				comm = realloc(comm, sizeof(char) *
				    (strlen(comm) + strlen(args[cur-1]) + 2));
				if (comm == NULL)
					bailout("malloc: %s", strerror(errno));
				(void)strcat(comm, args[cur-1]);
			} else {
				comm = realloc(comm, sizeof(char) *
				    (strlen(comm) + strlen(p) + 2));
				if (comm == NULL)
					bailout("malloc: %s", strerror(errno));
				(void)strcat(comm, p);
			}
		}
	free(qo);
	if (po)
		free(po);

	sprintf(buf, "%s/%s", dir, comm);

#if 0
	if (stat(buf, &sb) != 0)
		bailout("%s: %s", buf, strerror(errno));
#endif

	file = popen(buf, "r");
	if (file == NULL)
		bailout("popen: %s", strerror(errno));

	ftp->list = malloc(sizeof(char *) * lmax);
	if (ftp->list == NULL)
		bailout("malloc: %s", strerror(errno));

	for (i = 0; (p = fgetln(file, &len)) != NULL;) {
		if (len <= 1)
			continue;
		ftp->list[i] = malloc(len);
		if (ftp->list[i] == NULL)
			bailout("malloc: %s", strerror(errno));
		memcpy(ftp->list[i], p, len);
		ftp->list[i][len - 1] = '\0';
		if (++i == lmax - 2) {
			lmax += 10; 
			ftp->list = realloc(ftp->list, sizeof(char *) * lmax);
			if (ftp->list == NULL)
				bailout("realloc: %s", strerror(errno));
		}
	}
	pclose(file);
	if (i == 0) {
		ftp->list[0] = "";
		ftp->list[1] = NULL;
	} else {
		ftp->list[i] = NULL;
	}
	free(comm);
}

static char *
gen_escript(FTREE_ENTRY *ftp, char *dir, int max, char **args)
{
	char *p, *q, *qo, *po, *test, *comm;
	FILE *file;
	char buf[PATH_MAX+30];
	size_t len;
	int cur;
/*	struct stat sb; */

	if (ftp->data == NULL)
		bailout(catgets(catalog, 1, 22,
		    "Null filename in escript argument"));
	else
		qo = q = strdup(ftp->data);

	comm = malloc(sizeof(char) * strlen(q) + 2);
	if (comm == NULL)
		bailout("malloc: %s", strerror(errno));
	p = strsep(&q, ",");
	(void)strcpy(comm, p);
	po = NULL;
	if (q != NULL)
		for (po=p=strdup(q); p != NULL; p = strsep(&q, ",")) {
			(void)strcat(comm, " ");
			for (test=p; *test != '\0'; test++)
				if (*test == ',') {
					*test = '\0';
					test++;
					q = test;
					break;
				}
			cur = tstring(max, p);
			if (cur) {
				comm = realloc(comm, sizeof(char) *
				    (strlen(comm) + strlen(args[cur-1]) + 2));
				if (comm == NULL)
					bailout("malloc: %s", strerror(errno));
				(void)strcat(comm, args[cur-1]);
			} else {
				comm = realloc(comm, sizeof(char) *
				    (strlen(comm) + strlen(p) + 2));
				if (comm == NULL)
					bailout("malloc: %s", strerror(errno));
				(void)strcat(comm, p);
			}
		}

	free(qo);
	if (po)
		free(po);

	sprintf(buf, "%s/%s", dir, comm);

#if 0
	if (stat(buf, &sb) != 0)
		bailout("%s: %s", buf, strerror(errno));
#endif
	file = popen(buf, "r");
	if (file == NULL)
		bailout("popen: %s", strerror(errno));

	p = fgetln(file, &len);
	if (p != NULL) {
		q = strdup(p);
		q[len -1] = '\0';
	} else
		bailout("fgetln: %s", strerror(errno));

	pclose(file);
	free(comm);
	return(q);
}

static char *
gen_integer(FTREE_ENTRY *ftp, int max, char **args)
{
	char *q, *qo, *tmp;
	char buf[PATH_MAX+30];	
	int pre, min, maxi, cur;

	qo = q = strdup(ftp->data);
	tmp = strsep(&q, ",");
	cur = tstring(max, tmp);
	if (cur)
		pre = atoi(args[cur-1]);
	else
		pre = atoi(tmp);
	tmp = strsep(&q, ",");
	cur = tstring(max, tmp);
	if (cur)
		min = atoi(args[cur-1]);
	else
		min = atoi(tmp);
	tmp = strsep(&q, ",");
	cur = tstring(max, tmp);
	if (cur)
		maxi = atoi(args[cur-1]);
	else
		maxi = atoi(tmp);
	if (q == NULL)
		snprintf(buf, sizeof(buf), "%d,%d,%d", pre, min, maxi);
	else
		snprintf(buf, sizeof(buf), "%d,%d,%d,%s", pre, min, maxi, q);
	q = strdup(buf);
	free(qo);

	return(q);
}

static char *
gen_iscript(FTREE_ENTRY *ftp, char *dir, int max, char **args)
{
	char *p, *q, *qo, *po, *test, *comm, *tmp;
	FILE *file;
	char buf[PATH_MAX+30];
/*	struct stat sb; */
	size_t len;
	int cur, min, maxi, pre;

	qo = q = strdup(ftp->data);
	tmp = strsep(&q, ",");
	pre = atoi(tmp);
	tmp = strsep(&q, ",");
	min = atoi(tmp);
	tmp = strsep(&q, ",");
	maxi = atoi(tmp);

	comm = malloc(sizeof(char) * strlen(q) + 2);
	if (comm == NULL)
		bailout("malloc: %s", strerror(errno));
	tmp = strsep(&q, ",");
	(void)strcpy(comm, tmp);
	po = NULL;
	if (q != NULL)
		for (po=p=strdup(q); p != NULL; p = strsep(&q, ",")) {
			(void)strcat(comm, " ");
			for (test=p; *test != '\0'; test++)
				if (*test == ',') {
					*test = '\0';
					test++;
					q = test;
					break;
				}
			cur = tstring(max, p);
			if (cur) {
				comm = realloc(comm, sizeof(char) *
				    (strlen(comm) + strlen(args[cur-1]) + 2));
				if (comm == NULL)
					bailout("malloc: %s", strerror(errno));
				(void)strcat(comm, args[cur-1]);
			} else {
				comm = realloc(comm, sizeof(char) *
				    (strlen(comm) + strlen(p) + 2));
				if (comm == NULL)
					bailout("malloc: %s", strerror(errno));
				(void)strcat(comm, p);
			}
		}
	free(qo);
	if (po)
		free(po);

	sprintf(buf, "%s/%s", dir, comm);

#if 0
	if (stat(buf, &sb) != 0)
		bailout("%s: %s", buf, strerror(errno));
#endif
	file = popen(buf, "r");
	if (file == NULL)
		bailout("popen: %s", strerror(errno));

	p = fgetln(file, &len);
	if (p != NULL) {
		q = strdup(p);
		q[len -1] = '\0';
	} else
		bailout("fgetln: %s", strerror(errno));

	pclose(file);
	snprintf(buf, sizeof(buf), "%d,%d,%d,%s", pre, min, maxi, q);
	free(q);
	q = strdup(buf);

	free(comm);

	return(q);
}

static void
form_generate(struct cqForm *cqf, char *basedir, char **args)
{
	int i=0;
	int lrow=0;
	int max, cur;
	char *p;
	FTREE_ENTRY *ftp;

	for (max=0, cur=0; args[max] != NULL; max++);

	for (ftp = CIRCLEQ_FIRST(cqf); ftp != (void *)cqf;
	     ftp = CIRCLEQ_NEXT(ftp, cqFormEntries)) {

		F[i].type = LABEL;
		F[i].rows = 0;
		F[i].cols = 0;
		if (lrow >= ws.ws_row-6) {
			F[i].newpage = 1;
			lrow = 0;
		} else
			F[i].newpage = 0;
		F[i].frow = lrow;
		F[i].fcol = 0;
		F[i].v = strdup(ftp->desc);
		F[i].required = ftp->required;
		F[i].list = (char **)NULL;
		i++;

		if (ftp->type != DATAT_BLANK) {
			F[i].required = ftp->required;
			F[i].list = (char **)NULL;
			switch(ftp->type) {
			case DATAT_ENTRY:
				F[i].type = STRING;
				p = strsep(&ftp->data, ",");
				if (ftp->data == NULL)
					ftp->data = strdup("");
				ftp->elen = atoi(p);
				cur = tstring(max, ftp->data);
				if (cur)
					F[i].v = strdup(args[cur-1]);
				else
					F[i].v = strdup(ftp->data);
				break;
			case DATAT_LIST:
				F[i].type = ENUM;
				F[i].v = strdup(ftp->data);
				gen_list(ftp, max, args);
				F[i].list = ftp->list;
				break;
			case DATAT_FUNC:
				F[i].type = ENUM;
				F[i].v = strdup(ftp->data);
				gen_func(ftp, max, args);
				F[i].list = ftp->list;
				break;
			case DATAT_SCRIPT:
				F[i].type = ENUM;
				F[i].v = strdup(ftp->data);
				gen_script(ftp, basedir, max, args);
				F[i].list = ftp->list;
				break;
			case DATAT_MLIST:
				F[i].type = MULTI;
				F[i].v = strdup(ftp->data);
				gen_list(ftp, max, args);
				F[i].list = ftp->list;
				break;
			case DATAT_MFUNC:
				F[i].type = MULTI;
				F[i].v = strdup(ftp->data);
				gen_func(ftp, max, args);
				F[i].list = ftp->list;
				break;
			case DATAT_MSCRIPT:
				F[i].type = MULTI;
				F[i].v = strdup(ftp->data);
				gen_script(ftp, basedir, max, args);
				F[i].list = ftp->list;
				break;
			case DATAT_NOEDIT:
				F[i].type = NOEDIT;
				cur = tstring(max, ftp->data);
				if (cur)
					F[i].v = strdup(args[cur-1]);
				else
					F[i].v = strdup(ftp->data);
				ftp->elen = strlen(F[i].v);
				break;
			case DATAT_INVIS:
				F[i].type = INVIS;
				cur = tstring(max, ftp->data);
				if (cur)
					F[i].v = strdup(args[cur-1]);
				else
					F[i].v = strdup(ftp->data);
				ftp->elen = strlen(F[i].v);
				break;
			case DATAT_INTEGER:
				F[i].type = INTEGER;
				F[i].v = gen_integer(ftp, max, args);
				break;
			case DATAT_ISCRIPT:
				F[i].type = INTEGER;
				F[i].v = gen_iscript(ftp, basedir, max, args);
				break;					
			case DATAT_ESCRIPT:
				F[i].type = STRING;
				p = strsep(&ftp->data, ",");
				ftp->elen = atoi(p);
				F[i].v = gen_escript(ftp, basedir, max, args);
				if (strlen(F[i].v) > ftp->elen)
					ftp->elen = strlen(F[i].v);
				break;
			case DATAT_NESCRIPT:
				F[i].type = NOEDIT;
				F[i].v = gen_escript(ftp, basedir, max, args);
				ftp->elen = strlen(F[i].v);
				break;
			case DATAT_V4SCRIPT:
				F[i].type = IPV4;
				F[i].v = gen_escript(ftp, basedir, max, args);
				ftp->elen = 18;
				break;
			case DATAT_V4:
				F[i].type = IPV4;
				cur = tstring(max, ftp->data);
				if (cur)
					F[i].v = strdup(args[cur-1]);
				else {
					if (ftp->data == NULL)
						ftp->data = strdup("");
					F[i].v = strdup(ftp->data);
				}
				ftp->elen = 18;
				break;
			case DATAT_V6SCRIPT:
				F[i].type = IPV6;
				F[i].v = gen_escript(ftp, basedir, max, args);
				ftp->elen = 48;
				break;
			case DATAT_V6:
				F[i].type = IPV6;
				cur = tstring(max, ftp->data);
				if (cur)
					F[i].v = strdup(args[cur-1]);
				else {
					if (ftp->data == NULL)
						ftp->data = strdup("");
					F[i].v = strdup(ftp->data);
				}
				ftp->elen = 48;
				break;
			}
			F[i].rows = 1;
			F[i].rcols = F[i].cols = strlen_data(ftp);
			if (F[i].cols > 19) {
				F[i].bigfield = 1;
				F[i].cols = 19;
			} else
				F[i].bigfield = 0;
			F[i].newpage = 0;
			F[i].fcol = 55;
			F[i].frow = lrow;
			i++;
		}
		lrow++;
	}
	/* generate the final entry */
	F[i].type = (PF_field)NULL;
	F[i].rows = 0;
	F[i].cols = 0;
	F[i].frow = 0;
	F[i].fcol = 0;
	F[i].v = (char *)NULL;
	F[i].list = (char **)NULL;
	F[i].newpage = 0;
}

static void
tab_help(FORM *form)
{
	CDKLABEL *label;
	char *msg[4];
	char *buffer;
	FIELD *curfield;
	int lines;

	curfield = current_field(form);
	buffer = field_buffer(curfield, 1);
	if (buffer == NULL)
		return;
	if (*buffer == 's') {
		msg[0] = catgets(catalog, 3, 10, "The current field is a list "
		    "field, and selections can only be made from");
		msg[1] = catgets(catalog, 3, 11, "the pre-defined list.  "
		    "Please use TAB or the List(F4) command to edit.");
		msg[2] = catgets(catalog, 3, 12, "Press the ENTER key to "
		    "return from the list popup.");
		lines = 3;
	} else if (*buffer == 'm') {
		msg[0] = catgets(catalog, 3, 13, "The current field is a "
		    "multiple-selection list field.  The field can only");
		msg[1] = catgets(catalog, 3, 14, "be edited by issuing "
		    "the List(F4) command, and toggling desired options");
		msg[2] = catgets(catalog, 3, 15, "with the spacebar.  "
		    "Press the ENTER key to return from the list popup.");
		lines = 3;
	} else
		return;

	label = newCDKLabel(cdkscreen, CENTER, CENTER, msg, lines, TRUE, FALSE);
	activateCDKLabel(label, NULL);
	waitCDKLabel(label, NULL);
	destroyCDKLabel(label);
	touchwin(stdscr);
	wrefresh(stdscr);
}

int
handle_form(char *basedir, char *path, char **args)
{
	WINDOW *formwin, *boxwin;
	CDKLABEL *label;
	char *msg[1];
	FORM *menuform;
	FIELD **f;
	int done = FALSE;
	int c, i, j;
	FTREE_ENTRY *ftp;

	CIRCLEQ_INIT(&cqFormHead);
	cqFormHeadp = &cqFormHead;
	if (scan_form(cqFormHeadp, path))
		return(1);

	F = malloc(sizeof(FIELD_RECORD) *
	    (form_entries(&cqFormHead) * 2 + 1));
	if (F == NULL)
		bailout("malloc: %s", strerror(errno));
	fflush(NULL);

	/* generate a label to let the user know we are thinking */
	msg[0] = catgets(catalog, 4, 2, "Generating form data, please wait");
	label = newCDKLabel(cdkscreen, CENTER, CENTER, msg, 1, TRUE, FALSE);
	activateCDKLabel(label, NULL);

	if (args == NULL)
		form_generate(&cqFormHead, basedir, (char **)NULL);
	else
		form_generate(&cqFormHead, basedir, args);

	if (!(menuform = new_form(make_fields())))
		bailout(catgets(catalog, 1, 15, "error return from new_form"));

	set_form_userptr(menuform, "post");

	destroyCDKLabel(label);

	wclear(stdscr);
	formwin = subwin(stdscr, ws.ws_row-5, ws.ws_col-2, 1, 1);
	boxwin = subwin(stdscr, ws.ws_row-3, ws.ws_col, 0, 0);
	bottom_help(2);
	box(boxwin, 0, 0);
	keypad(formwin, TRUE);
	set_form_sub(menuform, formwin);
	post_form(menuform);
	wrefresh(stdscr);
	wrefresh(boxwin);
	form_status(menuform);
	while (!done) {
		switch (form_driver(menuform, c = get_request(formwin))) {
		case E_OK:
			if (c == REQ_NEXT_PAGE || c == REQ_PREV_PAGE ||
			    c == REQ_FIRST_PAGE || c == REQ_LAST_PAGE)
				form_status(menuform);
			break;
		case E_UNKNOWN_COMMAND:
			done = my_driver(menuform, c, basedir);
			break;
		case E_REQUEST_DENIED:
			tab_help(menuform);
			break;
		default:
			break;
		}
	}
	f = form_fields(menuform);
	unpost_form(menuform);
	while (*f)
		free_field(*f++);
	free_form(menuform);
	for (i=0; F[i].type != NULL; i++) {
		free(F[i].v);
		if (F[i].list != NULL) {
			for (j=0; F[i].list[j] != NULL; j++)
				free(F[i].list[j]);
			free(F[i].list);
		}
	}
	free(F);
	ftp = CIRCLEQ_FIRST(&cqFormHead);
	for (; !CIRCLEQ_EMPTY(&cqFormHead); ftp = CIRCLEQ_FIRST(&cqFormHead)) {
		CIRCLEQ_REMOVE(&cqFormHead, ftp, cqFormEntries);
		free(ftp->desc);
		free(ftp->origdata);
		free(ftp);
	}
	delwin(formwin);
	delwin(boxwin);
	wclear(stdscr);
	wrefresh(stdscr);
	if (done == 2)
		return(1);

	return(0);
}	

int
handle_preform(char *basedir, char *path)
{
	WINDOW *formwin, *boxwin;
	CDKLABEL *label;
	char *msg[1];
	FORM *menuform;
	FIELD **f;
	int done = FALSE;
	int c, i, j;
	char *args[2];
	FTREE_ENTRY *ftp;

	CIRCLEQ_INIT(&cqFormHead);
	cqFormHeadp = &cqFormHead;
	if (scan_form(cqFormHeadp, path))
		return(1);

	F = malloc(sizeof(FIELD_RECORD) *
	    (form_entries(&cqFormHead) * 2 + 1));
	if (F == NULL)
		bailout("malloc: %s", strerror(errno));
	fflush(NULL);

	/* generate a label to let the user know we are thinking */
	msg[0] = catgets(catalog, 4, 2, "Generating form data, please wait");
	label = newCDKLabel(cdkscreen, CENTER, CENTER, msg, 1, TRUE, FALSE);
	activateCDKLabel(label, NULL);

	args[0] = NULL;
	form_generate(&cqFormHead, basedir, args);

	if (!(menuform = new_form(make_fields())))
		bailout(catgets(catalog, 1, 15, "error return from new_form"));

	set_form_userptr(menuform, "pre");

	destroyCDKLabel(label);

	wclear(stdscr);
	formwin = subwin(stdscr, ws.ws_row-5, ws.ws_col-2, 1, 1);
	boxwin = subwin(stdscr, ws.ws_row-3, ws.ws_col, 0, 0);
	bottom_help(2);
	box(boxwin, 0, 0);
	keypad(formwin, TRUE);
	set_form_sub(menuform, formwin);
	post_form(menuform);
	wrefresh(stdscr);
	form_status(menuform);
	while (!done) {
		switch (form_driver(menuform, c = get_request(formwin))) {
		case E_OK:
			if (c == REQ_NEXT_PAGE || c == REQ_PREV_PAGE ||
			    c == REQ_FIRST_PAGE || c == REQ_LAST_PAGE)
				form_status(menuform);
			break;
		case E_UNKNOWN_COMMAND:
			done = my_driver(menuform, c, basedir);
			break;
		case E_REQUEST_DENIED:
			tab_help(menuform);
			break;
		default:
			break;
		}
	}
	f = form_fields(menuform);
	unpost_form(menuform);
	while (*f)
		free_field(*f++);
	free_form(menuform);
	if (done == 3) {
		for (i=0; F[i].type != NULL; i++) {
			free(F[i].v);
			if (F[i].list != NULL) {
				for (j=0; F[i].list[j] != NULL; j++)
					free(F[i].list[j]);
				free(F[i].list);
			}
		}
		free(F);
	}
	ftp = CIRCLEQ_FIRST(&cqFormHead);
	for (; !CIRCLEQ_EMPTY(&cqFormHead); ftp = CIRCLEQ_FIRST(&cqFormHead)) {
		CIRCLEQ_REMOVE(&cqFormHead, ftp, cqFormEntries);
		free(ftp->desc);
		free(ftp->origdata);
		free(ftp);
	}
	delwin(formwin);
	delwin(boxwin);
	wclear(stdscr);
	wrefresh(stdscr);
	if (done == 2)
		return(1);

	return(0);
}	
