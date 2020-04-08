#	$NetBSD: Makefile,v 1.1.46.1 2020/04/08 14:07:01 martin Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	tlsproxy

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

SRCS=   tlsproxy_state.c tlsproxy.c

DPADD+= ${LIBPTLS} ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}
LDADD+= ${LIBPTLS} ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}

DPADD+= ${LIBSSL} ${LIBCRYPTO}
LDADD+= -lssl -lcrypto

.include <bsd.prog.mk>
