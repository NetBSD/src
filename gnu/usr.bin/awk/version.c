
/********************************************
version.c
copyright 1991, 1992.  Michael D. Brennan

This is a source file for mawk, an implementation of
the AWK programming language.

Mawk is distributed without warranty under the terms of
the GNU General Public License, version 2, 1991.
********************************************/

/*$Log: version.c,v $
/*Revision 1.2  1993/07/02 23:58:02  jtc
/*Updated to mawk 1.1.4
/*
 * Revision 5.6.1.3  1993/05/05  00:02:18  mike
 * patch4
 *
 * Revision 5.6.1.2  1993/01/20  12:53:13  mike
 * d_to_l()
 *
 * Revision 5.6.1.1  1993/01/15  03:33:54  mike
 * patch3: safer double to int conversion
 *
 * Revision 5.6  1992/12/17  02:48:01  mike
 * 1.1.2d changes for DOS
 *
 * Revision 5.5  1992/12/02  03:18:12  mike
 * coherent patch
 *
 * Revision 5.4  1992/08/27  11:50:38  mike
 * patch2
 *
 * Revision 5.3  1992/03/03  16:42:23  brennan
 * patch 1
 *
 * Revision 5.2  92/01/22  05:34:10  brennan
 * version 1.1
 * 
 * Revision 5.1  91/12/05  07:56:33  brennan
 * 1.1 pre-release
 * 
*/

#include "mawk.h"
#include "patchlev.h"

static char rcsid[] =
"@(#) $Id: version.c,v 1.2 1993/07/02 23:58:02 jtc Exp $" ;

#define  VERSION_STRING  \
  "mawk 1.1%s%s %s, Copyright (C) Michael D. Brennan\n\n"

#define  DOS_STRING     ""

/* If use different command line syntax for MSDOS
   mark that in VERSION  */

#if  MSDOS 
#undef   DOS_STRING

#if  SM_DOS

#if  HAVE_REARGV
#define  DOS_STRING     "SM"
#else
#define  DOS_STRING     "SMDOS"
#endif  

#else  /* LM_DOS  */

#if  HAVE_REARGV
#define  DOS_STRING     "LM"
#else
#define  DOS_STRING     "LMDOS"
#endif  

#endif
#endif  /* MSDOS */

#ifdef THINK_C
#undef DOS_STRING
#define DOS_STRING ":Mac"
#endif

static char fmt[] = "%-14s%10lu\n" ;

/* print VERSION and exit */
void print_version()
{ 

  printf(VERSION_STRING, PATCH_STRING, DOS_STRING, DATE_STRING) ;
  fflush(stdout) ;

  print_compiler_id() ;
  fprintf(stderr, "compiled limits:\n") ;
  fprintf(stderr, fmt,  "largest field", (long)MAX_FIELD) ;
  fprintf(stderr, fmt,  "sprintf buffer", (long)SPRINTF_SZ) ;
  print_aux_limits() ;
  exit(0) ;
}


/*
  Extra info for MSDOS.  This code contributed by
  Ben Myers 
*/

#ifdef __TURBOC__
#include <alloc.h>   /* coreleft() */
#define  BORL
#endif

#ifdef __BORLANDC__
#include <alloc.h>   /* coreleft() */
#define  BORL
#endif

#ifdef  BORL 
#if     LM_DOS
extern unsigned  _stklen = 16 * 1024U ; 
   /*  4K of stack is enough for a user function call 
       nesting depth of 75 so this is enough for 300 */
#endif
#endif

#ifdef _MSC_VER
#include <malloc.h>
#endif

#ifdef __ZTC__
#include <dos.h>              /* _chkstack */
#endif


int print_compiler_id()
{

#ifdef  __TURBOC__
  fprintf(stderr, "MsDOS Turbo C++ %d.%d\n",
                __TURBOC__>>8, __TURBOC__&0xff) ;
#endif
  
#ifdef __BORLANDC__
  fprintf (stderr, "MS-DOS Borland C++ __BORLANDC__ %x\n",
        __BORLANDC__ );
#endif

#ifdef _MSC_VER
  fprintf (stderr, "MS-DOS Microsoft C/C++ _MSC_VER %u\n", _MSC_VER );
#endif

#ifdef __ZTC__
  fprintf (stderr, "MS-DOS Zortech C++ __ZTC__ %x\n", __ZTC__ );
#endif

  return 0 ; /*shut up */
}


int  print_aux_limits()
{
#ifdef BORL
  extern unsigned _stklen ;
  fprintf(stderr, fmt,  "stack size", (unsigned long)_stklen) ;
  fprintf(stderr, fmt,  "heap size",  (unsigned long) coreleft()) ;
#endif

#ifdef _MSC_VER
  fprintf(stderr, fmt,  "stack size", (unsigned long)_stackavail()) ;
#if   SM_DOS
  fprintf(stderr, fmt,  "heap size", (unsigned long) _memavl()) ;
#endif
#endif

#ifdef __ZTC__
/* large memory model only with ztc */
  fprintf(stderr, fmt,  "stack size??", (unsigned long)_chkstack()) ;
  fprintf(stderr, fmt,  "heap size", farcoreleft()) ;
#endif

  return 0 ;
}
