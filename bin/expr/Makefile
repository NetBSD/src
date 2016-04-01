#	$NetBSD: Makefile,v 1.15 2016/04/01 08:19:31 mrg Exp $

PROG=		expr
SRCS=		expr.y

.include <bsd.prog.mk>

# XXXGCC5
.if defined(HAVE_GCC) && ${HAVE_GCC} == 53 && ${ACTIVE_CC} == "gcc"
CFLAGS+= -fwrapv
.endif

