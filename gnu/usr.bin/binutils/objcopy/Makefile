#	$NetBSD: Makefile,v 1.5 2001/08/06 19:12:05 tv Exp $

PROG=		objcopy

.include <bsd.own.mk>
.if defined(USE_NEW_TOOLCHAIN)
.include "${.CURDIR}/../common/Makefile.prog"
.else
SRCS=		objcopy.c not-strip.c
MAN=		objcopy.1

.include <bsd.prog.mk>
.endif
