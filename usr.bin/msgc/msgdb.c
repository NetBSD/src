/*	$NetBSD: msgdb.c,v 1.9 1999/07/04 10:35:19 cgd Exp $	*/

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

/* mdb.c - message database manipulation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"

static struct id_rec *head = NULL, *tail = NULL;
static int msg_no = 0;

void define_msg (char *name, char *value)
{
	struct id_rec *tmp = (struct id_rec *)malloc(sizeof(struct id_rec));

	if (find_id (root, name))
		yyerror ("%s is defined twice", name);

	tmp->id     = name;
	tmp->msg    = value;
	tmp->msg_no = msg_no++;
	tmp->next   = NULL;
	if (tail == NULL)
		head = tail = tmp;
	else {
		tail->next = tmp;
		tail = tmp;
	}

	insert_id (&root, tmp);
}

static void write_str (FILE *f, char *str)
{
	(void)fprintf (f, "\"");
	while (*str) {
		if (*str == '\n')
			(void) fprintf (f, "\\n\"\n\""), str++;
		else if (*str == '"')
			(void) fprintf (f, "\\\""), str++;
		else
			(void) fprintf (f, "%c", *str++);
	}
	(void)fprintf (f, "\",");
}

/* Write out the msg files. */
void
write_msg_file ()
{
	FILE *out_file;
	FILE *sys_file;
	char hname[1024];
	char cname[1024];
	char sname[1024];
	char *sys_prefix;

	int nlen;
	int ch;

	struct id_rec *t;

	/* Generate file names */
	snprintf (hname, 1024, "%s.h", out_name);
	nlen = strlen(hname);
	if (hname[nlen-2] != '.' || hname[nlen-1] != 'h') {
		(void) fprintf (stderr, "%s: name `%s` too long.\n",
				prog_name, out_name);
		exit(1);
	}
	snprintf (cname, 1024, "%s.c", out_name);

	/* Open the msg_sys file first. */
	sys_prefix = getenv ("MSGDEF");
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
		"/* msg system definitions. */\n"
		"\n"
		"#ifndef MSG_DEFS_H\n"
		"#define MSG_DEFS_H\n"
		"#include <stdio.h>\n"
		"#include <stdlib.h>\n"
		"#include <string.h>\n"
		"#include <ctype.h>\n"
		"#include <stdarg.h>\n"
		"#include <curses.h>\n"
		"\n"
		"/* Prototypes */\n"
		"int  msg_window(WINDOW *window);\n"
		"char *msg_string (int msg_no);\n"
		"void msg_clear(void);\n"
		"void msg_standout(void);\n"
		"void msg_standend(void);\n"
		"void msg_display(int msg_no,...);\n"
		"void msg_display_add(int msg_no,...);\n"
		"int  msg_printf (char *fmt, ...);\n"
		"int  msg_printf_add (char *fmt, ...);\n"
		"void msg_prompt (int msg_no, char *def,"
			" char *val, int max_chars, ...);\n"
		"void msg_prompt_add (int msg_no, char *def,"
			" char *val, int max_chars, ...);\n"
		"void msg_prompt_noecho (int msg_no, char *def,"
			" char *val, int max_chars, ...);\n"
		"void msg_table_add(int msg_no,...);\n"
		"\n"
		"/* Message names */\n"
	      );
	for (t=head; t != NULL; t = t->next) {
		(void) fprintf (out_file, "#define MSG_%s\t%d\n",
				t->id, t->msg_no);
	}
	(void) fprintf (out_file, "\n#endif\n");

	fclose (out_file);

	/* Now the C file */
	out_file = fopen (cname, "w");
	if (out_file == NULL) {
		(void) fprintf (stderr, "%s: could not open %s.\n",
				prog_name, cname);
		exit (1);
	}

	/* hfile include ... */
	(void)fprintf (out_file, "#include \"%s\"\n", hname);

	/* msg_list */
	(void)fprintf (out_file, "char *msg_list[] = {\n");
	for (t=head ; t != NULL; t = t->next) 
		write_str (out_file, t->msg);
	(void)fprintf (out_file, "NULL};\n");

	/* sys file out! */
	while ((ch = fgetc(sys_file)) != EOF)
		fputc(ch, out_file);     	
	
	fclose (out_file);
	fclose (sys_file);
}
