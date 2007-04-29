/*	$NetBSD: supcmisc.c,v 1.16 2007/04/29 20:23:37 msaitoh Exp $	*/

/*
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * sup misc. routines, include list processing.
 **********************************************************************
 * HISTORY
 * Revision 1.5  92/08/11  12:07:22  mrt
 * 	Added release to FILEWHEN name.
 * 	Brad's changes: delinted and updated variable argument usage.
 * 	[92/07/26            mrt]
 *
 * Revision 1.3  89/08/15  15:31:28  bww
 * 	Updated to use v*printf() in place of _doprnt().
 * 	From "[89/04/19            mja]" at CMU.
 * 	[89/08/15            bww]
 *
 * 27-Dec-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fixed bug in ugconvert() which left pw uninitialized.
 *
 * 25-May-87  Doug Philips (dwp) at Carnegie-Mellon University
 *	Split off from sup.c and changed goaway to use printf
 *	instead of notify if thisC is NULL.
 *
 **********************************************************************
 */

#include "supcdefs.h"
#include "supextern.h"

struct liststruct {		/* uid and gid lists */
	char *Lname;		/* name */
	int Lnumber;		/* uid or gid */
	struct liststruct *Lnext;
};
typedef struct liststruct LIST;

#define HASHBITS	4
#define HASHSIZE	(1<<HASHBITS)
#define HASHMASK	(HASHSIZE-1)
#define LISTSIZE	(HASHSIZE*HASHSIZE)

static LIST *uidL[LISTSIZE];	/* uid and gid lists */
static LIST *gidL[LISTSIZE];

extern COLLECTION *thisC;	/* collection list pointer */

static int Lhash(char *);
static void Linsert(LIST **, char *, int);
static LIST *Llookup(LIST **, char *);

/*************************************************
 ***    P R I N T   U P D A T E   T I M E S    ***
 *************************************************/

void
prtime(void)
{
	char buf[STRINGLENGTH];
	char relsufix[STRINGLENGTH];
	time_t twhen;

	if ((thisC->Cflags & CFURELSUF) && thisC->Crelease)
		(void) sprintf(relsufix, ".%s", thisC->Crelease);
	else
		relsufix[0] = '\0';
	if (chdir(thisC->Cbase) < 0)
		logerr("Can't change to base directory %s for collection %s",
		    thisC->Cbase, thisC->Cname);
	twhen = getwhen(thisC->Cname, relsufix);
	(void) strcpy(buf, ctime(&twhen));
	buf[strlen(buf) - 1] = '\0';
	loginfo("Last update occurred at %s for collection %s%s",
	    buf, thisC->Cname, relsufix);
}

int 
establishdir(char *fname)
{
	char dpart[STRINGLENGTH], fpart[STRINGLENGTH];
	path(fname, dpart, fpart);
	return (estabd(fname, dpart));
}

int 
makedir(char *fname, unsigned int mode, struct stat * statp)
{
	if (lstat(fname, statp) != -1 && !S_ISDIR(statp->st_mode)) {
		if (unlink(fname) == -1) {
			notify("SUP: Can't delete %s\n", fname);
			return -1;
		}
	}
	(void) mkdir(fname, mode);

	return stat(fname, statp);
}

int 
estabd(char *fname, char *dname)
{
	char dpart[STRINGLENGTH], fpart[STRINGLENGTH];
	struct stat sbuf;
	int x;

	if (stat(dname, &sbuf) >= 0)
		return (FALSE);	/* exists */
	path(dname, dpart, fpart);
	if (strcmp(fpart, ".") == 0) {	/* dname is / or . */
		notify("SUP: Can't create directory %s for %s\n", dname, fname);
		return (TRUE);
	}
	x = estabd(fname, dpart);
	if (x)
		return (TRUE);
	if (makedir(dname, 0755, &sbuf) < 0) {
		vnotify("SUP: Can't create directory %s for %s\n", dname, fname);
		return TRUE;
	}
	vnotify("SUP Created directory %s for %s\n", dname, fname);
	return (FALSE);
}
/***************************************
 ***    L I S T   R O U T I N E S    ***
 ***************************************/

static int 
Lhash(char *name)
{
	/* Hash function is:  HASHSIZE * (strlen mod HASHSIZE) +
	 * (char   mod HASHSIZE) where "char" is last character of name (if
	 * name is non-null). */

	int len;
	char c;

	len = strlen(name);
	if (len > 0)
		c = name[len - 1];
	else
		c = 0;
	return (((len & HASHMASK) << HASHBITS) | (((int) c) & HASHMASK));
}

static void
Linsert(LIST ** table, char *name, int number)
{
	LIST *l;
	int lno;
	lno = Lhash(name);
	l = (LIST *) malloc(sizeof(LIST));
	if (l == NULL)
		goaway("Cannot allocate memory");
	l->Lname = name;
	l->Lnumber = number;
	l->Lnext = table[lno];
	table[lno] = l;
}

static LIST *
Llookup(LIST ** table, char *name)
{
	int lno;
	LIST *l;
	lno = Lhash(name);
	for (l = table[lno]; l && strcmp(l->Lname, name) != 0; l = l->Lnext);
	return (l);
}

void 
ugconvert(char *uname, char *gname, int *uid, int *gid, int *mode)
{
	LIST *u, *g;
	struct passwd *pw;
	struct group *gr;
	struct stat sbuf;
	static int defuid = -1;
	static int defgid;
	static int first = TRUE;

	if (first) {
		bzero(uidL, sizeof(uidL));
		bzero(gidL, sizeof(gidL));
		first = FALSE;
	}
	pw = NULL;
	if ((u = Llookup(uidL, uname)) != NULL)
		*uid = u->Lnumber;
	else if ((pw = getpwnam(uname)) != NULL) {
		Linsert(uidL, estrdup(uname), pw->pw_uid);
		*uid = pw->pw_uid;
	}
	if (u || pw) {
		if ((g = Llookup(gidL, gname)) != NULL) {
			*gid = g->Lnumber;
			return;
		}
		if ((gr = getgrnam(gname)) != NULL) {
			Linsert(gidL, estrdup(gname), gr->gr_gid);
			*gid = gr->gr_gid;
			return;
		}
		if (pw == NULL)
			pw = getpwnam(uname);
		*mode &= ~S_ISGID;
		*gid = pw->pw_gid;
		return;
	}
	*mode &= ~(S_ISUID | S_ISGID);
	if (defuid >= 0) {
		*uid = defuid;
		*gid = defgid;
		return;
	}
	if (stat(".", &sbuf) < 0) {
		*uid = defuid = getuid();
		*gid = defgid = getgid();
		return;
	}
	*uid = defuid = sbuf.st_uid;
	*gid = defgid = sbuf.st_gid;
}


/*********************************************
 ***    U T I L I T Y   R O U T I N E S    ***
 *********************************************/

void
notify(char *fmt, ...)
{				/* record error message */
	char buf[STRINGLENGTH];
	char collrelname[STRINGLENGTH];
	time_t tloc;
	static FILE *noteF = NULL;	/* mail program on pipe */
	va_list ap;

	va_start(ap, fmt);
	if (fmt == NULL) {
		if (noteF && noteF != stdout)
			(void) pclose(noteF);
		noteF = NULL;
		va_end(ap);
		return;
	}
	if ((thisC->Cflags & CFURELSUF) && thisC->Crelease)
		(void) sprintf(collrelname, "%s-%s", collname, thisC->Crelease);
	else
		(void) strcpy(collrelname, collname);

	if (noteF == NULL) {
		if ((thisC->Cflags & CFMAIL) && thisC->Cnotify) {
			(void) sprintf(buf, "mail -s \"SUP Upgrade of %s\" %s >/dev/null",
			    collrelname, thisC->Cnotify);
			noteF = popen(buf, "w");
			if (noteF == NULL) {
				logerr("Can't send mail to %s for %s",
				    thisC->Cnotify, collrelname);
				noteF = stdout;
			}
		} else
			noteF = stdout;
		tloc = time((time_t *) NULL);
		fprintf(noteF, "SUP Upgrade of %s at %s",
		    collrelname, ctime(&tloc));
		(void) fflush(noteF);
	}
	vfprintf(noteF, fmt, ap);
	va_end(ap);
	(void) fflush(noteF);
}

void
lockout(int on)
{				/* lock out interrupts */
	static sigset_t oset;
	sigset_t nset;

	if (on) {
		sigemptyset(&nset);
		sigaddset(&nset, SIGHUP);
		sigaddset(&nset, SIGINT);
		sigaddset(&nset, SIGTERM);
		sigaddset(&nset, SIGQUIT);
		(void) sigprocmask(SIG_BLOCK, &nset, &oset);
	} else {
		(void) sigprocmask(SIG_SETMASK, &oset, NULL);
	}
}

char *
fmttime(time_t time)
{
	static char buf[STRINGLENGTH];
	unsigned int len;

	(void) strcpy(buf, ctime(&time));
	len = strlen(buf + 4) - 6;
	(void) strncpy(buf, buf + 4, len);
	buf[len] = '\0';
	return (buf);
}
