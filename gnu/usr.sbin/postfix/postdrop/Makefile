#	$NetBSD: Makefile,v 1.4 2001/12/12 01:49:10 tv Exp $

PROG=	postdrop

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_SBINDIR}
BINGRP=	maildrop
BINMODE=2555

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=# defined

.include <bsd.prog.mk>
