/*
 *                     RCS rcsdiff operation
 */
/*****************************************************************************
 *                       generate difference between RCS revisions
 *****************************************************************************
 */

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

#include "rcsbase.h"

#if DIFF_L
static char const *setup_label P((struct buf*,char const*,char const[datesize]));
#endif
static void cleanup P((void));

static int exitstatus;
static RILE *workptr;
static struct stat workstat;

mainProg(rcsdiffId, "rcsdiff", "$Id: rcsdiff.c,v 1.2 1993/08/02 17:47:51 mycroft Exp $")
{
    static char const cmdusage[] =
	    "\nrcsdiff usage: rcsdiff [-q] [-rrev1 [-rrev2]] [-Vn] [diff options] file ...";

    int  revnums;                 /* counter for revision numbers given */
    char const *rev1, *rev2;	/* revision numbers from command line */
    char const *xrev1, *xrev2;	/* expanded revision numbers */
    char const *expandarg, *lexpandarg, *versionarg;
#if DIFF_L
    static struct buf labelbuf[2];
    int file_labels;
    char const **diff_label1, **diff_label2;
    char date2[datesize];
#endif
    char const *cov[9];
    char const **diffv, **diffp;	/* argv for subsidiary diff */
    char const **pp, *p, *diffvstr;
    struct buf commarg;
    struct buf numericrev;	/* expanded revision number */
    struct hshentries *gendeltas;	/* deltas to be generated */
    struct hshentry * target;
    char *a, *dcp, **newargv;
    register c;

    exitstatus = DIFF_SUCCESS;

    bufautobegin(&commarg);
    bufautobegin(&numericrev);
    revnums = 0;
    rev1 = rev2 = xrev2 = nil;
#if DIFF_L
    file_labels = 0;
#endif
    expandarg = versionarg = 0;
    suffixes = X_DEFAULT;

    /* Room for args + 2 i/o [+ 2 labels] + 1 file + 1 trailing null.  */
    diffp = diffv = tnalloc(char const*, argc + 4 + 2*DIFF_L);
    *diffp++ = nil;
    *diffp++ = nil;
    *diffp++ = DIFF;

    argc = getRCSINIT(argc, argv, &newargv);
    argv = newargv;
    while (a = *++argv,  0<--argc && *a++=='-') {
	dcp = a;
	while (c = *a++) switch (c) {
	    case 'r':
		    switch (++revnums) {
			case 1: rev1=a; break;
			case 2: rev2=a; break;
			default: faterror("too many revision numbers");
		    }
		    goto option_handled;
#if DIFF_L
	    case 'L':
		    if (++file_labels == 2)
			faterror("too many -L options");
		    /* fall into */
#endif
	    case 'C': case 'D': case 'F': case 'I':
		    *dcp++ = c;
		    if (*a)
			do *dcp++ = *a++;
			while (*a);
		    else {
			if (!--argc)
			    faterror("-%c needs following argument%s",
				    c, cmdusage
			    );
			*diffp++ = *argv++;
		    }
		    break;
	    case 'B': case 'H': case 'T':
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
	    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	    case 'h': case 'i': case 'n': case 'p':
	    case 't': case 'u': case 'w':
		    *dcp++ = c;
		    break;
	    case 'q':
		    quietflag=true;
		    break;
	    case 'x':
		    suffixes = *argv + 2;
		    goto option_handled;
	    case 'V':
		    versionarg = *argv;
		    setRCSversion(versionarg);
		    goto option_handled;
	    case 'k':
		    expandarg = *argv;
		    if (0 <= str2expmode(expandarg+2))
			goto option_handled;
		    /* fall into */
	    default:
		    faterror("unknown option: %s%s", *argv, cmdusage);
	    };
      option_handled:
	if (dcp != *argv+1) {
	    *dcp = 0;
	    *diffp++ = *argv;
	}
    } /* end of option processing */

    if (argc<1) faterror("no input file%s", cmdusage);

    for (pp = diffv+3, c = 0;  pp<diffp;  )
	    c += strlen(*pp++) + 1;
    diffvstr = a = tnalloc(char, c + 1);
    for (pp = diffv+3;  pp<diffp;  ) {
	    p = *pp++;
	    *a++ = ' ';
	    while ((*a = *p++))
		    a++;
    }
    *a = 0;

#if DIFF_L
    diff_label1 = diff_label2 = nil;
    if (file_labels < 2) {
	    if (!file_labels)
		    diff_label1 = diffp++;
	    diff_label2 = diffp++;
    }
#endif
    diffp[2] = nil;

    cov[0] = 0;
    cov[2] = CO;
    cov[3] = "-q";

    /* now handle all filenames */
    do {
	    ffree();

	    if (pairfilenames(argc, argv, rcsreadopen, true, false)  <=  0)
		    continue;
	    diagnose("===================================================================\nRCS file: %s\n",RCSfilename);
	    if (!rev2) {
		/* Make sure work file is readable, and get its status.  */
		if (!(workptr = Iopen(workfilename,FOPEN_R_WORK,&workstat))) {
		    eerror(workfilename);
		    continue;
		}
	    }


	    gettree(); /* reads in the delta tree */

	    if (Head==nil) {
		    error("no revisions present");
		    continue;
	    }
	    if (revnums==0  ||  !*rev1)
		    rev1  =  Dbranch ? Dbranch : Head->num;

	    if (!fexpandsym(rev1, &numericrev, workptr)) continue;
	    if (!(target=genrevs(numericrev.string,(char *)nil,(char *)nil,(char *)nil,&gendeltas))) continue;
	    xrev1=target->num;
#if DIFF_L
	    if (diff_label1)
		*diff_label1 = setup_label(&labelbuf[0], target->num, target->date);
#endif

	    lexpandarg = expandarg;
	    if (revnums==2) {
		    if (!fexpandsym(
			    *rev2 ? rev2  : Dbranch ? Dbranch  : Head->num,
			    &numericrev,
			    workptr
		    ))
			continue;
		    if (!(target=genrevs(numericrev.string,(char *)nil,(char *)nil,(char *)nil,&gendeltas))) continue;
		    xrev2=target->num;
	    } else if (
			target->lockedby
		&&	!lexpandarg
		&&	Expand == KEYVAL_EXPAND
		&&	WORKMODE(RCSstat.st_mode,true) == workstat.st_mode
	    )
		    lexpandarg = "-kkvl";
	    Izclose(&workptr);
#if DIFF_L
	    if (diff_label2)
		if (revnums == 2)
		    *diff_label2 = setup_label(&labelbuf[1], target->num, target->date);
		else {
		    time2date(workstat.st_mtime, date2);
		    *diff_label2 = setup_label(&labelbuf[1], workfilename, date2);
		}
#endif

	    diagnose("retrieving revision %s\n", xrev1);
	    bufscpy(&commarg, "-p");
	    bufscat(&commarg, xrev1);

	    cov[1] = diffp[0] = maketemp(0);
	    pp = &cov[4];
	    *pp++ = commarg.string;
	    if (lexpandarg)
		    *pp++ = lexpandarg;
	    if (versionarg)
		    *pp++ = versionarg;
	    *pp++ = RCSfilename;
	    *pp = 0;

	    if (runv(cov)) {
		    error("co failed");
		    continue;
	    }
	    if (!rev2) {
		    diffp[1] = workfilename;
		    if (workfilename[0] == '+') {
			/* Some diffs have options with leading '+'.  */
			char *dp = ftnalloc(char, strlen(workfilename)+3);
			diffp[1] = dp;
			*dp++ = '.';
			*dp++ = SLASH;
			VOID strcpy(dp, workfilename);
		    }
	    } else {
		    diagnose("retrieving revision %s\n",xrev2);
		    bufscpy(&commarg, "-p");
		    bufscat(&commarg, xrev2);
		    cov[1] = diffp[1] = maketemp(1);
		    cov[4] = commarg.string;
		    if (runv(cov)) {
			    error("co failed");
			    continue;
		    }
	    }
	    if (!rev2)
		    diagnose("diff%s -r%s %s\n", diffvstr, xrev1, workfilename);
	    else
		    diagnose("diff%s -r%s -r%s\n", diffvstr, xrev1, xrev2);

	    switch (runv(diffv)) {
		    case DIFF_SUCCESS:
			    break;
		    case DIFF_FAILURE:
			    if (exitstatus == DIFF_SUCCESS)
				    exitstatus = DIFF_FAILURE;
			    break;
		    default:
			    error("diff failed");
	    }
    } while (cleanup(),
	     ++argv, --argc >=1);


    tempunlink();
    exitmain(exitstatus);
}

    static void
cleanup()
{
    if (nerror) exitstatus = DIFF_TROUBLE;
    Izclose(&finptr);
    Izclose(&workptr);
}

#if lint
#	define exiterr rdiffExit
#endif
    exiting void
exiterr()
{
    tempunlink();
    _exit(DIFF_TROUBLE);
}

#if DIFF_L
	static char const *
setup_label(b, name, date)
	struct buf *b;
	char const *name;
	char const date[datesize];
{
	char *p;
	size_t l = strlen(name) + 3;
	bufalloc(b, l+datesize);
	p = b->string;
	VOID sprintf(p, "-L%s\t", name);
	VOID date2str(date, p+l);
	return p;
}
#endif
