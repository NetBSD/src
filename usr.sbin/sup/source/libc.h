/*	$NetBSD: libc.h,v 1.8 2009/01/15 15:58:42 christos Exp $	*/

/*
 * Copyright (c) 1991 Carnegie Mellon University
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
 *  Software Distribution Coordinator   or   Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the rights
 * to redistribute these changes.
 */
/*
 **********************************************************************
 * HISTORY

 * Revision 1.7  89/04/03  11:10:45  vanryzin
 * 	Changed definition of qsort for c++ to indicate the procedure
 * 	passed to qsort has parameters.  Since we were unsure if ANSI C
 * 	could handle the syntax I placed the new definition within #if
 * 	defined(c_plusplus) conditionals.  This may not be necessary
 * 	and perhaps should be fixed at a later time.
 * 	[89/04/03            vanryzin]
 * 
 * Revision 1.6  89/02/05  15:55:57  gm0w
 * 	Added extern char *errmsg().
 * 	[89/02/04            gm0w]
 * 
 * Revision 1.5  89/01/20  15:34:40  gm0w
 * 	Moved all of the STDC changes to other existing include files
 * 	back into this one.  Added non-STDC extern declarations for
 * 	all functions without int return values to match those defined
 * 	by STDC.  Added include of sysent.h.  Removed obsolete cdate
 * 	extern declaration.
 * 	[88/12/17            gm0w]
 * 
 * Revision 1.4  88/12/22  16:58:56  mja
 * 	Correct __STDC__ parameter type for getenv().
 * 	[88/12/20            dld]
 * 
 * Revision 1.3  88/12/14  23:31:42  mja
 * 	Made file reentrant.  Added declarations for __STDC__.
 * 	[88/01/06            jjk]
 * 
 * 30-Apr-88  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added pathof() extern.
 *
 * 01-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added getname() extern.
 *
 * 29-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added lseek() extern.
 *
 * 02-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added estrdup() extern.
 *
 * 14-Aug-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */

#ifndef	_LIBC_H_
#define	_LIBC_H_ 1

#ifndef _TYPES_
#include <sys/types.h>
#endif	/* _TYPES_ */

#ifndef	FILE
#include <stdio.h>
#endif	/* FILE */

#ifndef	_STRINGS_H_
#include <strings.h>
#endif	/* _STRINGS_H_ */

#ifndef	_STRING_H_
#include <string.h>
#endif	/* _STRINGS_H_ */

#ifndef	_TIME_H_
#include <time.h>
#endif	/* _TIME_H_ */

/*  CMU stdio additions */
extern FILE *fopenp(const char*, const char*, char*, char*);
extern FILE *fwantread(const char*, const char*, const char*, const char*);
extern FILE *fwantwrite(const char*, const char*, const char*, const char*,
			int);

/* CMU string routines */
extern char* foldup(char*, const char*);
extern char* folddown(char*, const char*);
extern char* sindex(const char*, const char*);
extern char _argbreak;
extern char* getstr(const char*, char*, char*);
extern int getstab(const char*, const char**, const char*);
extern int getsearch(const char*, const char**, const char*);
extern char* strarg(const char**, const char*, const char*, char*, char*);
extern int stabarg(const char**, const char*, const char*, const char**,
		   const char*);
extern int searcharg(const char**, const char*, const char*, const char**,
		     const char*);
extern int getint(const char*, int, int, int);
extern int intarg(const char**, const char*, const char*, int, int, int);
extern long getlong(const char*, long, long, long);
extern long longarg(const char**, const char*, const char*, long, long, long);
extern short getshort(const char*, short, short, short);
extern short shortarg(const char**, const char*, const char*,
		      short, short, short);
extern float getfloat(const char*, float, float, float);
extern float floatarg(const char**, const char*, const char*,
		      float, float, float);
extern double getdouble(const char*, double, double, double);
extern double doublearg(const char**, const char*, const char*,
			double, double, double);
extern unsigned int getoct(const char*, unsigned int, unsigned int,
			   unsigned int);
extern unsigned int octarg(const char**, const char*, const char*,
			   unsigned int, unsigned int, unsigned int);
extern unsigned int gethex(const char*, unsigned int, unsigned int,
			   unsigned int);
extern unsigned int hexarg(const char**, const char*, const char*,
			   unsigned int, unsigned int, unsigned int);
extern unsigned int atoh(const char*);
extern char *concat(const char*, int, ...);

/* CMU library routines */
extern char *getname(int);
extern char *pathof(char *);

/*  CMU time additions */
extern long gtime(const struct tm*);
extern long atot(const char*);

/* 4.3 BSD standard library routines; taken from man(3) */
#if defined(c_plusplus)
typedef int (*PFI2)(...);
#endif /* c_plusplus */
#endif	/* not _LIBC_H_ */
