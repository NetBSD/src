#	$NetBSD: Makefile,v 1.1 2000/04/30 21:22:07 perry Exp $

PROG=	postdrop

DIST=	${.CURDIR}/../../../dist/postfix/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_SBINDIR}
BINGRP=	maildrop
BINMODE=2755

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=1

.include <bsd.prog.mk>
