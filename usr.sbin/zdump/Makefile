#	$NetBSD: Makefile,v 1.14 2022/10/29 13:53:19 christos Exp $

.include "${.PARSEDIR}/../zic/Makefile.inc"
.include <bsd.own.mk>

PROG=	zdump
SRCS=	zdump.c
MAN=	zdump.8

LDADD+= -lintl
DPADD+= ${LIBINTL}

TZVERSION!= ${TOOL_CAT} ${NETBSDSRCDIR}/lib/libc/time/version
CPPFLAGS+=-DPKGVERSION='"(tzcode) "' -DTZVERSION='"${TZVERSION}"'

COPTS.zdump.c += -Wno-format-nonliteral
CWARNFLAGS.clang+= -Wno-error=bitwise-conditional-parentheses

.PATH: ${NETBSDSRCDIR}/lib/libc/time
.include <bsd.prog.mk>
