#	$NetBSD: Makefile,v 1.1 2003/07/25 16:32:35 mrg Exp $

.include <bsd.own.mk>

PROG=           g++
SRCS=		g++spec.c

CPPFLAGS+=	-I${BACKENDOBJ}

LINKS+=		${BINDIR}/g++ ${BINDIR}/c++

NOMAN=1
#MAN=		g++.1
#MLINKS=		g++.1 c++.1

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc ${DIST}/gcc/cp
