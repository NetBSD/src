
/********************************************
repl.h
copyright 1991, Michael D. Brennan

This is a source file for mawk, an implementation of
the AWK programming language.

Mawk is distributed without warranty under the terms of
the GNU General Public License, version 2, 1991.
********************************************/

/*$Log: repl.h,v $
/*Revision 1.2  1993/07/02 23:57:52  jtc
/*Updated to mawk 1.1.4
/*
 * Revision 5.1  1991/12/05  07:59:32  brennan
 * 1.1 pre-release
 *
*/

/* repl.h */

#ifndef  REPL_H
#define  REPL_H

PTR  PROTO( re_compile, (STRING *) ) ;
char *PROTO( re_uncompile, (PTR) ) ;


CELL *PROTO( repl_compile, (STRING *) ) ;
char *PROTO( repl_uncompile, (CELL *) ) ;
void  PROTO( repl_destroy, (CELL *) ) ;
CELL *PROTO( replv_cpy, (CELL *, CELL *) ) ;
CELL *PROTO( replv_to_repl, (CELL *, STRING *) ) ;

#endif
