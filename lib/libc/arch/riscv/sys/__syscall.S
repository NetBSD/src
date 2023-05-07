/*	$NetBSD: __syscall.S,v 1.4 2023/05/07 12:41:47 skrll Exp $	*/

#include "SYS.h"

ENTRY(__syscall)
	SYSTRAP(__syscall)
	JUMP_TO_CERROR()	/* error */
	ret			/* success */
END(__syscall)
STRONG_ALIAS(_syscall,__syscall)
WEAK_ALIAS(syscall,__syscall)
