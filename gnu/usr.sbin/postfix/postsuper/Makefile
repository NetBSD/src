#	$NetBSD: Makefile,v 1.3 2001/12/12 01:49:11 tv Exp $

PROG=	postsuper

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_SBINDIR}

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=# defined

.include <bsd.prog.mk>
