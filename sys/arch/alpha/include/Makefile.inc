# $NetBSD: Makefile.inc,v 1.1.24.1 2001/10/11 00:01:41 fvdl Exp $

.if !defined(_ALPHA_MAKEFILE_INC)
_ALPHA_MAKEFILE_INC=	yes

CFLAGS += -mno-fp-regs

.endif
