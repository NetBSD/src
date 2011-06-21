#	$NetBSD: Makefile,v 1.1.1.1 2011/06/21 02:48:27 mrg Exp $

PROG=		gplusplus
PROGNAME=	g++
SRCS=		g++spec.c

NOMAN=		# defined

.include <bsd.own.mk>

CPPFLAGS+=	-I${BACKENDOBJ}

LINKS+=		${BINDIR}/g++ ${BINDIR}/c++

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc ${DIST}/gcc/cp
