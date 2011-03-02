#	$NetBSD: Makefile,v 1.1 2011/03/02 22:24:55 tron Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	dnsblog

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}

.include <bsd.prog.mk>
