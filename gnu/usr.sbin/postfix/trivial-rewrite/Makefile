#	$NetBSD: Makefile,v 1.1 2000/04/30 21:22:18 perry Exp $

PROG=	trivial-rewrite

DIST=	${.CURDIR}/../../../dist/postfix/${PROG}
.PATH:	${DIST}

BINDIR= ${PFIX_LIBEXECDIR}

SRCS=	trivial-rewrite.c rewrite.c resolve.c transport.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=1

.include <bsd.prog.mk>
