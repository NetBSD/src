/********************************************
386bsd.h
copyright 1993, Michael D. Brennan

This is a source file for mawk, an implementation of
the AWK programming language.

Mawk is distributed without warranty under the terms of
the GNU General Public License, version 2, 1991.
********************************************/


/*
 * $Log: config.h,v $
 * Revision 1.3  1993/07/02 23:57:10  jtc
 * Updated to mawk 1.1.4
 *
 * Revision 1.1  1993/02/05  02:20:10  mike
 * Initial revision
 *
 */

#ifndef   CONFIG_H
#define   CONFIG_H    1

#define   FPE_TRAPS_ON                1
#define   FPE_ZERODIVIDE   FPE_FLTDIV_TRAP
#define   FPE_OVERFLOW     FPE_FLTOVF_TRAP

#define   HAVE_STRTOD		1

#define   HAVE_MATHERR		0

#define   DONT_PROTO_OPEN

#include "config/Idefault.h"

#endif  /* CONFIG_H  */
