#	$NetBSD: Makefile,v 1.1.2.2 2011/03/05 15:09:07 bouyer Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	dnsblog

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}

.include <bsd.prog.mk>
