/*	$NetBSD: mdb.c,v 1.18 2000/12/22 02:52:47 mrg Exp $	*/

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
#define MAX 1000
static int menu_no = 0;
static id_rec *menus[MAX];

/* Other defines */
#define OPT_SUB    1
#define OPT_ENDWIN 2
#define OPT_EXIT   4


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
	char *tmpstr;

	int nlen;

	int ch;

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
		"#include <curses.h>\n\n"
		);

	if (do_dynamic)
		(void) fprintf (out_file, "#define DYNAMIC_MENUS\n\n");

	(void) fprintf (out_file,
		"struct menudesc;\n"	
		"typedef\n"
		"struct menu_ent {\n"
		"	char   *opt_name;\n"
		"	int	opt_menu;\n"
		"	int	opt_flags;\n"
		"	int	(*opt_action)(struct menudesc *);\n"
		"} menu_ent ;\n\n"
		"#define OPT_SUB    1\n"
		"#define OPT_ENDWIN 2\n"
		"#define OPT_EXIT   4\n"
		"#define OPT_NOMENU -1\n\n"
		"typedef\n"
		"struct menudesc {\n"
		"	char     *title;\n"
		"	int      y, x;\n"
		"	int	 h, w;\n"
		"	int	 mopt;\n"
		"	int      numopts;\n"
		"	int	 cursel;\n"
		"	int	 topline;\n"
		"	menu_ent *opts;\n"
		"	WINDOW   *mw;\n"
		"	char     *helpstr;\n"
		"	char     *exitstr;\n"
		"	void    (*post_act)(void);\n"
		"	void    (*exit_act)(void);\n"
		"} menudesc ;\n"
		"\n"
		"/* defines for mopt field. */\n"
		"#define MC_NOEXITOPT 1\n"
		"#define MC_NOBOX 2\n"
		"#define MC_SCROLL 4\n"
		"#define MC_NOSHORTCUT 8\n"
		"#define MC_VALID 256\n"	
		);

	(void) fprintf (out_file, "%s",
		"\n"
		"/* initilization flag */\n"
		"extern int __m_endwin;\n"
		"\n"
		"/* Prototypes */\n"
		"int menu_init (void);\n"
		"void process_menu (int num);\n"
		"void __menu_initerror (void);\n"
		);

	if (do_dynamic)
		(void) fprintf (out_file, "%s",
			"int new_menu (char * title, menu_ent * opts, "
				"int numopts, \n"
				"\tint x, int y, int h, int w, int mopt,\n"
				"\tvoid (*post_act)(void), void (*exit_act), "
				"char * help);\n"
			"void free_menu (int menu_no);\n"
			);

	(void) fprintf (out_file, "\n/* Menu names */\n");
	for (i=0; i<menu_no; i++) {
		(void) fprintf (out_file, "#define MENU_%s\t%d\n",
				menus[i]->id, i);
	}

	if (do_dynamic)
		(void) fprintf (out_file, "\n#define DYN_MENU_START\t%d",
				menu_no);

	(void) fprintf (out_file, "\n#define MAX_STRLEN %d\n"
			"#endif\n", max_strlen);
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

	/* func defs */
	for (i=0; i<menu_no; i++) {
		if (strlen(menus[i]->info->postact.code)) {
			(void) fprintf (out_file,
				"void menu_%d_postact(void);\n"
				"void menu_%d_postact(void)\n{", i, i);
			if (menus[i]->info->postact.endwin)
				(void) fprintf (out_file, "\tendwin();\n"
					"\t__m_endwin = 1;\n");
			(void) fprintf (out_file,
					"\t%s\n}\n",
					menus[i]->info->postact.code);
		}
		if (strlen(menus[i]->info->exitact.code)) {
			(void) fprintf (out_file,
				"void menu_%d_exitact(void);\n"
				"void menu_%d_exitact(void)\n{", i, i);
			if (menus[i]->info->exitact.endwin)
				(void) fprintf (out_file, "\tendwin();\n"
					"\t__m_endwin = 1;\n");
			(void) fprintf (out_file, "\t%s\n}\n\n",
					menus[i]->info->exitact.code);
		}
		j = 0;
		toptn = menus[i]->info->optns;
		while (toptn != NULL) {
			if (strlen(toptn->optact.code)) {
				(void) fprintf (out_file,
					"int opt_act_%d_%d(menudesc *m);\n"
					"int opt_act_%d_%d(menudesc *m)\n"
					"{\t%s\n\treturn %s;\n}\n\n",
					i, j, i, j, toptn->optact.code,
					(toptn->doexit ? "1" : "0"));
						
				
			}
			j++;
			toptn = toptn->next;
		}

	}

	/* optentX */
	for (i=0; i<menu_no; i++) {
		if (menus[i]->info->numopt > 53) {
			(void) fprintf (stderr, "Menu %s has "
				"too many options.\n",
				menus[i]->info->title);
			exit (1);
		}
		toptn = menus[i]->info->optns;
		j = 0;
		(void) fprintf (out_file,
				"static menu_ent optent%d[] = {\n", i);
		while (toptn != NULL) {
			(void) fprintf (out_file, "\t{\"%s,%d,%d,",
				toptn->name+1, toptn->menu,
				(toptn->issub ? OPT_SUB : 0)
				+(toptn->doexit ? OPT_EXIT : 0)
				+(toptn->optact.endwin ? OPT_ENDWIN : 0));
			if (strlen(toptn->optact.code)) 
				(void) fprintf (out_file, "opt_act_%d_%d}",
					i, j);
			else
				(void) fprintf (out_file, "NULL}");
			(void) fprintf (out_file, "%s\n",
				(toptn->next ? "," : ""));
			j++;
			toptn = toptn->next;
		}
		(void) fprintf (out_file, "\t};\n\n");

	}


	/* menus */
	(void) fprintf (out_file, "static struct menudesc menu_def[] = {\n");
	for (i=0; i<menu_no; i++) {
		(void) fprintf (out_file,
			"\t{%s,%d,%d,%d,%d,%d,%d,0,0,optent%d,NULL,",
			menus[i]->info->title, 	menus[i]->info->y,
			menus[i]->info->x, menus[i]->info->h,
			menus[i]->info->w, menus[i]->info->mopt,
			menus[i]->info->numopt, i);
		if (menus[i]->info->helpstr == NULL)
			(void) fprintf (out_file, "NULL");
		else {
			tmpstr = menus[i]->info->helpstr;
			/* Skip an initial newline. */
			if (*tmpstr == '\n')
				tmpstr++;
			(void) fprintf (out_file, "\n\"");
			while (*tmpstr)
				if (*tmpstr != '\n')
				  fputc (*tmpstr++, out_file);
				else {
					(void) fprintf (out_file, "\\n\\\n");
					tmpstr++;
				}
			(void) fprintf (out_file, "\"");
		}
		(void) fprintf (out_file, ",");
		if (menus[i]->info->mopt & NOEXITOPT)
			(void) fprintf (out_file, "NULL");
		else if (menus[i]->info->exitstr != NULL)
			(void) fprintf (out_file, "%s",
				menus[i]->info->exitstr);
		else
			(void) fprintf (out_file, "\"Exit\"");
		if (strlen(menus[i]->info->postact.code))
			(void) fprintf (out_file, ",menu_%d_postact", i);
		else
			(void) fprintf (out_file, ",NULL");
		if (strlen(menus[i]->info->exitact.code))
			(void) fprintf (out_file, ",menu_%d_exitact", i);
		else
			(void) fprintf (out_file, ",NULL");

		(void) fprintf (out_file, "},\n");

	}
	(void) fprintf (out_file, "{NULL}};\n\n");

	/* __menu_initerror: initscr failed. */
	(void) fprintf (out_file, 
		"/* __menu_initerror: initscr failed. */\n"
		"void __menu_initerror (void) {\n");
	if (error_act.code == NULL) {
		(void) fprintf (out_file,
			"\t(void) fprintf (stderr, "
			"\"%%s: Could not initialize curses\\n\",prog_name);\n"
			"\texit(1);\n"
			"}\n");
	} else {
		if (error_act.endwin)
			(void) fprintf (out_file, "\tendwin();\n");
		(void) fprintf (out_file, "%s\n}\n", error_act.code);
	}

	/* Copy menu_sys.def file. */
	while ((ch = fgetc(sys_file)) != '\014')  /* Control-L */
		fputc(ch, out_file);     	

	if (do_dynamic) {
		while ((ch = fgetc(sys_file)) != '\n')
			/* skip it */;
		while ((ch = fgetc(sys_file)) != EOF)
			fputc(ch, out_file);
	}
		
	fclose (out_file);
	fclose (sys_file);
}
