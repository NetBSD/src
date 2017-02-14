#	$NetBSD: Makefile,v 1.2 2017/02/14 01:16:50 christos Exp $

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
