/*	$NetBSD: trap.h,v 1.6 1994/10/26 09:11:22 cgd Exp $	*/

/* Just use the common m68k definition */
#include <m68k/trap.h>

/* XXX - need documentation.  probably should be comment, for compat code */
#define	T_BRKPT		T_TRAP15
#define	T_WATCHPOINT	16
