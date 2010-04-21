#	$NetBSD: Makefile,v 1.1.4.2 2010/04/21 05:24:30 matt Exp $

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
