# $NetBSD: Makefile.inc,v 1.1.22.1 2002/01/10 19:37:04 thorpej Exp $

.if !defined(_ALPHA_MAKEFILE_INC)
_ALPHA_MAKEFILE_INC=	yes

CFLAGS += -mno-fp-regs

.endif
