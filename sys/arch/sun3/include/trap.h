/*
 *	$Id: trap.h,v 1.4 1994/01/08 19:08:56 cgd Exp $
 */

/* Just use the common m68k definition */
#include <m68k/trap.h>

/* XXX - need documentation.  probably should be comment, for compat code */
#define	T_BRKPT		T_TRAP15
#define	T_WATCHPOINT	16
