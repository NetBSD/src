#	$NetBSD: Makefile,v 1.11 2016/10/07 15:30:01 christos Exp $

.include "${.PARSEDIR}/../zic/Makefile.inc"
.include <bsd.own.mk>

PROG=	zdump
SRCS=	zdump.c
MAN=	zdump.8

TZVERSION!= ${TOOL_SED} -ne 's/^VERSION=[ 	]*//p' ${NETBSDSRCDIR}/lib/libc/time/Makefile
CPPFLAGS+=-DPKGVERSION='"(tzcode) "' -DTZVERSION='"${TZVERSION}"'

COPTS.zdump.c += -Wno-format-nonliteral

.PATH: ${NETBSDSRCDIR}/lib/libc/time
.include <bsd.prog.mk>
