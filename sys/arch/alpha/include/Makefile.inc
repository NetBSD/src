# $NetBSD: Makefile.inc,v 1.1.20.1 2001/10/22 20:41:10 nathanw Exp $

.if !defined(_ALPHA_MAKEFILE_INC)
_ALPHA_MAKEFILE_INC=	yes

CFLAGS += -mno-fp-regs

.endif
