#	from: @{#}Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.3 1998/04/03 02:20:30 tv Exp $

PROG=	g++
SRCS=	gcc.c obstack.c version.c g++spec.c choose-temp.c pexecute.c
BINDIR=	/usr/bin
CPPFLAGS+= -DGCC_NAME=\"/usr/bin/cc\" -DLANG_SPECIFIC_DRIVER

LINKS+=	${BINDIR}/g++ ${BINDIR}/c++

MAN=	g++.1
MLINKS=	g++.1 c++.1

.include <bsd.prog.mk>

.PATH: ${DIST}/cp
CPPFLAGS+= -I${DIST}/cp
