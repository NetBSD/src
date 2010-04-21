#	$NetBSD: Makefile,v 1.1.4.2 2010/04/21 05:24:31 matt Exp $

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
