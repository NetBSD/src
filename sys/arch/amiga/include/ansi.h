/*
 *	$Id: ansi.h,v 1.4 1994/01/26 21:12:12 mw Exp $
 */

#ifndef _MACHINE_ANSI_H
#define _MACHINE_ANSI_H

/* Just use the common m68k definition */
#include <m68k/ansi.h>

#undef _SIZE_T_
#define _SIZE_T_ unsigned long			/* sizeof() */

#endif /* _MACHINE_ANSI_H */
