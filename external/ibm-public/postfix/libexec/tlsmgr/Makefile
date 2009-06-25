#	$NetBSD: Makefile,v 1.1 2009/06/25 18:21:56 tron Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	tlsmgr

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPTLS} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPTLS} ${LIBPUTIL}

DPADD+= ${LIBSSL} ${LIBCRYPTO}
LDADD+= -lssl -lcrypto

.include <bsd.prog.mk>
