#	$NetBSD: Makefile,v 1.1 2009/06/23 15:02:56 tron Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	trivial-rewrite

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

BINDIR= ${PFIX_LIBEXECDIR}

SRCS=	trivial-rewrite.c rewrite.c resolve.c transport.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
