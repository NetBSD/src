/* $NetBSD: compat_Ovfork.S,v 1.1 2016/06/30 09:14:30 mrg Exp $ */

#include "SYS.h"

/* XXX should not be necessary or useful, no NetBSD/ia64 1.4 */
SYSCALL(vfork)
	/* XXX: watchout for return values for child/parent */ 
	RET
END(vfork)	
