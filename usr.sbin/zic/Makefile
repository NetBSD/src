#	$NetBSD: Makefile,v 1.17 2016/10/07 15:30:01 christos Exp $

.include "Makefile.inc"
.include <bsd.own.mk>

COPTS.zic.c	+= -Wno-format-nonliteral

PROG=	zic
SRCS=	zic.c
MAN=	zic.8
TZVERSION!= ${TOOL_SED} -ne 's/^VERSION=[ 	]*//p' ${NETBSDSRCDIR}/lib/libc/time/Makefile
CPPFLAGS+=-DPKGVERSION='"(tzcode) "' -DTZVERSION='"${TZVERSION}"'

.PATH: ${NETBSDSRCDIR}/lib/libc/time

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
