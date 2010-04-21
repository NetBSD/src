#	$NetBSD: Makefile,v 1.1.4.2 2010/04/21 05:24:30 matt Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	trivial-rewrite

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

SRCS=	trivial-rewrite.c rewrite.c resolve.c transport.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
