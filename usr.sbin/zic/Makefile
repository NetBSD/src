#	$NetBSD: Makefile,v 1.22 2022/10/29 13:53:19 christos Exp $

.include "Makefile.inc"
.include <bsd.own.mk>

COPTS.zic.c	+= -Wno-format-nonliteral

PROG=	zic
SRCS=	zic.c
MAN=	zic.8
.if !empty(TOOL_CAT:M*/*) && !exists(${TOOL_CAT})
TZVERSION=0
.else
TZVERSION!= ${TOOL_CAT} ${NETBSDSRCDIR}/lib/libc/time/version
.endif
CPPFLAGS+=-DPKGVERSION='"(tzcode) "' -DTZVERSION='"${TZVERSION}"'
CPPFLAGS+=-DZIC_BLOAT_DEFAULT='"slim"'

.PATH: ${NETBSDSRCDIR}/lib/libc/time

.if !defined(HOSTPROG)
LDADD+=-lintl -lutil
DPADD+=${LIBINTL} ${LIBUTIL}
.endif

foo:
	echo ${TZVERSION}

.include <bsd.prog.mk>
