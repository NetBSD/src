/*
 *	$Id: trap.h,v 1.5 1994/02/04 08:20:09 glass Exp $
 */

/* Just use the common m68k definition */
#include <m68k/trap.h>

/* XXX - need documentation.  probably should be comment, for compat code */
#define	T_BRKPT		T_TRAP15
#define	T_WATCHPOINT	16
