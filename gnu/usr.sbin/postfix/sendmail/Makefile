#	$NetBSD: Makefile,v 1.3 2001/12/12 01:49:12 tv Exp $

PROG=	sendmail

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=# defined

.include <bsd.prog.mk>
