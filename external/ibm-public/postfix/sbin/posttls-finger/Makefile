#	$NetBSD: Makefile,v 1.2 2014/07/08 09:58:12 martin Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	posttls-finger
SRCS=	posttls-finger.c tlsmgrmem.c

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

DPADD+= ${LIBPDNS} ${LIBPTLS} ${LIBPGLOBAL} ${LIBPUTIL} ${LIBCRYPTO}
LDADD+= ${LIBPDNS} ${LIBPTLS} ${LIBPGLOBAL} ${LIBPUTIL} ${LIBCRYPTO}

.include <bsd.prog.mk>
