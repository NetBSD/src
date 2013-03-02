#	$NetBSD: Makefile,v 1.8 2013/03/02 21:39:14 christos Exp $

.include "${.PARSEDIR}/../zic/Makefile.inc"
.include <bsd.own.mk>

PROG=	zdump
SRCS=	zdump.c ialloc.c
MAN=	zdump.8

COPTS.zdump.c += -Wno-format-nonliteral

.PATH: ${NETBSDSRCDIR}/lib/libc/time
.include <bsd.prog.mk>
