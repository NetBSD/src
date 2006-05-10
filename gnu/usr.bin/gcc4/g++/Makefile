#	$NetBSD: Makefile,v 1.1 2006/05/10 22:48:13 mrg Exp $

PROG=           g++
SRCS=		g++spec.c

NOMAN=		# defined

.include <bsd.own.mk>

CPPFLAGS+=	-I${BACKENDOBJ}

LINKS+=		${BINDIR}/g++ ${BINDIR}/c++

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc ${DIST}/gcc/cp
