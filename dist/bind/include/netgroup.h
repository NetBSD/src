/*	$NetBSD: netgroup.h,v 1.1.1.1.2.3 2003/10/27 04:40:21 jmc Exp $	*/

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
