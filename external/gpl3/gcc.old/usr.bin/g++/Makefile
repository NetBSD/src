#	$NetBSD: Makefile,v 1.1.1.1.4.2 2014/05/22 15:58:51 yamt Exp $

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
