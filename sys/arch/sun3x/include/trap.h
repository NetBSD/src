/*	$NetBSD: trap.h,v 1.1.1.1 1997/01/14 20:57:07 gwr Exp $	*/

#include <m68k/trap.h>

/*
 * XXX - need documentation.
 * probably should be comment,
 * for compat code
 */
#define	T_BRKPT		T_TRAP15
#define	T_WATCHPOINT	16
