#	from: @{#}Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.4 1998/08/16 23:24:27 tv Exp $

PROG=		g++
SRCS=		gcc.c g++spec.c prefix.c version.c choose-temp.c pexecute.c \
		mkstemp.c obstack.c
BINDIR=		/usr/bin
CPPFLAGS+=	-DLANG_SPECIFIC_DRIVER

LINKS+=		${BINDIR}/g++ ${BINDIR}/c++

MAN=		g++.1
MLINKS=		g++.1 c++.1

.include <bsd.prog.mk>

.PATH:		${DIST}/cp
CPPFLAGS+=	-I${DIST}/cp
