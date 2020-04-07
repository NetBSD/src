#	$NetBSD: Makefile,v 1.1 2011/03/02 22:24:56 tron Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	tlsproxy

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

SRCS=   tlsproxy_state.c tlsproxy.c

DPADD+= ${LIBPTLS} ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPTLS} ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

DPADD+= ${LIBSSL} ${LIBCRYPTO}
LDADD+= -lssl -lcrypto

.include <bsd.prog.mk>
