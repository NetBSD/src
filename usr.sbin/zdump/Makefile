#	$NetBSD: Makefile,v 1.7.2.1 2014/05/22 11:43:11 yamt Exp $

.include "${.PARSEDIR}/../zic/Makefile.inc"
.include <bsd.own.mk>

PROG=	zdump
SRCS=	zdump.c ialloc.c
MAN=	zdump.8

COPTS.zdump.c += -Wno-format-nonliteral

.PATH: ${NETBSDSRCDIR}/lib/libc/time
.include <bsd.prog.mk>
