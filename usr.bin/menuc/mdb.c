/*	$NetBSD: mdb.c,v 1.2 1997/11/09 20:59:14 phil Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
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
 *      This product includes software develooped for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* mdb.c - menu database manipulation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mdb.h"
#include "defs.h"

/* Data */
#define MAX 500
static menu_no = 0;
static id_rec *menus[MAX];


/* get_menu returns a pointer to a newly created id_rec or an old one. */

id_rec *
get_menu (char *name)
{
	id_rec *temp;

	temp = find_id (root, name);

	if (temp == NULL) {
		if (menu_no < MAX) {
			temp = (id_rec *) malloc (sizeof(id_rec));
			temp->id = strdup(name);
			temp->info = NULL;
			temp->menu_no = menu_no;
			menus[menu_no++] = temp;
			insert_id (&root, temp);
		} else {
			(void) fprintf (stderr, "Too many menus.  "
					"Increase MAX.\n");
			exit(1);
		}
	}

	return temp;
}


/* Verify that all menus are defined. */

void
check_defined (void)
{
	int i;

	for (i=0; i<menu_no; i++)
		if (!menus[i]->info)
			yyerror ("Menu '%s' undefined.", menus[i]->id);
}


/* Write out the menu file. */
void
write_menu_file (char *initcode)
{
	FILE *out_file;
	FILE *sys_file;
	int i, j;
	char hname[1024];
	char cname[1024];
	char sname[1024];
	char *sys_prefix;

	int nlen;

	char opt_ch;
	char ch;

	optn_info *toptn;

	/* Generate file names */
	snprintf (hname, 1024, "%s.h", out_name);
	nlen = strlen(hname);
	if (hname[nlen-2] != '.' || hname[nlen-1] != 'h') {
		(void) fprintf (stderr, "%s: name `%s` too long.\n",
				prog_name, out_name);
		exit(1);
	}
	snprintf (cname, 1024, "%s.c", out_name);

	/* Open the menu_sys file first. */
	sys_prefix = getenv ("MENUDEF");
	if (sys_prefix == NULL)
		sys_prefix = "/usr/share/misc";
	snprintf (sname, 1024, "%s/%s", sys_prefix, sys_name);
	sys_file = fopen (sname, "r");
	if (sys_file == NULL) {
		(void) fprintf (stderr, "%s: could not open %s.\n",
				prog_name, sname);
		exit (1);
	}

	/* Output the .h file first. */
	out_file = fopen (hname, "w");
	if (out_file == NULL) {
		(void) fprintf (stderr, "%s: could not open %s.\n",
				prog_name, hname);
		exit (1);
	}

	/* Write it */
	(void) fprintf (out_file, "%s",
		"/* menu system definitions. */\n"
		"\n"
		"#ifndef MENU_DEFS_H\n"
		"#define MENU_DEFS_H\n"
		"#include <stdlib.h>\n"
		"#include <string.h>\n"
		"#include <ctype.h>\n"
		"#include <curses.h>\n"
		"\n"
		"typedef\n"
		"struct menudesc {\n"
		"	char   *title;\n"
		"	int     y, x;\n"
		"	int	h, w;\n"
		"	int	mopt;\n"
		"	int     numopts;\n"
		"	int	cursel;\n"
		"	char   **opts;\n"
		"	WINDOW *mw;\n"
		"} menudesc ;\n"
		"\n"
		"/* defines for mopt field. */\n"
		"#define NOEXITOPT 1\n"
		"#define NOBOX 2\n"
		"\n"
		"/* initilization flag */\n"
		"extern int __m_endwin;\n"
		"\n"
		"/* Prototypes */\n"
		"void process_menu (int num);\n"
		"void __menu_initerror (void);\n"    
		"\n"
		"/* Menu names */\n"
	      );
	for (i=0; i<menu_no; i++) {
		(void) fprintf (out_file, "#define MENU_%s\t%d\n",
				menus[i]->id, i);
	}
	(void) fprintf (out_file, "\n#define MAX_STRLEN %d\n", max_strlen);
	(void) fprintf (out_file, "#endif\n");

	fclose (out_file);

	/* Now the C file */
	out_file = fopen (cname, "w");
	if (out_file == NULL) {
		(void) fprintf (stderr, "%s: could not open %s.\n",
				prog_name, cname);
		exit (1);
	}

	/* initial code */
	fprintf (out_file, "#include \"%s\"\n\n", hname);
	fprintf (out_file, "%s\n\n", initcode);

	/* data definitions */

	/* optstrX */
	for (i=0; i<menu_no; i++) {
		(void) fprintf (out_file, "static char *optstr%d[] = {\n", i);
		toptn = menus[i]->info->optns;
		opt_ch = 'a';
		while (toptn != NULL) {
			(void) fprintf (out_file, "\t\"%c: %s,\n", opt_ch++,
					toptn->name+1);
			toptn = toptn->next;
		}
		(void) fprintf (out_file, "\t(char *)NULL\n};\n\n");
	}

	/* menus */
	(void) fprintf (out_file, "static struct menudesc menus[] = {\n");
	for (i=0; i<menu_no; i++)
		(void) fprintf (out_file,
			"\t{%s,%d,%d,%d,%d,%d,%d,0,optstr%d,NULL},\n",
			menus[i]->info->title, 	menus[i]->info->y,
			menus[i]->info->x, menus[i]->info->h,
			menus[i]->info->w, menus[i]->info->mopt,
			menus[i]->info->numopt, i);
	(void) fprintf (out_file, "{NULL}};\n\n");

	/* num_menus */
	(void) fprintf (out_file, "int num_menus = %d;\n\n", menu_no);

	/* action code */
	(void) fprintf (out_file, "%s",
		"static int process_item (int *menu_no, int sel)\n"
		"{\n"
		"\tint retval = FALSE;\n"
		"\n"
		"\tswitch (*menu_no) {\n"
	);
	for (i=0; i<menu_no; i++) {
		(void) fprintf (out_file, "\tcase MENU_%s:\n",
				menus[i]->id);
		(void) fprintf (out_file, "\t\tswitch (sel) {\n"
				"\t\tcase -2:\n");
		if (menus[i]->info->postact.endwin)
			(void) fprintf (out_file, "\t\t\tendwin();\n"
					"\t\t__m_endwin = 1;\n");
		if (strlen(menus[i]->info->postact.code))
			(void) fprintf (out_file, "\t\t\t{%s}\n",
					menus[i]->info->postact.code);
		(void) fprintf (out_file, "\t\t\tbreak;\n");
		(void) fprintf (out_file, "\t\tcase -1:\n");
		if (menus[i]->info->exitact.endwin)
			(void) fprintf (out_file, "\t\t\tendwin();\n"
					"\t\t__m_endwin = 1;\n");
		if (strlen(menus[i]->info->exitact.code))
			(void) fprintf (out_file, "\t\t\t{%s}\n",
					menus[i]->info->exitact.code);
		(void) fprintf (out_file, "\t\t\tbreak;\n");
		j = 0;
		toptn = menus[i]->info->optns;
		while (toptn != NULL) {
			(void) fprintf (out_file, "\t\tcase %d:\n", j++);
			if (toptn->optact.endwin)
				(void) fprintf (out_file, "\t\t\tendwin();\n"
						"\t\t__m_endwin = 1;\n");
			if (strlen(toptn->optact.code))
				(void) fprintf (out_file, "\t\t\t{%s}\n",
						toptn->optact.code);
			if (toptn->menu >= 0)
				if (toptn->issub)
					(void) fprintf (out_file,
						"\t\t\tprocess_menu(%d);\n",
						toptn->menu);
				else
					(void) fprintf (out_file,
						"\t\t\t*menu_no = %d;\n",
						toptn->menu);
			if (toptn->doexit)
				(void) fprintf (out_file,
						"\t\t\tretval = TRUE;\n");
			(void) fprintf (out_file, "\t\t\tbreak;\n");
			toptn = toptn->next;
		}

		(void) fprintf (out_file, "\t\t}\n\t\tbreak;\n");
	}
	(void) fprintf (out_file, "\t}\n\t return retval;\n}\n\n");

	while ((ch = fgetc(sys_file)) != EOF)
		fputc(ch, out_file);     	
	
	/* __menu_initerror: initscr failed. */
	(void) fprintf (out_file, 
		"/* __menu_initerror: initscr failed. */\n"
		"void __menu_initerror (void) {\n");
	if (error_act.code == NULL) {
		(void) fprintf (out_file,
			"\t(void) fprintf (stderr, "
				"\"Could not initialize curses\\n\");\n"
			"\texit(1);\n"
			"}\n");
	} else {
		if (error_act.endwin)
			(void) fprintf (out_file, "\tendwin();\n");
		(void) fprintf (out_file, "%s\n}\n", error_act.code);
	}

	fclose (out_file);
	fclose (sys_file);
}
