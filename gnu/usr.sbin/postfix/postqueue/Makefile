#	$NetBSD: Makefile,v 1.3 2002/09/19 03:09:50 lukem Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	postqueue

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_SBINDIR}
BINGRP=	maildrop
BINMODE=2555

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
