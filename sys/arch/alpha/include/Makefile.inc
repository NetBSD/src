# $NetBSD: Makefile.inc,v 1.2 2001/10/05 15:25:02 simonb Exp $

.if !defined(_ALPHA_MAKEFILE_INC)
_ALPHA_MAKEFILE_INC=	yes

CFLAGS += -mno-fp-regs

.endif
