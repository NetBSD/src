#	$NetBSD: Makefile,v 1.14.8.1 2015/01/25 09:11:02 martin Exp $

.include "Makefile.inc"
.include <bsd.own.mk>

COPTS.zic.c	+= -Wno-format-nonliteral
COPTS.scheck.c	+= -Wno-format-nonliteral

PROG=	zic
SRCS=	zic.c scheck.c ialloc.c
MAN=	zic.8
TZVERSION!= ${TOOL_SED} -ne 's/VERSION=[ 	]*//p' ${NETBSDSRCDIR}/lib/libc/time/Makefile
CPPFLAGS+=-DPKGVERSION='"(tzcode) "' -DTZVERSION='"${TZVERSION}"'

.PATH: ${NETBSDSRCDIR}/lib/libc/time

.include <bsd.prog.mk>
