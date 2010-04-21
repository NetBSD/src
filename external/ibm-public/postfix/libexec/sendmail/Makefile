#	$NetBSD: Makefile,v 1.1.4.2 2010/04/21 05:24:29 matt Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	sendmail

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
