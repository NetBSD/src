#	$NetBSD: Makefile,v 1.3 2014/07/08 13:04:07 christos Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	posttls-finger
SRCS=	posttls-finger.c tlsmgrmem.c

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

DPADD+= ${LIBPDNS} ${LIBPTLS} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPDNS} ${LIBPTLS} ${LIBPGLOBAL} ${LIBPUTIL}

.if ${MKCRYPTO} != "no"
DPADD+= ${LIBCRYPTO}
LDADD+= -lcrypto
.endif

.include <bsd.prog.mk>
