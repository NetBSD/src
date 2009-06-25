#	$NetBSD: Makefile,v 1.1 2009/06/25 18:22:00 tron Exp $

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
