/*	$NetBSD: netgroup.h,v 1.1.1.1.4.3 2003/11/27 17:54:38 cyber Exp $	*/

#ifndef netgroup_h
#define netgroup_h

/*
 * The standard is crazy.  These values "belong" to getnetgrent() and
 * shouldn't be altered by the caller.
 */
int getnetgrent __P((/* const */ char **, /* const */ char **,
		     /* const */ char **));

int getnetgrent_r __P((char **, char **, char **, char *, int));

void setnetgrent __P((const char *));

void endnetgrent __P((void));

int innetgr __P((const char *, const char *, const char *, const char *));

#endif
