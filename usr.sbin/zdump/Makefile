#	$NetBSD: Makefile,v 1.13 2019/11/11 22:45:32 joerg Exp $

.include "${.PARSEDIR}/../zic/Makefile.inc"
.include <bsd.own.mk>

PROG=	zdump
SRCS=	zdump.c
MAN=	zdump.8

TZVERSION!= ${TOOL_CAT} ${NETBSDSRCDIR}/lib/libc/time/version
CPPFLAGS+=-DPKGVERSION='"(tzcode) "' -DTZVERSION='"${TZVERSION}"'

COPTS.zdump.c += -Wno-format-nonliteral
CWARNFLAGS.clang+= -Wno-error=bitwise-conditional-parentheses

.PATH: ${NETBSDSRCDIR}/lib/libc/time
.include <bsd.prog.mk>
