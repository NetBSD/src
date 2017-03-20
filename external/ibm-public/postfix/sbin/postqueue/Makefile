#	$NetBSD: Makefile,v 1.1.32.1 2017/03/20 06:56:42 pgoyette Exp $

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
