/*
 *                      RCS create/change operation
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

struct  Lockrev {
	char const *revno;
        struct  Lockrev   * nextrev;
};

struct  Symrev {
	char const *revno;
	char const *ssymbol;
        int     override;
        struct  Symrev  * nextsym;
};

struct Message {
	char const *revno;
	struct cbuf message;
	struct Message *nextmessage;
};

struct  Status {
	char const *revno;
	char const *status;
        struct  Status  * nextstatus;
};

enum changeaccess {append, erase};
struct chaccess {
	char const *login;
	enum changeaccess command;
	struct chaccess *nextchaccess;
};

struct delrevpair {
	char const *strt;
	char const *end;
        int     code;
};

static int buildeltatext P((struct hshentries const*));
static int removerevs P((void));
static int sendmail P((char const*,char const*));
static struct Lockrev *rmnewlocklst P((struct Lockrev const*));
static void breaklock P((struct hshentry const*));
static void buildtree P((void));
static void cleanup P((void));
static void doaccess P((void));
static void doassoc P((void));
static void dolocks P((void));
static void domessages P((void));
static void getaccessor P((char*,enum changeaccess));
static void getassoclst P((int,char*));
static void getchaccess P((char const*,enum changeaccess));
static void getdelrev P((char*));
static void getmessage P((char*));
static void getstates P((char*));
static void rcs_setstate P((char const*,char const*));
static void scanlogtext P((struct hshentry*,int));
static void setlock P((char const*));

static struct buf numrev;
static char const *headstate;
static int chgheadstate, exitstatus, lockhead, unlockcaller;
static struct Lockrev *newlocklst, *rmvlocklst;
static struct Message *messagelst, *lastmessage;
static struct Status *statelst, *laststate;
static struct Symrev *assoclst, *lastassoc;
static struct chaccess *chaccess, **nextchaccess;
static struct delrevpair delrev;
static struct hshentry *cuthead, *cuttail, *delstrt;
static struct hshentries *gendeltas;

mainProg(rcsId, "rcs", "$Id: rcs.c,v 1.2 1993/08/02 17:47:42 mycroft Exp $")
{
	static char const cmdusage[] =
		"\nrcs usage: rcs -{ae}logins -Afile -{blu}[rev] -cstring -{iLU} -{nNs}name[:rev] -orange -t[file] -Vn file ...";

	char *a, **newargv, *textfile;
	char const *branchsym, *commsyml;
	int branchflag, expmode, initflag;
	int e, r, strictlock, strict_selected, textflag;
	mode_t defaultRCSmode;	/* default mode for new RCS files */
	mode_t RCSmode;
	struct buf branchnum;
	struct stat workstat;
        struct  Lockrev *curlock,  * rmvlock, *lockpt;
        struct  Status  * curstate;

	nosetid();

	nextchaccess = &chaccess;
	branchsym = commsyml = textfile = nil;
	branchflag = strictlock = false;
	bufautobegin(&branchnum);
	curlock = rmvlock = nil;
	defaultRCSmode = 0;
	expmode = -1;
	suffixes = X_DEFAULT;
        initflag= textflag = false;
        strict_selected = 0;

        /*  preprocessing command options    */
	argc = getRCSINIT(argc, argv, &newargv);
	argv = newargv;
	while (a = *++argv,  0<--argc && *a++=='-') {
		switch (*a++) {

		case 'i':   /*  initial version  */
                        initflag = true;
                        break;

                case 'b':  /* change default branch */
			if (branchflag) redefined('b');
                        branchflag= true;
			branchsym = a;
                        break;

                case 'c':   /*  change comment symbol   */
			if (commsyml) redefined('c');
			commsyml = a;
                        break;

                case 'a':  /*  add new accessor   */
			getaccessor(*argv+1, append);
                        break;

                case 'A':  /*  append access list according to accessfile  */
			if (!*a) {
			    error("missing file name after -A");
                            break;
                        }
			*argv = a;
			if (0 < pairfilenames(1,argv,rcsreadopen,true,false)) {
			    while (AccessList) {
				getchaccess(str_save(AccessList->login),append);
				AccessList = AccessList->nextaccess;
			    }
			    Izclose(&finptr);
                        }
                        break;

                case 'e':    /*  remove accessors   */
			getaccessor(*argv+1, erase);
                        break;

                case 'l':    /*   lock a revision if it is unlocked   */
			if (!*a) {
			    /* Lock head or default branch.  */
                            lockhead = true;
                            break;
                        }
			lockpt = talloc(struct Lockrev);
			lockpt->revno = a;
                        lockpt->nextrev = nil;
                        if ( curlock )
                            curlock->nextrev = lockpt;
                        else
                            newlocklst = lockpt;
                        curlock = lockpt;
                        break;

                case 'u':   /*  release lock of a locked revision   */
			if (!*a) {
                            unlockcaller=true;
                            break;
                        }
			lockpt = talloc(struct Lockrev);
			lockpt->revno = a;
                        lockpt->nextrev = nil;
                        if (rmvlock)
                            rmvlock->nextrev = lockpt;
                        else
                            rmvlocklst = lockpt;
                        rmvlock = lockpt;

                        curlock = rmnewlocklst(lockpt);
                        break;

                case 'L':   /*  set strict locking */
                        if (strict_selected++) {  /* Already selected L or U? */
			   if (!strictlock)	  /* Already selected -U? */
			       warn("-L overrides -U.");
                        }
                        strictlock = true;
                        break;

                case 'U':   /*  release strict locking */
                        if (strict_selected++) {  /* Already selected L or U? */
			   if (strictlock)	  /* Already selected -L? */
			       warn("-L overrides -U.");
                        }
			else
			    strictlock = false;
                        break;

                case 'n':    /*  add new association: error, if name exists */
			if (!*a) {
			    error("missing symbolic name after -n");
                            break;
                        }
                        getassoclst(false, (*argv)+1);
                        break;

                case 'N':   /*  add or change association   */
			if (!*a) {
			    error("missing symbolic name after -N");
                            break;
                        }
                        getassoclst(true, (*argv)+1);
                        break;

		case 'm':   /*  change log message  */
			getmessage(a);
			break;

		case 'o':   /*  delete revisions  */
			if (delrev.strt) redefined('o');
			if (!*a) {
			    error("missing revision range after -o");
                            break;
                        }
                        getdelrev( (*argv)+1 );
                        break;

                case 's':   /*  change state attribute of a revision  */
			if (!*a) {
			    error("state missing after -s");
                            break;
                        }
                        getstates( (*argv)+1);
                        break;

                case 't':   /*  change descriptive text   */
                        textflag=true;
			if (*a) {
				if (textfile) redefined('t');
				textfile = a;
                        }
                        break;

		case 'I':
			interactiveflag = true;
			break;

                case 'q':
                        quietflag = true;
                        break;

		case 'x':
			suffixes = a;
			break;

		case 'V':
			setRCSversion(*argv);
			break;

		case 'k':    /*  set keyword expand mode  */
			if (0 <= expmode) redefined('k');
			if (0 <= (expmode = str2expmode(a)))
			    break;
			/* fall into */
                default:
			faterror("unknown option: %s%s", *argv, cmdusage);
                };
        }  /* end processing of options */

	if (argc<1) faterror("no input file%s", cmdusage);
        if (nerror) {
	    diagnose("%s aborted\n",cmdid);
	    exitmain(EXIT_FAILURE);
        }
	if (initflag) {
	    defaultRCSmode = umask((mode_t)0);
	    VOID umask(defaultRCSmode);
	    defaultRCSmode = (S_IRUSR|S_IRGRP|S_IROTH) & ~defaultRCSmode;
	}

        /* now handle all filenames */
        do {
	ffree();

        if ( initflag ) {
	    switch (pairfilenames(argc, argv, rcswriteopen, false, false)) {
                case -1: break;        /*  not exist; ok */
                case  0: continue;     /*  error         */
                case  1: error("file %s exists already", RCSfilename);
                         continue;
            }
	}
        else  {
	    switch (pairfilenames(argc, argv, rcswriteopen, true, false)) {
                case -1: continue;    /*  not exist      */
                case  0: continue;    /*  errors         */
                case  1: break;       /*  file exists; ok*/
            }
	}


        /* now RCSfilename contains the name of the RCS file, and
         * workfilename contains the name of the working file.
         * if !initflag, finptr contains the file descriptor for the
         * RCS file. The admin node is initialized.
         */

	diagnose("RCS file: %s\n", RCSfilename);

	RCSmode = defaultRCSmode;
	if (initflag) {
		if (stat(workfilename, &workstat) == 0)
			RCSmode = workstat.st_mode;
	} else {
		if (!checkaccesslist()) continue;
		gettree(); /* Read the delta tree.  */
		RCSmode = RCSstat.st_mode;
	}
	RCSmode &= ~(S_IWUSR|S_IWGRP|S_IWOTH);

        /*  update admin. node    */
        if (strict_selected) StrictLocks = strictlock;
	if (commsyml) {
		Comment.string = commsyml;
		Comment.size = strlen(commsyml);
	}
	if (0 <= expmode) Expand = expmode;

        /* update default branch */
	if (branchflag && expandsym(branchsym, &branchnum)) {
	    if (countnumflds(branchnum.string)) {
		Dbranch = branchnum.string;
            } else
                Dbranch = nil;
        }

	doaccess();	/* Update access list.  */

	doassoc();	/* Update association list.  */

	dolocks();	/* Update locks.  */

	domessages();	/* Update log messages.  */

        /*  update state attribution  */
        if (chgheadstate) {
            /* change state of default branch or head */
            if (Dbranch==nil) {
                if (Head==nil)
		     warn("can't change states in an empty tree");
                else Head->state = headstate;
            } else {
		rcs_setstate(Dbranch,headstate); /* Can't set directly */
            }
        }
        curstate = statelst;
        while( curstate ) {
            rcs_setstate(curstate->revno,curstate->status);
            curstate = curstate->nextstatus;
        }

        cuthead = cuttail = nil;
	if (delrev.strt && removerevs()) {
            /*  rebuild delta tree if some deltas are deleted   */
            if ( cuttail )
		VOID genrevs(cuttail->num, (char *)nil,(char *)nil,
			     (char *)nil, &gendeltas);
            buildtree();
        }

	if (nerror)
		continue;

        putadmin(frewrite);
        if ( Head )
           puttree(Head, frewrite);
	putdesc(textflag,textfile);

        if ( Head) {
	    if (!delrev.strt && !messagelst) {
		/* No revision was deleted and no message was changed.  */
		fastcopy(finptr, frewrite);
            } else {
		if (!cuttail || buildeltatext(gendeltas)) {
		    advise_access(finptr, MADV_SEQUENTIAL);
		    scanlogtext((struct hshentry *)nil, false);
                    /* copy rest of delta text nodes that are not deleted      */
		}
            }
        }
	Izclose(&finptr);
        if ( ! nerror ) {  /*  move temporary file to RCS file if no error */
	    /* update mode */
	    ignoreints();
	    r = chnamemod(&frewrite, newRCSfilename, RCSfilename, RCSmode);
	    e = errno;
	    keepdirtemp(newRCSfilename);
	    restoreints();
	    if (r != 0) {
		enerror(e, RCSfilename);
		error("saved in %s", newRCSfilename);
		dirtempunlink();
                break;
            }
	    diagnose("done\n");
        } else {
	    diagnose("%s aborted; %s unchanged.\n",cmdid,RCSfilename);
        }
	} while (cleanup(),
                 ++argv, --argc >=1);

	tempunlink();
	exitmain(exitstatus);
}       /* end of main (rcs) */

	static void
cleanup()
{
	if (nerror) exitstatus = EXIT_FAILURE;
	Izclose(&finptr);
	Ozclose(&fcopy);
	Ozclose(&frewrite);
	dirtempunlink();
}

	exiting void
exiterr()
{
	dirtempunlink();
	tempunlink();
	_exit(EXIT_FAILURE);
}


	static void
getassoclst(flag, sp)
int     flag;
char    * sp;
/*  Function:   associate a symbolic name to a revision or branch,      */
/*              and store in assoclst                                   */

{
        struct   Symrev  * pt;
	char const *temp;
        int                c;

        while( (c=(*++sp)) == ' ' || c == '\t' || c =='\n')  ;
        temp = sp;
	sp = checkid(sp, ':');  /*  check for invalid symbolic name  */
	c = *sp;   *sp = '\0';
        while( c == ' ' || c == '\t' || c == '\n')  c = *++sp;

        if ( c != ':' && c != '\0') {
	    error("invalid string %s after option -n or -N",sp);
            return;
        }

	pt = talloc(struct Symrev);
        pt->ssymbol = temp;
        pt->override = flag;
	if (c == '\0')  /*  delete symbol  */
            pt->revno = nil;
        else {
            while( (c = *++sp) == ' ' || c == '\n' || c == '\t')  ;
	    pt->revno = sp;
        }
        pt->nextsym = nil;
        if (lastassoc)
            lastassoc->nextsym = pt;
        else
            assoclst = pt;
        lastassoc = pt;
        return;
}


	static void
getchaccess(login, command)
	char const *login;
	enum changeaccess command;
{
	register struct chaccess *pt;

	*nextchaccess = pt = talloc(struct chaccess);
	pt->login = login;
	pt->command = command;
	pt->nextchaccess = nil;
	nextchaccess = &pt->nextchaccess;
}



	static void
getaccessor(opt, command)
	char *opt;
	enum changeaccess command;
/*   Function:  get the accessor list of options -e and -a,     */
/*		and store in chaccess				*/


{
        register c;
	register char *sp;

	sp = opt;
        while( ( c = *++sp) == ' ' || c == '\n' || c == '\t' || c == ',') ;
        if ( c == '\0') {
	    if (command == erase  &&  sp-opt == 1) {
		getchaccess((char const*)nil, command);
		return;
	    }
	    error("missing login name after option -a or -e");
	    return;
        }

        while( c != '\0') {
		getchaccess(sp, command);
		sp = checkid(sp,',');
		c = *sp;   *sp = '\0';
                while( c == ' ' || c == '\n' || c == '\t'|| c == ',')c =(*++sp);
        }
}


	static void
getmessage(option)
	char *option;
{
	struct Message *pt;
	struct cbuf cb;
	char *m;

	if (!(m = strchr(option, ':'))) {
		error("-m option lacks revision number");
		return;
	}
	*m++ = 0;
	cb = cleanlogmsg(m, strlen(m));
	if (!cb.size) {
		error("-m option lacks log message");
		return;
	}
	pt = talloc(struct Message);
	pt->revno = option;
	pt->message = cb;
	pt->nextmessage = 0;
	if (lastmessage)
		lastmessage->nextmessage = pt;
	else
		messagelst = pt;
	lastmessage = pt;
}


	static void
getstates(sp)
char    *sp;
/*   Function:  get one state attribute and the corresponding   */
/*              revision and store in statelst                  */

{
	char const *temp;
        struct  Status  *pt;
        register        c;

        while( (c=(*++sp)) ==' ' || c == '\t' || c == '\n')  ;
        temp = sp;
	sp = checkid(sp,':');  /* check for invalid state attribute */
	c = *sp;   *sp = '\0';
        while( c == ' ' || c == '\t' || c == '\n' )  c = *++sp;

        if ( c == '\0' ) {  /*  change state of def. branch or Head  */
            chgheadstate = true;
            headstate  = temp;
            return;
        }
        else if ( c != ':' ) {
	    error("missing ':' after state in option -s");
            return;
        }

        while( (c = *++sp) == ' ' || c == '\t' || c == '\n')  ;
	pt = talloc(struct Status);
        pt->status     = temp;
        pt->revno      = sp;
        pt->nextstatus = nil;
        if (laststate)
            laststate->nextstatus = pt;
        else
            statelst = pt;
        laststate = pt;
}



	static void
getdelrev(sp)
char    *sp;
/*   Function:  get revision range or branch to be deleted,     */
/*              and place in delrev                             */
{
        int    c;
        struct  delrevpair      *pt;
	int separator;

	pt = &delrev;
        while((c = (*++sp)) == ' ' || c == '\n' || c == '\t') ;

	/* Support old ambiguous '-' syntax; this will go away.  */
	if (strchr(sp,':'))
		separator = ':';
	else {
		if (strchr(sp,'-')  &&  VERSION(5) <= RCSversion)
		    warn("`-' is obsolete in `-o%s'; use `:' instead", sp);
		separator = '-';
	}

	if (c == separator) { /* -o:rev */
            while( (c = (*++sp)) == ' ' || c == '\n' || c == '\t')  ;
            pt->strt = sp;    pt->code = 1;
            while( c != ' ' && c != '\n' && c != '\t' && c != '\0') c =(*++sp);
            *sp = '\0';
	    pt->end = nil;
            return;
        }
        else {
            pt->strt = sp;
            while( c != ' ' && c != '\n' && c != '\t' && c != '\0'
		   && c != separator )  c = *++sp;
            *sp = '\0';
            while( c == ' ' || c == '\n' || c == '\t' )  c = *++sp;
            if ( c == '\0' )  {  /*   -o rev or branch   */
                pt->end = nil;   pt->code = 0;
                return;
            }
	    if (c != separator) {
		faterror("invalid range %s %s after -o", pt->strt, sp);
            }
            while( (c = *++sp) == ' ' || c == '\n' || c == '\t')  ;
	    if (!c) {  /* -orev: */
                pt->end = nil;   pt->code = 2;
                return;
            }
        }
	/* -orev1:rev2 */
	pt->end = sp;  pt->code = 3;
        while( c!= ' ' && c != '\n' && c != '\t' && c != '\0') c = *++sp;
        *sp = '\0';
}




	static void
scanlogtext(delta,edit)
	struct hshentry *delta;
	int edit;
/* Function: Scans delta text nodes up to and including the one given
 * by delta, or up to last one present, if delta==nil.
 * For the one given by delta (if delta!=nil), the log message is saved into
 * delta->log if delta==cuttail; the text is edited if EDIT is set, else copied.
 * Assumes the initial lexeme must be read in first.
 * Does not advance nexttok after it is finished, except if delta==nil.
 */
{
	struct hshentry const *nextdelta;
	struct cbuf cb;

	for (;;) {
		foutptr = 0;
		if (eoflex()) {
                    if(delta)
			faterror("can't find delta for revision %s", delta->num);
		    return; /* no more delta text nodes */
                }
		nextlex();
		if (!(nextdelta=getnum()))
		    faterror("delta number corrupted");
		if (nextdelta->selector) {
			foutptr = frewrite;
			aprintf(frewrite,DELNUMFORM,nextdelta->num,Klog);
                }
		getkeystring(Klog);
		if (nextdelta == cuttail) {
			cb = savestring(&curlogbuf);
			if (!delta->log.string)
			    delta->log = cleanlogmsg(curlogbuf.string, cb.size);
		} else if (nextdelta->log.string && nextdelta->selector) {
			foutptr = 0;
			readstring();
			foutptr = frewrite;
			putstring(foutptr, false, nextdelta->log, true);
			afputc(nextc, foutptr);
                } else {readstring();
                }
                nextlex();
		while (nexttok==ID && strcmp(NextString,Ktext)!=0)
			ignorephrase();
		getkeystring(Ktext);

		if (delta==nextdelta)
			break;
		readstring(); /* skip over it */

	}
	/* got the one we're looking for */
	if (edit)
		editstring((struct hshentry *)nil);
	else
		enterstring();
}



	static struct Lockrev *
rmnewlocklst(which)
	struct Lockrev const *which;
/*   Function:  remove lock to revision which->revno from newlocklst   */

{
        struct  Lockrev   * pt, *pre;

        while( newlocklst && (! strcmp(newlocklst->revno, which->revno))){
	    struct Lockrev *pn = newlocklst->nextrev;
	    tfree(newlocklst);
	    newlocklst = pn;
        }

        pt = pre = newlocklst;
        while( pt ) {
            if ( ! strcmp(pt->revno, which->revno) ) {
                pre->nextrev = pt->nextrev;
		tfree(pt);
		pt = pre->nextrev;
            }
            else {
                pre = pt;
                pt = pt->nextrev;
            }
        }
        return pre;
}



	static void
doaccess()
{
	register struct chaccess *ch;
	register struct access **p, *t;

	for (ch = chaccess;  ch;  ch = ch->nextchaccess) {
		switch (ch->command) {
		case erase:
			if (!ch->login)
			    AccessList = nil;
			else
			    for (p = &AccessList;  (t = *p);  )
				if (strcmp(ch->login, t->login) == 0)
					*p = t->nextaccess;
				else
					p = &t->nextaccess;
			break;
		case append:
			for (p = &AccessList;  ;  p = &t->nextaccess)
				if (!(t = *p)) {
					*p = t = ftalloc(struct access);
					t->login = ch->login;
					t->nextaccess = nil;
					break;
				} else if (strcmp(ch->login, t->login) == 0)
					break;
			break;
		}
	}
}


	static int
sendmail(Delta, who)
	char const *Delta, *who;
/*   Function:  mail to who, informing him that his lock on delta was
 *   broken by caller. Ask first whether to go ahead. Return false on
 *   error or if user decides not to break the lock.
 */
{
#ifdef SENDMAIL
	char const *messagefile;
	int old1, old2, c;
        FILE    * mailmess;
#endif


	aprintf(stderr, "Revision %s is already locked by %s.\n", Delta, who);
	if (!yesorno(false, "Do you want to break the lock? [ny](n): "))
		return false;

        /* go ahead with breaking  */
#ifdef SENDMAIL
	messagefile = maketemp(0);
	if (!(mailmess = fopen(messagefile, "w"))) {
	    efaterror(messagefile);
        }

	aprintf(mailmess, "Subject: Broken lock on %s\n\nYour lock on revision %s of file %s\nhas been broken by %s for the following reason:\n",
		basename(RCSfilename), Delta, getfullRCSname(), getcaller()
	);
	aputs("State the reason for breaking the lock:\n(terminate with single '.' or end of file)\n>> ", stderr);

        old1 = '\n';    old2 = ' ';
        for (; ;) {
	    c = getcstdin();
	    if (feof(stdin)) {
		aprintf(mailmess, "%c\n", old1);
                break;
            }
            else if ( c == '\n' && old1 == '.' && old2 == '\n')
                break;
            else {
		afputc(old1, mailmess);
                old2 = old1;   old1 = c;
		if (c=='\n') aputs(">> ", stderr);
            }
        }
	Ozclose(&mailmess);

	if (run(messagefile, (char*)nil, SENDMAIL, who, (char*)nil))
		warn("Mail may have failed."),
#else
		warn("Mail notification of broken locks is not available."),
#endif
		warn("Please tell `%s' why you broke the lock.", who);
	return(true);
}



	static void
breaklock(delta)
	struct hshentry const *delta;
/* function: Finds the lock held by caller on delta,
 * and removes it.
 * Sends mail if a lock different from the caller's is broken.
 * Prints an error message if there is no such lock or error.
 */
{
        register struct lock * next, * trail;
	char const *num;
        struct lock dummy;

	num=delta->num;
        dummy.nextlock=next=Locks;
        trail = &dummy;
        while (next!=nil) {
		if (strcmp(num, next->delta->num) == 0) {
			if (
				strcmp(getcaller(),next->login) != 0
			    &&	!sendmail(num, next->login)
			) {
			    error("%s still locked by %s", num, next->login);
			    return;
			}
			break; /* exact match */
                }
                trail=next;
                next=next->nextlock;
        }
        if (next!=nil) {
                /*found one */
		diagnose("%s unlocked\n",next->delta->num);
                trail->nextlock=next->nextlock;
                next->delta->lockedby=nil;
                Locks=dummy.nextlock;
        } else  {
		error("no lock set on revision %s", num);
        }
}



	static struct hshentry *
searchcutpt(object, length, store)
	char const *object;
	unsigned length;
	struct hshentries *store;
/*   Function:  Search store and return entry with number being object. */
/*              cuttail = nil, if the entry is Head; otherwise, cuttail */
/*              is the entry point to the one with number being object  */

{
	cuthead = nil;
	while (compartial(store->first->num, object, length)) {
		cuthead = store->first;
		store = store->rest;
	}
	return store->first;
}



	static int
branchpoint(strt, tail)
struct  hshentry        *strt,  *tail;
/*   Function: check whether the deltas between strt and tail	*/
/*		are locked or branch point, return 1 if any is  */
/*		locked or branch point; otherwise, return 0 and */
/*		mark deleted					*/

{
        struct  hshentry    *pt;
	struct lock const *lockpt;
        int     flag;


        pt = strt;
        flag = false;
        while( pt != tail) {
            if ( pt->branches ){ /*  a branch point  */
                flag = true;
		error("can't remove branch point %s", pt->num);
            }
	    lockpt = Locks;
	    while(lockpt && lockpt->delta != pt)
		lockpt = lockpt->nextlock;
	    if ( lockpt ) {
		flag = true;
		error("can't remove locked revision %s",pt->num);
	    }
            pt = pt->next;
        }

        if ( ! flag ) {
            pt = strt;
            while( pt != tail ) {
		pt->selector = false;
		diagnose("deleting revision %s\n",pt->num);
                pt = pt->next;
            }
        }
        return flag;
}



	static int
removerevs()
/*   Function:  get the revision range to be removed, and place the     */
/*              first revision removed in delstrt, the revision before  */
/*              delstrt in cuthead( nil, if delstrt is head), and the   */
/*              revision after the last removed revision in cuttail(nil */
/*              if the last is a leaf                                   */

{
	struct  hshentry *target, *target2, *temp;
	unsigned length;
	int flag;

        flag = false;
	if (!expandsym(delrev.strt, &numrev)) return 0;
	target = genrevs(numrev.string, (char*)nil, (char*)nil, (char*)nil, &gendeltas);
        if ( ! target ) return 0;
	if (cmpnum(target->num, numrev.string)) flag = true;
	length = countnumflds(numrev.string);

	if (delrev.code == 0) {  /*  -o  rev    or    -o  branch   */
	    if (length & 1)
		temp=searchcutpt(target->num,length+1,gendeltas);
	    else if (flag) {
		error("Revision %s doesn't exist.", numrev.string);
		return 0;
	    }
	    else
		temp = searchcutpt(numrev.string, length, gendeltas);
	    cuttail = target->next;
            if ( branchpoint(temp, cuttail) ) {
                cuttail = nil;
                return 0;
            }
            delstrt = temp;     /* first revision to be removed   */
            return 1;
        }

	if (length & 1) {   /*  invalid branch after -o   */
	    error("invalid branch range %s after -o", numrev.string);
            return 0;
        }

	if (delrev.code == 1) {  /*  -o  -rev   */
            if ( length > 2 ) {
                temp = searchcutpt( target->num, length-1, gendeltas);
                cuttail = target->next;
            }
            else {
                temp = searchcutpt(target->num, length, gendeltas);
                cuttail = target;
                while( cuttail && ! cmpnumfld(target->num,cuttail->num,1) )
                    cuttail = cuttail->next;
            }
            if ( branchpoint(temp, cuttail) ){
                cuttail = nil;
                return 0;
            }
            delstrt = temp;
            return 1;
        }

	if (delrev.code == 2) {   /*  -o  rev-   */
            if ( length == 2 ) {
                temp = searchcutpt(target->num, 1,gendeltas);
                if ( flag)
                    cuttail = target;
                else
                    cuttail = target->next;
            }
            else  {
                if ( flag){
                    cuthead = target;
                    if ( !(temp = target->next) ) return 0;
                }
                else
                    temp = searchcutpt(target->num, length, gendeltas);
		getbranchno(temp->num, &numrev);  /* get branch number */
		target = genrevs(numrev.string, (char*)nil, (char*)nil, (char*)nil, &gendeltas);
            }
            if ( branchpoint( temp, cuttail ) ) {
                cuttail = nil;
                return 0;
            }
            delstrt = temp;
            return 1;
        }

        /*   -o   rev1-rev2   */
	if (!expandsym(delrev.end, &numrev)) return 0;
	if (
		length != countnumflds(numrev.string)
	    ||	length>2 && compartial(numrev.string, target->num, length-1)
	) {
	    error("invalid revision range %s-%s", target->num, numrev.string);
            return 0;
        }

	target2 = genrevs(numrev.string,(char*)nil,(char*)nil,(char*)nil,&gendeltas);
        if ( ! target2 ) return 0;

        if ( length > 2) {  /* delete revisions on branches  */
            if ( cmpnum(target->num, target2->num) > 0) {
		if (cmpnum(target2->num, numrev.string))
                    flag = true;
                else
                    flag = false;
                temp = target;
                target = target2;
                target2 = temp;
            }
            if ( flag ) {
                if ( ! cmpnum(target->num, target2->num) ) {
		    error("Revisions %s-%s don't exist.", delrev.strt,delrev.end);
                    return 0;
                }
                cuthead = target;
                temp = target->next;
            }
            else
                temp = searchcutpt(target->num, length, gendeltas);
            cuttail = target2->next;
        }
        else { /*  delete revisions on trunk  */
            if ( cmpnum( target->num, target2->num) < 0 ) {
                temp = target;
                target = target2;
                target2 = temp;
            }
            else
		if (cmpnum(target2->num, numrev.string))
                    flag = true;
                else
                    flag = false;
            if ( flag ) {
                if ( ! cmpnum(target->num, target2->num) ) {
		    error("Revisions %s-%s don't exist.", delrev.strt, delrev.end);
                    return 0;
                }
                cuttail = target2;
            }
            else
                cuttail = target2->next;
            temp = searchcutpt(target->num, length, gendeltas);
        }
        if ( branchpoint(temp, cuttail) )  {
            cuttail = nil;
            return 0;
        }
        delstrt = temp;
        return 1;
}



	static void
doassoc()
/*   Function: add or delete(if revno is nil) association	*/
/*		which is stored in assoclst			*/

{
	char const *p;
	struct Symrev const *curassoc;
	struct  assoc   * pre,  * pt;

        /*  add new associations   */
        curassoc = assoclst;
        while( curassoc ) {
            if ( curassoc->revno == nil ) {  /* delete symbol  */
		pre = pt = Symbols;
                while( pt && strcmp(pt->symbol,curassoc->ssymbol) ) {
		    pre = pt;
		    pt = pt->nextassoc;
		}
		if ( pt )
		    if ( pre == pt )
			Symbols = pt->nextassoc;
		    else
			pre->nextassoc = pt->nextassoc;
		else
		    warn("can't delete nonexisting symbol %s",curassoc->ssymbol);
	    }
	    else {
		if (curassoc->revno[0]) {
		    p = 0;
		    if (expandsym(curassoc->revno, &numrev))
			p = fstr_save(numrev.string);
		} else if (!(p = tiprev()))
		    error("no latest revision to associate with symbol %s",
			    curassoc->ssymbol
		    );
		if (p)
		    VOID addsymbol(p, curassoc->ssymbol, curassoc->override);
	    }
            curassoc = curassoc->nextsym;
        }

}



	static void
dolocks()
/* Function: remove lock for caller or first lock if unlockcaller is set;
 *           remove locks which are stored in rmvlocklst,
 *           add new locks which are stored in newlocklst,
 *           add lock for Dbranch or Head if lockhead is set.
 */
{
	struct Lockrev const *lockpt;
	struct hshentry *target;

	if (unlockcaller) { /*  find lock for caller  */
            if ( Head ) {
		if (Locks) {
		    switch (findlock(true, &target)) {
		      case 0:
			breaklock(Locks->delta); /* remove most recent lock */
			break;
		      case 1:
			diagnose("%s unlocked\n",target->num);
			break;
		    }
		} else {
		    warn("No locks are set.");
		}
            } else {
		warn("can't unlock an empty tree");
            }
        }

        /*  remove locks which are stored in rmvlocklst   */
        lockpt = rmvlocklst;
        while( lockpt ) {
	    if (expandsym(lockpt->revno, &numrev)) {
		target = genrevs(numrev.string, (char *)nil, (char *)nil, (char *)nil, &gendeltas);
                if ( target )
		   if (!(countnumflds(numrev.string)&1) && cmpnum(target->num,numrev.string))
			error("can't unlock nonexisting revision %s",lockpt->revno);
                   else
			breaklock(target);
                        /* breaklock does its own diagnose */
            }
            lockpt = lockpt->nextrev;
        }

        /*  add new locks which stored in newlocklst  */
        lockpt = newlocklst;
        while( lockpt ) {
	    setlock(lockpt->revno);
            lockpt = lockpt->nextrev;
        }

	if (lockhead) {  /*  lock default branch or head  */
            if (Dbranch) {
		setlock(Dbranch);
	    } else if (Head) {
		if (0 <= addlock(Head))
		    diagnose("%s locked\n",Head->num);
            } else {
		warn("can't lock an empty tree");
            }
        }

}



	static void
setlock(rev)
	char const *rev;
/* Function: Given a revision or branch number, finds the corresponding
 * delta and locks it for caller.
 */
{
        struct  hshentry *target;

	if (expandsym(rev, &numrev)) {
	    target = genrevs(numrev.string, (char*)nil, (char*)nil,
			     (char*)nil, &gendeltas);
            if ( target )
	       if (!(countnumflds(numrev.string)&1) && cmpnum(target->num,numrev.string))
		    error("can't lock nonexisting revision %s", numrev.string);
               else
		    if (0 <= addlock(target))
			diagnose("%s locked\n", target->num);
        }
}


	static void
domessages()
{
	struct hshentry *target;
	struct Message *p;

	for (p = messagelst;  p;  p = p->nextmessage)
	    if (
		expandsym(p->revno, &numrev)  &&
		(target = genrevs(
			numrev.string, (char*)0, (char*)0, (char*)0, &gendeltas
		))
	    )
		target->log = p->message;
}


	static void
rcs_setstate(rev,status)
	char const *rev, *status;
/* Function: Given a revision or branch number, finds the corresponding delta
 * and sets its state to status.
 */
{
        struct  hshentry *target;

	if (expandsym(rev, &numrev)) {
	    target = genrevs(numrev.string, (char*)nil, (char*)nil,
			     (char*)nil, &gendeltas);
            if ( target )
	       if (!(countnumflds(numrev.string)&1) && cmpnum(target->num,numrev.string))
		    error("can't set state of nonexisting revision %s to %s",
			  numrev.string, status);
               else
                    target->state = status;
        }
}





	static int
buildeltatext(deltas)
	struct hshentries const *deltas;
/*   Function:  put the delta text on frewrite and make necessary   */
/*              change to delta text                                */
{
	register FILE *fcut;	/* temporary file to rebuild delta tree */
	char const *cutfilename, *diffilename;

	cutfilename = nil;
	cuttail->selector = false;
	scanlogtext(deltas->first, false);
        if ( cuthead )  {
	    cutfilename = maketemp(3);
	    if (!(fcut = fopen(cutfilename, FOPEN_W_WORK))) {
		efaterror(cutfilename);
            }

	    while (deltas->first != cuthead) {
		deltas = deltas->rest;
		scanlogtext(deltas->first, true);
            }

	    snapshotedit(fcut);
	    Ofclose(fcut);
        }

	while (deltas->first != cuttail)
	    scanlogtext((deltas = deltas->rest)->first, true);
	finishedit((struct hshentry *)nil, (FILE*)0, true);
	Ozclose(&fcopy);

        if ( cuthead ) {
	    diffilename = maketemp(0);
	    switch (run((char*)nil,diffilename,
			DIFF DIFF_FLAGS, cutfilename, resultfile, (char*)nil
	    )) {
		case DIFF_FAILURE: case DIFF_SUCCESS: break;
		default: faterror ("diff failed");
	    }
	    return putdtext(cuttail->num,cuttail->log,diffilename,frewrite,true);
	} else
	    return putdtext(cuttail->num,cuttail->log,resultfile,frewrite,false);
}



	static void
buildtree()
/*   Function:  actually removes revisions whose selector field  */
/*		is false, and rebuilds the linkage of deltas.	 */
/*              asks for reconfirmation if deleting last revision*/
{
	struct	hshentry   * Delta;
        struct  branchhead      *pt, *pre;

        if ( cuthead )
           if ( cuthead->next == delstrt )
                cuthead->next = cuttail;
           else {
                pre = pt = cuthead->branches;
                while( pt && pt->hsh != delstrt )  {
                    pre = pt;
                    pt = pt->nextbranch;
                }
                if ( cuttail )
                    pt->hsh = cuttail;
                else if ( pt == pre )
                    cuthead->branches = pt->nextbranch;
                else
                    pre->nextbranch = pt->nextbranch;
            }
	else {
            if ( cuttail == nil && !quietflag) {
		if (!yesorno(false, "Do you really want to delete all revisions? [ny](n): ")) {
		    error("No revision deleted");
		    Delta = delstrt;
		    while( Delta) {
			Delta->selector = true;
			Delta = Delta->next;
		    }
		    return;
		}
	    }
            Head = cuttail;
	}
        return;
}

#if lint
/* This lets us lint everything all at once. */

char const cmdid[] = "";

#define go(p,e) {int p P((int,char**)); void e P((void)); if(*argv)return p(argc,argv);if(*argv[1])e();}

	int
main(argc, argv)
	int argc;
	char **argv;
{
	go(ciId,	ciExit);
	go(coId,	coExit);
	go(identId,	identExit);
	go(mergeId,	mergeExit);
	go(rcsId,	exiterr);
	go(rcscleanId,	rcscleanExit);
	go(rcsdiffId,	rdiffExit);
	go(rcsmergeId,	rmergeExit);
	go(rlogId,	rlogExit);
	return 0;
}
#endif
