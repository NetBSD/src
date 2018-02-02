#	$NetBSD: Makefile,v 1.16 2018/02/02 01:02:39 mrg Exp $

PROG=		expr
SRCS=		expr.y

.include <bsd.prog.mk>

# XXXGCC5
.if defined(HAVE_GCC) && ${HAVE_GCC} >= 5 && ${ACTIVE_CC} == "gcc"
CFLAGS+= -fwrapv
.endif
