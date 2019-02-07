#	$NetBSD: Makefile,v 1.2 2019/02/07 06:01:04 mrg Exp $

.if ${MACHINE} == "x68k"
PROG=	memswitch
SRCS=	memswitch.c properties.c methods.c
.endif

MAN=	memswitch.8
MANSUBDIR=	/x68k

.include <bsd.own.mk>

# XXX Not sure how to fix this problem
.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.memswitch.c+=	-Wno-error=stringop-overflow
.endif

.include <bsd.prog.mk>
