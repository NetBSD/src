#	$NetBSD: Makefile,v 1.3 2001/12/12 01:49:13 tv Exp $

PROG=	trivial-rewrite

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR= ${PFIX_LIBEXECDIR}

SRCS=	trivial-rewrite.c rewrite.c resolve.c transport.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=# defined

.include <bsd.prog.mk>
