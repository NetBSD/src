#	$NetBSD: Makefile,v 1.2 2002/02/04 02:39:06 perry Exp $

PROG=	postqueue

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_SBINDIR}
BINGRP=	maildrop
BINMODE=2555

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=# defined

.include <bsd.prog.mk>
