#	$NetBSD: Makefile,v 1.2 2003/08/04 06:58:20 mrg Exp $

.include <bsd.own.mk>

PROG=           g++
SRCS=		g++spec.c

CPPFLAGS+=	-I${BACKENDOBJ}

LINKS+=		${BINDIR}/g++ ${BINDIR}/c++

NOMAN=		# defined

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc ${DIST}/gcc/cp
