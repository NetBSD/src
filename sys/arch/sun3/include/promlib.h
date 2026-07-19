/*	$NetBSD: promlib.h,v 1.1 2026/07/19 01:02:59 thorpej Exp $	*/

/*
 * Wrappers around the Sun3 monitor functions to that they may be
 * spelled and pronounced the same on Sun2 (which uses naming aligned
 * with SPARC) and Sun3.
 *
 * Written by Jason R. Thorpe, July 2026.
 * Public domain.
 */

#ifndef _SUN3_PROMLIB_H_
#define	_SUN3_PROMLIB_H_

#include <machine/mon.h>

#define	prom_abort	sunmon_abort
#define	prom_printf	mon_printf

#endif /* _SUN3_PROMLIB_H_ */
