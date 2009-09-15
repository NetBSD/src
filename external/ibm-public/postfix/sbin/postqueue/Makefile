#	$NetBSD: Makefile,v 1.1.2.2 2009/09/15 06:04:19 snj Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	postqueue

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

BINGRP=	maildrop
BINMODE=2555

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
