#	$NetBSD: Makefile,v 1.5 2001/08/06 19:12:06 tv Exp $

PROG=		strings

.include <bsd.own.mk>
.if defined(USE_NEW_TOOLCHAIN)
.include "${.CURDIR}/../common/Makefile.prog"
.else
SRCS=		strings.c
MAN=		strings.1

.include <bsd.prog.mk>
.endif
