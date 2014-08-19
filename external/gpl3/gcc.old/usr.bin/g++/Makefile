#	$NetBSD: Makefile,v 1.1.1.1.8.2 2014/08/19 23:58:23 tls Exp $

PROG=		gplusplus
PROGNAME=	g++
SRCS=		g++spec.c

NOMAN=		# defined

.include <bsd.own.mk>

CPPFLAGS+=	-I${BACKENDOBJ}

LINKS+=		${BINDIR}/g++ ${BINDIR}/c++

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc ${DIST}/gcc/cp

LDADD+=		-lintl
DPADD+=		${LIBINTL}
