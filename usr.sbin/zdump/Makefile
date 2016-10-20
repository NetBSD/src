#	$NetBSD: Makefile,v 1.12 2016/10/20 17:39:08 christos Exp $

.include "${.PARSEDIR}/../zic/Makefile.inc"
.include <bsd.own.mk>

PROG=	zdump
SRCS=	zdump.c
MAN=	zdump.8

TZVERSION!= ${TOOL_CAT} ${NETBSDSRCDIR}/lib/libc/time/version
CPPFLAGS+=-DPKGVERSION='"(tzcode) "' -DTZVERSION='"${TZVERSION}"'

COPTS.zdump.c += -Wno-format-nonliteral

.PATH: ${NETBSDSRCDIR}/lib/libc/time
.include <bsd.prog.mk>
