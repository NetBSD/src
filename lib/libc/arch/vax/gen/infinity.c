/*	$NetBSD: infinity.c,v 1.2 1997/07/13 18:45:46 christos Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char rcsid[] = "$Id: infinity.c,v 1.2 1997/07/13 18:45:46 christos Exp $";
#else
__RCSID("$NetBSD: infinity.c,v 1.2 1997/07/13 18:45:46 christos Exp $");
#endif
#endif /* not lint */
/*
 * XXX - THIS IS (probably) COMPLETELY WRONG ON VAX!!!
 */

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on a 387 */
char __infinity[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
