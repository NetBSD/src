/*  G C M D L I N  --  gkermit command line parser  */

/*
  Author:
    Frank da Cruz
    The Kermit Project
    Columbia University
    612 West 115th Street
    New York NY 10025-7799  USA
    http://www.columbia.edu/kermit/
    kermit@columbia.edu

  Copyright (C) 1999,
  The Trustees of Columbia University in the City of New York.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gkermit.h"

/* Externals */

extern int nfils, parity, text, backup, rpsiz, urpsiz, timint;
extern int literal, quiet, keep, streamok, nomodes, manual, xonxoff, noxonxoff;
extern char ttname[], *cmerrp, *cmarg, *cmarg2;
extern FILE * db;

/* Variables exported from this module */

extern char **cmlist;			/* Pointer to file list in argv */
extern char **xargv;			/* Global copies of argv */
extern int xargc;			/* and argc  */

/* Variables and symbols local to this module */

static int action = 0;			/* Action selected on command line */

_MYPROTOTYPE( static int doarg, (char) );
_MYPROTOTYPE( VOID fatal, (char *) );
_MYPROTOTYPE( VOID usage, (void) );

#ifndef NOGETENV
#define GARGC 32
#define GBUFSIZ 256
static char gbuf[GBUFSIZ], *gargs[GARGC], *gptr = NULL;
static int gargc;
#endif /* NOGETENV */

int					/* Command-line parser */
cmdlin() {
    char c;
    int x;
#ifndef NOGETENV
    char * p = NULL;
#endif /* NOGETENV */

    cmarg = "";				/* Initialize results */
    cmlist = NULL;
    action = 0;

#ifndef NOGETENV
    if ((p = getenv("GKERMIT"))) {
	int i, xc, flag = 0;
	char **xv = NULL;

	strncpy(gbuf,p,GBUFSIZ-1);	/* Make a pokeable copy */
	gbuf[GBUFSIZ-1] = NUL;
	gptr = p;
	p = gbuf;

	/* Turn it into an argument vector */

	for (i = 0; gbuf[i] && i < GBUFSIZ && gargc < GARGC; i++) {
	    if (!flag) {
		if (gbuf[i] <= SP)
		  continue;
		flag = 1;
		gargs[gargc++] = &gbuf[i];
	    } else if (flag && gbuf[i] <= SP) {
		gbuf[i] = NUL;
		flag = 0;
		continue;
	    }
	}
	xv = xargv;			/* Save original argument vector */
	xc = xargc;
	xargv = gargs;			/* Redirect it to the one we made */
	xargc = gargc;

	while (xargc-- > 0) {		/* Go through the words */
	    if (**xargv == '-') {	/* Got an option (begins with dash) */
		c = *(*xargv+1);	/* Get the option letter */
		x = doarg(c);		/* Go handle the option */
		if (x < 0) doexit(1);
	    } else {			/* No dash where expected */
		fprintf(stderr,
			"?GKERMIT variable option error: \"%s\"\n",
			*xargv
			);
		usage();		/* Give usage message */
		doexit(0);
	    }
	    xargv++;
	}
	xargv = xv;			/* Restore original argument vector */
	xargc = xc;
    }
#endif /* NOGETENV */

    while (--xargc > 0) {		/* Go through command line words */
	xargv++;
    	if (**xargv == '-') {		/* Got an option (begins with dash) */
	    c = *(*xargv+1);		/* Get the option letter */
	    x = doarg(c);		/* Go handle the option */
	    if (x < 0) doexit(1);
    	} else {			/* No dash where expected */
	    fprintf(stderr,"?Command-line option error: \"%s\"\n", *xargv);
	    usage();			/* Give usage message */
	    doexit(1);
	}
    }
    return(action);			/* Then do any requested protocol */
}

/*  D O A R G  --  Do a command-line argument.  */

static int
#ifdef __STDC__
doarg(char x)
#else
doarg(x) char x;
#endif /* __STDC__ */
{
    int z; char *xp, **p;

    xp = *xargv+1;			/* Pointer for bundled args */
    while (x) {
	switch (x) {
	  case 'r':			/* Receive */
	    if (action) fatal("conflicting actions");
	    action = 'v';
	    break;

	  case 's':			/* Send */
	    if (action) fatal("conflicting actions");
	    if (*(xp+1)) fatal("invalid argument bundling after -s");
	    nfils = 0;			/* Initialize file counter, flag */
	    cmlist = xargv+1;		/* Remember this pointer */
	    if (zchki(*cmlist) < 0)
	      fatal("file not found or not accessible");
	    while (--xargc > 0) {	/* Traverse the list */
		*xargv++;
		if (**xargv == '-')
		  break;
		nfils++;
	    }
	    xargc++, *xargv--;		/* Adjust argv/argc */
	    if (nfils < 1) fatal("missing filename for -s");
	    action = 's';
	    break;

	  case 'g':			/* get */
	    if (action) fatal("conflicting actions");
	    if (*(xp+1)) fatal("invalid argument bundling after -g");
	    *xargv++, xargc--;
	    if ((xargc == 0) || (**xargv == '-'))
	      fatal("missing filename for -g");
	    cmarg = *xargv;
	    action = 'r';
	    break;

	  case 'h':			/* Help */
	  case '?':
	    usage();
	    doexit(0);

	  case 'i':			/* Binary (image) file transfer */
	    manual = 1;
	    text = 0;
	    break;

	  case 'T':			/* Text file transfer */
	    manual = 1;
	    text = 1;
	    break;

	  case 'p':			/* Parity */
	    if (*(xp+1)) fatal("invalid argument bundling");
	    *xargv++, xargc--;
	    if ((xargc < 1) || (**xargv == '-'))
	      fatal("missing parity");
	    switch(x = **xargv) {
	      case 'e':			/* Even */
	      case 'o':			/* Odd */
	      case 'm':			/* Mark */
	      case 's': parity = x; break; /* Space */
	      case 'n': parity = 0; break; /* None */
	      default:  fatal("invalid parity");
	    }
	    break;

	  case 'w':			/* Writeover */
	    backup = 0;			/* Don't back up existing files */
	    break;

	  case 'd':			/* Debug */
	    p = xargv;
	    *p++;
	    if ((xargc < 2) || (**p == '-')) {
		db = fopen("debug.log","w");
	    } else {
		*xargv++, xargc--;
		db = fopen(*xargv,"w");
	    }
	    if (db) {
		extern char * versio, * build;
		if (!versio) versio = "";
		if (!build) build = "";
		debug = 1;
		setbuf(db,NULL);
		fprintf(db,"%s: %s\n",
			(*versio ? versio : "GKERMIT VERSION UNKNOWN"),
			(*build ? build : "BUILD UNKNOWN")
			);
		fprintf(db,"MAXPATHLEN = %d\n",MAXPATHLEN);
		if (gptr) fprintf(db,"GKERMIT=\"%s\"\n",gptr);
	    }
	    break;

	  case 'a':			/* As-name */
	    if (*(xp+1)) fatal("invalid argument bundling after -a");
	    *xargv++, xargc--;
	    if ((xargc == 0) || (**xargv == '-'))
	      fatal("missing name for -a");
	    cmarg2 = *xargv;
	    if (debug) fprintf(db,"as-name: %s\n",cmarg2);
	    break;

	  case 'e':
	    if (*(xp+1)) fatal("invalid argument bundling after -e");
	    xargv++, xargc--;
	    if ((xargc < 1) || (**xargv == '-'))
	      fatal("missing length");
	    z = atoi(*xargv);		/* Convert to number. */
	    if (z >= 40 && z <= MAXRP) {
		rpsiz = urpsiz = z;
		if (z > 94) rpsiz = 94;	/* Fallback if other Kermit can't */
	    } else {			/* do long packets. */
		fatal("Unsupported packet length");
	    }
	    break;

	  case 'b':			/* Timeout */
	    if (*(xp+1)) fatal("invalid argument bundling after -b");
	    xargv++, xargc--;
	    if ((xargc < 1) || (**xargv == '-'))
	      fatal("missing value");
	    z = atoi(*xargv);		/* Convert to number */
	    if (z < 0) z = 0;
	    if (z > 90) z = 90;
	    timint = z;
	    break;

	  case 'P':			/* Path (file) names literal */
	    literal = 1;
	    break;

	  case 'q':			/* Quiet */
	    quiet = 1;
	    break;

	  case 'K':			/* Keep incompletely received files */
	    keep = 1;
	    break;

	  case 'S':			/* Disable streaming */
	    streamok = -1;
	    break;

	  case 'X':			/* gkermit is an external protocol */
	    quiet = 1;			/* No messages */
            nomodes = 1;		/* Don't set tty modes */
	    break;

	  case 'x':			/* Force Xon/Xoff */
	    xonxoff = 1;
	    noxonxoff = 0;
	    break;

	  case '-':			/* Don't force Xon/Xoff */
	    if (*(xp+1) == 'x') {
		xonxoff = 0;
		noxonxoff = 1;
	    } else {
		fatal("invalid argument, type 'gkermit -h' for help");
	    }
	    xp++;
	    break;

	  default:			/* Anything else */
	    fatal("invalid argument, type 'gkermit -h' for help");
        }
	x = *++xp;			/* See if options are bundled */
    }
    if (debug) {
	if (action)
	  fprintf(db,"cmdlin action = %c\n",action);
	else
	  fprintf(db,"cmdlin action = (none)\n");
    }
    return(action);
}

VOID
fatal(msg) char *msg; {			/* Fatal error message */
    fprintf(stderr,"\r\nFatal: %s",msg); /* doexit supplies crlf.. */
    doexit(1);				/* Exit indicating failure */
}

VOID
usage() {
    extern char * versio, * build, * url, * email;
    if (!versio) versio = "";
    if (!*versio) versio = "gkermit UNKNOWN VERSION";
    if (!build) build = "UNKNOWN BUILD";
    if (!url) url = "";
    if (!email) email = "";
    fprintf(stderr,"%s: %s.\n",versio,build);
    fprintf(stderr,"Usage:  gkermit [ options ]\n");
    fprintf(stderr,"Options:\n");
    fprintf(stderr," -r      Receive files\n");
    fprintf(stderr," -s fn   Send files\n");
    fprintf(stderr," -g fn   Get files from server\n");
    fprintf(stderr," -a fn   As-name for single file\n");
    fprintf(stderr," -i      Image (binary) mode transfer\n");
    fprintf(stderr," -T      Text mode transfer\n");
    fprintf(stderr," -P      Path/filename conversion disabled\n");
    fprintf(stderr," -w      Write over existing files with same name\n");
    fprintf(stderr," -K      Keep incompletely received files\n");
    fprintf(stderr," -p x    Parity: x = o[dd],e[ven],m[ark],s[pace],n[one]\n")
      ;
    fprintf(stderr," -e n    Receive packet-length (40-%d)\n",MAXRP);
    fprintf(stderr," -b n    Timeout (sec, 0 = none)\n");
    fprintf(stderr," -x      Force Xon/Xoff (--x = Don't force Xon/Xoff)\n");
    fprintf(stderr," -S      Disable streaming\n");
    fprintf(stderr," -X      External protocol\n");
    fprintf(stderr," -q      Quiet (suppress messages)\n");
    fprintf(stderr," -d [fn] Debug to ./debug.log [or specified file]\n");
    fprintf(stderr," -h      Help (this message)\n");
    if (*url || *email)
      fprintf(stderr,"More info: %s <%s>",url,email);
}
