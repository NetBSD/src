/* $NetBSD: exect.S,v 1.1.66.1 2017/01/07 08:56:02 pgoyette Exp $ */
	
/* XXX:	 Stub */

#include <machine/asm.h>

#include "SYS.h"

ENTRY(exect, 3)
	// XXX break into debugger in ski
	break.i 1
END(exect)	
