#	$NetBSD: Makefile,v 1.3 2001/12/12 01:49:10 tv Exp $

PROG=	postfix

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_SBINDIR}

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=# defined

.include <bsd.prog.mk>
