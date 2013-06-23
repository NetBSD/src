#	$NetBSD: Makefile,v 1.7.8.1 2013/06/23 06:29:06 tls Exp $

.include "${.PARSEDIR}/../zic/Makefile.inc"
.include <bsd.own.mk>

PROG=	zdump
SRCS=	zdump.c ialloc.c
MAN=	zdump.8

COPTS.zdump.c += -Wno-format-nonliteral

.PATH: ${NETBSDSRCDIR}/lib/libc/time
.include <bsd.prog.mk>
