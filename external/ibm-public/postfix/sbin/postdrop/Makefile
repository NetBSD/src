#	$NetBSD: Makefile,v 1.1 2009/06/25 18:21:59 tron Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	postdrop

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

BINGRP=	maildrop
BINMODE=2555

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
