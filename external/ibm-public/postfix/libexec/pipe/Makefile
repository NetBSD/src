#	$NetBSD: Makefile,v 1.1 2009/06/25 18:21:55 tron Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	pipe

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
