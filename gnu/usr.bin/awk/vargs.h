
/********************************************
vargs.h
copyright 1992 Michael D. Brennan

This is a source file for mawk, an implementation of
the AWK programming language.

Mawk is distributed without warranty under the terms of
the GNU General Public License, version 2, 1991.
********************************************/

/*
$Log: vargs.h,v $
Revision 1.1  1993/07/02 23:58:01  jtc
Updated to mawk 1.1.4

 * Revision 1.1  1992/10/02  23:23:41  mike
 * Initial revision
 *
*/

/* provides common interface to <stdarg.h> or <varargs.h> 
   only used for error messages
*/


#if   HAVE_STDARG_H == 0
#include <varargs.h>

#ifndef  VA_ALIST

#define  VA_ALIST(type, arg)  (va_alist) va_dcl { type arg ;
#define  VA_ALIST2(t1,a1,t2,a2) (va_alist) va_dcl { t1 a1 ; t2 a2 ;

#endif

#define  VA_START(p,type, last)  va_start(p) ;\
                                 last = va_arg(p,type)


#define  VA_START2(p,t1,a1,t2,a2)  va_start(p) ;\
                                  a1 = va_arg(p,t1);\
                                  a2 = va_arg(p,t2)

#else  /* HAVE_STDARG_H  */
#include <stdarg.h>

#ifndef  VA_ALIST
#define  VA_ALIST(type, arg)  (type arg, ...) {
#define  VA_ALIST2(t1,a1,t2,a2)  (t1 a1,t2 a2,...) {
#endif

#define  VA_START(p,type,last)   va_start(p,last)

#define  VA_START2(p,t1,a1,t2,a2)  va_start(p,a2)

#endif

