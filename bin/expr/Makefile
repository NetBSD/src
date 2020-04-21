#	$NetBSD: Makefile,v 1.16.4.2 2020/04/21 19:37:32 martin Exp $

PROG=		expr
SRCS=		expr.y

.include <bsd.prog.mk>

# XXXGCC5
.if defined(HAVE_GCC) && ${HAVE_GCC} >= 5 && ${ACTIVE_CC} == "gcc"
CFLAGS+= -fwrapv
.endif
