# $NetBSD: _resumecontext.S,v 1.1 2004/03/01 19:26:29 drochner Exp $
	
#include <machine/asm.h>

LEAF_NOPROFILE(__resumecontext, 0)
	LDGP(ra)
	CALL(_resumecontext)
