
/********************************************
main.c
copyright 1991, Michael D. Brennan

This is a source file for mawk, an implementation of
the AWK programming language.

Mawk is distributed without warranty under the terms of
the GNU General Public License, version 2, 1991.
********************************************/

/* $Log: main.c,v $
/* Revision 1.2  1993/07/02 23:57:35  jtc
/* Updated to mawk 1.1.4
/*
 * Revision 5.2.1.1  1993/01/15  03:33:44  mike
 * patch3: safer double to int conversion
 *
 * Revision 5.2  1992/12/17  02:48:01  mike
 * 1.1.2d changes for DOS
 *
 * Revision 5.1  1991/12/05  07:56:14  brennan
 * 1.1 pre-release
 *
*/



/*  main.c  */

#include "mawk.h"
#include "code.h"
#include "init.h"
#include "fin.h"
#include "bi_vars.h"
#include "field.h"
#include "files.h"
#include <stdio.h>


short mawk_state ; /* 0 is compiling */
int  exit_code ;

int
main(argc , argv )
  int argc ; char **argv ;
{ 

  initialize(argc, argv) ;

  if ( parse() || compile_error_count )  mawk_exit(1) ;

  compile_cleanup() ;
  mawk_state = EXECUTION ;
  execute(code_ptr, eval_stack-1, 0) ;
  /* never returns */
  return 0 ;
}

void  mawk_exit(x)
  int x ;
{
#if  HAVE_REAL_PIPES
  close_out_pipes() ;  /* no effect, if no out pipes */
#else
#if  HAVE_FAKE_PIPES
  close_fake_pipes() ;
#endif
#endif

  exit(x) ;
}
