/* Copyright (C) 1982, 1988, 1989 Walter Tichy
   Copyright 1990, 1991 by Paul Eggert
   Distributed under license by the Free Software Foundation, Inc.

This file is part of RCS.

RCS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

RCS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RCS; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

Report problems and direct all questions to:

    rcs-bugs@cs.purdue.edu

*/

/*
 *                     RCS identification operation
 */

#include  "rcsbase.h"

static int match P((FILE*));
static void scanfile P((FILE*,char const*,int));

mainProg(identId, "ident", "$Id: ident.c,v 1.2 1993/08/02 17:47:04 mycroft Exp $")
/*  Ident searches the named files for all occurrences
 *  of the pattern $keyword:...$, where the keywords are
 *  Author, Date, Header, Id, Log, RCSfile, Revision, Source, and State.
 */

{
   FILE *fp;
   int quiet;
   int status = EXIT_SUCCESS;

   if ((quiet  =  argc > 1 && strcmp("-q",argv[1])==0)) {
        argc--; argv++;
   }

   if (argc<2)
	scanfile(stdin, (char*)0, quiet);

   while ( --argc > 0 ) {
      if (!(fp = fopen(*++argv, FOPEN_R))) {
	 VOID fprintf(stderr,  "%s error: can't open %s\n", cmdid, *argv);
	 status = EXIT_FAILURE;
      } else {
	 scanfile(fp, *argv, quiet);
	 if (argc>1) VOID putchar('\n');
      }
   }
   if (ferror(stdout) || fclose(stdout)!=0) {
      VOID fprintf(stderr,  "%s error: write error\n", cmdid);
      status = EXIT_FAILURE;
   }
   exitmain(status);
}

#if lint
	exiting void identExit() { _exit(EXIT_FAILURE); }
#endif


	static void
scanfile(file, name, quiet)
	register FILE *file;
	char const *name;
	int quiet;
/* Function: scan an open file with descriptor file for keywords.
 * Return false if there's a read error.
 */
{
   register int c;

   if (name)
      VOID printf("%s:\n", name);
   else
      name = "input";
   c = 0;
   for (;;) {
      if (c < 0) {
	 if (feof(file))
	    break;
	 if (ferror(file))
	    goto read_error;
      }
      if (c == KDELIM) {
	 if ((c = match(file)))
	    continue;
	 quiet = true;
      }
      c = getc(file);
   }
   if (!quiet)
      VOID fprintf(stderr, "%s warning: no id keywords in %s\n", cmdid, name);
   if (fclose(file) == 0)
      return;

 read_error:
   VOID fprintf(stderr, "%s error: %s: read error\n", cmdid, name);
   exit(EXIT_FAILURE);
}



	static int
match(fp)   /* group substring between two KDELIM's; then do pattern match */
   register FILE *fp;
{
   char line[BUFSIZ];
   register int c;
   register char * tp;

   tp = line;
   while ((c = getc(fp)) != VDELIM) {
      if (c < 0)
	 return c;
      switch (ctab[c]) {
	 case LETTER: case Letter:
	    *tp++ = c;
	    if (tp < line+sizeof(line)-4)
	       break;
	    /* fall into */
	 default:
	    return c ? c : '\n'/* anything but 0 or KDELIM or EOF */;
      }
   }
   if (tp == line)
      return c;
   *tp++ = c;
   if ((c = getc(fp)) != ' ')
      return c ? c : '\n';
   *tp++ = c;
   while( (c = getc(fp)) != KDELIM ) {
      if (c < 0  &&  feof(fp) | ferror(fp))
	    return c;
      switch (ctab[c]) {
	 default:
	    *tp++ = c;
	    if (tp < line+sizeof(line)-2)
	       break;
	    /* fall into */
	 case NEWLN: case UNKN:
	    return c ? c : '\n';
      }
   }
   if (tp[-1] != ' ')
      return c;
   *tp++ = c;     /*append trailing KDELIM*/
   *tp   = '\0';
   VOID fprintf(stdout, "     %c%s\n", KDELIM, line);
   return 0;
}
