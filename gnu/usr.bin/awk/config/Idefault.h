
/********************************************
Idefault.h
copyright 1991, 1992.  Michael D. Brennan

This is a source file for mawk, an implementation of
the AWK programming language.

Mawk is distributed without warranty under the terms of
the GNU General Public License, version 2, 1991.
********************************************/


/* $Log: Idefault.h,v $
/* Revision 1.2  1993/07/02 23:58:21  jtc
/* Updated to mawk 1.1.4
/*
 * Revision 3.18.1.1  1993/01/15  03:33:52  mike
 * patch3: safer double to int conversion
 *
 * Revision 3.18  1992/12/17  02:48:01  mike
 * 1.1.2d changes for DOS
 *
 * Revision 3.17  1992/11/26  15:35:52  mike
 * don't assume __STDC__ implies HAVE_STRERROR
 *
 * Revision 3.16  1992/11/22  19:00:43  mike
 * allow STDC assumptions to be overridden
 *
 * Revision 3.15  1992/07/08  16:16:08  brennan
 * don't attempt any #def or #undef with __STDC__
 *
*/


#ifdef __STDC__
#if   __STDC__  

#undef  HAVE_PROTOS
#define HAVE_PROTOS		1
#undef  HAVE_VOID_PTR
#define HAVE_VOID_PTR		1

/* these can be overidden */

#ifndef  HAVE_STDARG_H
#define HAVE_STDARG_H		1
#endif

#ifndef  HAVE_STRING_H
#define HAVE_STRING_H		1
#endif

#ifndef  HAVE_STDLIB_H
#define HAVE_STDLIB_H		1
#endif

#endif  
#endif

#ifdef	MSDOS

#ifndef  HAVE_REARGV
#define  HAVE_REARGV	0
#endif

#if HAVE_REARGV
#define  SET_PROGNAME()  reargv(&argc,&argv) ; progname = argv[0]
#else
#define  SET_PROGNAME()  progname = "mawk"
#endif

#define  MAX__INT	0x7fff

#if  HAVE_SMALL_MEMORY==0
#define  LM_DOS		1
#else
#define  LM_DOS		0
#endif

#define   SM_DOS	(!LM_DOS)

#define HAVE_REAL_PIPES		0
#define HAVE_FAKE_PIPES		1

#else /* not defined MSDOS */
#define   MSDOS		0
#define   LM_DOS	0
#define   SM_DOS	0

#endif /* MSDOS */


/* The most common configuration is defined here:
 
   no function prototypes
   have void*
   have matherr(), strtod(), fmod()
   uses <varargs.h>

   fpe_traps default to off
   and nan comparison is done correctly

   memory is not small

   OS is some flavor of Unix

*/

/* WARNING:  To port to a new configuration, don't make changes
   here.  This file is included at the end of your new 
   config.h 

   Read the file   mawk/INSTALL
*/


/*------------- compiler ----------------------------*/
/* do not have function prototypes */

#ifndef  HAVE_PROTOS
#define  HAVE_PROTOS		0
#endif

/* have type   void *    */
#ifndef  HAVE_VOID_PTR
#define  HAVE_VOID_PTR		1
#endif

/* logical test of double is OK */
#ifndef D2BOOL
#define D2BOOL(x)		(x)
#endif

/*---------------- library ----------------------*/


#ifndef  HAVE_MATHERR
#define  HAVE_MATHERR		1
#endif

#ifndef  HAVE_STRTOD
#define  HAVE_STRTOD		1
#endif

#ifndef  HAVE_FMOD
#define  HAVE_FMOD		1
#endif

#ifndef  HAVE_STRERROR
#define  HAVE_STRERROR		0
#endif

/* uses <varargs.h> instead of <stdarg.h> */
#ifndef  HAVE_STDARG_H	
#define  HAVE_STDARG_H		0
#endif

/* has <string.h>, 
   doesn't have <stdlib.h>  
   has <fcntl.h>
*/

#ifndef  HAVE_STRING_H
#define  HAVE_STRING_H		1
#endif

#ifndef  HAVE_STDLIB_H
#define  HAVE_STDLIB_H		0
#endif

#ifndef  HAVE_FCNTL_H
#define  HAVE_FCNTL_H		1
#endif

/* have pipes */
#ifndef  HAVE_REAL_PIPES
#define  HAVE_REAL_PIPES	1
#endif

#ifndef  HAVE_FAKE_PIPES	
#define  HAVE_FAKE_PIPES	0
#endif

/* don't have strerror() */
#ifndef  HAVE_STRERROR
#define  HAVE_STRERROR		0
#endif

#ifndef  SET_PROGNAME
#define  SET_PROGNAME()		{ char *strrchr() , *p ;\
                                  p = strrchr(argv[0],'/') ;\
				  progname = p ? p+1 : argv[0] ; }
#endif
				  
				  

/*------------- machine ------------------------*/

/* ints are 32bits, two complement */
#ifndef  MAX__INT     
#define  MAX__INT	0x7fffffff
#define  INT_FMT	"%d"
#endif

#ifndef  MAX__LONG
#define  MAX__LONG	0x7fffffff
#endif

#if  MAX__INT <= 0x7fff
#define  SHORT_INTS
#define  INT_FMT	"%ld"
#endif


/* default is IEEE754 and data space is not scarce */

#ifndef  FPE_TRAPS_ON
#define  FPE_TRAPS_ON		0
#endif

#ifndef   NOINFO_SIGFPE
#define   NOINFO_SIGFPE		0
#endif

#if   ! FPE_TRAPS_ON
#undef   NOINFO_SIGFPE  
#define  NOINFO_SIGFPE          0 /* make sure no one does
				     something stupid */
#endif


#if      NOINFO_SIGFPE
#define  CHECK_DIVZERO(x)	if( (x) == 0.0 )rt_error(dz_msg);else
#endif

/* SW_FP_CHECK is specific to V7 and XNX23A
	(1) is part of STDC_MATHERR def.
	(2) enables calls to XENIX-68K 2.3A clrerr(), iserr()
 */
#ifndef  SW_FP_CHECK
#define  SW_FP_CHECK		0
#endif

#ifndef  TURN_OFF_FPE_TRAPS
#define  TURN_OFF_FPE_TRAPS()	/* nothing */
#endif

#ifndef  TURN_ON_FPE_TRAPS
#define  TURN_ON_FPE_TRAPS()	/* nothing */
#endif

#ifndef  HAVE_SMALL_MEMORY
#define  HAVE_SMALL_MEMORY	0
#endif


/*------------------------------------------------*/




/* the painfull case: we need to catch fpe's and look at errno
   after lib calls */

#define  STDC_MATHERR	((SW_FP_CHECK || FPE_TRAPS_ON) && HAVE_MATHERR==0)



#if  HAVE_PROTOS
#define  PROTO(name, args)  name  args
#else
#define  PROTO(name, args)  name()
#endif


/* for Think C on the Macintosh, sizeof(size_t) != sizeof(unsigned
 * Rather than unilaterally imposing size_t, when not all compilers would
 * necessarily have it defined, we use the SIZE_T() macro where appropriate
 * to typecast function arguments
 */
#ifndef SIZE_T
#define SIZE_T(x) (x)
#endif
