#	$NetBSD: Makefile,v 1.5 2001/08/06 19:12:00 tv Exp $

PROG=		addr2line

.include <bsd.own.mk>
.if defined(USE_NEW_TOOLCHAIN)
.include "${.CURDIR}/../common/Makefile.prog"
.else
SRCS=		addr2line.c
MAN=		addr2line.1

.include <bsd.prog.mk>
.endif
