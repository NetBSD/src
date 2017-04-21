#	$NetBSD: Makefile,v 1.1.36.1 2017/04/21 16:52:54 bouyer Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	postqueue
SRCS=	postqueue.c showq_compat.c showq_json.c

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

BINGRP=	maildrop
BINMODE=2555

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
