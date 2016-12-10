/* $NetBSD: exect.S,v 1.2 2016/12/10 21:19:15 scole Exp $ */
	
/* XXX:	 Stub */

#include <machine/asm.h>

#include "SYS.h"

ENTRY(exect, 3)
	// XXX break into debugger in ski
	break.i 1
END(exect)	
