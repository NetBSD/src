#	$NetBSD: Makefile,v 1.1 2009/06/25 18:21:56 tron Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	verify

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

SRCS=	verify.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
